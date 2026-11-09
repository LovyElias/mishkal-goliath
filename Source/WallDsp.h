#pragma once
// Mishkal Wall - the sound. Every stage is its own small class; WallProcessor runs them in the chain's order.
// Real-time safe: everything is allocated in prepare(), nothing in process().
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <atomic>

namespace wall
{
inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float coefMs (float ms, double fs) { return ms <= 0.0f ? 0.0f : (float) std::exp (-1.0 / (0.001 * ms * fs)); }

// ---------------------------------------------------------------------------------------------------------------
// a biquad (transposed direct form II, double)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void setHighPass (double fs, double f, double q = 0.70710678)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q), a0 = 1.0 + al;
        b0 = (1.0 + c) / 2.0 / a0; b1 = -(1.0 + c) / a0; b2 = b0; a1 = -2.0 * c / a0; a2 = (1.0 - al) / a0;
    }
    void setLowPass (double fs, double f, double q = 0.70710678)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q), a0 = 1.0 + al;
        b0 = (1.0 - c) / 2.0 / a0; b1 = (1.0 - c) / a0; b2 = b0; a1 = -2.0 * c / a0; a2 = (1.0 - al) / a0;
    }
    void setHighShelf (double fs, double f, double gainDb, double q = 0.70710678)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / fs, c = std::cos (w), s = std::sin (w), al = s / (2.0 * q), sq = 2.0 * std::sqrt (A) * al;
        const double a0 = (A + 1) - (A - 1) * c + sq;
        b0 = A * ((A + 1) + (A - 1) * c + sq) / a0; b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0; b2 = A * ((A + 1) + (A - 1) * c - sq) / a0;
        a1 = 2 * ((A - 1) - (A + 1) * c) / a0; a2 = ((A + 1) - (A - 1) * c - sq) / a0;
    }
};

// K-weighting (ITU-R BS.1770) at any sample rate: a high shelf (+4 dB above ~1.7 kHz) then a high-pass (~38 Hz)
struct KWeight
{
    Biquad shelf, hp;
    void prepare (double fs)
    {
        // the standard's shelf and RLB filters, re-designed for fs (Brecht De Man's derivation)
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            shelf.b0 = (Vh + Vb * K / Q + K * K) / a0; shelf.b1 = 2.0 * (K * K - Vh) / a0; shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
            shelf.a1 = 2.0 * (K * K - 1.0) / a0; shelf.a2 = (1.0 - K / Q + K * K) / a0;
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773, K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            hp.b0 = 1.0 / a0; hp.b1 = -2.0 / a0; hp.b2 = 1.0 / a0; hp.a1 = 2.0 * (K * K - 1.0) / a0; hp.a2 = (1.0 - K / Q + K * K) / a0;
        }
        reset();
    }
    void reset() { shelf.reset(); hp.reset(); }
    float process (float x) { return hp.process (shelf.process (x)); }
};

// ---------------------------------------------------------------------------------------------------------------
// sliding minimum over the last W values (monotonic queue, fixed memory)
struct SlidingMin
{
    std::vector<float> v; std::vector<int> idx; int W = 1, head = 0, tail = 0, n = 0, cap = 1;
    void prepare (int maxW) { cap = maxW + 2; v.assign ((size_t) cap, 1.0f); idx.assign ((size_t) cap, 0); setWindow (1); }
    void setWindow (int w) { W = std::max (1, std::min (w, cap - 2)); head = tail = 0; n = 0; }
    float push (float x)
    {
        // drop larger values from the back
        while (tail != head) { const int b = (tail - 1 + cap) % cap; if (v[(size_t) b] >= x) tail = b; else break; }
        v[(size_t) tail] = x; idx[(size_t) tail] = n; tail = (tail + 1) % cap;
        while (idx[(size_t) head] <= n - W) head = (head + 1) % cap;   // out of the window
        ++n;
        if (n > (1 << 30)) { const int sh = n - W - 1; for (int k = head; k != tail; k = (k + 1) % cap) idx[(size_t) k] -= sh; n -= sh; }
        return v[(size_t) head];
    }
};
// running mean over the last W values
struct RunningMean
{
    std::vector<float> ring; int W = 1, pos = 0, cnt = 0; double sum = 0;
    void prepare (int maxW) { ring.assign ((size_t) std::max (1, maxW), 1.0f); setWindow (1, 1.0f); }
    void setWindow (int w, float fill) { W = std::max (1, std::min (w, (int) ring.size())); std::fill (ring.begin(), ring.begin() + W, fill); sum = (double) fill * W; pos = 0; cnt = 0; }
    float push (float x)
    {
        sum += (double) x - ring[(size_t) pos]; ring[(size_t) pos] = x; pos = (pos + 1) % W;
        if (++cnt >= 4096) { cnt = 0; sum = 0; for (int k = 0; k < W; ++k) sum += ring[(size_t) k]; }
        return (float) (sum / W);
    }
};
// a delay line
struct Delay
{
    std::vector<float> buf; int pos = 0, d = 0;
    void prepare (int maxD) { buf.assign ((size_t) std::max (1, maxD + 1), 0.0f); pos = 0; d = 0; }
    void setDelay (int samples) { d = std::max (0, std::min (samples, (int) buf.size() - 1)); }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    float process (float x)
    {
        buf[(size_t) pos] = x;
        const int r = (pos - d + (int) buf.size()) % (int) buf.size();
        pos = (pos + 1) % (int) buf.size();
        return buf[(size_t) r];
    }
};

// ---------------------------------------------------------------------------------------------------------------
// DC filter (5 Hz) and mono bass (the side's lows removed below a frequency)
struct DcFilter
{
    float x1[2] {}, y1[2] {}, R = 0.9995f;
    void prepare (double fs) { R = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * 5.0 / fs); x1[0] = x1[1] = y1[0] = y1[1] = 0; }
    float process (int c, float x) { const float y = x - x1[c] + R * y1[c]; x1[c] = x; y1[c] = y; return y; }
};
struct MonoBass
{
    Biquad hp; double fs = 48000; float was = -1;
    void prepare (double f) { fs = f; was = -1; hp.reset(); }
    void process (float& l, float& r, float hz)
    {
        if (hz != was) { hp.setHighPass (fs, hz, 0.70710678); was = hz; }
        const float m = 0.5f * (l + r), s = hp.process (0.5f * (l - r));
        l = m + s; r = m - s;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// PUNCH: transients (fast follower vs slow follower) + upward compression (quiet parts lifted)
struct PunchSettings { float transients = 0.4f; float upwardDb = 2.0f, thresholdDb = -30.0f; int speed = 1; int mode = 0; bool upwardOn = true; };
struct Punch
{
    double fs = 48000;
    float fast[2] {}, slow[2] {}, rms[2] {}, lift[2] {};
    void prepare (double f) { fs = f; reset(); }
    void reset() { for (int c = 0; c < 2; ++c) fast[c] = slow[c] = rms[c] = lift[c] = 0.0f; }
    // mode: 0 stereo, 1 mid only, 2 side only, 3 mid and side each on their own
    void process (float* l, float* r, int n, const PunchSettings& s)
    {
        const float sp = s.speed == 0 ? 2.0f : s.speed == 2 ? 0.5f : 1.0f;
        const float fA = coefMs (0.5f, fs), fR = coefMs (30.0f * sp, fs), sA = coefMs (12.0f * sp, fs), sR = coefMs (120.0f * sp, fs);
        const float rA = coefMs (10.0f, fs), rR = coefMs (100.0f * sp, fs), upRise = coefMs (350.0f * sp, fs), upFall = coefMs (25.0f, fs);
        const bool ms = s.mode != 0;
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            if (ms) { const float m = 0.5f * (a + b), sd = 0.5f * (a - b); a = m; b = sd; }
            float* ch[2] { &a, &b };
            const bool linked = s.mode == 0;
            float lvlLinked = std::max (std::abs (a), std::abs (b));
            for (int c = 0; c < 2; ++c)
            {
                if (s.mode == 1 && c == 1) continue;   // mid only
                if (s.mode == 2 && c == 0) continue;   // side only
                const float x = linked ? lvlLinked : std::abs (*ch[c]);
                fast[c] += (x > fast[c] ? 1.0f - fA : 1.0f - fR) * (x - fast[c]);
                slow[c] += (x > slow[c] ? 1.0f - sA : 1.0f - sR) * (x - slow[c]);
                float gDb = 0.0f;
                if (std::abs (s.transients) > 0.001f)
                {
                    const float t = juce::jlimit (0.0f, 12.0f, gainToDb ((fast[c] + 1.0e-5f) / (slow[c] + 1.0e-5f)));
                    gDb += t * 0.6f * s.transients * 1.25f;   // +100% = up to ~+9 dB on the sharpest hits
                }
                if (s.upwardOn && s.upwardDb > 0.01f)
                {
                    rms[c] += (x * x > rms[c] ? 1.0f - rA : 1.0f - rR) * (x * x - rms[c]);
                    const float lv = 10.0f * std::log10 (rms[c] + 1.0e-12f);
                    float want = juce::jlimit (0.0f, 1.0f, (s.thresholdDb - lv) / 12.0f) * s.upwardDb;
                    want *= juce::jlimit (0.0f, 1.0f, (lv + 72.0f) / 12.0f);          // never lift silence or hiss
                    lift[c] += (want > lift[c] ? 1.0f - upRise : 1.0f - upFall) * (want - lift[c]);
                    gDb += lift[c];
                }
                *ch[c] *= dbToGain (gDb);
                if (linked) { b *= dbToGain (gDb); break; }
            }
            if (ms) { const float m = a, sd = b; a = m + sd; b = m - sd; }
            l[i] = a; r[i] = b;
        }
    }
};

// ---------------------------------------------------------------------------------------------------------------
// CHARACTER: density, harmonics (even / odd), enhance; four flavours
struct CharacterSettings { float amount = 4.2f, density = 0.35f, evenOdd = -0.3f, enhance = 1.5f; int flavour = 2; };
struct Character
{
    double fs = 48000;
    std::array<Biquad, 2> tilt, presHp; float lowLp[2] {}; float lpA = 0.0f;
    int flavWas = -1;
    void prepare (double f) { fs = f; flavWas = -1; for (int c = 0; c < 2; ++c) { tilt[(size_t) c].reset(); presHp[(size_t) c].reset(); lowLp[c] = 0; } lpA = 1.0f - coefMs (1000.0f / (2.0f * juce::MathConstants<float>::pi * 120.0f), fs); for (auto& p : presHp) p.setHighPass (fs, 2500.0); }
    static float shape (int flav, float x)
    {
        switch (flav)
        {
            case 0:  return std::tanh (x);                                              // Clean
            case 1:  { const float y = std::tanh (1.1f * x); return y - 0.06f * y * y * y; }   // Tape: round, soft top
            case 3:  return std::tanh (1.6f * x) / 1.4f;                                // Console: tighter, more odd
            default: return x >= 0.0f ? std::tanh (x) : std::tanh (1.25f * x) / 1.25f;  // Tube: a little uneven = warm
        }
    }
    void process (float* l, float* r, int n, const CharacterSettings& s)
    {
        if (s.flavour != flavWas)
        {
            for (auto& t : tilt) { if (s.flavour == 1) t.setHighShelf (fs, 9000.0, -1.2); else if (s.flavour == 3) t.setHighShelf (fs, 5000.0, 0.6); else if (s.flavour == 2) t.setHighShelf (fs, 7000.0, -0.4); else t.setHighShelf (fs, 8000.0, 0.0); t.reset(); }
            flavWas = s.flavour;
        }
        const float drive = 0.15f + s.amount * 0.22f;                       // CHARACTER 0..10
        const float bias = s.evenOdd < 0.0f ? -s.evenOdd * 0.22f : 0.0f;    // towards EVEN: a small offset = 2nd / 4th harmonics
        const float oddMul = 1.0f + std::max (0.0f, s.evenOdd) * 0.8f;      // towards ODD: a harder curve
        const float wet = juce::jlimit (0.0f, 1.0f, 0.25f + 0.75f * s.density) * juce::jlimit (0.0f, 1.0f, s.amount / 2.0f);
        const float f0 = shape (s.flavour, bias * drive * oddMul);
        const float norm = 1.0f / std::max (0.05f, (shape (s.flavour, (0.01f + bias) * drive * oddMul) - f0) / 0.01f);   // small-signal gain = 1
        float* ch[2] { l, r };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                float y = (shape (s.flavour, (x + bias) * drive * oddMul) - f0) * norm;
                y = tilt[(size_t) c].process (y);
                if (s.enhance > 0.01f)
                {
                    const float p = presHp[(size_t) c].process (x);
                    lowLp[c] += lpA * (x - lowLp[c]);
                    y += s.enhance * (0.06f * std::tanh (3.0f * p) + 0.04f * std::tanh (2.0f * lowLp[c]));
                }
                ch[c][i] = x + wet * (y - x);
            }
    }
};

// ---------------------------------------------------------------------------------------------------------------
// BANDS (MLA style): 2..4 bands on gentle 1st-order splits that add back to the input exactly
struct BandSettings { float ratio = 0.0f, upwardDb = 0.0f, limitDb = 0.0f, gainDb = 0.0f; bool solo = false, bypass = false; };
struct BandsSettings
{
    int count = 2; std::array<float, 3> xover { 160.0f, 3000.0f, 9000.0f };
    std::array<BandSettings, 4> band {}; float masterThrDb = -18.0f; int linkMode = 1; bool scBoost = false;
};
struct Bands
{
    double fs = 48000;
    float lp[3][2] {};
    float env[4] {}, pk[4] {}, gDb[4] {};
    std::array<std::atomic<float>, 4> meterDb {};   // per band: + = expanding / lifting, - = compressing (for the meters)
    void prepare (double f) { fs = f; reset(); }
    void reset() { for (auto& a : lp) a[0] = a[1] = 0; for (int b = 0; b < 4; ++b) { env[b] = pk[b] = gDb[b] = 0; meterDb[(size_t) b] = 0; } }
    void process (float* l, float* r, int n, const BandsSettings& s)
    {
        const int nb = juce::jlimit (2, 4, s.count);
        float a[3]; for (int k = 0; k < nb - 1; ++k) a[k] = 1.0f - (float) std::exp (-2.0 * juce::MathConstants<double>::pi * s.xover[(size_t) k] / fs);
        const float eA = coefMs (10.0f, fs), eR = coefMs (150.0f, fs), pA = coefMs (0.5f, fs), pR = coefMs (60.0f, fs);
        const float gA = coefMs (8.0f, fs), gR = coefMs (150.0f, fs);
        bool anySolo = false; for (int b = 0; b < nb; ++b) anySolo |= s.band[(size_t) b].solo;
        for (int i = 0; i < n; ++i)
        {
            float bl[4] {}, br[4] {};
            float restL = l[i], restR = r[i];
            for (int k = 0; k < nb - 1; ++k)
            {
                lp[k][0] += a[k] * (restL - lp[k][0]); lp[k][1] += a[k] * (restR - lp[k][1]);
                bl[k] = lp[k][0]; br[k] = lp[k][1]; restL -= bl[k]; restR -= br[k];
            }
            bl[nb - 1] = restL; br[nb - 1] = restR;
            float grLow = 0.0f;
            float outL = 0.0f, outR = 0.0f;
            for (int b = 0; b < nb; ++b)
            {
                const auto& bs = s.band[(size_t) b];
                float x = std::max (std::abs (bl[b]), std::abs (br[b]));
                if (s.scBoost && b == nb - 1) x *= 2.0f;   // S/C boost: the top band reacts more
                env[b] += (x * x > env[b] ? 1.0f - eA : 1.0f - eR) * (x * x - env[b]);
                pk[b] += (x > pk[b] ? 1.0f - pA : 1.0f - pR) * (x - pk[b]);
                const float lv = 10.0f * std::log10 (env[b] + 1.0e-12f), thr = s.masterThrDb;
                float want = 0.0f;
                if (! bs.bypass)
                {
                    const float over = lv - thr;
                    if (bs.ratio < -0.01f)       // compress (the left side of the MLA's knob)
                    {
                        const float ratio = 1.4f + (-bs.ratio) * 4.6f, knee = 6.0f;
                        const float o = over < -knee * 0.5f ? 0.0f : over > knee * 0.5f ? over : (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
                        want -= o * (1.0f - 1.0f / ratio);
                    }
                    else if (bs.ratio > 0.01f)   // expand (the right side): above the threshold it opens up
                        want += juce::jlimit (0.0f, 6.0f, std::max (0.0f, over) * (0.2f + bs.ratio * 0.8f) * 0.5f);
                    if (bs.upwardDb > 0.01f)
                        want += juce::jlimit (0.0f, 1.0f, (thr - 6.0f - lv) / 12.0f) * bs.upwardDb * juce::jlimit (0.0f, 1.0f, (lv + 72.0f) / 12.0f);
                    if (bs.limitDb > 0.01f)
                    {
                        const float pdb = gainToDb (pk[b]), lim = thr + 6.0f;
                        want -= juce::jlimit (0.0f, bs.limitDb, std::max (0.0f, pdb - lim) * 0.875f);
                    }
                }
                if (b == 0) grLow = std::min (0.0f, want);
                if (b > 0 && grLow < 0.0f)
                {
                    const bool follow = s.linkMode == 4 || (s.linkMode == 1 && b == 1) || (s.linkMode == 2 && b == nb - 1) || (s.linkMode == 3);
                    if (follow) want += grLow * 0.5f;   // the low band leads: the rest move with the kick, half as much
                }
                gDb[b] += (want < gDb[b] ? 1.0f - gA : 1.0f - gR) * (want - gDb[b]);
                const float g = dbToGain (gDb[b] + (bs.bypass ? 0.0f : bs.gainDb));
                if (! anySolo || bs.solo) { outL += bl[b] * g; outR += br[b] * g; }
            }
            l[i] = outL; r[i] = outR;
        }
        for (int b = 0; b < 4; ++b) meterDb[(size_t) b] = b < nb ? gDb[b] : 0.0f;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// LEVELER: a slow rider towards a target (K-weighted level), within a range; holds still below Freeze
struct LevelerSettings { float targetDb = -12.0f, rangeDb = 6.0f, freezeDb = -40.0f; int speed = 0; };
struct Leveler
{
    double fs = 48000; KWeight kw[2]; float e = 0.0f, gain = 0.0f; std::atomic<float> gainNow { 0.0f };
    void prepare (double f) { fs = f; kw[0].prepare (fs); kw[1].prepare (fs); e = 0; gain = 0; }
    void process (float* l, float* r, int n, const LevelerSettings& s)
    {
        const float tc = s.speed == 0 ? 8000.0f : s.speed == 1 ? 4000.0f : 2000.0f;
        const float eC = coefMs (tc * 0.4f, fs);
        const float slew = (float) (3.0 / fs) * (s.speed == 2 ? 2.0f : 1.0f);   // dB per sample (3 dB a second)
        for (int i = 0; i < n; ++i)
        {
            const float a = kw[0].process (l[i]), b = kw[1].process (r[i]);
            e = eC * e + (1.0f - eC) * 0.5f * (a * a + b * b);
            if ((i & 15) == 0)
            {
                const float lv = 10.0f * std::log10 (e + 1.0e-12f) - 0.691f + 3.0f;
                if (lv > s.freezeDb)
                {
                    const float want = juce::jlimit (-s.rangeDb, s.rangeDb, s.targetDb - lv);
                    gain += juce::jlimit (-slew * 16.0f, slew * 16.0f, want - gain);
                }
            }
            const float g = dbToGain (gain);
            l[i] *= g; r[i] *= g;
        }
        gainNow = gain;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// GOLD: a converter-style clip - driven into a smooth, round ceiling
struct Gold
{
    static float curve (float x)
    {
        const float a = std::abs (x);
        if (a <= 0.5f) return x;
        const float y = 0.5f + 0.5f * std::tanh ((a - 0.5f) * 2.0f);   // rounds into 1.0, never past it
        return x < 0.0f ? -y : y;
    }
    std::atomic<float> hot { 0.0f };   // how often it's near the top (for its light)
    void process (float* l, float* r, int n, float driveDb)
    {
        if (driveDb <= 0.0f) { hot = 0.0f; return; }
        const float g = dbToGain (driveDb);
        int near = 0;
        for (int i = 0; i < n; ++i)
        {
            l[i] = curve (l[i] * g); r[i] = curve (r[i] * g);
            near += (std::abs (l[i]) > 0.9f || std::abs (r[i]) > 0.9f) ? 1 : 0;
        }
        hot = (float) near / (float) std::max (1, n);
    }
};

// ---------------------------------------------------------------------------------------------------------------
// CLIPPER: shaves up to Amount dB off the fastest peaks; hard / soft / analog / tube
struct ClipperSettings { float amountDb = 2.0f, knee = 0.4f, ceilingDb = -0.3f; int curve = 2; };
struct Clipper
{
    std::atomic<float> shaveDb { 0.0f };
    // the scope on the clipper's panel: recent (in, out) pairs
    static constexpr int scopeN = 512; std::array<std::atomic<float>, scopeN * 2> scope {}; std::atomic<int> scopePos { 0 }; int scopeTick = 0;
    // every curve is exactly 1:1 below its knee and never goes past 1.0
    static float clip (float x, int curve, float knee)
    {
        const float a = std::abs (x), sg = x < 0.0f ? -1.0f : 1.0f;
        const float k = juce::jlimit (0.02f, 0.9f, knee);
        auto tanhKnee = [] (float v, float w) { const float t = 1.0f - w; return v <= t ? v : t + w * std::tanh ((v - t) / w); };
        switch (curve)
        {
            case 0: return juce::jlimit (-1.0f, 1.0f, x);                                                  // hard
            case 1: { const float t = 1.0f - k; if (a <= t) return x; const float u = (a - t) / k;        // soft knee
                      return sg * (u >= 2.0f ? 1.0f : t + k * (u - u * u * 0.25f)); }
            case 3: return sg * tanhKnee (a, x >= 0.0f ? k : std::min (0.95f, k * 1.4f));                 // tube: the two halves bend differently
            default: return sg * tanhKnee (a, std::min (0.95f, k * 1.15f));                               // analog
        }
    }
    void process (float* l, float* r, int n, const ClipperSettings& s, int scopeEvery)
    {
        const float lvl = dbToGain (s.ceilingDb), inv = 1.0f / lvl, floorG = 1.0f / dbToGain (std::max (0.0f, s.amountDb));
        float worst = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float a = l[i] * inv, b = r[i] * inv;
            float ya = clip (a, s.curve, s.knee), yb = clip (b, s.curve, s.knee);
            // it takes at most Amount dB off a peak - whatever is left over goes on to the limiter
            if (std::abs (ya) < std::abs (a) * floorG) ya = a * floorG;
            if (std::abs (yb) < std::abs (b) * floorG) yb = b * floorG;
            const float pa = std::max (std::abs (a), std::abs (b)), py = std::max (std::abs (ya), std::abs (yb));
            if (pa > py + 1.0e-7f) worst = std::max (worst, gainToDb (pa / std::max (1.0e-6f, py)));
            if (++scopeTick >= scopeEvery)
            {
                scopeTick = 0; const int p = scopePos.load();
                scope[(size_t) (p * 2)] = juce::jlimit (-2.0f, 2.0f, a); scope[(size_t) (p * 2 + 1)] = juce::jlimit (-2.0f, 2.0f, ya);
                scopePos = (p + 1) % scopeN;
            }
            l[i] = ya * lvl; r[i] = yb * lvl;
        }
        shaveDb = std::max (worst, shaveDb.load() * 0.9f);
    }
};

// ---------------------------------------------------------------------------------------------------------------
// LIMITER: look-ahead, up to three stages (slow body, medium, fast peaks), ARC release, separate stereo links
struct LimiterStyle { const char* name; const char* blurb; float attackMs, releaseMs, slowShare, slowRelMs; int stages; };
inline const std::array<LimiterStyle, 9>& styles()
{
    static const std::array<LimiterStyle, 9> s {{
        { "GLASS",   "clean, invisible",              3.0f, 120.0f, 0.35f, 400.0f, 2 },
        { "HAMMER",  "drums keep their hit",          0.6f,  40.0f, 0.20f, 200.0f, 2 },
        { "RUACH",   "breathes, keeps life",          2.0f, 250.0f, 0.50f, 800.0f, 2 },
        { "STONE",   "solid on anything",             1.5f,  80.0f, 0.30f, 300.0f, 2 },
        { "JERICHO", "loud and forward",              0.5f,  25.0f, 0.15f, 120.0f, 2 },
        { "TOWER",   "multi-stage, tall & clean",     1.2f,  60.0f, 0.45f, 250.0f, 3 },
        { "MORTAR",  "glue for groups",               4.0f, 200.0f, 0.60f, 600.0f, 2 },
        { "REFUGE",  "never pumps",                   5.0f, 400.0f, 0.50f, 1000.0f, 2 },
        { "GOLIATH", "big low end, punch, wide top",  1.0f,  50.0f, 0.40f, 300.0f, 3 } }};
    return s;
}
struct LimiterSettings { int style = 5; float attackMs = 1.2f, releaseMs = 60.0f, lookaheadMs = 2.0f, linkT = 1.0f, linkR = 0.6f; int stages = 3; bool arc = true; float ceilingDb = -1.0f; };
struct Limiter
{
    double fs = 48000; int maxL = 1, L = 1, A = 1;
    SlidingMin minLinked, minOwn[2]; RunningMean meanLinked, meanOwn[2];
    std::array<Delay, 2> delay;
    float gLinked = 1.0f, gOwn[2] { 1.0f, 1.0f };
    float slowEnv = 0.0f, slowGrDb = 0.0f, midEnv = 0.0f, midGrDb = 0.0f, rmsS = 0.0f, pkS = 0.0f;
    std::atomic<float> grDb { 0.0f };
    void prepare (double f, int maxLookahead)
    {
        fs = f; maxL = std::max (1, maxLookahead);
        minLinked.prepare (maxL + 1); meanLinked.prepare (maxL + 1);
        for (int c = 0; c < 2; ++c) { minOwn[c].prepare (maxL + 1); meanOwn[c].prepare (maxL + 1); delay[(size_t) c].prepare (maxL + 1); }
        L = -1; reset();
    }
    void reset() { gLinked = gOwn[0] = gOwn[1] = 1.0f; slowEnv = slowGrDb = midEnv = midGrDb = rmsS = pkS = 0.0f; for (auto& d : delay) d.reset(); L = -1; }
    void setRate (double f) { fs = f; reset(); }
    int latency() const { return std::max (0, L); }
    void setTimes (int lookahead, int attack)
    {
        lookahead = juce::jlimit (1, maxL, lookahead); attack = juce::jlimit (1, lookahead + 1, attack);
        if (lookahead == L && attack == A) return;
        L = lookahead; A = attack;
        minLinked.setWindow (L + 1); meanLinked.setWindow (A, 1.0f);
        for (int c = 0; c < 2; ++c) { minOwn[c].setWindow (L + 1); meanOwn[c].setWindow (A, 1.0f); delay[(size_t) c].setDelay (L); }
    }
    void process (float* l, float* r, int n, const LimiterSettings& s, int lookaheadSamples, int attackSamples, float* grTrace = nullptr,
                  const float* keyL = nullptr, const float* keyR = nullptr)
    {
        setTimes (lookaheadSamples, attackSamples);
        const auto& st = styles()[(size_t) juce::jlimit (0, 8, s.style)];
        const float T = dbToGain (s.ceilingDb);
        const int stages = juce::jlimit (1, 3, s.stages);
        const float slowShare = stages >= 2 ? st.slowShare : 0.0f;
        const float sEa = coefMs (10.0f, fs), sGa = coefMs (15.0f, fs), mEa = coefMs (2.0f, fs), mGa = coefMs (3.0f, fs);
        const float dA = coefMs (300.0f, fs);
        float worst = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float x[2] { l[i], r[i] };
            // (V2: with the sidechain on, each side sees the louder of itself and the key)
            const float k0 = keyL != nullptr ? std::abs (keyL[i]) : 0.0f, k1 = keyR != nullptr ? std::abs (keyR[i]) : 0.0f;
            const float pk = std::max (std::max (std::abs (x[0]), std::abs (x[1])), std::max (k0, k1));
            // ARC: dense music (low crest) lets go slower, sparse music faster
            rmsS = dA * rmsS + (1.0f - dA) * pk * pk; pkS = std::max (pk, pkS * (1.0f - (1.0f - dA) * 0.25f));
            const float dens = juce::jlimit (0.0f, 1.0f, 2.0f * std::sqrt (rmsS) / (pkS + 1.0e-6f));
            const float arcMul = s.arc ? 0.5f + 2.0f * dens : 1.0f;
            // stage: slow (the body) - takes a share of what's over the threshold
            float pre = 1.0f;
            if (slowShare > 0.0f)
            {
                slowEnv = sEa * slowEnv + (1.0f - sEa) * pk * pk;
                const float over = std::max (0.0f, 10.0f * std::log10 (slowEnv + 1.0e-12f) - (s.ceilingDb - 6.0f));
                const float want = -std::min (12.0f, over * slowShare);
                const float rel = coefMs (st.slowRelMs * arcMul, fs);
                slowGrDb = want < slowGrDb ? sGa * slowGrDb + (1.0f - sGa) * want : rel * slowGrDb + (1.0f - rel) * want;
                pre *= dbToGain (slowGrDb);
            }
            if (stages >= 3)
            {
                midEnv = mEa * midEnv + (1.0f - mEa) * pk * pk * pre * pre;
                const float over = std::max (0.0f, 10.0f * std::log10 (midEnv + 1.0e-12f) - (s.ceilingDb - 2.0f));
                const float want = -std::min (6.0f, over * 0.3f);
                const float rel = coefMs (30.0f * arcMul, fs);
                midGrDb = want < midGrDb ? mGa * midGrDb + (1.0f - mGa) * want : rel * midGrDb + (1.0f - rel) * want;
                pre *= dbToGain (midGrDb);
            }
            x[0] *= pre; x[1] *= pre;
            const float d0x = std::max (std::abs (x[0]), k0 * pre), d1x = std::max (std::abs (x[1]), k1 * pre);
            // stage: fast look-ahead - guarantees the ceiling
            const float rq0 = std::min (1.0f, T / (d0x + 1.0e-9f)), rq1 = std::min (1.0f, T / (d1x + 1.0e-9f));
            const float relC = coefMs (s.releaseMs * arcMul, fs);
            const float sL = meanLinked.push (minLinked.push (std::min (rq0, rq1)));
            gLinked = sL < gLinked ? sL : gLinked * relC + (1.0f - relC) * sL;
            const float sO0 = meanOwn[0].push (minOwn[0].push (rq0)), sO1 = meanOwn[1].push (minOwn[1].push (rq1));
            gOwn[0] = sO0 < gOwn[0] ? sO0 : gOwn[0] * relC + (1.0f - relC) * sO0;
            gOwn[1] = sO1 < gOwn[1] ? sO1 : gOwn[1] * relC + (1.0f - relC) * sO1;
            const float lt = juce::jlimit (0.0f, 1.0f, s.linkT);
            const float lnL = std::log (std::max (1.0e-9f, gLinked));
            float g0 = std::exp (lt * lnL + (1.0f - lt) * std::log (std::max (1.0e-9f, gOwn[0])));
            float g1 = std::exp (lt * lnL + (1.0f - lt) * std::log (std::max (1.0e-9f, gOwn[1])));
            g0 = std::min (g0, gOwn[0]); g1 = std::min (g1, gOwn[1]);   // (never above the channel's own need)
            const float d0 = delay[0].process (x[0]), d1 = delay[1].process (x[1]);
            l[i] = d0 * g0; r[i] = d1 * g1;
            const float gAll = std::min (g0, g1) * pre;
            if (grTrace != nullptr) grTrace[i] = gAll;
            worst = std::min (worst, gainToDb (gAll));
        }
        grDb = -worst;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// true peak: the waveform between the samples, by 4x windowed-sinc interpolation (12 taps a phase)
struct TruePeak
{
    static constexpr int taps = 48, phases = 4, per = taps / phases, delaySamples = per / 2;
    std::array<std::array<float, per>, phases> h {};
    std::array<std::array<float, per * 2>, 2> hist {}; int pos = 0;
    TruePeak()
    {
        for (int k = 0; k < taps; ++k)
        {
            const double t = (k - (taps - 1) * 0.5) / phases, x = juce::MathConstants<double>::pi * t;
            const double sinc = std::abs (t) < 1.0e-9 ? 1.0 : std::sin (x) / x;
            const double win = 0.42 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * k / (taps - 1)) + 0.08 * std::cos (4.0 * juce::MathConstants<double>::pi * k / (taps - 1));
            h[(size_t) (k % phases)][(size_t) (k / phases)] = (float) (sinc * win);
        }
    }
    void reset() { for (auto& c : hist) c.fill (0.0f); pos = 0; }
    // push one sample per channel; returns the largest interpolated |value| around the sample `delaySamples` ago
    float push (float a, float b)
    {
        hist[0][(size_t) pos] = hist[0][(size_t) (pos + per)] = a;
        hist[1][(size_t) pos] = hist[1][(size_t) (pos + per)] = b;
        pos = (pos + 1) % per;
        float m = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int p = 0; p < phases; ++p)
            {
                float acc = 0.0f;
                const auto& hp = h[(size_t) p];
                for (int k = 0; k < per; ++k) acc += hp[(size_t) k] * hist[(size_t) c][(size_t) (pos + per - 1 - k)];
                m = std::max (m, std::abs (acc));
            }
        return m;
    }
};

// CEILING: the last safety net at the plug-in's rate - nothing over the ceiling (true peak when TP is on)
struct Ceiling
{
    static constexpr int look = 16;
    TruePeak tp; SlidingMin mn; RunningMean mean; std::array<Delay, 2> delay, align; float g = 1.0f;
    std::atomic<float> grDb { 0.0f };
    int latency() const { return look + TruePeak::delaySamples; }
    void prepare (double) { mn.prepare (look + 2); mn.setWindow (look + 1); mean.prepare (look + 2); mean.setWindow (look + 1, 1.0f); for (auto& d : delay) { d.prepare (look + TruePeak::delaySamples + 2); d.setDelay (look + TruePeak::delaySamples); } for (auto& d : align) { d.prepare (TruePeak::delaySamples + 2); d.setDelay (TruePeak::delaySamples); } tp.reset(); g = 1.0f; }
    void reset() { tp.reset(); for (auto& d : delay) d.reset(); for (auto& d : align) d.reset(); g = 1.0f; mn.setWindow (look + 1); mean.setWindow (look + 1, 1.0f); }
    void process (float* l, float* r, int n, float ceilingDb, bool truePeak, double fs)
    {
        const float T = dbToGain (ceilingDb - (truePeak ? 0.05f : 0.0f)), relC = coefMs (50.0f, fs);
        float worst = 1.0f;
        for (int i = 0; i < n; ++i)
        {
            // (the sample-peak path waits the same, so turning TP on / off doesn't change the latency)
            const float tpv = tp.push (l[i], r[i]);
            const float a0 = align[0].process (l[i]), a1 = align[1].process (r[i]);   // the sample the true peak is about
            const float d0 = delay[0].process (l[i]), d1 = delay[1].process (r[i]);
            const float level = truePeak ? std::max (tpv, std::max (std::abs (a0), std::abs (a1))) : std::max (std::abs (a0), std::abs (a1));
            const float need = std::min (1.0f, T / (level + 1.0e-9f));
            const float s = mean.push (mn.push (need));
            g = s < g ? s : g * relC + (1.0f - relC) * s;
            const float Th = dbToGain (ceilingDb);
            l[i] = juce::jlimit (-Th, Th, d0 * g); r[i] = juce::jlimit (-Th, Th, d1 * g);   // (and a hard stop, just in case)
            worst = std::min (worst, g);
        }
        grDb = -gainToDb (worst);
    }
};

// ---------------------------------------------------------------------------------------------------------------
// DITHER: TPDF to 16 .. 24 bits; shaping: 0 none (flat), 1 basic (first order), 2 optimized (Wannamaker's 5-tap, pushed up out of the ear's most sensitive range)
struct Dither
{
    juce::Random rng; float e[2][5] {};
    void process (float* l, float* r, int n, int bits, int shape = 1)
    {
        if (bits <= 0) return;
        const float q = 1.0f / (float) (1 << (bits - 1));
        static constexpr float opt[5] { 2.033f, -2.165f, 1.959f, -1.590f, 0.6149f };
        float* ch[2] { l, r };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float d = (rng.nextFloat() - rng.nextFloat()) * q;
                float fb = 0.0f;
                if (shape == 1) fb = e[c][0];
                else if (shape == 2) for (int k = 0; k < 5; ++k) fb += opt[k] * e[c][k];
                const float x = ch[c][i] - fb;
                const float y = std::round ((x + d) / q) * q;
                for (int k = 4; k > 0; --k) e[c][k] = e[c][k - 1];
                e[c][0] = juce::jlimit (-4.0f * q, 4.0f * q, y - x);
                ch[c][i] = y;
            }
    }
};
} // namespace wall
