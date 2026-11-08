#pragma once
// Mishkal Pressure - CLEAN: five compressors on one feed-forward core.
//   LE-11/25E (Renaissance-style, Goliath's LE-RC): Glide / Snap release, Color / Pure, Auto Release (ARC)
//   LE-755C   (RVox-style, Goliath's RVox):          one Compress knob, fixed 3:1 / 10 dB knee, automatic make-up, Gate
//   LE-DS1    (Weiss DS1-style):                     soft knee 0..1, attack, fast + slow release, Average, Full / Band
//   LE-MLA    (Maselec MLA-2-style):                  Input drives a fixed threshold, stepped ratio, ms/dB attack, s/dB release
//   LE-580E   (Pro-C 3-style):                        fourteen Styles, Dry / Wet, Character (Tube / Diode / Bright) pre or post
// Shared: knee, attack / release, Auto Attack, Auto Release, Peak / RMS, Range, Hold, Mix, Make-up, Auto Gain, Delta,
//         sidechain (internal / external, listen, gain, stereo link, Stereo / Mid / Side, look-ahead, detector EQ).

#include <juce_dsp/juce_dsp.h>
#include "VocalChain.h"

namespace pressure
{
enum Engine { E1125 = 0, E755, EDs1, EMla, E580, numEngines };

struct CleanSettings
{
    int   engine = E1125;
    float threshDb = -24.0f, ratio = 4.0f, kneeDb = 6.0f, ds1Knee = 0.6f;
    float attackMs = 8.0f, releaseMs = 120.0f;
    bool  autoRelease = false, autoAttack = false, rms = false;
    int   behavior = 0;      // 0 Glide, 1 Snap
    int   colour = 0;        // 0 Color, 1 Pure
    float dsRelFastMs = 200.0f, dsRelSlowMs = 1200.0f, dsAvgMs = 30.0f;
    int   dsBand = 0;        // 0 full range, 1 band (only the detector EQ's bell band is compressed)
    float makeupDb = 4.0f, mix = 1.0f;
    bool  autoGain = false, delta = false;
    float rangeDb = 24.0f, holdMs = 0.0f;
    // LE-755C
    float voxComp = 0.0f, voxGateDb = -80.0f, voxGainDb = 0.0f; bool voxMatch = false;
    // LE-MLA
    int   mlaIn = 8, mlaRatio = 3, mlaAtk = 3, mlaRel = 4; bool mlaLink = true;
    // LE-580E
    int   proStyle = 0, proChar = 0; float proDrive = 30.0f; bool proCharOn = true, proPost = false, proAutoThr = false;
    float proDryDb = -100.0f, proWetDb = 0.0f;
    // sidechain
    bool  external = false, listen = false;
    float scGainDb = 0.0f, link = 1.0f, lookMs = 2.0f;
    int   ms = 0;            // 0 stereo, 1 mid, 2 side
    float hpHz = 80.0f, bellHz = 3000.0f, bellDb = 3.0f, lpHz = 14000.0f;
};

static const char* const engineNames[numEngines] { "LE-11/25E", "LE-755C", "LE-DS1", "LE-MLA", "LE-580E" };
static const char* const proStyles[14] { "CLEAN", "CLASSIC", "OPTO", "VOCAL", "MASTERING", "BUS", "PUNCH", "PUMPING",
                                         "VERSATILE", "SMOOTH", "UPWARD", "TTM", "OP-EL", "VARI-MU" };
inline constexpr float mlaRatios[9] { 1.2f, 1.4f, 1.7f, 2.0f, 2.5f, 3.0f, 5.0f, 8.0f, 100.0f };
inline constexpr float mlaAtkPerDb[9] { 0.005f, 0.01f, 0.015f, 0.02f, 0.03f, 0.05f, 0.1f, 0.2f, 0.5f };   // ms per dB
inline constexpr float mlaRelPerDb[9] { 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.15f, 0.2f, 0.3f, 0.5f };      // s per dB

// a Pro-C 3-like style: how it shapes the knee, timing and detector
struct StyleInfo { float kneeAdd, atkMul, relMul, progDep, rmsMix, colour; };
inline StyleInfo styleInfo (int s)
{
    static const StyleInfo t[14] {
        { 0, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f },     // Clean: transparent
        { 2, 1.0f, 1.2f, 0.3f, 0.2f, 0.10f },    // Classic: feedback-ish, gentle colour
        { 8, 2.0f, 2.5f, 0.8f, 0.5f, 0.05f },    // Opto: slow and program dependent
        { 6, 0.8f, 1.0f, 0.5f, 0.3f, 0.0f },     // Vocal: soft, quick to catch
        { 10, 1.5f, 1.5f, 0.4f, 0.6f, 0.0f },    // Mastering: very soft, smooth
        { 4, 1.2f, 1.0f, 0.2f, 0.3f, 0.05f },    // Bus: glue
        { 3, 2.5f, 0.6f, 0.0f, 0.0f, 0.08f },    // Punch: lets transients through
        { 0, 0.5f, 0.4f, 0.0f, 0.0f, 0.10f },    // Pumping: fast release, audible
        { 5, 1.0f, 1.0f, 0.5f, 0.3f, 0.0f },     // Versatile
        { 12, 1.5f, 2.0f, 0.6f, 0.7f, 0.0f },    // Smooth
        { 6, 1.0f, 1.0f, 0.3f, 0.3f, 0.0f },     // Upward (behaves like a soft downward here in V1)
        { 6, 0.6f, 0.6f, 0.2f, 0.2f, 0.05f },    // TTM
        { 7, 1.6f, 2.0f, 0.7f, 0.5f, 0.06f },    // Op-El
        { 9, 1.8f, 2.2f, 0.6f, 0.4f, 0.12f } };  // Vari-Mu
    return t[juce::jlimit (0, 13, s)];
}

class CleanCompressor
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c) { look[c].assign (1 << 13, 0.0f); }
        lookPos = 0;
        tmp.setSize (2, juce::jmax (32, maxBlock));
        sat.prepare (sr);
        coefsAt = { -1.0f, -1.0f, -1.0f, -1.0f };
        CleanSettings d; updateFilters (d);   // second-order coefficients in place before the filters size their state
        reset();
    }
    void reset()
    {
        env = slowEnv = 0.0f; rmsAcc[0] = rmsAcc[1] = 0.0f; holdLeft = 0; gateEnv = 0.0f; avgGr = 0.0f; autoThrLvl = -18.0f;
        for (auto& f : hp) f.reset(); for (auto& f : bell) f.reset(); for (auto& f : lp) f.reset(); for (auto& f : band) f.reset();
        sat.reset();
    }
    static int lookSamples (const CleanSettings& s, double rate) { return juce::roundToInt (juce::jlimit (0.0f, 10.0f, s.lookMs) * 0.001 * rate); }

    float softKnee (float over, float ratio, float knee) const
    {
        const float slope = 1.0f / juce::jmax (1.0f, ratio) - 1.0f;
        if (knee <= 0.01f) return over > 0.0f ? slope * over : 0.0f;
        if (2.0f * over < -knee) return 0.0f;
        if (2.0f * over > knee) return slope * over;
        return slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
    }

    // the static curve each engine uses (dB of gain change, <= 0, for a detector level in dB)
    float curve (const CleanSettings& s, float lvl) const
    {
        switch (s.engine)
        {
            case E755: return softKnee (lvl - s.voxComp, 3.0f, 10.0f);
            case EDs1: { const float k = juce::jlimit (0.0f, 1.0f, s.ds1Knee) * 2.0f * std::abs (s.threshDb); return softKnee (lvl - s.threshDb, s.ratio, k); }
            case EMla: return softKnee (lvl - mlaThreshold (s), mlaRatios[juce::jlimit (0, 8, s.mlaRatio)], 8.0f);
            case E580: return softKnee (lvl - effThresh (s), s.ratio, s.kneeDb + styleInfo (s.proStyle).kneeAdd);
            default:   return softKnee (lvl - s.threshDb, s.ratio, s.kneeDb);
        }
    }
    static float mlaThreshold (const CleanSettings& s) { return -22.0f + (float) juce::jlimit (0, 20, s.mlaIn); }   // Input Gain 0 dB = all the way into it, -20 dB = barely
    float effThresh (const CleanSettings& s) const { return s.proAutoThr ? s.threshDb + (autoThrLvl + 18.0f) : s.threshDb; }

    // the make-up the engine adds on its own
    float autoMakeup (const CleanSettings& s) const
    {
        if (s.engine == E755)
            return s.voxMatch ? avgGr : -softKnee (0.0f - s.voxComp, 3.0f, 10.0f);   // RVox: louder as you push, or held even
        if (s.autoGain) return juce::jmin (24.0f, -0.6f * curve (s, -12.0f));
        return 0.0f;
    }

    // processes `buf` (n samples, nc channels); `key` = the external sidechain or null. Returns the deepest reduction (dB, >= 0)
    float process (juce::AudioBuffer<float>& buf, int nc, int n, const juce::AudioBuffer<float>* key, const CleanSettings& s)
    {
        updateFilters (s);
        const int la = lookSamples (s, sr), L = (int) look[0].size();
        const float scG = juce::Decibels::decibelsToGain (s.scGainDb);
        const auto si = styleInfo (s.proStyle);
        const bool pro = s.engine == E580, mla = s.engine == EMla, ds1 = s.engine == EDs1, vox = s.engine == E755, rc = s.engine == E1125;
        // timing
        float atkMs = s.attackMs, relMs = s.releaseMs;
        if (vox) { atkMs = 3.0f; relMs = 80.0f; }
        if (ds1) relMs = s.dsRelFastMs;
        if (pro) { atkMs *= si.atkMul; relMs *= si.relMul; }
        const bool useRms = s.rms || ds1 || (pro && si.rmsMix > 0.45f);
        const float rmsMs = ds1 ? juce::jmax (1.0f, s.dsAvgMs) : 10.0f;
        const float rmsK = coef (rmsMs);
        const float slowDepth = vox ? 0.5f : (s.autoRelease || ds1 || (pro && si.progDep > 0.4f)) ? 0.6f : 0.0f;
        const float relSlowMs = ds1 ? s.dsRelSlowMs : vox ? 600.0f : relMs * 5.0f;
        const float relSlowK = coef (relSlowMs), atkSlowK = coef (400.0f);
        const int holdN = (int) (s.holdMs * 0.001 * sr);
        const float makeup = juce::Decibels::decibelsToGain ((vox ? s.voxGainDb : s.makeupDb) + autoMakeup (s));   // LE-755C has its own Gain
        const float dryG = pro ? (s.proDryDb <= -60.0f ? 0.0f : juce::Decibels::decibelsToGain (s.proDryDb)) : 1.0f - s.mix;
        const float wetG = pro ? juce::Decibels::decibelsToGain (s.proWetDb) : s.mix;
        const float avgK = std::exp (-1.0f / (float) (1.0 * sr));
        const float thrK = std::exp (-1.0f / (float) (3.0 * sr));
        float maxGr = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            // ---- the detector
            float x[2] { buf.getSample (0, i), buf.getSample (juce::jmin (1, nc - 1), i) };
            float k[2] { x[0], x[1] };
            if (key != nullptr) { k[0] = key->getSample (0, i); k[1] = key->getSample (juce::jmin (1, key->getNumChannels() - 1), i); }
            if (s.ms == 1) { const float m = 0.5f * (k[0] + k[1]); k[0] = k[1] = m; }
            else if (s.ms == 2) { const float d = 0.5f * (k[0] - k[1]); k[0] = k[1] = d; }
            float lv[2];
            for (int c = 0; c < 2; ++c)
            {
                float v = k[c] * scG;
                v = hp[(size_t) c].processSample (v);
                v = bell[(size_t) c].processSample (v);
                v = lp[(size_t) c].processSample (v);
                scOut[c] = v;
                if (useRms) { rmsAcc[c] = rmsK * rmsAcc[c] + (1.0f - rmsK) * v * v; lv[c] = 0.5f * juce::Decibels::gainToDecibels (rmsAcc[c] * 2.0f, -120.0f); }
                else lv[c] = juce::Decibels::gainToDecibels (std::abs (v), -120.0f);
            }
            const float mx = juce::jmax (lv[0], lv[1]);
            if (s.proAutoThr) autoThrLvl = thrK * autoThrLvl + (1.0f - thrK) * juce::jmax (-60.0f, mx);
            // stereo link: 100% = both sides from the louder one; MLA ImageLink keeps short one-sided peaks to themselves
            const float linkAmt = mla ? (s.mlaLink ? 0.85f : 0.0f) : s.link;
            float lvl = mx;
            if (nc > 1 && linkAmt < 0.999f) lvl = juce::jmax (lv[0], lv[1]) * linkAmt + 0.5f * (lv[0] + lv[1]) * (1.0f - linkAmt);
            const float target = juce::jmax (-s.rangeDb, curve (s, lvl));   // <= 0

            // ---- the envelope
            float a = atkMs, r = relMs;
            if (mla)
            {
                const float depth = juce::jmax (1.0f, -target);
                a = mlaAtkPerDb[juce::jlimit (0, 8, s.mlaAtk)] * depth;
                r = mlaRelPerDb[juce::jlimit (0, 8, s.mlaRel)] * 1000.0f * juce::jmax (1.0f, -env);
            }
            if (s.autoAttack || (pro && si.progDep > 0.0f))   // fast for sudden peaks, slower for the body
            {
                const float jump = juce::jlimit (0.0f, 1.0f, (env - target) / 12.0f);
                a = a * (1.0f - 0.8f * jump);
            }
            if (rc && target > env)   // LE-RC: Glide slows the release near threshold, Snap speeds it up
            {
                const float depth = juce::jlimit (0.0f, 1.0f, -env / 6.0f);
                r *= s.behavior == 0 ? 1.0f + 2.0f * (1.0f - depth) : 0.35f + 0.65f * depth;
            }
            if (target < env) { env = coef (juce::jmax (0.01f, a)) * (env - target) + target; holdLeft = holdN; }
            else if (holdLeft > 0) --holdLeft;
            else env = coef (juce::jmax (1.0f, r)) * (env - target) + target;
            const float slowT = target * slowDepth;
            slowEnv = (slowT < slowEnv ? atkSlowK : relSlowK) * (slowEnv - slowT) + slowT;
            float gr = slowDepth > 0.0f ? juce::jmin (env, slowEnv) : env;   // dB <= 0
            // RVox's gate: about 1:4 below its level, keeping the gaps quiet
            if (vox && s.voxGateDb > -79.0f)
            {
                const float under = juce::jmax (0.0f, s.voxGateDb - mx);
                const float gt = -juce::jmin (40.0f, under * 3.0f);
                gateEnv = coef (gt < gateEnv ? 2.0f : 120.0f) * (gateEnv - gt) + gt;
                gr += gateEnv;
            }
            maxGr = juce::jmax (maxGr, -gr);
            avgGr = avgK * avgGr + (1.0f - avgK) * (-gr);
            const float g = juce::Decibels::decibelsToGain (gr);

            // ---- the audio, delayed by the look-ahead
            for (int c = 0; c < nc; ++c)
            {
                auto& line = look[c];
                line[(size_t) ((lookPos + i) & (L - 1))] = x[c];
                const float d = line[(size_t) ((lookPos + i - la) & (L - 1))];
                float y;
                if (s.listen) { buf.setSample (c, i, scOut[c]); continue; }
                if (ds1 && s.dsBand == 1)   // compress only the band
                {
                    const float b = band[(size_t) c].processSample (d);
                    y = d - b + b * g;
                }
                else y = d * g;
                // colour
                if (rc && s.colour == 0) y = sat.processSample (c, y, 3.0f, 0.12f, juce::jlimit (0.0f, 0.45f, 0.2f + 0.02f * -gr));
                else if (pro && s.proCharOn)
                {
                    const float dr = juce::jlimit (0.0f, 1.0f, s.proDrive / 100.0f);
                    const float drive = s.proChar == 0 ? 1.5f + 3.0f * dr : s.proChar == 1 ? 2.0f + 4.0f * dr : 1.2f + 2.0f * dr;
                    const float bias  = s.proChar == 0 ? 0.15f : s.proChar == 1 ? 0.02f : 0.06f;
                    const float amt   = juce::jlimit (0.0f, 0.6f, (0.15f + 0.4f * dr) + si.colour);
                    y = sat.processSample (c, y, drive, bias, amt);
                }
                y *= makeup;
                const float out = d * dryG + y * wetG;
                buf.setSample (c, i, s.delta ? d - y / juce::jmax (1.0e-6f, makeup) : out);
            }
        }
        lookPos = (lookPos + n) & (L - 1);
        return maxGr;
    }

private:
    float coef (float ms) const { return std::exp (-1.0f / (juce::jmax (0.01f, ms) * 0.001f * (float) sr)); }
    void updateFilters (const CleanSettings& s)
    {
        const std::array<float, 4> want { s.hpHz, s.bellHz, s.bellDb, s.lpHz };
        if (want == coefsAt) return;
        coefsAt = want;
        const double ny = sr * 0.45;
        auto h = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, juce::jlimit (10.0, ny, (double) s.hpHz), 0.707));
        auto b = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sr, juce::jlimit (20.0, ny, (double) s.bellHz), 1.0, juce::Decibels::decibelsToGain (s.bellDb)));
        auto l = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sr, juce::jlimit (200.0, ny, (double) s.lpHz), 0.707));
        auto bp = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, juce::jlimit (20.0, ny, (double) s.bellHz), 1.0));
        for (int c = 0; c < 2; ++c) { hp[(size_t) c].coefficients = h; bell[(size_t) c].coefficients = b; lp[(size_t) c].coefficients = l; band[(size_t) c].coefficients = bp; }
    }

    double sr = 48000.0;
    std::array<std::vector<float>, 2> look; int lookPos = 0;
    juce::AudioBuffer<float> tmp;
    float env = 0.0f, slowEnv = 0.0f, rmsAcc[2] {}, gateEnv = 0.0f, avgGr = 0.0f, autoThrLvl = -18.0f, scOut[2] {};
    int holdLeft = 0;
    std::array<juce::dsp::IIR::Filter<float>, 2> hp, bell, lp, band;
    std::array<float, 4> coefsAt {};
    vox::Saturator sat;
};
}
