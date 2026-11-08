#pragma once

// Resonance suppressor in the spirit of oeksound soothe.
//
// The signal is analysed in short overlapping FFT frames. For every frequency
// bin we compare its (time-smoothed) level with a "spectral envelope", a
// version of the spectrum smoothed across neighbouring frequencies. Anything
// that pokes out above that envelope by more than the Selectivity threshold is
// a resonance, and only that bin is turned down, by an amount set by Depth,
// with its own attack and release. Broad tone is left alone.
//
//   Depth        how hard resonances are pulled down (max 24 dB at 100%)
//   Sharpness    how narrow a peak must be to count (high = only narrow rings)
//   Selectivity  how far above the envelope a peak must be (high = fewer, louder ones)
//   Attack/Release  how fast each bin reacts and recovers
//   Low/High     focus range; outside it nothing is touched
//   Mix          wet/dry blend;  Delta: hear only what is being removed
//
// Stereo-linked (both channels get the same gains) so the image never shifts.
// Adds a fixed latency of one FFT frame (1024 samples at 44.1/48 kHz,
// 2048 at 88.2 kHz and above), reported to the host.

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <complex>
#include <memory>
#include <vector>

namespace vox
{

struct ResoSettings
{
    bool  on = true;
    float depth = 0.4f;          // 0..1
    float sharpness = 0.4f;      // 0..1
    float selectivity = 0.5f;    // 0..1
    float attackMs = 5.0f;
    float releaseMs = 80.0f;
    float lowHz = 300.0f;
    float highHz = 12000.0f;
    float mix = 1.0f;            // 0..1
    bool  delta = false;
    // V60 mid/side (stereo tracks): 0 Stereo, 1 Mid only, 2 Side only, 3 Mid + Side (each half on its own)
    int   msMode = 0;
    float sideDepth = 0.15f;     // depth for the side in Mid + Side
    bool  msEnabled = false;     // false = the original stereo-linked behaviour
    float link = 1.0f;           // V61 (Stereo mode): 1 = left and right share one detector, 0 = each side on its own
};

class ResonanceSuppressor
{
public:
    static constexpr int maxBins = 2049;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        order = sr >= 176400.0 ? 12 : sr >= 88200.0 ? 11 : 10;   // keep ~21 ms frames (same Hz resolution) at any rate, incl. 4x oversampling
        N = 1 << order;
        hop = N / 4;
        bins = N / 2 + 1;
        fft = std::make_unique<juce::dsp::FFT> (order);

        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i)   // periodic sqrt-Hann: analysis x synthesis = Hann
            window[(size_t) i] = (float) std::sqrt (0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * i / N)));

        for (int c = 0; c < 2; ++c)
        {
            inBuf[c].assign ((size_t) N, 0.0f);
            outBuf[c].assign ((size_t) N, 0.0f);
            frame[c].assign ((size_t) N * 2, 0.0f);
        }
        for (auto& d : det)
        {
            d.smoothPow.assign ((size_t) bins, 0.0f);
            d.levelDb.assign ((size_t) bins, -200.0f);
            d.prefix.assign ((size_t) bins + 1, 0.0);
            d.gainDb.assign ((size_t) bins, 0.0f);
            d.outGain.assign ((size_t) bins, 1.0f);
        }
        targetDb.assign ((size_t) bins, 0.0f);
        spreadDb.assign ((size_t) bins, 0.0f);
        numBins.store (bins);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (inBuf[c].begin(), inBuf[c].end(), 0.0f);
            std::fill (outBuf[c].begin(), outBuf[c].end(), 0.0f);
        }
        for (auto& d : det)
        {
            std::fill (d.smoothPow.begin(), d.smoothPow.end(), 0.0f);
            std::fill (d.gainDb.begin(), d.gainDb.end(), 0.0f);
        }
        for (auto& r : reductionDb) r.store (0.0f);
        pos = 0;
        hopCount = 0;
    }

    int getLatencySamples() const { return N; }
    int getFftSize() const { return N; }

    void process (juce::AudioBuffer<float>& buffer, const ResoSettings& s)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        settings = s;
        // V60: on stereo tracks the analysis runs on mid / side (exactly the same result in Stereo mode)
        const bool ms = s.msEnabled && numCh == 2;
        if (ms != msDomain) { msDomain = ms; reset(); }
        mono = numCh < 2;

        for (int i = 0; i < n; ++i)
        {
            if (ms)
            {
                const float l = buffer.getSample (0, i), r = buffer.getSample (1, i);
                inBuf[0][(size_t) pos] = 0.5f * (l + r);
                inBuf[1][(size_t) pos] = 0.5f * (l - r);
                const float m = outBuf[0][(size_t) pos], sd = outBuf[1][(size_t) pos];
                outBuf[0][(size_t) pos] = outBuf[1][(size_t) pos] = 0.0f;
                buffer.setSample (0, i, m + sd);
                buffer.setSample (1, i, m - sd);
            }
            else
            for (int c = 0; c < 2; ++c)
            {
                const float x = buffer.getSample (juce::jmin (c, numCh - 1), i);
                inBuf[c][(size_t) pos] = x;
                const float y = outBuf[c][(size_t) pos];
                outBuf[c][(size_t) pos] = 0.0f;
                if (c < numCh) buffer.setSample (c, i, y);
            }

            pos = (pos + 1) % N;
            if (++hopCount == hop)
            {
                hopCount = 0;
                processFrame();
            }
        }
    }

    // Live gain reduction per FFT bin (dB, <= 0), for the display
    std::array<std::atomic<float>, maxBins> reductionDb {};
    std::atomic<int> numBins { 0 };
    double getSampleRate() const { return sr; }

private:
    void processFrame()
    {
        // 1. Analysis: window the newest N samples (pos = oldest) and FFT
        for (int c = 0; c < 2; ++c)
        {
            auto& f = frame[c];
            for (int k = 0; k < N; ++k)
                f[(size_t) k] = inBuf[c][(size_t) ((pos + k) % N)] * window[(size_t) k];
            std::fill (f.begin() + N, f.end(), 0.0f);
            fft->performRealOnlyForwardTransform (f.data());
        }

        // 2-4. Detection and gains. Stereo: one linked detector on the louder of L / R.
        //      V60 mid/side: Mid or Side on its own, or both halves independently (Mid + Side).
        const auto& s = settings;
        int mode = msDomain ? juce::jlimit (0, 3, s.msMode) : 0;
        if (mono && s.msEnabled) mode = juce::jlimit (0, 3, s.msMode) == 2 ? -1 : 0;   // mono = all mid: Side has nothing to do
        const float mix = juce::jlimit (0.0f, 1.0f, s.mix);
        auto bin = [this] (int c, int k) { return std::complex<float> (frame[c][(size_t) (2 * k)], frame[c][(size_t) (2 * k + 1)]); };

        const float link = juce::jlimit (0.0f, 1.0f, s.link);
        const bool lr = mode == 0 && msDomain && link < 0.999f;   // V61: left and right partly on their own
        if (lr)
        {
            analyse (det[2], s.depth, [&] (int k) { return std::norm (bin (0, k) + bin (1, k)); });   // left
            analyse (det[3], s.depth, [&] (int k) { return std::norm (bin (0, k) - bin (1, k)); });   // right
        }
        if (mode == 0)
            analyse (det[0], s.depth, [&] (int k)
            {
                if (msDomain) { const auto m = bin (0, k), sd = bin (1, k); return juce::jmax (std::norm (m + sd), std::norm (m - sd)); }
                return juce::jmax (std::norm (bin (0, k)), std::norm (bin (1, k)));
            });
        if (mode == 1 || mode == 3) analyse (det[0], s.depth, [&] (int k) { return std::norm (bin (0, k)); });
        if (mode == 2)              analyse (det[1], s.depth, [&] (int k) { return std::norm (bin (1, k)); });
        if (mode == 3)              analyse (det[1], s.sideDepth, [&] (int k) { return std::norm (bin (1, k)); });

        for (int k = 0; k < bins; ++k)
        {
            // per-half output gain (1 = untouched)
            float g0 = 1.0f, g1 = 1.0f, shown = 0.0f;
            auto wetOf = [&] (const Det& d) { return 1.0f + mix * (juce::Decibels::decibelsToGain (d.gainDb[(size_t) k]) - 1.0f); };
            if (mode == 0)      { g0 = g1 = wetOf (det[0]); shown = det[0].gainDb[(size_t) k]; }
            else if (mode == 1) { g0 = wetOf (det[0]);      shown = det[0].gainDb[(size_t) k]; }
            else if (mode == 2) { g1 = wetOf (det[1]);      shown = det[1].gainDb[(size_t) k]; }
            else if (mode == 3) { g0 = wetOf (det[0]); g1 = wetOf (det[1]); shown = juce::jmin (det[0].gainDb[(size_t) k], det[1].gainDb[(size_t) k]); }
            if (lr)
            {
                // blend each side's own gain with the linked one (in dB), then apply on L / R and go back to M / S
                const float gl = link * det[0].gainDb[(size_t) k] + (1.0f - link) * det[2].gainDb[(size_t) k];
                const float gr = link * det[0].gainDb[(size_t) k] + (1.0f - link) * det[3].gainDb[(size_t) k];
                float wl = 1.0f + mix * (juce::Decibels::decibelsToGain (gl) - 1.0f);
                float wr = 1.0f + mix * (juce::Decibels::decibelsToGain (gr) - 1.0f);
                if (s.delta) { wl = 1.0f - wl; wr = 1.0f - wr; }
                auto scale = [&] (int idx)   // idx = real or imaginary slot of this bin (or its mirror)
                {
                    const float m = frame[0][(size_t) idx], sd = frame[1][(size_t) idx];
                    const float l = (m + sd) * wl, r = (m - sd) * wr;
                    frame[0][(size_t) idx] = 0.5f * (l + r);
                    frame[1][(size_t) idx] = 0.5f * (l - r);
                };
                scale (2 * k); scale (2 * k + 1);
                if (k > 0 && k < N / 2) { scale (2 * (N - k)); scale (2 * (N - k) + 1); }
                if (k < maxBins) reductionDb[(size_t) k].store (juce::jmin (gl, gr) * mix, std::memory_order_relaxed);
                continue;
            }
            if (s.delta) { g0 = 1.0f - g0; g1 = 1.0f - g1; }

            for (int c = 0; c < 2; ++c)
            {
                const float out = c == 0 ? g0 : g1;
                auto& f = frame[c];
                f[(size_t) (2 * k)] *= out;
                f[(size_t) (2 * k + 1)] *= out;
                if (k > 0 && k < N / 2)   // mirrored negative frequency
                {
                    f[(size_t) (2 * (N - k))] *= out;
                    f[(size_t) (2 * (N - k) + 1)] *= out;
                }
            }

            if (k < maxBins) reductionDb[(size_t) k].store (shown * mix, std::memory_order_relaxed);
        }

        // 5. Synthesis: inverse FFT, window, overlap-add.
        //    sqrt-Hann^2 at 75% overlap sums to 2, hence the 0.5.
        for (int c = 0; c < 2; ++c)
        {
            auto& f = frame[c];
            fft->performRealOnlyInverseTransform (f.data());
            for (int k = 0; k < N; ++k)
                outBuf[c][(size_t) ((pos + k) % N)] += f[(size_t) k] * window[(size_t) k] * 0.5f;
        }
    }

    struct Det { std::vector<float> smoothPow, levelDb, gainDb, outGain; std::vector<double> prefix; };

    // one detector: power per bin -> envelope comparison -> smoothed gain (dB) per bin
    template <typename PowFn>
    void analyse (Det& d, float depth, PowFn&& powerOf)
    {
        const auto& s = settings;
        const float powCoef = std::exp (-(float) hop / (0.02f * (float) sr));
        for (int k = 0; k < bins; ++k)
        {
            const float p = powerOf (k);
            d.smoothPow[(size_t) k] = powCoef * d.smoothPow[(size_t) k] + (1.0f - powCoef) * p;
            d.levelDb[(size_t) k] = 10.0f * std::log10 (d.smoothPow[(size_t) k] + 1.0e-20f);
            d.prefix[(size_t) k + 1] = d.prefix[(size_t) k] + d.levelDb[(size_t) k];
        }

        depth = juce::jlimit (0.0f, 1.0f, depth);
        const bool active = s.on && depth > 0.0f;
        const float widthOct = 1.0f - 0.75f * juce::jlimit (0.0f, 1.0f, s.sharpness);
        const float lowMul = std::pow (2.0f, -widthOct * 0.5f), highMul = std::pow (2.0f, widthOct * 0.5f);
        const float threshold = 2.0f + 10.0f * juce::jlimit (0.0f, 1.0f, s.selectivity);
        const float maxRed = 24.0f * depth;
        const float slope = 0.5f + depth;
        const float binHz = (float) (sr / N);
        const float lo = juce::jmax (20.0f, juce::jmin (s.lowHz, s.highHz)), hi = juce::jmax (s.lowHz, s.highHz);

        for (int k = 0; k < bins; ++k)
        {
            float t = 0.0f;
            if (active && k >= 2)
            {
                const int a = juce::jmax (1, (int) std::floor ((float) k * lowMul));
                const int b = juce::jmin (bins - 1, (int) std::ceil ((float) k * highMul));
                const float env = (float) ((d.prefix[(size_t) b + 1] - d.prefix[(size_t) a]) / (double) (b - a + 1));
                const float excess = d.levelDb[(size_t) k] - env - threshold;
                if (excess > 0.0f)
                {
                    // Focus range with a 1/3-octave fade at each edge
                    const float hz = (float) k * binHz;
                    const float fadeLo = juce::jlimit (0.0f, 1.0f, 3.0f * std::log2 (hz / lo) + 1.0f);
                    const float fadeHi = juce::jlimit (0.0f, 1.0f, 3.0f * std::log2 (hi / hz) + 1.0f);
                    t = -juce::jmin (maxRed, excess * slope) * fadeLo * fadeHi;
                }
            }
            targetDb[(size_t) k] = t;
        }

        // spread slightly across neighbours (smoother, fewer artefacts), then per-bin attack / release
        const float att = std::exp (-(float) hop / (juce::jmax (0.5f, s.attackMs) * 0.001f * (float) sr));
        const float rel = std::exp (-(float) hop / (juce::jmax (1.0f, s.releaseMs) * 0.001f * (float) sr));
        for (int k = 0; k < bins; ++k)
        {
            const float l = targetDb[(size_t) juce::jmax (0, k - 1)], r = targetDb[(size_t) juce::jmin (bins - 1, k + 1)];
            spreadDb[(size_t) k] = juce::jmin (targetDb[(size_t) k], 0.25f * l + 0.5f * targetDb[(size_t) k] + 0.25f * r);
        }
        for (int k = 0; k < bins; ++k)
        {
            auto& g = d.gainDb[(size_t) k];
            const float target = spreadDb[(size_t) k];
            g = (target < g ? att : rel) * (g - target) + target;
        }
    }

    double sr = 48000.0;
    int order = 10, N = 1024, hop = 256, bins = 513;
    std::array<Det, 4> det;   // [0] stereo / mid, [1] side, [2] / [3] left / right on their own (V61 link)
    bool msDomain = false, mono = false;
    int pos = 0, hopCount = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::array<std::vector<float>, 2> inBuf, outBuf, frame;
    std::vector<float> targetDb, spreadDb;
    ResoSettings settings;
};

} // namespace vox
