#pragma once
// Perach (Hebrew: "flower") - an adaptive tone shaper.
//
// Every ~5 ms it measures the broad tonal balance of the signal (octave-wide regions, pink-compensated so a
// naturally balanced source reads as flat), compares it with a target curve (flat plus the four tone knobs:
// Low / Body / Presence / Air) and gently boosts or cuts whole regions to close the gap - only as much as the
// material needs, moment to moment. Overall loudness is kept where it was (the correction is energy-weighted
// to average 0 dB), and it never lifts bins that sit near the noise floor.
// Above Amount 7 it adds "squash": frequency-dependent upward / downward compression of each region against
// its own longer-term level, which gives a denser, more finished sound.
//
// Same STFT framework as the resonance suppressor (sqrt-Hann, 75% overlap, latency = one frame).

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace vox
{

struct ToneShaperSettings
{
    bool  on = true;
    float amount = 5.0f;              // 0..10
    std::array<float, 4> tone {};     // Low, Body, Presence, Air: -6..+6 dB "want more / less"
    float speed = 5.0f;               // 0 (slow, smooth) .. 10 (fast, follows every syllable)
    float mix = 1.0f;                 // 0..1
};

class ToneShaper
{
public:
    static constexpr int maxBins = 2049;
    static constexpr float toneHz[4] { 110.0f, 450.0f, 2800.0f, 10000.0f };   // centres of the four tone regions

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        order = sr >= 176400.0 ? 12 : sr >= 88200.0 ? 11 : 10;   // ~21 ms frames at any rate
        N = 1 << order;
        hop = N / 4;
        bins = N / 2 + 1;
        fft = std::make_unique<juce::dsp::FFT> (order);
        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = (float) std::sqrt (0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * i / N)));
        for (int c = 0; c < 2; ++c)
        {
            inBuf[c].assign ((size_t) N, 0.0f);
            outBuf[c].assign ((size_t) N, 0.0f);
            frame[c].assign ((size_t) N * 2, 0.0f);
        }
        for (auto* v : { &pow_, &shortDb, &longDb, &envDb, &corr, &gainDb, &pink })
            v->assign ((size_t) bins, 0.0f);
        prefix.assign ((size_t) bins + 1, 0.0);
        const float binHz = (float) (sr / N);
        for (int k = 0; k < bins; ++k)   // +3 dB/oct: pink noise reads flat
            pink[(size_t) k] = 3.0f * std::log2 (juce::jmax (20.0f, (float) k * binHz) / 1000.0f);
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
        std::fill (pow_.begin(), pow_.end(), 0.0f);
        std::fill (shortDb.begin(), shortDb.end(), -120.0f);
        std::fill (longDb.begin(), longDb.end(), -120.0f);
        std::fill (gainDb.begin(), gainDb.end(), 0.0f);
        for (auto& g : shownDb) g.store (0.0f);
        pos = hopCount = 0;
    }

    int getLatencySamples() const { return N; }

    // returns the average size of the correction it is applying (dB, >= 0) for the meter
    float process (juce::AudioBuffer<float>& buffer, const ToneShaperSettings& s)
    {
        settings = s;
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                inBuf[c][(size_t) pos] = buffer.getSample (juce::jmin (c, numCh - 1), i);
                const float y = outBuf[c][(size_t) pos];
                outBuf[c][(size_t) pos] = 0.0f;
                if (c < numCh) buffer.setSample (c, i, y);
            }
            pos = (pos + 1) % N;
            if (++hopCount == hop) { hopCount = 0; processFrame(); }
        }
        return activity;
    }

    // what it is doing right now, per bin (dB, + boost / - cut), for a display
    std::array<std::atomic<float>, maxBins> shownDb {};
    std::atomic<int> numBins { 0 };
    double getSampleRate() const { return sr; }

private:
    void processFrame()
    {
        for (int c = 0; c < 2; ++c)
        {
            auto& f = frame[c];
            for (int k = 0; k < N; ++k)
                f[(size_t) k] = inBuf[c][(size_t) ((pos + k) % N)] * window[(size_t) k];
            std::fill (f.begin() + N, f.end(), 0.0f);
            fft->performRealOnlyForwardTransform (f.data());
        }

        const auto& s = settings;
        const float speed = juce::jlimit (0.0f, 10.0f, s.speed);
        const float hopSec = (float) hop / (float) sr;
        // level followers: short (the "now", 30..250 ms) and long (the region's own average, ~1.5 s)
        const float shortMs = 250.0f * std::pow (30.0f / 250.0f, speed / 10.0f);
        const float aShort = std::exp (-hopSec / (shortMs * 0.001f));
        const float aLong  = std::exp (-hopSec / 1.5f);

        double total = 0.0;
        float peakDb = -200.0f;
        for (int k = 0; k < bins; ++k)
        {
            float p = 0.0f;
            for (int c = 0; c < 2; ++c)
            {
                const float re = frame[c][(size_t) (2 * k)], im = frame[c][(size_t) (2 * k + 1)];
                p += re * re + im * im;
            }
            pow_[(size_t) k] = aShort * pow_[(size_t) k] + (1.0f - aShort) * p;
            const float db = 10.0f * std::log10 (pow_[(size_t) k] + 1.0e-20f);
            shortDb[(size_t) k] = db;
            longDb[(size_t) k] = longDb[(size_t) k] < -110.0f ? db : aLong * longDb[(size_t) k] + (1.0f - aLong) * db;
            total += pow_[(size_t) k];
            peakDb = juce::jmax (peakDb, db);
        }

        // broad spectral envelope: average power over about an octave around each bin (log-frequency smoothing)
        prefix[0] = 0.0;
        for (int k = 0; k < bins; ++k) prefix[(size_t) k + 1] = prefix[(size_t) k] + pow_[(size_t) k];
        const float lowMul = std::pow (2.0f, -0.6f), highMul = std::pow (2.0f, 0.6f);
        for (int k = 1; k < bins; ++k)
        {
            const int a = juce::jmax (1, (int) std::floor ((float) k * lowMul));
            const int b = juce::jmin (bins - 1, (int) std::ceil ((float) k * highMul));
            const double avg = (prefix[(size_t) b + 1] - prefix[(size_t) a]) / (double) (b - a + 1);
            envDb[(size_t) k] = 10.0f * (float) std::log10 (avg + 1.0e-20) + pink[(size_t) k];
        }
        envDb[0] = envDb[1];

        const bool active = s.on && total > 1.0e-10;   // silence: hold still
        const float amount = juce::jlimit (0.0f, 10.0f, s.amount);
        const float strength = juce::jmin (amount, 7.0f) / 7.0f * 0.75f;   // 0..0.75 of the gap closed
        const float squash = juce::jmax (0.0f, amount - 7.0f) / 3.0f;     // 0..1 above 7
        const float toneFactor = juce::jmin (1.0f, amount / 5.0f);
        const float binHz = (float) (sr / N);

        // the measured shape relative to its own (energy-weighted) mean, over the range that matters (60 Hz - 14 kHz)
        double wSum = 0.0, mSum = 0.0;
        for (int k = 1; k < bins; ++k)
        {
            const float hz = (float) k * binHz;
            if (hz < 60.0f || hz > 14000.0f) continue;
            const double w = 1.0 / (double) k;   // equal weight per octave
            wSum += w; mSum += w * envDb[(size_t) k];
        }
        const float meanDb = wSum > 0.0 ? (float) (mSum / wSum) : 0.0f;

        double cw = 0.0, cs = 0.0;
        for (int k = 0; k < bins; ++k)
        {
            float c = 0.0f;
            const float hz = (float) k * binHz;
            if (active && hz >= 25.0f && shortDb[(size_t) k] > peakDb - 60.0f)   // leave the noise floor alone
            {
                // target: flat (pink) plus the four tone regions (bells about 1.5 octaves wide)
                float target = 0.0f;
                for (int t = 0; t < 4; ++t)
                {
                    const float d = std::log2 (juce::jmax (20.0f, hz) / toneHz[t]) / 0.75f;
                    target += s.tone[(size_t) t] * std::exp (-0.5f * d * d);
                }
                const float shape = envDb[(size_t) k] - meanDb;
                // automatic part closes part of the gap; the tone knobs are asked for in full from Amount 5 up
                c = juce::jlimit (-6.0f, 6.0f, strength * -juce::jlimit (-24.0f, 24.0f, shape)) + toneFactor * target;   // automatic part: at most +/-6 dB
                if (squash > 0.0f)   // each region pulled toward its own long-term level (up and down)
                    c -= squash * 0.6f * juce::jlimit (-12.0f, 12.0f, shortDb[(size_t) k] - longDb[(size_t) k]);
                // fade out at the very top and bottom
                c *= juce::jlimit (0.0f, 1.0f, std::log2 (hz / 25.0f)) * juce::jlimit (0.0f, 1.0f, std::log2 (19000.0f / juce::jmax (1.0f, hz)) * 2.0f);
                c = juce::jlimit (-12.0f, 12.0f, c);
            }
            corr[(size_t) k] = c;
            const double w = pow_[(size_t) k];
            cw += w; cs += w * c;
        }
        // keep the overall level where it was: remove the energy-weighted average of the correction
        const float offset = juce::jlimit (-3.0f, 3.0f, cw > 0.0 ? (float) (cs / cw) : 0.0f);   // never lifts quiet regions by more than 3 dB to keep level

        const float smooth = std::exp (-hopSec / ((60.0f - 4.0f * speed) * 0.001f));   // 60..20 ms gain smoothing
        const float mix = juce::jlimit (0.0f, 1.0f, s.mix);
        double act = 0.0, actW = 0.0;
        for (int k = 0; k < bins; ++k)
        {
            const float target = corr[(size_t) k] == 0.0f && ! active ? 0.0f : juce::jlimit (-12.0f, 9.0f, corr[(size_t) k] - offset);
            auto& g = gainDb[(size_t) k];
            g = smooth * g + (1.0f - smooth) * target;
            const float lin = juce::Decibels::decibelsToGain (g * mix);
            for (int c = 0; c < 2; ++c)
            {
                auto& f = frame[c];
                f[(size_t) (2 * k)] *= lin;
                f[(size_t) (2 * k + 1)] *= lin;
                if (k > 0 && k < N / 2)
                {
                    f[(size_t) (2 * (N - k))] *= lin;
                    f[(size_t) (2 * (N - k) + 1)] *= lin;
                }
            }
            if (k < maxBins) shownDb[(size_t) k].store (g * mix, std::memory_order_relaxed);
            act += pow_[(size_t) k] * std::abs (g * mix); actW += pow_[(size_t) k];
        }
        activity = actW > 0.0 ? (float) (act / actW) : 0.0f;

        for (int c = 0; c < 2; ++c)
        {
            auto& f = frame[c];
            fft->performRealOnlyInverseTransform (f.data());
            for (int k = 0; k < N; ++k)
                outBuf[c][(size_t) ((pos + k) % N)] += f[(size_t) k] * window[(size_t) k] * 0.5f;
        }
    }

    double sr = 48000.0;
    int order = 10, N = 1024, hop = 256, bins = 513;
    int pos = 0, hopCount = 0;
    float activity = 0.0f;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::array<std::vector<float>, 2> inBuf, outBuf, frame;
    std::vector<float> pow_, shortDb, longDb, envDb, corr, gainDb, pink;
    std::vector<double> prefix;
    ToneShaperSettings settings;
};

} // namespace vox
