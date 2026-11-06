#pragma once

// Mishkal DSP engine: EQ -> Compressor -> De-esser -> Limiter.
// Depends only on juce_dsp so it can be tested outside a plugin host.

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>
#include "TahorEQ.h"
#include "ResonanceSuppressor.h"
#include "ToneShaper.h"
#include "EshTape.h"

namespace vox
{

//==============================================================================
enum class EqType   { TubeP1 = 0, EConsole, ClassA81, Airy, Api550, K9000, Helios69, Maag4 };   // V74: + LE-A550, LE-K9, LE-H69, LE-M4
inline constexpr int numCharEqs = 8;
enum class CompType { FetPurple = 0, RComp, VariMu660, StaLevel, Api2500, SslG, WeissDs1, SplIron, Neve33609 };   // V75: + five bus / mastering compressors
inline constexpr int numComps = 9;   // StaLevel = LE-STA (V52: its own unit)   // RComp = LE-RC (Renaissance-style compressor)
enum class LimMode  { Classic = 0, Multi };
enum class DsMode   { Split = 0, Wide };

struct CompUnitSettings
{
    bool     on   = false;
    CompType type = CompType::FetPurple;
    float    threshDb  = -18.0f;
    float    ratio     = 4.0f;
    float    attackMs  = 0.8f;
    float    releaseMs = 120.0f;
    float    makeupDb  = 0.0f;
    float    mix       = 1.0f;   // 0..1, parallel compression
    int      timeConstant = 1;   // 0..5 = Vari-Mu time constants 1..6
    float    scHpHz = 0.0f;      // sidechain high-pass on the detector (0 = off); shared by all units
    // LE-RC (Renaissance style)
    bool     arc = true;         // auto release: the release stretches under sustained compression
    int      behavior = 0;       // 0 = Glide (release slows near threshold, smooth), 1 = Snap (release speeds up near threshold, louder)
    int      character = 0;      // 0 = Color (gentle harmonics as it compresses), 1 = Pure (clean)
    bool     voxLink = true;     // RVox: linked = the output follows the threshold (automatic make-up); off = output is independent
    int      rcModel = 0;        // LE-RC: 0 = RComp (full compressor), 1 = RVox (one-knob vocal leveler with automatic make-up)
    int      fetRev = 0;         // LE-76 revision: 0 = Purple (modern), 1 = Blue Stripe (early, gritty), 2 = Black (low-noise, clean)
    // Tube slot: 0 = LE-660 (Fairchild-style), 1 = LE-STA (Sta-Level-style program level amplifier)
    int      tubeModel = 0;
    float    staInDb   = 0.0f;   // LE-STA Input: drives into its fixed threshold (-12..+24 dB)
    int      staMode   = 1;      // 0 = Single (slow, punchy), 1 = Double (the vintage behaviour), 2 = Triple (faster attack, for vocals)
    int      staRel    = 1;      // recovery: 0 = Fast, 1 = Medium, 2 = Slow, 3 = Very Slow
    // V75 bus / mastering compressors: switch positions (meaning depends on the unit) and LE-336's limiter
    std::array<int, 4> sw {};
    float    limThreshDb = 0.0f;  // LE-336 limiter threshold (0 = off)
};

// Order helpers: permutation <-> index in lexicographic order (n! entries)
// V74: the same lexicographic order, decoded directly (factorial number system) - fast enough for 8 items every block
template <size_t N>
inline std::array<int, N> permutationFromIndexFast (int index)
{
    std::vector<int> pool;
    for (size_t i = 0; i < N; ++i) pool.push_back ((int) i);
    std::array<int, N> p {};
    int fact = 1;
    for (size_t i = 2; i < N; ++i) fact *= (int) i;
    index = juce::jmax (0, index);
    for (size_t i = 0; i < N; ++i)
    {
        const int k = fact > 0 ? juce::jmin ((int) pool.size() - 1, index / fact) : 0;
        p[i] = pool[(size_t) k];
        pool.erase (pool.begin() + k);
        if (fact > 0) index %= fact;
        if (N - 1 - i > 0) fact /= juce::jmax (1, (int) (N - 1 - i));
    }
    return p;
}
template <size_t N>
inline int indexFromPermutationFast (const std::array<int, N>& order)
{
    std::vector<int> pool;
    for (size_t i = 0; i < N; ++i) pool.push_back ((int) i);
    int fact = 1;
    for (size_t i = 2; i < N; ++i) fact *= (int) i;
    int index = 0;
    for (size_t i = 0; i < N; ++i)
    {
        const auto it = std::find (pool.begin(), pool.end(), order[i]);
        const int k = (int) (it - pool.begin());
        index += k * fact;
        if (it != pool.end()) pool.erase (it);
        if (N - 1 - i > 0) fact /= juce::jmax (1, (int) (N - 1 - i));
    }
    return index;
}
template <size_t N>
inline std::array<int, N> permutationFromIndex (int index)
{
    std::array<int, N> p {};
    for (size_t i = 0; i < N; ++i) p[i] = (int) i;
    for (int k = 0; k < index && std::next_permutation (p.begin(), p.end()); ++k) {}
    return p;
}

template <size_t N>
inline int indexFromPermutation (const std::array<int, N>& order)
{
    std::array<int, N> p {};
    for (size_t i = 0; i < N; ++i) p[i] = (int) i;
    int index = 0;
    do { if (p == order) return index; ++index; } while (std::next_permutation (p.begin(), p.end()));
    return 0;
}

struct CharEqSettings
{
    bool  on     = false;
    float hpfHz  = 80.0f;
    float bodyHz = 180.0f,  bodyDb = 0.0f;                 // low shelf
    float mudHz  = 350.0f,  mudDb  = 0.0f, mudQ  = 1.2f;   // bell
    float presHz = 3500.0f, presDb = 0.0f, presQ = 0.9f;   // bell
    float airHz  = 12000.0f, airDb = 0.0f;                 // high shelf
    float silk   = 0.25f;                                  // Airy only: 0..1 high-band harmonics

    // Tube P1 only (Pultec-style program EQ). Boost/atten/bandwidth are 0..10 dial positions.
    float p1LowHz = 60.0f, p1LowBoost = 0.0f, p1LowAtten = 0.0f;
    float p1HighHz = 10000.0f, p1HighBw = 5.0f, p1HighBoost = 0.0f;
    float p1AttenHz = 10000.0f, p1HighAtten = 0.0f;
    int   place = 0;                                       // V71: 0 stereo, 1 mid only, 2 side only

    // V74 models (LE-A550, LE-K9, LE-H69, LE-M4): up to six bands, read straight from their own parameters
    std::array<float, 6> xHz { 100.0f, 400.0f, 1500.0f, 5000.0f, 10000.0f, 20000.0f }, xDb {}, xQ { 1, 1, 1, 1, 1, 1 };
    std::array<int, 6>   xShape {};                       // per band: 0 = its natural shape, 1 = the alternative (peak <-> shelf)
    float xHpfHz = 0.0f, xLpfHz = 0.0f;                   // 0 = off
    int   xMode = 0;                                      // LE-K9: 0 = E (constant Q), 1 = G (proportional Q)
};

struct ChainSettings
{
    float inputDb  = 0.0f;
    float outputDb = 0.0f;

    // Resonance suppressor (very first, soothe-style)
    ResoSettings reso {};

    // Clean-up EQ
    bool       cleanOn = true;
    int        cleanPhase = 0;      // Tahor EQ: 0 = Natural, 1 = Linear, 2 = Linear HQ
    int        cleanOs = 0;         // Tahor EQ (Natural): 0 = off, 1 = 2x, 2 = 4x
    bool       peakCatch = false;   // Spectra-style input peak catcher (before the clean-up EQ)
    float      peakInDb  = 0.0f;    // peak catcher: its own input drive (pushes into the -6 dB catch point)
    float      peakOutDb = 0.0f;    //               its own output trim
    bool       peakLink  = true;    //               linked: the output drops by whatever the input pushes
    bool       trackMode = false;   // low-latency tracking: zero-latency versions of every stage
    float      riderRangeDb = 0.0f; // vocal rider: how far it may ride (0 = off, up to 12 dB)
    float      breathDb  = 0.0f;    // breath control: how much breaths are turned down (0 = off, down to -24 dB)
    int        compScHpf = 0;       // compressor sidechain high-pass: 0 off, 1 = 80, 2 = 120, 3 = 160 Hz
    int        limSoft   = 0;       // limiter soft saturation (Lavry-style): 0 off, 1 = 2 dB, 2 = 3 dB, 3 = 4 dB, 4 = 6 dB
    CleanBands clean {};

    // Character EQs (second slot): four EQs, each with its own settings and
    // on/off. Any combination can run; active ones run in series
    // Tube P1 -> E-Console -> Class A 81 -> Airy.
    bool eqOn = true;
    std::array<CharEqSettings, numCharEqs> charEq { CharEqSettings {}, CharEqSettings { true }, CharEqSettings {}, CharEqSettings {} };

    // Gate (before the compressor; -80 dB = off)
    float    gateThreshDb = -80.0f;

    // Compressor
    // Compressors: three independent units, each with its own settings and
    // on/off. Any combination can run; active ones run in series
    // FET Purple -> Vox Leveler -> Vari-Mu 660. compOn is the section power.
    bool compOn = true;

    // Processing order (indexes into charEq / comps), and whether the
    // character EQs sit after the compressors instead of before them
    std::array<int, numCharEqs> eqOrder { 0, 1, 2, 3, 4, 5, 6, 7 };   // V74: all eight (older versions use the first four)
    std::array<int, numComps> compOrder { 0, 1, 2, 3, 4, 5, 6, 7, 8 };   // V75: all nine (older versions use the first four)
    bool eqAfterComp = false;
    std::array<CompUnitSettings, numComps> comps { CompUnitSettings { true,  CompType::FetPurple },
                                            CompUnitSettings { false, CompType::RComp },
                                            CompUnitSettings { false, CompType::VariMu660 },
                                            CompUnitSettings { false, CompType::StaLevel },
                                            CompUnitSettings { false, CompType::Api2500 },
                                            CompUnitSettings { false, CompType::SslG },
                                            CompUnitSettings { false, CompType::WeissDs1 },
                                            CompUnitSettings { false, CompType::SplIron },
                                            CompUnitSettings { false, CompType::Neve33609 } };

    // De-esser
    bool  dsOn       = true;
    float dsHz       = 6500.0f;
    float dsThreshDb = -30.0f;
    float dsRangeDb  = 8.0f;
    bool  dsListen   = false;
    DsMode dsMode    = DsMode::Split;
    bool  dsAuto     = false;   // threshold-less: follows the vocal level

    // Vox Flow (between the de-esser and the limiter)
    bool  flowOn     = true;
    int   flowStyle  = 0;        // FlowStyle
    float flowAmount = 0.5f;     // 0..1, 50% = the style as designed
    float flowMix    = 1.0f;     // 0..1
    int   flowModel  = 0;        // 0 = Zerem (multiband styles), 1 = Perach (adaptive tone shaper)
    ToneShaperSettings perach {};

    // Tape (just before the limiter)
    int   tapeModel = 0;        // 0 = Tape (machine settings), 1 = Esh (character saturator)
    EshSettings esh {};
    bool  tapeOn    = false;
    int   tapeStyle = 0;         // speed: 0 = 30 ips, 1 = 15 ips, 2 = 7.5 ips
    int   tapeWidth = 2;         // width: 0 = 1/4", 1 = 1/2", 2 = 1" (neutral), 3 = 2"
    int   tapeOs = 1;            // tape oversampling: 0 = 1x, 1 = 2x (default), 2 = 4x
    int   limOs  = 0;            // soft saturation + limiter oversampling: 0 = 1x (default), 1 = 2x, 2 = 4x
    bool  tapeNoise = false;     // tape hiss: level follows the speed and width (slower / narrower = more hiss)
    float tapeDrive = 0.5f;      // 0..1, 50% = the style's recommended setting
    float tapeMix   = 1.0f;      // 0..1, blends the saturation (the tape tone stays)
    float tapeMsDrive = 0.0f;    // V60: -1 (drive the mid harder) .. 0 (plain stereo) .. +1 (drive the sides harder)
    bool  msEnabled = false;     // V60: mid/side options are live (stereo tracks only)

    // Limiter (maximizer style: threshold pushes level up into the ceiling)
    bool    limOn        = true;
    LimMode limMode      = LimMode::Classic;
    float   limThreshDb  = 0.0f;
    float   limCeilingDb = -1.0f;
    float   limReleaseMs = 60.0f;
    bool    limArc       = true;
    float   limMix       = 1.0f;   // 0..1, dry is delayed to match the look-ahead
    bool    limMax       = false;  // V69: the Max engine (our take on the Ozone Maximizer's modes) instead of Classic / Multi
    int     limMaxStyle  = 3;      // V69: 0 Smart, 1 Transient, 2 Pumping, 3 Balanced, 4 Crisp, 5 Clip, 6 Spectral, 7 Bands
    float   limCharacter = 5.0f;   // V69: 0 fast .. 10 slow (scales the timing of every style)
    float   limTransients = 0.0f;  // V69: 0..1 transient emphasis before the limiting
    float   limUpward = 0.0f;      // V70: 0..12 dB upward compression (lifts the quiet parts) before the limiting
    bool    midOrderCustom = false;                  // V68: the user dragged the modules into their own order
    std::array<int, 5> midOrder { 0, 1, 2, 3, 4 };   // V68: 0 Char EQ, 1 Dynamics, 2 De-Ess, 3 Tone, 4 Tape
    bool    compDelta    = false;  // V65: hear what the compressors change (level-matched difference)
    bool    limDelta     = false;  // V65: hear what the limiter takes off
    bool    compScExt    = false;  // V64: compressors listen to the external sidechain (key) input
    bool    dsScExt      = false;  // V64: the de-esser listens to the external sidechain
    float   width        = 1.0f;   // V63: stereo width 0..2 (1 = as is); widening leaves the lows (below ~120 Hz) centred
    bool    limTp        = false;  // V62: true-peak detection (catches the peaks that appear between samples)
    int     limClip      = 0;      // V62: clipper before the limiter: 0 off, 1..5 = 1, 2, 3, 4, 6 dB
    float   limLink      = 1.0f;   // V61: 1 = left and right always get the same gain, 0 = each side limits on its own
};

inline float msToCoef (float ms, double sampleRate)
{
    return (float) std::exp (-1.0 / (juce::jmax (0.001, (double) ms) * 0.001 * sampleRate));
}

//==============================================================================
// Gentle analog-style colour: a biased tanh blended in parallel, normalised so
// quiet signals pass at unity, followed by a DC blocker.
class Saturator
{
public:
    void prepare (double sr) { r = (float) (1.0 - juce::MathConstants<double>::twoPi * 5.0 / sr); reset(); }
    void reset() { for (auto& s : state) s = {}; }

    // drive: curve steepness, bias: asymmetry (even harmonics), mix: amount
    float processSample (int ch, float x, float drive, float bias, float mix)
    {
        if (mix <= 0.0f) return x;

        const float tb = std::tanh (drive * bias);
        const float norm = drive * (1.0f - tb * tb);
        const float shaped = (std::tanh (drive * (x + bias)) - tb) / norm;
        const float y = x + mix * (shaped - x);

        auto& s = state[(size_t) ch];
        const float out = y - s.x1 + r * s.y1;   // DC blocker
        s.x1 = y; s.y1 = out;
        return out;
    }

private:
    struct DcState { float x1 = 0.0f, y1 = 0.0f; };
    std::array<DcState, 2> state {};
    float r = 0.9995f;
};

//==============================================================================
// Silk: a high-band exciter for the Airy EQ. Only the content above ~6 kHz is
// saturated (biased tanh = mostly even, tube-like harmonics), at 2x
// oversampling to avoid aliasing fizz. Just the generated harmonics are mixed
// back in, so the original signal, its level and its phase are untouched.
class SilkExciter
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = juce::jmax (32, maxBlockSize);
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversampler->initProcessing ((size_t) maxBlock);
        band.setSize (2, maxBlock);
        for (auto& f : highpass)
        {
            f.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, juce::jmin (6000.0f, (float) (sr * 0.4)), 0.707f);
            f.prepare ({ sr, (juce::uint32) maxBlock, 1 });
        }
        dcR = (float) (1.0 - juce::MathConstants<double>::twoPi * 20.0 / sr);
        reset();
    }

    void reset()
    {
        if (oversampler) oversampler->reset();
        for (auto& f : highpass) f.reset();
        for (auto& d : dc) d = {};
    }

    // amount: 0..1
    void process (juce::AudioBuffer<float>& buffer, float amount)
    {
        if (amount <= 0.001f || oversampler == nullptr) return;

        const int numCh = juce::jmin (2, buffer.getNumChannels());
        // Bias is applied after the drive gain, so the curve's asymmetry (and
        // the even/odd balance) stays the same at every Silk setting.
        const float drive = 1.0f + 5.0f * amount;
        constexpr float bias = 0.3f;
        const float tb = std::tanh (bias);
        const float norm = drive * (1.0f - tb * tb);
        const float mix = 0.35f * std::sqrt (amount);

        for (int start = 0; start < buffer.getNumSamples(); start += maxBlock)
        {
            const int n = juce::jmin (maxBlock, buffer.getNumSamples() - start);

            // 1. isolate the air band
            for (int c = 0; c < 2; ++c)
            {
                auto* dst = band.getWritePointer (c);
                if (c < numCh)
                    for (int i = 0; i < n; ++i)
                        dst[i] = highpass[(size_t) c].processSample (buffer.getSample (c, start + i));
                else
                    juce::FloatVectorOperations::copy (dst, band.getReadPointer (0), n);
            }

            // 2. generate harmonics at 2x: keep only (shaped - original)
            juce::dsp::AudioBlock<float> block (band.getArrayOfWritePointers(), 2, (size_t) n);
            auto up = oversampler->processSamplesUp (block);
            for (size_t c = 0; c < up.getNumChannels(); ++c)
            {
                auto* d = up.getChannelPointer (c);
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                {
                    const float h = d[i];
                    d[i] = (std::tanh (drive * h + bias) - tb) / norm - h;
                }
            }
            oversampler->processSamplesDown (block);

            // 3. add the harmonics back (DC-blocked: even harmonics create offset)
            for (int c = 0; c < numCh; ++c)
            {
                auto& st = dc[(size_t) c];
                const auto* h = band.getReadPointer (c);
                for (int i = 0; i < n; ++i)
                {
                    const float y = h[i] - st.x1 + dcR * st.y1;
                    st.x1 = h[i]; st.y1 = y;
                    buffer.setSample (c, start + i, buffer.getSample (c, start + i) + mix * y);
                }
            }
        }
    }

private:
    struct Dc { float x1 = 0.0f, y1 = 0.0f; };
    double sr = 48000.0;
    int maxBlock = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> band;
    std::array<juce::dsp::IIR::Filter<float>, 2> highpass;
    std::array<Dc, 2> dc {};
    float dcR = 0.997f;
};

//==============================================================================
// Five-band vocal EQ (high-pass, body shelf, mud bell, presence bell, air shelf)
// with three characters:
//   Clean      - textbook digital curves, 12 dB/oct high-pass, no colour
//   E-Console  - British console: 18 dB/oct high-pass, constant-Q bells,
//                firmer shelves, faint odd-harmonic console grit
//   Class A 81 - 1081-style: proportional-Q bells (broad at small boosts),
//                shelves with the classic bump/dip, transformer warmth
class VocalEQ
{
public:
    static constexpr int numSlots = 8;   // HPF needs two slots for 18 dB/oct; V74: the new models use up to 8
    static constexpr int maxChannels = 2;
    using Biquad = std::array<float, 6>;   // b0 b1 b2 a0 a1 a2
    using Design = std::array<Biquad, numSlots>;
    using AC = juce::dsp::IIR::ArrayCoefficients<float>;

    // Tube P1: Pultec-style curves, tuned so that
    //  - Low Boost 10 alone gives a broad ~+13 dB lift around the chosen frequency
    //  - Boost + Atten together give the classic "trick": lows up (~+4 dB at 60 Hz
    //    with both at 10) and a dip just above (~-6 dB at 150-200 Hz)
    //  - High Boost is a bell whose width follows Bandwidth (sharp .. broad), up to ~+16 dB
    //  - High Atten is a gentle shelf cut, up to ~-16 dB
    static Design designTubeP1 (const CharEqSettings& s, double sr)
    {
        auto f = [sr] (float hz) { return juce::jlimit (10.0f, (float) (sr * 0.45), hz); };
        auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };
        const Biquad identity { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
        const float lf = s.p1LowHz, b = juce::jlimit (0.0f, 10.0f, s.p1LowBoost), a = juce::jlimit (0.0f, 10.0f, s.p1LowAtten);
        const float hb = juce::jlimit (0.0f, 10.0f, s.p1HighBoost), ha = juce::jlimit (0.0f, 10.0f, s.p1HighAtten);
        const float bwQ = 2.5f * std::pow (0.45f / 2.5f, juce::jlimit (0.0f, 10.0f, s.p1HighBw) / 10.0f);   // sharp 2.5 .. broad 0.45

        Design d;
        d.fill (identity);
        d[0] = b > 0.0f ? Biquad (AC::makePeakFilter (sr, f (lf),        0.5f, g (b * 1.0f)))   : identity;
        d[1] = b > 0.0f ? Biquad (AC::makeLowShelf   (sr, f (lf * 1.3f), 0.6f, g (b * 0.4f)))   : identity;
        d[2] = a > 0.0f ? Biquad (AC::makePeakFilter (sr, f (lf * 2.5f), 0.5f, g (-a * 1.0f)))  : identity;
        d[3] = a > 0.0f ? Biquad (AC::makeLowShelf   (sr, f (lf * 1.5f), 0.5f, g (-a * 0.5f)))  : identity;
        d[4] = hb > 0.0f ? Biquad (AC::makePeakFilter (sr, f (s.p1HighHz), bwQ, g (hb * 1.6f))) : identity;
        d[5] = ha > 0.0f ? Biquad (AC::makeHighShelf  (sr, f (s.p1AttenHz), 0.5f, g (-ha * 1.6f))) : identity;
        return d;
    }

    static Design design (const CharEqSettings& s, EqType type, double sr)
    {
        if (type == EqType::TubeP1) return designTubeP1 (s, sr);
        if ((int) type >= (int) EqType::Api550) return designV74 (s, type, sr);

        auto f = [sr] (float hz) { return juce::jlimit (10.0f, (float) (sr * 0.45), hz); };
        auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };
        const Biquad identity { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

        float hpfQ = 0.707f, shelfQ = 0.707f, mudQ = s.mudQ, presQ = s.presQ;
        float airQ = -1.0f, airHz = s.airHz;   // airQ < 0: same as shelfQ
        Biquad hpfFirst = identity;

        switch (type)
        {
            case EqType::EConsole:
            {
                auto fo = AC::makeFirstOrderHighPass (sr, f (s.hpfHz));
                hpfFirst = { fo[0], fo[1], 0.0f, fo[2], fo[3], 0.0f };
                hpfQ = 1.0f;          // 1st order + Q=1 biquad = 3rd-order Butterworth
                shelfQ = 0.8f;
                break;
            }
            case EqType::ClassA81:
            {
                hpfQ = 0.8f;
                shelfQ = 1.1f;        // overshoot gives the famous dip/bump
                auto prop = [] (float q, float db) { return q * (0.45f + 0.55f * juce::jmin (std::abs (db), 15.0f) / 15.0f); };
                mudQ  = prop (s.mudQ,  s.mudDb);
                presQ = prop (s.presQ, s.presDb);
                break;
            }
            case EqType::Airy:
            {
                // AirEQ-style: gentle cuts, a slightly resonant "Earth" shelf,
                // bells that boost broad and cut narrow, and a very wide Air
                // band that rises slowly and keeps going, so it opens up the
                // top without the harshness of a steep shelf. No colour.
                hpfQ = 0.6f;
                shelfQ = 0.9f;
                airQ = 0.4f;
                airHz = s.airHz * 0.8f;
                auto adapt = [] (float q, float db)
                {
                    const float shape = 0.7f + 0.3f * juce::jmin (std::abs (db), 15.0f) / 15.0f;
                    return q * shape * (db >= 0.0f ? 0.8f : 1.25f);
                };
                mudQ  = adapt (s.mudQ,  s.mudDb);
                presQ = adapt (s.presQ, s.presDb);
                break;
            }
            case EqType::TubeP1:
            default: break;
        }
        if (airQ < 0.0f) airQ = shelfQ;

        Design d;
        d.fill (identity);
        d[0] = hpfFirst;
        d[1] = AC::makeHighPass   (sr, f (s.hpfHz),  hpfQ);
        d[2] = AC::makeLowShelf   (sr, f (s.bodyHz), shelfQ, g (s.bodyDb));
        d[3] = AC::makePeakFilter (sr, f (s.mudHz),  mudQ,  g (s.mudDb));
        d[4] = AC::makePeakFilter (sr, f (s.presHz), presQ, g (s.presDb));
        d[5] = AC::makeHighShelf  (sr, f (airHz),    airQ,   g (s.airDb));
        return d;
    }

    // V74: four more classic EQs, each with its own curves (the colour is in process()):
    //   LE-A550 (API 550B-style): four stepped bands, proportional Q (broad when gentle, tighter when pushed), low and
    //            high switch between shelf and peak, an optional 50 Hz - 15 kHz band-pass filter, a little transformer weight
    //   LE-K9   (SSL 9000K-style): high-pass / low-pass, LF and HF shelves (or bells), two mid bells with Q, E or G curves
    //   LE-H69  (Helios 69-style): stepped bass shelf, stepped broad mid, 10 kHz treble shelf, a soft top end
    //   LE-M4   (Maag EQ4-style): fixed, very broad bands (sub, 40, 160, 650, 2.5k) and the wide Air band (2.5k-40k)
    static Design designV74 (const CharEqSettings& s, EqType type, double sr)
    {
        auto f = [sr] (float hz) { return juce::jlimit (10.0f, (float) (sr * 0.45), hz); };
        auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };
        const Biquad identity { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
        Design d;
        d.fill (identity);
        auto bell  = [&] (float hz, float q, float db) { return std::abs (db) < 0.01f ? identity : Biquad (AC::makePeakFilter (sr, f (hz), q, g (db))); };
        auto lowS  = [&] (float hz, float q, float db) { return std::abs (db) < 0.01f ? identity : Biquad (AC::makeLowShelf  (sr, f (hz), q, g (db))); };
        auto highS = [&] (float hz, float q, float db) { return std::abs (db) < 0.01f ? identity : Biquad (AC::makeHighShelf (sr, f (hz), q, g (db))); };
        const auto& hz = s.xHz; const auto& db = s.xDb;
        switch (type)
        {
            case EqType::Api550:
            {
                auto pq = [] (float gain) { return 0.55f + 0.95f * juce::jmin (std::abs (gain), 12.0f) / 12.0f; };   // proportional Q
                d[0] = s.xShape[0] ? bell (hz[0], pq (db[0]), db[0]) : lowS (hz[0], 0.65f, db[0]);
                d[1] = bell (hz[1], pq (db[1]), db[1]);
                d[2] = bell (hz[2], pq (db[2]), db[2]);
                d[3] = s.xShape[3] ? bell (hz[3], pq (db[3]), db[3]) : highS (hz[3], 0.65f, db[3]);
                if (s.xHpfHz > 0.0f)   // the band-pass filter
                {
                    d[4] = AC::makeHighPass (sr, f (50.0f), 0.707f);
                    d[5] = AC::makeLowPass  (sr, f (15000.0f), 0.707f);
                }
                d[6] = bell (45.0f, 0.6f, 0.35f);   // the output transformer's gentle low-end weight
                break;
            }
            case EqType::K9000:
            {
                const bool gMode = s.xMode == 1;
                // E: constant Q; G: proportional (narrows as the gain goes up) and shelves with a little overshoot
                auto q = [gMode] (float base, float gain)
                         { return gMode ? base * (0.55f + 0.75f * juce::jmin (std::abs (gain), 15.0f) / 15.0f) : base; };
                const float shelfQ = gMode ? 0.95f : 0.65f;
                if (s.xHpfHz > 0.0f)
                {
                    auto fo = AC::makeFirstOrderHighPass (sr, f (s.xHpfHz));
                    d[0] = { fo[0], fo[1], 0.0f, fo[2], fo[3], 0.0f };
                    d[1] = AC::makeHighPass (sr, f (s.xHpfHz), 1.0f);          // 18 dB/oct
                }
                if (s.xLpfHz > 0.0f) d[2] = AC::makeLowPass (sr, f (s.xLpfHz), 0.707f);   // 12 dB/oct
                d[3] = s.xShape[0] ? bell (hz[0], q (0.9f, db[0]), db[0]) : lowS (hz[0], shelfQ, db[0]);
                d[4] = bell (hz[1], q (s.xQ[1], db[1]), db[1]);
                d[5] = bell (hz[2], q (s.xQ[2], db[2]), db[2]);
                d[6] = s.xShape[3] ? bell (hz[3], q (0.9f, db[3]), db[3]) : highS (hz[3], shelfQ, db[3]);
                break;
            }
            case EqType::Helios69:
            {
                // bass: a slightly resonant shelf when boosting (that thick, rounded low end), a softer one cutting
                d[0] = db[0] >= 0.0f ? lowS (hz[0], 1.0f, db[0]) : lowS (hz[0] * 1.5f, 0.55f, db[0]);
                // mid: broad and forward when boosting, narrower when cutting
                d[1] = bell (hz[1], db[1] >= 0.0f ? 0.8f : 1.4f, db[1]);
                d[2] = highS (10000.0f, 0.6f, db[2]);
                d[3] = AC::makeLowPass (sr, f (19000.0f), 0.6f);              // the transformers' soft top
                d[4] = bell (90.0f, 0.7f, 0.4f);                              // and their low-mid weight
                break;
            }
            case EqType::Maag4:
            {
                d[0] = lowS (20.0f, 0.5f, db[0]);                             // SUB: below the kick, felt more than heard
                d[1] = bell (40.0f,   0.55f, db[1]);
                d[2] = bell (160.0f,  0.55f, db[2]);
                d[3] = bell (650.0f,  0.55f, db[3]);
                d[4] = bell (2500.0f, 0.55f, db[4]);
                // AIR: a very wide shelf that keeps rising past the chosen frequency (above Nyquist it still tilts the top)
                const float airHz = juce::jmin (hz[5], (float) (sr * 0.45));
                const float stretch = hz[5] > sr * 0.45 ? 0.8f : 1.0f;
                d[5] = highS (airHz * 0.75f, 0.32f, db[5] * stretch);
                break;
            }
            default: break;
        }
        return d;
    }

    // Combined magnitude in dB at a frequency (used to draw the EQ curve)
    static float magnitudeDb (const Design& d, double hz, double sr)
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sr;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        double mag = 1.0;

        for (auto& c : d)
        {
            auto num = (double) c[0] + (double) c[1] * z1 + (double) c[2] * z2;
            auto den = (double) c[3] + (double) c[4] * z1 + (double) c[5] * z2;
            mag *= std::abs (num) / juce::jmax (1.0e-12, std::abs (den));
        }
        return (float) juce::Decibels::gainToDecibels (mag, -120.0);
    }

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sr, 4096, 1 };

        for (auto& ch : filters)
            for (auto& flt : ch)
            {
                flt.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
                flt.prepare (spec);
            }

        sat.prepare (sr);
        silk.prepare (sr, maxBlockSize);
        msBuf.setSize (1, juce::jmax (32, maxBlockSize) * 4);   // V71 (room for oversampled blocks)
        haveDesign = false;
    }

    void update (const CharEqSettings& s, EqType t)
    {
        type = t;
        silkAmount = t == EqType::Airy ? juce::jlimit (0.0f, 1.0f, s.silk) : 0.0f;
        auto d = design (s, t, sr);
        if (haveDesign && std::memcmp (&d, &current, sizeof (Design)) == 0)
            return;

        current = d;
        haveDesign = true;

        for (auto& ch : filters)
            for (int b = 0; b < numSlots; ++b)
                *ch[(size_t) b].coefficients = d[(size_t) b];   // no allocation
    }

    void reset()
    {
        for (auto& ch : filters)
            for (auto& flt : ch)
                flt.reset();
        sat.reset();
        silk.reset();
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 71
        // V71 M/S: the EQ (and its colour) works on the mid or the side only; the other half passes untouched
        const int pl = buffer.getNumChannels() >= 2 ? juce::jlimit (0, 2, place) : 0;
        if (pl != placeWas) { reset(); placeWas = pl; }
        if (pl != 0)
        {
            const int n = buffer.getNumSamples();
            if (msBuf.getNumSamples() < n) msBuf.setSize (1, n, false, false, true);
            auto* l = buffer.getWritePointer (0); auto* r = buffer.getWritePointer (1);
            auto* x = msBuf.getWritePointer (0);
            for (int i = 0; i < n; ++i) x[i] = pl == 1 ? 0.5f * (l[i] + r[i]) : 0.5f * (l[i] - r[i]);
            juce::AudioBuffer<float> one (msBuf.getArrayOfWritePointers(), 1, n);
            processChannels (one, 1);
            for (int i = 0; i < n; ++i)
            {
                const float m = pl == 1 ? x[i] : 0.5f * (l[i] + r[i]);
                const float sd = pl == 2 ? x[i] : 0.5f * (l[i] - r[i]);
                l[i] = m + sd; r[i] = m - sd;
            }
            return;
        }
       #endif
        processChannels (buffer, juce::jmin (buffer.getNumChannels(), maxChannels));
    }
    int place = 0;   // V71: set by the chain each block

private:
    void processChannels (juce::AudioBuffer<float>& buffer, int numCh)
    {
        const int n = buffer.getNumSamples();

        float drive = 1.0f, bias = 0.0f, mix = 0.0f;
        if (type == EqType::Api550)        { drive = 1.8f; bias = 0.06f; mix = 0.20f; }   // V74: 2520 op-amp punch, mostly odd
        else if (type == EqType::K9000)    { drive = 1.1f; bias = 0.0f;  mix = 0.03f; }   // V74: the clean modern console
        else if (type == EqType::Helios69) { drive = 2.0f; bias = 0.18f; mix = 0.22f; }   // V74: discrete class A, brash
        else if (type == EqType::Maag4)    { drive = 1.0f; bias = 0.0f;  mix = 0.0f;  }   // V74: transparent
        else if (type == EqType::EConsole) { drive = 1.3f; bias = 0.0f;  mix = 0.12f; }
        else if (type == EqType::TubeP1)   { drive = 1.5f; bias = 0.20f; mix = 0.15f; }   // tube + transformer warmth
        else if (type == EqType::ClassA81) { drive = 1.6f; bias = 0.12f; mix = 0.22f; }

        for (int c = 0; c < numCh; ++c)
        {
            auto* x = buffer.getWritePointer (c);
            auto& bands = filters[(size_t) c];

            for (int i = 0; i < n; ++i)
            {
                float v = x[i];
                for (auto& flt : bands)
                    v = flt.processSample (v);
                x[i] = sat.processSample (c, v, drive, bias, mix);
            }
        }

        silk.process (buffer, silkAmount);   // Airy only
    }

    double sr = 48000.0;
    juce::AudioBuffer<float> msBuf;
    int placeWas = 0;
    std::array<std::array<juce::dsp::IIR::Filter<float>, numSlots>, maxChannels> filters;
    Saturator sat;
    SilkExciter silk;
    float silkAmount = 0.0f;
    EqType type = EqType::EConsole;
    Design current {};
    bool haveDesign = false;
};

//==============================================================================
// Stereo-linked compressor with three characters:
//   FET Purple  - 1176-style: ultra-fast attack range, hard-ish knee,
//                 program-dependent release, FET grit that grows with GR
//   LE-RC - Renaissance-style compressor: Ratio / Attack / Release / Gain, AUTO release,
//           Glide or Snap release shape, Color or Pure tone. (Was the RVox-style leveler:)
//                 automatic make-up so peaks stay pinned near 0 dBFS, clean
//   Vari-Mu 660 - Fairchild-style: ratio rises with level (very soft knee),
//                 six time-constant settings instead of attack/release,
//                 tube warmth
class VocalCompressor
{
public:
    struct Timing { float attackMs, fastReleaseMs, slowReleaseMs, slowAttackMs, slowDepth; };

    // Fairchild 660 time constants (attack ms, release ms; 5 & 6 are program dependent)
    static Timing variMuTiming (int tc)
    {
        static const Timing table[6] = {
            { 0.2f,   300.0f,    300.0f,    1.0f, 0.0f },
            { 0.2f,   800.0f,    800.0f,    1.0f, 0.0f },
            { 0.4f,  2000.0f,   2000.0f,    1.0f, 0.0f },
            { 0.4f,  5000.0f,   5000.0f,    1.0f, 0.0f },
            { 0.4f,  2000.0f,  10000.0f, 1000.0f, 0.6f },
            { 0.2f,   300.0f,  25000.0f, 2000.0f, 0.5f } };
        return table[juce::jlimit (0, 5, tc)];
    }

    // LE-STA: program-dependent dual time constants. The release recovers quickly at first, then the last few dB
    // come back slowly (the slow envelope), so it "breathes" instead of pumping.
    static Timing staTiming (int mode, int rel)
    {
        static const Timing table[3] = {
            { 30.0f, 1200.0f, 5000.0f, 1500.0f, 0.60f },    // Single: slow attack and release - transients punch through
            { 10.0f,  500.0f, 3500.0f,  900.0f, 0.55f },    // Double: the original unit's behaviour
            {  2.5f,  300.0f, 2200.0f,  600.0f, 0.50f } };  // Triple: faster attack, still a slow, smooth release (vocals)
        static const float relMul[4] { 0.5f, 1.0f, 2.0f, 4.0f };
        auto t = table[juce::jlimit (0, 2, mode)];
        const float m = relMul[juce::jlimit (0, 3, rel)];
        t.fastReleaseMs *= m; t.slowReleaseMs *= m;
        return t;
    }
    static constexpr float staThresholdDb = -18.0f;   // fixed, like the original's regulator-set threshold: Input decides how hard it works

    // LE-STA: its own unit (V52), or the second model of the tube slot (V49-V51)
    static bool isSta (const CompUnitSettings& s)
    {
        return s.type == CompType::StaLevel || (s.type == CompType::VariMu660 && s.tubeModel == 1);
    }

    static Timing timingFor (const CompUnitSettings& s)
    {
        if (isSta (s)) return staTiming (s.staMode, s.staRel);
        switch (s.type)
        {
            case CompType::RComp:
                if (s.rcModel == 1) return { 3.0f, 80.0f, 600.0f, 500.0f, 0.5f };   // RVox: fixed fast timing (the original leveler)
                return { s.attackMs, s.releaseMs, s.releaseMs * 5.0f, 400.0f, s.arc ? 0.6f : 0.0f };
            case CompType::VariMu660:  return variMuTiming (s.timeConstant);
            case CompType::FetPurple:
            default:
                if (s.fetRev == 1) return { s.attackMs * 0.8f, s.releaseMs * 0.85f, s.releaseMs * 3.0f, 250.0f, 0.5f };   // Blue Stripe: snappier, more pump
                if (s.fetRev == 2) return { s.attackMs * 1.15f, s.releaseMs, s.releaseMs * 4.5f, 350.0f, 0.65f };      // Black: a touch smoother
                return { s.attackMs, s.releaseMs, s.releaseMs * 4.0f, 300.0f, 0.6f };
        }
    }

    // Static gain curve: desired gain change (dB, <= 0) for a level over threshold
    static float gainComputer (const CompUnitSettings& s, float over)
    {
        auto softKnee = [] (float o, float ratio, float knee)
        {
            const float slope = 1.0f / juce::jmax (1.0f, ratio) - 1.0f;
            if (2.0f * o < -knee) return 0.0f;
            if (2.0f * o >  knee) return slope * o;
            return slope * (o + knee * 0.5f) * (o + knee * 0.5f) / (2.0f * knee);
        };

        switch (s.type)
        {
            case CompType::RComp: return s.rcModel == 1 ? softKnee (over, 3.0f, 10.0f)    // RVox: fixed 3:1, very soft knee
                                                        : softKnee (over, s.ratio, 6.0f);   // gentle 6 dB knee
            case CompType::StaLevel:
            case CompType::VariMu660:
            {
                if (isSta (s))   // LE-STA: about 3:1 once it's working, a wide soft knee, never a hard wall
                {
                    const float x = over + 8.0f;
                    return x <= 0.0f ? 0.0f : -0.70f * x * x / (x + 10.0f);
                }
                const float x = over + 6.0f;   // starts working ~6 dB under threshold
                return x <= 0.0f ? 0.0f : -0.85f * x * x / (x + 18.0f);
            }
            case CompType::FetPurple:
            default: return softKnee (over, s.ratio, s.fetRev == 1 ? 2.5f : s.fetRev == 2 ? 5.0f : 4.0f);   // Blue Stripe bites harder, Black is smoother
        }
    }

    static float autoMakeupDb (const CompUnitSettings& s)
    {
        // RVox: automatic make-up so a 0 dBFS peak still comes out near 0 dBFS (lower the threshold = more level, not less)
        if (s.type == CompType::RComp && s.rcModel == 1 && s.voxLink) return -gainComputer (s, 0.0f - s.threshDb);   // Link off: Output only
        return 0.0f;   // every other unit uses its own Gain / Make-up
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate; sat.prepare (sr);
        scCoef = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, 80.0, 0.707f));
        for (auto& f : scFilter) f.coefficients = scCoef;
        scHz = 80.0f;
        reset();
    }
    void reset()
    {
        fast = slow = 0.0f; sat.reset(); for (auto& f : scFilter) f.reset();
        staLow = {}; staTop = {}; staCv = 0.0f; staThump = 0.0f;
    }

    const juce::AudioBuffer<float>* key = nullptr;   // V64: external sidechain (same length as the block), or null

    // Returns the deepest gain reduction in this block (dB, >= 0)
    float process (juce::AudioBuffer<float>& buffer, const CompUnitSettings& s)
    {
        const bool useSc = s.scHpHz > 0.0f && scCoef != nullptr;
        if (useSc && std::abs (s.scHpHz - scHz) > 0.5f)
        {
            *scCoef = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, s.scHpHz, 0.707f);
            scHz = s.scHpHz;
        }
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        const Timing t = timingFor (s);
        const float att   = msToCoef (t.attackMs, sr);
        const float relF  = msToCoef (t.fastReleaseMs, sr);
        const float relS  = msToCoef (t.slowReleaseMs, sr);
        const float attS  = msToCoef (t.slowAttackMs, sr);
        const float makeup = s.makeupDb + autoMakeupDb (s);
        float maxGr = 0.0f;
        // LE-STA: Input drives into the fixed threshold; output transformer and tube colour afterwards
        const bool sta = isSta (s);
        const float staIn = sta ? juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 24.0f, s.staInDb)) : 1.0f;
        const float threshDb = sta ? staThresholdDb : s.threshDb;
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float kLow  = 1.0f - std::exp (-twoPi * 120.0f  / (float) sr);    // transformer low end (breaks up first)
        const float kTop  = 1.0f - std::exp (-twoPi * 9000.0f / (float) sr);    // a soft top
        const float kCv   = 1.0f - std::exp (-twoPi * 6.0f    / (float) sr);    // control voltage leaking through: the "thump"
        const float kTh   = 1.0f - std::exp (-twoPi * 40.0f   / (float) sr);
        static constexpr float thumpPerDb[3] { 0.0015f, 0.0025f, 0.004f };      // more thump on the faster modes
        const float thumpAmt = sta ? thumpPerDb[juce::jlimit (0, 2, s.staMode)] : 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c)
            {
                // the detector can ignore the low end (sidechain high-pass) so plosives and boom don't over-trigger
                const float x = key != nullptr ? key->getSample (juce::jmin (c, key->getNumChannels() - 1), i) : buffer.getSample (c, i);
                peak = juce::jmax (peak, std::abs (useSc ? scFilter[(size_t) c].processSample (x) : x));
            }

            const float over = juce::Decibels::gainToDecibels (peak * staIn, -120.0f) - threshDb;
            const float target = gainComputer (s, over);

            // Fast envelope follows the signal; slow envelope charges up only
            // under sustained compression, stretching the release afterwards.
            float rel = relF;
            if (s.type == CompType::RComp && s.rcModel == 0 && target > fast)
            {
                // Glide: the release slows down as the gain reduction gets shallow (a smooth tail).
                // Snap: the release speeds up near threshold (more transparent and louder).
                const float depth = juce::jlimit (0.0f, 1.0f, -fast / 6.0f);
                const float mul = s.behavior == 0 ? 1.0f + 2.0f * (1.0f - depth) : 0.35f + 0.65f * depth;
                rel = msToCoef (t.fastReleaseMs * mul, sr);
            }
            fast = (target < fast ? att : rel) * (fast - target) + target;
            const float slowTarget = target * t.slowDepth;
            slow = (slowTarget < slow ? attS : relS) * (slow - slowTarget) + slowTarget;

            const float gr = juce::jmin (fast, slow);   // dB, <= 0
            const float gain = juce::Decibels::decibelsToGain (gr + makeup);
            maxGr = juce::jmax (maxGr, -gr);

            float drive = 1.0f, bias = 0.0f, mix = 0.0f;
            if (s.type == CompType::FetPurple && s.fetRev == 1)   // Blue Stripe: early revision - more grit and even harmonics, coloured even at rest
            {
                drive = 1.3f + 0.03f * -gr;
                bias = 0.12f;
                mix = juce::jlimit (0.0f, 0.55f, 0.2f + 0.02f * -gr);
            }
            else if (s.type == CompType::FetPurple && s.fetRev == 2)   // Black: low-noise revision - clean until pushed
            {
                drive = 1.0f + 0.025f * -gr;
                bias = 0.03f;
                mix = juce::jlimit (0.0f, 0.2f, 0.03f + 0.01f * -gr);
            }
            else if (s.type == CompType::FetPurple)
            {
                drive = 1.0f + 0.04f * -gr;
                bias = 0.05f;
                mix = juce::jlimit (0.0f, 0.35f, 0.08f + 0.015f * -gr);
            }
            else if (sta)   // LE-STA: thick and velvety, more distortion the harder it compresses
            {
                drive = 1.6f + 0.02f * -gr; bias = 0.10f;
                mix = juce::jlimit (0.0f, 0.5f, 0.22f + 0.014f * -gr);
            }
            else if (s.type == CompType::VariMu660)
            {
                drive = 1.4f; bias = 0.15f;
                mix = juce::jlimit (0.0f, 0.35f, 0.18f + 0.01f * -gr);
            }
            else if (s.type == CompType::RComp && s.rcModel == 0 && s.character == 0)   // Color: gentle harmonics that grow as it compresses
            {
                drive = 3.0f; bias = 0.12f;
                mix = juce::jlimit (0.0f, 0.45f, 0.2f + 0.02f * -gr);
            }

            const bool preColour = s.type == CompType::FetPurple && s.fetRev == 1;
            float thump = 0.0f;
            if (sta)
            {
                staCv += kCv * (gr - staCv);                       // fast gain changes leak a low "thump"
                staThump += kTh * ((gr - staCv) * thumpAmt - staThump);
                thump = staThump;
            }
            for (int c = 0; c < numCh; ++c)
            {
                const float dry = buffer.getSample (c, i);
                if (sta)
                {
                    float y = sat.processSample (c, dry * staIn * gain, drive, bias, mix);
                    auto& lo = staLow[(size_t) c];
                    lo += kLow * (y - lo);
                    const float lowDrive = 1.2f + 0.05f * -gr;      // the low end saturates first
                    y = y - lo + std::tanh (lo * lowDrive) / lowDrive;
                    auto& tp = staTop[(size_t) c];
                    tp += kTop * (y - tp);
                    y += 0.18f * (tp - y);
                    y += thump;
                    buffer.setSample (c, i, dry + s.mix * (y - dry));
                    continue;
                }
                // Blue Stripe: the grit comes from the hot input stage, before the gain comes down,
                // so the harder you drive it the dirtier it gets (like the early units)
                const float wet = preColour ? sat.processSample (c, dry, drive, bias, mix) * gain
                                            : sat.processSample (c, dry * gain, drive, bias, mix);
                buffer.setSample (c, i, dry + s.mix * (wet - dry));   // Mix: parallel compression
            }
        }
        return maxGr;
    }

private:
    double sr = 48000.0;
    float fast = 0.0f, slow = 0.0f, scHz = 80.0f;
    std::array<float, 2> staLow {}, staTop {};
    float staCv = 0.0f, staThump = 0.0f;
    Saturator sat;
    juce::dsp::IIR::Coefficients<float>::Ptr scCoef;
    std::array<juce::dsp::IIR::Filter<float>, 2> scFilter;
};

//==============================================================================
// V75: five bus / mastering compressors, each with its own detector, curve, timing and colour
//   LE-2500 (API 2500-style): feed-forward (New) or feedback (Old), hard / medium / soft knee, and Thrust - the
//            detector hears less low end (Med) or a tilt toward the highs (Loud), so the lows punch through
//   LE-G    (SSL G bus-style): VCA glue - stepped ratio 2 / 4 / 10, stepped attack, release 0.1 - 1.2 s or Auto
//   LE-DS1  (Weiss DS1-style): transparent mastering dynamics - peak or RMS detection, hard or soft knee, and an
//            Average release that recovers fast from short peaks and slowly under sustained level
//   LE-IRON (SPL Iron-style): tube mastering compressor - Bias sets the ratio and the tube colour, the Rectifier
//            (silicon / germanium / LED) the knee and speed, Airbass keeps the lows and highs out of the detector
//   LE-336  (Neve 33609-style): diode-bridge compressor with stepped ratio and recovery (A1 / A2 auto), followed by
//            its own limiter (threshold, fast / slow attack, recovery), transformer colour that grows with the work
class MasterCompressor
{
public:
    void prepare (double sampleRate) { sr = sampleRate; sat.prepare (sr); reset(); }
    void reset()
    {
        fast = slow = lim = limSlow = 0.0f; ms = 0.0; lastGain = 1.0f; sat.reset();
        for (auto& st : f) st = {};
    }
    const juce::AudioBuffer<float>* key = nullptr;

    // stepped values (the hardware's switch positions)
    static float apiAttackMs (int i)   { static const float v[5] { 0.3f, 1.0f, 3.0f, 10.0f, 30.0f }; return v[juce::jlimit (0, 4, i)]; }
    static float sslRatio (int i)      { static const float v[3] { 2.0f, 4.0f, 10.0f }; return v[juce::jlimit (0, 2, i)]; }
    static float sslAttackMs (int i)   { static const float v[6] { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f }; return v[juce::jlimit (0, 5, i)]; }
    static float sslReleaseMs (int i)  { static const float v[5] { 100.0f, 300.0f, 600.0f, 1200.0f, -1.0f }; return v[juce::jlimit (0, 4, i)]; }   // -1 = Auto
    static float ironAttackMs (int i)  { static const float v[6] { 0.5f, 1.0f, 3.0f, 8.0f, 20.0f, 50.0f }; return v[juce::jlimit (0, 5, i)]; }
    static float ironReleaseMs (int i) { static const float v[6] { 50.0f, 100.0f, 200.0f, 400.0f, 800.0f, -1.0f }; return v[juce::jlimit (0, 5, i)]; }
    static float neveRatio (int i)     { static const float v[5] { 1.5f, 2.0f, 3.0f, 4.0f, 6.0f }; return v[juce::jlimit (0, 4, i)]; }
    static float neveCompRecMs (int i) { static const float v[6] { 100.0f, 400.0f, 800.0f, 1500.0f, -1.0f, -2.0f }; return v[juce::jlimit (0, 5, i)]; }   // -1 A1, -2 A2
    static float neveLimRecMs (int i)  { static const float v[6] { 50.0f, 100.0f, 200.0f, 800.0f, -1.0f, -2.0f }; return v[juce::jlimit (0, 5, i)]; }

    static float softKnee (float o, float ratio, float knee)
    {
        const float slope = 1.0f / juce::jmax (1.0f, ratio) - 1.0f;
        if (knee <= 0.01f) return o > 0.0f ? slope * o : 0.0f;
        if (2.0f * o < -knee) return 0.0f;
        if (2.0f * o >  knee) return slope * o;
        return slope * (o + knee * 0.5f) * (o + knee * 0.5f) / (2.0f * knee);
    }

    // static curve for the GR tests and the screen: dB of gain change for a level over threshold
    static float curve (const CompUnitSettings& s, float over)
    {
        switch (s.type)
        {
            case CompType::Api2500:  { static const float kn[3] { 0.0f, 6.0f, 12.0f }; return softKnee (over, s.ratio, kn[juce::jlimit (0, 2, s.sw[0])]); }
            case CompType::SslG:     return softKnee (over, s.ratio, 2.0f);
            case CompType::WeissDs1: return softKnee (over, s.ratio, s.sw[0] == 1 ? 10.0f : 0.0f);
            case CompType::SplIron:
            {
                static const float kn[3] { 4.0f, 10.0f, 16.0f };
                const float r = s.ratio * (1.0f + juce::jmax (0.0f, over) / 24.0f);   // vari-mu: the ratio grows the harder it works
                return softKnee (over, r, kn[juce::jlimit (0, 2, s.sw[1])]);
            }
            case CompType::Neve33609: return softKnee (over, s.ratio, 6.0f);
            default: return 0.0f;
        }
    }

    float process (juce::AudioBuffer<float>& buffer, const CompUnitSettings& s)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        const float twoPi = juce::MathConstants<float>::twoPi;
        auto onePole = [&] (float hz) { return 1.0f - std::exp (-twoPi * hz / (float) sr); };
        const float kSc = s.scHpHz > 0.0f ? onePole (s.scHpHz) : 0.0f;
        // timing (ms; release < 0 = program dependent)
        float attMs = 10.0f, relMs = 300.0f, slowRelMs = 0.0f, slowDepth = 0.0f, rmsMs = 0.0f;
        float tiltHz = 0.0f, tiltAmt = 0.0f;   // detector low-end reduction (Thrust / Airbass)
        float lpHz = 0.0f;                     // detector top cut (Airbass)
        bool feedback = false;
        float drive = 1.0f, bias = 0.0f, mixBase = 0.0f, mixPerDb = 0.0f;
        float limAttMs = 2.0f, limRelMs = 100.0f, limSlowMs = 0.0f, limDepth = 0.0f;
        const bool limOn = s.type == CompType::Neve33609 && s.limThreshDb < -0.05f;
        auto autoRel = [&] (float fastMs, float slowMs, float depth) { relMs = fastMs; slowRelMs = slowMs; slowDepth = depth; };
        switch (s.type)
        {
            case CompType::Api2500:
                attMs = apiAttackMs (s.sw[3]); relMs = s.releaseMs;
                if (s.sw[1] == 1) { tiltHz = 150.0f; tiltAmt = 0.55f; }          // Thrust Med: less low end in the detector
                if (s.sw[1] == 2) { tiltHz = 400.0f; tiltAmt = 0.8f; }           // Thrust Loud: a tilt toward the highs
                feedback = s.sw[2] == 1;                                         // Old: feedback detection
                drive = 1.6f; bias = feedback ? 0.08f : 0.05f; mixBase = 0.08f; mixPerDb = 0.01f;
                break;
            case CompType::SslG:
            {
                attMs = s.attackMs; const float r = s.releaseMs;
                if (r < 0.0f) autoRel (100.0f, 1200.0f, 0.6f); else relMs = r;
                drive = 1.2f; bias = 0.0f; mixBase = 0.03f; mixPerDb = 0.004f;
                break;
            }
            case CompType::WeissDs1:
                attMs = s.attackMs; relMs = s.releaseMs;
                if (s.sw[2] == 1) autoRel (s.releaseMs, s.releaseMs * 6.0f, 0.7f);   // Average release
                if (s.sw[1] == 1) rmsMs = 10.0f;                                       // RMS detection
                break;
            case CompType::SplIron:
            {
                static const float attMul[3] { 1.0f, 1.5f, 3.0f }, relMul[3] { 1.0f, 1.2f, 1.4f };
                const int rect = juce::jlimit (0, 2, s.sw[1]);
                attMs = s.attackMs * attMul[rect];
                if (s.releaseMs < 0.0f) autoRel (100.0f * relMul[rect], 1500.0f * relMul[rect], 0.6f); else relMs = s.releaseMs * relMul[rect];
                if (s.sw[2] == 1) { tiltHz = 150.0f; tiltAmt = 0.75f; lpHz = 5000.0f; }   // Airbass: the lows and highs stay out of the detector
                const float b = (float) juce::jlimit (0, 3, s.sw[0]);
                drive = 1.3f + 0.15f * b; bias = 0.14f; mixBase = 0.05f + 0.04f * b; mixPerDb = 0.008f;
                rmsMs = 3.0f;                                                                    // tube detectors average a little
                break;
            }
            case CompType::Neve33609:
            {
                attMs = 4.0f;
                const float r = s.releaseMs;
                if (r == -1.0f) autoRel (40.0f, 2000.0f, 0.6f); else if (r == -2.0f) autoRel (150.0f, 5000.0f, 0.6f); else relMs = r;
                limAttMs = s.sw[1] == 1 ? 4.0f : 2.0f;
                const float lr = neveLimRecMs (s.sw[0]);
                if (lr == -1.0f) { limRelMs = 40.0f; limSlowMs = 2000.0f; limDepth = 0.6f; }
                else if (lr == -2.0f) { limRelMs = 150.0f; limSlowMs = 5000.0f; limDepth = 0.6f; }
                else limRelMs = lr;
                drive = 1.5f; bias = 0.1f; mixBase = 0.08f; mixPerDb = 0.012f;
                break;
            }
            default: break;
        }
        const float att = msToCoef (attMs, sr), rel = msToCoef (relMs, sr);
        const float relS = slowRelMs > 0.0f ? msToCoef (slowRelMs, sr) : 0.0f, attS = msToCoef (400.0f, sr);
        const float kRms = rmsMs > 0.0f ? onePole (1000.0f / (twoPi * rmsMs)) : 0.0f;
        const float kTilt = tiltHz > 0.0f ? onePole (tiltHz) : 0.0f, kLp = lpHz > 0.0f ? onePole (lpHz) : 0.0f;
        const float lAtt = msToCoef (limAttMs, sr), lRel = msToCoef (limRelMs, sr);
        const float lRelS = limSlowMs > 0.0f ? msToCoef (limSlowMs, sr) : 0.0f;
        const float makeup = s.makeupDb;
        float maxGr = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c)
            {
                float x = key != nullptr ? key->getSample (juce::jmin (c, key->getNumChannels() - 1), i) : buffer.getSample (c, i);
                auto& st = f[(size_t) c];
                if (feedback) x *= lastGain;                                   // feedback: it hears its own output
                if (kSc > 0.0f) { st.sc += kSc * (x - st.sc); x -= st.sc; }   // the shared sidechain high-pass
                if (kTilt > 0.0f) { st.tilt += kTilt * (x - st.tilt); x -= tiltAmt * st.tilt; }
                if (kLp > 0.0f) { st.lp += kLp * (x - st.lp); x = st.lp; }
                peak = juce::jmax (peak, std::abs (x));
            }
            float level = peak;
            if (kRms > 0.0f) { ms += kRms * ((double) peak * peak - ms); level = (float) std::sqrt (2.0 * ms); level = juce::jmax (level, peak * 0.5f); }
            const float lvlDb = juce::Decibels::gainToDecibels (level, -120.0f);
            const float target = curve (s, lvlDb - s.threshDb);
            fast = (target < fast ? att : rel) * (fast - target) + target;
            if (slowDepth > 0.0f)
            {
                const float st = target * slowDepth;
                slow = (st < slow ? attS : relS) * (slow - st) + st;
            }
            float gr = slowDepth > 0.0f ? juce::jmin (fast, slow) : fast;
            if (limOn)   // LE-336: the limiter after the compressor, on the compressed level
            {
                const float o2 = lvlDb + gr - s.limThreshDb;
                const float t2 = o2 > 0.0f ? -o2 * (1.0f - 1.0f / 20.0f) : 0.0f;
                lim = (t2 < lim ? lAtt : lRel) * (lim - t2) + t2;
                if (limDepth > 0.0f) { const float st = t2 * limDepth; limSlow = (st < limSlow ? attS : lRelS) * (limSlow - st) + st; }
                gr += limDepth > 0.0f ? juce::jmin (lim, limSlow) : lim;
            }
            maxGr = juce::jmax (maxGr, -gr);
            lastGain = juce::Decibels::decibelsToGain (gr);
            const float gain = juce::Decibels::decibelsToGain (gr + makeup);
            const float mixCol = juce::jlimit (0.0f, 0.5f, mixBase + mixPerDb * -gr);
            for (int c = 0; c < numCh; ++c)
            {
                const float dry = buffer.getSample (c, i);
                const float wet = mixCol > 0.0f ? sat.processSample (c, dry * gain, drive, bias, mixCol) : dry * gain;
                buffer.setSample (c, i, dry + s.mix * (wet - dry));
            }
        }
        return maxGr;
    }

private:
    double sr = 48000.0;
    float fast = 0.0f, slow = 0.0f, lim = 0.0f, limSlow = 0.0f, lastGain = 1.0f;
    double ms = 0.0;
    struct State { float sc = 0.0f, tilt = 0.0f, lp = 0.0f; };
    std::array<State, 2> f {};
    Saturator sat;
};

//==============================================================================
// Vocal rider: an automatic fader before the compressors (like a mix engineer riding the vocal).
// It compares the short-term level (~300 ms) with the vocal's own long-term average (~8 s) and moves
// the fader to close the gap, within +/- range. It holds still during silence, breaths and very quiet
// bits (it never boosts noise), and moves smoothly (~150 ms).
class VocalRider
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        hp = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 100.0, 0.707f);
        for (auto& f : hpf) f.coefficients = hp;
        reset();
    }
    void reset() { shortDb = -100.0f; refDb = -100.0f; gainDb = 0.0f; for (auto& f : hpf) f.reset(); }

    // returns the current ride (dB, + = pushed up, - = pulled down)
    float process (juce::AudioBuffer<float>& buffer, float rangeDb)
    {
        const int n = buffer.getNumSamples();
        const int nc = juce::jmin (2, buffer.getNumChannels());
        if (rangeDb <= 0.05f) { gainDb = 0.0f; return 0.0f; }

        // short-term level, followed in dB so it reacts as fast to a quieter phrase as to a louder one
        double e = 0.0;
        for (int i = 0; i < n; ++i)
        {
            float x = 0.0f;
            for (int c = 0; c < nc; ++c) x += hpf[(size_t) c].processSample (buffer.getSample (c, i));
            x /= (float) nc;
            e += (double) x * x;
        }
        const float blockDb = (float) (10.0 * std::log10 (e / (double) juce::jmax (1, n) + 1.0e-12));
        const float aShort = (float) std::exp (-(double) n / (0.25 * sr));
        if (blockDb > -60.0f)
            shortDb = shortDb < -90.0f ? blockDb : aShort * shortDb + (1.0f - aShort) * blockDb;
        const float aRef   = (float) std::exp (-(double) n / (8.0 * sr));
        const float aGain  = (float) std::exp (-1.0 / (0.15 * sr));
        float startDb = gainDb;
        const bool singing = blockDb > -50.0f;

        // long-term reference: only learns while the vocal is actually singing
        if (singing)
            refDb = refDb < -90.0f ? shortDb : aRef * refDb + (1.0f - aRef) * shortDb;

        // target: close the gap, within range; hold still in silences / breaths / very quiet bits
        float target = gainDb;
        if (singing && shortDb > refDb - 20.0f)
            target = juce::jlimit (-rangeDb, rangeDb, refDb - shortDb);

        for (int i = 0; i < n; ++i)
        {
            startDb = aGain * startDb + (1.0f - aGain) * target;
            const float g = juce::Decibels::decibelsToGain (startDb);
            for (int c = 0; c < nc; ++c) buffer.setSample (c, i, buffer.getSample (c, i) * g);
        }
        gainDb = startDb;
        return gainDb;
    }

private:
    double sr = 48000.0;
    float shortDb = -100.0f, refDb = -100.0f, gainDb = 0.0f;
    juce::dsp::IIR::Coefficients<float>::Ptr hp;
    std::array<juce::dsp::IIR::Filter<float>, 2> hpf;
};

//==============================================================================
// Breath control: finds breaths and turns them down by a set amount (instead of gating them out).
// A breath is airy (more energy at 3-9 kHz than in the 150-900 Hz voice body), clearly quieter than
// the singing (12-50 dB below the vocal's recent level) - so vowels, consonants and silence are left alone.
class BreathControl
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        loC = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 370.0, 0.8f);
        hiC = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 5200.0, 0.9f);
        for (int c = 0; c < 2; ++c) { lo[(size_t) c].coefficients = loC; hi[(size_t) c].coefficients = hiC; }
        reset();
    }
    void reset() { loEnv = hiEnv = fullEnv = 0.0f; vocalEnv = 0.0f; gain = 1.0f; for (auto& f : lo) f.reset(); for (auto& f : hi) f.reset(); }

    // returns the current reduction (dB, >= 0)
    float process (juce::AudioBuffer<float>& buffer, float breathDb)
    {
        const int n = buffer.getNumSamples();
        const int nc = juce::jmin (2, buffer.getNumChannels());
        if (breathDb >= -0.05f) { gain = 1.0f; return 0.0f; }

        const float env  = (float) std::exp (-1.0 / (0.010 * sr));   // 10 ms band followers
        const float vAtt = (float) std::exp (-1.0 / (0.005 * sr));
        const float vRel = (float) std::exp (-1.0 / (2.0 * sr));     // the vocal level, held ~2 s
        const float gAtt = (float) std::exp (-1.0 / (0.015 * sr));
        const float gRel = (float) std::exp (-1.0 / (0.060 * sr));
        const float dip = juce::Decibels::decibelsToGain (breathDb);
        float maxRed = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float x = 0.0f;
            for (int c = 0; c < nc; ++c) x += buffer.getSample (c, i);
            x /= (float) nc;
            const float l = lo[0].processSample (x), h = hi[0].processSample (x);
            loEnv   = env * loEnv   + (1.0f - env) * l * l;
            hiEnv   = env * hiEnv   + (1.0f - env) * h * h;
            fullEnv = env * fullEnv + (1.0f - env) * x * x;
            vocalEnv = fullEnv > vocalEnv ? vAtt * vocalEnv + (1.0f - vAtt) * fullEnv : vRel * vocalEnv + (1.0f - vRel) * fullEnv;

            const float fullDb  = 10.0f * std::log10 (fullEnv + 1.0e-12f);
            const float vocalDb = 10.0f * std::log10 (vocalEnv + 1.0e-12f);
            const bool airy = hiEnv > 0.7f * loEnv;
            const bool isBreath = airy && fullDb < vocalDb - 12.0f && fullDb > vocalDb - 50.0f && fullDb > -75.0f;

            const float target = isBreath ? dip : 1.0f;
            gain = target < gain ? gAtt * gain + (1.0f - gAtt) * target : gRel * gain + (1.0f - gRel) * target;
            for (int c = 0; c < nc; ++c) buffer.setSample (c, i, buffer.getSample (c, i) * gain);
            maxRed = juce::jmax (maxRed, -juce::Decibels::gainToDecibels (gain, -60.0f));
        }
        return maxRed;
    }

private:
    double sr = 48000.0;
    float loEnv = 0.0f, hiEnv = 0.0f, fullEnv = 0.0f, vocalEnv = 0.0f, gain = 1.0f;
    juce::dsp::IIR::Coefficients<float>::Ptr loC, hiC;
    std::array<juce::dsp::IIR::Filter<float>, 2> lo, hi;
};

//==============================================================================
// Split-band de-esser: only the band above the crossover is turned down,
// so the body of the voice is untouched. Linkwitz-Riley bands sum back flat.
class DeEsser
{
public:
    // Two styles:
    //   Split - only the band above Freq is turned down (transparent, modern)
    //   Wide  - the whole voice dips briefly on each "s", keyed from the band
    //           above Freq (classic hardware style, often more natural)
    // Auto  - threshold-less: sibilance is judged relative to the overall vocal
    //         level, so it keeps working when the singer gets louder or softer.
    //         The Threshold knob then acts as sensitivity.
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        crossover.prepare ({ sr, 4096, 2 });
        keyCrossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        keyCrossover.prepare ({ sr, 4096, 2 });
        reset();
    }

    void reset() { crossover.reset(); keyCrossover.reset(); env = 0.0f; fullEnv = 0.0f; }

    // Auto mode: how far the sibilance band may rise relative to the full signal (dB)
    static float autoTriggerDb (float thresholdDb) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, thresholdDb), -60.0f, 0.0f, -24.0f, 0.0f); }

    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        crossover.setCutoffFrequency (juce::jlimit (1000.0f, (float) (sr * 0.45), s.dsHz));
        keyCrossover.setCutoffFrequency (juce::jlimit (1000.0f, (float) (sr * 0.45), s.dsHz));

        const bool wide = s.dsMode == DsMode::Wide;
        // Wide uses a touch slower timing so the dip on the whole voice is smooth
        const float attack  = msToCoef (wide ? 1.0f : 0.5f, sr);
        const float release = msToCoef (wide ? 80.0f : 60.0f, sr);
        const float fullAtt = msToCoef (10.0f, sr), fullRel = msToCoef (150.0f, sr);
        constexpr float ratio = 4.0f;
        const float autoTrigger = autoTriggerDb (s.dsThreshDb);
        float maxGr = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float in[2] {}, low[2] {}, high[2] {};
            float peak = 0.0f, fullPeak = 0.0f;

            for (int c = 0; c < numCh; ++c)
            {
                in[c] = buffer.getSample (c, i);
                crossover.processSample (c, in[c], low[c], high[c]);
                if (key == nullptr)
                {
                    peak = juce::jmax (peak, std::abs (high[c]));
                    fullPeak = juce::jmax (fullPeak, std::abs (in[c]));
                }
            }
            if (key != nullptr)   // V64: listen to the external sidechain instead
                for (int c = 0; c < juce::jmin (2, key->getNumChannels()); ++c)
                {
                    float kl = 0.0f, kh = 0.0f;
                    const float kx = key->getSample (c, i);
                    keyCrossover.processSample (c, kx, kl, kh);
                    peak = juce::jmax (peak, std::abs (kh));
                    fullPeak = juce::jmax (fullPeak, std::abs (kx));
                }

            env = peak > env ? attack * env + (1.0f - attack) * peak
                             : release * env + (1.0f - release) * peak;
            fullEnv = fullPeak > fullEnv ? fullAtt * fullEnv + (1.0f - fullAtt) * fullPeak
                                         : fullRel * fullEnv + (1.0f - fullRel) * fullPeak;

            const float hfDb = juce::Decibels::gainToDecibels (env, -120.0f);
            float over;
            if (s.dsAuto)
            {
                // relative: sibilance band vs the vocal's overall level; gated below -60 dBFS
                const float fullDb = juce::Decibels::gainToDecibels (fullEnv, -120.0f);
                over = fullDb > -60.0f ? (hfDb - fullDb) - autoTrigger : -100.0f;
            }
            else
                over = hfDb - s.dsThreshDb;

            const float grDb = juce::jlimit (0.0f, s.dsRangeDb, over * (1.0f - 1.0f / ratio));
            const float g = juce::Decibels::decibelsToGain (-grDb);
            maxGr = juce::jmax (maxGr, grDb);

            for (int c = 0; c < numCh; ++c)
            {
                float out;
                if (s.dsListen)  out = high[c];                     // hear the sibilance band
                else if (wide)   out = in[c] * g;                   // whole voice dips
                else             out = low[c] + high[c] * g;        // only the top band dips
                buffer.setSample (c, i, out);
            }
        }
        return maxGr;
    }

    const juce::AudioBuffer<float>* key = nullptr;   // V64: external sidechain, or null
private:
    double sr = 48000.0;
    juce::dsp::LinkwitzRileyFilter<float> crossover, keyCrossover;
    float env = 0.0f, fullEnv = 0.0f;
};

//==============================================================================
// RVox-style gate: a soft downward expander (about 1:4 below threshold, up to
// 40 dB of range) with hysteresis and hold, so breaths and room noise duck
// smoothly without chopping the ends of words.
class VoxGate
{
public:
    static constexpr float offBelowDb = -79.9f;

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { env = 0.0f; gainDb = 0.0f; holdLeft = 0; open = true; }

    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        const bool active = s.gateThreshDb > offBelowDb;
        const float envAtt = msToCoef (0.5f, sr), envRel = msToCoef (40.0f, sr);
        const float opening = msToCoef (1.0f, sr), closing = msToCoef (120.0f, sr);
        const int holdSamples = (int) (0.06 * sr);
        constexpr float hysteresis = 4.0f, slope = 3.0f, range = 40.0f;
        float maxGr = 0.0f;

        float maxLevel = -120.0f;

        for (int i = 0; i < n; ++i)
        {
            float target = 0.0f;

            // detector always runs, so the gate's level meter works even with the gate off
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c)
                peak = juce::jmax (peak, std::abs (buffer.getSample (c, i)));
            env = peak > env ? envAtt * env + (1.0f - envAtt) * peak
                             : envRel * env + (1.0f - envRel) * peak;
            const float level = juce::Decibels::gainToDecibels (env, -120.0f);
            maxLevel = juce::jmax (maxLevel, level);

            if (active)
            {
                if (level > s.gateThreshDb)                     { open = true; holdLeft = holdSamples; }
                else if (level < s.gateThreshDb - hysteresis)   { if (holdLeft > 0) --holdLeft; else open = false; }

                if (! open)
                    target = juce::jmax (-range, (level - s.gateThreshDb) * slope);
            }

            gainDb = (target > gainDb ? opening : closing) * (gainDb - target) + target;
            maxGr = juce::jmax (maxGr, -gainDb);

            if (gainDb < -0.001f)
            {
                const float g = juce::Decibels::decibelsToGain (gainDb);
                for (int c = 0; c < numCh; ++c)
                    buffer.setSample (c, i, buffer.getSample (c, i) * g);
            }
        }
        lastLevelDb = maxLevel;
        return maxGr;
    }

    float getLevelDb() const { return lastLevelDb; }   // detector level for the gate meter

private:
    double sr = 48000.0;
    float env = 0.0f, gainDb = 0.0f, lastLevelDb = -120.0f;
    int holdLeft = 0;
    bool open = true;
};

//==============================================================================
// L3-style multiband stage: splits at 250 Hz and 4 kHz (phase-matched
// Linkwitz-Riley bands) and gently reins in whichever band is pushing the
// peaks, before the final brickwall. The mids get the lightest touch so the
// voice itself pumps less.
class MultibandPreLimiter
{
public:
    bool phaseOnly = false;   // same crossovers summed at unity, for the dry path
    float headroomDb = 3.0f, weightScale = 1.0f;   // V69 Spectral: acts only right at the ceiling, more gently

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sr, 4096, 2 };
        split1.prepare (spec); split2.prepare (spec); lowAllpass.prepare (spec);
        split1.setCutoffFrequency (250.0f);
        split2.setCutoffFrequency (4000.0f);
        lowAllpass.setCutoffFrequency (4000.0f);
        lowAllpass.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        reset();
    }

    void reset()
    {
        split1.reset(); split2.reset(); lowAllpass.reset();
        for (auto& e : env) e = 0.0f;
        for (auto& g : grDb) g = 0.0f;
    }

    // In-place on the already-driven signal. ceiling is linear.
    void processSample (float* x, int numCh, float ceiling)
    {
        float bands[3][2] {};
        float peak[3] {};

        for (int c = 0; c < numCh; ++c)
        {
            float low, rest, mid, high;
            split1.processSample (c, x[c], low, rest);
            split2.processSample (c, rest, mid, high);
            low = lowAllpass.processSample (c, low);
            bands[0][c] = low; bands[1][c] = mid; bands[2][c] = high;
            for (int b = 0; b < 3; ++b) peak[b] = juce::jmax (peak[b], std::abs (bands[b][c]));
        }

        const float ceilDb = juce::Decibels::gainToDecibels (ceiling);
        static constexpr float weight[3] { 0.8f, 0.45f, 0.8f };   // mids: lightest touch
        const float att = msToCoef (1.0f, sr), rel = msToCoef (80.0f, sr);
        float g[3];

        for (int b = 0; b < 3; ++b)
        {
            env[b] = peak[b] > env[b] ? att * env[b] + (1.0f - att) * peak[b]
                                      : rel * env[b] + (1.0f - rel) * peak[b];
            const float over = juce::Decibels::gainToDecibels (env[b], -120.0f) - (ceilDb - headroomDb);
            const float target = over > 0.0f ? -over * weight[b] * weightScale : 0.0f;
            grDb[b] = (target < grDb[b] ? att : rel) * (grDb[b] - target) + target;
            g[b] = juce::Decibels::decibelsToGain (grDb[b]);
        }

        if (phaseOnly) g[0] = g[1] = g[2] = 1.0f;
        for (int c = 0; c < numCh; ++c)
            x[c] = bands[0][c] * g[0] + bands[1][c] * g[1] + bands[2][c] * g[2];
    }

private:
    double sr = 48000.0;
    juce::dsp::LinkwitzRileyFilter<float> split1, split2, lowAllpass;
    float env[3] {}, grDb[3] {};
};

//==============================================================================
// Zerem (formerly Vox Flow): a 5-band processor with selectable styles, sitting between
// the de-esser and the limiter. Bands:
//   boom < 180 Hz < mud < 700 Hz < body < 2.5 kHz < harsh < 6.5 kHz < air.
// Each style sets every band's threshold, ratio, timing and tone, plus a
// gentle high-shelf "sheen". Amount scales both how hard the bands work and
// how much of the style's tone is applied (50% = the style as designed).
// Bands are phase-matched (Linkwitz-Riley + allpass compensation) and sum flat.
enum class FlowStyle { Pop = 0, Rap, MidForward, RnB, AiryBallad, Podcast, Dense, Drums, Bass, Guitar, Keys, MixBus };

struct FlowBand { float threshRel, ratio, attackMs, releaseMs, gainDb; };
struct FlowStyleInfo { const char* name; const char* description; FlowBand bands[5]; float sheenDb; };

inline const FlowStyleInfo& flowStyleInfo (int style)
{
    static const FlowStyleInfo styles[] = {
        { "Pop Vocal", "Polished and forward: tight lows, smooth top, gentle sheen",
          { { -12, 3.0f, 10, 120, -1.0f }, { -14, 2.5f, 15, 150, -1.0f }, { -8, 1.5f, 20, 200, 0.0f },
            { -18, 4.0f, 3, 80, 0.0f }, { -18, 3.0f, 1, 60, 0.0f } }, 2.0f },
        { "Rap Vocal", "Punchy and upfront: fast control, firm body, crisp top",
          { { -14, 4.0f, 5, 90, -0.5f }, { -15, 3.0f, 8, 110, -1.5f }, { -12, 2.0f, 8, 120, 1.0f },
            { -18, 4.0f, 2, 60, 0.0f }, { -18, 3.0f, 1, 50, 0.5f } }, 1.0f },
        { "Mid Forward", "Mid-range boost: pushes the body of the voice to the front",
          { { -12, 2.5f, 10, 120, -1.0f }, { -14, 2.5f, 12, 140, -1.0f }, { -10, 1.5f, 15, 180, 3.0f },
            { -16, 3.0f, 3, 80, 0.0f }, { -18, 2.5f, 2, 70, 0.0f } }, 0.0f },
        { "R&B Smooth", "Silky and warm: slow, gentle control and soft highs",
          { { -12, 2.5f, 20, 200, 0.0f }, { -14, 2.5f, 25, 220, -1.0f }, { -10, 1.5f, 30, 250, 0.0f },
            { -20, 5.0f, 6, 140, -0.5f }, { -20, 2.5f, 4, 120, 1.5f } }, 1.5f },
        { "Airy Ballad", "Open and breathy: light touch with lots of air",
          { { -12, 2.0f, 15, 180, -1.0f }, { -14, 2.0f, 20, 200, -0.5f }, { -8, 1.3f, 25, 250, 0.0f },
            { -18, 3.0f, 4, 100, 0.0f }, { -20, 2.0f, 2, 90, 1.0f } }, 3.0f },
        { "Podcast", "Clear speech: controlled lows, even level, tamed top",
          { { -12, 4.0f, 8, 120, -2.0f }, { -14, 3.0f, 10, 140, -2.0f }, { -12, 2.0f, 10, 150, 1.0f },
            { -18, 3.0f, 3, 80, 0.0f }, { -18, 2.0f, 2, 70, -1.0f } }, 0.0f },
        { "Dense", "Raise RMS: every band held tight for a thick, loud vocal",
          { { -18, 4.0f, 8, 100, 1.0f }, { -18, 4.0f, 10, 110, 1.0f }, { -16, 4.0f, 10, 120, 1.0f },
            { -20, 4.0f, 3, 70, 1.0f }, { -22, 4.0f, 2, 60, 1.0f } }, 1.0f },
        // Instrument and mix styles
        { "Drums", "Punch and snap: slower attacks let hits through, ring and boxiness tamed",
          { { -14, 3.0f, 20, 120, 1.0f }, { -14, 3.0f, 15, 120, -1.5f }, { -12, 2.0f, 15, 120, 0.0f },
            { -16, 3.0f, 5, 80, -0.5f }, { -18, 2.5f, 3, 60, 1.0f } }, 1.5f },
        { "Bass", "Even and solid: lows locked in, growl kept, top tidied",
          { { -18, 4.0f, 10, 150, 1.0f }, { -16, 3.0f, 10, 140, -1.0f }, { -12, 2.0f, 10, 120, 1.0f },
            { -16, 3.0f, 4, 80, -1.0f }, { -18, 2.0f, 2, 60, -1.0f } }, 0.0f },
        { "Guitar", "Focused and present: boom and fizz tamed, mids forward",
          { { -12, 3.0f, 10, 120, -2.0f }, { -14, 2.5f, 12, 140, -1.0f }, { -10, 1.5f, 15, 160, 1.5f },
            { -16, 3.5f, 3, 70, -1.0f }, { -18, 2.5f, 2, 60, 0.0f } }, 0.5f },
        { "Keys", "Wide and clear: gentle control, less mud, soft sparkle",
          { { -12, 2.0f, 20, 180, -1.0f }, { -14, 2.5f, 20, 200, -1.5f }, { -10, 1.5f, 25, 220, 0.0f },
            { -18, 2.5f, 5, 100, 0.0f }, { -18, 2.0f, 3, 90, 1.0f } }, 1.5f },
        { "Mix Bus", "Glue: light, slow control on every band for a polished mix",
          { { -12, 1.8f, 30, 200, 0.0f }, { -12, 1.6f, 30, 200, 0.0f }, { -12, 1.5f, 30, 200, 0.0f },
            { -14, 1.8f, 20, 160, 0.0f }, { -14, 1.6f, 20, 160, 0.0f } }, 1.0f },
    };
    return styles[juce::jlimit (0, (int) std::size (styles) - 1, style)];
}
inline constexpr int numFlowStyles = 12;

class FlowMultiband
{
public:
    static constexpr int numBands = 5;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec specs { sr, 4096, 2 };
        const float xo[4] { 180.0f, 700.0f, 2500.0f, 6500.0f };
        for (int i = 0; i < 4; ++i)
        {
            split[i].prepare (specs);
            split[i].setCutoffFrequency (juce::jmin (xo[i], (float) (sr * 0.4)));
        }
        // Allpass compensation: band k must also pass through every later crossover
        int n = 0;
        for (int band = 0; band < 3; ++band)
            for (int x = band + 1; x < 4; ++x)
            {
                ap[n].prepare (specs);
                ap[n].setType (juce::dsp::LinkwitzRileyFilterType::allpass);
                ap[n].setCutoffFrequency (juce::jmin (xo[x], (float) (sr * 0.4)));
                ++n;
            }
        for (auto& f : sheen)
        {
            f.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            f.prepare ({ sr, 4096, 1 });
        }
        configuredStyle = -1;
        configure (0, 0.5f);
        reset();
    }

    void reset()
    {
        for (auto& f : split) f.reset();
        for (auto& f : ap) f.reset();
        for (auto& f : sheen) f.reset();
        for (auto& g : grDb) g = 0.0f;
    }

    // Set style and amount (0..1). Cheap when nothing changed; never allocates.
    void configure (int style, float amount)
    {
        amount = juce::jlimit (0.0f, 1.0f, amount);
        if (style == configuredStyle && std::abs (amount - configuredAmount) < 1.0e-4f) return;
        configuredStyle = style;
        configuredAmount = amount;

        const auto& info = flowStyleInfo (style);
        const float tone = juce::jmin (1.5f, amount * 2.0f);          // 0 at 0%, 1 at 50%, 1.5 at 100%
        refDb = juce::jmap (amount, 0.0f, 1.0f, 8.0f, -10.0f) - 1.0f;  // -1 dBFS reference at 50%
        for (int b = 0; b < numBands; ++b)
        {
            const auto& sp = info.bands[b];
            att[b] = (float) std::exp (-1.0 / (sp.attackMs  * 0.001 * sr));
            rel[b] = (float) std::exp (-1.0 / (sp.releaseMs * 0.001 * sr));
            ratio[b] = sp.ratio;
            thresh[b] = refDb + sp.threshRel;
            makeup[b] = juce::Decibels::decibelsToGain (sp.gainDb * tone);
        }
        const auto shelf = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
            sr, juce::jmin (8000.0f, (float) (sr * 0.4)), 0.6f, juce::Decibels::decibelsToGain (info.sheenDb * tone));
        for (auto& f : sheen) *f.coefficients = shelf;
    }

    // phaseOnly: same crossovers summed at unity (no compression, no tone).
    // Used on the dry path so Mix blends without phase cancellation.
    bool phaseOnly = false;

    void processSample (float* x, int numCh)
    {
        float bands[numBands][2] {};
        float peak[numBands] {};

        for (int c = 0; c < numCh; ++c)
        {
            float rest = x[c];
            for (int i = 0; i < 4; ++i)
            {
                float lo, hi;
                split[i].processSample (c, rest, lo, hi);
                bands[i][c] = lo;
                rest = hi;
            }
            bands[4][c] = rest;

            int n = 0;
            for (int band = 0; band < 3; ++band)
                for (int xo = band + 1; xo < 4; ++xo)
                    bands[band][c] = ap[n++].processSample (c, bands[band][c]);

            for (int b = 0; b < numBands; ++b)
                peak[b] = juce::jmax (peak[b], std::abs (bands[b][c]));
        }

        if (phaseOnly)
        {
            for (int c = 0; c < numCh; ++c)
            {
                float sum = 0.0f;
                for (int b = 0; b < numBands; ++b) sum += bands[b][c];
                x[c] = sum;
            }
            return;
        }

        constexpr float knee = 6.0f;
        float g[numBands];
        for (int b = 0; b < numBands; ++b)
        {
            const float over = juce::Decibels::gainToDecibels (peak[b], -120.0f) - thresh[b];
            const float slope = 1.0f / ratio[b] - 1.0f;
            float target;
            if (2.0f * over < -knee)     target = 0.0f;
            else if (2.0f * over > knee) target = slope * over;
            else                         target = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);

            grDb[b] = (target < grDb[b] ? att[b] : rel[b]) * (grDb[b] - target) + target;
            g[b] = juce::Decibels::decibelsToGain (grDb[b]) * makeup[b];
        }

        for (int c = 0; c < numCh; ++c)
        {
            float sum = 0.0f;
            for (int b = 0; b < numBands; ++b) sum += bands[b][c] * g[b];
            x[c] = sheen[(size_t) c].processSample (sum);
        }
    }

    float getBandGrDb (int b) const { return -grDb[b]; }

private:
    double sr = 48000.0;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, 4> split;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, 6> ap;
    std::array<juce::dsp::IIR::Filter<float>, 2> sheen;
    float grDb[numBands] {}, att[numBands] {}, rel[numBands] {}, makeup[numBands] {}, ratio[numBands] {}, thresh[numBands] {};
    float refDb = -1.0f, configuredAmount = -1.0f;
    int configuredStyle = -1;
};

// The Vox Flow stage: styled multiband + phase-matched dry path for Mix
class VoxFlow
{
public:
    void prepare (double sampleRate)
    {
        wet.prepare (sampleRate);
        dry.phaseOnly = true;
        dry.prepare (sampleRate);
    }

    void reset() { wet.reset(); dry.reset(); for (auto& b : bandGr) b.store (0.0f); }

    // Returns the deepest band gain reduction this block (dB, >= 0)
    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int numCh = juce::jmin (2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        wet.configure (s.flowStyle, s.flowAmount);
        const float mix = juce::jlimit (0.0f, 1.0f, s.flowMix);
        float bandMax[FlowMultiband::numBands] {};

        for (int i = 0; i < n; ++i)
        {
            float w[2] {}, d[2] {};
            for (int c = 0; c < numCh; ++c) w[c] = d[c] = buffer.getSample (c, i);
            wet.processSample (w, numCh);
            dry.processSample (d, numCh);
            for (int c = 0; c < numCh; ++c)
                buffer.setSample (c, i, d[c] + mix * (w[c] - d[c]));
            for (int b = 0; b < FlowMultiband::numBands; ++b)
                bandMax[b] = juce::jmax (bandMax[b], wet.getBandGrDb (b));
        }

        float deepest = 0.0f;
        for (int b = 0; b < FlowMultiband::numBands; ++b)
        {
            bandGr[(size_t) b].store (bandMax[b] * mix);
            deepest = juce::jmax (deepest, bandMax[b] * mix);
        }
        return deepest;
    }

    std::array<std::atomic<float>, FlowMultiband::numBands> bandGr {};   // for the display

private:
    FlowMultiband wet, dry;
};

//==============================================================================
// Look-ahead brickwall limiter. The gain curve is a sliding minimum followed by
// a moving average over the look-ahead window, which guarantees the output
// never passes the ceiling while keeping the gain changes smooth.
//==============================================================================
// Exact ITU-R BS.1770 K-weighting at any sample rate (the standard's pre-filter shelf + RLB high-pass,
// designed the same way as libebur128), so readings match broadcast meters.
struct KWeighting
{
    juce::dsp::IIR::Coefficients<float>::Ptr shelf, highPass;
    explicit KWeighting (double fs)
    {
        const double pi = juce::MathConstants<double>::pi;
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (pi * f0 / fs), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            shelf = new juce::dsp::IIR::Coefficients<float> ((float) ((Vh + Vb * K / Q + K * K) / a0), (float) (2.0 * (K * K - Vh) / a0),
                                                            (float) ((Vh - Vb * K / Q + K * K) / a0), 1.0f,
                                                            (float) (2.0 * (K * K - 1.0) / a0), (float) ((1.0 - K / Q + K * K) / a0));
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (pi * f0 / fs), a0 = 1.0 + K / Q + K * K;
            highPass = new juce::dsp::IIR::Coefficients<float> (1.0f, -2.0f, 1.0f, 1.0f,
                                                               (float) (2.0 * (K * K - 1.0) / a0), (float) ((1.0 - K / Q + K * K) / a0));
        }
    }
};

//==============================================================================
// Loudness meter to ITU-R BS.1770-4 / EBU R128: K-weighting, 400 ms blocks every 100 ms,
// short-term = last 3 s, integrated = gated (absolute -70 LUFS, then relative -10 LU).
// Allocation-free on the audio thread (fixed history + a 0.1 dB histogram for the gating).
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        chunkLen = juce::jmax (1, (int) std::round (sampleRate * 0.1));
        const KWeighting kw (sampleRate);
        for (int c = 0; c < 2; ++c) { kShelf[(size_t) c].coefficients = kw.shelf; kHp[(size_t) c].coefficients = kw.highPass; }
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c) { kShelf[(size_t) c].reset(); kHp[(size_t) c].reset(); }
        chunks.fill (0.0); chunkCount = 0; chunkAcc = 0.0; chunkPos = 0;
        resetIntegrated();
    }

    void resetIntegrated()
    {
        histCount.fill (0); histEnergy.fill (0.0);
        integrated.store (-100.0f);
    }

    void process (const juce::AudioBuffer<float>& b)
    {
        if (resetRequest.exchange (false)) resetIntegrated();
        const int n = b.getNumSamples(), nc = juce::jmin (2, b.getNumChannels());
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < nc; ++c)
            {
                const float y = kHp[(size_t) c].processSample (kShelf[(size_t) c].processSample (b.getSample (c, i)));
                chunkAcc += (double) y * y;
            }
            if (++chunkPos >= chunkLen) finishChunk();
        }
    }

    std::atomic<float> shortTerm { -100.0f }, integrated { -100.0f };
    std::atomic<bool> resetRequest { false };   // the UI asks; the audio thread resets

private:
    static constexpr int numChunks = 30;          // 3 s of 100 ms chunks
    static constexpr int histBins = 800;          // -70 ... +10 LUFS in 0.1 dB steps
    static float lufs (double meanSquare) { return meanSquare > 0.0 ? (float) (-0.691 + 10.0 * std::log10 (meanSquare)) : -100.0f; }

    void finishChunk()
    {
        chunks[(size_t) (chunkCount % numChunks)] = chunkAcc / (double) chunkLen;
        ++chunkCount; chunkAcc = 0.0; chunkPos = 0;

        // short-term: mean of the last 30 chunks (3 s)
        const int have = (int) juce::jmin ((juce::int64) numChunks, chunkCount);
        double st = 0.0;
        for (int k = 0; k < have; ++k) st += chunks[(size_t) k];
        shortTerm.store (have >= numChunks ? lufs (st / have) : lufs (st / juce::jmax (1, have)));

        // 400 ms gating block (75 % overlap): the last 4 chunks
        if (chunkCount >= 4)
        {
            double e = 0.0;
            for (int k = 1; k <= 4; ++k) e += chunks[(size_t) ((chunkCount - k) % numChunks)];
            e /= 4.0;
            const float lb = lufs (e);
            if (lb > -70.0f)   // absolute gate
            {
                const int bin = juce::jlimit (0, histBins - 1, (int) ((lb + 70.0f) * 10.0f));
                ++histCount[(size_t) bin]; histEnergy[(size_t) bin] += e;
            }
            // relative gate: -10 LU below the absolute-gated mean
            double sum = 0.0; juce::int64 cnt = 0;
            for (int k = 0; k < histBins; ++k) { sum += histEnergy[(size_t) k]; cnt += histCount[(size_t) k]; }
            if (cnt > 0)
            {
                const float rel = lufs (sum / (double) cnt) - 10.0f;
                const int from = juce::jlimit (0, histBins, (int) std::ceil ((rel + 70.0f) * 10.0f));
                double s2 = 0.0; juce::int64 c2 = 0;
                for (int k = from; k < histBins; ++k) { s2 += histEnergy[(size_t) k]; c2 += histCount[(size_t) k]; }
                integrated.store (c2 > 0 ? lufs (s2 / (double) c2) : -100.0f);
            }
        }
    }

    std::array<juce::dsp::IIR::Filter<float>, 2> kShelf, kHp;
    std::array<double, numChunks> chunks {};
    std::array<juce::int64, histBins> histCount {};
    std::array<double, histBins> histEnergy {};
    juce::int64 chunkCount = 0;
    double chunkAcc = 0.0;
    int chunkLen = 4800, chunkPos = 0;
};

//==============================================================================
// Soft saturation, in the spirit of the Lavry AD converters' digital "Soft Saturation":
// no attack or release - every sample above the threshold is rounded off instantly on a smooth curve
// (slope 1 at the threshold, so it's seamless), signal below the threshold is untouched, then make-up
// gain brings the peaks back to full scale. Setting G dB: threshold at -2G dBFS, peaks held under -G dBFS,
// then +G dB. So 3 dB = threshold -6 dBFS, +3 dB; 6 dB = threshold -12 dBFS, +6 dB (as on the hardware).
// V62: a clipper in front of the limiter: pushes the signal up by N dB and shaves the peaks that would cross the
// limiter's knee with a short, smooth knee, so the limiter has less to do (louder, punchier masters).
struct Clipper
{
    static float amountDb (int setting)
    {
        static constexpr float dbs[6] { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f };
        return dbs[juce::jlimit (0, 5, setting)];
    }
    // clipLevel = the level the limiter maps onto its ceiling; returns the most it shaved (dB)
    static float process (juce::AudioBuffer<float>& buffer, int setting, float clipLevel)
    {
        const float d = amountDb (setting);
        if (d <= 0.0f) return 0.0f;
        const float gain = juce::Decibels::decibelsToGain (d);
        const float knee = 0.85f * clipLevel, span = clipLevel - knee;
        float most = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float v = x[i] * gain, a = std::abs (v);
                if (a > knee)
                {
                    const float shaped = knee + span * std::tanh ((a - knee) / span);
                    most = juce::jmax (most, a / shaped);
                    x[i] = std::copysign (shaped, v);
                }
                else x[i] = v;
            }
        }
        return juce::Decibels::gainToDecibels (juce::jmax (1.0f, most));
    }
};

struct SoftSaturation
{
    static float amountDb (int setting)
    {
        static constexpr float dbs[5] { 0.0f, 2.0f, 3.0f, 4.0f, 6.0f };
        return dbs[juce::jlimit (0, 4, setting)];
    }

    // returns the largest peak rounding this block (dB, before the make-up gain)
    static float process (juce::AudioBuffer<float>& buffer, int setting)
    {
        const float g = amountDb (setting);
        if (g <= 0.0f) return 0.0f;
        const float t = juce::Decibels::decibelsToGain (-2.0f * g);   // threshold
        const float c = juce::Decibels::decibelsToGain (-g);          // peaks never exceed this
        const float span = c - t;
        const float makeup = juce::Decibels::decibelsToGain (g);
        float most = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float x = d[i], a = std::abs (x);
                float y = x;
                if (a > t)
                {
                    const float shaped = t + span * std::tanh ((a - t) / span);
                    y = std::copysign (shaped, x);
                    most = juce::jmax (most, a / shaped);
                }
                d[i] = y * makeup;
            }
        }
        return most > 1.0f ? juce::Decibels::gainToDecibels (most) : 0.0f;
    }
};

//==============================================================================
// Input peak catcher, in the spirit of the Spectra Sonics 610 limiter:
// a fixed catch point (-6 dBFS) right after the Input knob, so how hard it works depends on how hot
// you drive the input. 0.25 ms look-ahead catches each peak before it arrives, with no overshoot;
// release is fast after a lone spike and slower when it works continuously. Stereo-linked.
// Always runs (passing through when off) so the plugin's latency never changes.
class PeakCatcher
{
public:
    static constexpr double lookaheadSeconds = 0.00025;
    static constexpr float thresholdDb = -6.0f, kneeDb = 2.0f;

    // lookaheadOverride: when oversampled, a whole multiple of the base-rate look-ahead (keeps latency an integer)
    void prepare (double sampleRate, int lookaheadOverride = 0)
    {
        sr = sampleRate;
        L = lookaheadOverride > 0 ? lookaheadOverride : juce::jmax (1, (int) std::round (lookaheadSeconds * sr));
        win = L + 1;
        need.assign ((size_t) win, 1.0f);
        smooth.assign ((size_t) win, 1.0f);
        for (auto& d : delay) d.assign ((size_t) L, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (need.begin(), need.end(), 1.0f);
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        held = 1.0f; avgGrDb = 0.0f;
        sum = (double) win;
        std::fill (smooth.begin(), smooth.end(), 1.0f);
        wIdx = dIdx = 0;
    }

    int getLatencySamples() const { return zeroLatency ? 0 : L; }
    int getLookaheadSamples() const { return L; }
    bool zeroLatency = false;   // tracking mode: instant attack, no look-ahead

    // returns the largest gain reduction this block (dB, positive)
    float process (juce::AudioBuffer<float>& buffer, bool on)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        const float thr = juce::Decibels::decibelsToGain (thresholdDb);
        const float avgCoef = (float) std::exp (-1.0 / (0.2 * sr));   // ~200 ms memory of how busy it is
        float maxGr = 0.0f;

        sum = 0.0;
        for (auto v : smooth) sum += v;

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c) peak = juce::jmax (peak, std::abs (buffer.getSample (c, i)));

            // gain this sample needs (soft knee around the catch point)
            float g = 1.0f;
            if (on && peak > thr * juce::Decibels::decibelsToGain (-kneeDb * 0.5f))
            {
                const float overDb = juce::Decibels::gainToDecibels (peak) - thresholdDb;
                const float grDb = overDb >= kneeDb * 0.5f ? overDb
                                                           : (overDb + kneeDb * 0.5f) * (overDb + kneeDb * 0.5f) / (2.0f * kneeDb);
                g = juce::Decibels::decibelsToGain (-grDb);
            }

            // hold the lowest gain seen over the look-ahead, release smoothly after it
            need[(size_t) wIdx] = g;
            float lowest = 1.0f;
            for (auto v : need) lowest = juce::jmin (lowest, v);
            const float relMs = 8.0f + 70.0f * juce::jlimit (0.0f, 1.0f, avgGrDb / 1.5f);   // program-dependent release
            const float relCoef = (float) std::exp (-1.0 / (relMs * 0.001 * sr));
            held = lowest < held ? lowest : lowest + (held - lowest) * relCoef;

            // average the held gain over the window: the gain is fully down exactly when the peak arrives
            sum += (double) held - (double) smooth[(size_t) wIdx];
            smooth[(size_t) wIdx] = held;
            const float gain = juce::jmin (1.0f, (float) (sum / win));
            if (++wIdx >= win) wIdx = 0;

            if (zeroLatency)   // no look-ahead: act on this very sample (instant attack, same release)
            {
                const float g0 = juce::jmin (gain, held);
                for (int c = 0; c < numCh; ++c) buffer.setSample (c, i, buffer.getSample (c, i) * g0);
            }
            else
            {
                for (int c = 0; c < numCh; ++c)
                {
                    auto& d = delay[(size_t) c];
                    const float x = buffer.getSample (c, i);
                    const float y = d[(size_t) dIdx];
                    d[(size_t) dIdx] = x;
                    buffer.setSample (c, i, y * gain);
                }
                if (++dIdx >= L) dIdx = 0;
            }

            // "how busy is it" follows the peaks themselves, not its own release, so a lone spike stays fast
            const float needDb = -juce::Decibels::gainToDecibels (g, -60.0f);
            avgGrDb = avgCoef * avgGrDb + (1.0f - avgCoef) * needDb;
            maxGr = juce::jmax (maxGr, -juce::Decibels::gainToDecibels (gain, -60.0f));
        }
        return maxGr;
    }

private:
    double sr = 48000.0, sum = 0.0;
    int L = 12, win = 13, wIdx = 0, dIdx = 0;
    std::vector<float> need, smooth;
    std::array<std::vector<float>, 2> delay;
    float held = 1.0f, avgGrDb = 0.0f;
};

//==============================================================================
// Tape: pre-emphasis -> 2x oversampled tape saturation -> de-emphasis -> head bump -> high-frequency
// roll-off. Each tape speed is a set of "best settings" (like real tape, the slower the speed, the lower
// the head bump, the softer the top and the more saturation):
//   30 ips  - best for vocals: gentle, smooths harsh highs and esses (the pre-emphasised top saturates
//             first), small bump around 90 Hz, open top
//   15 ips  - best for drums: rounds transients for punch and glue, fuller bump around 55 Hz, softer top
//   7.5 ips - best for guitars: warmest, richest harmonics, low bump around 40 Hz, a touch of mid
//             presence, smooth top
// Drive 50% = the recommended level for the style (+/-12 dB around it). Mix blends only the
// saturation, so blending never causes phasiness. Off = a pure delay matching the oversampler,
// with a 20 ms crossfade, so latency never changes and the plugin still nulls.
enum class TapeStyle { Ips30 = 0, Ips15, Ips7_5 };

struct TapeStyleInfo
{
    const char* name; const char* blurb;
    float emphDb, emphHz, baseDrive, bias, bumpHz, bumpDb, dipDb, hfHz, midDb;
};
inline const TapeStyleInfo& tapeStyleInfo (int i)
{
    static const TapeStyleInfo info[3] {
        { "30 ips",  "Best for vocals: smooth highs, gentle glue",            4.0f, 5000.0f, 1.3f, 0.10f, 90.0f, 0.8f, -0.3f, 20000.0f, 0.0f },
        { "15 ips",  "Best for drums: rounded transients, punch and weight",  3.0f, 5000.0f, 2.2f, 0.05f, 55.0f, 2.0f, -0.7f, 16000.0f, 0.0f },
        { "7.5 ips", "Best for guitars: warm harmonics, smooth top",          2.0f, 4500.0f, 2.5f, 0.22f, 40.0f, 1.5f, -0.5f, 12000.0f, 0.8f },
    };
    return info[juce::jlimit (0, 2, i)];
}
inline constexpr int numTapeStyles = 3;

// Tape width shapes the character on top of the speed:
//   1/4"  narrow, least headroom: saturates soonest, slightly softer top
//   1/2"  mastering format, most headroom: cleanest, wide extended low end
//   1"    balanced (the speeds' reference tuning)
//   2"    big multitrack sound: punchiest, fullest low-end bump, most glue
struct TapeWidthInfo { const char* name; float driveMul, bumpMul, bumpHzMul, hfMul; };
inline const TapeWidthInfo& tapeWidthInfo (int i)
{
    static const TapeWidthInfo info[4] {
        { "1/4\"", 1.45f, 0.80f, 1.00f, 0.85f },
        { "1/2\"", 0.75f, 1.20f, 0.90f, 1.10f },
        { "1\"",   1.00f, 1.00f, 1.00f, 1.00f },
        { "2\"",   1.20f, 1.40f, 0.90f, 0.95f },
    };
    return info[juce::jlimit (0, 3, i)];
}
inline constexpr int numTapeWidths = 4;

// Tape hiss level (dBFS RMS, before the tape's de-emphasis and roll-off shape it).
// Slower speeds and narrower tape hiss more, like the real thing.
inline float tapeHissDb (int style, int width)
{
    static constexpr float bySpeed[3] { -66.0f, -61.0f, -55.0f };   // 30, 15, 7.5 ips
    static constexpr float byWidth[4] { 3.0f, -3.0f, 0.0f, -2.0f }; // 1/4", 1/2", 1", 2"
    return bySpeed[juce::jlimit (0, 2, style)] + byWidth[juce::jlimit (0, 3, width)];
}

class TapeStage
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = juce::jmax (32, maxBlockSize);
        lat[0] = 0;
        for (int k = 0; k < 2; ++k)   // 2x (one stage) and 4x (two stages); integer latency
        {
            os[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (
                2, (size_t) (k + 1), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
            os[(size_t) k]->initProcessing ((size_t) maxBlock);
            lat[(size_t) k + 1] = (int) std::round (os[(size_t) k]->getLatencyInSamples());
        }
        dryDelay.setSize (2, lat[2] + 1);
        wet.setSize (2, maxBlock);
        for (auto* c : { &preC, &deC, &bumpC, &dipC, &hfC, &midC })
            *c = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass (sr, 1000.0, 0.7));
        for (int ch = 0; ch < 2; ++ch)
        {
            pre[(size_t) ch].coefficients = preC;   de[(size_t) ch].coefficients = deC;
            bump[(size_t) ch].coefficients = bumpC; dip[(size_t) ch].coefficients = dipC;
            hf[(size_t) ch].coefficients = hfC;     mid[(size_t) ch].coefficients = midC;
        }
        for (int k = 0; k < 3; ++k) sats[(size_t) k].prepare (sr * (double) (1 << k));
        fade.reset (sr, 0.02);
        fade.setCurrentAndTargetValue (0.0f);
        hiss.reset (sr, 0.03);
        hiss.setCurrentAndTargetValue (0.0f);
        lastStyle = -1;
        reset();
    }

    void reset()
    {
        resetTape();
        dryDelay.clear();
        dPos = 0;
    }

    // clears only the tape's own state (the plain-signal delay keeps running, so switching on never clicks)
    void resetTape()
    {
        for (auto& o : os) if (o) o->reset();
        for (int ch = 0; ch < 2; ++ch)
            for (auto* f : { &pre[(size_t) ch], &de[(size_t) ch], &bump[(size_t) ch], &dip[(size_t) ch], &hf[(size_t) ch], &mid[(size_t) ch] })
                f->reset();
        for (auto& st : sats) st.reset();
    }

    int getLatencySamples() const { return zeroLatency ? 0 : lat[(size_t) osIdx]; }
    int latencyFor (int osSetting) const { return lat[(size_t) juce::jlimit (0, 2, osSetting)]; }
    bool zeroLatency = false;   // tracking mode: saturate at the base rate, no oversampler delay

    // returns the output level (dBFS RMS) for the VU meter, or -100 when off
    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int n = buffer.getNumSamples();
        const int nc = juce::jmin (2, buffer.getNumChannels());
        if (n > maxBlock || os[0] == nullptr) return -100.0f;

        // oversampling setting (tracking mode always runs at the base rate)
        const int want = zeroLatency ? 0 : juce::jlimit (0, 2, s.tapeOs);
        if (want != osIdx) { osIdx = want; resetTape(); dryDelay.clear(); }
        const int L = lat[(size_t) osIdx];
        auto& sat = sats[(size_t) osIdx];

        fade.setTargetValue (s.tapeOn ? 1.0f : 0.0f);
        const bool needWet = s.tapeOn || fade.isSmoothing() || fade.getCurrentValue() > 0.0f;

        // the "off" path: a plain delay matching the oversampler, always running
        const int len = dryDelay.getNumSamples();
        for (int c = 0; c < nc; ++c)
        {
            auto* line = dryDelay.getWritePointer (c);
            const auto* in = buffer.getReadPointer (c);
            auto* out = buffer.getWritePointer (c);
            auto* w = wet.getWritePointer (c);
            int pos = dPos;
            for (int i = 0; i < n; ++i)
            {
                const float x = in[i];
                w[i] = x;                                  // wet starts from the input
                line[pos] = x;
                int rd = pos - L; if (rd < 0) rd += len;
                out[i] = line[rd];
                if (++pos >= len) pos = 0;
            }
        }
        dPos = (dPos + n) % len;
        if (! needWet) { lastStyle = -1; return -100.0f; }

        const int key = s.tapeStyle * 8 + s.tapeWidth;
        if (key != lastStyle) { design (s.tapeStyle, s.tapeWidth); if (fade.getCurrentValue() <= 0.0f) resetTape(); lastStyle = key; }
        const auto& st = tapeStyleInfo (s.tapeStyle);
        const float drive = st.baseDrive * tapeWidthInfo (s.tapeWidth).driveMul * juce::Decibels::decibelsToGain ((juce::jlimit (0.0f, 1.0f, s.tapeDrive) - 0.5f) * 24.0f);
        const float mixAmt = juce::jlimit (0.0f, 1.0f, s.tapeMix);
        // hiss amplitude for a uniform source (RMS x sqrt 3), glided so the button never clicks
        hiss.setTargetValue (s.tapeNoise ? juce::Decibels::decibelsToGain (tapeHissDb (s.tapeStyle, s.tapeWidth)) * 1.7320508f : 0.0f);

        // pre-emphasis (highs pushed into the tape first, like the real record EQ)
        for (int c = 0; c < nc; ++c)
        {
            auto* w = wet.getWritePointer (c);
            for (int i = 0; i < n; ++i) w[i] = pre[(size_t) c].processSample (w[i]);
        }
        // saturation at 2x (tracking mode: at the base rate, so there's no oversampler delay)
        juce::dsp::AudioBlock<float> block (wet.getArrayOfWritePointers(), (size_t) nc, (size_t) n);
        if (osIdx == 0)
        {
            for (int c = 0; c < nc; ++c)
            {
                auto* d = wet.getWritePointer (c);
                for (int i = 0; i < n; ++i) d[i] = sat.processSample (c, d[i], drive, st.bias, mixAmt);
            }
        }
        else
        {
            auto& oversampler = os[(size_t) osIdx - 1];
            auto up = oversampler->processSamplesUp (block);
            for (size_t c = 0; c < up.getNumChannels(); ++c)
            {
                auto* d = up.getChannelPointer (c);
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                    d[i] = sat.processSample ((int) c, d[i], drive, st.bias, mixAmt);
            }
            oversampler->processSamplesDown (block);
        }
        // de-emphasis, head bump (+ the dip above it), mid presence, high-frequency roll-off
        double e = 0.0;
        for (int c = 0; c < nc; ++c)
        {
            auto* w = wet.getWritePointer (c);
            auto* out = buffer.getWritePointer (c);
            auto hissGain = hiss;   // same glide on both channels
            for (int i = 0; i < n; ++i)
            {
                const float h = hissGain.getNextValue();
                if (h > 0.0f) w[i] += h * noise (c);   // hiss goes on the tape, then gets its EQ like the signal
                float y = de[(size_t) c].processSample (w[i]);
                y = bump[(size_t) c].processSample (y);
                y = dip[(size_t) c].processSample (y);
                if (st.midDb != 0.0f) y = mid[(size_t) c].processSample (y);
                y = hf[(size_t) c].processSample (y);
                w[i] = y;
                e += (double) y * y;
            }
            juce::ignoreUnused (out);
        }
        hiss.skip (n);
        // crossfade from the plain (delayed) signal to the tape
        for (int i = 0; i < n; ++i)
        {
            const float a = fade.getNextValue();
            for (int c = 0; c < nc; ++c)
            {
                auto* out = buffer.getWritePointer (c);
                out[i] = out[i] + a * (wet.getSample (c, i) - out[i]);
            }
        }
        return juce::Decibels::gainToDecibels ((float) std::sqrt (e / (double) juce::jmax (1, n * nc)), -100.0f);
    }

private:
    void design (int style, int width)
    {
        using AC = juce::dsp::IIR::ArrayCoefficients<float>;
        const auto& st = tapeStyleInfo (style);
        const auto& wd = tapeWidthInfo (width);
        const double nyq = sr * 0.45;
        const float bumpHz = st.bumpHz * wd.bumpHzMul;
        *preC  = AC::makeHighShelf (sr, st.emphHz, 0.7f, juce::Decibels::decibelsToGain (st.emphDb));
        *deC   = AC::makeHighShelf (sr, st.emphHz, 0.7f, juce::Decibels::decibelsToGain (-st.emphDb));
        *bumpC = AC::makePeakFilter (sr, bumpHz, 1.2f, juce::Decibels::decibelsToGain (st.bumpDb * wd.bumpMul));
        *dipC  = AC::makePeakFilter (sr, bumpHz * 2.3f, 1.5f, juce::Decibels::decibelsToGain (st.dipDb * wd.bumpMul));
        *midC  = AC::makePeakFilter (sr, 1600.0, 0.7f, juce::Decibels::decibelsToGain (st.midDb));
        *hfC   = AC::makeLowPass (sr, juce::jmin ((double) (st.hfHz * wd.hfMul), nyq), 0.6f);
    }

    double sr = 48000.0;
    int maxBlock = 512, dPos = 0, lastStyle = -1, osIdx = 1;
    std::array<int, 3> lat {};   // latency at 1x, 2x, 4x
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> os;   // 2x, 4x
    juce::AudioBuffer<float> dryDelay, wet;
    juce::dsp::IIR::Coefficients<float>::Ptr preC, deC, bumpC, dipC, hfC, midC;
    std::array<juce::dsp::IIR::Filter<float>, 2> pre, de, bump, dip, hf, mid;
    std::array<Saturator, 3> sats;   // prepared for 1x, 2x and 4x
    juce::SmoothedValue<float> fade, hiss;

    // independent white noise per channel (xorshift32: cheap, no allocation), uniform -1..1
    std::array<juce::uint32, 2> rng { 0x9e3779b9u, 0x7f4a7c15u };
    float noise (int ch)
    {
        auto& x = rng[(size_t) ch];
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        return (float) x * (2.0f / 4294967295.0f) - 1.0f;
    }
};

//==============================================================================
// V69 "Bands" (in the spirit of the multiband IRC 5): four bands, each with its own gain and its own speed.
// When the sum would cross the ceiling, the bands carrying the most peak energy are turned down first,
// the lows slowly (no distortion), the highs quickly (no dulling). The final limiter catches what's left.
class FourBandPreLimiter
{
public:
    bool phaseOnly = false;
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sr, 4096, 2 };
        for (auto* f : { &x1, &x2, &x3, &ap2a, &ap3a, &ap3b }) f->prepare (spec);
        x1.setCutoffFrequency (120.0f); x2.setCutoffFrequency (1000.0f); x3.setCutoffFrequency (6000.0f);
        ap2a.setType (juce::dsp::LinkwitzRileyFilterType::allpass); ap2a.setCutoffFrequency (1000.0f);   // low band: 1k + 6k allpasses
        ap3a.setType (juce::dsp::LinkwitzRileyFilterType::allpass); ap3a.setCutoffFrequency (6000.0f);
        ap3b.setType (juce::dsp::LinkwitzRileyFilterType::allpass); ap3b.setCutoffFrequency (6000.0f);   // low-mid band: 6k allpass
        reset();
    }
    void reset()
    {
        for (auto* f : { &x1, &x2, &x3, &ap2a, &ap3a, &ap3b }) f->reset();
        for (auto& g : gain) g = 1.0f;
        for (auto& e : env) e = 0.0f;
        envAll = 0.0f;
    }
    // just the split (phase-matched, sums flat): b[band][channel]
    void split (const float* x, int numCh, float (&b)[4][2])
    {
        for (int c = 0; c < numCh; ++c)
        {
            float lo, rest, lm, up, mh, hi;
            x1.processSample (c, x[c], lo, rest);
            x2.processSample (c, rest, lm, up);
            x3.processSample (c, up, mh, hi);
            lo = ap3a.processSample (c, ap2a.processSample (c, lo));
            lm = ap3b.processSample (c, lm);
            b[0][c] = lo; b[1][c] = lm; b[2][c] = mh; b[3][c] = hi;
        }
    }
    // x: the driven samples (in place); ceiling linear; charMul scales the release times
    void processSample (float* x, int numCh, float ceiling, float charMul)
    {
        float b[4][2] {};
        float pk[4] {}, all = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            all = juce::jmax (all, std::abs (x[c]));
            float lo, rest, lm, up, mh, hi;
            x1.processSample (c, x[c], lo, rest);
            x2.processSample (c, rest, lm, up);
            x3.processSample (c, up, mh, hi);
            lo = ap3a.processSample (c, ap2a.processSample (c, lo));
            lm = ap3b.processSample (c, lm);
            b[0][c] = lo; b[1][c] = lm; b[2][c] = mh; b[3][c] = hi;
            for (int k = 0; k < 4; ++k) pk[k] = juce::jmax (pk[k], std::abs (b[k][c]));
        }
        if (phaseOnly) { for (int c = 0; c < numCh; ++c) x[c] = b[0][c] + b[1][c] + b[2][c] + b[3][c]; return; }
        // how far the (worst-case) sum is over a point just under the ceiling, and each band's share of it
        float total = 0.0f;
        for (int k = 0; k < 4; ++k) { env[k] = juce::jmax (pk[k], env[k] * envRel); total += env[k]; }
        envAll = juce::jmax (all, envAll * envRel);   // the real (broadband) peak decides how much is needed
        const float knee = ceiling * 0.95f;
        static constexpr float relMs[4] { 220.0f, 90.0f, 45.0f, 25.0f };
        // the bands carrying the most peak energy give the most (weighted by their share squared), each at most
        // -6 dB, so the tone only leans a little; bands that hardly contribute stay put (less pumping elsewhere)
        float target[4] { 1.0f, 1.0f, 1.0f, 1.0f };
        const float needed = envAll - knee;
        if (needed > 0.0f && total > 1.0e-9f)
        {
            float w[4], wSum = 0.0f;
            for (int k = 0; k < 4; ++k) { const float sh = env[k] / total; w[k] = sh * sh; wSum += w[k]; }
            for (int k = 0; k < 4; ++k)
                if (env[k] > 1.0e-9f)
                    target[k] = juce::jlimit (0.25f, 1.0f, 1.0f - needed * (w[k] / wSum) / env[k]);
        }
        for (int k = 0; k < 4; ++k)
        {
            const float rel = 1.0f - msToCoef (relMs[k] * charMul, sr);
            const float att = 1.0f - msToCoef (1.5f, sr);
            gain[k] += (target[k] - gain[k]) * (target[k] < gain[k] ? att : rel);   // quick down, each band its own speed up
        }
        for (int c = 0; c < numCh; ++c)
            x[c] = b[0][c] * gain[0] + b[1][c] * gain[1] + b[2][c] * gain[2] + b[3][c] * gain[3];
    }
private:
    double sr = 48000.0;
    juce::dsp::LinkwitzRileyFilter<float> x1, x2, x3, ap2a, ap3a, ap3b;
    float gain[4] { 1, 1, 1, 1 }, env[4] {}, envAll = 0.0f;
    const float envRel = 0.999f;
};

class LookaheadLimiter
{
public:
    static constexpr double lookaheadSeconds = 0.0015;

    // lookaheadOverride: when oversampled, a whole multiple of the base-rate look-ahead (keeps latency an integer)
    void prepare (double sampleRate, int lookaheadOverride = 0)
    {
        sr = sampleRate;
        L = lookaheadOverride > 0 ? lookaheadOverride : juce::jmax (1, (int) std::round (lookaheadSeconds * sr));
        win = L + 1;
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 62
        D = tpLen / 2;   // V62: peaks are judged 8 samples late (so true peak can look both sides); the audio waits for it
       #endif
        targets.assign ((size_t) win, 1.0f);
        smooth.assign ((size_t) win, 1.0f);
        for (auto& ch : own) { ch.targets.assign ((size_t) win, 1.0f); ch.smooth.assign ((size_t) win, 1.0f); }
        for (auto& d : delay) d.assign ((size_t) (L + D), 0.0f);
        for (auto& d : dryDelay) d.assign ((size_t) (L + D), 0.0f);
        multi.prepare (sr);
        multiDry.phaseOnly = true;  multiDry.prepare (sr);
        bands4.prepare (sr);
        bands4Dry.phaseOnly = true; bands4Dry.prepare (sr);
        for (auto& bp : bandPath) { bp.targets.assign ((size_t) (L + D + 1), 1.0f); bp.smooth.assign ((size_t) (L + D + 1), 1.0f); }   // covers the whole delay
        for (auto& ch : bandDelay) for (auto& d : ch) d.assign ((size_t) (L + D), 0.0f);
        auto lp = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sr, 150.0, 0.7071);
        bassLp.coefficients = new juce::dsp::IIR::Coefficients<float> (lp);
        reset();
    }

    void reset()
    {
        std::fill (targets.begin(), targets.end(), 1.0f);
        std::fill (smooth.begin(), smooth.end(), 1.0f);
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        for (auto& d : dryDelay) std::fill (d.begin(), d.end(), 0.0f);
        sum = (double) win;
        held = 1.0f;
        avgGrDb = 0.0f;
        wIdx = dIdx = 0;
        resetOwn();
        for (auto& h : tpHist) h.fill (0.0f);
        multi.reset();
        multiDry.reset();
        bands4.reset(); bands4Dry.reset(); bassLp.reset();
        resetBands();
        tFast = tSlow = 0.0f; lowE = totE = 0.0; sumShort = 0.0;
        upEnv = 0.0f; upRefDb = -100.0f; upTargetDb = upGainDb = 0.0f;
    }

    int getLatencySamples() const { return zeroLatency ? 0 : L + D; }
    int getLookaheadSamples() const { return L + D; }
    bool zeroLatency = false;   // tracking mode: no look-ahead (instant attack + hard ceiling)

    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int n = buffer.getNumSamples();
        // Maximizer style: pulling the threshold down raises the level by the same amount
        const float drive   = s.limOn ? juce::Decibels::decibelsToGain (-s.limThreshDb) : 1.0f;
        const float ceiling = juce::Decibels::decibelsToGain (s.limCeilingDb);
        const bool  maxEngine = s.limOn && s.limMax;   // V69
        const int   style = juce::jlimit (0, 8, s.limMaxStyle);
        const bool  multiband = s.limOn && ((! maxEngine && s.limMode == LimMode::Multi) || (maxEngine && style == 6));
        const bool  fourBand = maxEngine && style == 7;
        const float avgCoef = msToCoef (400.0f, sr);
        float relStep = 1.0f - msToCoef (s.limReleaseMs, sr);
        float minGain = 1.0f;
        // V69 Max: character scales every time constant (0 = 4x faster, 5 = as designed, 10 = 4x slower)
        const float charMul = std::pow (2.0f, (juce::jlimit (0.0f, 10.0f, s.limCharacter) - 5.0f) / 2.5f);
        multi.headroomDb = maxEngine ? 0.5f : 3.0f;   multi.weightScale = maxEngine ? 0.6f : 1.0f;
        // the attack ramp: the full look-ahead, or shorter for the styles that keep transients sharp
        const int ws = ! maxEngine ? win : (style == 1 || style == 5) ? juce::jmax (2, win / 3) : style == 4 ? juce::jmax (2, win / 2) : win;
        wsNow = ws;   // (re-summed every block, like the long window, so it can't drift)
        sumShort = 0.0;
        for (int k = 0; k < ws; ++k) sumShort += smooth[(size_t) ((wIdx - 1 - k + 2 * win) % win)];
        const float tAtt = 1.0f - msToCoef (1.0f, sr), tRelF = 1.0f - msToCoef (25.0f, sr);
        const float tAttS = 1.0f - msToCoef (15.0f, sr), tRelS = 1.0f - msToCoef (150.0f, sr);
        upEnvCoef = 1.0f - msToCoef (60.0f, sr);                  // V70: upward compression timing
        upRise = 1.0f - msToCoef (350.0f, sr);                    // quiet part arrives: come up slowly (no breathing)
        upFall = 1.0f - msToCoef (25.0f, sr);                     // loud part arrives: get out of the way fast
        upRefFall = (float) (8.0 * 1.0 / sr);                     // the loud reference sinks 1 dB a second
        const float shareCoef = 1.0f - msToCoef (300.0f, sr);

        // Re-sum once per block so floating-point drift can never build up
        sum = 0.0;
        for (auto v : smooth) sum += v;
        // V61 channel link: below 100% each side also runs its own gain path, and the two are blended (in dB)
        const float link = juce::jlimit (0.0f, 1.0f, s.limLink);
        const bool unlinked = numCh == 2 && link < 0.999f;
        if (fourBand && ! bandsActive) resetBands();   // V69: start the band paths clean
        bandsActive = fourBand;
        if (fourBand) for (auto& bp : bandPath) { bp.sum = 0.0; for (auto v : bp.smooth) bp.sum += v; }
        if (unlinked && ! ownActive) resetOwn();
        ownActive = unlinked;
        if (unlinked) for (auto& ch : own) { ch.sum = 0.0; for (auto v : ch.smooth) ch.sum += v; }

        for (int i = 0; i < n; ++i)
        {
            float in[2] {}, raw[2] {};
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c)
            {
                raw[c] = buffer.getSample (c, i);
                in[c] = raw[c] * drive;
            }
            if (s.limUpward > 0.01f && ! s.limDelta)   // V70 upward compression: quiet passages come up toward the loud ones
            {
                float lvl = 0.0f;
                for (int c = 0; c < numCh; ++c) lvl = juce::jmax (lvl, std::abs (raw[c]));
                upEnv += upEnvCoef * (lvl * lvl - upEnv);
                if ((i & 7) == 0)
                {
                    const float envDb = 10.0f * std::log10 (upEnv + 1.0e-12f);
                    upRefDb = envDb > upRefDb ? envDb : upRefDb - upRefFall;                 // the loud level, let go slowly
                    const float gap = upRefDb - envDb;                                        // how far under the loud parts this is
                    float lift = juce::jlimit (0.0f, s.limUpward, (gap - 6.0f) * 0.6f);       // the top 6 dB is left alone
                    lift *= juce::jlimit (0.0f, 1.0f, (envDb + 70.0f) / 15.0f);               // never lift silence or hiss
                    upTargetDb = lift;
                }
                upGainDb += (upTargetDb > upGainDb ? upRise : upFall) * (upTargetDb - upGainDb);
                const float ug = juce::Decibels::decibelsToGain (upGainDb);
                for (int c = 0; c < numCh; ++c) in[c] *= ug;
            }
            else upGainDb = 0.0f;

            if (maxEngine)
            {
                // transient emphasis: lift the attack of hits before they're limited, so they survive it
                float mono = 0.0f;
                for (int c = 0; c < numCh; ++c) mono = juce::jmax (mono, std::abs (in[c]));
                tFast += (mono > tFast ? tAtt : tRelF) * (mono - tFast);
                tSlow += (mono > tSlow ? tAttS : tRelS) * (mono - tSlow);
                if (s.limTransients > 0.001f)
                {
                    const float lift = juce::jlimit (0.0f, 1.0f, tFast / juce::jmax (1.0e-6f, tSlow) - 1.0f);
                    const float tg = 1.0f + juce::jlimit (0.0f, 1.0f, s.limTransients) * lift;   // up to +6 dB on a hit
                    for (int c = 0; c < numCh; ++c) in[c] *= tg;
                }
                // how much of the energy is bass (Smart: bass-heavy music recovers slower, so it doesn't distort)
                const float lo = bassLp.processSample (0.5f * (in[0] + in[numCh > 1 ? 1 : 0]));
                lowE += shareCoef * ((double) lo * lo - lowE);
                totE += shareCoef * ((double) mono * mono - totE);
                if (style == 5)   // Clip: shave the very tops on a smooth curve, the limiter does the rest
                {
                    const float cl = ceiling * 1.12f, kn = ceiling * 0.95f;
                    for (int c = 0; c < numCh; ++c)
                    {
                        const float a = std::abs (in[c]);
                        // shaves up to about 5 dB off the tallest peaks, never more: past that it's the limiter's job
                        // (a clipper that flattens everything sounds broken, and rings over the ceiling when oversampled)
                        if (a > kn) in[c] = std::copysign (kn + (cl - kn) * std::tanh ((a - kn) / (cl - kn)) + juce::jmax (0.0f, a - 2.0f * kn), in[c]);
                    }
                }
                if (fourBand)   // V69 Bands: per-band gains with the same look-ahead as the main limiter
                {
                    float b[4][2] {};
                    bands4.split (in, numCh, b);
                    float pk[4] {}, all = 0.0f, total = 0.0f;
                    for (int c = 0; c < numCh; ++c)
                    {
                        float sumc = 0.0f;
                        for (int k = 0; k < 4; ++k) { pk[k] = juce::jmax (pk[k], std::abs (b[k][c])); sumc += b[k][c]; }
                        all = juce::jmax (all, std::abs (sumc));
                    }
                    for (int k = 0; k < 4; ++k) total += pk[k];
                    // the bands carrying the most peak energy give the most (share squared), each down to -12 dB at most
                    float t[4] { 1.0f, 1.0f, 1.0f, 1.0f };
                    const float needed = all - ceiling;
                    if (needed > 0.0f && total > 1.0e-9f)
                    {
                        float w[4], wSum = 0.0f;
                        for (int k = 0; k < 4; ++k) { const float sh = pk[k] / total; w[k] = sh * sh; wSum += w[k]; }
                        for (int k = 0; k < 4; ++k)
                            if (pk[k] > 1.0e-9f) t[k] = juce::jlimit (0.25f, 1.0f, 1.0f - needed * (w[k] / wSum) / pk[k]);
                    }
                    static constexpr float bandRelMs[4] { 90.0f, 36.0f, 18.0f, 10.0f };   // lows slow (no distortion), highs quick
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& bp = bandPath[(size_t) k];
                        bp.targets[(size_t) bIdx] = t[k];
                        float m = 1.0f;
                        for (auto v : bp.targets) m = juce::jmin (m, v);
                        bp.held = juce::jmin (m, bp.held + (1.0f - bp.held) * (1.0f - msToCoef (bandRelMs[k] * charMul, sr)));
                        bp.sum += (double) bp.held - (double) bp.smooth[(size_t) bIdx];
                        bp.smooth[(size_t) bIdx] = bp.held;
                        bandGainNow[k] = (float) (bp.sum / (double) bp.smooth.size());
                    }
                    bIdx = (bIdx + 1) % (int) bandPath[0].targets.size();
                    {   // for the GR meter: the band gains, weighted by how much each band carries right now
                        float num = 0.0f;
                        for (int k = 0; k < 4; ++k) num += pk[k] * bandGainNow[k];
                        bandMeterGain = total > 1.0e-6f ? num / total : 1.0f;
                    }
                    // the main limiter now judges what the bands will leave (their targets), and the delayed bands
                    // are what gets played (see the output below)
                    for (int c = 0; c < numCh; ++c)
                    {
                        float pb = 0.0f;
                        for (int k = 0; k < 4; ++k) { pb += b[k][c] * t[k]; bandSamples[k][c] = b[k][c]; }
                        in[c] = pb;
                    }
                }
            }
            if (multiband)
            {
                multi.processSample (in, numCh, ceiling);
                multiDry.processSample (raw, numCh, ceiling);    // match the wet path's phase
            }

            float tpc[2] { std::abs (in[0]), std::abs (in[numCh > 1 ? 1 : 0]) };
            if (D == 0 || zeroLatency)
                for (int c = 0; c < numCh; ++c)
                    peak = juce::jmax (peak, std::abs (in[c]));
            else   // V62: judge the sample D behind (the audio is delayed D more to match); true peak also looks between samples
            {
                for (int c = 0; c < numCh; ++c)
                {
                    auto& h = tpHist[(size_t) c];
                    for (int k = 0; k < tpLen - 1; ++k) h[(size_t) k] = h[(size_t) k + 1];
                    h[(size_t) tpLen - 1] = in[c];
                    tpc[c] = std::abs (h[(size_t) tpLen / 2 - 1]);
                    peak = juce::jmax (peak, tpc[c]);
                }
            }
            if (s.limTp && D > 0 && ! zeroLatency)   // V62 true peak (8x, 16-tap interpolation): the converter in a phone or a
            {                                         // streaming encoder can't make an over the ceiling
                for (int c = 0; c < numCh; ++c)
                {
                    auto& h = tpHist[(size_t) c];
                    float m = juce::jmax (std::abs (h[(size_t) tpLen / 2 - 1]), std::abs (h[(size_t) tpLen / 2]));
                    for (int f = 0; f < tpFracs; ++f)
                    {
                        float v = 0.0f;
                        for (int k = 0; k < tpLen; ++k) v += tpTaps[(size_t) f][(size_t) k] * h[(size_t) k];
                        m = juce::jmax (m, std::abs (v));
                    }
                    tpc[c] = juce::jmax (tpc[c], m);
                    peak = juce::jmax (peak, m);
                }
            }

            // ARC: short, sparse peaks recover fast (loud and open); dense,
            // sustained limiting recovers slowly (no pumping or distortion).
            if (maxEngine && (i & 15) == 0)   // V69: each style's release
            {
                float relMs = 60.0f;
                const float bassShare = totE > 1.0e-12 ? (float) juce::jlimit (0.0, 1.0, lowE / totE) : 0.0f;
                switch (style)
                {
                    case 0:  relMs = (10.0f + avgGrDb * avgGrDb * 5.0f) * (1.0f + 1.5f * bassShare); break;   // Smart
                    case 1:  relMs = (5.0f + avgGrDb * avgGrDb * 4.0f) * (1.0f + 1.2f * bassShare); break;    // Transient
                    case 2:  relMs = 250.0f; break;                                                             // Pumping: free, long
                    case 4:  relMs = 8.0f; break;                                                               // Crisp: grit over pumping
                    case 5:  relMs = 15.0f; break;                                                              // Clip
                    case 8:  relMs = juce::jlimit (80.0f, 600.0f, 80.0f + avgGrDb * avgGrDb * 40.0f) * (1.0f + bassShare); break;   // V72 Safe: slow, smooth, never grabs
                    default: relMs = juce::jlimit (20.0f, 300.0f, 20.0f + avgGrDb * avgGrDb * 8.0f); break;    // Balanced, Spectral, Bands
                }
                relStep = 1.0f - msToCoef (juce::jlimit (2.0f, 1200.0f, relMs * charMul), sr);
            }
            else if (s.limArc && (i & 15) == 0)
                relStep = 1.0f - msToCoef (juce::jlimit (8.0f, 400.0f, 8.0f + avgGrDb * avgGrDb * 6.0f), sr);

            const float target = (s.limOn && peak > ceiling) ? ceiling / peak : 1.0f;
            targets[(size_t) wIdx] = target;

            float m = 1.0f;
            for (auto t : targets) m = juce::jmin (m, t);

            held = juce::jmin (m, held + (1.0f - held) * relStep);

            sum += (double) held - (double) smooth[(size_t) wIdx];
            sumShort += (double) held - (double) smooth[(size_t) ((wIdx - wsNow + win) % win)];
            smooth[(size_t) wIdx] = held;
            const float g = zeroLatency ? held : (wsNow < win ? (float) (sumShort / (double) wsNow) : (float) (sum / (double) win));
            float gc[2] { g, g };
            if (unlinked)
                for (int c = 0; c < 2; ++c)
                {
                    auto& o = own[(size_t) c];
                    const float pc = tpc[c];
                    o.targets[(size_t) wIdx] = (s.limOn && pc > ceiling) ? ceiling / pc : 1.0f;
                    float mc = 1.0f;
                    for (auto t : o.targets) mc = juce::jmin (mc, t);
                    o.held = juce::jmin (mc, o.held + (1.0f - o.held) * relStep);
                    o.sum += (double) o.held - (double) o.smooth[(size_t) wIdx];
                    o.smooth[(size_t) wIdx] = o.held;
                    const float gOwn = zeroLatency ? o.held : (float) (o.sum / (double) win);
                    // geometric blend: always between the linked gain and the side's own, so the ceiling still holds
                    gc[c] = g * std::pow (juce::jmax (1.0e-6f, gOwn / juce::jmax (1.0e-6f, g)), 1.0f - link);
                }
            minGain = juce::jmin (minGain, juce::jmin (gc[0], gc[1]) * (fourBand ? bandMeterGain : 1.0f));
            const float grNow = -juce::Decibels::gainToDecibels (g, -60.0f);
            avgGrDb = avgCoef * avgGrDb + (1.0f - avgCoef) * grNow;

            for (int c = 0; c < numCh; ++c)
            {
                auto& d = delay[(size_t) c];
                float delayed = d[(size_t) dIdx];
                d[(size_t) dIdx] = in[c];
                if (fourBand)   // V69 Bands: play the delayed bands with their own gains
                {
                    float y = 0.0f, full = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& bd = bandDelay[(size_t) k][(size_t) c];
                        y += bd[(size_t) dIdx] * bandGainNow[k];
                        full += bd[(size_t) dIdx];
                        bd[(size_t) dIdx] = bandSamples[k][c];
                    }
                    delayed = y;
                    bandFull[c] = full;
                }
                auto& dd = dryDelay[(size_t) c];
                float dry = dd[(size_t) dIdx];   // untouched input, aligned with the look-ahead
                dd[(size_t) dIdx] = raw[c];
                if (zeroLatency) { delayed = in[c]; dry = raw[c]; }   // tracking mode: act on this very sample

                float out = delayed * gc[c];
                if (s.limOn && s.limDelta)   // V65: hear what the limiter takes off (back at the input's level)
                    out = ((fourBand && ! zeroLatency ? bandFull[c] : delayed) - delayed * gc[c]) / juce::jmax (1.0e-3f, drive);
                else if (s.limOn)
                {
                    if (s.limMix >= 0.999f) out = juce::jlimit (-ceiling, ceiling, out);   // safety (ceiling guaranteed)
                    else                    out = dry + s.limMix * (out - dry);           // Mix: parallel limiting
                }
                buffer.setSample (c, i, out);
            }

            wIdx = (wIdx + 1) % win;
            dIdx = (dIdx + 1) % (L + D);
        }

        return -juce::Decibels::gainToDecibels (minGain, -60.0f);
    }

private:
    double sr = 48000.0;
    int L = 72, win = 73, wIdx = 0, dIdx = 0, D = 0;
    std::vector<float> targets, smooth;
    std::array<std::vector<float>, 2> delay, dryDelay;
    double sum = 0.0;
    float held = 1.0f, avgGrDb = 0.0f;
    MultibandPreLimiter multi, multiDry;

    // V62 true peak: the last 32 samples per channel and Kaiser-windowed sinc taps for 7 points between samples 15
    // and 16 (accurate to about 0.3 dB right up to 20 kHz; the audio is delayed D = 16 samples to match)
    static constexpr int tpLen = 32, tpFracs = 7;
    std::array<std::array<float, tpLen>, 2> tpHist {};
    const std::array<std::array<float, tpLen>, tpFracs> tpTaps = []
    {
        std::array<std::array<float, tpLen>, tpFracs> t {};
        auto i0 = [] (double x) { double sum = 1.0, term = 1.0; for (int k = 1; k < 30; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; } return sum; };
        const double beta = 7.0;
        for (int f = 0; f < tpFracs; ++f)
        {
            const double frac = (f + 1) / 8.0;
            double sum = 0.0;
            for (int k = 0; k < tpLen; ++k)
            {
                const double x = (double) k - (tpLen / 2 - 1 + frac);
                const double pix = juce::MathConstants<double>::pi * x;
                const double sinc = std::abs (x) < 1.0e-9 ? 1.0 : std::sin (pix) / pix;
                const double r = x / (tpLen / 2 + 0.5);
                const double w = i0 (beta * std::sqrt (juce::jmax (0.0, 1.0 - r * r))) / i0 (beta);
                t[(size_t) f][(size_t) k] = (float) (sinc * w);
                sum += sinc * w;
            }
            for (auto& v : t[(size_t) f]) v = (float) (v / sum);
        }
        return t;
    }();

    // V69 Max engine state
    FourBandPreLimiter bands4, bands4Dry;
    struct BandPath { std::vector<float> targets, smooth; double sum = 0.0; float held = 1.0f; };
    std::array<BandPath, 4> bandPath;
    std::array<std::array<std::vector<float>, 2>, 4> bandDelay;
    float bandMeterGain = 1.0f, bandFull[2] {};
    float upEnv = 0.0f, upRefDb = -100.0f, upTargetDb = 0.0f, upGainDb = 0.0f, upEnvCoef = 0.01f, upRise = 0.001f, upFall = 0.01f, upRefFall = 0.0005f;   // V70
    float bandGainNow[4] { 1, 1, 1, 1 }, bandSamples[4][2] {};
    int bIdx = 0;
    bool bandsActive = false;
    void resetBands()
    {
        for (auto& bp : bandPath)
        {
            std::fill (bp.targets.begin(), bp.targets.end(), 1.0f);
            std::fill (bp.smooth.begin(), bp.smooth.end(), 1.0f);
            bp.sum = (double) bp.smooth.size(); bp.held = 1.0f;
        }
        for (auto& ch : bandDelay) for (auto& d : ch) std::fill (d.begin(), d.end(), 0.0f);
        for (auto& g : bandGainNow) g = 1.0f;
        bIdx = 0;
    }
    juce::dsp::IIR::Filter<float> bassLp;
    float tFast = 0.0f, tSlow = 0.0f;
    double lowE = 0.0, totE = 0.0, sumShort = 0.0;
    int wsNow = 0;

    // V61: each side's own gain path (only runs while the link is below 100%)
    struct OwnPath { std::vector<float> targets, smooth; double sum = 0.0; float held = 1.0f; };
    std::array<OwnPath, 2> own;
    bool ownActive = false;
    void resetOwn()
    {
        for (auto& o : own)
        {
            std::fill (o.targets.begin(), o.targets.end(), 1.0f);
            std::fill (o.smooth.begin(), o.smooth.end(), 1.0f);
            o.sum = (double) win; o.held = 1.0f;
        }
    }
};

//==============================================================================
// Final stage: Lavry-style soft saturation, optionally oversampled (2x / 4x) so
// its rounded-off peaks don't alias, then the look-ahead brickwall limiter at the
// normal rate. The limiter's gain moves are smooth (look-ahead + averaging), so it
// barely aliases on its own, and running it after the down-sampling filter means
// any filter overshoot is caught too: the ceiling stays guaranteed with no
// hard clipping of the oversampled signal.
class LimiterStage
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        lim.prepare (sampleRate);
        lat[0] = 0;
        for (int k = 0; k < 2; ++k)
        {
            os[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (
                2, (size_t) (k + 1), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
            os[(size_t) k]->initProcessing ((size_t) juce::jmax (32, maxBlockSize));
            lat[(size_t) k + 1] = (int) std::round (os[(size_t) k]->getLatencyInSamples());
        }
        reset();
    }

    void reset()
    {
        lim.reset();
        for (auto& o : os) if (o) o->reset();
    }

    int getLatencySamples() const { return zeroLatency ? 0 : lim.getLookaheadSamples() + lat[(size_t) osIdx]; }
    int latencyFor (int osSetting) const { return lim.getLookaheadSamples() + lat[(size_t) juce::jlimit (0, 2, osSetting)]; }
    bool zeroLatency = false;

    // returns the larger of the soft-saturation and limiter gain reduction (dB)
    float process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int want = zeroLatency ? 0 : juce::jlimit (0, 2, s.limOs);
        if (want != osIdx) { osIdx = want; reset(); }
        lim.zeroLatency = zeroLatency;

        // soft saturation runs inside the oversampler (always, so the latency never changes with the soft setting)
        float softDb = 0.0f;
        const float clipLevel = juce::Decibels::decibelsToGain (s.limCeilingDb + s.limThreshDb);   // where the limiter starts
        if (osIdx == 0)
        {
            softDb = s.limOn ? SoftSaturation::process (buffer, s.limSoft) : 0.0f;
            if (s.limOn) softDb = juce::jmax (softDb, Clipper::process (buffer, s.limClip, clipLevel));
        }
        else
        {
            const int nc = juce::jmin (2, buffer.getNumChannels());
            juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nc, (size_t) buffer.getNumSamples());
            auto& o = os[(size_t) osIdx - 1];
            auto up = o->processSamplesUp (block);
            float* ptrs[2] { up.getChannelPointer (0), up.getChannelPointer ((size_t) (nc - 1)) };
            juce::AudioBuffer<float> hi (ptrs, nc, (int) up.getNumSamples());   // refers to the oversampled data, no allocation
            softDb = s.limOn ? SoftSaturation::process (hi, s.limSoft) : 0.0f;
            if (s.limOn) softDb = juce::jmax (softDb, Clipper::process (hi, s.limClip, clipLevel));
            o->processSamplesDown (block);
        }
        return juce::jmax (softDb, lim.process (buffer, s));
    }

private:
    LookaheadLimiter lim;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> os;
    std::array<int, 3> lat {};   // oversampler latency at 1x, 2x, 4x
    int osIdx = 0;
};

//==============================================================================
struct Meters
{
    std::atomic<float> inDb { -100.0f }, outDb { -100.0f };
    // Left/right peaks for the edge meters: linear, the highest since the UI last read them (the UI swaps in 0)
    std::atomic<float> inPeak[2] { 0.0f, 0.0f }, outPeak[2] { 0.0f, 0.0f };
    static void holdMax (std::atomic<float>& a, float v)
    {
        float cur = a.load();
        while (v > cur && ! a.compare_exchange_weak (cur, v)) {}
    }
    static void storePeaks (std::atomic<float> (&dst)[2], const juce::AudioBuffer<float>& b)
    {
        const int nc = b.getNumChannels();
        if (nc <= 0) return;
        for (int c = 0; c < 2; ++c)
            holdMax (dst[c], b.getMagnitude (juce::jmin (c, nc - 1), 0, b.getNumSamples()));   // mono feeds both
    }
    std::atomic<float> gateGr { 0.0f }, compGr { 0.0f }, dsGr { 0.0f }, limGr { 0.0f };
    std::atomic<float> correlation { 1.0f };
    std::atomic<float> rawEnergy { 0.0f }, rawPeak { 0.0f };   // V66: raw input since the UI last read (it swaps in 0)
    std::atomic<int> rawSamples { 0 };   // V63: output phase correlation, -1 (out of phase) .. +1 (mono)
    std::atomic<float> gateInDb { -120.0f };   // what the gate hears (dBFS)
    std::atomic<float> flowGr { 0.0f };
    std::atomic<float> peakGr { 0.0f };       // input peak catcher, dB
    std::atomic<float> tapeVuDb { -100.0f };  // tape output level (RMS dBFS) for its VU meter
    std::atomic<float> riderDb { 0.0f };      // vocal rider's current ride (dB, +/-)
    std::atomic<float> breathGr { 0.0f };     // breath control reduction (dB)
};

class VocalChain
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        peakCatcher.prepare (sampleRate);
        reso.prepare (sampleRate);
        cleanup.prepare (sampleRate, maxBlockSize);
        for (auto& e : eqs) e.prepare (sampleRate, maxBlockSize);
        gate.prepare (sampleRate);
        breath.prepare (sampleRate);
        rider.prepare (sampleRate);
        for (auto& c : comps) c.prepare (sampleRate);
        for (auto& c : mcomps) c.prepare (sampleRate);   // V75
        deEsser.prepare (sampleRate);
        flow.prepare (sampleRate);
        perach.prepare (sampleRate);
        esh.prepare (sampleRate);
        tape.prepare (sampleRate, maxBlockSize);
        limiter.prepare (sampleRate, maxBlockSize);
        inGain.reset (sampleRate, 0.02);
        outGain.reset (sampleRate, 0.02);
        inGain.setCurrentAndTargetValue (1.0f);
        outGain.setCurrentAndTargetValue (1.0f);

        // band solo: everything allocated here, never on the audio thread
        soloSr = sampleRate;
        soloCoeffs = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sampleRate, 1000.0, 1.0));
        for (auto& f : soloFilter) { f.coefficients = soloCoeffs; f.reset(); }
        soloBuf.setSize (2, juce::jmax (1, maxBlockSize));
        soloDelay.setSize (2, perach.getLatencySamples() + tape.latencyFor (2) + limiter.latencyFor (2) + 1);   // long enough for any oversampling
        soloDelay.clear();
        soloPos = 0;
        soloWasOn = false;

        // V60 tape M/S drive: the side input and the drive amount, delayed to line up with the tape's output
        msLen = tape.latencyFor (2) + juce::jmax (1, maxBlockSize) + 1;
        msSideIn.assign ((size_t) msLen, 0.0f);
        msAmtLine.assign ((size_t) msLen, 0.0f);
        msMidIn.assign ((size_t) msLen, 0.0f);
        eInM = eInS = eOutM = eOutS = 0.0;
        msEnvCoef = 1.0f - std::exp (-1.0f / (0.3f * (float) sampleRate));
        msPos = 0; msAmt = 0.0f;
        msCoef = 1.0f - std::exp (-1.0f / (0.05f * (float) sampleRate));
        msLow.coefficients = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, 150.0, 0.7071));
        msLow.reset();
        widthLow.coefficients = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, 120.0, 0.7071));
        widthLow.reset();
        widthNow = 1.0f;
        compPre.setSize (2, juce::jmax (1, maxBlockSize));
        dPre = dPost = 0.0;
        dCoef = 1.0 - std::exp (-1.0 / (0.3 * sampleRate));
        widthCoef = 1.0f - std::exp (-1.0f / (0.05f * (float) sampleRate));
        corrCoef = 1.0f - std::exp (-1.0f / (0.3f * (float) sampleRate));
        corrLR = corrLL = corrRR = 0.0;
    }

    void reset()
    {
        peakCatcher.reset(); reso.reset(); cleanup.reset(); for (auto& e : eqs) e.reset(); gate.reset(); breath.reset(); rider.reset(); for (auto& c : comps) c.reset(); for (auto& c : mcomps) c.reset(); deEsser.reset(); flow.reset(); perach.reset(); esh.reset(); tape.reset(); limiter.reset();
    }

    int getLatencySamples() const { return latencyFor (trackMode, tapeOsNow, limOsNow, cleanPhaseNow, cleanOsNow); }
    // latency in normal mode, or in low-latency tracking mode (resonance suppressor paused, no look-aheads)
    int latencyFor (bool track) const { return latencyFor (track, tapeOsNow, limOsNow); }
    int latencyFor (bool track, int tapeOs, int limOs, int cleanPhase = -1, int cleanOs = -1, int flowModel = -1) const
    {
        if (track) return 0;
        if (cleanPhase < 0) { cleanPhase = cleanPhaseNow; cleanOs = cleanOsNow; }
        if (flowModel < 0) flowModel = flowModelNow;
        return peakCatcher.getLookaheadSamples() + reso.getLatencySamples() + cleanup.latencyFor (cleanPhase, cleanOs)
             + (flowModel == 1 ? perach.getLatencySamples() : 0)   // Perach (selected in place of Zerem): one analysis frame
             + tape.latencyFor (tapeOs) + limiter.latencyFor (limOs);
    }
    int latencyFor (const ChainSettings& s) const { return latencyFor (s.trackMode, s.tapeOs, s.limOs, s.cleanPhase, s.cleanOs, s.flowModel); }
    int maxLatency() const { return juce::jmax (latencyFor (false, 2, 2, 2, 0, 1), latencyFor (false, 2, 2, 0, 2, 1)); }
    bool isTrackMode() const { return trackMode; }

    void process (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int n = buffer.getNumSamples();
        if (n == 0) return;

        {   // V66: the raw input (before the Input knob) for Auto Input: energy and peak, collected until the UI reads them
            double e = 0.0; float pk = 0.0f;
            const int nc = juce::jmin (2, buffer.getNumChannels());
            for (int c = 0; c < nc; ++c)
            {
                const auto* x = buffer.getReadPointer (c);
                for (int i = 0; i < n; ++i) { e += (double) x[i] * x[i]; pk = juce::jmax (pk, std::abs (x[i])); }
            }
            meters.rawEnergy.store (meters.rawEnergy.load() + (float) (e / juce::jmax (1, nc)));
            meters.rawSamples.store (meters.rawSamples.load() + n);
            Meters::holdMax (meters.rawPeak, pk);
        }
        inGain.setTargetValue (juce::Decibels::decibelsToGain (s.inputDb));
        inGain.applyGain (buffer, n);
        meters.inDb.store (juce::Decibels::gainToDecibels (peakOf (buffer), -100.0f));
        Meters::storePeaks (meters.inPeak, buffer);

        // Tracking mode: every stage switches to its zero-latency version
        if (s.trackMode != trackMode)
        {
            trackMode = s.trackMode;
            peakCatcher.zeroLatency = tape.zeroLatency = limiter.zeroLatency = cleanup.zeroLatency = trackMode;
            peakCatcher.reset(); tape.reset(); limiter.reset(); reso.reset(); cleanup.reset();
        }

        tapeOsNow = juce::jlimit (0, 2, s.tapeOs);
        limOsNow  = juce::jlimit (0, 2, s.limOs);
        cleanPhaseNow = juce::jlimit (0, 2, s.cleanPhase);
        cleanOsNow    = juce::jlimit (0, 2, s.cleanOs);

        // Spectra-style peak catcher: grabs the fastest peaks before anything else sees them
        {
            // its own input and output (only while it is switched in); gains ramp across the block so moves never click
            const float inDb  = s.peakCatch ? s.peakInDb : 0.0f;
            const float outDb = s.peakCatch ? (s.peakLink ? s.peakOutDb - s.peakInDb : s.peakOutDb) : 0.0f;
            const float gIn = juce::Decibels::decibelsToGain (inDb), gOut = juce::Decibels::decibelsToGain (outDb);
            auto ramp = [&buffer] (float& last, float target)
            {
                if (std::abs (last - target) > 1.0e-6f || std::abs (target - 1.0f) > 1.0e-6f)
                    for (int c = 0; c < buffer.getNumChannels(); ++c)
                        buffer.applyGainRamp (c, 0, buffer.getNumSamples(), last, target);
                last = target;
            };
            ramp (peakInGain, gIn);
            Meters::holdMax (meters.peakGr, peakCatcher.process (buffer, s.peakCatch));   // UI swaps in 0 when it reads
            ramp (peakOutGain, gOut);
        }

        preTap.push (buffer);
        if (! s.trackMode)
            reso.process (buffer, s.reso);   // always runs (passes through when off) so latency stays fixed
        cleanup.msEnabled = s.msEnabled;
        cleanup.process (buffer, s.clean, s.cleanOn, s.cleanPhase, s.cleanOs);   // always runs: keeps its latency constant
        postTap.push (buffer);

        // Band solo: listen to just the region around one clean-up band (Pro-Q style).
        // The rest of the chain keeps running underneath so un-soloing is seamless.
        const int sb = soloBand.load();
        const bool soloOn = sb >= 0 && sb < numCleanBands && s.clean[(size_t) sb].on && soloBuf.getNumSamples() >= n;
        if (soloOn)
        {
            const auto& b = s.clean[(size_t) sb];
            const bool narrow = b.type == BandType::Bell || b.type == BandType::Notch;
            const float q = narrow ? juce::jlimit (0.5f, 8.0f, b.q) : 0.7f;
            *soloCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (soloSr, juce::jlimit (20.0f, 20000.0f, b.freq), q);
            if (! soloWasOn) for (auto& f : soloFilter) f.reset();
            const int nc = juce::jmin (2, buffer.getNumChannels());
            for (int c = 0; c < nc; ++c)
            {
                soloBuf.copyFrom (c, 0, buffer, c, 0, n);
                auto* d = soloBuf.getWritePointer (c);
                for (int i = 0; i < n; ++i) d[i] = soloFilter[(size_t) c].processSample (d[i]);
            }
        }
        soloWasOn = soloOn;

        // Character EQs, in the user's order
        auto runCharEqs = [&]
        {
            for (int t : s.eqOrder)
            {
                const bool active = s.eqOn && s.charEq[(size_t) t].on;
                auto& e = eqs[(size_t) t];
                if (active && ! eqWasOn[(size_t) t]) e.reset();   // start clean when switched in
                eqWasOn[(size_t) t] = active;
                if (active) { e.update (s.charEq[(size_t) t], (EqType) t); e.place = s.msEnabled ? s.charEq[(size_t) t].place : 0; e.process (buffer); }
            }
            charTap.push (buffer);   // V43: the Character EQ screen's live spectrum (after the EQs)
        };

        // Gate, then the active compressors in the user's order
        auto runDynamics = [&]
        {
            meters.gateGr.store (gate.process (buffer, s));
            meters.gateInDb.store (gate.getLevelDb());
            // breaths turned down, then the rider evens out the phrases before the compressors
            meters.breathGr.store (s.compOn ? breath.process (buffer, s.breathDb) : 0.0f);
            meters.riderDb.store  (s.compOn ? rider.process (buffer, s.riderRangeDb) : 0.0f);
            static constexpr float scHz[4] { 0.0f, 80.0f, 120.0f, 160.0f };
            const float scHpHz = scHz[juce::jlimit (0, 3, s.compScHpf)];

            const bool delta = s.compDelta && s.compOn && compPre.getNumSamples() >= n;   // V65
            if (delta) for (int c = 0; c < juce::jmin (2, buffer.getNumChannels()); ++c) compPre.copyFrom (c, 0, buffer, c, 0, n);
            float totalGr = 0.0f;   // the meter shows the combined gain reduction
            for (int ui : s.compOrder)
            {
                const auto u = (size_t) ui;
                const bool active = s.compOn && s.comps[u].on;
                const bool master = u >= 4;   // V75: the bus / mastering units
                if (active && ! compWasOn[u]) { if (master) mcomps[u - 4].reset(); else comps[u].reset(); }
                compWasOn[u] = active;
                auto unit = s.comps[u];
                unit.scHpHz = scHpHz;
                float gr = 0.0f;
                if (master)
                {
                    mcomps[u - 4].key = s.compScExt ? keyFor (n) : nullptr;
                    gr = active ? mcomps[u - 4].process (buffer, unit) : 0.0f;
                }
                else
                {
                comps[u].key = s.compScExt ? keyFor (n) : nullptr;   // V64
                gr = active ? comps[u].process (buffer, unit) : 0.0f;
                }
                compUnitGr[u].store (gr);
                totalGr += gr;
            }
            meters.compGr.store (totalGr);
            if (delta)   // V65: what the compressors changed = the input minus the output, with the make-up taken back out
            {
                const int nc = juce::jmin (2, buffer.getNumChannels());
                for (int i = 0; i < n; ++i)
                {
                    float a = 0.0f, b = 0.0f;
                    for (int c = 0; c < nc; ++c) { a += compPre.getSample (c, i); b += buffer.getSample (c, i); }
                    dPre  += dCoef * ((double) a * a - dPre);
                    dPost += dCoef * ((double) b * b - dPost);
                    const float k = dPost > 1.0e-12 ? (float) juce::jlimit (0.05, 20.0, std::sqrt (dPre / dPost)) : 1.0f;
                    for (int c = 0; c < nc; ++c) buffer.setSample (c, i, compPre.getSample (c, i) - buffer.getSample (c, i) * k);
                }
            }
        };

        auto runDeEss = [&]
        {
            dsTap.push (buffer);   // feeds the spectrum meter in the de-esser's Freq slider
            deEsser.key = s.dsScExt ? keyFor (n) : nullptr;   // V64
            meters.dsGr.store   (s.dsOn   ? deEsser.process (buffer, s) : 0.0f);
        };
        auto runTone = [&]
        {
            flowModelNow = s.trackMode ? 0 : juce::jlimit (0, 1, s.flowModel);
            if (flowModelNow == 1)
            {
                // Perach replaces Zerem. It always runs while selected (passing through when off) so latency stays fixed.
                if (flowModelWas != 1) perach.reset();
                auto ps = s.perach;
                ps.on = s.flowOn;
                meters.flowGr.store (perach.process (buffer, ps));
            }
            else
            {
                if (s.flowOn != flowWasOn || flowModelWas != 0) { flow.reset(); flowWasOn = s.flowOn; }
                meters.flowGr.store (s.flowOn ? flow.process (buffer, s) : 0.0f);
            }
            flowModelWas = flowModelNow;
        };
        auto runTape = [&]
        {
            // V60 tape M/S drive: tilt mid vs side into the tape, take it back out after (level stays put),
            // and keep the tape's low-end change off the sides so the bass stays centred.
            const bool tapeMs = s.msEnabled && buffer.getNumChannels() >= 2;
            if (tapeMs) msTapePre (buffer, s);
            if (s.tapeModel == 1)
            {
                // Esh in place of the tape machine. The tape stage still runs (passing through) so latency never changes.
                auto pass = s;
                pass.tapeOn = false;
                tape.process (buffer, pass);
                auto es = s.esh;
                es.on = s.tapeOn;
                meters.tapeVuDb.store (esh.process (buffer, es));
            }
            else
                meters.tapeVuDb.store (tape.process (buffer, s));   // always runs: keeps latency constant
            if (tapeMs) msTapePost (buffer);
        };
        // V68: the middle of the chain in the user's order (0 Char EQ, 1 Dynamics, 2 De-Ess, 3 Tone, 4 Tape);
        // the standard order follows the Char EQ pre / post switch
        if (s.midOrderCustom)
        {
            for (int m : s.midOrder)
                switch (m)
                {
                    case 0: runCharEqs(); break;
                    case 1: runDynamics(); break;
                    case 2: runDeEss(); break;
                    case 3: runTone(); break;
                    default: runTape(); break;
                }
        }
        else
        {
            if (s.eqAfterComp) { runDynamics(); runCharEqs(); }
            else               { runCharEqs();  runDynamics(); }
            runDeEss();
            runTone();
            runTape();
        }
        if (buffer.getNumChannels() >= 2) applyWidth (buffer, s.width);   // V63: before the limiter, so it catches any new peaks
        // Lavry-style soft saturation rounds the peaks first, then the brickwall catches anything left
        // (both inside the limiter stage, optionally oversampled). Always runs: keeps latency constant.
        meters.limGr.store (limiter.process (buffer, s));

        outGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));
        outGain.applyGain (buffer, n);
        if (buffer.getNumChannels() >= 2) measureCorrelation (buffer);   // V63: the phase meter

        // Band solo replaces the output, delayed by the tape + limiter latency so it stays in time
        {
            const int len = soloDelay.getNumSamples();
            const int d = juce::jlimit (0, len - 1, (flowModelNow == 1 ? perach.getLatencySamples() : 0) + tape.getLatencySamples() + limiter.getLatencySamples());
            const int nc = juce::jmin (2, buffer.getNumChannels());
            for (int c = 0; c < nc; ++c)
            {
                auto* line = soloDelay.getWritePointer (c);
                auto* out = buffer.getWritePointer (c);
                const float* in = soloBuf.getReadPointer (c);
                int pos = soloPos;
                for (int i = 0; i < n; ++i)
                {
                    line[pos] = soloOn ? in[i] : 0.0f;
                    int rd = pos - d; if (rd < 0) rd += len;
                    if (soloOn) out[i] = line[rd];
                    if (++pos >= len) pos = 0;
                }
            }
            soloPos = (soloPos + n) % len;
        }
        meters.outDb.store (juce::Decibels::gainToDecibels (peakOf (buffer), -100.0f));
        Meters::storePeaks (meters.outPeak, buffer);
    }

    Meters meters;
    void setSidechain (const juce::AudioBuffer<float>* b) { sidechain = b; }   // V64
    PeakCatcher peakCatcher;
    float peakInGain = 1.0f, peakOutGain = 1.0f;   // last applied peak-catcher in / out gains (for click-free ramps)
    TapeStage tape;

    // V65: the compressors' input (for listening to what they change) and the level match
    juce::AudioBuffer<float> compPre;
    double dPre = 0.0, dPost = 0.0, dCoef = 0.0001;

    // V64: the external sidechain for this block (set by the processor; null when the DAW sends none)
    const juce::AudioBuffer<float>* sidechain = nullptr;
    const juce::AudioBuffer<float>* keyFor (int n) const
    {
        return sidechain != nullptr && sidechain->getNumChannels() > 0 && sidechain->getNumSamples() >= n ? sidechain : nullptr;
    }

    // V63 width + phase meter ----------------------------------------------------------------------
    juce::dsp::IIR::Filter<float> widthLow;
    float widthNow = 1.0f, widthCoef = 0.001f;
    double corrLR = 0.0, corrLL = 0.0, corrRR = 0.0;
    float corrCoef = 0.0001f;

    void applyWidth (juce::AudioBuffer<float>& buffer, float target)
    {
        target = juce::jlimit (0.0f, 2.0f, target);
        if (std::abs (target - 1.0f) < 1.0e-4f && std::abs (widthNow - 1.0f) < 1.0e-4f) { widthNow = 1.0f; return; }
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            widthNow += (target - widthNow) * widthCoef;
            const float m = 0.5f * (l[i] + r[i]), sd = 0.5f * (l[i] - r[i]);
            const float lo = widthLow.processSample (sd);   // the side's lows
            // narrowing narrows everything; widening only widens above ~120 Hz, so the bass stays in the middle
            const float s2 = lo * juce::jmin (widthNow, 1.0f) + (sd - lo) * widthNow;
            l[i] = m + s2;
            r[i] = m - s2;
        }
    }

    void measureCorrelation (const juce::AudioBuffer<float>& buffer)
    {
        const auto* l = buffer.getReadPointer (0);
        const auto* r = buffer.getReadPointer (1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            corrLR += corrCoef * ((double) l[i] * r[i] - corrLR);
            corrLL += corrCoef * ((double) l[i] * l[i] - corrLL);
            corrRR += corrCoef * ((double) r[i] * r[i] - corrRR);
        }
        const double den = std::sqrt (corrLL * corrRR);
        meters.correlation.store (den > 1.0e-9 ? (float) juce::jlimit (-1.0, 1.0, corrLR / den) : 1.0f);
    }

    // V60 tape M/S drive ----------------------------------------------------------------------------
    std::vector<float> msSideIn, msMidIn, msAmtLine;
    double eInM = 0, eInS = 0, eOutM = 0, eOutS = 0;   // slow energy followers: keep the mid/side balance as it went in
    float msEnvCoef = 0.001f;
    int msLen = 1, msPos = 0, msBlockStart = 0;
    float msAmt = 0.0f, msCoef = 0.01f;
    juce::dsp::IIR::Filter<float> msLow;
    static constexpr float msLn2 = 0.69314718f;   // 6 dB: at full drive the favoured half goes in 6 dB hotter, the other 6 dB cooler

    void msTapePre (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        const int n = buffer.getNumSamples();
        const float target = s.tapeOn ? juce::jlimit (-1.0f, 1.0f, s.tapeMsDrive) : 0.0f;
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        msBlockStart = msPos;
        for (int i = 0; i < n; ++i)
        {
            msAmt += (target - msAmt) * msCoef;
            const float a = std::abs (msAmt) < 1.0e-6f ? 0.0f : msAmt;
            const float m = 0.5f * (l[i] + r[i]), sd = 0.5f * (l[i] - r[i]);
            msSideIn[(size_t) msPos] = sd;
            msMidIn[(size_t) msPos] = m;
            msAmtLine[(size_t) msPos] = a;
            if (a != 0.0f)
            {
                const float gS = std::exp (msLn2 * a), gM = 1.0f / gS;
                l[i] = m * gM + sd * gS;
                r[i] = m * gM - sd * gS;
            }
            if (++msPos >= msLen) msPos = 0;
        }
    }

    void msTapePost (juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        const int L = tape.getLatencySamples();
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        int rd = msBlockStart - L;
        while (rd < 0) rd += msLen;
        for (int i = 0; i < n; ++i)
        {
            const float a = msAmtLine[(size_t) rd], sIn = msSideIn[(size_t) rd], mIn = msMidIn[(size_t) rd];
            if (++rd >= msLen) rd = 0;
            float m = 0.5f * (l[i] + r[i]), sd = 0.5f * (l[i] - r[i]);
            const float gS = a != 0.0f ? std::exp (msLn2 * a) : 1.0f, gM = 1.0f / gS;
            m *= gS;          // = 1 / gM
            sd *= gM;         // = 1 / gS
            // the tape's low-end change (head bump etc.) stays on the mid: the side lows go back to how they went in
            const float lowDiff = msLow.processSample (sd - sIn);   // keeps running so switching in is smooth
            if (a == 0.0f) continue;
            sd -= std::abs (a) * lowDiff;
            // the harder-driven half gets squashed more by the tape: level-match each half so the stereo width
            // stays as it went in (the overall level still follows the tape, like plain stereo)
            const double k = msEnvCoef;
            eInM  += k * ((double) mIn * mIn - eInM);    eInS  += k * ((double) sIn * sIn - eInS);
            eOutM += k * ((double) m * m - eOutM);       eOutS += k * ((double) sd * sd - eOutS);
            if (eInM > 1.0e-10 && eInS > 1.0e-10 && eOutM > 1.0e-10 && eOutS > 1.0e-10)
            {
                const double tot = (eOutM + eOutS) / (eInM + eInS);
                const float cM = (float) juce::jlimit (0.5, 2.0, std::sqrt (eInM * tot / eOutM));
                const float cS = (float) juce::jlimit (0.5, 2.0, std::sqrt (eInS * tot / eOutS));
                const float w = std::abs (a);
                m  *= 1.0f + w * (cM - 1.0f);
                sd *= 1.0f + w * (cS - 1.0f);
            }
            l[i] = m + sd;
            r[i] = m - sd;
        }
    }
private:
    bool trackMode = false;
    int tapeOsNow = 1, limOsNow = 0, cleanPhaseNow = 0, cleanOsNow = 0, flowModelNow = 0, flowModelWas = 0;
public:
    EshTape esh;                         // V39: Esh, the character tape saturator (in place of Tape when selected)
    ToneShaper perach;                   // V37: Perach, the adaptive tone shaper (in place of Zerem when selected)
    std::atomic<int> soloBand { -1 };   // clean-up band being soloed, -1 = none (UI sets it; not saved)
    ResonanceSuppressor reso;
    TahorStage cleanup;
    AnalyzerTap preTap, postTap, dsTap, charTap;

private:
    static float peakOf (const juce::AudioBuffer<float>& b)
    {
        float p = 0.0f;
        for (int c = 0; c < b.getNumChannels(); ++c)
            p = juce::jmax (p, b.getMagnitude (c, 0, b.getNumSamples()));
        return p;
    }

    std::array<VocalEQ, numCharEqs> eqs;
    std::array<bool, numCharEqs> eqWasOn {};
    double soloSr = 48000.0;
    juce::dsp::IIR::Coefficients<float>::Ptr soloCoeffs;
    std::array<juce::dsp::IIR::Filter<float>, 2> soloFilter;
    juce::AudioBuffer<float> soloBuf, soloDelay;
    int soloPos = 0;
    bool soloWasOn = false;
    VoxGate gate;
    BreathControl breath;
    VocalRider rider;
    std::array<VocalCompressor, 4> comps;
    std::array<MasterCompressor, 5> mcomps;   // V75
    std::array<bool, numComps> compWasOn {};
public:
    std::array<std::atomic<float>, numComps> compUnitGr {};   // per-unit GR, for the tab lights / tests
private:
    DeEsser deEsser;
public:
    VoxFlow flow;
private:
    bool flowWasOn = true;
    LimiterStage limiter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGain, outGain;
};

} // namespace vox
