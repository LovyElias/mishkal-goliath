#pragma once
// Mishkal Pressure - VINTAGE: fifteen units on one character core. Each unit's own controls (its parameters, listed in
// vintageParams()) are turned into one or two compressor stages with that unit's detector, curve, timing and colour.
//   LE-76 (1176), LE-660 (Fairchild), LE-STA (Sta-Level), LE-2500 (API), LE-G (SSL bus), LE-IRON (Manley Vari-Mu),
//   LE-336 (Neve 33609), LE-8X (Distressor), LE-160 (dbx 160), LE-7 (Fatso), LE-760 (Compex), LE-SHM (Shadow Hills),
//   LE-2A (LA-2A), LE-1B (CL 1B), LE-VSC (Vertigo VSC-3)

#include <juce_dsp/juce_dsp.h>
#include "VocalChain.h"

namespace pressure
{
inline constexpr int numVintage = 15;

// ---------------------------------------------------------------- the parameters
struct VParam
{
    const char* id; const char* name; char kind;   // 'F' float, 'C' choice, 'B' bool
    float lo, hi, def, centre;                     // F: range, default, skew centre (0 = linear); C / B: def = index / state
    const char* choices;                           // C: "|"-separated
    const char* unit;                              // what the screen adds after the value
};

inline const std::vector<VParam>& vintageParams()
{
    static const std::vector<VParam> p {
        // shared
        { "vDelta", "Vintage Delta", 'B', 0, 1, 0, 0, nullptr, "" },
        { "vScHpf", "Vintage SC HPF", 'C', 0, 1, 0, 0, "OFF|ON", "" },
        { "vLink", "Vintage Link", 'B', 0, 1, 1, 0, nullptr, "" },
        { "vThd", "Vintage THD", 'F', 0, 100, 50, 0, nullptr, "%" },
        // LE-76
        { "v0In", "LE-76 Input", 'F', 0, 40, 22, 0, nullptr, "dB+" },
        { "v0Ratio", "LE-76 Ratio", 'C', 0, 0, 0, 0, "4|8|12|20", "" },
        { "v0All", "LE-76 All Buttons", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v0Atk", "LE-76 Attack", 'F', 0.02f, 0.8f, 0.25f, 0.2f, nullptr, "ms" },
        { "v0Rel", "LE-76 Release", 'F', 50, 1100, 250, 250, nullptr, "ms" },
        { "v0Out", "LE-76 Output", 'F', -20, 20, 9, 0, nullptr, "dB+" },
        { "v0Mix", "LE-76 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v0Meter", "LE-76 Meter", 'C', 0, 0, 0, 0, "GR|+8|+4|OFF", "" },
        { "v0Rev", "LE-76 Revision", 'C', 0, 0, 0, 0, "A|B|C|D|E|F|G|H|LN", "" },
        // LE-660
        { "v1In", "LE-660 Input Gain", 'C', 0, 0, 0, 0, "-12|-8|-4|0|+4|+8", "dB" },
        { "v1Thr", "LE-660 Threshold", 'F', 0, 10, 4, 0, nullptr, "" },
        { "v1Tc", "LE-660 Time Constant", 'C', 0, 0, 1, 0, "1|2|3|4|5|6", "" },
        { "v1Out", "LE-660 Output", 'F', -20, 20, 4, 0, nullptr, "dB+" },
        { "v1Dc", "LE-660 DC Threshold", 'F', -5, 5, 0, 0, nullptr, "" },
        { "v1Mix", "LE-660 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v1Lat", "LE-660 Lat / Vert", 'C', 0, 0, 0, 0, "OFF|ON", "" },
        // LE-STA
        { "v2In", "LE-STA Input", 'F', -12, 24, 10, 0, nullptr, "dB+" },
        { "v2Mode", "LE-STA Mode", 'C', 0, 0, 1, 0, "SINGLE|DOUBLE|TRIPLE", "" },
        { "v2Rel", "LE-STA Release", 'C', 0, 0, 2, 0, "1|2|3|4|5|6", "" },
        { "v2Out", "LE-STA Output", 'F', -20, 20, 4, 0, nullptr, "dB+" },
        { "v2Mix", "LE-STA Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v2Meter", "LE-STA Meter", 'C', 0, 0, 0, 0, "GR|VU", "" },
        // LE-2500
        { "v3Thr", "LE-2500 Threshold", 'F', -40, 0, -18, 0, nullptr, "dB" },
        { "v3Ratio", "LE-2500 Ratio", 'C', 0, 0, 3, 0, "1.5|2|3|4|6|10|INF", ":1" },
        { "v3Knee", "LE-2500 Knee", 'C', 0, 0, 1, 0, "HARD|MED|SOFT", "" },
        { "v3Thrust", "LE-2500 Thrust", 'C', 0, 0, 0, 0, "NORM|MED|LOUD", "" },
        { "v3Atk", "LE-2500 Attack", 'C', 0, 0, 3, 0, ".03|.1|.3|1|3|10", "ms" },
        { "v3Rel", "LE-2500 Release", 'C', 0, 0, 2, 0, ".05|.1|.2|.5|1|2", "s" },
        { "v3Var", "LE-2500 Var Release", 'F', 0.05f, 3, 1.4f, 0, nullptr, "s" },
        { "v3Link", "LE-2500 Link", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v3Mk", "LE-2500 Make-up", 'F', 0, 20, 4, 0, nullptr, "dB+" },
        { "v3Mix", "LE-2500 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v3In", "LE-2500 Comp In", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v3Type", "LE-2500 Type", 'C', 0, 0, 0, 0, "NEW|OLD", "" },
        // LE-G
        { "v4Thr", "LE-G Threshold", 'F', -24, 0, -12, 0, nullptr, "dB" },
        { "v4Ratio", "LE-G Ratio", 'C', 0, 0, 1, 0, "2|4|10", ":1" },
        { "v4In", "LE-G Comp In", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v4Atk", "LE-G Attack", 'C', 0, 0, 4, 0, ".1|.3|1|3|10|30", "ms" },
        { "v4Rel", "LE-G Release", 'C', 0, 0, 4, 0, ".1|.3|.6|1.2|AUTO", "s" },
        { "v4Mk", "LE-G Make-up", 'F', 0, 20, 3, 0, nullptr, "dB+" },
        { "v4Mix", "LE-G Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v4Fade", "LE-G Auto Fade", 'C', 0, 0, 0, 0, "OFF|ON", "" },
        // LE-IRON
        { "v5In", "LE-IRON Input", 'C', 0, 0, 3, 0, "-12|-8|-4|0|+4|+8|+12", "dB" },
        { "v5Thr", "LE-IRON Threshold", 'F', -40, 0, -20, 0, nullptr, "dB" },
        { "v5Mode", "LE-IRON Mode", 'C', 0, 0, 0, 0, "COMPRESS|LIMIT", "" },
        { "v5Hp", "LE-IRON HP Sidechain", 'C', 0, 0, 1, 0, "OFF|100 Hz", "" },
        { "v5Atk", "LE-IRON Attack", 'F', 25, 70, 35, 0, nullptr, "ms" },
        { "v5Rec", "LE-IRON Recovery", 'C', 0, 0, 2, 0, ".2|.4|.6|4|8", "s" },
        { "v5Out", "LE-IRON Output", 'F', -20, 20, 4, 0, nullptr, "dB+" },
        { "v5Mix", "LE-IRON Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        // LE-336
        { "v6Thr", "LE-336 Threshold", 'F', -28, 0, -14, 0, nullptr, "dB" },
        { "v6Ratio", "LE-336 Ratio", 'C', 0, 0, 2, 0, "1.5|2|3|4|6", ":1" },
        { "v6Atk", "LE-336 Attack", 'C', 0, 0, 1, 0, "FAST|SLOW", "" },
        { "v6Rec", "LE-336 Recovery", 'C', 0, 0, 4, 0, "100|400|800|1.5k|A1|A2", "" },
        { "v6Gain", "LE-336 Gain", 'F', 0, 20, 6, 0, nullptr, "dB+" },
        { "v6Lim", "LE-336 Limit", 'F', 4, 15, 12, 0, nullptr, "dB+" },
        { "v6LimRec", "LE-336 Limit Recovery", 'C', 0, 0, 1, 0, "50|100|200|A1|A2", "" },
        { "v6CompIn", "LE-336 Comp In", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v6LimIn", "LE-336 Limit In", 'B', 0, 1, 1, 0, nullptr, "" },
        // LE-8X
        { "v7In", "LE-8X Input", 'F', 0, 10, 4.5f, 0, nullptr, "" },
        { "v7Ratio", "LE-8X Ratio", 'C', 0, 0, 3, 0, "1:1|2:1|3:1|4:1|6:1|10:1|20:1|NUKE", "" },
        { "v7Brit", "LE-8X Brit Mode", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v7Atk", "LE-8X Attack", 'F', 0, 10, 5, 0, nullptr, "" },
        { "v7Rel", "LE-8X Release", 'F', 0, 10, 5, 0, nullptr, "" },
        { "v7Out", "LE-8X Output", 'F', 0, 10, 4, 0, nullptr, "" },
        { "v7Mix", "LE-8X Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v7DetHp", "LE-8X Detector HP", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v7DetBand", "LE-8X Detector Band", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v7DetLink", "LE-8X Detector Link", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v7Audio", "LE-8X Audio", 'C', 0, 0, 0, 0, "DIST 2|DIST 3|HP", "" },
        // LE-160
        { "v8Thr", "LE-160 Threshold", 'F', -40, 20, -10, 0, nullptr, "dB" },
        { "v8Ratio", "LE-160 Compression", 'F', 1, 20, 4, 4, nullptr, ":1" },
        { "v8Easy", "LE-160 OverEasy", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v8Out", "LE-160 Output Gain", 'F', -20, 20, 6, 0, nullptr, "dB+" },
        { "v8Mix", "LE-160 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v8Meter", "LE-160 Meter", 'C', 0, 0, 2, 0, "IN|OUT|GR", "" },
        // LE-7
        { "v9In", "LE-7 Input", 'F', 0, 10, 6, 0, nullptr, "" },
        { "v9Warm", "LE-7 Warmth", 'F', 0, 10, 4, 0, nullptr, "" },
        { "v9Tranny", "LE-7 Tranny", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v9Out", "LE-7 Output", 'F', 0, 10, 5, 0, nullptr, "" },
        { "v9Mix", "LE-7 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v9Mode", "LE-7 Compressor", 'C', 0, 0, 0, 0, "BUSS|G.P.|TRACK", "" },
        { "v9Spank", "LE-7 Spank", 'B', 0, 1, 0, 0, nullptr, "" },
        // LE-760
        { "v10Thr", "LE-760 Threshold", 'C', 0, 0, 2, 0, "-20|-16|-12|-8|-4|0", "dB" },
        { "v10Ratio", "LE-760 Ratio", 'C', 0, 0, 2, 0, "1|2|3|5|10|20", ":1" },
        { "v10Atk", "LE-760 Attack", 'C', 0, 0, 1, 0, "250us|2.5ms|25ms", "" },
        { "v10Rel", "LE-760 Release", 'C', 0, 0, 5, 0, "25ms|.1|.4|1.6|3.2|AUTO", "" },
        { "v10Lim", "LE-760 Limiter", 'C', 0, 0, 1, 0, "OFF|ON|PRE-EMP", "" },
        { "v10Exp", "LE-760 Expander", 'C', 0, 0, 0, 0, "OFF|EXP|GATE", "" },
        { "v10Out", "LE-760 Output", 'F', -20, 20, 3, 0, nullptr, "dB+" },
        { "v10Mix", "LE-760 Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        // LE-SHM
        { "v11OThr", "LE-SHM Opto Threshold", 'F', 0, 10, 4, 0, nullptr, "" },
        { "v11OGain", "LE-SHM Opto Gain", 'F', 0, 8, 2, 0, nullptr, "dB+" },
        { "v11Trans", "LE-SHM Transformer", 'C', 0, 0, 1, 0, "NICKEL|IRON|STEEL", "" },
        { "v11DThr", "LE-SHM Discrete Threshold", 'F', 0, 10, 5, 0, nullptr, "" },
        { "v11DRatio", "LE-SHM Discrete Ratio", 'C', 0, 0, 2, 0, "1.2|1.5|2|3|4|6", ":1" },
        { "v11DAtk", "LE-SHM Discrete Attack", 'C', 0, 0, 2, 0, "1|2|3|4|5|6", "" },
        { "v11DRec", "LE-SHM Discrete Recovery", 'C', 0, 0, 5, 0, ".1|.25|.4|.8|1.2|DUAL", "" },
        { "v11DGain", "LE-SHM Discrete Gain", 'F', 0, 8, 1.5f, 0, nullptr, "dB+" },
        { "v11Mix", "LE-SHM Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v11OptoIn", "LE-SHM Opto In", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v11DiscIn", "LE-SHM Discrete In", 'B', 0, 1, 1, 0, nullptr, "" },
        { "v11ScF", "LE-SHM SC Filter", 'C', 0, 0, 1, 0, "OFF|90 Hz", "" },
        // LE-2A
        { "v12Peak", "LE-2A Peak Reduction", 'F', 0, 100, 45, 0, nullptr, "" },
        { "v12Gain", "LE-2A Gain", 'F', 0, 100, 50, 0, nullptr, "" },
        { "v12Mode", "LE-2A Mode", 'C', 0, 0, 0, 0, "COMPRESS|LIMIT", "" },
        { "v12Mix", "LE-2A Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v12Meter", "LE-2A Meter", 'C', 0, 0, 0, 0, "GR|+10|+4", "" },
        // LE-1B
        { "v13Thr", "LE-1B Threshold", 'F', -32, 0, -16, 0, nullptr, "dB" },
        { "v13Ratio", "LE-1B Ratio", 'F', 1.5f, 10, 4, 4, nullptr, ":1" },
        { "v13Timing", "LE-1B Timing", 'C', 0, 0, 1, 0, "FIXED|MANUAL|F + M", "" },
        { "v13Atk", "LE-1B Attack", 'F', 0.5f, 300, 25, 25, nullptr, "ms" },
        { "v13Rel", "LE-1B Release", 'F', 50, 10000, 500, 600, nullptr, "ms" },
        { "v13Gain", "LE-1B Gain", 'F', -10, 20, 6, 0, nullptr, "dB+" },
        { "v13Mix", "LE-1B Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v13Sc", "LE-1B Sidechain", 'C', 0, 0, 0, 0, "INTERNAL|EXTERNAL", "" },
        // LE-VSC
        { "v14Thr", "LE-VSC Threshold", 'F', -20, 20, 2, 0, nullptr, "dBu" },
        { "v14Ratio", "LE-VSC Ratio", 'C', 0, 0, 3, 0, "SOFT|2|3|4|6|8|10|BRICK", ":1" },
        { "v14Det", "LE-VSC Detector", 'C', 0, 0, 0, 0, "PEAK|RMS", "" },
        { "v14Atk", "LE-VSC Attack", 'C', 0, 0, 3, 0, ".1|.3|1|3|10|40", "ms" },
        { "v14Rel", "LE-VSC Release", 'C', 0, 0, 5, 0, ".05|.1|.3|1.2|A.F|A.S", "" },
        { "v14Mk", "LE-VSC Make-up", 'F', 0, 20, 4, 0, nullptr, "dB+" },
        { "v14Mix", "LE-VSC Mix", 'F', 0, 100, 100, 0, nullptr, "%" },
        { "v14Byp", "LE-VSC Bypass", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v14Stereo", "LE-VSC Stereo SC", 'B', 0, 1, 0, 0, nullptr, "" },
        { "v14ScF", "LE-VSC SC Filter", 'C', 0, 0, 1, 0, "OFF|60 Hz", "" },
    };
    return p;
}
inline const VParam* findVParam (const juce::String& id)
{
    for (auto& v : vintageParams()) if (id == v.id) return &v;
    return nullptr;
}

// ---------------------------------------------------------------- one stage of compression with a unit's character
struct VStage
{
    bool on = true;
    enum Curve { Knee = 0, VariMu, Sta, Opto } curve = Knee;
    float threshDb = -18.0f, ratio = 4.0f, kneeDb = 4.0f;
    float attackMs = 5.0f, releaseMs = 200.0f;
    float slowDepth = 0.0f, slowAttackMs = 400.0f, slowReleaseMs = 1500.0f;   // program-dependent release (0 = none)
    bool  rms = false, feedback = false;
    float scHpHz = 0.0f, tiltDb = 0.0f, bandDb = 0.0f;   // detector: high pass, lean toward the highs (Thrust), presence (8X Band)
    float link = 1.0f;
    float drive = 1.0f, bias = 0.0f, satBase = 0.0f, satPerDb = 0.0f, satMax = 0.0f; bool preColour = false;
    float lowSat = 0.0f, topHz = 0.0f;                  // transformer low end, a soft top (0 = none)
    float makeupDb = 0.0f;
    float limitDb = 0.0f, limitRelMs = 100.0f;          // a limiter after it (0 = none)
    float expThrDb = -200.0f, expRatio = 1.0f;          // an expander / gate under it
    float rangeDb = 40.0f;
};
struct VintageSettings
{
    std::array<VStage, 2> st;   // the second only on LE-SHM (opto, then discrete)
    int stages = 1;
    float inDb = 0.0f, outDb = 0.0f, mix = 1.0f;
    bool delta = false, ms = false, bypass = false, autoFade = false;
    float thd = 1.0f;           // the Character drawer's THD (1 = as designed)
};

class VintageStageDsp
{
public:
    void prepare (double rate) { sr = rate; sat.prepare (sr); reset(); filtAt = {}; setFilters (VStage {}, true); }
    void reset()
    {
        fast = slow = lim = 0.0f; rmsAcc[0] = rmsAcc[1] = 0.0f; fbGain = 1.0f; low = {}; top = {}; expEnv = 0.0f;
        for (auto& f : hp) f.reset(); for (auto& f : tilt) f.reset(); for (auto& f : bandF) f.reset(); sat.reset();
    }
    // processes one sample pair in place; returns this sample's reduction (dB, >= 0)
    float tick (float* x, int nc, const float* key, const VStage& s, float thd)
    {
        float lv[2];
        for (int c = 0; c < nc; ++c)
        {
            float d = key != nullptr ? key[c] : (s.feedback ? x[c] * fbGain : x[c]);
            if (s.scHpHz > 0.0f) d = hp[(size_t) c].processSample (d);
            if (std::abs (s.tiltDb) > 0.05f) d = tilt[(size_t) c].processSample (d);
            if (s.bandDb > 0.05f) d = bandF[(size_t) c].processSample (d);
            if (s.rms) { rmsAcc[c] = rmsK * rmsAcc[c] + (1.0f - rmsK) * d * d; lv[c] = 0.5f * juce::Decibels::gainToDecibels (2.0f * rmsAcc[c], -120.0f); }
            else lv[c] = juce::Decibels::gainToDecibels (std::abs (d), -120.0f);
        }
        if (nc == 1) lv[1] = lv[0];
        const float mx = juce::jmax (lv[0], lv[1]);
        // unlinked: the two sides' power averaged (not their dB - a silent side mustn't hide the other)
        const float avg = 10.0f * std::log10 (0.5f * (std::pow (10.0f, lv[0] * 0.1f) + std::pow (10.0f, lv[1] * 0.1f)) + 1.0e-12f);
        const float lvl = mx * s.link + avg * (1.0f - s.link);
        const float over = lvl - s.threshDb;
        float target = 0.0f;
        switch (s.curve)
        {
            case VStage::VariMu: { const float xx = over + 6.0f; target = xx <= 0.0f ? 0.0f : -0.85f * xx * xx / (xx + 18.0f) * (s.ratio / 4.0f); break; }
            case VStage::Sta:    { const float xx = over + 8.0f; target = xx <= 0.0f ? 0.0f : -0.70f * xx * xx / (xx + 10.0f); break; }
            case VStage::Opto:   { const float xx = over + 6.0f; target = xx <= 0.0f ? 0.0f : -(1.0f - 1.0f / juce::jmax (1.0f, s.ratio)) * xx * xx / (xx + 8.0f); break; }
            default:             target = MasterKnee (over, s.ratio, s.kneeDb); break;
        }
        target = juce::jmax (-s.rangeDb, target);
        fast = (target < fast ? coef (s.attackMs) : coef (s.releaseMs)) * (fast - target) + target;
        float gr = fast;
        if (s.slowDepth > 0.0f)
        {
            const float st = target * s.slowDepth;
            slow = (st < slow ? coef (s.slowAttackMs) : coef (s.slowReleaseMs)) * (slow - st) + st;
            gr = juce::jmin (fast, slow);
        }
        // the expander / gate under the threshold
        if (s.expThrDb > -150.0f)
        {
            const float under = s.expThrDb - mx;
            const float et = under > 0.0f ? -juce::jmin (60.0f, under * (s.expRatio - 1.0f)) : 0.0f;
            expEnv = (et < expEnv ? coef (1.0f) : coef (80.0f)) * (expEnv - et) + et;
            gr += expEnv;
        }
        float g = juce::Decibels::decibelsToGain (gr);
        fbGain = g;
        const float mk = juce::Decibels::decibelsToGain (s.makeupDb);
        const float satAmt = juce::jlimit (0.0f, s.satMax, s.satBase + s.satPerDb * -gr) * thd;
        float outPk = 0.0f;
        for (int c = 0; c < nc; ++c)
        {
            float y = x[c];
            if (s.preColour && satAmt > 0.0f) y = sat.processSample (c, y, s.drive, s.bias, satAmt) * g;
            else { y *= g; if (satAmt > 0.0f) y = sat.processSample (c, y, s.drive, s.bias, satAmt); }
            if (s.lowSat > 0.0f)   // the transformer: the low end breaks up first
            {
                auto& lo = low[(size_t) c]; lo += kLow * (y - lo);
                const float ld = 1.0f + s.lowSat * thd;
                y = y - lo + std::tanh (lo * ld) / ld;
            }
            if (s.topHz > 0.0f) { auto& tp = top[(size_t) c]; tp += kTop * (y - tp); y = tp; }
            y *= mk;
            x[c] = y; outPk = juce::jmax (outPk, std::abs (y));
        }
        float lr = 0.0f;
        if (s.limitDb != 0.0f)   // the limiter after it: a ceiling, fast in, its own recovery
        {
            const float ceil = juce::Decibels::decibelsToGain (s.limitDb);
            const float want = outPk > ceil ? juce::Decibels::gainToDecibels (ceil / outPk) : 0.0f;
            lim = (want < lim ? coef (0.1f) : coef (s.limitRelMs)) * (lim - want) + want;
            const float lg = juce::Decibels::decibelsToGain (lim);
            for (int c = 0; c < nc; ++c) x[c] *= lg;
            lr = -lim;
        }
        return -gr + lr;
    }
    void setFilters (const VStage& s, bool force = false)
    {
        const std::array<float, 3> want { s.scHpHz, s.tiltDb, s.bandDb };
        if (! force && want == filtAt) return;
        filtAt = want;
        const double ny = sr * 0.45;
        auto h = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, juce::jlimit (10.0, ny, (double) juce::jmax (20.0f, s.scHpHz)), 0.707);
        auto t = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (sr, 1000.0, 0.5, juce::Decibels::decibelsToGain (s.tiltDb));
        auto b = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sr, 4000.0, 0.8, juce::Decibels::decibelsToGain (s.bandDb));
        for (int c = 0; c < 2; ++c)
        {
            if (force) { hp[(size_t) c].coefficients = new juce::dsp::IIR::Coefficients<float> (h); tilt[(size_t) c].coefficients = new juce::dsp::IIR::Coefficients<float> (t);
                         bandF[(size_t) c].coefficients = new juce::dsp::IIR::Coefficients<float> (b); hp[(size_t) c].reset(); tilt[(size_t) c].reset(); bandF[(size_t) c].reset(); }
            else { *hp[(size_t) c].coefficients = h; *tilt[(size_t) c].coefficients = t; *bandF[(size_t) c].coefficients = b; }
        }
    }
    void setRate (const VStage& s)
    {
        rmsK = std::exp (-1.0f / (0.02f * (float) sr));
        kLow = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 120.0f / (float) sr);
        kTop = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmax (1000.0f, s.topHz > 0.0f ? s.topHz : 20000.0f) / (float) sr);
    }
private:
    static float MasterKnee (float o, float ratio, float knee)
    {
        const float slope = 1.0f / juce::jmax (1.0f, ratio) - 1.0f;
        if (knee <= 0.01f) return o > 0.0f ? slope * o : 0.0f;
        if (2.0f * o < -knee) return 0.0f;
        if (2.0f * o > knee) return slope * o;
        return slope * (o + knee * 0.5f) * (o + knee * 0.5f) / (2.0f * knee);
    }
    float coef (float ms) const { return std::exp (-1.0f / (juce::jmax (0.01f, ms) * 0.001f * (float) sr)); }
    double sr = 48000.0;
    float fast = 0.0f, slow = 0.0f, lim = 0.0f, rmsAcc[2] {}, fbGain = 1.0f, expEnv = 0.0f, rmsK = 0.99f, kLow = 0.01f, kTop = 1.0f;
    std::array<float, 2> low {}, top {};
    std::array<juce::dsp::IIR::Filter<float>, 2> hp, tilt, bandF;
    std::array<float, 3> filtAt {};
    vox::Saturator sat;
};

class Vintage
{
public:
    std::atomic<float> grDb { 0.0f };
    void prepare (double rate) { sr = rate; for (auto& s : stages) s.prepare (rate); fade = 0.0f; quiet = 0; }
    void process (juce::AudioBuffer<float>& b, int nc, int n, const juce::AudioBuffer<float>* key, const VintageSettings& v)
    {
        if (v.bypass) { grDb.store (0.0f); return; }
        for (int k = 0; k < v.stages; ++k) { stages[(size_t) k].setFilters (v.st[(size_t) k]); stages[(size_t) k].setRate (v.st[(size_t) k]); }
        const float inG = juce::Decibels::decibelsToGain (v.inDb), outG = juce::Decibels::decibelsToGain (v.outDb);
        const float fadeStep = 1.0f / (float) (5.0 * sr);
        float maxGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float x[2] { b.getSample (0, i), b.getSample (juce::jmin (1, nc - 1), i) }, dry[2] { x[0], x[1] };
            float ky[2] {}; const float* kp = nullptr;
            if (key != nullptr) { ky[0] = key->getSample (0, i); ky[1] = key->getSample (juce::jmin (1, key->getNumChannels() - 1), i); kp = ky; }
            if (v.ms && nc == 2) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = s; }
            x[0] *= inG; x[1] *= inG;
            float gr = 0.0f;
            for (int k = 0; k < v.stages; ++k)
                if (v.st[(size_t) k].on)
                {
                    auto st = v.st[(size_t) k];
                    if (v.ms) st.link = 0.0f;
                    gr += stages[(size_t) k].tick (x, nc, kp, st, v.thd);
                }
            if (v.ms && nc == 2) { const float m = x[0], s = x[1]; x[0] = m + s; x[1] = m - s; }
            // LE-G's Auto Fade: after a silence, the output fades back in over five seconds
            float fg = 1.0f;
            if (v.autoFade)
            {
                const bool silent = std::abs (dry[0]) + std::abs (dry[1]) < 1.0e-5f;
                quiet = silent ? quiet + 1 : 0;
                if (quiet > (int) (2.0 * sr)) fade = 0.0f;
                fade = juce::jmin (1.0f, fade + fadeStep);
                fg = fade * fade;
            }
            for (int c = 0; c < nc; ++c)
            {
                const float wet = x[c] * outG * fg;
                b.setSample (c, i, v.delta ? dry[c] - wet / juce::jmax (1.0e-6f, outG * inG) : dry[c] + v.mix * (wet - dry[c]));
            }
            maxGr = juce::jmax (maxGr, gr);
        }
        grDb.store (maxGr);
    }
private:
    double sr = 48000.0;
    std::array<VintageStageDsp, 2> stages;
    float fade = 1.0f; int quiet = 0;
};

// ---------------------------------------------------------------- each unit's controls -> its stages
// get (id) returns the parameter's plain value (a choice's index, a bool's 0 / 1)
inline VintageSettings vintageFor (int unit, const std::function<float (const char*)>& get)
{
    VintageSettings v; auto& s = v.st[0];
    auto idx = [&] (const char* id) { return (int) std::round (get (id)); };
    auto on = [&] (const char* id) { return get (id) > 0.5f; };
    v.delta = on ("vDelta"); v.thd = get ("vThd") / 50.0f;
    const float scHp = idx ("vScHpf") == 1 ? 100.0f : 0.0f;
    s.link = on ("vLink") ? 1.0f : 0.0f; s.scHpHz = scHp;
    switch (unit)
    {
        case 0:   // LE-76: Input drives a fixed threshold; four ratios (or all buttons); fast FET timing; the revision's colour
        {
            static const float ra[4] { 4, 8, 12, 20 };
            const bool all = on ("v0All");
            s.threshDb = -get ("v0In"); s.ratio = all ? 20.0f : ra[juce::jlimit (0, 3, idx ("v0Ratio"))]; s.kneeDb = all ? 8.0f : 4.0f;
            s.attackMs = get ("v0Atk"); s.releaseMs = get ("v0Rel"); s.slowDepth = 0.6f; s.slowReleaseMs = get ("v0Rel") * 4.0f; s.slowAttackMs = 300.0f;
            const int r = idx ("v0Rev");
            const bool blue = r == 0, black = r == 4 || r == 7;
            s.drive = blue ? 1.3f : 1.0f; s.bias = blue ? 0.12f : black ? 0.03f : 0.05f; s.preColour = blue;
            s.satBase = blue ? 0.2f : black ? 0.03f : 0.08f; s.satPerDb = blue ? 0.02f : 0.015f; s.satMax = all ? 0.6f : blue ? 0.55f : 0.35f;
            if (all) { s.drive += 0.6f; s.satBase += 0.1f; }
            s.makeupDb = 0.0f; v.outDb = get ("v0Out"); v.mix = get ("v0Mix") * 0.01f;
            break;
        }
        case 1:   // LE-660: vari-mu - stepped Input Gain, Threshold, six Time Constants; Lat / Vert = mid / side
        {
            static const float ig[6] { -12, -8, -4, 0, 4, 8 };
            static const float tA[6] { 0.2f, 0.2f, 0.4f, 0.4f, 0.4f, 0.2f }, tR[6] { 300, 800, 2000, 5000, 2000, 300 }, sR[6] { 300, 800, 2000, 5000, 10000, 25000 }, sD[6] { 0, 0, 0, 0, 0.6f, 0.5f };
            const int tc = juce::jlimit (0, 5, idx ("v1Tc"));
            v.inDb = ig[juce::jlimit (0, 5, idx ("v1In"))];
            s.curve = VStage::VariMu; s.ratio = 4.0f; s.threshDb = -6.0f - get ("v1Thr") * 3.0f + get ("v1Dc") * 0.8f;
            s.attackMs = tA[tc]; s.releaseMs = tR[tc]; s.slowDepth = sD[tc]; s.slowReleaseMs = sR[tc]; s.slowAttackMs = tc == 5 ? 2000.0f : 1000.0f;
            s.drive = 1.4f; s.bias = 0.15f; s.satBase = 0.18f; s.satPerDb = 0.01f; s.satMax = 0.35f;
            v.outDb = get ("v1Out") - v.inDb; v.mix = get ("v1Mix") * 0.01f; v.ms = idx ("v1Lat") == 1;
            break;
        }
        case 2:   // LE-STA: Input into a fixed threshold, Single / Double / Triple, six releases; thick and velvety
        {
            static const float att[3] { 30, 10, 2.5f }, fr[3] { 1200, 500, 300 }, slr[3] { 5000, 3500, 2200 }, mul[6] { 0.35f, 0.5f, 1.0f, 1.5f, 2.0f, 4.0f };
            const int m = juce::jlimit (0, 2, idx ("v2Mode")); const float rm = mul[juce::jlimit (0, 5, idx ("v2Rel"))];
            v.inDb = get ("v2In"); s.curve = VStage::Sta; s.threshDb = -18.0f;
            s.attackMs = att[m]; s.releaseMs = fr[m] * rm; s.slowDepth = 0.55f; s.slowReleaseMs = slr[m] * rm; s.slowAttackMs = 900.0f;
            s.drive = 1.6f; s.bias = 0.1f; s.satBase = 0.22f; s.satPerDb = 0.014f; s.satMax = 0.5f; s.lowSat = 0.3f; s.topHz = 9000.0f;
            v.outDb = get ("v2Out") - v.inDb * 0.5f; v.mix = get ("v2Mix") * 0.01f;
            break;
        }
        case 3:   // LE-2500: stepped ratio / attack / release, Hard / Med / Soft knee, Thrust, New (feed-forward) or Old (feedback)
        {
            static const float ra[7] { 1.5f, 2, 3, 4, 6, 10, 40 }, kn[3] { 0, 6, 12 }, at[6] { 0.03f, 0.1f, 0.3f, 1, 3, 10 }, re[6] { 50, 100, 200, 500, 1000, 2000 }, th[3] { 0, 3, 6 };
            s.on = on ("v3In"); s.threshDb = get ("v3Thr"); s.ratio = ra[juce::jlimit (0, 6, idx ("v3Ratio"))]; s.kneeDb = kn[juce::jlimit (0, 2, idx ("v3Knee"))];
            s.attackMs = at[juce::jlimit (0, 5, idx ("v3Atk"))]; s.releaseMs = re[juce::jlimit (0, 5, idx ("v3Rel"))] * (get ("v3Var") / 1.4f);
            const int t = juce::jlimit (0, 2, idx ("v3Thrust")); s.tiltDb = th[t]; s.scHpHz = juce::jmax (scHp, t == 0 ? 0.0f : t == 1 ? 120.0f : 200.0f);
            s.feedback = idx ("v3Type") == 1; s.link = get ("v3Link") * 0.01f;
            s.drive = 1.2f; s.bias = 0.05f; s.satBase = 0.04f; s.satPerDb = 0.006f; s.satMax = 0.15f;
            s.makeupDb = get ("v3Mk"); v.mix = get ("v3Mix") * 0.01f;
            break;
        }
        case 4:   // LE-G: the bus glue - VCA, three ratios, stepped attack, release (or Auto), the IN button, Auto Fade
        {
            static const float ra[3] { 2, 4, 10 }, at[6] { 0.1f, 0.3f, 1, 3, 10, 30 }, re[5] { 100, 300, 600, 1200, -1 };
            s.on = on ("v4In"); s.threshDb = get ("v4Thr"); s.ratio = ra[juce::jlimit (0, 2, idx ("v4Ratio"))]; s.kneeDb = 2.0f;
            s.attackMs = at[juce::jlimit (0, 5, idx ("v4Atk"))];
            const float r = re[juce::jlimit (0, 4, idx ("v4Rel"))];
            if (r < 0.0f) { s.releaseMs = 150.0f; s.slowDepth = 0.6f; s.slowReleaseMs = 1200.0f; s.slowAttackMs = 300.0f; } else s.releaseMs = r;
            s.drive = 1.1f; s.bias = 0.03f; s.satBase = 0.02f; s.satPerDb = 0.005f; s.satMax = 0.1f;
            s.makeupDb = get ("v4Mk"); v.mix = get ("v4Mix") * 0.01f; v.autoFade = idx ("v4Fade") == 1;
            break;
        }
        case 5:   // LE-IRON: vari-mu tube mastering - stepped input, Threshold, Compress / Limit, the HP sidechain
        {
            static const float in[7] { -12, -8, -4, 0, 4, 8, 12 }, rc[5] { 200, 400, 600, 4000, 8000 };
            v.inDb = in[juce::jlimit (0, 6, idx ("v5In"))];
            s.curve = VStage::VariMu; s.threshDb = get ("v5Thr"); s.ratio = idx ("v5Mode") == 1 ? 10.0f : 3.0f;
            s.attackMs = get ("v5Atk"); s.releaseMs = rc[juce::jlimit (0, 4, idx ("v5Rec"))];
            s.scHpHz = juce::jmax (scHp, idx ("v5Hp") == 1 ? 100.0f : 0.0f);
            s.drive = 1.5f; s.bias = 0.12f; s.satBase = 0.12f; s.satPerDb = 0.012f; s.satMax = 0.35f; s.topHz = 14000.0f;
            v.outDb = get ("v5Out") - v.inDb; v.mix = get ("v5Mix") * 0.01f;
            break;
        }
        case 6:   // LE-336: diode bridge - stepped ratio, Fast / Slow, recovery (A1 / A2 auto), then its own limiter
        {
            static const float ra[5] { 1.5f, 2, 3, 4, 6 }, rc[6] { 100, 400, 800, 1500, -1, -2 }, lr[5] { 50, 100, 200, -1, -2 };
            s.on = on ("v6CompIn"); s.threshDb = get ("v6Thr"); s.ratio = ra[juce::jlimit (0, 4, idx ("v6Ratio"))]; s.kneeDb = 6.0f;
            s.attackMs = idx ("v6Atk") == 0 ? 3.0f : 10.0f;
            const float r = rc[juce::jlimit (0, 5, idx ("v6Rec"))];
            if (r < 0.0f) { s.releaseMs = r < -1.5f ? 100.0f : 200.0f; s.slowDepth = 0.6f; s.slowReleaseMs = r < -1.5f ? 5000.0f : 1500.0f; } else s.releaseMs = r;
            if (on ("v6LimIn"))
            {
                s.limitDb = -(get ("v6Lim") - 4.0f) * 0.6f - 0.3f;
                const float l = lr[juce::jlimit (0, 4, idx ("v6LimRec"))]; s.limitRelMs = l < 0.0f ? (l < -1.5f ? 800.0f : 300.0f) : l;
            }
            s.drive = 1.3f; s.bias = 0.08f; s.satBase = 0.08f; s.satPerDb = 0.01f; s.satMax = 0.3f; s.lowSat = 0.2f;
            s.makeupDb = get ("v6Gain"); s.on = s.on || on ("v6LimIn"); if (! on ("v6CompIn")) s.ratio = 1.0f;
            break;
        }
        case 7:   // LE-8X: Input (0-10) drives it, ratio buttons to Nuke, Brit mode, detector HP / Band / Link, Dist 2 / Dist 3
        {
            static const float ra[8] { 1, 2, 3, 4, 6, 10, 20, 60 };
            const int ri = juce::jlimit (0, 7, idx ("v7Ratio"));
            s.threshDb = -get ("v7In") * 4.0f; s.ratio = on ("v7Brit") ? 20.0f : ra[ri]; s.kneeDb = ri == 0 ? 12.0f : ri >= 6 ? 1.0f : 5.0f;
            s.attackMs = 0.05f * std::pow (600.0f, get ("v7Atk") / 10.0f); s.releaseMs = 50.0f * std::pow (70.0f, get ("v7Rel") / 10.0f);
            s.scHpHz = on ("v7DetHp") ? 150.0f : scHp; s.bandDb = on ("v7DetBand") ? 6.0f : 0.0f; s.link = on ("v7DetLink") ? 1.0f : 0.0f;
            const int au = idx ("v7Audio");
            s.drive = au == 1 ? 2.2f : 1.6f; s.bias = au == 0 ? 0.15f : au == 1 ? 0.0f : 0.05f;
            s.satBase = au == 2 ? 0.02f : 0.12f; s.satPerDb = 0.015f; s.satMax = 0.5f;
            if (au == 2) s.scHpHz = juce::jmax (s.scHpHz, 80.0f);
            v.outDb = (get ("v7Out") - 5.0f) * 3.0f + get ("v7In") * 1.5f; v.mix = get ("v7Mix") * 0.01f;
            break;
        }
        case 8:   // LE-160: Threshold, Compression, OverEasy, Output - RMS, program dependent
        {
            s.threshDb = get ("v8Thr") * 0.75f - 8.0f; s.ratio = get ("v8Ratio"); s.kneeDb = on ("v8Easy") ? 14.0f : 0.5f;
            s.rms = true; s.attackMs = 6.0f; s.releaseMs = 120.0f; s.slowDepth = 0.4f; s.slowReleaseMs = 600.0f;
            s.drive = 1.2f; s.bias = 0.05f; s.satBase = 0.04f; s.satPerDb = 0.008f; s.satMax = 0.2f;
            v.outDb = get ("v8Out"); v.mix = get ("v8Mix") * 0.01f;
            break;
        }
        case 9:   // LE-7: tape-like - Input drives warmth and squash, Warmth softens the top, Tranny, Buss / G.P. / Track, Spank
        {
            static const float ra[3] { 2, 3, 6 }, at[3] { 30, 10, 3 }, re[3] { 400, 200, 80 };
            const int m = juce::jlimit (0, 2, idx ("v9Mode"));
            const float in = get ("v9In"), wm = get ("v9Warm");
            s.threshDb = -6.0f - in * 2.5f; s.ratio = ra[m]; s.kneeDb = 8.0f; s.attackMs = at[m]; s.releaseMs = re[m]; s.rms = m == 0;
            s.drive = 1.4f + in * 0.15f; s.bias = 0.12f; s.satBase = 0.1f + in * 0.03f; s.satPerDb = 0.01f; s.satMax = 0.6f;
            s.topHz = 20000.0f / (1.0f + wm * 0.35f); s.lowSat = on ("v9Tranny") ? 0.5f : 0.0f;
            if (on ("v9Spank")) { s.limitDb = -1.0f; s.limitRelMs = 30.0f; }
            v.outDb = (get ("v9Out") - 5.0f) * 3.0f; v.mix = get ("v9Mix") * 0.01f;
            break;
        }
        case 10:  // LE-760: stepped threshold / ratio, attack toggle, releases + Auto, the limiter (or pre-emphasis), expander / gate
        {
            static const float th[6] { -20, -16, -12, -8, -4, 0 }, ra[6] { 1, 2, 3, 5, 10, 20 }, at[3] { 0.25f, 2.5f, 25 }, re[6] { 25, 100, 400, 1600, 3200, -1 };
            s.threshDb = th[juce::jlimit (0, 5, idx ("v10Thr"))]; s.ratio = ra[juce::jlimit (0, 5, idx ("v10Ratio"))]; s.kneeDb = 3.0f;
            s.attackMs = at[juce::jlimit (0, 2, idx ("v10Atk"))];
            const float r = re[juce::jlimit (0, 5, idx ("v10Rel"))];
            if (r < 0.0f) { s.releaseMs = 120.0f; s.slowDepth = 0.6f; s.slowReleaseMs = 1600.0f; } else s.releaseMs = r;
            const int lm = idx ("v10Lim"); if (lm > 0) { s.limitDb = -0.5f; s.limitRelMs = 60.0f; if (lm == 2) s.tiltDb = 6.0f; }
            const int ex = idx ("v10Exp"); if (ex > 0) { s.expThrDb = -50.0f; s.expRatio = ex == 1 ? 2.0f : 20.0f; }
            s.drive = 1.15f; s.bias = 0.04f; s.satBase = 0.03f; s.satPerDb = 0.006f; s.satMax = 0.15f;
            v.outDb = get ("v10Out"); v.mix = get ("v10Mix") * 0.01f;
            break;
        }
        case 11:  // LE-SHM: an optical stage, then a discrete one, through a choice of transformers
        {
            static const float dr[6] { 1.2f, 1.5f, 2, 3, 4, 6 }, da[6] { 0.5f, 1, 3, 8, 20, 40 }, dc[6] { 100, 250, 400, 800, 1200, -1 };
            static const float tl[3] { 0.15f, 0.4f, 0.6f }, tt[3] { 0, 15000, 11000 };
            auto& o = v.st[0]; auto& d = v.st[1]; v.stages = 2; d = o;
            const int tr = juce::jlimit (0, 2, idx ("v11Trans"));
            const float scf = idx ("v11ScF") == 1 ? 90.0f : scHp;
            o.on = on ("v11OptoIn"); o.curve = VStage::Opto; o.ratio = 3.0f; o.threshDb = -get ("v11OThr") * 3.0f;
            o.attackMs = 10.0f; o.releaseMs = 120.0f; o.slowDepth = 0.6f; o.slowReleaseMs = 2000.0f; o.scHpHz = scf; o.makeupDb = get ("v11OGain");
            d.on = on ("v11DiscIn"); d.curve = VStage::Knee; d.threshDb = -get ("v11DThr") * 3.0f; d.ratio = dr[juce::jlimit (0, 5, idx ("v11DRatio"))]; d.kneeDb = 4.0f;
            d.attackMs = da[juce::jlimit (0, 5, idx ("v11DAtk"))];
            const float r = dc[juce::jlimit (0, 5, idx ("v11DRec"))];
            if (r < 0.0f) { d.releaseMs = 100.0f; d.slowDepth = 0.6f; d.slowReleaseMs = 1200.0f; } else d.releaseMs = r;
            d.scHpHz = scf; d.makeupDb = get ("v11DGain");
            d.drive = 1.3f; d.bias = 0.08f; d.satBase = 0.04f; d.satPerDb = 0.008f; d.satMax = 0.25f; d.lowSat = tl[tr]; d.topHz = tt[tr];
            v.mix = get ("v11Mix") * 0.01f;
            break;
        }
        case 12:  // LE-2A: optical - Peak Reduction and Gain, Compress / Limit; slow, program-dependent release
        {
            s.curve = VStage::Opto; s.ratio = idx ("v12Mode") == 1 ? 12.0f : 3.0f; s.threshDb = -get ("v12Peak") * 0.5f;
            s.attackMs = 10.0f; s.releaseMs = 60.0f; s.slowDepth = 0.7f; s.slowAttackMs = 400.0f; s.slowReleaseMs = 2500.0f; s.rms = true;
            s.drive = 1.4f; s.bias = 0.15f; s.satBase = 0.12f; s.satPerDb = 0.01f; s.satMax = 0.35f; s.topHz = 16000.0f;
            v.outDb = (get ("v12Gain") - 50.0f) * 0.4f; v.mix = get ("v12Mix") * 0.01f;
            break;
        }
        case 13:  // LE-1B: optical tube - Threshold, Ratio, Fixed / Manual / Fixed + Manual timing, Attack, Release, Gain
        {
            const int tm = juce::jlimit (0, 2, idx ("v13Timing"));
            s.curve = VStage::Opto; s.threshDb = get ("v13Thr"); s.ratio = get ("v13Ratio");
            s.attackMs = tm == 0 ? 15.0f : get ("v13Atk"); s.releaseMs = tm == 0 ? 400.0f : get ("v13Rel");
            if (tm != 1) { s.slowDepth = 0.6f; s.slowReleaseMs = 2500.0f; }
            s.drive = 1.4f; s.bias = 0.14f; s.satBase = 0.1f; s.satPerDb = 0.01f; s.satMax = 0.3f;
            s.makeupDb = get ("v13Gain"); v.mix = get ("v13Mix") * 0.01f;
            break;
        }
        default:  // LE-VSC: discrete VCA - threshold (dBu), Soft / 2-10 / Brick, Peak / RMS, stepped attack, release + Auto Fast / Slow
        {
            static const float ra[8] { 2, 2, 3, 4, 6, 8, 10, 40 }, at[6] { 0.1f, 0.3f, 1, 3, 10, 40 }, re[6] { 50, 100, 300, 1200, -1, -2 };
            const int ri = juce::jlimit (0, 7, idx ("v14Ratio"));
            s.threshDb = get ("v14Thr") - 18.0f; s.ratio = ra[ri]; s.kneeDb = ri == 0 ? 15.0f : ri == 7 ? 0.0f : 4.0f;
            s.rms = idx ("v14Det") == 1; s.attackMs = at[juce::jlimit (0, 5, idx ("v14Atk"))];
            const float r = re[juce::jlimit (0, 5, idx ("v14Rel"))];
            if (r < 0.0f) { s.releaseMs = r < -1.5f ? 300.0f : 100.0f; s.slowDepth = 0.6f; s.slowReleaseMs = r < -1.5f ? 2500.0f : 900.0f; } else s.releaseMs = r;
            s.link = on ("v14Stereo") ? 0.0f : s.link; s.scHpHz = juce::jmax (scHp, idx ("v14ScF") == 1 ? 60.0f : 0.0f);
            s.drive = 1.05f; s.bias = 0.02f; s.satBase = 0.01f; s.satPerDb = 0.004f; s.satMax = 0.08f;
            s.makeupDb = get ("v14Mk"); v.mix = get ("v14Mix") * 0.01f; v.bypass = on ("v14Byp");
            break;
        }
    }
    return v;
}
}
