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
inline constexpr int numCleanBands = 30;   // Tahor V16: thirty bands (Pro-Q 4 has 24)

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
    int      place = 0;         // V60: 0 = Stereo (both channels), 1 = Mid only, 2 = Side only (stereo tracks); Tahor: 3 = Left, 4 = Right
    bool     sc = false;        // Tahor: the dynamic detector listens to the sidechain (key) input
    bool     spectral = false;  // Tahor: spectral dynamics - a much narrower, faster detector that only catches what pokes out
    float    dynAttMs = 5.0f, dynRelMs = 120.0f;   // Tahor V9: per band
    float    specSel = 3.0f;    // Tahor V9: spectral selectivity (how much narrower the detector is)

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

    // Tahor: cut slopes 0..5 = 6, 12, 18, 24, 36, 48 dB/oct (odd orders get a first-order section first)
    static int cutOrder (int slope) { static const int o[6] { 1, 2, 3, 4, 6, 8 }; return o[juce::jlimit (0, 5, slope)]; }
    // Butterworth biquad Qs for an order (the first-order section, if any, is not listed)
    static int cutQs (int order, double* qs)
    {
        switch (order)
        {
            case 1: return 0;
            case 2: qs[0] = 0.7071; return 1;
            case 3: qs[0] = 1.0; return 1;
            case 4: qs[0] = 0.5412; qs[1] = 1.3066; return 2;
            case 6: qs[0] = 0.5176; qs[1] = 0.7071; qs[2] = 1.9319; return 3;
            default: qs[0] = 0.5098; qs[1] = 0.6013; qs[2] = 0.9000; qs[3] = 2.5629; return 4;
        }
    }
    static int stagesFor (const CleanBand& b) { if (! b.isCut()) return 1; const int o = cutOrder (b.slope); return o / 2 + o % 2; }

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
                // Butterworth: the band's Q scales the most resonant stage, so Q 0.71 = flat Butterworth
                const int order = cutOrder (b.slope);
                double qs[4]; const int nb = cutQs (order, qs);
                const float scale = q / 0.7071f;
                int n = 0;
                if (order % 2 == 1)
                {
                    auto fo = b.type == BandType::LowCut ? AC::makeFirstOrderHighPass (sr, f) : AC::makeFirstOrderLowPass (sr, f);
                    d.st[(size_t) n++] = { fo[0], fo[1], 0.0f, fo[2], fo[3], 0.0f };
                }
                for (int s = 0; s < nb; ++s)
                {
                    float sq = (float) qs[s];
                    if (s == nb - 1) sq *= scale;
                    d.st[(size_t) n++] = b.type == BandType::LowCut ? AC::makeHighPass (sr, f, sq) : AC::makeLowPass (sr, f, sq);
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
            for (auto& det : band.detW)
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
            for (auto& det : band.detW) det.reset();
            band.env = {}; band.envW = {}; band.dynDb = 0.0f; band.initialised = false;
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
                const int place = juce::jlimit (0, 4, b.place);
                const bool msBand = (place == 1 || place == 2) && numCh > 1;

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
                    for (auto& det : st.detW) det.reset();
                    st.envW = {};
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
                    const auto bp = AC::makeBandPass (sr, f, juce::jmax (0.7f, st.q) * (b.spectral ? juce::jlimit (1.0f, 10.0f, b.specSel) : 1.0f));
                    for (auto& det : st.detector) *det.coefficients = bp;
                    if (b.spectral) { const auto wide = AC::makeBandPass (sr, f, 0.5f); for (auto& det : st.detW) *det.coefficients = wide; }
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
                            const float kin = keyAt (b, 0, start + i, v);
                            const float side = std::abs (st.detector[0].processSample (kin));
                            e = side > e ? envAtt * e + (1.0f - envAtt) * side
                                         : envRel * e + (1.0f - envRel) * side;
                            if (b.spectral) { auto& w = st.envW[0]; const float sw = std::abs (st.detW[0].processSample (kin));
                                              w = sw > w ? envAtt * w + (1.0f - envAtt) * sw : envRel * w + (1.0f - envRel) * sw; }
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
                    if (place >= 3 && numCh > 1 && c != place - 3) continue;   // Tahor: a Left or Right band
                    auto* x = buffer.getWritePointer (c, start);
                    auto& filters = st.filters[(size_t) c];

                    for (int i = 0; i < len; ++i)
                    {
                        float v = x[i];

                        if (dynamic)
                        {
                            auto& e = st.env[(size_t) c];
                            const float kin = keyAt (b, c, start + i, v);
                            const float side = std::abs (st.detector[(size_t) c].processSample (kin));
                            e = side > e ? envAtt * e + (1.0f - envAtt) * side
                                         : envRel * e + (1.0f - envRel) * side;
                            if (b.spectral) { auto& w = st.envW[(size_t) c]; const float sw = std::abs (st.detW[(size_t) c].processSample (kin));
                                              w = sw > w ? envAtt * w + (1.0f - envAtt) * sw : envRel * w + (1.0f - envRel) * sw; }
                        }

                        for (int s = 0; s < d.n; ++s)
                            v = filters[(size_t) s].processSample (v);
                        x[i] = v;
                    }
                }

                if (dynamic)
                {
                    // Full range is reached 10 dB over the threshold
                    const int ci = place == 4 && numCh > 1 ? 1 : 0;
                    const bool both = numCh > 1 && place != 3 && place != 4;
                    const float env = both ? juce::jmax (st.env[0], st.env[1]) : st.env[(size_t) ci];
                    float over = juce::Decibels::gainToDecibels (env, -120.0f) - b.dynThresh;
                    if (b.spectral)   // V9: only what pokes out above its surroundings (a tone, a ring) - not the whole band
                    {
                        const float envW = both ? juce::jmax (st.envW[0], st.envW[1]) : st.envW[(size_t) ci];
                        const float pokes = juce::Decibels::gainToDecibels (env, -120.0f) - juce::Decibels::gainToDecibels (envW, -120.0f) + 6.0f;
                        over = juce::jmin (over, pokes * 1.7f);
                    }
                    const float target = b.dynRange * juce::jlimit (0.0f, 1.0f, over / 10.0f);
                    const float bAtt = std::exp (-(float) sub / (juce::jmax (0.1f, b.dynAttMs) * 0.001f * (float) sr));
                    const float bRel = std::exp (-(float) sub / (juce::jmax (1.0f, b.dynRelMs) * 0.001f * (float) sr));
                    juce::ignoreUnused (dynAtt, dynRel);
                    const float coef = std::abs (target) > std::abs (st.dynDb) ? bAtt : bRel;
                    st.dynDb = coef * st.dynDb + (1.0f - coef) * target;
                }

                dynOffsetDb[(size_t) bi].store (st.dynDb);
            }
        }
    }

    // Current dynamic gain movement per band (dB), for the display
    std::array<std::atomic<float>, numCleanBands> dynOffsetDb {};

    // Tahor: the sidechain (key) input for bands set to SIDECHAIN; keyStep = oversampling factor of this EQ
    const float* keyL = nullptr; const float* keyR = nullptr; int keyStep = 1, keyLen = 0;

private:
    float keyAt (const CleanBand& b, int c, int i, float own) const
    {
        if (! b.sc || keyL == nullptr) return own;
        const int k = juce::jlimit (0, juce::jmax (0, keyLen - 1), i / juce::jmax (1, keyStep));
        return (c == 1 && keyR != nullptr ? keyR : keyL)[k];
    }

    float msToCoefLocal (float ms) const { return (float) std::exp (-1.0 / (ms * 0.001 * sr)); }

    struct BandState
    {
        std::array<std::array<juce::dsp::IIR::Filter<float>, maxStages>, 2> filters;
        std::array<juce::dsp::IIR::Filter<float>, 2> detector, detW;   // V9: detW = the wide "surroundings" detector (spectral)
        float freq = 1000.0f, gain = 0.0f, q = 1.0f;
        std::array<float, 2> env {}, envW {};
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
