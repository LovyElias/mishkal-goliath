#pragma once

// Tahor EQ processing modes.
//
//   Natural    the minimum-phase IIR bands (CleanupEQ), zero latency. Optional 2x / 4x
//              oversampling removes "cramping": without it, bells and shelves near
//              20 kHz get squashed by the digital filter design.
//   Linear     a linear-phase FIR built from the exact analog band shapes: no phase
//   Linear HQ  shift at all, no cramping, finer low-frequency resolution in HQ.
//              Runs as a partitioned FFT convolution; new curves crossfade in.
//              Dynamic bands keep working: their detectors run on the input and the
//              gain moves are delayed to line up with the delayed audio.

#include "CleanupEQ.h"
#include <complex>
#include <vector>

namespace vox
{

enum class TahorPhase { Natural = 0, Linear, LinearHQ };

//==============================================================================
// Exact analog (s-plane) magnitude of one band, in dB. Same shapes and Q
// conventions as the IIR designs in CleanupEQ, minus the high-frequency cramping.
inline double tahorAnalogBandDb (const CleanBand& b, double freq, double gainDb, double q, double hz)
{
    const double f0 = juce::jlimit (10.0, 40000.0, freq);
    q = juce::jlimit (0.05, 30.0, q);
    const double x = hz / f0, x2 = x * x, d = (1.0 - x2) * (1.0 - x2);
    auto db = [] (double p) { return 10.0 * std::log10 (juce::jmax (1.0e-30, p)); };   // power -> dB

    switch (b.type)
    {
        case BandType::Bell:
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            return db ((d + x2 * (A / q) * (A / q)) / (d + x2 / ((A * q) * (A * q))));
        }
        case BandType::LowShelf:
        case BandType::HighShelf:
        {
            const double A = std::pow (10.0, gainDb / 40.0), sA = std::sqrt (A);
            // low shelf:  A (s^2 + sA/Q s + A) / (A s^2 + sA/Q s + 1);  high shelf: the mirror image
            const double k = x2 * (sA / q) * (sA / q);
            const double nLo = (A - x2) * (A - x2) + k, dLo = (1.0 - A * x2) * (1.0 - A * x2) + k;
            const double nHi = (1.0 - A * x2) * (1.0 - A * x2) + k, dHi = (A - x2) * (A - x2) + k;
            return b.type == BandType::LowShelf ? db (A * A * nLo / dLo) : db (A * A * nHi / dHi);
        }
        case BandType::Notch:
            return db (d / (d + x2 / (q * q)));
        case BandType::LowCut:
        case BandType::HighCut:
        {
            const int order = CleanupEQ::cutOrder (b.slope);
            double qs[CleanupEQ::maxStages]; const int n = CleanupEQ::cutQs (order, qs);
            double total = 0.0;
            if (order % 2 == 1) total += b.type == BandType::LowCut ? db (x2 / (1.0 + x2)) : db (1.0 / (1.0 + x2));
            for (int st = 0; st < n; ++st)
            {
                double sq = qs[st];
                if (st == n - 1) sq *= q / 0.7071;
                const double den = d + x2 / (sq * sq);
                total += b.type == BandType::LowCut ? db (x2 * x2 / den) : db (1.0 / den);
            }
            return total;
        }
    }
    return 0.0;
}

//==============================================================================
// The same shapes as linear power ratios over many frequencies at once
// (constants hoisted, no logs or powers per point): used to design the FIR.
inline void tahorAnalogBandPower (const CleanBand& b, double freq, double gainDb, double q,
                                  const double* hz, float* outPow, int n)
{
    const double f0 = juce::jlimit (10.0, 40000.0, freq), inv = 1.0 / f0;
    q = juce::jlimit (0.05, 30.0, q);
    switch (b.type)
    {
        case BandType::Bell:
        {
            const double A = std::pow (10.0, gainDb / 40.0), a = (A / q) * (A / q), c = 1.0 / ((A * q) * (A * q));
            for (int i = 0; i < n; ++i) { const double x2 = hz[i] * inv * hz[i] * inv, d = (1.0 - x2) * (1.0 - x2);
                                          outPow[i] = (float) ((d + x2 * a) / (d + x2 * c)); }
            return;
        }
        case BandType::LowShelf:
        case BandType::HighShelf:
        {
            const double A = std::pow (10.0, gainDb / 40.0), kq = (std::sqrt (A) / q) * (std::sqrt (A) / q), A2 = A * A;
            const bool lo = b.type == BandType::LowShelf;
            for (int i = 0; i < n; ++i)
            {
                const double x2 = hz[i] * inv * hz[i] * inv, k = x2 * kq;
                const double p1 = (A - x2) * (A - x2) + k, p2 = (1.0 - A * x2) * (1.0 - A * x2) + k;
                outPow[i] = (float) (A2 * (lo ? p1 / p2 : p2 / p1));
            }
            return;
        }
        case BandType::Notch:
        {
            const double c = 1.0 / (q * q);
            for (int i = 0; i < n; ++i) { const double x2 = hz[i] * inv * hz[i] * inv, d = (1.0 - x2) * (1.0 - x2);
                                          outPow[i] = (float) (d / juce::jmax (1.0e-30, d + x2 * c)); }
            return;
        }
        case BandType::LowCut:
        case BandType::HighCut:
        {
            const int order = CleanupEQ::cutOrder (b.slope);
            double qs[CleanupEQ::maxStages]; const int stages = CleanupEQ::cutQs (order, qs);
            double cs[CleanupEQ::maxStages];
            for (int st = 0; st < stages; ++st) { double sq = qs[st]; if (st == stages - 1) sq *= q / 0.7071; cs[st] = 1.0 / (sq * sq); }
            const bool lc = b.type == BandType::LowCut;
            for (int i = 0; i < n; ++i)
            {
                const double x2 = hz[i] * inv * hz[i] * inv, d = (1.0 - x2) * (1.0 - x2), num = lc ? x2 * x2 : 1.0;
                double pw = order % 2 == 1 ? (lc ? x2 / (1.0 + x2) : 1.0 / (1.0 + x2)) : 1.0;
                for (int st = 0; st < stages; ++st) pw *= num / juce::jmax (1.0e-30, d + x2 * cs[st]);
                outPow[i] = (float) pw;
            }
            return;
        }
    }
    for (int i = 0; i < n; ++i) outPow[i] = 1.0f;
}

//==============================================================================
class LinearPhaseEQ
{
public:
    // kernelBase: kernel length at 48 kHz (scaled to the sample rate, power of two)
    void prepare (double sampleRate, int kernelBase)
    {
        sr = sampleRate;
        N = juce::nextPowerOfTwo (juce::jmax (512, (int) std::round (kernelBase * sr / 48000.0)));
        P = juce::nextPowerOfTwo (juce::jmax (64, (int) std::round (256.0 * sr / 48000.0)));
        K = N / P;
        bins = N / 2 + 1;

        designFft = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 ((double) N)));
        blockFft  = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 ((double) (2 * P))));
        designBuf.assign ((size_t) (2 * N), 0.0f);
        for (auto& k : kernel) k.assign ((size_t) N, 0.0f);
        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i)   // Blackman window centred on the kernel's middle
        {
            const double t = (double) i / (double) N;
            window[(size_t) i] = (float) (0.42 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * t)
                                               + 0.08 * std::cos (2.0 * juce::MathConstants<double>::twoPi * t));
        }
        bandDb.assign ((size_t) numCleanBands, std::vector<float> ((size_t) bins, 1.0f));
        totalDb.assign ((size_t) bins, 1.0f);
        binHz.resize ((size_t) bins);
        for (int k = 0; k < bins; ++k) binHz[(size_t) k] = juce::jmax (1.0, (double) k * sr / (double) N);

        const size_t spec = (size_t) (P + 1);
        for (int c = 0; c < 2; ++c)
            for (auto* H : { &Hcur[(size_t) c], &Hold[(size_t) c], &Hnext[(size_t) c] }) H->assign ((size_t) K * spec, {});
        for (int c = 0; c < 2; ++c)
        {
            fdl[(size_t) c].assign ((size_t) K * spec, {});
            frame[(size_t) c].assign ((size_t) (2 * P), 0.0f);
            outBlk[(size_t) c].assign ((size_t) P, 0.0f);
        }
        work.assign ((size_t) (4 * P), 0.0f);
        workB.assign ((size_t) (4 * P), 0.0f);
        acc.assign (spec, {});
        accOld.assign (spec, {});

        // dynamic bands: detectors on the input + a delay that lines the gain moves up with the audio
        // (minus the blocks a redesign takes to come in, so the gain lands on time)
        dynDelayBlocks = juce::jmax (1, (N / 2 + P) / P - (1 + (K + partsPerBlock - 1) / partsPerBlock));
        for (auto& d : dynHist) d.assign ((size_t) dynDelayBlocks + 1, 0.0f);
        for (auto* set : { &detectors, &detW })
            for (auto& det : *set)
                for (auto& f : det)
                {
                    f.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
                    f.prepare ({ sr, 4096, 1 });
                }
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (fdl[(size_t) c].begin(), fdl[(size_t) c].end(), std::complex<float> {});
            std::fill (frame[(size_t) c].begin(), frame[(size_t) c].end(), 0.0f);
            std::fill (outBlk[(size_t) c].begin(), outBlk[(size_t) c].end(), 0.0f);
        }
        head = 0; pos = 0;
        for (auto& det : detectors) for (auto& f : det) f.reset();
        for (auto& det : detW) for (auto& f : det) f.reset();
        for (auto& e : env) e = {};
        for (auto& e : envW) e = {};
        for (auto& d : dynDb) d = 0.0f;
        for (auto& h : dynHist) std::fill (h.begin(), h.end(), 0.0f);
        histPos = 0;
        for (auto& k : keys) k = {};
        keysValid = false;
        fadePending = false;
        designStage = 0; nextPart = 0;
        blocksSinceDesign = 1 << 20;
        // start from a pure delay (flat), so the first real curve fades in from silence-free audio
        designFlat();
        Hold = Hcur;
        msKernels = false;
        for (auto& d : dynOffsetDb) d.store (0.0f);
    }

    int getLatencySamples() const { return N / 2 + P; }
    int getKernelLength() const { return N; }

    // on = the Tahor EQ power switch; off fades to a flat (pure delay) kernel so latency never changes
    // V60: msEnabled = convolve in the mid/side domain (stereo tracks), so bands can sit on Mid or Side only.
    // With no Mid/Side bands both halves use the same kernel, which is identical to plain stereo.
    bool msEnabled = false;

    void process (juce::AudioBuffer<float>& buffer, const CleanBands& bands, bool on)
    {
        const int n = buffer.getNumSamples();
        const int nc = juce::jmin (2, buffer.getNumChannels());
        if (nc == 0) return;
        const float envAtt = (float) std::exp (-1.0 / (0.003 * sr)), envRel = (float) std::exp (-1.0 / (0.080 * sr));
        const bool ms = msEnabled && nc == 2;
        monoNow = nc < 2;
        if (ms != msDomain) { msDomain = ms; reset(); }   // only when the track layout changes

        for (int i = 0; i < n; ++i)
        {
            const float l = buffer.getSample (0, i), r = buffer.getSample (nc - 1, i);
            const float m = 0.5f * (l + r), sd = 0.5f * (l - r);
            for (int c = 0; c < 2; ++c)
            {
                const float x = c == 0 ? l : r;
                frame[(size_t) c][(size_t) (P + pos)] = ms ? (c == 0 ? m : sd) : x;

                // dynamic band detectors (same timing as the Natural mode)
                if (on)
                    for (int b = 0; b < numCleanBands; ++b)
                    {
                        if (! isDynamic (bands[(size_t) b])) continue;
                        const int place = placeOf (bands[(size_t) b], nc);
                        if (place != 0 && c == 1) continue;   // Mid / Side bands listen to one signal
                        float in = place == 1 ? m : place == 2 ? sd : x;
                        if (bands[(size_t) b].sc && keyL != nullptr)   // Tahor: SIDECHAIN band
                        {
                            const int k = juce::jlimit (0, juce::jmax (0, keyLen - 1), i);
                            const float kl = keyL[k], kr = keyR != nullptr ? keyR[k] : kl;
                            in = place == 1 ? 0.5f * (kl + kr) : place == 2 ? 0.5f * (kl - kr) : (c == 0 ? kl : kr);
                        }
                        const float side = std::abs (detectors[(size_t) b][(size_t) c].processSample (in));
                        auto& e = env[(size_t) b][(size_t) c];
                        e = side > e ? envAtt * e + (1.0f - envAtt) * side : envRel * e + (1.0f - envRel) * side;
                        if (bands[(size_t) b].spectral)
                        {
                            const float sw = std::abs (detW[(size_t) b][(size_t) c].processSample (in));
                            auto& w = envW[(size_t) b][(size_t) c];
                            w = sw > w ? envAtt * w + (1.0f - envAtt) * sw : envRel * w + (1.0f - envRel) * sw;
                        }
                    }
            }
            if (ms)
            {
                const float mo = outBlk[0][(size_t) pos], so = outBlk[1][(size_t) pos];
                buffer.getWritePointer (0)[i] = mo + so;
                buffer.getWritePointer (1)[i] = mo - so;
            }
            else
                for (int c = 0; c < nc; ++c)
                    buffer.getWritePointer (c)[i] = outBlk[(size_t) c][(size_t) pos];

            if (++pos == P)
            {
                pos = 0;
                processBlock (bands, on);
            }
        }
    }

    // Current dynamic movement per band (dB), for the display
    std::array<std::atomic<float>, numCleanBands> dynOffsetDb {};
    const float* keyL = nullptr; const float* keyR = nullptr; int keyLen = 0;   // Tahor: sidechain input

private:
    static bool isDynamic (const CleanBand& b) { return b.on && b.dyn && b.hasGain(); }
    // mono: everything is mid (a Side band has nothing to act on)
    static int placeOf (const CleanBand& b, int nc) { const int p = juce::jlimit (0, 2, b.place); return nc < 2 ? (p == 2 ? -1 : 0) : p; }

    struct Key { bool on = false; int type = -1, slope = -1, place = 0; float freq = 0, gain = 0, q = 0; bool operator!= (const Key& o) const
        { return on != o.on || type != o.type || slope != o.slope || place != o.place || std::abs (freq - o.freq) > 1.0e-3f * freq
              || std::abs (gain - o.gain) > 0.02f || std::abs (q - o.q) > 1.0e-3f * q; } };

    void processBlock (const CleanBands& bands, bool on)
    {
        const float dynAtt = (float) std::exp (-(double) P / (0.005 * sr));
        const float dynRel = (float) std::exp (-(double) P / (0.120 * sr));

        // 1. dynamic gain per band, delayed so it lines up with the audio coming out of the FIR
        for (int b = 0; b < numCleanBands; ++b)
        {
            const auto& bd = bands[(size_t) b];
            float target = 0.0f;
            if (on && isDynamic (bd))
            {
                // detector: band-pass at the band's frequency (redesigned only when it moves)
                const float f = juce::jlimit (20.0f, (float) (sr * 0.45), bd.freq);
                const float dq = juce::jmax (0.7f, bd.q) * (bd.spectral ? juce::jlimit (1.0f, 10.0f, bd.specSel) : 1.0f);
                if (std::abs (f - detF[(size_t) b]) > 0.5f || std::abs (dq - detQ[(size_t) b]) > 1.0e-3f)
                {
                    detF[(size_t) b] = f; detQ[(size_t) b] = dq;
                    const auto bp = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, f, dq);
                    for (auto& flt : detectors[(size_t) b]) *flt.coefficients = bp;
                    const auto wide = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, f, 0.5f);
                    for (auto& flt : detW[(size_t) b]) *flt.coefficients = wide;
                }
                const float e = juce::jmax (env[(size_t) b][0], env[(size_t) b][1]);
                float over = juce::Decibels::gainToDecibels (e, -120.0f) - bd.dynThresh;
                if (bd.spectral)   // V9: only what pokes out above its surroundings
                {
                    const float w = juce::jmax (envW[(size_t) b][0], envW[(size_t) b][1]);
                    over = juce::jmin (over, (juce::Decibels::gainToDecibels (e, -120.0f) - juce::Decibels::gainToDecibels (w, -120.0f) + 6.0f) * 1.7f);
                }
                target = bd.dynRange * juce::jlimit (0.0f, 1.0f, over / 10.0f);
            }
            juce::ignoreUnused (dynAtt, dynRel);
            const float bAtt = (float) std::exp (-(double) P / (juce::jmax (0.1, (double) bd.dynAttMs) * 0.001 * sr));
            const float bRel = (float) std::exp (-(double) P / (juce::jmax (1.0, (double) bd.dynRelMs) * 0.001 * sr));
            const float coef = std::abs (target) > std::abs (dynDb[(size_t) b]) ? bAtt : bRel;
            dynDb[(size_t) b] = coef * dynDb[(size_t) b] + (1.0f - coef) * target;
            dynHist[(size_t) b][(size_t) histPos] = dynDb[(size_t) b];
        }
        const int readPos = (histPos + 1) % (dynDelayBlocks + 1);   // the oldest entry = dynDelayBlocks ago
        histPos = readPos;

        // 2. redesign the kernel when anything moved (dynamic-only moves are rate-limited)
        bool changed = false, staticChange = false;
        std::array<Key, numCleanBands> now;
        for (int b = 0; b < numCleanBands; ++b)
        {
            const auto& bd = bands[(size_t) b];
            const float dyn = isDynamic (bd) ? dynHist[(size_t) b][(size_t) readPos] : 0.0f;
            dynOffsetDb[(size_t) b].store (dyn);
            Key k;
            k.on = on && bd.on;
            {
                const int pl = juce::jlimit (0, 2, bd.place);
                k.place = msDomain ? pl : 0;                     // stereo without M/S, or mono: one shared curve
                if (! msDomain && pl == 2 && monoNow) k.on = false;   // mono: a Side band has nothing to act on
            }
            if (! k.on) k.place = 0;
            if (k.on) { k.type = (int) bd.type; k.slope = bd.slope; k.freq = bd.freq; k.gain = bd.gain + dyn; k.q = bd.q; }
            now[(size_t) b] = k;
            if (! keysValid || k != keys[(size_t) b])
            {
                changed = true;
                const bool dynOnly = keysValid && k.on && keys[(size_t) b].on && k.type == keys[(size_t) b].type && k.place == keys[(size_t) b].place
                                     && k.freq == keys[(size_t) b].freq && k.q == keys[(size_t) b].q && k.slope == keys[(size_t) b].slope;
                staticChange = staticChange || ! dynOnly;
            }
        }
        ++blocksSinceDesign;
        const int dynEvery = juce::jmax (1, (int) std::round (0.01 * sr / P));   // dynamic curves update ~every 10 ms
        // A redesign is spread over a few partition blocks so no single audio callback spikes:
        // block 1 builds the new kernel, the next ones transform it into partition spectra (16 at a time).
        if (designStage > 0)
        {
            const int end = juce::jmin (K, nextPart + partsPerBlock);
            transformPartitions (nextPart, end, Hnext);
            nextPart = end;
            if (nextPart >= K)
            {
                std::swap (Hold, Hcur);    // the outgoing curve, for the crossfade
                std::swap (Hcur, Hnext);   // the new curve
                msKernels = msKernelsNext;
                fadePending = true;
                designStage = 0;
            }
        }
        else if (changed && (staticChange || blocksSinceDesign >= dynEvery))
        {
            for (int b = 0; b < numCleanBands; ++b)
                if (! keysValid || now[(size_t) b] != keys[(size_t) b])
                {
                    updateBandDb (b, bands[(size_t) b], now[(size_t) b]);
                    keys[(size_t) b] = now[(size_t) b];
                }
            keysValid = true;
            buildKernel();
            designStage = 1;
            nextPart = 0;
            blocksSinceDesign = 0;
        }

        // 3. convolve: new input frame -> spectrum -> frequency-domain delay line -> sum of partitions
        const size_t spec = (size_t) (P + 1);
        for (int c = 0; c < 2; ++c)
        {
            auto& fr = frame[(size_t) c];
            std::fill (work.begin(), work.end(), 0.0f);
            std::copy (fr.begin(), fr.end(), work.begin());
            blockFft->performRealOnlyForwardTransform (work.data(), true);
            auto* X = reinterpret_cast<std::complex<float>*> (work.data());
            std::copy (X, X + spec, fdl[(size_t) c].begin() + (ptrdiff_t) ((size_t) head * spec));

            std::fill (acc.begin(), acc.end(), std::complex<float> {});
            if (fadePending) std::fill (accOld.begin(), accOld.end(), std::complex<float> {});
            for (int k = 0; k < K; ++k)
            {
                const int slot = (head - k + K) % K;
                const auto* xs = fdl[(size_t) c].data() + (size_t) slot * spec;
                const auto* hs = Hcur[(size_t) c].data() + (size_t) k * spec;
                for (size_t j = 0; j < spec; ++j) acc[j] += xs[j] * hs[j];
                if (fadePending)
                {
                    const auto* ho = Hold[(size_t) c].data() + (size_t) k * spec;
                    for (size_t j = 0; j < spec; ++j) accOld[j] += xs[j] * ho[j];
                }
            }
            inverse (acc, work);
            auto& out = outBlk[(size_t) c];
            if (fadePending)
            {
                inverse (accOld, workB);
                for (int i = 0; i < P; ++i)
                {
                    const float w = ((float) i + 0.5f) / (float) P;
                    out[(size_t) i] = workB[(size_t) (P + i)] + w * (work[(size_t) (P + i)] - workB[(size_t) (P + i)]);
                }
            }
            else
                for (int i = 0; i < P; ++i) out[(size_t) i] = work[(size_t) (P + i)];

            // slide the frame: the new half becomes the old half
            std::copy (fr.begin() + P, fr.end(), fr.begin());
        }
        fadePending = false;
        head = (head + 1) % K;
    }

    void inverse (const std::vector<std::complex<float>>& spectrum, std::vector<float>& dst)
    {
        std::fill (dst.begin(), dst.end(), 0.0f);
        auto* Y = reinterpret_cast<std::complex<float>*> (dst.data());
        std::copy (spectrum.begin(), spectrum.end(), Y);
        blockFft->performRealOnlyInverseTransform (dst.data());
    }

    void updateBandDb (int b, const CleanBand& bd, const Key& k)
    {
        auto& arr = bandDb[(size_t) b];   // linear power ratio per bin
        if (! k.on) { std::fill (arr.begin(), arr.end(), 1.0f); return; }
        tahorAnalogBandPower (bd, k.freq, k.gain, k.q, binHz.data(), arr.data(), bins);
    }

    void designFlat()
    {
        for (auto& arr : bandDb) std::fill (arr.begin(), arr.end(), 1.0f);
        buildKernel();
        transformPartitions (0, K, Hcur);
        designStage = 0;
    }

    // zero-phase spectrum -> symmetric impulse -> centred, windowed kernel
    // kernel 0 = Stereo + Mid bands, kernel 1 = Stereo + Side bands (only built when a band sits on Mid or Side)
    void buildKernel()
    {
        msKernelsNext = false;
        for (const auto& k : keys) if (k.on && k.place > 0) msKernelsNext = true;
        for (int c = 0; c < (msKernelsNext ? 2 : 1); ++c)
        {
            std::fill (totalDb.begin(), totalDb.end(), 1.0f);   // product of every band's power ratio
            for (int b = 0; b < numCleanBands; ++b)
            {
                const int pl = keys[(size_t) b].place;
                if (pl > 0 && pl != c + 1) continue;   // a Mid band stays out of the side kernel and vice versa
                juce::FloatVectorOperations::multiply (totalDb.data(), bandDb[(size_t) b].data(), bins);
            }

            std::fill (designBuf.begin(), designBuf.end(), 0.0f);
            auto* S = reinterpret_cast<std::complex<float>*> (designBuf.data());
            for (int i = 0; i < bins; ++i)
                S[i] = { std::sqrt (juce::jlimit (1.0e-12f, 1.0e4f, totalDb[(size_t) i])), 0.0f };   // -120 .. +40 dB
            designFft->performRealOnlyInverseTransform (designBuf.data());
            auto& kn = kernel[(size_t) c];
            for (int i = 0; i < N; ++i)   // circular shift by N/2 so the impulse sits in the middle
                kn[(size_t) i] = designBuf[(size_t) ((i + N / 2) % N)] * window[(size_t) i];
        }
    }

    // kernel partitions [from, to) -> their spectra
    void transformPartitions (int from, int to, std::array<std::vector<std::complex<float>>, 2>& dst)
    {
        const size_t spec = (size_t) (P + 1);
        for (int c = 0; c < (msKernelsNext ? 2 : 1); ++c)
            for (int k = from; k < to; ++k)
            {
                std::fill (work.begin(), work.end(), 0.0f);
                std::copy (kernel[(size_t) c].begin() + k * P, kernel[(size_t) c].begin() + (k + 1) * P, work.begin());
                blockFft->performRealOnlyForwardTransform (work.data(), true);
                const auto* H = reinterpret_cast<std::complex<float>*> (work.data());
                std::copy (H, H + spec, dst[(size_t) c].begin() + (ptrdiff_t) ((size_t) k * spec));
            }
        if (! msKernelsNext)   // both halves share one curve
            std::copy (dst[0].begin() + (ptrdiff_t) ((size_t) from * spec), dst[0].begin() + (ptrdiff_t) ((size_t) to * spec),
                       dst[1].begin() + (ptrdiff_t) ((size_t) from * spec));
    }

    double sr = 48000.0;
    int N = 4096, P = 256, K = 16, bins = 2049;
    std::unique_ptr<juce::dsp::FFT> designFft, blockFft;
    std::vector<float> designBuf, window, totalDb, work, workB;
    std::array<std::vector<float>, 2> kernel;
    std::vector<double> binHz;
    std::vector<std::vector<float>> bandDb;
    std::array<std::vector<std::complex<float>>, 2> Hcur, Hold, Hnext;
    std::vector<std::complex<float>> acc, accOld;
    bool msDomain = false, msKernels = false, msKernelsNext = false, monoNow = false;
    int designStage = 0, nextPart = 0;
    static constexpr int partsPerBlock = 32;
    std::array<std::vector<std::complex<float>>, 2> fdl;
    std::array<std::vector<float>, 2> frame, outBlk;
    int head = 0, pos = 0;
    bool fadePending = false, keysValid = false;
    int blocksSinceDesign = 0;
    std::array<Key, numCleanBands> keys {};

    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, numCleanBands> detectors, detW;   // V9: detW = spectral surroundings
    std::array<std::array<float, 2>, numCleanBands> env {}, envW {};
    std::array<float, numCleanBands> dynDb {}, detF {}, detQ {};
    std::array<std::vector<float>, numCleanBands> dynHist;
    int dynDelayBlocks = 1, histPos = 0;
};

//==============================================================================
// The whole Tahor EQ stage: picks the mode, keeps the latency fixed for the
// current mode (even with the EQ switched off), and reports it.
class TahorStage
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        for (int k = 0; k < 3; ++k) natural[(size_t) k].prepare (sr * (double) (1 << k));
        lat[0] = 0;
        for (int k = 0; k < 2; ++k)
        {
            os[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (
                2, (size_t) (k + 1), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
            os[(size_t) k]->initProcessing ((size_t) juce::jmax (32, maxBlockSize));
            lat[(size_t) k + 1] = (int) std::round (os[(size_t) k]->getLatencyInSamples());
        }
        linear.prepare (sr, 4096);
        linearHQ.prepare (sr, 16384);
        reset();
    }

    void reset()
    {
        for (auto& e : natural) e.reset();
        for (auto& o : os) if (o) o->reset();
        linear.reset();
        linearHQ.reset();
    }

    bool zeroLatency = false;   // TRACK mode: always Natural, no oversampling
    const float* keyL = nullptr; const float* keyR = nullptr; int keyLen = 0;   // Tahor: sidechain input (base rate)
    bool msEnabled = false;     // V60: bands may sit on Mid or Side (stereo tracks)

    int latencyFor (int phase, int osSetting) const
    {
        switch ((TahorPhase) juce::jlimit (0, 2, phase))
        {
            case TahorPhase::Linear:   return linear.getLatencySamples();
            case TahorPhase::LinearHQ: return linearHQ.getLatencySamples();
            case TahorPhase::Natural:  break;
        }
        return lat[(size_t) juce::jlimit (0, 2, osSetting)];
    }
    int getLatencySamples() const { return zeroLatency ? 0 : latencyFor (phaseNow, osNow); }

    void process (juce::AudioBuffer<float>& buffer, const CleanBands& bands, bool on, int phase, int osSetting)
    {
        phase = zeroLatency ? 0 : juce::jlimit (0, 2, phase);
        osSetting = zeroLatency ? 0 : juce::jlimit (0, 2, osSetting);
        if (phase != phaseNow || (phase == 0 && osSetting != osNow))
        {
            phaseNow = phase; osNow = osSetting;
            reset();
        }

        linear.msEnabled = linearHQ.msEnabled = msEnabled;
        linear.keyL = linearHQ.keyL = keyL; linear.keyR = linearHQ.keyR = keyR; linear.keyLen = linearHQ.keyLen = keyLen;
        for (int k = 0; k < 3; ++k) { natural[(size_t) k].keyL = keyL; natural[(size_t) k].keyR = keyR; natural[(size_t) k].keyLen = keyLen; natural[(size_t) k].keyStep = 1 << k; }
        if (phaseNow == (int) TahorPhase::Linear)   { linear.process (buffer, bands, on);   mirrorDyn (linear.dynOffsetDb);   return; }
        if (phaseNow == (int) TahorPhase::LinearHQ) { linearHQ.process (buffer, bands, on); mirrorDyn (linearHQ.dynOffsetDb); return; }

        auto& eq = natural[(size_t) osNow];
        if (osNow == 0)
        {
            if (on) eq.process (buffer, bands);
        }
        else
        {
            // the oversampler always runs (even with the EQ off) so the latency stays put
            const int nc = juce::jmin (2, buffer.getNumChannels());
            juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nc, (size_t) buffer.getNumSamples());
            auto& o = os[(size_t) osNow - 1];
            auto up = o->processSamplesUp (block);
            if (on)
            {
                float* ptrs[2] { up.getChannelPointer (0), up.getChannelPointer ((size_t) (nc - 1)) };
                juce::AudioBuffer<float> hi (ptrs, nc, (int) up.getNumSamples());
                eq.process (hi, bands);
            }
            o->processSamplesDown (block);
        }
        if (! on) for (auto& d : dynOffsetDb) d.store (0.0f);
        else      mirrorDyn (eq.dynOffsetDb);
    }

    // the sample rate the Natural bands are designed at (for drawing the exact curve)
    double designRateFor (int phase, int osSetting) const
    {
        if (phase != 0) return 0.0;   // 0 = analog (linear-phase modes)
        return sr * (double) (1 << juce::jlimit (0, 2, osSetting));
    }

    std::array<std::atomic<float>, numCleanBands> dynOffsetDb {};

private:
    void mirrorDyn (const std::array<std::atomic<float>, numCleanBands>& src)
    {
        for (size_t i = 0; i < (size_t) numCleanBands; ++i) dynOffsetDb[i].store (src[i].load());
    }

    double sr = 48000.0;
    std::array<CleanupEQ, 3> natural;   // designed at 1x, 2x, 4x
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> os;
    std::array<int, 3> lat {};
    LinearPhaseEQ linear, linearHQ;
    int phaseNow = 0, osNow = 0;
};

} // namespace vox
