#pragma once

// Mishkal Tahor - the eleven vintage units of CHARACTER: names, faceplates (straight from the approved Claude Design
// canvas), knobs, and how each knob drives the Character EQ (vox::VocalEQ).

#include <juce_core/juce_core.h>
#include "Dsp/VocalChain.h"

namespace tahor
{

inline constexpr int numUnits = 16;   // V9: + LE-Esh, LE-Shemen, LE-Kav, LE-Even, LE-Kavod
inline constexpr int unitValues = 11;   // values per unit (u<U>v<j>), all 0..1
inline constexpr int maxChain = 4;

enum class KnobKind { Chicken, Console, Neve, Stack, Skirt, Knurl };

struct KnobDef { const char* label; int v; int v2 = -1; };   // v2 = the outer ring of a stacked knob

struct UnitDef
{
    const char* name; const char* desc; float wear;
    const char* bg[3]; const char* line; const char* ink; const char* sub; const char* led; const char* col;
    KnobKind kind; const char* cap[2];          // cap gradient (inner, outer) for one-colour caps
    const char* caps[6];                         // per-knob cap colours (console / neve), nullptr otherwise
    const char* outer[2];                        // stacked: the outer ring's two colours
    const char* tick;
    std::vector<KnobDef> knobs;
    std::vector<float> defaults;                 // unitValues entries
    vox::EqType type;
    std::vector<const char*> switches;           // LE-550B: LO SHELF, HI SHELF, IN (values 8, 9, 10)
};

namespace detail
{
    inline int idx (float v, int n) { return juce::jlimit (0, n - 1, (int) std::round (v * (float) (n - 1))); }
    inline float pick (float v, std::initializer_list<float> l) { return *(l.begin() + idx (v, (int) l.size())); }
    inline float bip (float v, float range) { return (v * 2.0f - 1.0f) * range; }           // 0..1 -> -range..+range
    inline float logHz (float v, float lo, float hi) { return lo * std::pow (hi / lo, v); }
    inline float pos (float x, std::initializer_list<float> l)                            // value -> knob position
    { int i = 0, best = 0; for (auto f : l) { if (std::abs (f - x) < std::abs (*(l.begin() + best) - x)) best = i; ++i; } return (float) best / (float) (l.size() - 1); }
    inline float bpos (float db, float range) { return (db / range + 1.0f) * 0.5f; }
}

inline const std::vector<UnitDef>& units()
{
    using namespace detail;
    static const std::vector<UnitDef> u {
        { "LE-EP1", "Tube program EQ", 1.0f, { "#3b3430", "#40382f", "#372f29" }, "#5a4e40", "#efe2c4", "#c2b28e", "#ffb547", "#f3a93c",
          KnobKind::Chicken, {}, {}, {}, "#f3e8cf",
          { { "LOW FREQ", 0 }, { "BOOST", 1 }, { "ATTEN", 2 }, { "HIGH FREQ", 3 }, { "BOOST", 4 }, { "WIDTH", 5 } },
          { 2.0f / 3.0f, 0, 0, 4.0f / 6.0f, 0, 0.7f, 0.5f, 0, 0, 0, 0 }, vox::EqType::TubeP1, {} },   // V11: + atten select (6), high atten (7)
        { "LE-ME5", "Tube midrange EQ", 0.95f, { "#332c27", "#382f29", "#2f2823" }, "#54473a", "#efe2c4", "#c2b28e", "#ff6fae", "#ff6fae",
          KnobKind::Chicken, {}, {}, {}, "#f3e8cf",
          { { "LOW PEAK", 0 }, { "BOOST", 1 }, { "DIP FREQ", 2 }, { "DIP", 3 }, { "HIGH PEAK", 4 }, { "BOOST", 5 } },
          { 0.0f, 0, 0.4f, 0, 0.6f, 0, 0, 0, 0, 0, 0 }, vox::EqType::Me5, {} },
        { "LE-E4K", "British E-series console", 0.5f, { "#2b3034", "#30363b", "#283034" }, "#4a535a", "#e7ecef", "#a7b2b9", "#ff5a4f", "#ff5a4f",
          KnobKind::Console, {}, { "#cfd3d6", "#c0392b", "#2e8b3e", "#1f5fbf", "#a0522d" }, {}, "#eef2f4",
          { { "HPF", 0 }, { "LF", 1 }, { "LMF", 2 }, { "HMF", 3 }, { "HF", 4 } },
          { 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0, 0, 0, 0, 0, 0 }, vox::EqType::EConsole, {} },
        { "LE-N73", "British console preamp EQ", 0.8f, { "#6d7b8a", "#748392", "#677584" }, "#93a1b0", "#f3f6f9", "#dde4eb", "#ff3b30", "#ff8a3d",
          KnobKind::Stack, { "#9aa0a6", "#4a4f55" }, {}, { "#a52a22", "#cf463d" }, "#f7f9fb",
          { { "HPF", 0 }, { "LOW", 1, 2 }, { "MID", 3, 4 }, { "HIGH", 5, 6 } },
          { 0.0f, 0.5f, 2.0f / 3.0f, 0.5f, 0.6f, 0.5f, 0.5f, 0, 0, 0, 0 }, vox::EqType::N73, {} },
        { "LE-N81", "Class A console, 81-style", 0.75f, { "#3a4552", "#3f4a58", "#36414d" }, "#5b6878", "#eef2f6", "#b3c0ce", "#ff3b30", "#4f8cff",
          KnobKind::Neve, {}, { "#cfd3d6", "#c0392b", "#1f5fbf", "#1f5fbf", "#c0392b" }, {}, "#f2f5f8",
          { { "HPF", 0 }, { "LOW", 1 }, { "MID FREQ", 2 }, { "MID", 3 }, { "HIGH", 4 } },
          { 0.0f, 0.5f, 0.4f, 0.5f, 0.5f, 0, 0, 0, 0, 0, 0 }, vox::EqType::ClassA81, {} },
        { "LE-550B", "Stepped American EQ", 0.55f, { "#8f9297", "#989ba0", "#888b90" }, "#b8bbbf", "#121314", "#2c2f33", "#3d63ff", "#3d63ff",
          KnobKind::Stack, { "#3a3a3a", "#050505" }, {}, { "#7d8287", "#b3b8bd" }, "#0e0f10",
          { { "LOW", 0, 1 }, { "LOW-MID", 2, 3 }, { "HI-MID", 4, 5 }, { "HIGH", 6, 7 } },
          { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 2.0f / 6.0f, 0.5f, 0.5f, 1, 1, 1 }, vox::EqType::Api550,
          { "LO SHELF", "HI SHELF", "IN" } },
        { "LE-K9", "Console channel EQ, 9000K-style", 0.3f, { "#5a5e63", "#5f6368", "#555a5e" }, "#7a7f85", "#f2f3f4", "#d0d3d6", "#4ade80", "#cfd3d8",
          KnobKind::Skirt, { "#d8dcdf", "#6d7276" }, {}, {}, "#f6f7f8",
          { { "HPF", 0 }, { "LF", 1 }, { "LMF", 2 }, { "HMF", 3 }, { "HF", 4 }, { "LPF", 5 } },
          { 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0, 0, 0, 0, 0 }, vox::EqType::K9000, {} },
        { "LE-H69", "Helios 69-style", 0.8f, { "#25303f", "#2a3545", "#222c3a" }, "#46566c", "#e8eef6", "#a9b9cd", "#a78bfa", "#a78bfa",
          KnobKind::Knurl, { "#d8cdb4", "" }, {}, {}, "#eef3f9",
          { { "BASS", 0 }, { "MID FREQ", 1 }, { "MID", 2 }, { "TREBLE", 3 } },
          { 0.5f, 0.6f, 0.5f, 0.5f, 0, 0, 0, 0, 0, 0, 0 }, vox::EqType::Helios69, {} },
        { "LE-SP1", "Air EQ", 0.08f, { "#b9bcc0", "#c2c5c9", "#b3b6ba" }, "#dadcde", "#1d2023", "#3d4247", "#46b7c9", "#46b7c9",
          KnobKind::Knurl, { "#9ea3a8", "" }, {}, {}, "#15181b",
          { { "LOW", 0 }, { "MID", 1 }, { "AIR FREQ", 2 }, { "AIR", 3 } },
          { 0.5f, 0.5f, 0.75f, 0, 0, 0, 0, 0, 0, 0, 0 }, vox::EqType::Airy, {} },
        { "LE-M4", "Broad bands + Air, EQ4-style", 0.1f, { "#1f2226", "#24272b", "#1c1f22" }, "#3e434a", "#ece6da", "#b3a68e", "#e6c46a", "#e6c46a",
          KnobKind::Knurl, { "#c9a54a", "" }, {}, {}, "#f2ece0",
          { { "SUB", 0 }, { "40", 1 }, { "160", 2 }, { "650", 3 }, { "2.5K", 4 }, { "AIR", 5 } },
          { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0, 0, 0, 0, 0, 0 }, vox::EqType::Maag4, {} },
        { "LE-MP2", "Passive tube mastering EQ", 0.18f, { "#17181a", "#1b1c1f", "#141517" }, "#3a3c40", "#e9ecef", "#a8adb3", "#7ad08a", "#7ad08a",
          KnobKind::Skirt, { "#e9ecef", "#8a9097" }, {}, {}, "#e9ecef",
          { { "LOW", 0 }, { "GAIN", 1 }, { "LO-MID", 2 }, { "HI-MID", 3 }, { "HIGH", 4 }, { "GAIN", 5 } },
          { 0.5f, 0.5f, 0.5f, 0.5f, 4.0f / 6.0f, 0.5f, 0, 0, 0, 0, 0 }, vox::EqType::Mp2, {} },
        // V9: five more
        { "LE-Esh", "Tube EQ, HG-Q-style", 0.6f, { "#2a1d17", "#2f211a", "#261a14" }, "#5a3a2a", "#f2e6d8", "#c9b29a", "#ff7a2f", "#ff7a2f",
          KnobKind::Knurl, { "#b8b2a8", "" }, {}, { "#8a3a18", "#c4562a" }, "#f2e6d8",
          { { "LOW", 0 }, { "BOOST", 1 }, { "CUT", 2 }, { "MID", 4, 3 }, { "HIGH", 5 }, { "BOOST", 6 }, { "CUT", 7 } },
          { 1.0f / 3.0f, 0, 0, 0.5f, 0.5f, 0.5f, 0, 0, 0, 0, 0 }, vox::EqType::Esh, {} },
        { "LE-Shemen", "Mastering parametric, 432C-style", 0.35f, { "#2c3540", "#313b47", "#29323c" }, "#4d5b6b", "#eef2f6", "#aebccb", "#7fd3ff", "#7fb8ff",
          KnobKind::Skirt, { "#e3e6e9", "#7d848b" }, {}, {}, "#eef2f6",
          { { "LOW FREQ", 0 }, { "LOW", 1 }, { "MID FREQ", 2 }, { "MID", 3 }, { "WIDTH", 4 }, { "HIGH FREQ", 5 }, { "HIGH", 6 } },
          { 0.45f, 0.5f, 0.5f, 0.5f, 0.4f, 0.55f, 0.5f, 0, 0, 0, 0 }, vox::EqType::Shemen, {} },
        { "LE-Kav", "Precision mastering EQ, PQ-style", 0.1f, { "#141517", "#18191c", "#121315" }, "#33363b", "#e9ecef", "#9aa1a8", "#ffd24a", "#ffd24a",
          KnobKind::Stack, { "#cfd3d6", "#5a5f64" }, {}, { "#2f3237", "#5b6066" }, "#e9ecef",
          { { "LOW", 1, 0 }, { "LO-MID", 3, 2 }, { "MID", 5, 4 }, { "HI-MID", 7, 6 }, { "HIGH", 9, 8 }, { "Q", 10 } },
          { 0.4f, 0.5f, 0.45f, 0.5f, 0.5f, 0.5f, 0.55f, 0.5f, 0.6f, 0.5f, 0.4f }, vox::EqType::Kav, {} },
        { "LE-Even", "Passive EQ, Passeq-style", 0.4f, { "#3c3a38", "#42403d", "#383634" }, "#5f5b56", "#f1ece4", "#bdb4a6", "#e05a3a", "#d9825b",
          KnobKind::Skirt, { "#2b2b2b", "#9a9088" }, {}, { "#7d7368", "#a89a8a" }, "#f1ece4",
          { { "LO BOOST", 1, 0 }, { "LO CUT", 3, 2 }, { "HI BOOST", 5, 4 }, { "HI CUT", 7, 6 } },
          { 0.5f, 0, 0.5f, 0, 0.5f, 0, 0.5f, 0, 0, 0, 0 }, vox::EqType::Even, {} },
        { "LE-Kavod", "Passive tube mastering EQ, 825Q-style", 0.3f, { "#b8a77a", "#c2b285", "#ae9e72" }, "#d6c79e", "#1e1a12", "#4a412e", "#ffcf4a", "#e8c35a",
          KnobKind::Chicken, { "#2a2622", "#0a0806" }, {}, { "#5a4a2a", "#8a7244" }, "#1e1a12",
          { { "LOW", 1, 0 }, { "LO-MID", 3, 2 }, { "MID", 5, 4 }, { "HI-MID", 7, 6 }, { "HIGH", 9, 8 } },
          { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0 }, vox::EqType::Kavod, {} },
    };
    return u;
}

// V10: the big tube and mastering units are two rack units tall (the extra unit on top)
inline bool unit2U (int u) { return u == 0 || u == 10 || u == 11 || u == 14 || u == 15; }
inline float unitPlateH (int u) { return unit2U (u) ? 176.0f : 92.0f; }

// V11: the two-unit gear lays its controls out like the originals - a row of big gain knobs over a row of
// frequency selectors (Pultec: BOOST / ATTEN / BANDWIDTH / BOOST / ATTEN over CPS / KCS / ATTEN SEL)
struct PanelKnob { const char* label; int v; int row; float col; };
inline const std::vector<PanelKnob>& unitLayout2U (int u)
{
    static const std::vector<PanelKnob> none;
    static const std::vector<std::vector<PanelKnob>> L {
        { { "BOOST", 1, 0, 0 }, { "ATTEN", 2, 0, 1 }, { "BANDWIDTH", 5, 0, 2 }, { "BOOST", 4, 0, 3 }, { "ATTEN", 7, 0, 4 },
          { "LOW FREQ CPS", 0, 1, 0.5f }, { "HIGH FREQ KCS", 3, 1, 2.5f }, { "ATTEN SEL KCS", 6, 1, 4 } },                    // LE-EP1
        { { "LOW", 1, 0, 0 }, { "LO-MID", 2, 0, 1 }, { "HI-MID", 3, 0, 2 }, { "HIGH", 5, 0, 3 },
          { "LOW FREQ", 0, 1, 0 }, { "HIGH FREQ", 4, 1, 3 } },                                                               // LE-MP2
        { { "LOW BOOST", 1, 0, 0 }, { "LOW CUT", 2, 0, 1 }, { "MID", 4, 0, 2 }, { "HIGH BOOST", 6, 0, 3 }, { "HIGH CUT", 7, 0, 4 },
          { "LOW FREQ", 0, 1, 0.5f }, { "MID FREQ", 3, 1, 2 }, { "HIGH FREQ", 5, 1, 3.5f } },                                 // LE-Esh
        { { "LO BOOST", 1, 0, 0 }, { "LO CUT", 3, 0, 1 }, { "HI BOOST", 5, 0, 2 }, { "HI CUT", 7, 0, 3 },
          { "FREQ", 0, 1, 0 }, { "FREQ", 2, 1, 1 }, { "FREQ", 4, 1, 2 }, { "FREQ", 6, 1, 3 } },                              // LE-Even
        { { "LOW", 1, 0, 0 }, { "LO-MID", 3, 0, 1 }, { "MID", 5, 0, 2 }, { "HI-MID", 7, 0, 3 }, { "HIGH", 9, 0, 4 },
          { "FREQ", 0, 1, 0 }, { "FREQ", 2, 1, 1 }, { "FREQ", 4, 1, 2 }, { "FREQ", 6, 1, 3 }, { "FREQ", 8, 1, 4 } },           // LE-Kavod
    };
    switch (u) { case 0: return L[0]; case 10: return L[1]; case 11: return L[2]; case 14: return L[3]; case 15: return L[4]; default: return none; }
}

// V9: a one-word hint for each unit, shown just above its faceplate
inline const char* unitHint (int unit)
{
    static const char* h[numUnits] { "Weight", "Mids", "Bite", "Warmth", "Girth", "Punch", "Clean", "Grit", "Air", "Open", "Gloss",
                                     "Fire", "Silk", "Precision", "Foundation", "Elegance" };
    return h[juce::jlimit (0, numUnits - 1, unit)];
}

// lists used by the stepped knobs
namespace lists
{
    inline const std::initializer_list<float> ep1Low { 20, 30, 60, 100 }, ep1High { 3000, 4000, 5000, 8000, 10000, 12000, 16000 };
    inline const std::initializer_list<float> me5Low { 200, 300, 500, 700, 1000 }, me5Dip { 200, 300, 500, 700, 1000, 1500, 2000, 3000, 4000, 5000, 7000 },
                                              me5High { 1500, 2000, 3000, 3500, 4000, 5000 };
    inline const std::initializer_list<float> n73Hpf { 0, 50, 80, 160, 300 }, n73Low { 35, 60, 110, 220 }, n73Mid { 360, 700, 1600, 3200, 4800, 7200 },
                                              n73High { 10000, 12000, 16000 };
    inline const std::initializer_list<float> n81Hpf { 0, 45, 70, 80, 160, 360 }, n81Mid { 360, 700, 1600, 3200, 4800, 7200 };
    inline const std::initializer_list<float> a550b1 { 30, 40, 50, 100, 200, 300, 400 }, a550b2 { 75, 150, 240, 400, 500, 700, 1000 },
                                              a550b3 { 800, 1500, 3000, 5000, 8000, 10000, 12500 }, a550b4 { 2500, 5000, 7000, 10000, 12500, 15000, 20000 };
    inline const std::initializer_list<float> h69Mid { 700, 1000, 1500, 2000, 3000, 5000 };
    inline const std::initializer_list<float> sp1Air { 2500, 5000, 10000, 20000, 40000 };
    inline const std::initializer_list<float> mp2Low { 22, 33, 47, 68, 100, 150, 220 }, mp2High { 2200, 3300, 4700, 6800, 8200, 12000, 16000 };
    // V9
    inline const std::initializer_list<float> eshLow { 30, 60, 100, 200 }, eshMid { 300, 500, 800, 1200, 2000, 3000, 5000 }, eshHigh { 3000, 5000, 8000, 10000, 12000, 16000 };
    inline const std::initializer_list<float> evenLB { 10, 22, 47, 68, 100, 150, 220 }, evenLC { 22, 47, 100, 220, 470, 1000 },
                                              evenHB { 1500, 2200, 3300, 4700, 6800, 10000, 15000 }, evenHC { 2200, 4700, 6800, 10000, 15000, 22000 };
    inline const std::initializer_list<float> kvLow { 20, 30, 60, 100, 150 }, kvLm { 150, 250, 400, 600, 900 }, kvMid { 900, 1500, 2200, 3300, 4700 },
                                              kvHm { 3300, 4700, 6800, 8200, 10000 }, kvHigh { 6800, 10000, 12000, 16000, 20000 };
}
// V9: a continuous frequency knob (the same mapping the nodes use)
inline float contHz (float a, float lo, float hi) { return a < 0.02f ? lo * 0.7f : detail::logHz ((a - 0.02f) / 0.98f, lo, hi); }

inline juce::String hzText (float hz, bool unit = true)
{
    if (hz >= 1000.0f)
    {
        const float k = hz / 1000.0f;
        auto s = std::abs (k - std::round (k)) < 0.05f ? juce::String ((int) std::round (k)) : juce::String (k, 1);
        return s + (unit ? " kHz" : "k");
    }
    return juce::String ((int) std::round (hz)) + (unit ? " Hz" : "");
}
inline juce::String num (float x, int dec = 1)
{
    if (std::abs (x) < 0.05f) return "0";
    auto s = std::abs (x - std::round (x)) < 0.05f ? juce::String ((int) std::round (std::abs (x))) : juce::String (std::abs (x), dec);
    return (x < 0 ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) : juce::String ("+")) + s;
}
inline juce::String dial (float x) { auto s = std::abs (x - std::round (x)) < 0.05f ? juce::String ((int) std::round (x)) : juce::String (x, 1); return s; }

// the 550B / N73 gain steps
inline float step550 (float v) { return (float) (detail::idx (v, 13) - 6) * 2.0f; }

// knob value text, exactly as the faceplate prints it
inline juce::String unitValueText (int unit, int knob, const float* v)
{
    using namespace detail; using namespace lists;
    const auto& d = units()[(size_t) unit].knobs[(size_t) knob];
    const float a = v[d.v];
    const float b = d.v2 >= 0 ? v[d.v2] : 0.0f;
    const auto db = [] (float x) { return num (x) + " dB"; };
    switch (unit)
    {
        case 0: { if (knob == 0) return hzText (pick (a, ep1Low)); if (knob == 3) return hzText (pick (a, ep1High)); return dial (a * 10.0f); }
        case 1: { if (knob == 0) return hzText (pick (a, me5Low)); if (knob == 2) return hzText (pick (a, me5Dip)); if (knob == 4) return hzText (pick (a, me5High)); return dial (a * 10.0f); }
        case 2: { if (knob == 0) return a < 0.02f ? juce::String ("off") : hzText (logHz ((a - 0.02f) / 0.98f, 16, 350)); return db (bip (a, 15)); }
        case 3: { if (knob == 0) { const float h = pick (a, n73Hpf); return h <= 0 ? juce::String ("off") : hzText (h); }
                  const auto& l = knob == 1 ? n73Low : knob == 2 ? n73Mid : n73High;
                  return hzText (pick (b, l), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, knob == 2 ? 18.0f : 16.0f)); }
        case 4: { if (knob == 0) { const float h = pick (a, n81Hpf); return h <= 0 ? juce::String ("off") : hzText (h); }
                  if (knob == 2) return hzText (pick (a, n81Mid)); return db (bip (a, knob == 3 ? 18.0f : 16.0f)); }
        case 5: { const auto& l = knob == 0 ? a550b1 : knob == 1 ? a550b2 : knob == 2 ? a550b3 : a550b4;
                  return hzText (pick (b, l), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (step550 (a)); }
        case 6: { if (knob == 0) return a < 0.02f ? juce::String ("off") : hzText (logHz ((a - 0.02f) / 0.98f, 16, 350));
                  if (knob == 5) return a < 0.02f ? juce::String ("off") : hzText (logHz ((a - 0.02f) / 0.98f, 22000, 3000));
                  return db (bip (a, 15)); }
        case 7: { if (knob == 1) return hzText (pick (a, h69Mid)); return num (bip (a, 10)); }
        case 8: { if (knob == 2) return hzText (pick (a, sp1Air)); if (knob == 3) return db (a * 12.0f); return db (bip (a, 12)); }
        case 9: { if (knob == 5) return num (a * 15.0f); return num (bip (a, 10)); }
        case 10: { if (knob == 0) return hzText (pick (a, mp2Low)); if (knob == 4) return hzText (pick (a, mp2High), false);
                   if (knob == 2) return juce::String ("470") + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, 6));
                   if (knob == 3) return juce::String ("3.3k") + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, 6));
                   return num (bip (a, 12)); }
        case 11: { if (knob == 0) return hzText (pick (a, eshLow)); if (knob == 4) return hzText (pick (a, eshHigh));
                   if (knob == 3) return hzText (pick (b, eshMid), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, 10));
                   return dial (a * 10.0f); }
        case 12: { if (knob == 0) return hzText (contHz (a, 20, 500)); if (knob == 2) return hzText (contHz (a, 200, 8000)); if (knob == 5) return hzText (contHz (a, 1500, 20000));
                   if (knob == 4) return "Q " + juce::String (0.4f + a * 2.6f, 1); return db (bip (a, 16)); }
        case 13: { if (knob == 5) return "Q " + juce::String (0.4f + a * 3.6f, 1);
                   static const float lo[5] { 20, 60, 300, 1500, 3000 }, hi[5] { 400, 1200, 6000, 16000, 20000 };
                   return hzText (contHz (b, lo[knob], hi[knob]), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, 16)); }
        case 14: { const auto& l = knob == 0 ? evenLB : knob == 1 ? evenLC : knob == 2 ? evenHB : evenHC;
                   return hzText (pick (b, l), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + dial (a * 10.0f); }
        case 15: { const auto& l = knob == 0 ? kvLow : knob == 1 ? kvLm : knob == 2 ? kvMid : knob == 3 ? kvHm : kvHigh;
                   return hzText (pick (b, l), false) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + num (bip (a, knob == 0 || knob == 4 ? 10.0f : 8.0f)); }
        default: break;
    }
    return {};
}

// V11: the text for one value (the two-unit layouts show each value on its own knob)
inline juce::String valueTextFor (int unit, int vi, const float* v)
{
    if (unit == 0 && vi == 6) return hzText (detail::pick (v[6], { 5000, 10000, 20000 }));
    if (unit == 0 && vi == 7) return dial (v[7] * 10.0f);
    const auto& U = units()[(size_t) unit];
    for (int k = 0; k < (int) U.knobs.size(); ++k)
    {
        const auto& d = U.knobs[(size_t) k];
        if (d.v != vi && d.v2 != vi) continue;
        const auto t = unitValueText (unit, k, v);
        if (d.v2 < 0) return t;
        const auto dot = juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 "));
        return d.v2 == vi ? t.upToFirstOccurrenceOf (dot, false, false) : t.fromFirstOccurrenceOf (dot, false, false);
    }
    return {};
}

// a unit's values -> the Character EQ's settings
inline vox::CharEqSettings unitSettings (int unit, const float* v)
{
    using namespace detail; using namespace lists;
    vox::CharEqSettings c;
    c.on = true;
    switch (unit)
    {
        case 0:
            c.p1LowHz = pick (v[0], ep1Low); c.p1LowBoost = v[1] * 10.0f; c.p1LowAtten = v[2] * 10.0f;
            c.p1HighHz = pick (v[3], ep1High); c.p1HighBoost = v[4] * 10.0f; c.p1HighBw = v[5] * 10.0f;
            c.p1AttenHz = pick (v[6], { 5000, 10000, 20000 }); c.p1HighAtten = v[7] * 10.0f;   // V11: the real EQP-1A's high attenuation
            break;
        case 1:
            c.xHz[0] = pick (v[0], me5Low); c.xDb[0] = v[1] * 10.0f;
            c.xHz[1] = pick (v[2], me5Dip); c.xDb[1] = -v[3] * 10.0f;
            c.xHz[2] = pick (v[4], me5High); c.xDb[2] = v[5] * 10.0f;
            break;
        case 2:
            c.hpfHz = v[0] < 0.02f ? 5.0f : logHz ((v[0] - 0.02f) / 0.98f, 16, 350);
            c.bodyHz = 100.0f; c.bodyDb = bip (v[1], 15);
            c.mudHz = 400.0f; c.mudQ = 0.7f; c.mudDb = bip (v[2], 15);
            c.presHz = 2500.0f; c.presQ = 0.7f; c.presDb = bip (v[3], 15);
            c.airHz = 8000.0f; c.airDb = bip (v[4], 15);
            break;
        case 3:
            c.xHpfHz = pick (v[0], n73Hpf);
            c.xHz[0] = pick (v[2], n73Low); c.xDb[0] = bip (v[1], 16);
            c.xHz[1] = pick (v[4], n73Mid); c.xDb[1] = bip (v[3], 18);
            c.xDb[2] = bip (v[5], 16);   // the 12 kHz shelf (its frequency is fixed in the design; the ring picks 10 / 12 / 16 kHz)
            c.xHz[2] = pick (v[6], n73High);
            break;
        case 4:
        {
            const float h = pick (v[0], n81Hpf);
            c.hpfHz = h <= 0 ? 5.0f : h;
            c.bodyHz = 110.0f; c.bodyDb = bip (v[1], 16);
            c.mudDb = 0.0f;
            c.presHz = pick (v[2], n81Mid); c.presDb = bip (v[3], 18); c.presQ = 1.0f;
            c.airHz = 12000.0f; c.airDb = bip (v[4], 16);
            break;
        }
        case 5:
            c.xDb[0] = step550 (v[0]); c.xHz[0] = pick (v[1], a550b1);
            c.xDb[1] = step550 (v[2]); c.xHz[1] = pick (v[3], a550b2);
            c.xDb[2] = step550 (v[4]); c.xHz[2] = pick (v[5], a550b3);
            c.xDb[3] = step550 (v[6]); c.xHz[3] = pick (v[7], a550b4);
            c.xShape[0] = v[8] > 0.5f ? 0 : 1; c.xShape[3] = v[9] > 0.5f ? 0 : 1;   // switch lit = shelf
            break;
        case 6:
            c.xHpfHz = v[0] < 0.02f ? 0.0f : logHz ((v[0] - 0.02f) / 0.98f, 16, 350);
            c.xLpfHz = v[5] < 0.02f ? 0.0f : logHz ((v[5] - 0.02f) / 0.98f, 22000, 3000);
            c.xHz[0] = 90.0f;   c.xDb[0] = bip (v[1], 15);
            c.xHz[1] = 300.0f;  c.xDb[1] = bip (v[2], 15); c.xQ[1] = 0.8f;
            c.xHz[2] = 3000.0f; c.xDb[2] = bip (v[3], 15); c.xQ[2] = 0.8f;
            c.xHz[3] = 10000.0f; c.xDb[3] = bip (v[4], 15);
            break;
        case 7:
            c.xHz[0] = 100.0f; c.xDb[0] = bip (v[0], 10);
            c.xHz[1] = pick (v[1], h69Mid); c.xDb[1] = bip (v[2], 10);
            c.xDb[2] = bip (v[3], 10);
            break;
        case 8:
            c.hpfHz = 5.0f;
            c.bodyHz = 80.0f; c.bodyDb = bip (v[0], 12);
            c.mudHz = 800.0f; c.mudQ = 0.6f; c.mudDb = bip (v[1], 12);
            c.presDb = 0.0f;
            c.airHz = pick (v[2], sp1Air); c.airDb = v[3] * 12.0f;
            c.silk = 0.25f;
            break;
        case 9:
            c.xDb[0] = bip (v[0], 10); c.xDb[1] = bip (v[1], 10); c.xDb[2] = bip (v[2], 10);
            c.xDb[3] = bip (v[3], 10); c.xDb[4] = bip (v[4], 10); c.xDb[5] = v[5] * 15.0f; c.xHz[5] = 20000.0f;
            break;
        case 10:
            c.xHz[0] = pick (v[0], mp2Low); c.xDb[0] = bip (v[1], 12);
            c.xHz[1] = 470.0f; c.xDb[1] = bip (v[2], 6);
            c.xHz[2] = 3300.0f; c.xDb[2] = bip (v[3], 6);
            c.xHz[3] = pick (v[4], mp2High); c.xDb[3] = bip (v[5], 12);
            break;
        case 11:   // LE-Esh
            c.xHz[0] = pick (v[0], eshLow); c.xDb[0] = v[1] * 12.0f; c.xDb[1] = -v[2] * 12.0f;
            c.xHz[2] = pick (v[3], eshMid); c.xDb[2] = bip (v[4], 10);
            c.xHz[3] = pick (v[5], eshHigh); c.xDb[3] = v[6] * 12.0f; c.xDb[4] = -v[7] * 12.0f;
            break;
        case 12:   // LE-Shemen
            c.xHz[0] = contHz (v[0], 20, 500); c.xDb[0] = bip (v[1], 16);
            c.xHz[1] = contHz (v[2], 200, 8000); c.xDb[1] = bip (v[3], 16); c.xQ[1] = 0.4f + v[4] * 2.6f;
            c.xHz[2] = contHz (v[5], 1500, 20000); c.xDb[2] = bip (v[6], 16);
            break;
        case 13:   // LE-Kav
            c.xHz[0] = contHz (v[0], 20, 400);    c.xDb[0] = bip (v[1], 16);
            c.xHz[1] = contHz (v[2], 60, 1200);   c.xDb[1] = bip (v[3], 16);
            c.xHz[2] = contHz (v[4], 300, 6000);  c.xDb[2] = bip (v[5], 16);
            c.xHz[3] = contHz (v[6], 1500, 16000); c.xDb[3] = bip (v[7], 16);
            c.xHz[4] = contHz (v[8], 3000, 20000); c.xDb[4] = bip (v[9], 16);
            c.xQ[1] = 0.4f + v[10] * 3.6f;
            break;
        case 14:   // LE-Even
            c.xHz[0] = pick (v[0], evenLB); c.xDb[0] = v[1] * 14.0f;
            c.xHz[1] = pick (v[2], evenLC); c.xDb[1] = -v[3] * 14.0f;
            c.xHz[2] = pick (v[4], evenHB); c.xDb[2] = v[5] * 12.0f;
            c.xHz[3] = pick (v[6], evenHC); c.xDb[3] = -v[7] * 14.0f;
            break;
        case 15:   // LE-Kavod
            c.xHz[0] = pick (v[0], kvLow);  c.xDb[0] = bip (v[1], 10);
            c.xHz[1] = pick (v[2], kvLm);   c.xDb[1] = bip (v[3], 8);
            c.xHz[2] = pick (v[4], kvMid);  c.xDb[2] = bip (v[5], 8);
            c.xHz[3] = pick (v[6], kvHm);   c.xDb[3] = bip (v[7], 8);
            c.xHz[4] = pick (v[8], kvHigh); c.xDb[4] = bip (v[9], 10);
            break;
        default: break;
    }
    return c;
}

// V2: the nodes drawn on a unit's curve (drag them instead of the knobs): each is locked to what the hardware can do -
// its frequency steps (or a fixed frequency), its gain steps, and its Q only where the unit has one
struct UnitNode
{
    const char* name;
    int fv = -1;                 // value index of the frequency (-1 = fixed)
    std::vector<float> steps;    // stepped frequencies (empty = continuous lo..hi, log)
    float lo = 0, hi = 0;        // continuous range (an "off" at the bottom of the knob); lo > hi = turns down (LPF)
    float fixedHz = 0;
    int gv = -1;                 // value index of the gain (-1 = a cut: no gain)
    int gmode = 0;               // 0 = -range..+range, 1 = boost 0..range, 2 = dip 0..-range, 3 = 550B steps
    float range = 0;
    int qv = -1;                 // value index of a width / Q (wheel)
};

inline const std::vector<UnitNode>& unitNodes (int unit)
{
    using namespace lists;
    auto L = [] (std::initializer_list<float> l) { return std::vector<float> (l); };
    static const std::vector<std::vector<UnitNode>> n {
        { { "Low boost", 0, L (ep1Low), 0, 0, 0, 1, 1, 14 }, { "Low atten", 0, L (ep1Low), 0, 0, 0, 2, 2, 10 }, { "High boost", 3, L (ep1High), 0, 0, 0, 4, 1, 16, 5 },
          { "High atten", 6, { 5000, 10000, 20000 }, 0, 0, 0, 7, 2, 16 } },
        { { "Low peak", 0, L (me5Low), 0, 0, 0, 1, 1, 10 }, { "Dip", 2, L (me5Dip), 0, 0, 0, 3, 2, 10 }, { "High peak", 4, L (me5High), 0, 0, 0, 5, 1, 10 } },
        { { "HPF", 0, {}, 16, 350 }, { "LF", -1, {}, 0, 0, 100, 1, 0, 15 }, { "LMF", -1, {}, 0, 0, 400, 2, 0, 15 }, { "HMF", -1, {}, 0, 0, 2500, 3, 0, 15 }, { "HF", -1, {}, 0, 0, 8000, 4, 0, 15 } },
        { { "HPF", 0, L (n73Hpf) }, { "Low", 2, L (n73Low), 0, 0, 0, 1, 0, 16 }, { "Mid", 4, L (n73Mid), 0, 0, 0, 3, 0, 18 }, { "High", 6, L (n73High), 0, 0, 0, 5, 0, 16 } },
        { { "HPF", 0, L (n81Hpf) }, { "Low", -1, {}, 0, 0, 110, 1, 0, 16 }, { "Mid", 2, L (n81Mid), 0, 0, 0, 3, 0, 18 }, { "High", -1, {}, 0, 0, 12000, 4, 0, 16 } },
        { { "Low", 1, L (a550b1), 0, 0, 0, 0, 3, 12 }, { "Low-mid", 3, L (a550b2), 0, 0, 0, 2, 3, 12 }, { "Hi-mid", 5, L (a550b3), 0, 0, 0, 4, 3, 12 }, { "High", 7, L (a550b4), 0, 0, 0, 6, 3, 12 } },
        { { "HPF", 0, {}, 16, 350 }, { "LF", -1, {}, 0, 0, 90, 1, 0, 15 }, { "LMF", -1, {}, 0, 0, 300, 2, 0, 15 }, { "HMF", -1, {}, 0, 0, 3000, 3, 0, 15 }, { "HF", -1, {}, 0, 0, 10000, 4, 0, 15 }, { "LPF", 5, {}, 22000, 3000 } },
        { { "Bass", -1, {}, 0, 0, 100, 0, 0, 10 }, { "Mid", 1, L (h69Mid), 0, 0, 0, 2, 0, 10 }, { "Treble", -1, {}, 0, 0, 10000, 3, 0, 10 } },
        { { "Low", -1, {}, 0, 0, 80, 0, 0, 12 }, { "Mid", -1, {}, 0, 0, 800, 1, 0, 12 }, { "Air", 2, L (sp1Air), 0, 0, 0, 3, 1, 12 } },
        { { "Sub", -1, {}, 0, 0, 20, 0, 0, 10 }, { "40", -1, {}, 0, 0, 40, 1, 0, 10 }, { "160", -1, {}, 0, 0, 160, 2, 0, 10 }, { "650", -1, {}, 0, 0, 650, 3, 0, 10 },
          { "2.5K", -1, {}, 0, 0, 2500, 4, 0, 10 }, { "Air", -1, {}, 0, 0, 15000, 5, 1, 15 } },
        { { "Low", 0, L (mp2Low), 0, 0, 0, 1, 0, 12 }, { "Lo-mid", -1, {}, 0, 0, 470, 2, 0, 6 }, { "Hi-mid", -1, {}, 0, 0, 3300, 3, 0, 6 }, { "High", 4, L (mp2High), 0, 0, 0, 5, 0, 12 } },
        // V9
        { { "Low boost", 0, L (eshLow), 0, 0, 0, 1, 1, 12 }, { "Low cut", 0, L (eshLow), 0, 0, 0, 2, 2, 12 }, { "Mid", 3, L (eshMid), 0, 0, 0, 4, 0, 10 },
          { "High boost", 5, L (eshHigh), 0, 0, 0, 6, 1, 12 }, { "High cut", 5, L (eshHigh), 0, 0, 0, 7, 2, 12 } },
        { { "Low", 0, {}, 20, 500, 0, 1, 0, 16 }, { "Mid", 2, {}, 200, 8000, 0, 3, 0, 16, 4 }, { "High", 5, {}, 1500, 20000, 0, 6, 0, 16 } },
        { { "Low", 0, {}, 20, 400, 0, 1, 0, 16 }, { "Lo-mid", 2, {}, 60, 1200, 0, 3, 0, 16, 10 }, { "Mid", 4, {}, 300, 6000, 0, 5, 0, 16, 10 },
          { "Hi-mid", 6, {}, 1500, 16000, 0, 7, 0, 16, 10 }, { "High", 8, {}, 3000, 20000, 0, 9, 0, 16 } },
        { { "Low boost", 0, L (evenLB), 0, 0, 0, 1, 1, 14 }, { "Low cut", 2, L (evenLC), 0, 0, 0, 3, 2, 14 },
          { "High boost", 4, L (evenHB), 0, 0, 0, 5, 1, 12 }, { "High cut", 6, L (evenHC), 0, 0, 0, 7, 2, 14 } },
        { { "Low", 0, L (kvLow), 0, 0, 0, 1, 0, 10 }, { "Lo-mid", 2, L (kvLm), 0, 0, 0, 3, 0, 8 }, { "Mid", 4, L (kvMid), 0, 0, 0, 5, 0, 8 },
          { "Hi-mid", 6, L (kvHm), 0, 0, 0, 7, 0, 8 }, { "High", 8, L (kvHigh), 0, 0, 0, 9, 0, 10 } },
    };
    return n[(size_t) juce::jlimit (0, numUnits - 1, unit)];
}

// V9: a boost and a cut on the same frequency (Pultec-style) sit a little apart, so each can be grabbed
inline float nodePairOffset (int unit, int node)
{
    const auto& n = unitNodes (unit);
    if (node < 0 || node >= (int) n.size()) return 0.0f;
    const auto& a = n[(size_t) node];
    if (a.fv < 0 || (a.gmode != 1 && a.gmode != 2)) return 0.0f;
    for (size_t j = 0; j < n.size(); ++j)
        if ((int) j != node && n[j].fv == a.fv && (n[j].gmode == 1 || n[j].gmode == 2) && n[j].gmode != a.gmode)
            return a.gmode == 1 ? 9.0f : -9.0f;   // boost to the right, cut to the left
    return 0.0f;
}
inline const juce::StringArray& presetTagNames()
{
    static const juce::StringArray t { "Vocals", "Drums", "Instruments", "Mastering", "Mix Bus", "Tahor Surgical", "Reso Clean-up", "Character", "Effects", "Utility", "Mine" };   // V11
    return t;
}

// where a node sits (Hz, dB) for the unit's values
inline float nodeHz (const UnitNode& d, const float* v)
{
    if (d.fv < 0) return d.fixedHz;
    const float a = v[d.fv];
    if (! d.steps.empty())
    {
        const int i = detail::idx (a, (int) d.steps.size());
        float f = d.steps[(size_t) i];
        if (f <= 0) f = d.steps.size() > 1 ? d.steps[1] * 0.6f : 20.0f;   // "off": parked just below the first step
        return f;
    }
    if (a < 0.02f) return d.lo < d.hi ? d.lo * 0.7f : d.lo * 1.1f;
    return detail::logHz ((a - 0.02f) / 0.98f, d.lo, d.hi);
}
inline bool nodeOff (const UnitNode& d, const float* v)
{
    if (d.fv < 0 || d.gv >= 0) return false;
    if (! d.steps.empty()) return d.steps[(size_t) detail::idx (v[d.fv], (int) d.steps.size())] <= 0;
    return v[d.fv] < 0.02f;
}
inline float nodeDb (const UnitNode& d, const float* v)
{
    if (d.gv < 0) return 0.0f;
    const float a = v[d.gv];
    switch (d.gmode)
    {
        case 1: return a * d.range;
        case 2: return -a * d.range;
        case 3: return step550 (a);
        default: return detail::bip (a, d.range);
    }
}
// the knob positions for a dragged node (snapped to the hardware's steps)
inline float freqPos (const UnitNode& d, float hz)
{
    if (! d.steps.empty())
    {
        int best = 0; float bd = 1.0e9f;
        for (size_t i = 0; i < d.steps.size(); ++i)
        {
            if (d.steps[i] <= 0) continue;
            const float dd = std::abs (std::log2 (hz / d.steps[i]));
            if (dd < bd) { bd = dd; best = (int) i; }
        }
        if (d.steps[0] <= 0 && d.steps.size() > 1 && hz < d.steps[1] * 0.75f) best = 0;   // dragged off the bottom: off
        return (float) best / (float) (d.steps.size() - 1);
    }
    const bool down = d.lo > d.hi;
    if ((! down && hz < d.lo * 0.85f) || (down && hz > d.lo * 1.05f)) return 0.0f;
    const float t = std::log (hz / d.lo) / std::log (d.hi / d.lo);
    return 0.02f + 0.98f * juce::jlimit (0.0f, 1.0f, t);
}
inline float gainPos (const UnitNode& d, float db)
{
    switch (d.gmode)
    {
        case 1: return juce::jlimit (0.0f, 1.0f, db / d.range);
        case 2: return juce::jlimit (0.0f, 1.0f, -db / d.range);
        case 3: return (float) (juce::jlimit (-6, 6, (int) std::round (db / 2.0f)) + 6) / 12.0f;
        default: return juce::jlimit (0.0f, 1.0f, (db / d.range + 1.0f) * 0.5f);
    }
}

} // namespace tahor
