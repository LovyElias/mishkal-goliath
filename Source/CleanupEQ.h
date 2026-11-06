#pragma once

// Clean-up EQ: 8 surgical bands in the spirit of FabFilter Pro-Q.
// Bell, shelves, notch, and low/high cuts from 12 to 48 dB/oct. Any bell or
// shelf can be made dynamic: it only moves when that frequency gets loud,
// which is ideal for taming resonances, harshness and boomy notes.

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <complex>

namespace vox
{

enum class BandType { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch };
inline constexpr int numCleanBands = 8;

struct CleanBand
{
    bool     on   = false;
    BandType type = BandType::Bell;
    float    freq = 1000.0f;
    float    gain = 0.0f;       // dB
    float    q    = 1.0f;
    int      slope = 1;         // cuts: 0..3 = 12, 24, 36, 48 dB/oct
    bool     dyn  = false;
    float    dynThresh = -30.0f;
    float    dynRange  = -6.0f; // extra gain (dB) applied when fully over threshold
    int      place = 0;         // V60: 0 = Stereo (both channels), 1 = Mid only, 2 = Side only (stereo tracks)

    bool hasGain() const { return type == BandType::Bell || type == BandType::LowShelf || type == BandType::HighShelf; }
    bool isCut() const   { return type == BandType::LowCut || type == BandType::HighCut; }
};

using CleanBands = std::array<CleanBand, numCleanBands>;

//==============================================================================
class CleanupEQ
{
public:
    static constexpr int maxStages = 4;
    using Biquad = std::array<float, 6>;   // b0 b1 b2 a0 a1 a2
    struct BandDesign { std::array<Biquad, maxStages> st {}; int n = 0; };
    using AC = juce::dsp::IIR::ArrayCoefficients<float>;

    static int stagesFor (const CleanBand& b) { return b.isCut() ? juce::jlimit (1, 4, b.slope + 1) : 1; }

    static BandDesign design (const CleanBand& b, float freq, float gainDb, float q, double sr)
    {
        BandDesign d;
        const float f = juce::jlimit (10.0f, (float) (sr * 0.45), freq);
        q = juce::jlimit (0.05f, 30.0f, q);
        const float g = juce::Decibels::decibelsToGain (gainDb);

        switch (b.type)
        {
            case BandType::Bell:      d.st[0] = AC::makePeakFilter (sr, f, q, g); d.n = 1; break;
            case BandType::LowShelf:  d.st[0] = AC::makeLowShelf   (sr, f, q, g); d.n = 1; break;
            case BandType::HighShelf: d.st[0] = AC::makeHighShelf  (sr, f, q, g); d.n = 1; break;
            case BandType::Notch:     d.st[0] = AC::makeNotch      (sr, f, q);    d.n = 1; break;
            case BandType::LowCut:
            case BandType::HighCut:
            {
                // Butterworth stage Qs for 2nd, 4th, 6th and 8th order. The band's
                // Q scales the most resonant stage, so Q 0.71 = flat Butterworth.
                static const float qs[4][4] = { { 0.7071f },
                                                { 0.5412f, 1.3066f },
                                                { 0.5176f, 0.7071f, 1.9319f },
                                                { 0.5098f, 0.6013f, 0.9000f, 2.5629f } };
                const int n = stagesFor (b);
                const float scale = q / 0.7071f;
                for (int s = 0; s < n; ++s)
                {
                    float sq = qs[n - 1][s];
                    if (s == n - 1) sq *= scale;
                    d.st[(size_t) s] = b.type == BandType::LowCut ? AC::makeHighPass (sr, f, sq)
                                                                  : AC::makeLowPass  (sr, f, sq);
                }
                d.n = n;
                break;
            }
        }
        return d;
    }

    static double bandMagnitude (const BandDesign& d, double hz, double sr)
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sr;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        double mag = 1.0;
        for (int s = 0; s < d.n; ++s)
        {
            const auto& c = d.st[(size_t) s];
            auto num = (double) c[0] + (double) c[1] * z1 + (double) c[2] * z2;
            auto den = (double) c[3] + (double) c[4] * z1 + (double) c[5] * z2;
            mag *= std::abs (num) / juce::jmax (1.0e-12, std::abs (den));
        }
        return mag;
    }

    //==========================================================================
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sr, 4096, 1 };
        for (auto& band : state)
        {
            for (auto& ch : band.filters)
                for (auto& f : ch)
                {
                    f.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
                    f.prepare (spec);
                }
            for (auto& det : band.detector)
            {
                det.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
                det.prepare (spec);
            }
        }
        reset();
    }

    void reset()
    {
        for (auto& band : state)
        {
            for (auto& ch : band.filters) for (auto& f : ch) f.reset();
            for (auto& det : band.detector) det.reset();
            band.env = {}; band.dynDb = 0.0f; band.initialised = false;
        }
        for (auto& d : dynOffsetDb) d.store (0.0f);
    }

    void process (juce::AudioBuffer<float>& buffer, const CleanBands& bands)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        constexpr int sub = 32;   // coefficients are refreshed every 32 samples

        const float smooth = 1.0f - std::exp (-(float) sub / (0.025f * (float) sr));
        const float envAtt = msToCoefLocal (3.0f), envRel = msToCoefLocal (80.0f);
        const float dynAtt = std::exp (-(float) sub / (0.005f * (float) sr));
        const float dynRel = std::exp (-(float) sub / (0.120f * (float) sr));

        for (int start = 0; start < n; start += sub)
        {
            const int len = juce::jmin (sub, n - start);

            for (int bi = 0; bi < numCleanBands; ++bi)
            {
                const auto& b = bands[(size_t) bi];
                auto& st = state[(size_t) bi];
                // V60 mid/side: on a mono track everything is "mid", so a Side band has nothing to work on
                const int place = juce::jlimit (0, 2, b.place);
                const bool msBand = place != 0 && numCh > 1;

                if (! b.on || (place == 2 && numCh < 2))
                {
                    st.initialised = false;
                    st.dynDb = 0.0f;
                    dynOffsetDb[(size_t) bi].store (0.0f);
                    continue;
                }

                // (Re)start cleanly when switched on or when the filter shape changes
                if (! st.initialised || st.type != b.type || st.slope != b.slope || st.place != place)
                {
                    st.place = place;
                    for (auto& ch : st.filters) for (auto& f : ch) f.reset();
                    for (auto& det : st.detector) det.reset();
                    st.freq = b.freq; st.gain = b.gain; st.q = b.q;
                    st.type = b.type; st.slope = b.slope;
                    st.env = {}; st.dynDb = 0.0f;
                    st.initialised = true;
                }

                // Glide toward the target settings (no zipper noise while dragging)
                st.freq *= std::pow (b.freq / st.freq, smooth);
                st.gain += (b.gain - st.gain) * smooth;
                st.q    += (b.q - st.q) * smooth;

                const bool dynamic = b.dyn && b.hasGain();
                if (! dynamic) st.dynDb = 0.0f;

                const auto d = design (b, st.freq, st.gain + st.dynDb, st.q, sr);
                for (auto& ch : st.filters)
                    for (int s = 0; s < d.n; ++s)
                        *ch[(size_t) s].coefficients = d.st[(size_t) s];

                if (dynamic)
                {
                    const float f = juce::jlimit (20.0f, (float) (sr * 0.45), st.freq);
                    const auto bp = AC::makeBandPass (sr, f, juce::jmax (0.7f, st.q));
                    for (auto& det : st.detector) *det.coefficients = bp;
                }

                if (msBand)
                {
                    // encode to mid/side, filter only the chosen half, decode (stereo bands never take this path)
                    auto* l = buffer.getWritePointer (0, start);
                    auto* r = buffer.getWritePointer (1, start);
                    auto& filters = st.filters[0];
                    for (int i = 0; i < len; ++i)
                    {
                        const float m = 0.5f * (l[i] + r[i]), sd = 0.5f * (l[i] - r[i]);
                        float v = place == 1 ? m : sd;
                        if (dynamic)
                        {
                            auto& e = st.env[0];
                            const float side = std::abs (st.detector[0].processSample (v));
                            e = side > e ? envAtt * e + (1.0f - envAtt) * side
                                         : envRel * e + (1.0f - envRel) * side;
                        }
                        for (int s = 0; s < d.n; ++s)
                            v = filters[(size_t) s].processSample (v);
                        const float m2 = place == 1 ? v : m, s2 = place == 1 ? sd : v;
                        l[i] = m2 + s2;
                        r[i] = m2 - s2;
                    }
                }
                else
                for (int c = 0; c < numCh; ++c)
                {
                    auto* x = buffer.getWritePointer (c, start);
                    auto& filters = st.filters[(size_t) c];

                    for (int i = 0; i < len; ++i)
                    {
                        float v = x[i];

                        if (dynamic)
                        {
                            auto& e = st.env[(size_t) c];
                            const float side = std::abs (st.detector[(size_t) c].processSample (v));
                            e = side > e ? envAtt * e + (1.0f - envAtt) * side
                                         : envRel * e + (1.0f - envRel) * side;
                        }

                        for (int s = 0; s < d.n; ++s)
                            v = filters[(size_t) s].processSample (v);
                        x[i] = v;
                    }
                }

                if (dynamic)
                {
                    // Full range is reached 10 dB over the threshold
                    const float env = numCh > 1 ? juce::jmax (st.env[0], st.env[1]) : st.env[0];
                    const float over = juce::Decibels::gainToDecibels (env, -120.0f) - b.dynThresh;
                    const float target = b.dynRange * juce::jlimit (0.0f, 1.0f, over / 10.0f);
                    const float coef = std::abs (target) > std::abs (st.dynDb) ? dynAtt : dynRel;
                    st.dynDb = coef * st.dynDb + (1.0f - coef) * target;
                }

                dynOffsetDb[(size_t) bi].store (st.dynDb);
            }
        }
    }

    // Current dynamic gain movement per band (dB), for the display
    std::array<std::atomic<float>, numCleanBands> dynOffsetDb {};

private:
    float msToCoefLocal (float ms) const { return (float) std::exp (-1.0 / (ms * 0.001 * sr)); }

    struct BandState
    {
        std::array<std::array<juce::dsp::IIR::Filter<float>, maxStages>, 2> filters;
        std::array<juce::dsp::IIR::Filter<float>, 2> detector;
        float freq = 1000.0f, gain = 0.0f, q = 1.0f;
        std::array<float, 2> env {};
        float dynDb = 0.0f;
        BandType type = BandType::Bell;
        int slope = 1, place = 0;
        bool initialised = false;
    };

    double sr = 48000.0;
    std::array<BandState, numCleanBands> state;
};

//==============================================================================
// Lock-free tap that hands audio to the spectrum analyzer on the UI thread
class AnalyzerTap
{
public:
    static constexpr int size = 16384;

    void push (const juce::AudioBuffer<float>& b)
    {
        const int n = b.getNumSamples();
        const int numCh = juce::jmin (2, b.getNumChannels());
        if (numCh == 0) return;
        const float scale = 1.0f / (float) numCh;

        const auto scope = fifo.write (juce::jmin (n, fifo.getFreeSpace()));
        auto copy = [&] (int dest, int count, int srcOffset)
        {
            for (int i = 0; i < count; ++i)
            {
                float v = 0.0f;
                for (int c = 0; c < numCh; ++c) v += b.getSample (c, srcOffset + i);
                data[(size_t) (dest + i)] = v * scale;
            }
        };
        copy (scope.startIndex1, scope.blockSize1, 0);
        copy (scope.startIndex2, scope.blockSize2, scope.blockSize1);
    }

    // UI thread: appends everything available to `out` (a sliding window)
    void pull (std::vector<float>& window)
    {
        const auto scope = fifo.read (fifo.getNumReady());
        auto append = [&] (int start, int count)
        {
            if (count <= 0) return;
            const int w = (int) window.size();
            if (count >= w)
            {
                std::copy (data.begin() + start + count - w, data.begin() + start + count, window.begin());
                return;
            }
            std::move (window.begin() + count, window.end(), window.begin());
            std::copy (data.begin() + start, data.begin() + start + count, window.end() - count);
        };
        append (scope.startIndex1, scope.blockSize1);
        append (scope.startIndex2, scope.blockSize2);
    }

private:
    juce::AbstractFifo fifo { size };
    std::array<float, size> data {};
};

} // namespace vox
