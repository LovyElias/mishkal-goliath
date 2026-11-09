#pragma once
// Mishkal Pressure - HELPERS (LE-4): Lift > Rider > Breath > 1964, each with its own on / off.
//   LIFT    two bands (Low / High) that bring up the quiet detail: Upward (a shelf under / over the band's frequency) or
//           Dyn EQ (a bell at it). Below the band's threshold it lifts by up to Lift (a gentle 1.2:1 upward curve).
//   RIDER   rides the level toward Target (short-term loudness, ~K-weighted) within +/- Range, at Slow / Med / Fast.
//   BREATH  Goliath's breath control: airy, quiet gaps between phrases are turned down by Amount; Sense sets how quiet a
//           gap must be, Fade how gently it dips and returns.
//   1964    the Spectra 1964 610-style peak limiter: super clean - gain changes only, no saturation, no colour.
//           Input sets the threshold (dBu), Output the level, Slope the ratio, Release 0..10, Pad Off / 10 / 20.

#include <juce_dsp/juce_dsp.h>

namespace pressure
{
// V12: the six orders the three sections can run in (0 HELPERS, 1 CLEAN, 2 VINTAGE) - the "order" parameter's choices
inline constexpr int chainOrders[6][3] { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
inline int chainOrderIndex (int a, int b, int c)
{
    for (int i = 0; i < 6; ++i) if (chainOrders[i][0] == a && chainOrders[i][1] == b && chainOrders[i][2] == c) return i;
    return 0;
}
struct HelperSettings
{
    bool liftOn = true, rideOn = true, breathOn = true, peakOn = true;
    int  liftMode = 0;                                   // 0 Upward, 1 Dyn EQ
    float lowHz = 120.0f, lowThr = -30.0f, lowLift = 4.0f;
    bool lowSolo = false, highSolo = false;              // listen to just that band (as Lift shapes it)
    float highHz = 6000.0f, highThr = -34.0f, highLift = 3.0f;
    float rideRange = 6.0f, rideTarget = -16.0f; int rideSpeed = 1;   // 0 Slow, 1 Med, 2 Fast
    float breathDb = -8.0f, breathSense = 55.0f, breathFadeMs = 40.0f;
    float inDbu = -40.0f, outDbu = 4.0f, slope = 2.0f, release = 3.0f; int pad = 0;   // pad: 0 off, 1 = 10 dB, 2 = 20 dB
};

inline float liftCurve (float lvl, float thr, float lift)   // dB of lift (>= 0) for a band level
{
    const float under = thr - lvl, kn = 6.0f;
    const float u = under <= -kn * 0.5f ? 0.0f : under >= kn * 0.5f ? under : (under + kn * 0.5f) * (under + kn * 0.5f) / (2.0f * kn);
    return juce::jlimit (0.0f, juce::jmax (0.0f, lift), u * (1.0f - 1.0f / 1.2f));
}
inline float peakThreshold (const HelperSettings& s)   // the 1964's threshold in dBFS (0 dBFS = +18 dBu); the pad raises it
{
    return juce::jlimit (-60.0f, 0.0f, s.inDbu + 26.0f + (s.pad == 1 ? 10.0f : s.pad == 2 ? 20.0f : 0.0f));
}

class Helpers
{
public:
    // what the screen shows
    std::atomic<float> peakGr { 0.0f }, rideDb { 0.0f }, breathRed { 0.0f }, liftLow { 0.0f }, liftHigh { 0.0f },
                       lowLvl { -90.0f }, highLvl { -90.0f }, loudIn { -60.0f }, loudOut { -60.0f }, outPeak { 0.0f };
    std::atomic<int> breaths { 0 };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        kHp = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 100.0, 0.707f);
        for (auto& f : kw) f.coefficients = kHp;
        bLo = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 370.0, 0.8f);
        bHi = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 5200.0, 0.9f);
        brLo.coefficients = bLo; brHi.coefficients = bHi;
        setLift (HelperSettings {}, true);
        reset();
    }
    void reset()
    {
        for (auto& f : kw) f.reset(); brLo.reset(); brHi.reset();
        for (auto& b : bands) for (auto& f : b) f.reset();
        loEnv = hiEnv = fullEnv = vocalEnv = 0.0f; bGain = 1.0f; wasBreath = false;
        shortMs = 1.0e-6f; rideNow = 0.0f; envLow = envHigh = 1.0e-9f; pkEnv = 0.0f; outMs = 1.0e-6f;
    }

    void process (juce::AudioBuffer<float>& buf, int nc, int n, const HelperSettings& s)
    {
        setLift (s, false);
        const double inMs = meanSquare (buf, nc, n);
        // ---- LIFT
        float lLow = 0.0f, lHigh = 0.0f;
        const bool solo = s.lowSolo || s.highSolo;
        if (solo || (s.liftOn && (s.lowLift > 0.05f || s.highLift > 0.05f)))
        {
            const float eK = std::exp (-1.0f / (0.030f * (float) sr));   // 30 ms band followers
            const float gK = std::exp (-1.0f / (0.050f * (float) sr));
            for (int i = 0; i < n; ++i)
            {
                float lo[2], hi[2];
                for (int c = 0; c < nc; ++c)
                {
                    const float x = buf.getSample (c, i);
                    lo[c] = bands[0][(size_t) c].processSample (x);
                    hi[c] = bands[1][(size_t) c].processSample (x);
                }
                const float ml = lo[0] * lo[0] + (nc > 1 ? lo[1] * lo[1] : 0.0f), mh = hi[0] * hi[0] + (nc > 1 ? hi[1] * hi[1] : 0.0f);
                envLow = eK * envLow + (1.0f - eK) * ml / (float) nc;
                envHigh = eK * envHigh + (1.0f - eK) * mh / (float) nc;
                const float dl = 10.0f * std::log10 (envLow + 1.0e-12f) + 3.0f, dh = 10.0f * std::log10 (envHigh + 1.0e-12f) + 3.0f;
                // silence isn't lifted: below -70 dB the band rests
                const float tl = dl > -70.0f ? liftCurve (dl, s.lowThr, s.lowLift) : 0.0f;
                const float th = dh > -70.0f ? liftCurve (dh, s.highThr, s.highLift) : 0.0f;
                gLow = gK * gLow + (1.0f - gK) * tl; gHigh = gK * gHigh + (1.0f - gK) * th;
                const float lg = s.liftOn ? gLow : 0.0f, hg = s.liftOn ? gHigh : 0.0f;
                const float al = juce::Decibels::decibelsToGain (lg) - 1.0f, ah = juce::Decibels::decibelsToGain (hg) - 1.0f;
                if (solo)   // SOLO: only the soloed band(s), lifted as Lift would
                    for (int c = 0; c < nc; ++c) buf.setSample (c, i, (s.lowSolo ? lo[c] * (1.0f + al) : 0.0f) + (s.highSolo ? hi[c] * (1.0f + ah) : 0.0f));
                else
                    for (int c = 0; c < nc; ++c) buf.setSample (c, i, buf.getSample (c, i) + lo[c] * al + hi[c] * ah);
                lLow = juce::jmax (lLow, gLow); lHigh = juce::jmax (lHigh, gHigh);
            }
            lowLvl.store (10.0f * std::log10 (envLow + 1.0e-12f) + 3.0f); highLvl.store (10.0f * std::log10 (envHigh + 1.0e-12f) + 3.0f);
        }
        else { gLow = gHigh = 0.0f; lowLvl.store (-90.0f); highLvl.store (-90.0f); }
        liftLow.store (lLow); liftHigh.store (lHigh);
        if (solo) { peakGr.store (0.0f); breathRed.store (0.0f); return; }   // soloing a band: hear it alone, nothing after it

        // ---- RIDER: short-term loudness (high-passed, ~400 ms) toward Target
        {
            double e = 0.0;
            for (int i = 0; i < n; ++i)
            {
                float x = 0.0f;
                for (int c = 0; c < nc; ++c) x += kw[(size_t) c].processSample (buf.getSample (c, i));
                x /= (float) nc; e += (double) x * x;
            }
            const float a = std::exp (-(float) n / (0.4f * (float) sr));
            shortMs = a * shortMs + (1.0f - a) * (float) (e / juce::jmax (1, n));
            const float lvl = 10.0f * std::log10 (shortMs + 1.0e-12f) - 0.7f;   // ~LUFS for a vocal
            loudIn.store (lvl);
            float target = rideNow;
            if (! s.rideOn || s.rideRange <= 0.05f) target = 0.0f;
            else if (lvl > -50.0f) target = juce::jlimit (-s.rideRange, s.rideRange, s.rideTarget - lvl);   // holds still in the gaps
            static constexpr float speedS[3] { 1.2f, 0.45f, 0.15f };
            const float gA = std::exp (-1.0f / (speedS[juce::jlimit (0, 2, s.rideSpeed)] * (float) sr));
            for (int i = 0; i < n; ++i)
            {
                rideNow = gA * rideNow + (1.0f - gA) * target;
                const float g = juce::Decibels::decibelsToGain (rideNow);
                for (int c = 0; c < nc; ++c) buf.setSample (c, i, buf.getSample (c, i) * g);
            }
            rideDb.store (rideNow);
        }

        // ---- BREATH
        float red = 0.0f;
        if (s.breathOn && s.breathDb < -0.05f)
        {
            const float env  = std::exp (-1.0f / (0.010f * (float) sr));
            const float vAtt = std::exp (-1.0f / (0.005f * (float) sr));
            const float vRel = std::exp (-1.0f / (2.0f * (float) sr));
            const float fade = juce::jmax (2.0f, s.breathFadeMs) * 0.001f;
            const float gAtt = std::exp (-1.0f / (fade * 0.4f * (float) sr));
            const float gRel = std::exp (-1.0f / (fade * 1.5f * (float) sr));
            const float dip = juce::Decibels::decibelsToGain (s.breathDb);
            const float gapDb = 22.0f - 0.18f * juce::jlimit (0.0f, 100.0f, s.breathSense);   // more Sense: a breath can be closer to the voice
            for (int i = 0; i < n; ++i)
            {
                float x = 0.0f;
                for (int c = 0; c < nc; ++c) x += buf.getSample (c, i);
                x /= (float) nc;
                const float l = brLo.processSample (x), h = brHi.processSample (x);
                loEnv = env * loEnv + (1.0f - env) * l * l;
                hiEnv = env * hiEnv + (1.0f - env) * h * h;
                fullEnv = env * fullEnv + (1.0f - env) * x * x;
                vocalEnv = fullEnv > vocalEnv ? vAtt * vocalEnv + (1.0f - vAtt) * fullEnv : vRel * vocalEnv + (1.0f - vRel) * fullEnv;
                const float fullDb = 10.0f * std::log10 (fullEnv + 1.0e-12f), vocalDb = 10.0f * std::log10 (vocalEnv + 1.0e-12f);
                const bool isBreath = hiEnv > 0.7f * loEnv && fullDb < vocalDb - gapDb && fullDb > vocalDb - 50.0f && fullDb > -75.0f;
                if (isBreath && ! wasBreath) breaths.fetch_add (1);
                wasBreath = isBreath;
                const float t = isBreath ? dip : 1.0f;
                bGain = t < bGain ? gAtt * bGain + (1.0f - gAtt) * t : gRel * bGain + (1.0f - gRel) * t;
                for (int c = 0; c < nc; ++c) buf.setSample (c, i, buf.getSample (c, i) * bGain);
                red = juce::jmax (red, -juce::Decibels::gainToDecibels (bGain, -60.0f));
            }
        }
        else bGain = 1.0f;
        breathRed.store (red);

        // ---- 1964: clean peak limiting - fast attack, program-like release, gain only
        float gr = 0.0f;
        if (s.peakOn)
        {
            const float thr = peakThreshold (s), ratio = juce::jmax (1.0f, s.slope);
            const float relMs = 50.0f * std::pow (200.0f, juce::jlimit (0.0f, 10.0f, s.release) / 10.0f);   // 50 ms .. 10 s
            const float aK = std::exp (-1.0f / (0.0002f * (float) sr)), rK = std::exp (-1.0f / (relMs * 0.001f * (float) sr));
            const float outG = juce::Decibels::decibelsToGain (s.outDbu - 4.0f);
            for (int i = 0; i < n; ++i)
            {
                float pk = 0.0f;
                for (int c = 0; c < nc; ++c) pk = juce::jmax (pk, std::abs (buf.getSample (c, i)));
                const float over = juce::Decibels::gainToDecibels (pk, -120.0f) - thr;
                const float want = over > 0.0f ? over * (1.0f - 1.0f / ratio) : 0.0f;   // dB to take off
                pkEnv = want > pkEnv ? aK * pkEnv + (1.0f - aK) * want : rK * pkEnv + (1.0f - rK) * want;
                const float g = juce::Decibels::decibelsToGain (-pkEnv) * outG;
                for (int c = 0; c < nc; ++c) buf.setSample (c, i, buf.getSample (c, i) * g);
                gr = juce::jmax (gr, pkEnv);
            }
        }
        else pkEnv = 0.0f;
        peakGr.store (gr);
        {
            const float a = std::exp (-(float) n / (0.4f * (float) sr));
            outMs = a * outMs + (1.0f - a) * (float) meanSquare (buf, nc, n);
            loudOut.store (10.0f * std::log10 (outMs + 1.0e-12f) - 0.7f);
            float pk = 0.0f; for (int c = 0; c < nc; ++c) pk = juce::jmax (pk, buf.getMagnitude (c, 0, n));
            outPeak.store (pk);
        }
        juce::ignoreUnused (inMs);
    }

private:
    static double meanSquare (const juce::AudioBuffer<float>& b, int nc, int n)
    {
        double e = 0.0;
        for (int c = 0; c < nc; ++c) { auto* x = b.getReadPointer (c); for (int i = 0; i < n; ++i) e += (double) x[i] * x[i]; }
        return e / (double) juce::jmax (1, n * nc);
    }
    void setLift (const HelperSettings& s, bool force)
    {
        const std::array<float, 3> want { s.lowHz, s.highHz, (float) s.liftMode };
        if (! force && want == liftAt) return;
        liftAt = want;
        const double ny = sr * 0.45;
        const double lf = juce::jlimit (20.0, ny, (double) s.lowHz), hf = juce::jlimit (200.0, ny, (double) s.highHz);
        // Upward: everything under the low frequency / over the high one. Dyn EQ: a bell (band-pass) at each
        auto lo = s.liftMode == 0 ? juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sr, lf, 0.707)
                                  : juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, lf, 0.9);
        auto hi = s.liftMode == 0 ? juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, hf, 0.707)
                                  : juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, hf, 0.9);
        for (int c = 0; c < 2; ++c)
        {
            if (bands[0][(size_t) c].coefficients == nullptr || force) { bands[0][(size_t) c].coefficients = new juce::dsp::IIR::Coefficients<float> (lo); bands[1][(size_t) c].coefficients = new juce::dsp::IIR::Coefficients<float> (hi); bands[0][(size_t) c].reset(); bands[1][(size_t) c].reset(); }
            else { *bands[0][(size_t) c].coefficients = lo; *bands[1][(size_t) c].coefficients = hi; }
        }
    }

    double sr = 48000.0;
    juce::dsp::IIR::Coefficients<float>::Ptr kHp, bLo, bHi;
    std::array<juce::dsp::IIR::Filter<float>, 2> kw;
    juce::dsp::IIR::Filter<float> brLo, brHi;
    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, 2> bands;   // [low / high][channel]
    std::array<float, 3> liftAt {};
    float envLow = 1.0e-9f, envHigh = 1.0e-9f, gLow = 0.0f, gHigh = 0.0f;
    float shortMs = 1.0e-6f, rideNow = 0.0f, outMs = 1.0e-6f;
    float loEnv = 0.0f, hiEnv = 0.0f, fullEnv = 0.0f, vocalEnv = 0.0f, bGain = 1.0f; bool wasBreath = false;
    float pkEnv = 0.0f;
};
}
