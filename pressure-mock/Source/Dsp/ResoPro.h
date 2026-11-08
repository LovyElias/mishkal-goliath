#pragma once

// Mishkal Tahor - RESO: the resonance suppressor, grown from Goliath's (ResonanceSuppressor.h) into a soothe-style unit:
//   Soft / Hard, Depth, Detail (sharpness + selectivity together), Attack, Release, Max cut, Mix, Trim,
//   Stereo / Mid / Side, Delta, Sidechain (the detector listens to the key input), Stereo link,
//   Tilt for Detail / Attack / Release (leans each towards the lows or the highs),
//   up to six shaping nodes (Bell, Bandpass, Low shelf, High shelf, Tilt, Notch, Low cut, High cut) that push or relax
//   the processing by frequency, the focus range (the red edges), Low latency (half the FFT size) and
//   Minimum or Linear phase (minimum phase turns each frame's gain curve into a minimum-phase filter: no pre-ringing).

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <complex>
#include <memory>
#include <vector>

namespace tahor
{

enum class NodeShape { Bell = 0, Bandpass, LowShelf, HighShelf, Tilt, Notch, LowCut, HighCut };
inline constexpr int numNodeShapes = 8;
inline constexpr int maxResoNodes = 6;

struct ResoNode
{
    bool on = false;
    int shape = 0;
    float hz = 1000.0f;
    float amount = 0.0f;   // dB: + pushes harder, - relaxes
    float q = 1.0f;        // width (0.2 broad .. 6 narrow)
    bool operator!= (const ResoNode& o) const { return on != o.on || shape != o.shape || hz != o.hz || amount != o.amount || q != o.q; }
};

// how a node shapes the processing at frequency f: an amount in dB (added) and a mask (multiplied, cuts / notch)
inline float nodeDb (const ResoNode& n, float f)
{
    if (! n.on) return 0.0f;
    const float oct = std::log2 (juce::jmax (1.0f, f) / n.hz);
    const float q = juce::jlimit (0.1f, 10.0f, n.q);
    switch ((NodeShape) n.shape)
    {
        case NodeShape::Bell:      { const float u = oct * q * 1.2f; return n.amount * std::exp (-u * u); }
        case NodeShape::Bandpass:  { const float r = f / n.hz - n.hz / f; return n.amount / std::sqrt (1.0f + q * q * r * r * 4.0f); }
        case NodeShape::LowShelf:  return n.amount / (1.0f + std::exp (4.0f * q * oct));
        case NodeShape::HighShelf: return n.amount / (1.0f + std::exp (-4.0f * q * oct));
        case NodeShape::Tilt:      return n.amount * juce::jlimit (0.0f, 3.0f, 1.0f + oct * q * 1.07f);
        default: break;
    }
    return 0.0f;
}
inline float nodeMask (const ResoNode& n, float f)
{
    if (! n.on) return 1.0f;
    const float oct = std::log2 (juce::jmax (1.0f, f) / n.hz);
    const float q = juce::jlimit (0.1f, 10.0f, n.q);
    switch ((NodeShape) n.shape)
    {
        case NodeShape::Notch:   { const float u = oct * q * 3.0f; const float d = juce::jlimit (0.0f, 1.0f, 0.5f + std::abs (n.amount) / 12.0f); return 1.0f - d * std::exp (-u * u); }
        case NodeShape::LowCut:  return 1.0f / (1.0f + std::exp (-6.0f * q * oct));
        case NodeShape::HighCut: return 1.0f / (1.0f + std::exp (6.0f * q * oct));
        default: break;
    }
    return 1.0f;
}

struct ResoProSettings
{
    bool  on = true;
    float depth = 5.0f;          // 0..10
    float detail = 0.5f;         // 0..1: sharpness + selectivity
    float attackMs = 5.0f, releaseMs = 80.0f;
    float maxCutDb = 12.0f;      // the deepest any frequency may be cut
    float mix = 1.0f;            // 0..1
    float trimDb = 0.0f;
    bool  hard = false;
    int   msMode = 0;            // 0 stereo, 1 mid, 2 side
    bool  delta = false, sidechain = false, minPhase = true;
    float link = 1.0f;
    float detailTilt = 0.0f, attackTilt = 0.0f, releaseTilt = 0.0f;   // -1..1
    float lowHz = 40.0f, highHz = 14000.0f;
    std::array<ResoNode, maxResoNodes> nodes {};
};

class ResoPro
{
public:
    static constexpr int maxBins = 4097;

    void prepare (double sampleRate, bool lowLatency)
    {
        sr = sampleRate;
        int o = sr >= 176400.0 ? 12 : sr >= 88200.0 ? 11 : 10;
        if (lowLatency) --o;
        order = o; N = 1 << order; hop = N / 4; bins = N / 2 + 1;
        fft = std::make_unique<juce::dsp::FFT> (order);
        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = (float) std::sqrt (0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * i / N)));
        for (int c = 0; c < 2; ++c)
        {
            inBuf[c].assign ((size_t) N, 0.0f); keyIn[c].assign ((size_t) N, 0.0f);
            outBuf[c].assign ((size_t) N, 0.0f);
            frame[c].assign ((size_t) N * 2, 0.0f); keyFrame[c].assign ((size_t) N * 2, 0.0f);
        }
        for (auto& d : det)
        {
            d.smoothPow.assign ((size_t) bins, 0.0f); d.levelDb.assign ((size_t) bins, -200.0f);
            d.prefix.assign ((size_t) bins + 1, 0.0); d.gainDb.assign ((size_t) bins, 0.0f);
        }
        targetDb.assign ((size_t) bins, 0.0f); spreadDb.assign ((size_t) bins, 0.0f);
        weightDb.assign ((size_t) bins, 0.0f); mask.assign ((size_t) bins, 1.0f);
        cep.assign ((size_t) N * 2, 0.0f);
        for (auto& h : minH) h.assign ((size_t) bins, { 1.0f, 0.0f });
        gl.assign ((size_t) bins, 1.0f); gr.assign ((size_t) bins, 1.0f);
        nodesWas.fill (ResoNode { true, -1 });
        numBins.store (bins);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (inBuf[c].begin(), inBuf[c].end(), 0.0f); std::fill (keyIn[c].begin(), keyIn[c].end(), 0.0f);
            std::fill (outBuf[c].begin(), outBuf[c].end(), 0.0f);
        }
        for (auto& d : det) { std::fill (d.smoothPow.begin(), d.smoothPow.end(), 0.0f); std::fill (d.gainDb.begin(), d.gainDb.end(), 0.0f); }
        for (auto& r : reductionDb) r.store (0.0f);
        reductionNow.store (0.0f);
        pos = 0; hopCount = 0;
    }

    int getLatencySamples() const { return N; }
    double getSampleRate() const { return sr; }

    // key: the sidechain input (may be null); used when settings.sidechain is on
    void process (juce::AudioBuffer<float>& buffer, const ResoProSettings& s, const juce::AudioBuffer<float>* key)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        settings = s;
        useKey = s.sidechain && key != nullptr && key->getNumChannels() > 0 && key->getNumSamples() >= n;
        stereo = numCh == 2;
        const float trim = juce::Decibels::decibelsToGain (s.on ? s.trimDb : 0.0f);
        for (int i = 0; i < n; ++i)
        {
            const float l = buffer.getSample (0, i), r = buffer.getSample (numCh - 1, i);
            inBuf[0][(size_t) pos] = stereo ? 0.5f * (l + r) : l;   // always mid / side inside (stereo)
            inBuf[1][(size_t) pos] = stereo ? 0.5f * (l - r) : 0.0f;
            if (useKey)
            {
                const float kl = key->getSample (0, i), kr = key->getSample (juce::jmin (1, key->getNumChannels() - 1), i);
                keyIn[0][(size_t) pos] = 0.5f * (kl + kr); keyIn[1][(size_t) pos] = 0.5f * (kl - kr);
            }
            const float m = outBuf[0][(size_t) pos], sd = outBuf[1][(size_t) pos];
            outBuf[0][(size_t) pos] = outBuf[1][(size_t) pos] = 0.0f;
            if (stereo) { buffer.setSample (0, i, (m + sd) * trim); buffer.setSample (1, i, (m - sd) * trim); }
            else        buffer.setSample (0, i, m * trim);
            pos = (pos + 1) % N;
            if (++hopCount == hop) { hopCount = 0; processFrame(); }
        }
    }

    std::array<std::atomic<float>, maxBins> reductionDb {};   // per bin, for the display
    std::atomic<int> numBins { 0 };
    std::atomic<float> reductionNow { 0.0f };                  // overall (energy-weighted), for the meter

private:
    using C = std::complex<float>;
    void forward (std::array<std::vector<float>, 2>& in, std::array<std::vector<float>, 2>& out)
    {
        for (int c = 0; c < 2; ++c)
        {
            auto& f = out[(size_t) c];
            for (int k = 0; k < N; ++k) f[(size_t) k] = in[(size_t) c][(size_t) ((pos + k) % N)] * window[(size_t) k];
            std::fill (f.begin() + N, f.end(), 0.0f);
            fft->performRealOnlyForwardTransform (f.data());
        }
    }

    void updateWeights()
    {
        bool changed = false;
        for (int i = 0; i < maxResoNodes; ++i) if (settings.nodes[(size_t) i] != nodesWas[(size_t) i]) changed = true;
        if (! changed) return;
        nodesWas = settings.nodes;
        const float binHz = (float) (sr / N);
        for (int k = 0; k < bins; ++k)
        {
            const float f = juce::jmax (1.0f, (float) k * binHz);
            float db = 0.0f, m = 1.0f;
            for (auto& nd : settings.nodes) { db += nodeDb (nd, f); m *= nodeMask (nd, f); }
            weightDb[(size_t) k] = db; mask[(size_t) k] = m;
        }
    }

    // minimum-phase version of a (zero-phase) gain curve, by the real cepstrum
    void minPhase (const std::vector<float>& g, std::vector<C>& h)
    {
        for (int k = 0; k < bins; ++k) { cep[(size_t) (2 * k)] = std::log (juce::jmax (1.0e-5f, g[(size_t) k])); cep[(size_t) (2 * k + 1)] = 0.0f; }
        std::fill (cep.begin() + 2 * bins, cep.end(), 0.0f);
        fft->performRealOnlyInverseTransform (cep.data());   // real cepstrum (scaled by 1/N)
        for (int i = 1; i < N / 2; ++i) cep[(size_t) i] *= 2.0f;
        std::fill (cep.begin() + N / 2 + 1, cep.end(), 0.0f);
        fft->performRealOnlyForwardTransform (cep.data());
        for (int k = 0; k < bins; ++k) h[(size_t) k] = std::exp (C (cep[(size_t) (2 * k)], cep[(size_t) (2 * k + 1)]));
    }

    void processFrame()
    {
        forward (inBuf, frame);
        if (useKey) forward (keyIn, keyFrame);
        const auto& s = settings;
        updateWeights();
        auto& src = useKey ? keyFrame : frame;
        auto bin = [&src] (int c, int k) { return C (src[c][(size_t) (2 * k)], src[c][(size_t) (2 * k + 1)]); };
        const float mix = juce::jlimit (0.0f, 1.0f, s.mix);
        const int mode = stereo ? juce::jlimit (0, 2, s.msMode) : 0;
        const float link = juce::jlimit (0.0f, 1.0f, s.link);
        const bool lr = mode == 0 && stereo && link < 0.999f;

        if (mode == 0)
            analyse (det[0], [&] (int k) { if (! stereo) return std::norm (bin (0, k)); const auto m = bin (0, k), sd = bin (1, k); return juce::jmax (std::norm (m + sd), std::norm (m - sd)); });
        if (mode == 1) analyse (det[0], [&] (int k) { return std::norm (bin (0, k)); });
        if (mode == 2) analyse (det[1], [&] (int k) { return std::norm (bin (1, k)); });
        if (lr)
        {
            analyse (det[2], [&] (int k) { return std::norm (bin (0, k) + bin (1, k)); });
            analyse (det[3], [&] (int k) { return std::norm (bin (0, k) - bin (1, k)); });
        }

        // the gain curve(s): gl / gr (lr) or gl (one curve: stereo, or the mid / side half that is processed)
        double eSum = 0.0, rSum = 0.0;
        for (int k = 0; k < bins; ++k)
        {
            float a = 0.0f, b = 0.0f;
            const auto& d = det[mode == 2 ? 1 : 0];
            if (lr) { a = link * d.gainDb[(size_t) k] + (1.0f - link) * det[2].gainDb[(size_t) k];
                      b = link * d.gainDb[(size_t) k] + (1.0f - link) * det[3].gainDb[(size_t) k]; }
            else a = b = d.gainDb[(size_t) k];
            if (! s.on) a = b = 0.0f;
            gl[(size_t) k] = 1.0f + mix * (juce::Decibels::decibelsToGain (a) - 1.0f);
            gr[(size_t) k] = 1.0f + mix * (juce::Decibels::decibelsToGain (b) - 1.0f);
            const float shown = juce::jmin (a, b) * mix;
            if (k < maxBins) reductionDb[(size_t) k].store (shown, std::memory_order_relaxed);
            const float p = d.smoothPow[(size_t) k];
            eSum += p; rSum += p * gl[(size_t) k] * gl[(size_t) k];
        }
        reductionNow.store (eSum > 1.0e-12 ? (float) (10.0 * std::log10 (juce::jmax (1.0e-6, rSum / eSum))) : 0.0f);   // the energy taken out

        const bool mp = s.minPhase;
        if (mp) { minPhase (gl, minH[0]); if (lr) minPhase (gr, minH[1]); }
        auto gainAt = [&] (int which, int k) -> C
        {
            C g = mp ? minH[(size_t) (lr ? which : 0)][(size_t) k] : C ((which == 0 ? gl : gr)[(size_t) k], 0.0f);
            return s.delta ? C (1.0f, 0.0f) - g : g;
        };
        for (int k = 0; k < bins; ++k)
        {
            C m (frame[0][(size_t) (2 * k)], frame[0][(size_t) (2 * k + 1)]);
            C sd (frame[1][(size_t) (2 * k)], frame[1][(size_t) (2 * k + 1)]);
            if (lr)
            {
                const C l = (m + sd) * gainAt (0, k), r = (m - sd) * gainAt (1, k);
                m = 0.5f * (l + r); sd = 0.5f * (l - r);
            }
            else
            {
                const C g = gainAt (0, k);
                if (mode == 0)      { m *= g; sd *= g; }
                else if (mode == 1) { m *= g; if (s.delta) sd = 0.0f; }
                else                { sd *= g; if (s.delta) m = 0.0f; }
            }
            frame[0][(size_t) (2 * k)] = m.real(); frame[0][(size_t) (2 * k + 1)] = m.imag();
            frame[1][(size_t) (2 * k)] = sd.real(); frame[1][(size_t) (2 * k + 1)] = sd.imag();
        }
        for (int c = 0; c < 2; ++c)
        {
            auto& f = frame[c];
            for (int k = 1; k < N / 2; ++k)   // the mirrored half (conjugate)
            {
                f[(size_t) (2 * (N - k))] = f[(size_t) (2 * k)];
                f[(size_t) (2 * (N - k) + 1)] = -f[(size_t) (2 * k + 1)];
            }
            fft->performRealOnlyInverseTransform (f.data());
            for (int k = 0; k < N; ++k) outBuf[c][(size_t) ((pos + k) % N)] += f[(size_t) k] * window[(size_t) k] * 0.5f;
        }
    }

    struct Det { std::vector<float> smoothPow, levelDb, gainDb; std::vector<double> prefix; };

    template <typename PowFn>
    void analyse (Det& d, PowFn&& powerOf)
    {
        const auto& s = settings;
        const float powCoef = std::exp (-(float) hop / (0.02f * (float) sr));
        for (int k = 0; k < bins; ++k)
        {
            d.smoothPow[(size_t) k] = powCoef * d.smoothPow[(size_t) k] + (1.0f - powCoef) * powerOf (k);
            d.levelDb[(size_t) k] = 10.0f * std::log10 (d.smoothPow[(size_t) k] + 1.0e-20f);
            d.prefix[(size_t) k + 1] = d.prefix[(size_t) k] + d.levelDb[(size_t) k];
        }
        const float depth = juce::jlimit (0.0f, 1.0f, s.depth / 12.5f);   // 5 = Goliath's default depth (0.4)
        const bool active = s.on && depth > 0.0f;
        const float binHz = (float) (sr / N);
        const float lo = juce::jmax (20.0f, juce::jmin (s.lowHz, s.highHz)), hi = juce::jmax (s.lowHz, s.highHz);
        const float maxCut = juce::jlimit (0.0f, 30.0f, s.maxCutDb);
        for (int k = 0; k < bins; ++k)
        {
            float t = 0.0f;
            if (active && k >= 2)
            {
                const float hz = (float) k * binHz;
                const float tl = juce::jlimit (-1.0f, 1.0f, std::log2 (hz / 1000.0f) / 5.0f);   // -1 lows .. +1 highs
                const float detail = juce::jlimit (0.0f, 1.0f, s.detail + 0.5f * s.detailTilt * tl);
                const float widthOct = 1.0f - 0.75f * detail;
                const float lowMul = std::pow (2.0f, -widthOct * 0.5f), highMul = std::pow (2.0f, widthOct * 0.5f);
                const float threshold = s.hard ? 1.0f + 6.0f * detail : 2.0f + 10.0f * detail;
                const float dEff = juce::jlimit (0.0f, 2.0f, depth * std::pow (2.0f, weightDb[(size_t) k] / 6.0f));
                const float slope = s.hard ? 1.0f + 2.0f * dEff : 0.5f + dEff;
                const float maxRed = juce::jmin (maxCut, 24.0f * dEff);
                const int a = juce::jmax (1, (int) std::floor ((float) k * lowMul));
                const int b = juce::jmin (bins - 1, (int) std::ceil ((float) k * highMul));
                const float env = (float) ((d.prefix[(size_t) b + 1] - d.prefix[(size_t) a]) / (double) (b - a + 1));
                const float excess = d.levelDb[(size_t) k] - env - threshold;
                if (excess > 0.0f)
                {
                    const float fadeLo = juce::jlimit (0.0f, 1.0f, 3.0f * std::log2 (hz / lo) + 1.0f);
                    const float fadeHi = juce::jlimit (0.0f, 1.0f, 3.0f * std::log2 (hi / hz) + 1.0f);
                    t = -juce::jmin (maxRed, excess * slope) * fadeLo * fadeHi * mask[(size_t) k];
                }
            }
            targetDb[(size_t) k] = t;
        }
        for (int k = 0; k < bins; ++k)
        {
            const float l = targetDb[(size_t) juce::jmax (0, k - 1)], r = targetDb[(size_t) juce::jmin (bins - 1, k + 1)];
            spreadDb[(size_t) k] = juce::jmin (targetDb[(size_t) k], 0.25f * l + 0.5f * targetDb[(size_t) k] + 0.25f * r);
        }
        const bool tiltT = std::abs (s.attackTilt) > 1.0e-3f || std::abs (s.releaseTilt) > 1.0e-3f;
        float att = std::exp (-(float) hop / (juce::jmax (0.5f, s.attackMs) * 0.001f * (float) sr));
        float rel = std::exp (-(float) hop / (juce::jmax (1.0f, s.releaseMs) * 0.001f * (float) sr));
        for (int k = 0; k < bins; ++k)
        {
            if (tiltT)
            {
                const float tl = juce::jlimit (-1.0f, 1.0f, std::log2 (juce::jmax (1.0f, (float) k * binHz) / 1000.0f) / 5.0f);
                att = std::exp (-(float) hop / (juce::jmax (0.5f, s.attackMs * std::pow (4.0f, s.attackTilt * tl)) * 0.001f * (float) sr));
                rel = std::exp (-(float) hop / (juce::jmax (1.0f, s.releaseMs * std::pow (4.0f, s.releaseTilt * tl)) * 0.001f * (float) sr));
            }
            auto& g = d.gainDb[(size_t) k];
            const float target = spreadDb[(size_t) k];
            g = (target < g ? att : rel) * (g - target) + target;
        }
    }

    double sr = 48000.0;
    int order = 10, N = 1024, hop = 256, bins = 513;
    std::array<Det, 4> det;
    bool stereo = true, useKey = false;
    int pos = 0, hopCount = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, cep;
    std::array<std::vector<float>, 2> inBuf, keyIn, outBuf, frame, keyFrame;
    std::vector<float> targetDb, spreadDb, weightDb, mask, gl, gr;
    std::array<std::vector<C>, 2> minH;
    std::array<ResoNode, maxResoNodes> nodesWas {};
    ResoProSettings settings;
};

} // namespace tahor
