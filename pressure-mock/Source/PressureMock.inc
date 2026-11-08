// ===== PRESSURE: switches and controls =====
const char* tahor::penv (const char* name)
{
   #if PRESSURE_MOCK
    if (auto* v = std::getenv (name)) return v;   // the snapshot harness: the environment says how to draw
    return nullptr;
   #else
    return gPressCanvas != nullptr ? gPressCanvas->flagFor (name) : nullptr;
   #endif
}

const char* Canvas::flagFor (const char* name)
{
    auto is = [name] (const char* k) { return std::strcmp (name, k) == 0; };
    static const char* yes = "1";
    const int eng = (int) proc.param ("cEngine");
    if (is ("PRESS_RVOX")) return tab == 1 && eng == 1 ? yes : nullptr;
    if (is ("PRESS_DS1"))  return tab == 1 && eng == 2 ? yes : nullptr;
    if (is ("PRESS_MLA"))  return tab == 1 && eng == 3 ? yes : nullptr;
    if (is ("PRESS_PRO"))  return tab == 1 && eng == 4 ? yes : nullptr;
    if (is ("PRESS_KNEE")) return kneeOn ? yes : nullptr;
    if (is ("PRESS_ENV") || is ("PRESS_CHAR") || is ("PRESS_DYNEQ")) return envOn ? yes : nullptr;
    if (is ("PRESS_SC"))   return scOn && tab == 1 ? yes : nullptr;
    if (is ("PRESS_TOPOFF")) return topOff ? yes : nullptr;
    if (is ("PRESS_DS1_BAND")) return (int) proc.param ("cDsBand") == 1 ? yes : nullptr;
    if (is ("PRESS_RVOX_DEMO")) return tab == 1 && eng == 1 && proc.param ("cVoxComp") < -0.5f ? yes : nullptr;
    if (is ("PRESS_MLA_LAMPOFF")) return proc.grDb.load() < 0.3f ? yes : nullptr;   // green while it isn't compressing
    if (is ("PRESS_VUNIT")) { static char b[8]; std::snprintf (b, sizeof (b), "%d", juce::jlimit (0, 14, preview)); return b; }
    return nullptr;
}

int Canvas::pctl (R r, const juce::String& pid, int kind)
{
    pz.push_back (pid);
    const int idx = (int) pz.size() - 1;
    dz (r, Hit::PCtl, idx, kind);
    return idx;
}
float Canvas::pn (const juce::String& pid) const
{
    if (auto* p = proc.apvts.getParameter (pid)) return p->getValue();
    return 0.0f;
}
juce::String Canvas::pchoice (const juce::String& pid) const
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (pid))) return p->getCurrentChoiceName();
    return {};
}
juce::String Canvas::pfmt (const juce::String& pid) const
{
    const float v = proc.param (pid);
    const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");
    auto db = [&] (float x, int dec, bool plus)
    {
        if (std::abs (x) < 0.05f && dec > 0) return juce::String ("0.0 dB");
        return (x < 0 ? minus : plus && x > 0 ? juce::String ("+") : juce::String()) + juce::String (std::abs (x), dec) + " dB";
    };
    auto ms = [] (float x)
    {
        if (x >= 1000.0f) return juce::String (x / 1000.0f, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " s";
        if (x < 0.005f) return juce::String ("0 ms");
        if (x < 1.0f) return juce::String (x, 2) + " ms";
        if (x < 10.0f) return juce::String (x, 1) + " ms";
        return juce::String ((int) std::round (x)) + " ms";
    };
    if (pid == "cThresh") return db (v, 1, false);
    if (pid == "cRatio")  return (v >= 19.95f ? juce::String ("INF") : juce::String (v, v < 10.0f ? 1 : 0).trimCharactersAtEnd ("0").trimCharactersAtEnd (".")) + " : 1";
    if (pid == "cKnee")   return juce::String (v, 0) + " dB";
    if (pid == "cDsKnee") return juce::String (v, 2);
    if (pid == "cAttack" || pid == "cRelease" || pid == "cDsRelF" || pid == "cDsRelS" || pid == "cDsAvg" || pid == "cHold") return ms (v);
    if (pid == "cMakeup" || pid == "cVoxGain" || pid == "cScGain") return db (v, 1, true);
    if (pid == "cProWet") return db (v, 1, true);
    if (pid == "cProDry") return v <= -59.95f ? minus + juce::String::fromUTF8 ("\xe2\x88\x9e") : db (v, 1, false);
    if (pid == "cVoxComp") return db (v, 1, false);
    if (pid == "cVoxGate") return v <= -79.5f ? juce::String ("Off") : minus + juce::String ((int) std::round (-v)) + " dB";
    if (pid == "cMix" || pid == "cScLink" || pid == "cProDrive") return juce::String ((int) std::round (v)) + "%";
    if (pid == "cRange") return juce::String (v, 0) + " dB";
    if (pid == "cMlaIn") return v < 0.5f ? juce::String ("0 dB") : minus + juce::String ((int) std::round (v)) + " dB";
    if (pid == "cMlaRatio") { const auto c = pchoice (pid); return c == "INF" ? c : c + " : 1"; }
    if (pid == "cMlaAtk") return pchoice (pid) + " ms/dB";
    if (pid == "cMlaRel") return pchoice (pid) + " s/dB";
    if (pid == "cLook") return juce::String (v, 1) + " ms";
    if (auto* p = proc.apvts.getParameter (pid)) return p->getCurrentValueAsText();
    return {};
}

static bool liftDynEq() { return penv ("PRESS_LIFTMODE") != nullptr && std::atoi (penv ("PRESS_LIFTMODE")) == 1; }
// Pressure - the dynamics screens (mock): a scrolling picture of the level, newest on the right.
// HELPER: the rider, breath control and peak catcher. CLEAN: a modern compressor drawn on the screen.
// VINTAGE: the same picture over the chained hardware.

namespace
{
    inline float dynY (float db) { return 40.0f + juce::jlimit (0.0f, 62.0f, -db) / 60.0f * 590.0f; }
    // HELPER, classy: soft, harmonised tones - sea glass, champagne, mist and sage - instead of bright primaries
    inline juce::Colour helperAccent() { if (auto* c = penv ("PRESS_HCOL")) return col (c); return col ("#b08d57"); }   // HELPER: bronze (from the vintage metals)
    const juce::Colour helperCx = col ("#e3bf7a"), peakCx = col ("#e3bf7a");
    // gain reduction: red, except LE-DS1, whose display is black with grey bars like the real DS1-MK3
    inline juce::Colour grColour() { return penv ("PRESS_MLA") != nullptr ? col ("#8fcaff") : penv ("PRESS_DS1") != nullptr ? col ("#a7abb1") : col ("#ff5a6a"); }
    // each CLEAN engine wears the colour of the original it comes from
    inline bool gHelperTab = false;   // HELPER is drawing: everything wears its teal
    juce::Colour engineColour()
    {
        if (gHelperTab) return helperAccent();
        if (gCompMode)   // VINTAGE: jewel tones - classy, each from its original
        {
            static const char* vc[15] { "#8a3a3f",    // LE-76: oxblood          (leather & wood - the old studio)
                                       "#b5763e",    // LE-660: cognac
                                       "#c4a27a",    // LE-STA: saddle tan
                                       "#6b4a3a",    // LE-2500: walnut
                                       "#5f6b4e",    // LE-G: olive
                                       "#2f4a6b",    // LE-IRON: navy leather
                                       "#7a5a8a",    // LE-336: aubergine
                                       "#3f7a5e",    // LE-8X: racing green
                                       "#c9a227",    // LE-160: mustard
                                       "#3d8a8a",    // LE-7: teal
                                       "#7d8590",    // LE-760: slate
                                       "#e6d6b8",    // LE-SHM: cream
                                       "#b9767a",    // LE-2A: dusty rose
                                       "#5677ab",    // LE-1B: denim
                                       "#d0682f" };  // LE-VSC: burnt orange  // LE-336: slate blue (the Neve's blue-grey)
            const int vi = juce::jlimit (0, 14, penv ("PRESS_VUNIT") ? std::atoi (penv ("PRESS_VUNIT")) : 0);
            if (auto* vs = penv ("PRESS_VCOLS")) { auto t = juce::StringArray::fromTokens (vs, ",", ""); if (vi < t.size()) return col (t[vi].toRawUTF8()); }   // (mock: try a palette)
            return col (vc[vi]);
        }
        if (penv ("PRESS_RVOX") != nullptr) return col ("#e8a98f");   // LE-755C: rose gold
        if (penv ("PRESS_DS1")  != nullptr) { if (auto* c = penv ("PRESS_DS1_COL")) return col (c); return col ("#efe5cf"); }   // LE-DS1: white gold (the DS1-MK3's white face, in the gold family)
        if (penv ("PRESS_MLA")  != nullptr) return col ("#ecd59c");   // LE-MLA: pale gold over an ivory wave - Maselec MLA-2: silver and black, like the DS1 look - its blue light is in the meters and the reduction
        if (penv ("PRESS_PRO")  != nullptr) { if (auto* c = penv ("PRESS_PRO_COL")) return col (c); return col ("#c9a24e"); }   // LE-PRO: antique brass   // Pro-C 3: its yellow
        if (penv ("PRESS_VIOLET") != nullptr) return col ("#a99bff");
        if (auto* c = penv ("PRESS_RC_COL")) return col (c);
        return col ("#a8916a");   // LE-RC: smoked gold                                               // LE-RC (Renaissance Comp): its amber faders
    }
}
#define cleanC engineColour()
#define helperC helperAccent()
#define peakC helperAccent()
#define grC grColour()

void Canvas::pushHistory()
{
    const int n = (int) ((PW - 50.0f) / 2.0f);
    if ((int) hIn.size() != n) { hIn.assign ((size_t) n, -90.0f); hOut = hIn; hGr.assign ((size_t) n, 0.0f); hRide = hGr; hBreath = hGr; hPos = 0; }
    const float in = juce::Decibels::gainToDecibels (juce::jmax (proc.inMeterL.load(), proc.inMeterR.load()), -90.0f);
    // the compressor (soft knee) the screen draws
    const bool rvox0 = penv ("PRESS_RVOX") != nullptr && penv ("PRESS_RVOX_DEMO") == nullptr;
    const bool heavy = penv ("PRESS_GR_HEAVY") != nullptr;   // (mock: pushed hard, to show lots of gain reduction)
    const float thrNow = heavy ? -42.0f : rvox0 ? 0.0f : mThr, ratioNow = heavy ? 6.0f : mRatio;
    float over = in - thrNow, gr = 0.0f;
    if (over > mKnee * 0.5f) gr = over * (1.0f - 1.0f / ratioNow);
    else if (over > -mKnee * 0.5f) { const float k = over + mKnee * 0.5f; gr = (1.0f - 1.0f / ratioNow) * k * k / (2.0f * mKnee); }
    compEnv = gr > compEnv ? compEnv + (gr - compEnv) * 0.6f : compEnv + (gr - compEnv) * 0.09f;
    // the rider: lifts or lowers the phrase toward -16 dB, slowly, within +/-6 dB
    if (in > -50.0f) slowLvl += (in - slowLvl) * 0.04f;
    const float ride = juce::jlimit (-6.0f, 6.0f, -16.0f - slowLvl);
    // breath control: the quiet, airy gaps between phrases are turned down
    const bool breath = in < -34.0f && in > -56.0f;
    breathEnv += ((breath ? -8.0f : 0.0f) - breathEnv) * 0.25f;
    hIn[(size_t) hPos] = in;
   #if PRESSURE_MOCK
    hGr[(size_t) hPos] = tab == 0 ? juce::jmax (0.0f, (in + ride + breathEnv) - (-14.0f)) * 0.8f : compEnv;   // HELPER: what the 1964 catches (mock: the peaks over its threshold)
    hRide[(size_t) hPos] = ride;
    hBreath[(size_t) hPos] = breathEnv;
    hOut[(size_t) hPos] = tab == 0 ? juce::jmin (-6.0f, in + ride + breathEnv) : in - compEnv;
   #else
    // the real thing: CLEAN's reduction and what comes out (HELPERS and VINTAGE pass the sound through in V1)
    const float out = juce::Decibels::gainToDecibels (juce::jmax (proc.meterL.load(), proc.meterR.load()), -90.0f);
    hGr[(size_t) hPos] = tab == 1 && proc.param ("tahorOn") > 0.5f ? proc.grDb.load() : 0.0f;
    hRide[(size_t) hPos] = 0.0f;
    hBreath[(size_t) hPos] = 0.0f;
    hOut[(size_t) hPos] = out;
    juce::ignoreUnused (ride);
   #endif
    hPos = (hPos + 1) % n;
}

void Canvas::paintDynScreen (juce::Graphics& g, int kind)
{
    const int n = (int) hIn.size();
    if (n < 2) return;
    const float xR = PW - 50.0f;
    auto tc = kind == 0 ? helperC : kind == 1 ? cleanC : amber;
    if (kind == 1 && penv ("PRESS_RVOX") != nullptr) tc = col ("#dcc4b8");   // LE-755C: a warm blush wave under the rose gold
    if (kind == 1 && penv ("PRESS_PRO") != nullptr) tc = col ("#b8a57f");    // LE-PRO: a bronze-tinted wave under the brass
    if (kind == 1 && penv ("PRESS_MLA") != nullptr) tc = col ("#ebe7de");    // LE-MLA: ivory wave
    if (kind == 1 && penv ("PRESS_DS1") != nullptr) tc = col ("#6e6a63");    // LE-DS1: a black wave (its black display)
    if (kind == 0) tc = helperC.interpolatedWith (col ("#77777c"), 0.55f);   // HELPER: a wave tinted by its colour, like VINTAGE   // HELPER: a sea-glass tinted wave, quiet like the compressors'
    if (kind == 1 && gCompMode) tc = cleanC.interpolatedWith (col ("#77777c"), 0.55f);   // VINTAGE: a wave tinted by the unit's jewel colour
    else if (kind == 1 && penv ("PRESS_RVOX") == nullptr && penv ("PRESS_DS1") == nullptr && penv ("PRESS_MLA") == nullptr && penv ("PRESS_PRO") == nullptr)
        tc = col ("#7e7567");                                                     // LE-RC: a slate wave under the smoked gold
    if (kind == 1) if (auto* w = penv ("PRESS_WAVE_COL")) tc = col (w);   // (mock: try a separate waveform colour)
    auto at = [&] (const std::vector<float>& h, int age) { return h[(size_t) ((hPos - 1 - age + 2 * n) % n)]; };
    // the waveform, Pro-C style: centred like a clip in the DAW, in dB so the quiet parts still show -
    // what comes in in dark grey, what goes out over it in the tab's colour with a light edge
    const float cy = 340.0f, hh = 250.0f;
    auto hOf = [hh] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * hh; };
    auto wave = [&] (const std::vector<float>& h, juce::FillType fillC, juce::FillType edgeC, float edgeW)
    {
        juce::Path top, fill;
        for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = cy - hOf (at (h, a)); if (a == 0) top.startNewSubPath (x, y); else top.lineTo (x, y); }
        fill = top;
        for (int a = n - 1; a >= 0; --a) fill.lineTo (xR - (float) a * 2.0f, cy + hOf (at (h, a)));
        fill.closeSubPath();
        juce::Path bot;
        for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = cy + hOf (at (h, a)); if (a == 0) bot.startNewSubPath (x, y); else bot.lineTo (x, y); }
        g.setFillType (fillC); g.fillPath (fill);
        if (edgeW > 0.0f) { g.setFillType (edgeC); g.strokePath (top, juce::PathStrokeType (edgeW)); g.strokePath (bot, juce::PathStrokeType (edgeW)); }
    };
    wave (hIn, juce::Colours::white.withAlpha (0.09f), juce::Colours::white.withAlpha (0.18f), 1.0f);
    wave (hOut, tc.withAlpha (0.24f), tc.withAlpha (0.9f), 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (0.0f, cy, xR, 1.0f);
    if (kind == 1 && penv ("PRESS_RVOX") != nullptr && penv ("PRESS_RVOX_DEMO") != nullptr)   // gate on only
    {
        const float gate = -48.0f, gh = hOf (gate);
        const auto gc = col ("#e8b84a");
        for (int a = 0; a < n; ++a)
            if (at (hIn, a) < gate + 3.0f && at (hIn, a) > -85.0f)
            {
                const float x = xR - (float) a * 2.0f, h = juce::jmax (2.0f, hOf (at (hIn, a)));
                g.setColour (gc.withAlpha (0.35f)); g.fillRect (x - 1.0f, cy - h, 2.0f, h * 2.0f);
            }
        for (float yy : { cy - gh, cy + gh })
        {
            juce::ColourGradient lg (gc.withAlpha (0.0f), 0, 0, gc.withAlpha (0.0f), xR, 0, false);
            lg.addColour (0.15, gc.withAlpha (0.3f)); lg.addColour (0.85, gc.withAlpha (0.3f));
            g.setGradientFill (lg); g.fillRect (0.0f, yy - 0.5f, xR, 1.0f);
        }
        text (g, U8 ("GATE  \xe2\x88\x92") + "48 dB", font (Face::CondMedium, 9, 1.4f), gc.withAlpha (0.8f), R (xR - 260, cy + gh + 6, 90, 11));
    }
    if (kind == 0)
    {
        // the rider's move (around the middle line), the breaths it turned down, and the peak catcher's line
        const float mid = cy;
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (0.0f, mid, xR, 1.0f);
        juce::Path rp;
        for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = mid - at (hRide, a) * 10.0f; if (a == 0) rp.startNewSubPath (x, y); else rp.lineTo (x, y); }
        const float x0 = 214.0f, x1 = PW - 214.0f;   // the readouts live between the glass panels
        auto fadeX = [&] (juce::Colour c, float a) { juce::ColourGradient lg (c.withAlpha (0.0f), x0 - 60, 0, c.withAlpha (0.0f), x1 + 60, 0, false);
                                                      lg.addColour (0.12, c.withAlpha (a)); lg.addColour (0.88, c.withAlpha (a)); return lg; };
        g.setGradientFill (fadeX (helperC, 0.18f)); g.strokePath (rp, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill (fadeX (helperC, 0.9f)); g.strokePath (rp, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (int a = 0; a < n; ++a)
            if (at (hBreath, a) < -2.0f)
            {
                const float x = xR - (float) a * 2.0f;
                if (x < x0 || x > x1) continue;
                g.setColour (helperC.withAlpha (0.08f + 0.02f * -at (hBreath, a))); g.fillRect (x - 0.5f, 612.0f, 1.0f, 14.0f);
            }
        // the 1964's ceiling: a fine solid line, top and bottom, fading out before the panels (no dashes)
        const float py = cy - hOf (-6.0f);
        for (float yy : { py, cy + hOf (-6.0f) }) { g.setGradientFill (fadeX (peakC, 0.55f)); g.fillRect (x0 - 60, yy - 0.5f, x1 - x0 + 120, 1.0f); }
        auto f = font (Face::CondMedium, 9, 1.6f);
        auto pill = [&] (float x, float y, const juce::String& t, juce::Colour c)
        {   // a quiet glass tag
            const float w = textWidth (f, t) + 20;
            const R b (x - w, y - 9, w, 18);
            fillRound (g, b, 9, col ("#121216", 0.6f)); strokeRound (g, b, 9, juce::Colours::white.withAlpha (0.1f));
            g.setColour (c); g.fillEllipse (b.getX() + 8, b.getCentreY() - 2, 4, 4);
            text (g, t, f, c.interpolatedWith (juce::Colours::white, 0.3f), b.withTrimmedLeft (8), juce::Justification::centred);
        };
        // the 1964's gain reduction: the same thin red line as the compressors, hanging from the top
        {
            juce::Path edge;
            for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = 40.0f + juce::jmin (24.0f, at (hGr, a)) * 9.0f; if (a == 0) edge.startNewSubPath (x, y); else edge.lineTo (x, y); }
            g.setColour (col ("#000000", 0.45f)); g.strokePath (edge, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col ("#ff4d5e")); g.strokePath (edge, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const float ride = at (hRide, 0);
        pill (x1 - 10, mid - ride * 10.0f - 18, "RIDER  " + juce::String (ride > 0 ? "+" : ride < 0 ? MINUS : juce::String()) + juce::String (std::abs (ride), 1) + " dB", helperC);
        pill (x1 - 10, py - 14, U8 ("1964  \xe2\x88\x92") + "6.0 dB", peakC);
        pill (x1 - 10, 600, U8 ("BREATHS  \xe2\x88\x92") + "8 dB", helperC);
        return;
    }
    // gain reduction, hanging from the top (1 dB = 9 px)
    {
        juce::Path fill, edge;
        fill.startNewSubPath (xR, 40.0f);
        for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = 40.0f + juce::jmin (24.0f, at (hGr, a)) * 9.0f; fill.lineTo (x, y); if (a == 0) edge.startNewSubPath (x, y); else edge.lineTo (x, y); }
        fill.lineTo (xR - (float) (n - 1) * 2.0f, 40.0f); fill.closeSubPath();
        if (kind == 1)
        {
            // every CLEAN engine: just a red line, like Pro-C 3 - no fill
            g.setColour (col ("#000000", 0.45f)); g.strokePath (edge, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col ("#ff4d5e")); g.strokePath (edge, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (kind == 1 && penv ("PRESS_DS1") != nullptr)
        {
            // LE-DS1: like its display - the reduction in black, with a grey edge
            g.setGradientFill (juce::ColourGradient (col ("#000000", 0.55f), 0, 40, col ("#000000", 0.85f), 0, 200, false)); g.fillPath (fill);
            g.setColour (col ("#000000", 0.6f)); g.strokePath (edge, juce::PathStrokeType (4.0f));
            g.setColour (col ("#a7abb1")); g.strokePath (edge, juce::PathStrokeType (1.6f));
        }
        else
        {
        g.setGradientFill (juce::ColourGradient (grC.withAlpha (0.05f), 0, 40, grC.withAlpha (0.35f), 0, 200, false)); g.fillPath (fill);
        g.setColour (grC.withAlpha (0.9f)); g.strokePath (edge, juce::PathStrokeType (1.4f));
        }
    }
    // the threshold: a line across the screen, with its handle on the right
    {
        // no line: the parts of the waveform that reach past the threshold glow (they are what's being squashed)
        const float ty = cy - hOf (mThr), tb = cy + hOf (mThr);
        const auto glowC = tc.interpolatedWith (juce::Colours::white, 0.35f);
        for (int side = 0; side < 2; ++side)
        {
            g.saveState();
            g.reduceClipRegion ((side == 0 ? R (0, -100, xR + 2, ty + 100) : R (0, tb, xR + 2, 900)).toNearestInt());
            // the glow grows from nothing at the threshold to full a little past it - no hard edge
            const float y0 = side == 0 ? ty : tb, y1 = side == 0 ? ty - 70.0f : tb + 70.0f;
            auto grad = [&] (juce::Colour c, float peak) { return juce::FillType (juce::ColourGradient (c.withAlpha (0.0f), 0, y0, c.withAlpha (peak), 0, y1, false)); };
            wave (hIn, grad (glowC, 0.18f), grad (glowC, 0.4f), 6.0f);
            wave (hIn, grad (glowC, 0.55f), grad (juce::Colours::white, 0.95f), 1.4f);
            g.restoreState();
        }
    }
    // the knee: input against output, the dot is where the sound is now (CLEAN draws its own in the left panel)
    if (! addOpen && kind != 1)
    {
        const R k (xR - 162, 54, 136, 136);
        fillRound (g, k, 10, col ("#121216", 0.82f)); strokeRound (g, k, 10, juce::Colours::white.withAlpha (0.1f));
        const R a = k.reduced (14);
        auto px = [&] (float db) { return a.getX() + (db + 60.0f) / 60.0f * a.getWidth(); };
        auto py = [&] (float db) { return a.getBottom() - (db + 60.0f) / 60.0f * a.getHeight(); };
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (px (-60), py (-60), px (0), py (0), 1.0f);
        juce::Path c;
        for (int i = 0; i <= 60; ++i)
        {
            const float in = -60.0f + (float) i, over = in - mThr;
            float gr = 0.0f;
            if (over > mKnee * 0.5f) gr = over * (1.0f - 1.0f / mRatio);
            else if (over > -mKnee * 0.5f) { const float q = over + mKnee * 0.5f; gr = (1.0f - 1.0f / mRatio) * q * q / (2.0f * mKnee); }
            if (i == 0) c.startNewSubPath (px (in), py (in - gr)); else c.lineTo (px (in), py (in - gr));
        }
        g.setColour (tc); g.strokePath (c, juce::PathStrokeType (1.6f));
        const float now = juce::jlimit (-60.0f, 0.0f, at (hIn, 0));
        g.setColour (juce::Colours::white); g.fillEllipse (px (now) - 3, py (now - at (hGr, 0)) - 3, 6, 6);
        text (g, "KNEE", font (Face::CondMedium, 9, 1.4f), grey, R (k.getX() + 10, k.getY() + 6, 60, 10));
        text (g, juce::String (mRatio, 1).trimCharactersAtEnd (".0") + ":1", font (Face::Regular, 10), ink, R (k.getRight() - 50, k.getBottom() - 18, 40, 12), juce::Justification::centredRight);
    }
    // how much it is taking off right now (CLEAN has its own meter)
    if (! addOpen && kind != 1)
    {
        const float grNow = at (hGr, 0);
        auto f = font (Face::Regular, 11);
        const juce::String t = "GR  " + (grNow > 0.05f ? MINUS : juce::String()) + juce::String (grNow, 1) + " dB";
        const R b (xR - 162, 196, 136, 24);
        fillRound (g, b, 12, col ("#121216", 0.82f)); strokeRound (g, b, 12, grC.withAlpha (0.5f));
        text (g, t, f, grC.interpolatedWith (juce::Colours::white, 0.3f), b, juce::Justification::centred);
    }
}

// a modern hardware knob: matte body with a knurled rim, a scale of ticks, the value as a glowing arc, a white index
static constexpr int kVinUnits = 15;   // VINTAGE units in the mock (the first seven + LE-8X + seven more)
static bool gHelperKnob = false;
// HELPER's own knob - three candidates (mock: PRESS_HKNOB = 1 machined, 2 skirted, 3 glass ring)
static void helperKnob (juce::Graphics& g, R b, float pos, juce::Colour c, const juce::String& val)
{
    const int style = penv ("PRESS_HKNOB") ? std::atoi (penv ("PRESS_HKNOB")) : 2;   // chosen: 2, skirted vintage
    pos = juce::jlimit (0.0f, 1.0f, pos);
    const auto ctr = b.getCentre();
    const float r = b.getWidth() * 0.5f, a0 = -135.0f * pi / 180.0f, a1 = 135.0f * pi / 180.0f, av = a0 + (a1 - a0) * pos;
    auto arcPath = [&] (float rr, float from, float to) { juce::Path p; p.addCentredArc (ctr.x, ctr.y, rr, rr, 0.0f, from, to, true); return p; };
    if (style == 3)
    {
        // GLASS RING: no body at all - a thick smoked-glass ring, the value filling it in the tool's colour, the number in the middle
        const float rr = r - 5.0f;
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.strokePath (arcPath (rr, a0, a1), juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (arcPath (rr + 4.0f, a0, a1), juce::PathStrokeType (1.0f));
        if (pos > 0.005f)
        {
            g.setColour (c.withAlpha (0.22f)); g.strokePath (arcPath (rr, a0, av), juce::PathStrokeType (14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            juce::ColourGradient cg (c.withMultipliedBrightness (0.7f), ctr.x - r, ctr.y + r, c.interpolatedWith (juce::Colours::white, 0.25f), ctr.x + r, ctr.y - r, false);
            g.setGradientFill (cg); g.strokePath (arcPath (rr, a0, av), juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const P d (ctr.x + rr * std::sin (av), ctr.y - rr * std::cos (av));
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (d.x - 6, d.y - 5, 12, 12);
        g.setColour (col ("#f4f1ea")); g.fillEllipse (d.x - 4.5f, d.y - 4.5f, 9, 9);
        text (g, val, font (Face::Light, r > 30 ? 14.0f : 12.0f, 0.3f), ink, b.withSizeKeepingCentre (b.getWidth() - 16, 18), juce::Justification::centred);
        return;
    }
    // shared: a soft shadow under the body
    const R body = b.reduced (style == 2 ? 2.0f : 6.0f);
    { juce::Path p; p.addEllipse (body.translated (0, 3)); cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 8, {}), p); }
    if (style == 2)
    {
        // SKIRTED: like the old hardware - a wide dark skirt with a 0-10 scale printed on it and a bronze pointer line, a small glossy cap
        g.setGradientFill (juce::ColourGradient (col ("#2a2724"), body.getX(), body.getY(), col ("#0d0c0b"), body.getRight(), body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (body.reduced (0.5f), 1.0f);
        const float sr = body.getWidth() * 0.5f;
        for (int i = 0; i <= 10; ++i)
        {
            const float t = a0 + (a1 - a0) * (float) i / 10.0f;
            const bool major = i % 5 == 0;
            g.setColour (juce::Colours::white.withAlpha (major ? 0.55f : 0.28f));
            const float r0 = sr - (major ? 7.0f : 5.0f), r1 = sr - 2.5f;
            g.drawLine (ctr.x + r0 * std::sin (t), ctr.y - r0 * std::cos (t), ctr.x + r1 * std::sin (t), ctr.y - r1 * std::cos (t), major ? 1.3f : 0.9f);
        }
        if (pos > 0.005f) { g.setColour (c.withAlpha (0.8f)); g.strokePath (arcPath (sr - 9.5f, a0, av), juce::PathStrokeType (1.6f)); }
        // the pointer, printed on the skirt, running out from the cap
        g.setColour (c.interpolatedWith (juce::Colours::white, 0.2f));
        const float p0 = sr * 0.42f, p1 = sr - 2.0f;
        g.drawLine (ctr.x + p0 * std::sin (av), ctr.y - p0 * std::cos (av), ctr.x + p1 * std::sin (av), ctr.y - p1 * std::cos (av), 2.2f);
        const R cap = body.reduced (body.getWidth() * 0.3f);
        g.setGradientFill (juce::ColourGradient (col ("#4a4640"), cap.getCentreX(), cap.getY(), col ("#121110"), cap.getCentreX(), cap.getBottom(), false));
        g.fillEllipse (cap);
        g.setColour (juce::Colours::white.withAlpha (0.22f)); g.fillEllipse (cap.getX() + cap.getWidth() * 0.25f, cap.getY() + cap.getHeight() * 0.12f, cap.getWidth() * 0.5f, cap.getHeight() * 0.22f);
        return;
    }
    // MACHINED: a turned-metal knob - a thin bronze arc outside, a brushed silver-dark face with fine circular grain, a bronze index line
    {
        if (pos > 0.005f)
        {
            g.setColour (c.withAlpha (0.18f)); g.strokePath (arcPath (r - 1.5f, a0, av), juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (c); g.strokePath (arcPath (r - 1.5f, a0, av), juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.strokePath (arcPath (r - 1.5f, av, a1), juce::PathStrokeType (1.0f));
        g.setGradientFill (juce::ColourGradient (col ("#5a5853"), body.getX(), body.getY(), col ("#1a1918"), body.getRight(), body.getBottom(), false));
        g.fillEllipse (body);
        const R face = body.reduced (3.0f);
        g.setGradientFill (juce::ColourGradient (col ("#3b3a37"), face.getX(), face.getY(), col ("#1d1c1b"), face.getRight(), face.getBottom(), false));
        g.fillEllipse (face);
        for (float rr = 3.0f; rr < face.getWidth() * 0.5f; rr += 2.2f)   // the turned grain
        {
            g.setColour (juce::Colours::white.withAlpha (0.025f + 0.02f * std::sin (rr)));
            g.drawEllipse (ctr.x - rr, ctr.y - rr, rr * 2, rr * 2, 0.6f);
        }
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.16f), face.getX(), face.getY(), juce::Colours::white.withAlpha (0.0f), face.getCentreX(), face.getCentreY(), true);
        g.setGradientFill (sheen); g.fillEllipse (face);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (face, 1.0f);
        const float i0 = face.getWidth() * 0.18f, i1 = face.getWidth() * 0.47f;
        g.setColour (c.interpolatedWith (juce::Colours::white, 0.15f));
        g.drawLine (ctr.x + i0 * std::sin (av), ctr.y - i0 * std::cos (av), ctr.x + i1 * std::sin (av), ctr.y - i1 * std::cos (av), 2.4f);
    }
}   // HELPER's knobs: the same family, but the arc is a ring of lit segments and the cap glows in the tool's colour
static int gKnobSteps = 0;   // > 0: a stepped knob (like the Maselec's precision switches) - detent dots, not ticks
static const char* const* gKnobLabels = nullptr;   // optional: a name at each step, like the markings round a rotary switch
static void modernKnob (juce::Graphics& g, R b, float pos, juce::Colour c)
{
    pos = juce::jlimit (0.0f, 1.0f, pos);
    const auto ctr = b.getCentre();
    const float r = b.getWidth() * 0.5f, a0 = -135.0f * pi / 180.0f, a1 = 135.0f * pi / 180.0f, av = a0 + (a1 - a0) * pos;
    if (gKnobSteps > 1)
    {
        for (int i = 0; i < gKnobSteps; ++i)
        {
            const float f = (float) i / (float) (gKnobSteps - 1), t = a0 + (a1 - a0) * f, rr = r + 6.0f;
            const bool sel = std::abs (f - pos) < 0.5f / (float) (gKnobSteps - 1);
            g.setColour (sel ? c.interpolatedWith (juce::Colours::white, 0.3f) : juce::Colours::white.withAlpha (f <= pos ? 0.45f : 0.2f));
            const float d = sel ? 3.4f : 2.2f;
            g.fillEllipse (ctr.x + rr * std::sin (t) - d * 0.5f, ctr.y - rr * std::cos (t) - d * 0.5f, d, d);
            if (gKnobLabels != nullptr)
            {
                const float rl = r + 17.0f;
                text (g, gKnobLabels[i], font (Face::CondMedium, 8.5f, 0.6f), sel ? c.interpolatedWith (juce::Colours::white, 0.4f) : juce::Colours::white.withAlpha (0.42f),
                      R (ctr.x + rl * std::sin (t) - 16, ctr.y - rl * std::cos (t) - 5, 32, 10), juce::Justification::centred);
            }
        }
    }
    // ticks
    for (int i = 0; i <= (gKnobSteps > 1 ? -1 : 10); ++i)
    {
        const float t = a0 + (a1 - a0) * (float) i / 10.0f;
        const float r0 = r + 3.0f, r1 = r + (i % 5 == 0 ? 8.0f : 6.0f);
        g.setColour (juce::Colours::white.withAlpha (i % 5 == 0 ? 0.45f : 0.22f));
        g.drawLine (ctr.x + r0 * std::sin (t), ctr.y - r0 * std::cos (t), ctr.x + r1 * std::sin (t), ctr.y - r1 * std::cos (t), 1.0f);
    }
    if (gHelperKnob)   // HELPER: lit segments round the knob, like a row of little lamps
    {
        const int nSeg = 27;
        for (int i = 0; i < nSeg; ++i)
        {
            const float f = (float) i / (float) (nSeg - 1), t = a0 + (a1 - a0) * f;
            const bool lit = f <= pos + 0.001f;
            const float r0 = r - 4.0f, r1 = r + 1.0f;
            if (lit) { g.setColour (c.withAlpha (0.22f)); g.drawLine (ctr.x + r0 * std::sin (t), ctr.y - r0 * std::cos (t), ctr.x + r1 * std::sin (t), ctr.y - r1 * std::cos (t), 5.0f); }
            g.setColour (lit ? c.interpolatedWith (juce::Colours::white, f > pos - 0.04f ? 0.45f : 0.1f) : juce::Colours::white.withAlpha (0.10f));
            g.drawLine (ctr.x + r0 * std::sin (t), ctr.y - r0 * std::cos (t), ctr.x + r1 * std::sin (t), ctr.y - r1 * std::cos (t), 2.0f);
        }
    }
    else
    // the value arc, glowing
    {
        juce::Path track; track.addCentredArc (ctr.x, ctr.y, r - 1.5f, r - 1.5f, 0.0f, a0, a1, true);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (pos > 0.005f)
        {
            juce::Path arc; arc.addCentredArc (ctr.x, ctr.y, r - 1.5f, r - 1.5f, 0.0f, a0, av, true);
            g.setColour (c.withAlpha (0.25f)); g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (c); g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    // the body: a knurled rim, then a matte cap
    const R body = b.reduced (5.0f);
    { juce::Path p; p.addEllipse (body.translated (0, 3)); cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 6, {}), p); }
    g.setGradientFill (juce::ColourGradient (col ("#3a3b41"), body.getX(), body.getY(), col ("#121316"), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        const float rr = body.getWidth() * 0.5f;
        for (int i = 0; i < 48; ++i)
        {
            const float t = (float) i / 48.0f * juce::MathConstants<float>::twoPi + av;
            g.drawLine (ctr.x + (rr - 2.5f) * std::sin (t), ctr.y - (rr - 2.5f) * std::cos (t), ctr.x + rr * std::sin (t), ctr.y - rr * std::cos (t), 0.8f);
        }
    }
    const R cap = body.reduced (body.getWidth() * 0.12f);
    g.setGradientFill (juce::ColourGradient (col ("#2e2f35"), cap.getCentreX(), cap.getY(), col ("#18191d"), cap.getCentreX(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (cap.reduced (0.5f), 1.0f);
    if (gHelperKnob)
    {
        // a thin ring of the tool's colour round the cap, and a glowing dot for the index
        g.setColour (c.withAlpha (0.35f)); g.drawEllipse (cap.reduced (1.5f), 1.2f);
        const float ir = cap.getWidth() * 0.34f;
        const P d (ctr.x + ir * std::sin (av), ctr.y - ir * std::cos (av));
        g.setColour (c.withAlpha (0.35f)); g.fillEllipse (d.x - 5, d.y - 5, 10, 10);
        g.setColour (c.interpolatedWith (juce::Colours::white, 0.55f)); g.fillEllipse (d.x - 2.6f, d.y - 2.6f, 5.2f, 5.2f);
        return;
    }
    // the index
    const float i0 = cap.getWidth() * 0.12f, i1 = cap.getWidth() * 0.46f;
    g.setColour (col ("#f4f1ea"));
    g.drawLine (ctr.x + i0 * std::sin (av), ctr.y - i0 * std::cos (av), ctr.x + i1 * std::sin (av), ctr.y - i1 * std::cos (av), 2.2f);
}

// glass: a dark tinted pane whose opacity follows alphaAt (u across, t down, both 0..1) - drawn as a picture so it can
// fade in any direction
static void drawGlass (juce::Graphics& g, R s, std::function<float (float, float)> alphaAt, float tint = 0.92f)
{
    const float k = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int iw = juce::jmax (2, juce::roundToInt (s.getWidth() * k)), ih = juce::jmax (2, juce::roundToInt (s.getHeight() * k));
    juce::Image im (juce::Image::ARGB, iw, ih, true);
    {
        juce::Graphics ig (im);
        ig.addTransform (juce::AffineTransform::translation (-s.getX(), -s.getY()).scaled (k));
        ig.setGradientFill (juce::ColourGradient (col ("#23242c", tint), 0, s.getY(), col ("#121317", tint), 0, s.getBottom(), false));
        ig.fillRect (s);
        ig.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.045f), s.getX(), s.getY(), juce::Colours::white.withAlpha (0.0f), s.getRight(), s.getBottom(), false));
        ig.fillRect (s);
    }
    juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < ih; ++y)
        for (int x = 0; x < iw; ++x)
            reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->multiplyAlpha (juce::jlimit (0.0f, 1.0f, alphaAt ((float) x / (float) (iw - 1), (float) y / (float) (ih - 1))));
    g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / k).translated (s.getX(), s.getY()));
}
static float smoothStep (float v) { v = juce::jlimit (0.0f, 1.0f, v); return v * v * (3.0f - 2.0f * v); }

// a glass button: clear with a hint of violet; the dot is what says it's a button (lit when on). flip = a two-way
// button showing its current setting, with a small swap mark
static void glassButton (juce::Graphics& g, R r, const juce::String& t, bool on, bool flip = false)
{
    g.setColour (cleanC.withAlpha (on ? 0.10f : 0.05f)); g.fillRoundedRectangle (r, 6);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, r.getY(), juce::Colours::white.withAlpha (0.0f), 0, r.getCentreY(), false));
    g.fillRoundedRectangle (r.reduced (1), 5);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.16f : 0.1f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1.0f);
    const P d (r.getX() + 12, r.getCentreY());
    if (on) { g.setColour (cleanC.withAlpha (0.35f)); g.fillEllipse (d.x - 6, d.y - 6, 12, 12); }
    g.setColour (on ? cleanC.interpolatedWith (juce::Colours::white, 0.2f) : juce::Colours::white.withAlpha (0.22f)); g.fillEllipse (d.x - 3, d.y - 3, 6, 6);
    text (g, t, font (Face::CondMedium, 9.5f, 1.3f), on ? ink : col ("#b9b3a7"), r.withTrimmedLeft (22).withTrimmedRight (flip ? 18 : 6), juce::Justification::centredLeft);
    if (flip)
    {
        juce::Path sw; const float sx = r.getRight() - 15, sy = r.getCentreY();
        sw.startNewSubPath (sx, sy - 3); sw.lineTo (sx + 8, sy - 3); sw.lineTo (sx + 5, sy - 6);
        sw.startNewSubPath (sx + 8, sy + 3); sw.lineTo (sx, sy + 3); sw.lineTo (sx + 3, sy + 6);
        g.setColour (juce::Colours::white.withAlpha (0.45f)); g.strokePath (sw, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

// CLEAN: the compressor as a modern piece of hardware, floating in the middle of the screen (like Pro-C's controls),
// laid out like the vintage gear - name and switches, a gain-reduction meter, the knobs, then the routing buttons
void Canvas::paintCleanUnit (juce::Graphics& g)
{
    const float pw = 1080.0f, ph = 168.0f, cyMid = 286.0f;   // the unit sits a little high so the sidechain drawer fits under it
    const R b (PW * 0.5f - pw * 0.5f, cyMid - ph * 0.5f, pw, ph);
    paintSidechainDrawer (g, b);
    // the plate: dark anodised aluminium, a fine brushed grain, a lit edge in the tab's colour, four hex screws
    {
        juce::Path p; p.addRoundedRectangle (b, 12);
        cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 26, { 0, 10 }), p);
        g.setGradientFill (juce::ColourGradient (col ("#25262c", 0.94f), 0, b.getY(), col ("#141519", 0.94f), 0, b.getBottom(), false));
        g.fillPath (p);
        g.saveState(); g.reduceClipRegion (p);
        juce::Random rnd (7);
        for (float y = b.getY() + 1; y < b.getBottom(); y += 1.5f)
        { g.setColour (juce::Colours::white.withAlpha (0.008f + 0.012f * rnd.nextFloat())); g.fillRect (b.getX(), y, b.getWidth(), 0.6f); }
        g.setGradientFill (juce::ColourGradient (cleanC.withAlpha (0.0f), b.getX(), 0, cleanC.withAlpha (0.0f), b.getRight(), 0, false));
        juce::ColourGradient edge (cleanC.withAlpha (0.0f), b.getX(), 0, cleanC.withAlpha (0.0f), b.getRight(), 0, false);
        edge.addColour (0.5, cleanC.withAlpha (0.9f));
        g.setGradientFill (edge); g.fillRect (b.getX() + 40, b.getY(), b.getWidth() - 80, 1.5f);
        g.restoreState();
        g.setColour (juce::Colours::white.withAlpha (0.09f)); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1.0f);
        for (auto s : { P (b.getX() + 12, b.getY() + 12), P (b.getRight() - 12, b.getY() + 12), P (b.getX() + 12, b.getBottom() - 12), P (b.getRight() - 12, b.getBottom() - 12) })
        {
            g.setGradientFill (juce::ColourGradient (col ("#55575e"), s.x - 3, s.y - 3, col ("#1c1d21"), s.x + 3, s.y + 3, false));
            g.fillEllipse (s.x - 4, s.y - 4, 8, 8);
            juce::Path hx; for (int i = 0; i < 6; ++i) { const float t = (float) i * pi / 3.0f; if (i == 0) hx.startNewSubPath (s.x + 2 * std::cos (t), s.y + 2 * std::sin (t)); else hx.lineTo (s.x + 2 * std::cos (t), s.y + 2 * std::sin (t)); }
            hx.closeSubPath(); g.setColour (col ("#0d0d10")); g.fillPath (hx);
        }
    }
    const float x0 = b.getX() + 28, top = b.getY() + 22;
    // the name, the engine, and the character switches as backlit push-buttons
    {
        text (g, "LE-RC", font (Face::Light, 26, 2.0f), ink, R (x0, top - 4, 160, 30));
        text (g, "PROGRAM COMPRESSOR", font (Face::CondMedium, 9, 1.8f), grey, R (x0 + 2, top + 26, 170, 11));
        const R eng (x0, top + 46, 150, 22);
        fillRound (g, eng, 6, col ("#121317")); strokeRound (g, eng, 6, col ("#34353b"));
        text (g, U8 ("Engine \xc2\xb7 LE-RC"), font (Face::Regular, 11), ink, eng.reduced (10, 0).withTrimmedRight (12));
        icon (g, Icon::TriDown, R (eng.getRight() - 16, eng.getCentreY() - 4, 8, 8), grey);
        auto push = [&] (R r, const char* t, bool on)
        {
            fillRound (g, r, 5, on ? col ("#2a2550") : col ("#17181c"));
            strokeRound (g, r, 5, on ? cleanC.withAlpha (0.8f) : col ("#34353b"));
            if (on) { g.setColour (cleanC.withAlpha (0.18f)); g.fillRoundedRectangle (r.expanded (2), 6); }
            text (g, t, font (Face::CondMedium, 10, 1.4f), on ? ink : grey, r, juce::Justification::centred);
        };
        const float bw = 72;
        push (R (x0, top + 78, bw, 22), "GLIDE", true);  push (R (x0 + bw + 6, top + 78, bw, 22), "SNAP", false);
        push (R (x0, top + 104, bw, 22), "COLOR", true); push (R (x0 + bw + 6, top + 104, bw, 22), "PURE", false);
    }
    // the gain-reduction meter: a modern needle meter behind smoked glass
    const float grNow = hGr.empty() ? 0.0f : hGr[(size_t) ((hPos - 1 + (int) hGr.size()) % (int) hGr.size())];
    {
        const R mw (x0 + 186, b.getY() + 22, 150, 124);
        fillRound (g, mw, 8, col ("#0b0b0e")); strokeRound (g, mw, 8, juce::Colours::white.withAlpha (0.1f));
        const P pc (mw.getCentreX(), mw.getBottom() - 16);
        const float rr = 92.0f, s0 = -50.0f * pi / 180.0f, s1 = 50.0f * pi / 180.0f;
        g.saveState(); g.reduceClipRegion (mw.reduced (2).toNearestInt());
        auto fs = font (Face::Regular, 8.5f);
        const int marks[7] { 0, 1, 2, 3, 5, 10, 20 };
        auto angOf = [&] (float gr) { return s1 - (s1 - s0) * std::log1p (juce::jlimit (0.0f, 20.0f, gr)) / std::log1p (20.0f); };
        for (int mk : marks)
        {
            const float t = angOf ((float) mk);
            g.setColour (mk >= 10 ? grC.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.55f));
            g.drawLine (pc.x + (rr - 8) * std::sin (t), pc.y - (rr - 8) * std::cos (t), pc.x + rr * std::sin (t), pc.y - rr * std::cos (t), 1.2f);
            text (g, juce::String (mk), fs, juce::Colours::white.withAlpha (0.6f), R (pc.x + (rr - 20) * std::sin (t) - 10, pc.y - (rr - 20) * std::cos (t) - 6, 20, 12), juce::Justification::centred);
        }
        { juce::Path arc; arc.addCentredArc (pc.x, pc.y, rr, rr, 0.0f, s0, s1, true); g.setColour (juce::Colours::white.withAlpha (0.25f)); g.strokePath (arc, juce::PathStrokeType (1.0f)); }
        text (g, "GR dB", font (Face::CondMedium, 9.5f, 1.6f), cleanC, R (mw.getX() + 12, mw.getBottom() - 22, 50, 12), juce::Justification::centredLeft);
        const float t = angOf (grNow);
        g.setColour (cleanC.withAlpha (0.3f)); g.drawLine (pc.x, pc.y, pc.x + (rr - 4) * std::sin (t), pc.y - (rr - 4) * std::cos (t), 4.0f);
        g.setColour (col ("#f4f1ea")); g.drawLine (pc.x, pc.y, pc.x + (rr - 4) * std::sin (t), pc.y - (rr - 4) * std::cos (t), 1.4f);
        g.setColour (col ("#26272c")); g.fillEllipse (pc.x - 7, pc.y - 7, 14, 14);
        g.restoreState();
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, mw.getY(), juce::Colours::white.withAlpha (0.0f), 0, mw.getCentreY(), false));
        g.fillRoundedRectangle (mw.reduced (2), 7);
    }
    // the knobs: THRESHOLD bigger, the rest in a row, labels above and values below like the vintage gear
    {
        struct K { const char* lab; juce::String val; float pos; float size; };
        const K ks[7] { { "THRESHOLD", MINUS + juce::String (-mThr, 1) + " dB", 1.0f - (-mThr) / 60.0f, 72 },
                        { "RATIO", juce::String (mRatio, 1).trimCharactersAtEnd (".0") + ":1", 0.35f, 52 },
                        { "KNEE", juce::String (mKnee, 0) + " dB", 0.3f, 52 }, { "ATTACK", "8.0 ms", 0.4f, 52 },
                        { "RELEASE", "Auto", 0.55f, 52 }, { "MAKE-UP", "+4.0 dB", 0.6f, 52 }, { "MIX", "100%", 1.0f, 52 } };
        float x = x0 + 360;
        const float midY = b.getCentreY() + 4;
        for (auto& k : ks)
        {
            const float colW = k.size + 20;
            text (g, k.lab, font (Face::CondMedium, 9.5f, 1.4f), col ("#b9b3a7"), R (x - 8, midY - k.size * 0.5f - 22, colW + 16, 11), juce::Justification::centred);
            modernKnob (g, R (x + (colW - k.size) * 0.5f, midY - k.size * 0.5f, k.size, k.size), k.pos, cleanC);
            text (g, k.val, font (Face::Regular, 11.5f), ink, R (x - 8, midY + k.size * 0.5f + 10, colW + 16, 13), juce::Justification::centred);
            x += colW + (k.size > 60 ? 10.0f : 0.0f);
        }
    }
    // routing, on the right: backlit buttons
    {
        const float rx = b.getRight() - 28 - 96;
        auto btn = [&] (float y, const juce::String& t, bool on)
        {
            const R r (rx, y, 96, 24);
            fillRound (g, r, 6, on ? col ("#2a2550") : col ("#17181c")); strokeRound (g, r, 6, on ? cleanC.withAlpha (0.8f) : col ("#34353b"));
            g.setColour (on ? cleanC : col ("#3a3936")); g.fillEllipse (r.getX() + 9, r.getCentreY() - 3, 6, 6);
            text (g, t, font (Face::CondMedium, 10, 1.3f), on ? ink : grey, r.withTrimmedLeft (20), juce::Justification::centredLeft);
        };
        btn (b.getY() + 30, "AUTO GAIN", true);
        btn (b.getY() + 60, "DELTA", false);
        btn (b.getY() + 90, "OVERSAMPLE", false);
        btn (b.getY() + 120, U8 ("SIDECHAIN \xe2\x96\xbe"), true);
    }
}

// the sidechain drawer, under the unit (like Pro-C 3's): what the compressor listens to -
// its source, a listen button, a small EQ for the detector, its level, stereo link / mid-side and look-ahead
void Canvas::paintSidechainDrawer (juce::Graphics& g, R unit)
{
    const R s (unit.getX() + 36, unit.getBottom() - 10, unit.getWidth() - 72, 136);
    drawGlass (g, s.withTrimmedTop (-40).withTrimmedBottom (-12), [] (float u, float t)
               { return smoothStep (t / 0.4f) * smoothStep (u / 0.08f) * smoothStep ((1.0f - u) / 0.08f); });
    const float y0 = s.getY() + 22;
    auto fh = font (Face::CondMedium, 9.5f, 1.6f), fv = font (Face::Regular, 10.5f);
    // source and listen
    const float x0 = s.getX() + 22;
    {
        text (g, "SIDECHAIN", font (Face::CondMedium, 11, 2.0f), ink, R (x0, y0, 120, 13));
        text (g, "what the compressor hears", font (Face::Regular, 9.5f), grey, R (x0, y0 + 15, 160, 12));
        const R seg (x0, y0 + 36, 150, 22);
        fillRound (g, seg, 6, col ("#0e0f12")); strokeRound (g, seg, 6, col ("#2e2f35"));
        for (int i = 0; i < 2; ++i)
        {
            const R c (seg.getX() + (float) i * 75, seg.getY(), 75, 22);
            if (i == 0) { fillRound (g, c.reduced (2), 4, col ("#2a2550")); strokeRound (g, c.reduced (2), 4, cleanC.withAlpha (0.7f)); }
            text (g, i == 0 ? "INTERNAL" : "EXTERNAL", font (Face::CondMedium, 9.5f, 1.2f), i == 0 ? ink : grey, c, juce::Justification::centred);
        }
        const R ls (x0, y0 + 66, 150, 22);
        fillRound (g, ls, 6, col ("#17181c")); strokeRound (g, ls, 6, col ("#34353b"));
        g.setColour (col ("#3a3936")); g.fillEllipse (ls.getX() + 9, ls.getCentreY() - 3, 6, 6);
        text (g, "LISTEN TO SIDECHAIN", font (Face::CondMedium, 9.5f, 1.2f), grey, ls.withTrimmedLeft (20), juce::Justification::centredLeft);
    }
    // the detector's EQ: high-pass, a focus bell and low-pass on a small frequency screen
    {
        const R e (x0 + 176, s.getY() + 14, 320, s.getHeight() - 28);
        fillRound (g, e, 8, col ("#0b0b0e")); strokeRound (g, e, 8, juce::Colours::white.withAlpha (0.08f));
        auto fxS = [&] (float hz) { return e.getX() + 8 + (e.getWidth() - 16) * std::log10 (hz / 20.0f) / 3.0f; };
        const float midY = e.getCentreY() + 4, dbPx = 3.2f;
        g.saveState(); g.reduceClipRegion (e.reduced (1).toNearestInt());
        for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            g.setColour (juce::Colours::white.withAlpha (hz == 100.0f || hz == 1000.0f || hz == 10000.0f ? 0.08f : 0.04f));
            g.fillRect (std::round (fxS (hz)), e.getY(), 1.0f, e.getHeight());
        }
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (e.getX(), midY, e.getWidth(), 1.0f);
        // what the sidechain sounds like (faint)
        {
            juce::Path sp; sp.startNewSubPath (e.getX(), e.getBottom());
            for (int i = 0; i <= 80; ++i)
            {
                const float t = (float) i / 80.0f, hz = 20.0f * std::pow (1000.0f, t);
                const float lvl = -10.0f - 14.0f * std::abs (std::log10 (hz / 300.0f)) + 3.0f * std::sin ((float) i * 1.7f);
                sp.lineTo (fxS (hz), juce::jlimit (e.getY(), e.getBottom(), midY - (lvl + 12.0f) * 1.6f));
            }
            sp.lineTo (e.getRight(), e.getBottom()); sp.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillPath (sp);
        }
        // the filter curve: 12 dB/oct high-pass at 80 Hz, +3 dB bell at 3 kHz, low-pass at 14 kHz
        auto respDb = [] (float hz)
        {
            const float hp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (80.0f / hz, 4.0f)));
            const float lp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hz / 14000.0f, 4.0f)));
            const float oct = std::log2 (hz / 3000.0f);
            return hp + lp + 3.0f * std::exp (-oct * oct * 1.4f);
        };
        juce::Path c;
        for (int i = 0; i <= 200; ++i)
        {
            const float hz = 20.0f * std::pow (1000.0f, (float) i / 200.0f);
            const float y = juce::jlimit (e.getY() + 2, e.getBottom() - 2, midY - respDb (hz) * dbPx);
            if (i == 0) c.startNewSubPath (fxS (hz), y); else c.lineTo (fxS (hz), y);
        }
        juce::Path cf (c); cf.lineTo (e.getRight(), midY); cf.lineTo (e.getX(), midY); cf.closeSubPath();
        g.setColour (cleanC.withAlpha (0.12f)); g.fillPath (cf);
        g.setColour (cleanC); g.strokePath (c, juce::PathStrokeType (1.6f));
        const struct { float hz; const char* t; } nodes[3] { { 80.0f, "HP 80 Hz" }, { 3000.0f, "3 kHz +3" }, { 14000.0f, "LP 14k" } };
        for (int i = 0; i < 3; ++i)
        {
            const float x = fxS (nodes[i].hz), y = juce::jlimit (e.getY() + 8, e.getBottom() - 8, midY - respDb (nodes[i].hz) * dbPx);
            g.setColour (cleanC.withAlpha (0.3f)); g.fillEllipse (x - 8, y - 8, 16, 16);
            g.setColour (cleanC); g.fillEllipse (x - 5, y - 5, 10, 10);
            text (g, juce::String (i + 1), font (Face::SemiBold, 8), col ("#16161a"), R (x - 5, y - 5, 10, 10), juce::Justification::centred);
            text (g, nodes[i].t, fv, col ("#b9b3a7"), R (x - 40, y + 8, 80, 12), juce::Justification::centred);
        }
        g.restoreState();
        text (g, "DETECTOR EQ", fh, grey, R (e.getX() + 10, e.getY() + 6, 100, 11));
    }
    // level, link and look-ahead
    {
        struct K { const char* lab; const char* val; float pos; };
        const K ks[4] { { "SC LEVEL", "0.0 dB", 0.5f }, { "LINK", "100%", 1.0f }, { "M / S", "Stereo", 0.5f }, { "LOOK-AHEAD", "2.0 ms", 0.2f } };
        float x = x0 + 512;
        for (auto& k : ks)
        {
            const float sz = 38, colW = 56;
            text (g, k.lab, fh, col ("#b9b3a7"), R (x - 8, y0 - 6, colW + 16, 11), juce::Justification::centred);
            modernKnob (g, R (x + (colW - sz) * 0.5f, y0 + 14, sz, sz), k.pos, cleanC);
            text (g, k.val, fv, ink, R (x - 8, y0 + 64, colW + 16, 13), juce::Justification::centred);
            x += colW + 4;
        }
    }
}

// CLEAN, Helper style: the waveform has the middle of the screen; threshold and ratio on the left, attack and release
// on the right, in modern glass panels; the rest in a slim strip along the bottom
void Canvas::paintCleanTab (juce::Graphics& g)
{
    const bool mla = penv ("PRESS_MLA") != nullptr;   // LE-MLA: the Maselec MLA-2 - drive into a fixed threshold, stepped controls
    if (! gCompMode)   // the curve the knee drawer draws: the engine's own
    {
        mThr = proc.param ("cThresh"); mRatio = proc.param ("cRatio"); mKnee = proc.param ("cKnee");
        if (penv ("PRESS_DS1") != nullptr) mKnee = proc.param ("cDsKnee") * 2.0f * std::abs (mThr);
        if (mla) { pressure::CleanSettings cs; cs.mlaIn = (int) proc.param ("cMlaIn"); mThr = pressure::CleanCompressor::mlaThreshold (cs);
                   mRatio = pressure::mlaRatios[juce::jlimit (0, 8, (int) proc.param ("cMlaRatio"))]; mKnee = 8.0f; }
        if (penv ("PRESS_PRO") != nullptr) mKnee += pressure::styleInfo ((int) proc.param ("cProStyle")).kneeAdd;
    }
    const float pw = 196.0f, top = 46.0f, ph = 588.0f;
    // the side panels: glass running the full height of the screen - nearly clear where it meets the waveform and at the
    // very top and bottom, most solid behind the knobs, then easing off again at the outer edge
    auto panel = [&] (R s0, bool right)
    {
        const R s (s0.getX(), 0, s0.getWidth(), 670);
        const float k = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const int iw = juce::roundToInt (s.getWidth() * k), ih = juce::roundToInt (s.getHeight() * k);
        juce::Image im (juce::Image::ARGB, iw, ih, true);
        {
            juce::Graphics ig (im);
            ig.addTransform (juce::AffineTransform::translation (-s.getX(), -s.getY()).scaled (k));
            ig.setGradientFill (juce::ColourGradient (col ("#23242c", 0.92f), 0, s.getY(), col ("#121317", 0.92f), 0, s.getBottom(), false));
            ig.fillRect (s);
            // a faint sheen, like light on glass
            ig.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), s.getX(), s.getY() + 120, juce::Colours::white.withAlpha (0.0f), s.getRight(), s.getY() + 360, false));
            ig.fillRect (s);
        }
        {
            juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
            auto smooth = [] (float v) { v = juce::jlimit (0.0f, 1.0f, v); return v * v * (3.0f - 2.0f * v); };
            std::vector<float> fx ((size_t) iw);
            for (int x = 0; x < iw; ++x)
            {
                const float u = right ? (float) x / (float) (iw - 1) : 1.0f - (float) x / (float) (iw - 1);   // 0 = the waveform side, 1 = the outer edge
                const float rise = 0.1f + 0.9f * smooth (u / 0.42f), ease = 1.0f - 0.45f * smooth ((u - 0.82f) / 0.18f);
                fx[(size_t) x] = rise * ease;
            }
            for (int y = 0; y < ih; ++y)
            {
                const float t = (float) y / (float) (ih - 1);
                const float fy = smooth (t / 0.17f) * smooth ((1.0f - t) / 0.15f);
                for (int x = 0; x < iw; ++x)
                    reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->multiplyAlpha (fx[(size_t) x] * fy);
            }
        }
        g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / k).translated (s.getX(), s.getY()));
    };
    auto title = [&] (R s, const char* t)
    {
        text (g, t, font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (s.getX(), s.getY(), s.getWidth(), 12), juce::Justification::centred);
    };
    auto bigKnob = [&] (float cx, float y, float sz, const char* lab, const juce::String& val, float pos)
    {
        title (R (cx - 80, y, 160, 12), lab);
        modernKnob (g, R (cx - sz * 0.5f, y + 24, sz, sz), pos, cleanC);
        text (g, val, font (Face::Light, sz > 90 ? 20.0f : 16.0f, 0.5f), ink, R (cx - 80, y + 24 + sz + 12, 160, 22), juce::Justification::centred);
        return y + 24 + sz + 12 + 22;
    };
    // PRESSURE: a knob tied to its parameter (kind 3 = drawn the other way round: up = a lower value)
    auto bigKnobP = [&] (float cx, float y, float sz, const char* lab, const juce::String& pid, int kind = 0)
    {
        const float pos = kind == 3 ? 1.0f - pn (pid) : pn (pid);
        pctl (R (cx - sz * 0.5f - 6, y + 18, sz + 12, sz + 12), pid, kind);
        return bigKnob (cx, y, sz, lab, pfmt (pid), pos);
    };
    auto smallKnobP = [&] (float kx, float y, float sz, float w, const char* lab, const juce::String& pid, float fsz = 14.0f)
    {
        title (R (kx - w * 0.5f, y, w, 12), lab);
        modernKnob (g, R (kx - sz * 0.5f, y + 20, sz, sz), pn (pid), cleanC);
        text (g, pfmt (pid), font (Face::Light, fsz, fsz < 13.0f ? 0.3f : 0.5f), ink, R (kx - w * 0.5f, y + 20 + sz + 8, w, fsz + 4), juce::Justification::centred);
        pctl (R (kx - sz * 0.5f - 4, y + 16, sz + 8, sz + 8), pid, 0);
    };
    auto buttonP = [&] (R r, const char* t, const juce::String& pid) { glassButton (g, r, t, pon (pid)); pctl (r, pid, 1); };
    // ---- VINTAGE (glass): the unit's controls on the glass panels, its faceplate small at the bottom
    if (gCompMode)
    {
        const int unit = juce::jlimit (0, kVinUnits - 1, penv ("PRESS_VUNIT") ? std::atoi (penv ("PRESS_VUNIT")) : preview);
        // each unit's own controls - the real panel's knobs, rotary switches, push buttons and toggles, in glass
        mThr = -18.0f; mRatio = 4.0f; mKnee = unit == 1 || unit == 5 || unit == 12 || unit == 13 ? 14.0f : unit == 2 || unit == 11 ? 10.0f : unit == 8 ? 8.0f : 4.0f;   // vari-mu and tube units: soft knees
        auto stepped = [&] (float cx, float y, float sz, const char* lab, const char* val, int n, int at, const char* const* labs)
        {
            title (R (cx - 80, y, 160, 12), lab);   // the title sits higher, so the switch's markings have room
            gKnobSteps = n; gKnobLabels = labs;
            const float yy = bigKnob (cx, y + 14, sz, "", U8 (val), n > 1 ? (float) at / (float) (n - 1) : 0.5f);
            gKnobSteps = 0; gKnobLabels = nullptr;
            return yy;
        };
        // push buttons: little square caps with a light (1176 ratio and meter, SSL / Neve IN)
        auto pushRow = [&] (R area, const char* tl, std::initializer_list<const char*> labs, int lit, bool all = false, int mask = 0)
        {
            if (tl != nullptr) title (R (area.getX(), area.getY(), area.getWidth(), 12), tl);
            const int n = (int) labs.size();
            const float gp = 6, w = (area.getWidth() - gp * (float) (n - 1)) / (float) n, y = area.getY() + (tl != nullptr ? 18.0f : 0.0f);
            int i = 0;
            for (auto* l : labs)
            {
                const R b (area.getX() + (float) i * (w + gp), y, w, 30);
                const bool on = all || i == lit || ((mask >> i) & 1) != 0;
                g.setGradientFill (juce::ColourGradient (col (on ? "#2c2c33" : "#1d1d22", 0.95f), 0, b.getY(), col ("#121216", 0.95f), 0, b.getBottom(), false));
                g.fillRoundedRectangle (b, 4);
                g.setColour (on ? cleanC.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.14f)); g.drawRoundedRectangle (b.reduced (0.5f), 4, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (b.getX() + 3, b.getY() + 1.5f, b.getWidth() - 6, 1.0f);   // the cap's top edge catching light
                const P led (b.getCentreX(), b.getY() + 7);
                if (on) { g.setColour (cleanC.withAlpha (0.35f)); g.fillEllipse (led.x - 5, led.y - 5, 10, 10); }
                g.setColour (on ? cleanC.interpolatedWith (juce::Colours::white, 0.35f) : juce::Colours::white.withAlpha (0.15f)); g.fillEllipse (led.x - 2, led.y - 2, 4, 4);
                text (g, l, font (Face::CondMedium, 9.5f, 0.8f), on ? ink : grey, R (b.getX(), b.getY() + 13, b.getWidth(), 13), juce::Justification::centred);
                ++i;
            }
            return y + 30;
        };
        // a bat toggle: a metal lever that leans towards the position it's in (API, Neve, Manley, Gates)
        auto batToggle = [&] (R area, const char* tl, std::initializer_list<const char*> labs, int at)
        {
            title (R (area.getX(), area.getY(), area.getWidth(), 12), tl);
            const int n = (int) labs.size();
            const float y = area.getY() + 18, w = area.getWidth() / (float) n;
            int i = 0;
            for (auto* l : labs)
            {
                text (g, l, font (Face::CondMedium, 9, 1.0f), i == at ? cleanC.interpolatedWith (juce::Colours::white, 0.35f) : grey, R (area.getX() + w * (float) i, y, w, 11), juce::Justification::centred);
                ++i;
            }
            const P nut (area.getCentreX(), y + 34);
            const float target = area.getX() + w * ((float) at + 0.5f);
            const float ang = std::atan2 (target - nut.x, 22.0f);
            const P tip (nut.x + 20.0f * std::sin (ang), nut.y - 20.0f * std::cos (ang));
            g.setGradientFill (juce::ColourGradient (col ("#3a3a42"), nut.x, nut.y - 9, col ("#141418"), nut.x, nut.y + 9, false));
            g.fillEllipse (nut.x - 9, nut.y - 9, 18, 18);
            g.setColour (juce::Colours::white.withAlpha (0.18f)); g.drawEllipse (nut.x - 9, nut.y - 9, 18, 18, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawLine (nut.x + 1, nut.y + 1, tip.x + 1, tip.y + 1, 5.0f);
            g.setGradientFill (juce::ColourGradient (col ("#f2f2f4"), nut.x - 3, 0, col ("#8a8c92"), nut.x + 3, 0, false));
            g.drawLine (nut.x, nut.y, tip.x, tip.y, 3.6f);
            g.setColour (col ("#e8e9ec")); g.fillEllipse (tip.x - 3.2f, tip.y - 3.2f, 6.4f, 6.4f);
            g.setColour (cleanC.withAlpha (0.6f)); g.fillEllipse (tip.x - 1.2f, tip.y - 1.2f, 2.4f, 2.4f);
            return nut.y + 14;
        };
        auto smallPair = [&] (float cx, float y, const char* l1, const char* v1, float p1, const char* l2, const char* v2, float p2)
        {
            const float sz = 56;
            const char* labs[2] { l1, l2 }; const char* vals[2] { v1, v2 }; const float ps[2] { p1, p2 };
            for (int i = 0; i < 2; ++i)
            {
                const float kx = cx + (i == 0 ? -46.0f : 46.0f);
                title (R (kx - 46, y, 92, 12), labs[i]);
                modernKnob (g, R (kx - sz * 0.5f, y + 20, sz, sz), ps[i], cleanC);
                text (g, U8 (vals[i]), font (Face::Light, 14, 0.5f), ink, R (kx - 46, y + 20 + sz + 8, 92, 18), juce::Justification::centred);
            }
            return y + 20 + sz + 8 + 18;
        };
        const R sL (0, top, pw, ph), sR (PW - pw, top, pw, ph);
        panel (sL, false); panel (sR, true);
        const float cxL = sL.getCentreX(), cxR = sR.getCentreX();
        float yL = sL.getY() + 62, yR = sR.getY() + 62;
        const R rowL (sL.getX() + 16, 0, pw - 32, 0), rowR (sR.getX() + 16, 0, pw - 32, 0);
        auto atY = [] (R r, float y, float h) { return r.withY (y).withHeight (h); };
        switch (unit)
        {
            case 0:   // LE-76 (1176): Input and Output knobs, Attack and Release, the ratio push buttons and the meter buttons
            {
                yL = bigKnob (cxL, yL, 104, "INPUT", "+22 dB", 0.55f) + 26;
                yL = pushRow (atY (rowL, yL, 48), "RATIO", { "4", "8", "12", "20" }, 0) + 10;
                glassButton (g, atY (rowL, yL, 22), "ALL BUTTONS IN", false);
                text (g, "the famous all-buttons smash", font (Face::Regular, 9.5f), grey, atY (rowL, yL + 28, 12), juce::Justification::centred);
                yL += 56;
                // REVISION: every 1176 there was, one button - each click steps to the next (A, B, C, D, E, F, G, H, LN)
                {
                    static const char* rv[9] { "A", "B", "C", "D", "E", "F", "G", "H", "LN" };
                    static const char* rn[9] { "BLUE STRIPE", "EARLY BLACK", "CLASS A", "BLACKFACE", "LOW NOISE", "BLACKFACE", "SILVER", "SILVER LN", "REISSUE" };
                    const int r = penv ("PRESS_REV") ? juce::jlimit (0, 8, std::atoi (penv ("PRESS_REV"))) : 0;
                    // each revision lights in its own colour - the look of that version
                    static const char* rc[9] { "#4f8dff",    // A: blue stripe
                                               "#e9e6df",    // B: early black - a plain white light
                                               "#ffb347",    // C: class A - warm amber
                                               "#f2ead8",    // D: blackface - soft ivory
                                               "#5fd38a",    // E: low noise - green
                                               "#d9c9a3",    // F: blackface - champagne
                                               "#c9d1da",    // G: silver
                                               "#a9d8ff",    // H: silver LN - ice blue
                                               "#ff5a4f" };  // LN: the reissue - red
                    const auto revC = col (rc[r]);
                    title (atY (rowL, yL, 12), "REVISION");
                    const R rb = atY (rowL, yL + 18, 26);
                    glassButton (g, rb, (juce::String ("REV ") + rv[r] + "  /  " + rn[r]).toRawUTF8(), true, true);
                    {   // its light, glowing the revision's colour (drawn over the button's own dot)
                        const P d (rb.getX() + 12, rb.getCentreY());
                        g.setColour (col ("#1a1a1f")); g.fillEllipse (d.x - 4, d.y - 4, 8, 8);
                        g.setGradientFill (juce::ColourGradient (revC.withAlpha (0.65f), d.x, d.y, revC.withAlpha (0.0f), d.x + 11, d.y, true));
                        g.fillEllipse (d.x - 11, d.y - 11, 22, 22);
                        g.setColour (revC.interpolatedWith (juce::Colours::white, 0.25f)); g.fillEllipse (d.x - 3, d.y - 3, 6, 6);
                        g.setColour (revC.withAlpha (0.5f)); g.drawRoundedRectangle (rb.reduced (0.5f), 6, 1.0f);
                    }
                    for (int i = 0; i < 9; ++i)
                    {
                        const P d (cxL - 32 + (float) i * 8, yL + 54);
                        g.setColour (i == r ? revC : juce::Colours::white.withAlpha (0.2f)); g.fillEllipse (d.x - 2, d.y - 2, 4, 4);
                    }
                }
                yR = bigKnob (cxR, yR, 76, "ATTACK", "0.25 ms", 0.4f) + 8;
                yR = bigKnob (cxR, yR, 76, "RELEASE", "250 ms", 0.45f) + 12;
                yR = smallPair (cxR, yR, "OUTPUT", "+9.0 dB", 0.45f, "MIX", "100%", 1.0f) + 14;
                pushRow (atY (rowR, yR, 48), "METER", { "GR", "+8", "+4", "OFF" }, 0);
                break;
            }
            case 1:   // LE-660 (Fairchild): Input Gain (stepped), Threshold, the 6-position Time Constant switch
            {
                static const char* ig[6] { "-12", "-8", "-4", "0", "+4", "+8" };
                static const char* tcl[6] { "1", "2", "3", "4", "5", "6" };
                yL = stepped (cxL, yL, 104, "INPUT GAIN", "0 dB", 6, 3, ig) + 26;
                yL = bigKnob (cxL, yL, 80, "THRESHOLD", "4.0", 0.6f) + 20;
                yR = stepped (cxR, yR, 80, "TIME CONSTANT", "2  -  0.8 s", 6, 1, tcl) + 14;
                yR = bigKnob (cxR, yR, 64, "OUTPUT", "+6.0 dB", 0.5f) + 12;
                yR = smallPair (cxR, yR, "DC THRESH", "0", 0.5f, "MIX", "100%", 1.0f);
                break;
            }
            case 2:   // LE-STA (Gates Sta-Level): Input and Output, the Single / Double / Triple mode switch, the 6-position release
            {
                static const char* rl[6] { "1", "2", "3", "4", "5", "6" };
                yL = bigKnob (cxL, yL, 104, "INPUT", "+10 dB", 0.45f) + 26;
                yL = batToggle (atY (rowL, yL, 60), "MODE", { "SINGLE", "DOUBLE", "TRIPLE" }, 1);
                yR = stepped (cxR, yR, 80, "RELEASE", "3", 6, 2, rl) + 18;
                yR = bigKnob (cxR, yR, 72, "OUTPUT", "+4.0 dB", 0.5f) + 12;
                yR = bigKnob (cxR, yR, 56, "MIX", "100%", 1.0f);
                break;
            }
            case 3:   // LE-2500 (API 2500): stepped Threshold / Ratio / Attack / Release, Make-up, and its three toggles
            {
                static const char* ra[7] { "1.5", "2", "3", "4", "6", "10", "INF" };
                static const char* at[6] { ".03", ".1", ".3", "1", "3", "10" };
                static const char* re[6] { ".05", ".1", ".2", ".5", "1", "2" };
                yL = bigKnob (cxL, yL - 8, 88, "THRESHOLD", U8 ("\xe2\x88\x92" "18 dB"), 0.55f) + 8;
                yL = stepped (cxL, yL, 60, "RATIO", "4 : 1", 7, 3, ra) + 20;
                yL = batToggle (atY (rowL, yL, 60), "KNEE", { "HARD", "MED", "SOFT" }, 1) + 4;
                // THRUST: like the API's - one click steps NORM > MED > LOUD, and its little curve shows what the detector hears
                // (MED and LOUD lean the detector away from the lows, so the bass pumps less)
                {
                    const int th = penv ("PRESS_THRUST") ? juce::jlimit (0, 2, std::atoi (penv ("PRESS_THRUST"))) : 0;
                    const R tile = atY (rowL, yL, 74);
                    // each setting has its own colour: NORM a calm ivory, MED warm amber, LOUD hot ember
                    const juce::Colour tc = th == 0 ? col ("#d8d0c2") : th == 1 ? col ("#e8a24f") : col ("#ff5f3c");
                    title (R (tile.getX(), tile.getY(), tile.getWidth(), 12), "THRUST");
                    const R b = tile.withTrimmedTop (18);
                    g.setGradientFill (juce::ColourGradient (col ("#1d1d22", 0.95f), 0, b.getY(), col ("#121216", 0.95f), 0, b.getBottom(), false));
                    g.fillRoundedRectangle (b, 6);
                    g.setColour (tc.withAlpha (0.55f)); g.drawRoundedRectangle (b.reduced (0.5f), 6, 1.0f);
                    g.setColour (tc.withAlpha (0.10f)); g.fillRoundedRectangle (b.reduced (1), 5);   // the tile takes on the setting's colour
                    // the three positions, the lit one bright (click anywhere on it to step on)
                    const char* tl[3] { "NORM", "MED", "LOUD" };
                    for (int i = 0; i < 3; ++i)
                    {
                        const R c (b.getX() + 6 + (float) i * (b.getWidth() - 12) / 3.0f, b.getY() + 4, (b.getWidth() - 12) / 3.0f, 12);
                        if (i == th) { g.setColour (tc.withAlpha (0.35f)); g.fillRoundedRectangle (c.reduced (4, 0), 6); }
                        text (g, tl[i], font (Face::CondMedium, 9, 1.2f), i == th ? ink : grey, c, juce::Justification::centred);
                    }
                    // the curve: frequency across (20 Hz to 20 kHz), how much the detector hears up and down
                    const R gr (b.getX() + 10, b.getY() + 20, b.getWidth() - 20, b.getHeight() - 26);
                    g.setColour (juce::Colours::white.withAlpha (0.06f));
                    for (float hz : { 100.0f, 1000.0f, 10000.0f }) { const float x = gr.getX() + std::log10 (hz / 20.0f) / 3.0f * gr.getWidth(); g.fillRect (x, gr.getY(), 1.0f, gr.getHeight()); }
                    auto ptAt = [&] (float u)
                    {
                        const float hz = 20.0f * std::pow (1000.0f, u);
                        // NORM: flat; MED: a gentle lean up; LOUD: a steeper lean, lows well down
                        const float oct = std::log2 (hz / 1000.0f);
                        const float db = th == 0 ? 0.0f : th == 1 ? juce::jlimit (-9.0f, 3.0f, 1.5f * oct) : juce::jlimit (-15.0f, 5.0f, 3.0f * oct);
                        return P (gr.getX() + u * gr.getWidth(), gr.getY() + gr.getHeight() * 0.3f - db / 15.0f * gr.getHeight() * 0.68f);   // flat sits high; the lows lean down
                    };
                    juce::Path cv;
                    for (int i = 0; i <= 60; ++i) { const P pt = ptAt ((float) i / 60.0f); if (i == 0) cv.startNewSubPath (pt); else cv.lineTo (pt); }
                    g.setColour (tc.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                    g.setColour (tc.interpolatedWith (juce::Colours::white, 0.35f)); g.strokePath (cv, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                    // a dot running back and forth along it, up and down the lean, with a short glowing trail
                    {
                        const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
                        const float u = penv ("PRESS_THRUST_U") ? (float) std::atof (penv ("PRESS_THRUST_U")) : 0.5f + 0.5f * (float) std::sin (tms / 1200.0);
                        const float dir = penv ("PRESS_THRUST_U") ? 1.0f : ((float) std::cos (tms / 1200.0) >= 0.0f ? 1.0f : -1.0f);
                        // it leaves a trail of little sparkles behind it, like the knee's dot - drifting off the line and fading
                        juce::Random rnd (11);
                        auto star = [&] (P c, float r, juce::Colour cc)
                        {
                            juce::Path s;
                            s.startNewSubPath (c.x, c.y - r); s.quadraticTo (c.x, c.y, c.x + r, c.y); s.quadraticTo (c.x, c.y, c.x, c.y + r);
                            s.quadraticTo (c.x, c.y, c.x - r, c.y); s.quadraticTo (c.x, c.y, c.x, c.y - r); s.closeSubPath();
                            g.setColour (cc); g.fillPath (s);
                        };
                        for (int i = 28; i >= 1; --i)
                        {
                            const float age = (float) i / 28.0f;
                            const P c0 = ptAt (juce::jlimit (0.0f, 1.0f, u - dir * (float) i * 0.011f));
                            const P c = c0.translated ((rnd.nextFloat() - 0.5f) * 10.0f * age, (rnd.nextFloat() - 0.5f) * 12.0f * age);
                            const auto sc = tc.interpolatedWith (juce::Colour::fromHSV (std::fmod ((float) i * 0.05f, 1.0f), 0.6f, 1.0f, 1.0f), 0.35f);
                            if (i % 2 == 0) star (c, 3.6f * (1.0f - age) + 1.0f, sc.withAlpha (0.85f * (1.0f - age)));
                            else { g.setColour (sc.withAlpha (0.55f * (1.0f - age))); g.fillEllipse (c.x - 1.1f, c.y - 1.1f, 2.2f, 2.2f); }
                        }
                        const P c = ptAt (u);
                        g.setGradientFill (juce::ColourGradient (tc.withAlpha (0.6f), c.x, c.y, tc.withAlpha (0.0f), c.x + 10, c.y, true));
                        g.fillEllipse (c.x - 10, c.y - 10, 20, 20);
                        g.setColour (tc.interpolatedWith (juce::Colours::white, 0.5f)); g.fillEllipse (c.x - 3.2f, c.y - 3.2f, 6.4f, 6.4f);
                        g.setColour (juce::Colours::white); g.fillEllipse (c.x - 1.4f, c.y - 1.4f, 2.8f, 2.8f);
                    }
                    yL = tile.getBottom();
                }
                yR = stepped (cxR, yR, 64, "ATTACK", "1 ms", 6, 3, at) + 6;
                yR = stepped (cxR, yR, 64, "RELEASE", ".2 s", 6, 2, re) + 2;
                valueBar (g, atY (rowR, yR, 36), "VAR RELEASE", "1.4 s", 0.55f); yR += 44;   // the variable release beside the stepped one
                valueBar (g, atY (rowR, yR, 36), "LINK", U8 ("100%  \xc2\xb7  shape HPF"), 1.0f); yR += 46;   // how much the two sides share, and its filter
                yR = smallPair (cxR, yR, "MAKE-UP", "+4.0 dB", 0.2f, "MIX", "100%", 1.0f) + 8;
                pushRow (atY (rowR, yR, 30), nullptr, { "COMP IN" }, 0);
                break;
            }
            case 4:   // LE-G (SSL G bus): Threshold, the 3-step Ratio, 6-step Attack, 5-step Release with Auto, Make-up, the IN button
            {
                static const char* ra[3] { "2", "4", "10" };
                static const char* at[6] { ".1", ".3", "1", "3", "10", "30" };
                static const char* re[5] { ".1", ".3", ".6", "1.2", "AUTO" };
                yL = bigKnob (cxL, yL, 104, "THRESHOLD", U8 ("\xe2\x88\x92" "12 dB"), 0.5f) + 22;
                yL = stepped (cxL, yL, 72, "RATIO", "4 : 1", 3, 1, ra) + 18;
                pushRow (atY (rowL, yL, 30), nullptr, { "COMP IN" }, 0);
                yR = stepped (cxR, yR, 72, "ATTACK", "10 ms", 6, 4, at) + 8;
                yR = stepped (cxR, yR, 72, "RELEASE", "AUTO", 5, 4, re) + 12;
                yR = smallPair (cxR, yR, "MAKE-UP", "+3.0 dB", 0.15f, "MIX", "100%", 1.0f);
                break;
            }
            case 5:   // LE-IRON (Manley Vari-Mu): Threshold, Compress / Limit, the HP sidechain, Attack 25-70 ms, 5-step Recovery, Output
            {
                static const char* rc[5] { ".2", ".4", ".6", "4", "8" };
                static const char* in[7] { "-12", "-8", "-4", "0", "+4", "+8", "+12" };
                yL = stepped (cxL, yL - 6, 64, "INPUT", "0 dB", 7, 3, in) + 6;   // the stepped input attenuator
                yL = bigKnob (cxL, yL, 84, "THRESHOLD", U8 ("\xe2\x88\x92" "20 dB"), 0.5f) + 12;
                yL = batToggle (atY (rowL, yL, 60), "MODE", { "COMPRESS", "LIMIT" }, 0) + 2;
                yL = batToggle (atY (rowL, yL, 60), "HP SIDECHAIN", { "OFF", "100 Hz" }, 1);
                yR = bigKnob (cxR, yR, 72, "ATTACK", "35 ms", 0.25f) + 8;
                yR = stepped (cxR, yR, 72, "RECOVERY", "0.6 s", 5, 2, rc) + 12;
                yR = smallPair (cxR, yR, "OUTPUT", "+4.0 dB", 0.5f, "MIX", "100%", 1.0f);
                break;
            }
            case 7:   // LE-8X (Empirical Labs Distressor EL8-X): four knobs, and everything else on its buttons
            {
                yL = bigKnob (cxL, yL, 104, "INPUT", "4.5", 0.45f) + 24;
                // RATIO: one button steps through them all, a light for each (Nuke is the brick wall)
                yL = pushRow (atY (rowL, yL, 48), "RATIO", { "1:1", "2:1", "3:1", "4:1" }, 3) + 6;
                yL = pushRow (atY (rowL, yL, 30), nullptr, { "6:1", "10:1", "20:1", "NUKE" }, -1) + 12;
                // the X: British mode (the all-buttons sound, at any ratio)
                glassButton (g, atY (rowL, yL, 22), "BRIT MODE", false);
                text (g, "the all-buttons smash, at any ratio", font (Face::Regular, 9.5f), grey, atY (rowL, yL + 28, 12), juce::Justification::centred);
                yR = bigKnob (cxR, yR, 68, "ATTACK", "5.0", 0.5f) + 6;
                yR = bigKnob (cxR, yR, 68, "RELEASE", "5.0", 0.5f) + 10;
                yR = smallPair (cxR, yR, "OUTPUT", "4.0", 0.4f, "MIX", "100%", 1.0f) + 10;
                // DETECTOR: what it listens with - each can be on (HP filter, band emphasis, stereo link)
                yR = pushRow (atY (rowR, yR, 48), "DETECTOR", { "HP", "BAND", "LINK" }, -1, false, 0b001) + 8;
                // AUDIO: what it adds - Dist 2 (tape-like, even), Dist 3 (tube-like, odd), and a gentle HP on the audio
                pushRow (atY (rowR, yR, 48), "AUDIO", { "DIST 2", "DIST 3", "HP" }, 0);
                break;
            }
            case 8:   // LE-160 (dbx 160): Threshold, Compression (ratio), Output; OverEasy; the meter's In / Out / GR
            {
                yL = bigKnob (cxL, yL, 104, "THRESHOLD", U8 ("\xe2\x88\x92" "10 dB"), 0.5f) + 22;
                yL = bigKnob (cxL, yL, 80, "COMPRESSION", "4 : 1", 0.45f) + 20;
                glassButton (g, atY (rowL, yL, 22), "OVEREASY", false);
                text (g, "the soft knee dbx made famous", font (Face::Regular, 9.5f), grey, atY (rowL, yL + 28, 12), juce::Justification::centred);
                yR = bigKnob (cxR, yR, 84, "OUTPUT GAIN", "+6.0 dB", 0.55f) + 14;
                yR = bigKnob (cxR, yR, 60, "MIX", "100%", 1.0f) + 18;
                pushRow (atY (rowR, yR, 48), "METER", { "IN", "OUT", "GR" }, 2);
                break;
            }
            case 9:   // LE-7 (Empirical Labs FATSO): Input drives it, Warmth tames the top, Output; the four compressor modes; Tranny
            {
                yL = bigKnob (cxL, yL, 104, "INPUT", "6.0", 0.6f) + 22;
                yL = bigKnob (cxL, yL, 80, "WARMTH", "4.0", 0.4f) + 20;
                pushRow (atY (rowL, yL, 30), nullptr, { "TRANNY" }, 0);
                text (g, "low-end weight from the transformer", font (Face::Regular, 9.5f), grey, atY (rowL, yL + 36, 12), juce::Justification::centred);
                yR = bigKnob (cxR, yR, 84, "OUTPUT", "5.0", 0.5f) + 14;
                yR = bigKnob (cxR, yR, 60, "MIX", "100%", 1.0f) + 18;
                yR = pushRow (atY (rowR, yR, 48), "COMPRESSOR", { "BUSS", "G.P.", "TRACK" }, 0) + 6;
                pushRow (atY (rowR, yR, 30), nullptr, { "SPANK" }, -1);
                break;
            }
            case 10:  // LE-760 (Compex F760X-RS): stepped Threshold / Ratio, attack toggle, 9 releases + Auto; the limiter and expander / gate
            {
                static const char* th[6] { "-20", "-16", "-12", "-8", "-4", "0" };
                static const char* ra[6] { "1", "2", "3", "5", "10", "20" };
                static const char* re[6] { "25ms", ".1", ".4", "1.6", "3.2", "AUTO" };
                yL = stepped (cxL, yL, 80, "THRESHOLD", "\xe2\x88\x92" "12 dB", 6, 2, th) + 10;
                yL = stepped (cxL, yL, 64, "RATIO", "3 : 1", 6, 2, ra) + 12;
                yL = batToggle (atY (rowL, yL, 60), "ATTACK", { "250us", "2.5ms", "25ms" }, 1);
                yR = stepped (cxR, yR, 64, "RELEASE", "AUTO", 6, 5, re) + 8;
                yR = batToggle (atY (rowR, yR, 60), "LIMITER", { "OFF", "ON", "PRE-EMP" }, 1) + 2;
                yR = batToggle (atY (rowR, yR, 60), "EXPANDER", { "OFF", "EXP", "GATE" }, 0) + 6;
                yR = smallPair (cxR, yR, "OUTPUT", "+3.0 dB", 0.5f, "MIX", "100%", 1.0f);
                break;
            }
            case 11:  // LE-SHM (Shadow Hills Mastering Compressor): the optical stage on the left, the discrete stage on the right, the transformers
            {
                title (atY (rowL, yL - 16, 12), "OPTICAL");
                yL = bigKnob (cxL, yL + 4, 88, "THRESHOLD", "4.0", 0.4f) + 12;
                yL = bigKnob (cxL, yL, 64, "GAIN", "+2.0 dB", 0.3f) + 18;
                yL = pushRow (atY (rowL, yL, 48), "TRANSFORMER", { "NICKEL", "IRON", "STEEL" }, 1) + 8;
                text (g, "clean  /  thick  /  bright", font (Face::Regular, 9.5f), grey, atY (rowL, yL, 12), juce::Justification::centred);
                title (atY (rowR, yR - 16, 12), "DISCRETE");
                yR = smallPair (cxR, yR + 4, "THRESHOLD", "5.0", 0.5f, "RATIO", "2 : 1", 0.3f) + 14;
                yR = smallPair (cxR, yR, "ATTACK", "3", 0.4f, "RECOVERY", "DUAL", 1.0f) + 14;
                yR = smallPair (cxR, yR, "GAIN", "+1.5 dB", 0.3f, "MIX", "100%", 1.0f) + 10;
                pushRow (atY (rowR, yR, 30), nullptr, { "OPTO IN", "DISCRETE IN" }, 0, true);
                break;
            }
            case 12:  // LE-2A (LA-2A): Peak Reduction and Gain - that's it; Compress / Limit; the meter's GR / +10 / +4
            {
                yL = bigKnob (cxL, yL, 110, "PEAK REDUCTION", "45", 0.45f) + 26;
                yL = batToggle (atY (rowL, yL, 60), "MODE", { "COMPRESS", "LIMIT" }, 0);
                yR = bigKnob (cxR, yR, 96, "GAIN", "50", 0.5f) + 18;
                yR = bigKnob (cxR, yR, 60, "MIX", "100%", 1.0f) + 18;
                pushRow (atY (rowR, yR, 48), "METER", { "GR", "+10", "+4" }, 0);
                break;
            }
            case 13:  // LE-1B (Tube-Tech CL 1B): Threshold, Ratio, Attack, Release, Gain; Fixed / Manual / Fixed-Manual timing
            {
                yL = bigKnob (cxL, yL, 104, "THRESHOLD", U8 ("\xe2\x88\x92" "16 dB"), 0.5f) + 22;
                yL = bigKnob (cxL, yL, 80, "RATIO", "4 : 1", 0.4f) + 16;
                yL = batToggle (atY (rowL, yL, 60), "TIMING", { "FIXED", "MANUAL", "F + M" }, 1);
                yR = bigKnob (cxR, yR, 72, "ATTACK", "25 ms", 0.45f) + 8;
                yR = bigKnob (cxR, yR, 72, "RELEASE", "0.5 s", 0.4f) + 12;
                yR = smallPair (cxR, yR, "GAIN", "+6 dB", 0.4f, "MIX", "100%", 1.0f);
                break;
            }
            case 14:  // LE-VSC (Vertigo VSC-3): Threshold, ratio 2-10 plus Soft and Brick, 10 attacks, 10 releases plus Auto Fast / Slow, Make-up, Peak / RMS
            {
                static const char* ra[8] { "SOFT", "2", "3", "4", "6", "8", "10", "BRICK" };
                static const char* at[6] { ".1", ".3", "1", "3", "10", "40" };
                static const char* re[6] { ".05", ".1", ".3", "1.2", "A.F", "A.S" };
                yL = bigKnob (cxL, yL, 96, "THRESHOLD", "+2 dBu", 0.5f) + 14;
                yL = stepped (cxL, yL, 72, "RATIO", "4 : 1", 8, 3, ra) + 14;
                yL = batToggle (atY (rowL, yL, 60), "DETECTOR", { "PEAK", "RMS" }, 0);
                yR = stepped (cxR, yR, 64, "ATTACK", "3 ms", 6, 3, at) + 6;
                yR = stepped (cxR, yR, 64, "RELEASE", "AUTO SLOW", 6, 5, re) + 10;
                yR = smallPair (cxR, yR, "MAKE-UP", "+4 dB", 0.2f, "MIX", "100%", 1.0f) + 10;
                pushRow (atY (rowR, yR, 30), nullptr, { "BYPASS" }, -1);
                break;
            }
            case 6:   // LE-336 (Neve 33609): compressor - Threshold, 5-step Ratio, Fast / Slow, 6-step Recovery; the limiter, Gain
            {
                static const char* ra[5] { "1.5", "2", "3", "4", "6" };
                static const char* rc[6] { "100", "400", "800", "1.5k", "A1", "A2" };
                yL = bigKnob (cxL, yL, 96, "THRESHOLD", U8 ("\xe2\x88\x92" "14 dB"), 0.5f) + 14;
                yL = stepped (cxL, yL, 72, "RATIO", "3 : 1", 5, 2, ra) + 14;
                yL = batToggle (atY (rowL, yL, 60), "ATTACK", { "FAST", "SLOW" }, 1);
                yR = stepped (cxR, yR, 72, "RECOVERY", "A1", 6, 4, rc) + 8;
                yR = smallPair (cxR, yR, "GAIN", "+6 dB", 0.3f, "LIMIT", "+12 dB", 0.6f) + 4;
                static const char* lr[5] { "50", "100", "200", "A1", "A2" };
                yR = stepped (cxR, yR, 56, "LIMIT RECOVERY", "100 ms", 5, 1, lr) + 8;   // the limiter's own recovery
                pushRow (atY (rowR, yR, 30), nullptr, { "COMP IN", "LIMIT IN" }, 0, true);
                break;
            }
        }
        {
            const double since = juce::Time::getMillisecondCounterHiRes() - kneeTouch;
            const float alpha = (kneeOn || penv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
            if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);
            paintKneeEdge (g, kneeOn || penv ("PRESS_KNEE") != nullptr);
        }
        {   // on the right: the CHARACTER drawer (what the unit does to the tone), in place of the envelope
            const bool eo = envOn || penv ("PRESS_CHAR") != nullptr;
            if (eo) paintEnvOverlay (g, R (PW - 218.0f - 359.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), 1.0f);
            paintEnvEdge (g, eo);
        }
        juce::ignoreUnused (unit);   // (no faceplate: the glass panels carry the unit)
        paintTopStrip (g, penv ("PRESS_TOPOFF") == nullptr);
        paintScDrawer (g, penv ("PRESS_SC") != nullptr);
        return;
    }
    // RVox: Goliath's one-knob vocal leveler - pushing COMPRESS levels the vocal and turns it up (like RVox);
    // LEVEL MATCH holds the level even instead, so you hear the compression, not the volume
    if (penv ("PRESS_RVOX") != nullptr)
    {
        {
            const R s (0, top, pw, ph);
            panel (s, false);
            const float cx = s.getCentreX();
            float y = s.getY() + 70;
            y = bigKnobP (cx + 4, y, 118, "COMPRESS", "cVoxComp", 3) + 30;   // pushing it (clockwise) lowers the threshold
            // RVox's gate (Goliath's soft one, about 1:4 below its level): keeps the quiet gaps quiet as you push Compress
            y = bigKnobP (cx, y, 70, "GATE", "cVoxGate") + 8;
            text (g, "keeps the gaps between", font (Face::Regular, 10), grey, R (s.getX() + 10, y, pw - 20, 13), juce::Justification::centred);
            text (g, "phrases quiet", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 14, pw - 20, 13), juce::Justification::centred);
        }
        {
            const R s (PW - pw, top, pw, ph);
            panel (s, true);
            const float cx = s.getCentreX();
            float y = s.getY() + 70;
            y = bigKnobP (cx, y, 96, "GAIN", "cVoxGain") + 20;
            {
                const R br (s.getX() + 18, y, pw - 36, 22);
                // LEVEL MATCH: off, pushing Compress turns it up (like RVox); on, the level stays even
                buttonP (br, "LEVEL MATCH", "cVoxMatch");
                text (g, "off: pushing Compress", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 28, pw - 20, 13), juce::Justification::centred);
                text (g, "turns the level up", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 42, pw - 20, 13), juce::Justification::centred);
                y += 72;
            }
            y = bigKnobP (cx, y, 64, "MIX", "cMix");
        }
        // the knee drawer stays: RVox's own curve - a fixed 3:1 with a very soft 10 dB knee, display only
        {
            const float keepT = mThr, keepR = mRatio, keepK = mKnee;
            mThr = proc.param ("cVoxComp"); mRatio = 3.0f; mKnee = 10.0f;
            const double since = juce::Time::getMillisecondCounterHiRes() - kneeTouch;
            const float alpha = (kneeOn || penv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
            if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);
            paintKneeEdge (g, kneeOn || penv ("PRESS_KNEE") != nullptr);
            mThr = keepT; mRatio = keepR; mKnee = keepK;
            {   // the envelope drawer: the right panel's twin of the knee
                const bool eo = envOn || penv ("PRESS_ENV") != nullptr;
                if (eo) paintEnvOverlay (g, R (PW - 218.0f - 359.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), 1.0f);
                paintEnvEdge (g, eo);
            }
        }
        paintTopStrip (g, penv ("PRESS_TOPOFF") == nullptr);
        paintScDrawer (g, penv ("PRESS_SC") != nullptr);
        return;
    }
    // left: threshold, ratio, and the knee with its little graph
    {
        const R s (0, top, pw, ph);
        panel (s, false);
        const float cx = s.getCentreX();
        float ratioY = 0;
        float y = s.getY() + 22;
        if (mla)
        {
            // the MLA-2 has no threshold knob: INPUT GAIN drives the sound into it (0 to -20 dB, 1 dB steps)
            // the MLA-2's bi-colour lamp: green when the compressor is in, flashing red the moment it starts to compress
            {
                const bool red = penv ("PRESS_MLA_LAMPOFF") == nullptr;
                const auto lc = red ? col ("#ff4a3d") : col ("#4ade80");
                const P l (cx + 62, y + 6);
                g.setGradientFill (juce::ColourGradient (lc.withAlpha (0.55f), l.x, l.y, lc.withAlpha (0.0f), l.x + 11, l.y, true));
                g.fillEllipse (l.x - 11, l.y - 11, 22, 22);
                g.setColour (lc.interpolatedWith (juce::Colours::white, 0.25f)); g.fillEllipse (l.x - 3.5f, l.y - 3.5f, 7, 7);
                g.setColour (juce::Colours::white.withAlpha (0.7f)); g.fillEllipse (l.x - 1.5f, l.y - 2.2f, 2.2f, 2.2f);
            }
            gKnobSteps = 21; y = bigKnobP (cx, y, 104, "INPUT GAIN", "cMlaIn", 3) + 22;
            gKnobSteps = 9;  y = bigKnobP (cx, y, 80, "RATIO", "cMlaRatio") + 24;   // 1.2 1.4 1.7 2 2.5 3 5 8 inf
            gKnobSteps = 0;
            // ImageLink: links the two sides, but lets a short peak on one side through on its own
            buttonP (R (s.getX() + 18, y, pw - 36, 24), "IMAGELINK", "cMlaLink");
            text (g, "links L / R, lets short peaks through", font (Face::Regular, 9.5f), grey, R (s.getX() + 8, y + 30, pw - 16, 12), juce::Justification::centred);
            ratioY = y;
        }
        else {
        y = bigKnobP (cx, y, 104, "THRESHOLD", "cThresh") + 22;
        ratioY = y;
        y = bigKnobP (cx, y, 80, "RATIO", "cRatio") + 10;
        y += 14;   // the ratio picks are gone - the wheel does it all (LE-DS1's wheel turns past 1:1 into expansion)
        // the knee: its knob (the graph appears big over the screen while you touch it)
        if (penv ("PRESS_DS1") != nullptr)
        {
            // the real DS1's soft-knee: a variable amount, 0 (hard) to 1 (from 0 dBFS down to twice the threshold)
            y = bigKnobP (cx, y, 64, "SOFT KNEE", "cDsKnee");
        }
        else
            y = bigKnobP (cx, y, 64, "KNEE", "cKnee");
        if (penv ("PRESS_PRO") != nullptr)
        {
            // LE-PRO's STYLE (like Pro-C 3): one compressor, many characters - click the arrows or the name for the list
            const R b (s.getX() + 18, 650.0f - 21.0f, pw - 36, 42);
            g.setColour (cleanC.withAlpha (0.08f)); g.fillRoundedRectangle (b, 6);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, b.getY(), juce::Colours::white.withAlpha (0.0f), 0, b.getCentreY(), false));
            g.fillRoundedRectangle (b.reduced (1), 5);
            g.setColour (cleanC.withAlpha (0.45f)); g.drawRoundedRectangle (b.reduced (0.5f), 6, 1.0f);
            text (g, "STYLE", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (b.getX() + 8, b.getY() + 5, 60, 11));
            text (g, pchoice ("cProStyle").toUpperCase(), font (Face::Regular, 13.5f, 1.2f), ink, R (b.getX() + 22, b.getY() + 17, b.getWidth() - 44, 17), juce::Justification::centred);
            pctl (b.withWidth (b.getWidth() * 0.5f), "cProStyle", 2); pctl (b.withTrimmedLeft (b.getWidth() * 0.5f), "cProStyle", 1);
            auto chev = [&] (float x, bool left)
            {
                juce::Path c; const float yy = b.getY() + 25.5f;
                if (left) { c.startNewSubPath (x + 3, yy - 5); c.lineTo (x - 2, yy); c.lineTo (x + 3, yy + 5); }
                else      { c.startNewSubPath (x - 3, yy - 5); c.lineTo (x + 2, yy); c.lineTo (x - 3, yy + 5); }
                g.setColour (cleanC.withAlpha (0.85f)); g.strokePath (c, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };
            chev (b.getX() + 14, true); chev (b.getRight() - 14, false);
            text (g, juce::String ((int) proc.param ("cProStyle") + 1) + " / 14", font (Face::CondMedium, 8.5f, 1.2f), grey, R (b.getRight() - 58, b.getY() + 5, 50, 11), juce::Justification::centredRight);
        }
        }
        // the knee window's tab (replaced by the drawer's glass edge - kept off)
        if (false)
        {
            const bool on = kneeOn || penv ("PRESS_KNEE") != nullptr;
            const R t (pw - 1, ratioY + 18, 30, 92);
            juce::Path tp; tp.addRoundedRectangle (t.getX(), t.getY(), t.getWidth(), t.getHeight(), 8, 8, false, true, false, true);
            g.setGradientFill (juce::ColourGradient (on ? col ("#2a2550", 0.95f) : col ("#1c1d22", 0.9f), t.getX(), 0, on ? col ("#1d1a38", 0.95f) : col ("#141519", 0.9f), t.getRight(), 0, false));
            g.fillPath (tp);
            g.setColour (on ? cleanC.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.1f)); g.strokePath (tp, juce::PathStrokeType (1.0f));
            // a tiny knee curve as its icon
            const R ic (t.getX() + 6, t.getY() + 10, 18, 18);
            juce::Path kc; kc.startNewSubPath (ic.getX(), ic.getBottom()); kc.lineTo (ic.getX() + 9, ic.getY() + 8);
            kc.quadraticTo (ic.getX() + 12, ic.getY() + 5, ic.getRight(), ic.getY() + 3);
            g.setColour (on ? cleanC : grey); g.strokePath (kc, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // KNEE, written down the tab
            g.saveState();
            g.addTransform (juce::AffineTransform::rotation (pi * 0.5f, t.getCentreX(), t.getY() + 58));
            text (g, "KNEE", font (Face::CondMedium, 9.5f, 2.0f), on ? ink : grey, R (t.getCentreX() - 30, t.getY() + 52, 60, 12), juce::Justification::centred);
            g.restoreState();
            zone (t, Hit::Piano, 77);   // (mock: the click target for the knee window)
        }
    }
    // Pro-C 3 style knee display: big, over the waveform, while Threshold, Ratio or Knee is touched - fades 4 s after
    {
        const double since = juce::Time::getMillisecondCounterHiRes() - kneeTouch;
        const float alpha = (kneeOn || penv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
        if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);   // the Figma size
        paintKneeEdge (g, kneeOn || penv ("PRESS_KNEE") != nullptr);
        {   // the envelope drawer: the right panel's twin of the knee
            const bool eo = envOn || penv ("PRESS_ENV") != nullptr;
            if (eo) paintEnvOverlay (g, R (PW - 218.0f - 359.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), 1.0f);
            paintEnvEdge (g, eo);
        }
    }
    // right: attack and release (with auto release), then make-up and mix side by side, and auto gain
    {
        const R s (PW - pw, top, pw, ph);
        panel (s, true);
        const float cx = s.getCentreX();
        float y = s.getY() + 22;
        const bool ds1 = penv ("PRESS_DS1") != nullptr;
        if (mla)
        {
            // attack in ms per dB and release in s per dB, stepped - and both follow the music on their own
            gKnobSteps = 9; y = bigKnobP (cx, y, 76, "ATTACK", "cMlaAtk") + 10;
            gKnobSteps = 9; y = bigKnobP (cx, y, 76, "RELEASE", "cMlaRel") + 4;
            gKnobSteps = 0;
            text (g, "both follow the program", font (Face::Regular, 9.5f), grey, R (s.getX() + 8, y, pw - 16, 12), juce::Justification::centred);
            y += 32;
        }
        else if (ds1)
        {
            y = bigKnobP (cx, y, 70, "ATTACK", "cAttack") + 8;
            // two releases, like the real DS1: fast for the peaks, slow for the average level
            const float sz = 54;
            smallKnobP (cx - 46, y, sz, 92, "REL FAST", "cDsRelF");
            smallKnobP (cx + 46, y, sz, 92, "REL SLOW", "cDsRelS");
            y += 20 + sz + 8 + 18 + 12;
            valueBar (g, R (s.getX() + 18, y, pw - 36, 42), "AVERAGE", pfmt ("cDsAvg"), pn ("cDsAvg"));
            pctl (R (s.getX() + 18, y, pw - 36, 42), "cDsAvg", 0);
            y += 42 + 22;
        }
        else
        {
            y = bigKnobP (cx, y, 76, "ATTACK", "cAttack") + 10;
            y = bigKnobP (cx, y, 76, "RELEASE", "cRelease") + 6;
        }
        auto lit = [&] (float yy, const char* t, const char* pid)
        {
            // faint until the mouse comes over it (like Range and Hold) - but always clear when it's on
            const R br (s.getX() + 18, yy, pw - 36, 22);
            const bool near = br.expanded (8).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_RH") != nullptr || pon (pid);
            g.beginTransparencyLayer (near ? 1.0f : 0.14f);
            glassButton (g, br, t, pon (pid));
            g.endTransparencyLayer();
            pctl (br, pid, 1);
        };
        if (! ds1 && ! mla) { lit (y, "AUTO RELEASE", "cAutoRel"); y += 36; }
        // LE-PRO: make-up, then separate dry and wet levels (linked so they cross over, like Pro-C 3)
        if (penv ("PRESS_PRO") != nullptr)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (s.getX() + 18, y - 10, pw - 36, 1.0f);
            const float sz = 44;
            smallKnobP (cx - 58, y, sz, 64, "MAKE-UP", "cMakeup", 12.5f);
            smallKnobP (cx, y, sz, 64, "DRY", "cProDry", 12.5f);
            smallKnobP (cx + 58, y, sz, 64, "WET", "cProWet", 12.5f);
            // the link between dry and wet: a small chain
            {
                const P l (cx + 29, y + 20 + sz * 0.5f);
                g.setColour (cleanC.withAlpha (0.8f));
                g.drawRoundedRectangle (l.x - 6, l.y - 2.5f, 7, 5, 2.5f, 1.2f); g.drawRoundedRectangle (l.x - 1, l.y - 2.5f, 7, 5, 2.5f, 1.2f);
            }
            y += 20 + sz + 8 + 16 + 16;
        }
        else
        // make-up and mix, a pair
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (s.getX() + 18, y - 10, pw - 36, 1.0f);
            const float sz = 56;
            smallKnobP (cx - 46, y, sz, 92, mla ? "OUTPUT" : "MAKE-UP", "cMakeup");
            smallKnobP (cx + 46, y, sz, 92, "MIX", "cMix");
            y += 20 + sz + 8 + 18 + 14;
        }
        lit (y, "AUTO GAIN", "cAutoGain");
        // range and hold: slim value bars level with the sidechain's look-ahead row - faint until the mouse comes over them
        // (not on LE-MLA: its timing is the program's)
        if (! mla)
        {
            const float bw = (pw - 36 - 10) * 0.5f, ry = 650.0f - 21.0f;
            const R both (s.getX() + 18, ry, pw - 36, 42);
            const bool near = both.expanded (10).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_RH") != nullptr;
            g.beginTransparencyLayer (near ? 1.0f : 0.14f);
            valueBar (g, R (s.getX() + 18, ry, bw, 42), "RANGE", pfmt ("cRange"), pn ("cRange"));
            valueBar (g, R (s.getX() + 18 + bw + 10, ry, bw, 42), "HOLD", pfmt ("cHold"), pn ("cHold"));
            pctl (R (s.getX() + 18, ry, bw, 42), "cRange", 0); pctl (R (s.getX() + 18 + bw + 10, ry, bw, 42), "cHold", 0);
            g.endTransparencyLayer();
        }
    }
    paintTopStrip (g, penv ("PRESS_TOPOFF") == nullptr);
    paintScDrawer (g, penv ("PRESS_SC") != nullptr);
    // the strip along the bottom: off for now (Lovy is deciding where its controls go)
    if (false)
    {
        const R b (pw + 30, 640.0f - 70.0f, PW - 2 * pw - 74, 62);
        juce::Path p; p.addRoundedRectangle (b, 10);
        cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 16, { 0, 6 }), p);
        g.setGradientFill (juce::ColourGradient (col ("#22232a", 0.9f), 0, b.getY(), col ("#131418", 0.9f), 0, b.getBottom(), false));
        g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        const float cy = b.getCentreY();
        float x = b.getX() + 16;
        // (the LE-RC name and engine menu are off the strip for now)
        auto push = [&] (R r, const juce::String& t, bool on)
        {
            fillRound (g, r, 5, on ? col ("#2a2550") : col ("#17181c"));
            strokeRound (g, r, 5, on ? cleanC.withAlpha (0.8f) : col ("#34353b"));
            text (g, t, font (Face::CondMedium, 9.5f, 1.3f), on ? ink : grey, r, juce::Justification::centred);
        };
        push (R (x, cy - 11, 48, 22), "GLIDE", true);  push (R (x + 50, cy - 11, 48, 22), "SNAP", false); x += 108;
        push (R (x, cy - 11, 48, 22), "COLOR", true); push (R (x + 50, cy - 11, 48, 22), "PURE", false); x += 114;
        // gain reduction, as a bar meter
        {
            const int nh = (int) hGr.size();
            const float grn = nh > 0 ? hGr[(size_t) ((hPos - 1 + nh) % nh)] : 0.0f;
            text (g, "GR", font (Face::CondMedium, 9.5f, 1.4f), col ("#b9b3a7"), R (x, cy - 6, 22, 12));
            const R mb (x + 26, cy - 5, 150, 10);
            fillRound (g, mb, 4, col ("#0b0b0e"));
            fillRound (g, R (mb.getRight() - mb.getWidth() * juce::jmin (1.0f, grn / 20.0f), mb.getY(), mb.getWidth() * juce::jmin (1.0f, grn / 20.0f), mb.getHeight()), 4, grC);
            text (g, MINUS + juce::String (grn, 1) + " dB", font (Face::Regular, 11), grC.interpolatedWith (juce::Colours::white, 0.3f), R (mb.getRight() + 8, cy - 7, 70, 14));
        }
        push (R (b.getRight() - 16 - 104, cy - 11, 104, 22), U8 ("SIDECHAIN \xe2\x96\xb4"), false);
    }
}

void Canvas::paintSidePanel (juce::Graphics& g, bool right, const char* title, std::initializer_list<std::pair<const char*, float>> knobs, juce::Colour c)
{
    juce::ignoreUnused (g, right, title, knobs, c);
}

// HELPER: the three helpers in glass panels on either side (like Reso's), Flow in its own panel
void Canvas::paintLevelTab (juce::Graphics& g)
{
    // HELPER in glass: the same panels as CLEAN and VINTAGE, each tool a small card - left the level tools, right the tone tools
    gHelperTab = true;
    const float pw = 196.0f, top = 46.0f, ph = 588.0f;
    auto panel = [&] (R s0, bool right)
    {
        const R s (s0.getX(), 0, s0.getWidth(), 670);
        const float k = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const int iw = juce::roundToInt (s.getWidth() * k), ih = juce::roundToInt (s.getHeight() * k);
        juce::Image im (juce::Image::ARGB, iw, ih, true);
        {
            juce::Graphics ig (im);
            ig.addTransform (juce::AffineTransform::translation (-s.getX(), -s.getY()).scaled (k));
            ig.setGradientFill (juce::ColourGradient (col ("#23242c", 0.92f), 0, s.getY(), col ("#121317", 0.92f), 0, s.getBottom(), false));
            ig.fillRect (s);
            ig.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), s.getX(), s.getY() + 120, juce::Colours::white.withAlpha (0.0f), s.getRight(), s.getY() + 360, false));
            ig.fillRect (s);
        }
        {
            juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
            auto smooth = [] (float v) { v = juce::jlimit (0.0f, 1.0f, v); return v * v * (3.0f - 2.0f * v); };
            std::vector<float> fx ((size_t) iw);
            for (int x = 0; x < iw; ++x)
            {
                const float u = right ? (float) x / (float) (iw - 1) : 1.0f - (float) x / (float) (iw - 1);
                fx[(size_t) x] = (0.1f + 0.9f * smooth (u / 0.42f)) * (1.0f - 0.45f * smooth ((u - 0.82f) / 0.18f));
            }
            for (int y = 0; y < ih; ++y)
            {
                const float t = (float) y / (float) (ih - 1);
                const float fy = smooth (t / 0.12f) * smooth ((1.0f - t) / 0.1f);
                for (int x = 0; x < iw; ++x) reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->multiplyAlpha (fx[(size_t) x] * fy);
            }
        }
        g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / k).translated (s.getX(), s.getY()));
    };
    auto fh = font (Face::CondMedium, 10, 2.0f);
    // a card's title: a coloured dot (lit when on) and the tool's name, a hairline under it
    bool cardOn = true;   // the card being drawn is on (its controls dim when it's off)
    // a card's title: a coloured dot (lit when on), the tool's name, its on / off button at the right end, a hairline under it
    auto card = [&] (R s, float& y, const char* name, juce::Colour c, bool on)
    {
        cardOn = on;
        const float x = s.getX() + 18, w = s.getWidth() - 36;
        g.setColour (on ? c : juce::Colours::white.withAlpha (0.2f)); g.fillEllipse (x, y + 3, 5, 5);
        text (g, name, fh, on ? ink : grey, R (x + 12, y, w - 40, 12));
        {   // the power button: every tool can be switched off - small and quiet, lit in the tool's tone when on
            const R pb (x + w - 14, y - 1, 14, 14);
            g.setColour (on ? c.withAlpha (0.55f) : juce::Colours::white.withAlpha (0.14f)); g.drawEllipse (pb.reduced (0.5f), 1.0f);
            icon (g, Icon::Power, pb.reduced (3.5f), on ? c.interpolatedWith (juce::Colours::white, 0.25f) : grey, 1.1f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x, y + 18, w, 1.0f);
        y += 28;
    };
    // a row of small knobs: label above, value under
    auto knobs = [&] (R s, float& y, std::initializer_list<std::tuple<const char*, juce::String, float>> ks, juce::Colour c)
    {
        const int n = (int) ks.size();
        const float w = (s.getWidth() - 36) / (float) n, sz = n >= 3 ? 48.0f : n == 2 ? 64.0f : 76.0f;
        const bool ring = penv ("PRESS_HKNOB") != nullptr && std::atoi (penv ("PRESS_HKNOB")) == 3;
        int i = 0;
        gHelperKnob = false;   // (the compressors' own knob)
        if (! cardOn) g.beginTransparencyLayer (0.3f);   // an off tool's knobs sit back
        for (auto& k : ks)
        {
            const float cx = s.getX() + 18 + w * ((float) i + 0.5f);
            text (g, std::get<0> (k), font (Face::CondMedium, 9.5f, 1.6f), col ("#b9b3a7"), R (cx - w * 0.5f, y, w, 11), juce::Justification::centred);
            helperKnob (g, R (cx - sz * 0.5f, y + 17, sz, sz), std::get<2> (k), c, std::get<1> (k));
            if (! ring) text (g, std::get<1> (k), font (Face::Light, n >= 3 ? 12.5f : 14.0f, 0.4f), ink, R (cx - w * 0.5f, y + 17 + sz + 6, w, 16), juce::Justification::centred);
            ++i;
        }
        gHelperKnob = false;
        if (! cardOn) g.endTransparencyLayer();
        y += 17 + sz + (ring ? 12.0f : 6.0f + 16.0f + 10.0f);
    };
    auto seg = [&] (R r, std::initializer_list<const char*> labs, int at, juce::Colour c)
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        const int n = (int) labs.size(); int i = 0;
        for (auto* l : labs)
        {
            const R cc (r.getX() + r.getWidth() * (float) i / (float) n, r.getY(), r.getWidth() / (float) n, r.getHeight());
            if (i == at) { g.setColour (c.withAlpha (0.3f)); g.fillRoundedRectangle (cc.reduced (2), (r.getHeight() - 4) * 0.5f); }
            text (g, l, font (Face::CondMedium, 9, 1.0f), i == at ? ink : grey, cc, juce::Justification::centred);
            ++i;
        }
    };
    const auto teal = helperC, hpC = col ("#7fd3ff"), brC = helperC, flC = col ("#c08bff"), liC = helperC;
    // a column of knobs, one under another (Lift's bands): label above, value under
    auto knobCol = [&] (float cx, float y, float colW, std::initializer_list<std::tuple<const char*, juce::String, float>> ks, juce::Colour c, float sz)
    {
        const bool ring = penv ("PRESS_HKNOB") != nullptr && std::atoi (penv ("PRESS_HKNOB")) == 3;
        if (! cardOn) g.beginTransparencyLayer (0.3f);
        for (auto& k : ks)
        {
            text (g, std::get<0> (k), font (Face::CondMedium, 9.5f, 1.6f), col ("#b9b3a7"), R (cx - colW * 0.5f, y, colW, 11), juce::Justification::centred);
            helperKnob (g, R (cx - sz * 0.5f, y + 16, sz, sz), std::get<2> (k), c, std::get<1> (k));
            if (! ring) text (g, std::get<1> (k), font (Face::Light, 13.0f, 0.4f), ink, R (cx - colW * 0.5f, y + 16 + sz + 5, colW, 16), juce::Justification::centred);
            y += 16 + sz + (ring ? 14.0f : 5.0f + 16.0f + 12.0f);
        }
        if (! cardOn) g.endTransparencyLayer();
        return y;
    };
    // ---- right: Lift (its two bands side by side, a column each) and the Breath under it - next to its Tone drawer
    {
        const R s (PW - pw, top, pw, ph);
        panel (s, true);
        float y = s.getY() + 36;
        card (s, y, "LIFT  /  2-BAND", liC, true);
        seg (R (s.getX() + 18, y, s.getWidth() - 36, 18), { "UPWARD", "DYN EQ" }, liftDynEq() ? 1 : 0, liC);
        y += 30;
        const float colW = (s.getWidth() - 36) * 0.5f, cL = s.getX() + 18 + colW * 0.5f, cR = cL + colW;
        text (g, "LOW", font (Face::CondMedium, 9.5f, 2.0f), ink, R (cL - colW * 0.5f, y, colW, 11), juce::Justification::centred);
        text (g, "HIGH", font (Face::CondMedium, 9.5f, 2.0f), ink, R (cR - colW * 0.5f, y, colW, 11), juce::Justification::centred);
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (cL + colW * 0.5f - 0.5f, y + 18, 1.0f, 290.0f);
        y += 20;
        knobCol (cL, y, colW, { { "FREQ", "120 Hz", 0.25f }, { "THRESH", U8 ("\xe2\x88\x92" "30 dB"), 0.4f }, { "LIFT", "+4 dB", 0.6f } }, liC, 54.0f);
        y = knobCol (cR, y, colW, { { "FREQ", "6 kHz", 0.7f }, { "THRESH", U8 ("\xe2\x88\x92" "34 dB"), 0.35f }, { "LIFT", "+3 dB", 0.55f } }, liC, 54.0f) + 4;
        {   // Breath's light: blinks each time it catches a breath, and how many it has caught
            const int n = (int) hBreath.size();
            auto bAt = [&] (int age) { return hBreath[(size_t) ((hPos - 1 - age + 2 * n) % n)]; };
            const bool lit = bAt (0) < -3.0f;
            int caught = 0; for (int a = 1; a < n; ++a) if (bAt (a - 1) < -3.0f && bAt (a) >= -3.0f) ++caught;
            const float lx = s.getRight() - 18 - 14 - 24, ly = y + 1.5f;
            if (lit) { g.setGradientFill (juce::ColourGradient (brC.withAlpha (0.7f), lx + 4, ly + 4, brC.withAlpha (0.0f), lx + 14, ly + 4, true)); g.fillEllipse (lx - 6, ly - 6, 20, 20); }
            g.setColour (lit ? brC.interpolatedWith (juce::Colours::white, 0.45f) : juce::Colours::white.withAlpha (0.12f)); g.fillEllipse (lx, ly, 8, 8);
            g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (lx, ly, 8, 8, 0.8f);
            juce::ignoreUnused (caught);
        }
        card (s, y, "BREATH", brC, penv ("PRESS_HOFF") == nullptr);
        knobs (s, y, { { "AMOUNT", U8 ("\xe2\x88\x92" "8 dB"), 0.33f }, { "SENSE", "55 %", 0.55f }, { "FADE", "40 ms", 0.4f } }, brC);
        {   // the breaths it has caught lately: each one a soft bronze dip, scrolling with the music
            const int n = (int) hBreath.size();
            auto bAt = [&] (int age) { return hBreath[(size_t) ((hPos - 1 - age + 2 * n) % n)]; };
            int caught = 0; for (int a = 1; a < n; ++a) if (bAt (a - 1) < -3.0f && bAt (a) >= -3.0f) ++caught;
            const float x = s.getX() + 18, w = s.getWidth() - 36;
            y += 2;
            text (g, "CAUGHT", font (Face::CondMedium, 8.5f, 1.6f), col ("#b9b3a7"), R (x, y, 80, 10));
            text (g, juce::String (caught) + " breaths", font (Face::CondMedium, 9.5f, 1.0f), brC.interpolatedWith (juce::Colours::white, 0.3f), R (x + w - 90, y - 1, 90, 11), juce::Justification::centredRight);
            const R lane (x, y + 16, w, 30);
            fillRound (g, lane, 5, col ("#000000", 0.35f)); strokeRound (g, lane, 5, juce::Colours::white.withAlpha (0.08f));
            g.setColour (juce::Colours::white.withAlpha (0.1f)); g.fillRect (lane.getX() + 4, lane.getY() + 6, lane.getWidth() - 8, 1.0f);
            juce::Path p; const int cols = (int) (lane.getWidth() - 8);
            const int span = juce::jmin (n - 1, 300);
            for (int i = 0; i <= cols; ++i)
            {
                const int age = (int) ((float) (cols - i) / (float) cols * (float) span);
                const float yy = lane.getY() + 6 - bAt (age) / 8.0f * (lane.getHeight() - 12);
                if (i == 0) p.startNewSubPath (lane.getX() + 4 + (float) i, yy); else p.lineTo (lane.getX() + 4 + (float) i, yy);
            }
            juce::Path f (p); f.lineTo (lane.getRight() - 4, lane.getY() + 6); f.lineTo (lane.getX() + 4, lane.getY() + 6); f.closeSubPath();
            g.setGradientFill (juce::ColourGradient (brC.withAlpha (0.05f), 0, lane.getY() + 6, brC.withAlpha (0.4f), 0, lane.getBottom() - 6, false)); g.fillPath (f);
            g.setColour (brC.interpolatedWith (juce::Colours::white, 0.3f)); g.strokePath (p, juce::PathStrokeType (1.2f));
        }
    }
    // ---- left: the Spectra 1964 peak limiter (its 610 Complimiter's controls), then the Rider
    {
        const R s (0, top, pw, ph);
        panel (s, false);
        float y = s.getY() + 36;
        const float ty = y;
        card (s, y, "1964 PEAK", peakC, true);
        {   // its two lights: white when peaks cross the threshold, red on overload
            auto led = [&] (float x, juce::Colour c, bool on, const char* l)
            {
                const P d (x, ty + 5);
                if (on) { g.setColour (c.withAlpha (0.4f)); g.fillEllipse (d.x - 6, d.y - 6, 12, 12); }
                g.setColour (on ? c : c.withAlpha (0.25f)); g.fillEllipse (d.x - 2.6f, d.y - 2.6f, 5.2f, 5.2f);
                text (g, l, font (Face::CondMedium, 7.5f, 1.0f), grey, R (d.x - 16, d.y + 5, 32, 9), juce::Justification::centred);
            };
            led (s.getRight() - 86, col ("#f4f1ea"), true, "THR");
            led (s.getRight() - 62, col ("#ff4a3d"), false, "OVER");
        }
        // BUILD NOTE: the 1964 must be super clean, like the Spectra 1964 610 - no saturation, no distortion, no colour stage of any kind.
        text (g, "TRANSPARENT  /  NO COLOUR", font (Face::CondMedium, 7.5f, 1.4f), grey, R (s.getX() + 30, y - 9, s.getWidth() - 50, 9));
        y += 4;
        knobs (s, y, { { "INPUT", U8 ("\xe2\x88\x92" "40 dBu"), 0.55f }, { "OUTPUT", "+4 dBu", 0.5f } }, peakC);
        knobs (s, y, { { "SLOPE", "2 : 1", 0.5f }, { "RELEASE", "3", 0.3f } }, peakC);
        seg (R (s.getX() + 18, y, (s.getWidth() - 36) * 0.48f, 18), { "GR", "+4", "+8" }, 0, peakC);
        seg (R (s.getX() + 18 + (s.getWidth() - 36) * 0.52f, y, (s.getWidth() - 36) * 0.48f, 18), { "OFF", "10", "20" }, 0, peakC);   // the pad: off, 10 or 20 dB
        text (g, "METER", font (Face::CondMedium, 7.5f, 1.0f), grey, R (s.getX() + 18, y + 20, 80, 9));
        text (g, "PAD", font (Face::CondMedium, 7.5f, 1.0f), grey, R (s.getX() + 18 + (s.getWidth() - 36) * 0.52f, y + 20, 40, 9));
        y += 46;
        card (s, y, "RIDER", teal, true);
        knobs (s, y, { { "RANGE", U8 ("\xc2\xb1" "6 dB"), 0.5f }, { "TARGET", U8 ("\xe2\x88\x92" "16"), 0.6f }, { "SPEED", "MED", 0.5f } }, teal);
        {   // the Rider's meter: how far it is riding right now (+/-6 dB from the centre), and the loudness in vs its target
            const int n = (int) hRide.size();
            const float ride = hRide[(size_t) ((hPos - 1 + n) % n)];
            const float x = s.getX() + 18, w = s.getWidth() - 36;
            y += 4;
            text (g, "RIDING", font (Face::CondMedium, 8.5f, 1.6f), col ("#b9b3a7"), R (x, y, 80, 10));
            text (g, juce::String (ride > 0.05f ? "+" : ride < -0.05f ? MINUS : juce::String()) + juce::String (std::abs (ride), 1) + " dB",
                  font (Face::CondMedium, 9.5f, 1.0f), teal.interpolatedWith (juce::Colours::white, 0.3f), R (x + w - 80, y - 1, 80, 11), juce::Justification::centredRight);
            const R bar (x, y + 17, w, 8);
            fillRound (g, bar, 4, col ("#000000", 0.4f)); strokeRound (g, bar, 4, juce::Colours::white.withAlpha (0.1f));
            const float cx = bar.getCentreX(), ex = cx + ride / 6.0f * (w * 0.5f - 4);
            const R fillR (juce::jmin (cx, ex), bar.getY() + 2, std::abs (ex - cx), 4);
            g.setGradientFill (juce::ColourGradient (teal.withAlpha (0.25f), cx, 0, teal.withAlpha (0.9f), ex, 0, false)); g.fillRoundedRectangle (fillR, 2.0f);
            g.setGradientFill (juce::ColourGradient (teal.withAlpha (0.6f), ex, bar.getCentreY(), teal.withAlpha (0.0f), ex + 10, bar.getCentreY(), true)); g.fillEllipse (ex - 10, bar.getCentreY() - 10, 20, 20);
            g.setColour (juce::Colours::white); g.fillEllipse (ex - 2.5f, bar.getCentreY() - 2.5f, 5, 5);
            const char* tl[] { "\xe2\x88\x92" "6", "\xe2\x88\x92" "3", "0", "+3", "+6" };
            for (int i = 0; i < 5; ++i)
            {
                const float tx = bar.getX() + 4 + (w - 8) * (float) i / 4.0f;
                g.setColour (juce::Colours::white.withAlpha (i == 2 ? 0.35f : 0.15f)); g.fillRect (tx - 0.5f, bar.getBottom() + 2, 1.0f, i == 2 ? 5.0f : 3.0f);
                text (g, juce::String::fromUTF8 (tl[i]), font (Face::Regular, 8), grey, R (tx - 12, bar.getBottom() + 7, 24, 9), juce::Justification::centred);
            }
            // loudness: what's coming in, and where the Rider is bringing it (its target)
            const float yl = bar.getBottom() + 26;
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x, yl - 6, w, 1.0f);
            auto read = [&] (float rx, const char* l, float v, juce::Colour c)
            {
                text (g, l, font (Face::CondMedium, 7.5f, 1.4f), grey, R (rx, yl, 60, 9));
                text (g, MINUS + juce::String (std::abs (v), 1), font (Face::Light, 15.0f, 0.3f), c, R (rx, yl + 10, 60, 18));
            };
            read (x, "IN", slowLvl, ink);
            read (x + w * 0.42f, "OUT", slowLvl + ride, teal.interpolatedWith (juce::Colours::white, 0.35f));
            text (g, "LUFS", font (Face::CondMedium, 7.5f, 1.4f), grey, R (x + w - 40, yl + 15, 40, 9), juce::Justification::centredRight);
            g.setColour (teal.withAlpha (0.6f)); g.drawArrow (juce::Line<float> (x + w * 0.27f, yl + 19, x + w * 0.38f, yl + 19), 1.0f, 5.0f, 4.0f);
        }
        juce::ignoreUnused (hpC, flC);
    }
    // the left edge: the KNEE drawer back, like the compressors - here it draws the 1964's curve (threshold and slope)
    {
        const float keepT = mThr, keepR = mRatio, keepK = mKnee;
        mThr = -14.0f; mRatio = 2.0f; mKnee = 6.0f;
        const double since = juce::Time::getMillisecondCounterHiRes() - kneeTouch;
        const float alpha = (kneeOn || penv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
        if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);
        paintKneeEdge (g, kneeOn || penv ("PRESS_KNEE") != nullptr);
        mThr = keepT; mRatio = keepR; mKnee = keepK;
    }
    // the right edge: the TONE drawer - the high pass and the two Lift bands on a frequency graph, moving with the music
    {
        const bool eo = envOn || penv ("PRESS_DYNEQ") != nullptr;
        if (eo) paintEnvOverlay (g, R (PW - 218.0f - 359.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), 1.0f);
        paintEnvEdge (g, eo);
    }
    gHelperTab = false;
}

// the knee display: input level across, output level up, the curve in white turning green up to where the sound is now,
// the threshold as crossed lines, the knee as a soft band around it
static void unusedKnee() {}
void Canvas::paintKneeContent (juce::Graphics& g, R k)
{
    const auto green = col ("#58e08f");
    // (no background: like Pro-C, the knee is drawn straight onto the waveform)
    const R a = k.reduced (44, 96);
    auto px = [&] (float db) { return a.getX() + (db + 60.0f) / 60.0f * a.getWidth(); };
    auto py = [&] (float db) { return a.getBottom() - (db + 60.0f) / 60.0f * a.getHeight(); };
    auto fs = font (Face::Regular, 9);
    for (int db = -60; db <= 0; db += 12)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRect (px ((float) db), a.getY(), 1.0f, a.getHeight());
        g.fillRect (a.getX(), py ((float) db), a.getWidth(), 1.0f);
        text (g, db == 0 ? juce::String ("0") : MINUS + juce::String (-db), fs, grey, R (px ((float) db) - 12, a.getBottom() + 6, 24, 10), juce::Justification::centred);
        if (db > -60 && db < 0) text (g, MINUS + juce::String (-db), fs, grey, R (a.getX() + 4, py ((float) db) + 2, 24, 10), juce::Justification::centredLeft);
    }
    // the knee band and the threshold
    g.setColour (cleanC.withAlpha (0.10f));
    g.fillRect (R (px (mThr - mKnee * 0.5f), a.getY(), px (mThr + mKnee * 0.5f) - px (mThr - mKnee * 0.5f), a.getHeight()));
    {
        // the threshold: a thin glass line, very faint, that slowly breathes brighter and back (a 3.5 s cycle)
        const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
        const float br = 0.5f + 0.5f * std::sin ((float) (tms / 3500.0 * juce::MathConstants<double>::twoPi));
        auto glassLine = [&] (R r, bool vertical)
        {
            const float glow = 0.04f + 0.08f * br, core = 0.12f + 0.22f * br;
            juce::ColourGradient gg (juce::Colours::white.withAlpha (0.0f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.0f), vertical ? r.getX() : r.getRight(), vertical ? r.getBottom() : r.getY(), false);
            gg.addColour (0.5, juce::Colours::white.withAlpha (glow));
            g.setGradientFill (gg); g.fillRect (vertical ? r.withSizeKeepingCentre (6, r.getHeight()) : r.withSizeKeepingCentre (r.getWidth(), 6));
            juce::ColourGradient cg (cleanC.withAlpha (0.0f), r.getX(), r.getY(), cleanC.withAlpha (0.0f), vertical ? r.getX() : r.getRight(), vertical ? r.getBottom() : r.getY(), false);
            cg.addColour (0.2, juce::Colours::white.withAlpha (core * 0.6f)); cg.addColour (0.5, juce::Colours::white.withAlpha (core)); cg.addColour (0.8, juce::Colours::white.withAlpha (core * 0.6f));
            g.setGradientFill (cg); g.fillRect (vertical ? r.withSizeKeepingCentre (1, r.getHeight()) : r.withSizeKeepingCentre (r.getWidth(), 1));
        };
        glassLine (R (px (mThr), a.getY(), 0, a.getHeight()), true);
        glassLine (R (a.getX(), py (mThr), a.getWidth(), 0), false);
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawLine (px (-60), py (-60), px (0), py (0), 1.0f);
    // the curve
    auto outOf = [&] (float in)
    {
        const float over = in - mThr;
        float gr = 0.0f;
        if (over > mKnee * 0.5f) gr = over * (1.0f - 1.0f / mRatio);
        else if (over > -mKnee * 0.5f) { const float q = over + mKnee * 0.5f; gr = (1.0f - 1.0f / mRatio) * q * q / (2.0f * mKnee); }
        return in - gr;
    };
    const int nh = (int) hIn.size();
    const float now = nh > 0 ? juce::jlimit (-60.0f, 0.0f, hIn[(size_t) ((hPos - 1 + nh) % nh)]) : -60.0f;
    juce::Path all, lit;
    for (int i = 0; i <= 240; ++i)
    {
        const float in = -60.0f + (float) i * 0.25f;
        const P pt (px (in), py (outOf (in)));
        if (i == 0) all.startNewSubPath (pt); else all.lineTo (pt);
        if (in <= now) { if (i == 0) lit.startNewSubPath (pt); else lit.lineTo (pt); }
    }
    // the curve fades out towards the top and the bottom, so it melts into the graph
    auto fadeY = [&] (juce::Colour c, float peak)
    {
        juce::ColourGradient cg (c.withAlpha (0.0f), 0, a.getY() - 10, c.withAlpha (0.0f), 0, a.getBottom() + 4, false);
        cg.addColour (0.3, c.withAlpha (peak)); cg.addColour (0.62, c.withAlpha (peak));
        return cg;
    };
    g.setGradientFill (fadeY (juce::Colours::black, 0.4f)); g.strokePath (all, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (fadeY (juce::Colours::white, 0.95f)); g.strokePath (all, juce::PathStrokeType (2.0f));
    juce::ignoreUnused (lit);
    // the trail: where the sound was over the last half second, as sparkles that shrink and fade
    {
        juce::Random rnd (hPos * 7919);
        auto star = [&] (P c, float r, juce::Colour cc)
        {
            juce::Path s;
            s.startNewSubPath (c.x, c.y - r); s.quadraticTo (c.x, c.y, c.x + r, c.y); s.quadraticTo (c.x, c.y, c.x, c.y + r);
            s.quadraticTo (c.x, c.y, c.x - r, c.y); s.quadraticTo (c.x, c.y, c.x, c.y - r); s.closeSubPath();
            g.setColour (cc); g.fillPath (s);
        };
        const int trail = juce::jmin (70, nh);
        for (int a2 = trail - 1; a2 >= 1; --a2)
        {
            const float lv = juce::jlimit (-60.0f, 0.0f, hIn[(size_t) ((hPos - 1 - a2 + 2 * nh) % nh)]);
            const float age = (float) a2 / (float) trail;
            const P c (px (lv), py (outOf (lv)));
            const float lift = 0.2f + 0.8f * smoothStep ((a.getBottom() - c.y) / (a.getHeight() * 0.55f));
            const float jx = (rnd.nextFloat() - 0.5f) * 22.0f * age, jy = (rnd.nextFloat() - 0.5f) * 22.0f * age;
            // rainbow sparkles: each one a different colour, drifting through the spectrum along the trail
            const auto rc = juce::Colour::fromHSV (std::fmod ((float) (hPos + a2) * 0.045f, 1.0f), 0.7f, 1.0f, 1.0f);
            if (a2 % 2 == 0) star (c.translated (jx, jy), 5.0f * (1.0f - age) + 1.2f, rc.withAlpha (0.85f * (1.0f - age) * lift));
            else { g.setColour (rc.withAlpha (0.5f * (1.0f - age) * lift)); g.fillEllipse (c.x + jx - 1.4f, c.y + jy - 1.4f, 2.8f, 2.8f); }
        }
    }
    // the dot itself: a bright core with a soft glow
    {
        const P c (px (now), py (outOf (now)));
        const auto gold = col ("#ffd27a");
        // dims as it slides down the curve, brightens as it climbs
        const float lift = 0.15f + 0.85f * smoothStep ((a.getBottom() - c.y) / (a.getHeight() * 0.55f));
        g.setGradientFill (juce::ColourGradient (gold.withAlpha (0.6f * lift), c.x, c.y, gold.withAlpha (0.0f), c.x + 16, c.y, true));
        g.fillEllipse (c.x - 16, c.y - 16, 32, 32);
        g.setColour (gold.interpolatedWith (juce::Colours::white, 0.45f).withAlpha (0.25f + 0.75f * lift)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9, 9);
        g.setColour (juce::Colours::white.withAlpha (0.2f + 0.8f * lift)); g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
    }
    // what it says
    text (g, gHelperTab ? "1964  CURVE" : "KNEE", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 100, 12));
    if (penv ("PRESS_MLA") != nullptr)   // LE-MLA: the threshold is fixed - Input Gain drives into it - and so is the knee
        text (g, U8 ("drive \xe2\x88\x92" "8 dB  \xc2\xb7  2:1  \xc2\xb7  soft knee"), font (Face::Regular, 10.5f), ink, R (a.getRight() - 214, a.getY() - 31, 214, 13), juce::Justification::centredRight);
    else
    text (g, MINUS + juce::String (-mThr, 1) + U8 (" dB  \xc2\xb7  ") + juce::String (mRatio, 1).trimCharactersAtEnd (".0") + U8 (":1  \xc2\xb7  ") + (penv ("PRESS_DS1") != nullptr ? juce::String ("soft knee 0.6") : mKnee < 0.5f ? juce::String ("hard knee") : "knee " + juce::String (mKnee, 0) + " dB"),
          font (Face::Regular, 10.5f), ink, R (a.getRight() - 214, a.getY() - 31, 214, 13), juce::Justification::centredRight);
    if (penv ("PRESS_RVOX") != nullptr || penv ("PRESS_MLA") != nullptr || gCompMode)
        text (g, "FIXED", font (Face::CondMedium, 9, 1.6f), cleanC, R (a.getX() + 40, a.getY() - 30, 60, 12));
    text (g, "IN", font (Face::CondMedium, 9, 1.6f), grey, R (a.getRight() - 20, a.getBottom() + 16, 20, 10), juce::Justification::centredRight);
    text (g, "OUT", font (Face::CondMedium, 9, 1.6f), grey, R (a.getX() + 6, a.getY() + 4, 30, 10), juce::Justification::centredLeft);
}

// the knee window: drawn into its own picture, then faded out towards the top and the bottom of the screen
void Canvas::paintKneeOverlay (juce::Graphics& g, R k, float alpha)
{
    const float sc = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int iw = juce::roundToInt (k.getWidth() * sc), ih = juce::roundToInt (k.getHeight() * sc);
    juce::Image im (juce::Image::ARGB, juce::jmax (1, iw), juce::jmax (1, ih), true);
    {
        juce::Graphics ig (im);
        ig.addTransform (juce::AffineTransform::translation (-k.getX(), -k.getY()).scaled (sc));
        paintKneeContent (ig, k);
    }
    {
        juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < ih; ++y)
        {
            const float t = (float) y / (float) juce::jmax (1, ih - 1);
            const float f = juce::jlimit (0.0f, 1.0f, juce::jmin (t / 0.16f, (1.0f - t) / 0.16f)) * alpha;
            const float e = f * f * (3.0f - 2.0f * f);   // smooth
            for (int x = 0; x < iw; ++x)
            {
                auto* p = reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y));
                p->multiplyAlpha (e);
            }
        }
    }
    g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / sc).translated (k.getX(), k.getY()));
}

// the knee drawer's edge (like Pro-C 3): a thin glass strip from the top of the screen to the bottom, with an arrow
// handle in the middle - open, it sits at the graph's right side (arrow to tuck it away); closed, it rests against
// the left panel (arrow to pull it out)
void Canvas::paintKneeEdge (juce::Graphics& g, bool open)
{
    const float ex = open ? 600.0f : 200.0f;
    const R strip (ex, 0, 14, 670);
    // the glass, fading out at the very top and bottom
    {
        juce::ColourGradient vg (juce::Colours::white.withAlpha (0.0f), 0, strip.getY(), juce::Colours::white.withAlpha (0.0f), 0, strip.getBottom(), false);
        vg.addColour (0.12, juce::Colours::white.withAlpha (0.05f)); vg.addColour (0.88, juce::Colours::white.withAlpha (0.05f));
        g.setGradientFill (vg); g.fillRect (strip);
        juce::ColourGradient lg (juce::Colours::white.withAlpha (0.0f), 0, strip.getY(), juce::Colours::white.withAlpha (0.0f), 0, strip.getBottom(), false);
        lg.addColour (0.12, cleanC.withAlpha (0.45f)); lg.addColour (0.5, juce::Colours::white.withAlpha (0.55f)); lg.addColour (0.88, cleanC.withAlpha (0.45f));
        g.setGradientFill (lg); g.fillRect (strip.getCentreX() - 0.5f, strip.getY(), 1.0f, strip.getHeight());
    }
    // the arrow handle: barely there until the mouse comes near, then bright
    {
        const R h (strip.getCentreX() - 12, 310, 24, 60);
        dz (h.expanded (8, 6), Hit::PDrawer, 0);
        const bool near = h.expanded (30, 40).contains (mouse - display.getPosition());
        g.beginTransparencyLayer (near ? 1.0f : 0.18f);
        juce::Path p; p.addRoundedRectangle (h, 12);
        cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 10, { 0, 3 }), p);
        g.setGradientFill (juce::ColourGradient (col ("#2c2b38", 0.85f), h.getX(), h.getY(), col ("#17171d", 0.85f), h.getRight(), h.getBottom(), false));
        g.fillPath (p);
        g.setColour (cleanC.withAlpha (0.7f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        // two chevrons pointing the way it will move
        for (int i = 0; i < 2; ++i)
        {
            const float cy = h.getCentreY() - 6 + (float) i * 12, cx = h.getCentreX();
            juce::Path c;
            if (open) { c.startNewSubPath (cx + 3, cy - 4); c.lineTo (cx - 2, cy); c.lineTo (cx + 3, cy + 4); }
            else      { c.startNewSubPath (cx - 3, cy - 4); c.lineTo (cx + 2, cy); c.lineTo (cx - 3, cy + 4); }
            g.setColour (ink); g.strokePath (c, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.endTransparencyLayer();
    }
}

// an arrow handle (shared by the drawers): faint until the mouse comes near; chevrons point the way it will move
void Canvas::paintArrowHandle (juce::Graphics& g, R h, int dir, const juce::String& label)
{
    const bool near = h.expanded (30, 30).contains (mouse - display.getPosition());
    g.beginTransparencyLayer (near ? 1.0f : 0.2f);
    juce::Path p; p.addRoundedRectangle (h, juce::jmin (h.getWidth(), h.getHeight()) * 0.5f);
    g.setGradientFill (juce::ColourGradient (col ("#2c2b38", 0.85f), h.getX(), h.getY(), col ("#17171d", 0.85f), h.getRight(), h.getBottom(), false));
    g.fillPath (p);
    g.setColour (cleanC.withAlpha (0.7f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    const P c = h.getCentre().translated (label.isEmpty() ? 0.0f : -textWidth (font (Face::CondMedium, 9, 1.6f), label) * 0.5f - 6.0f, 0.0f);
    juce::Path ch;   // dir: 0 up, 1 down
    if (dir == 0) { ch.startNewSubPath (c.x - 4, c.y + 2); ch.lineTo (c.x, c.y - 2); ch.lineTo (c.x + 4, c.y + 2); }
    else          { ch.startNewSubPath (c.x - 4, c.y - 2); ch.lineTo (c.x, c.y + 2); ch.lineTo (c.x + 4, c.y - 2); }
    g.setColour (ink); g.strokePath (ch, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (label.isNotEmpty())
        text (g, label, font (Face::CondMedium, 9, 1.6f), ink, R (c.x + 9, h.getY(), h.getRight() - c.x - 9, h.getHeight()), juce::Justification::centredLeft);
    g.endTransparencyLayer();
}

// the top strip: the finer controls - Range, Hold, Glide/Snap, Color/Pure, Auto Attack, Peak/RMS and Delta - in glass
// along the top of the screen; its arrow tucks it away
void Canvas::paintTopStrip (juce::Graphics& g, bool open)
{
    const float x0 = 214.0f, x1 = PW - 214.0f;
    if (! open) { paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 1, 44, 14), 1, {}); dz (R ((x0 + x1) * 0.5f - 28, 0, 56, 20), Hit::PDrawer, 2); return; }
    const R s (x0, 0, x1 - x0, 42);
    drawGlass (g, s, [] (float u, float t) { return (0.3f + 0.7f * smoothStep ((1.0f - t) / 0.6f)) * smoothStep (u / 0.06f) * smoothStep ((1.0f - u) / 0.06f); });
    const float cy = 16.0f;
    // flip buttons show what they're set to (click to change); toggles light up
    auto flip = [&] (R r, const char* now, const char*) { glassButton (g, r, now, true, true); };
    auto toggle = [&] (R r, const char* t, bool on) { glassButton (g, r, t, on); };
    // PRESSURE: tied to their parameters - a flip shows its setting (click for the next), a toggle lights when on
    auto flipP = [&] (R r, const juce::String& pre, const char* pid) { glassButton (g, r, (pre + pchoice (pid).toUpperCase()).toRawUTF8(), true, true); pctl (r, pid, 1); };
    auto toggleP = [&] (R r, const char* t, const char* pid) { glassButton (g, r, t, pon (pid)); pctl (r, pid, 1); };
    auto handle = [&] (R h) { paintArrowHandle (g, h, 0, {}); dz (h.expanded (6, 4), Hit::PDrawer, 2); };
    const float bw = 96, gap = 14;
    if (gCompMode)   // VINTAGE: each unit's own switches, then Delta
    {
        const int u = penv ("PRESS_VUNIT") ? std::atoi (penv ("PRESS_VUNIT")) : 0;
        std::vector<std::pair<const char*, float>> fl;
        switch (u)
        {
            // (SC HPF: the "bass doesn't pump it" filter, added to the units whose originals have none)
            case 0:  fl = { { "SC HPF  OFF", 104 } }; break;                                      // LE-76: its buttons are on the panels
            case 1:  fl = { { "LAT / VERT  OFF", 140 }, { "SC HPF  OFF", 104 } }; break;          // LE-660: mid-side ("lateral / vertical")
            case 2:  fl = { { "METER  GR", 110 }, { "SC HPF  OFF", 104 } }; break;                // LE-STA
            case 3:  fl = { { "TYPE  NEW", 104 } }; break;                                        // LE-2500: feed-forward (new) or feedback (old)
            case 4:  fl = { { "AUTO FADE  OFF", 130 } }; break;                                   // LE-G
            case 5:  fl = { { "LINK", 76 } }; break;                                              // LE-IRON
            case 6:  fl = { { "LINK", 76 }, { "SC HPF  OFF", 104 } }; break;                      // LE-336
            case 7:  fl = { { "IMAGE LINK", 104 } }; break;                                       // LE-8X: the X's image link
            case 8:  fl = { { "SC HPF  OFF", 104 } }; break;                                      // LE-160
            case 9:  fl = { { "SC HPF  OFF", 104 } }; break;                                      // LE-7
            case 10: fl = { { "LINK", 76 } }; break;                                              // LE-760
            case 11: fl = { { "LINK", 76 }, { "SC FILTER  90 Hz", 140 } }; break;                 // LE-SHM
            case 12: fl = { { "SC HPF  OFF", 104 } }; break;                                      // LE-2A
            case 13: fl = { { "SC  INTERNAL", 120 } }; break;                                     // LE-1B
            default: fl = { { "STEREO SC", 110 }, { "SC FILTER  60 Hz", 140 } }; break;           // LE-VSC
        }
        float total = 76;
        for (auto& f : fl) total += f.second + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        for (auto& f : fl) { flip (R (x, cy - 11, f.second, 22), f.first, ""); x += f.second + gap; }
        toggle (R (x, cy - 11, 76, 22), "DELTA", false);
        handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
        return;
    }
    if (penv ("PRESS_PRO") != nullptr)   // LE-PRO: Character (off / warm / punchy) with its drive, Peak / RMS, Delta
    {
        // Character: Tube / Diode / Bright, its Drive, and whether it colours before or after the compressor;
        // Auto Threshold keeps the compression the same when the level going in changes
        const float total = 140 + 96 + 76 + 130 + 76 + gap * 4;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flipP (R (x, cy - 11, 140, 22), "CHARACTER  ", "cProChar"); x += 140 + gap;
        glassButton (g, R (x, cy - 11, 96, 22), ("DRIVE  " + pfmt ("cProDrive")).toRawUTF8(), proc.param ("cProDrive") > 0.5f);
        pctl (R (x, cy - 11, 96, 22), "cProDrive", 0); x += 96 + gap;   // drag it for more or less
        flipP (R (x, cy - 11, 76, 22), "", "cProPre"); x += 76 + gap;
        toggleP (R (x, cy - 11, 130, 22), "AUTO THRESHOLD", "cProAutoThr"); x += 130 + gap;
        toggleP (R (x, cy - 11, 76, 22), "DELTA", "cDelta");
        handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
        return;
    }
    if (penv ("PRESS_MLA") != nullptr)   // LE-MLA: its meter switch (gain reduction or VU) and Delta
    {
        const float total = 120 + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flipP (R (x, cy - 11, 120, 22), "METER  ", "cMlaMeter"); x += 120 + gap;
        toggleP (R (x, cy - 11, 76, 22), "DELTA", "cDelta");
        handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
        return;
    }
    if (penv ("PRESS_DS1") != nullptr)   // LE-DS1: full range or band-selective (only the chosen band is compressed), and Delta
    {
        const float total = 120 + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flipP (R (x, cy - 11, 120, 22), "", "cDsBand"); x += 120 + gap;
        toggleP (R (x, cy - 11, 76, 22), "DELTA", "cDelta");
        handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
        return;
    }
    if (penv ("PRESS_RVOX") != nullptr)   // RVox's timing and tone are fixed: only Peak / RMS and Delta apply
    {
        const float total = bw + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flipP (R (x, cy - 11, bw, 22), "", "cPeak"); x += bw + gap;
        toggleP (R (x, cy - 11, 76, 22), "DELTA", "cDelta");
        handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
        return;
    }
    const float total = bw * 3 + 104 + 76 + gap * 4;
    float x = (x0 + x1) * 0.5f - total * 0.5f;
    flipP (R (x, cy - 11, bw, 22), "", "cGlide"); x += bw + gap;
    flipP (R (x, cy - 11, bw, 22), "", "cColor"); x += bw + gap;
    flipP (R (x, cy - 11, bw, 22), "", "cPeak"); x += bw + gap;
    toggleP (R (x, cy - 11, 104, 22), "AUTO ATTACK", "cAutoAtk"); x += 104 + gap;
    toggleP (R (x, cy - 11, 76, 22), "DELTA", "cDelta");
    handle (R ((x0 + x1) * 0.5f - 22, 31, 44, 14));
}

// a slim value bar: a label, the value, and a thin bar filled to it - drag up / down (or left / right) to change
void Canvas::valueBar (juce::Graphics& g, R r, const char* label, const juce::String& value, float pos)
{
    g.setColour (cleanC.withAlpha (0.05f)); g.fillRoundedRectangle (r, 6);   // glass, like the buttons
    g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1.0f);
    text (g, label, font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (r.getX() + 8, r.getY() + 5, r.getWidth() - 16, 11));
    text (g, value, font (Face::Regular, 11.5f), ink, R (r.getX() + 8, r.getY() + 17, r.getWidth() - 16, 13));
    const R t (r.getX() + 8, r.getBottom() - 9, r.getWidth() - 16, 3);
    fillRound (g, t, 1.5f, juce::Colours::white.withAlpha (0.08f));
    fillRound (g, t.withWidth (t.getWidth() * juce::jlimit (0.0f, 1.0f, pos)), 1.5f, cleanC);
    if (pos > 0.01f) { g.setColour (cleanC.withAlpha (0.35f)); g.fillRoundedRectangle (t.withWidth (t.getWidth() * pos).expanded (0, 2), 3); }
}

// the sidechain: closed, a handle on the bottom edge. Open, the detector EQ rises big onto the screen behind a horizontal
// glass edge (blended in like the knee), with the sidechain controls in a row along the bottom centre
// HELPERS' bottom glass window: the De-esser and the Pop control, side by side
static void paintHelperDrawerContent (juce::Graphics& g, R k, double tms)
{
    const auto c = helperC, ink2 = col ("#b9b3a7");
    const float half = (k.getWidth() - 40.0f) * 0.5f;
    auto light = [&] (P at, bool lit)
    {
        if (lit) { g.setGradientFill (juce::ColourGradient (c.withAlpha (0.7f), at.x, at.y, c.withAlpha (0.0f), at.x + 11, at.y, true)); g.fillEllipse (at.x - 11, at.y - 11, 22, 22); }
        g.setColour (lit ? c.interpolatedWith (juce::Colours::white, 0.45f) : juce::Colours::white.withAlpha (0.12f)); g.fillEllipse (at.x - 4, at.y - 4, 8, 8);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (at.x - 4, at.y - 4, 8, 8, 0.8f);
    };
    auto header = [&] (float x, float w, float y, const char* name, const juce::String& sub, bool lit, const juce::String& now)
    {
        g.setColour (c); g.fillEllipse (x, y + 3, 5, 5);
        text (g, name, font (Face::CondMedium, 10, 2.0f), ink, R (x + 12, y, 120, 12));
        text (g, sub, font (Face::CondMedium, 7.5f, 1.4f), grey, R (x + 12, y + 14, 200, 9));
        const R pb (x + w - 14, y - 1, 14, 14);
        g.setColour (c.withAlpha (0.55f)); g.drawEllipse (pb.reduced (0.5f), 1.0f);
        icon (g, Icon::Power, pb.reduced (3.5f), c.interpolatedWith (juce::Colours::white, 0.25f), 1.1f);
        light (P (x + w - 34, y + 6), lit);
        text (g, now, font (Face::CondMedium, 9.5f, 1.0f), c.interpolatedWith (juce::Colours::white, 0.3f), R (x + w - 146, y, 100, 12), juce::Justification::centredRight);
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x, y + 26, w, 1.0f);
    };
    auto knobRow = [&] (float x, float y, float w, std::initializer_list<std::tuple<const char*, juce::String, float>> ks)
    {
        const int n = (int) ks.size(); const float cw = w / (float) n, sz = 40.0f; int i = 0;
        for (auto& kk : ks)
        {
            const float cx = x + cw * ((float) i++ + 0.5f);
            text (g, std::get<0> (kk), font (Face::CondMedium, 8.5f, 1.5f), ink2, R (cx - cw * 0.5f, y, cw, 10), juce::Justification::centred);
            helperKnob (g, R (cx - sz * 0.5f, y + 14, sz, sz), std::get<2> (kk), c, std::get<1> (kk));
            text (g, std::get<1> (kk), font (Face::Light, 11.5f, 0.3f), ink, R (cx - cw * 0.5f, y + 14 + sz + 3, cw, 14), juce::Justification::centred);
        }
    };
    auto screen = [&] (R r) { fillRound (g, r, 6, col ("#000000", 0.35f)); strokeRound (g, r, 6, juce::Colours::white.withAlpha (0.08f)); };
    const float top = k.getY() + 22, scW = 168.0f, scH = 96.0f;
    const int style = penv ("PRESS_DSTYLE") ? std::atoi (penv ("PRESS_DSTYLE")) : 0;
    const float sNow = juce::jlimit (0.0f, 1.0f, (float) std::sin (tms / 260.0 + 4.4) * 1.3f - 0.1f), red = sNow * 5.5f;
    // the De-esser's picture: the top of the spectrum, its band, the esses carved out
    auto essScreen = [&] (R a)
    {
        auto fx = [&] (float hz) { return a.getX() + std::log2 (hz / 2000.0f) / 3.0f * a.getWidth(); };
        for (float hz : { 2000.0f, 4000.0f, 8000.0f, 16000.0f })
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (fx (hz), a.getY(), 1.0f, a.getHeight());
            text (g, juce::String ((int) (hz / 1000)) + "k", font (Face::Regular, 7.5f), grey, R (fx (hz) - 10, a.getBottom() + 3, 20, 9), juce::Justification::centred);
        }
        const float f0 = 6500.0f;
        g.setColour (c.withAlpha (0.13f)); g.fillRect (R (fx (f0 / 1.45f), a.getY(), fx (f0 * 1.45f) - fx (f0 / 1.45f), a.getHeight()));
        juce::Path in, out; juce::Random rnd (3);
        for (int i = 0; i <= 120; ++i)
        {
            const float hz = 2000.0f * std::pow (8.0f, (float) i / 120.0f);
            const float o = std::log2 (hz / 7000.0f);
            const float lvl = 0.22f + 0.5f * std::exp (-o * o * 2.0f) * (0.45f + sNow * 0.8f) + 0.05f * rnd.nextFloat();
            const float ob = std::log2 (hz / f0), cut = red / 6.0f * 0.5f * std::exp (-ob * ob * 3.5f);
            const P pi (fx (hz), a.getBottom() - juce::jmin (1.0f, lvl) * a.getHeight());
            const P po (fx (hz), a.getBottom() - juce::jlimit (0.0f, 1.0f, lvl - cut) * a.getHeight());
            if (i == 0) { in.startNewSubPath (pi); out.startNewSubPath (po); } else { in.lineTo (pi); out.lineTo (po); }
        }
        g.setColour (juce::Colours::white.withAlpha (0.28f)); g.strokePath (in, juce::PathStrokeType (1.0f));
        juce::Path f (out); f.lineTo (a.getRight(), a.getBottom()); f.lineTo (a.getX(), a.getBottom()); f.closeSubPath();
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.38f), 0, a.getY(), c.withAlpha (0.03f), 0, a.getBottom(), false)); g.fillPath (f);
        g.setColour (juce::Colours::white.withAlpha (0.92f)); g.strokePath (out, juce::PathStrokeType (1.4f));
    };
    // Pop's picture: the low end over time, thumps over its line flattened
    auto popScreen = [&] (R a, bool labels)
    {
        const float thrY = a.getY() + a.getHeight() * 0.4f;
        juce::Random rnd (11);
        const float sx = a.getWidth() / 152.0f;
        for (int i = 0; i < (int) a.getWidth(); i += 2)
        {
            const float t = (float) i / sx;
            float v = 0.15f + 0.2f * rnd.nextFloat();
            for (float pk : { 14.0f, 58.0f, 100.0f, 128.0f })
            { const float d = t - pk; if (d > 0 && d < 16) v = juce::jmax (v, 0.97f * std::exp (-d / 4.5f)); }
            const float y = a.getBottom() - v * a.getHeight(), yo = juce::jmax (y, thrY);
            if (y < thrY) { g.setColour (juce::Colours::white.withAlpha (0.16f)); g.fillRect (a.getX() + (float) i, y, 1.4f, yo - y); }
            g.setColour (y < thrY ? c.withAlpha (0.9f) : c.withAlpha (0.38f)); g.fillRect (a.getX() + (float) i, yo, 1.4f, a.getBottom() - yo);
        }
        g.setColour (c.withAlpha (0.75f)); g.fillRect (a.getX(), thrY, a.getWidth(), 1.0f);
        if (labels)
        {
            text (g, "CAUGHT  4", font (Face::CondMedium, 7.5f, 1.2f), grey, R (a.getX(), a.getBottom() + 3, 80, 9));
            text (g, "time", font (Face::Regular, 7.5f), grey, R (a.getRight() - 40, a.getBottom() + 3, 40, 9), juce::Justification::centredRight);
        }
    };
    auto divider = [&] (float h)
    {
        juce::ColourGradient dg (juce::Colours::white.withAlpha (0.0f), 0, top, juce::Colours::white.withAlpha (0.0f), 0, top + h, false); dg.addColour (0.5, juce::Colours::white.withAlpha (0.1f));
        g.setGradientFill (dg); g.fillRect (k.getCentreX() - 0.5f, top, 1.0f, h);
    };
    if (style == 1)
    {   // B: HARDWARE - a lit bronze VU dial for each tool showing what it's taking out, bigger knobs beside it
        auto vu = [&] (R r, float grDb, const char* cap)
        {
            fillRound (g, r, 8, col ("#0b0b0e", 0.85f));
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.16f), r.getCentreX(), r.getBottom(), c.withAlpha (0.0f), r.getCentreX(), r.getY(), false));
            g.fillRoundedRectangle (r.reduced (2), 7);
            strokeRound (g, r, 8, juce::Colours::white.withAlpha (0.12f));
            const P pv (r.getCentreX(), r.getBottom() + 14);
            const float rad = r.getHeight() - 4, a0 = -0.85f, a1 = 0.85f;   // radians either side of straight up
            auto at = [&] (float ang, float rr) { return P (pv.x + rr * std::sin (ang), pv.y - rr * std::cos (ang)); };
            juce::Path arc; arc.addCentredArc (pv.x, pv.y, rad - 12, rad - 12, 0, a0, a1, true);
            g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (arc, juce::PathStrokeType (1.0f));
            const float dbs[] { 0, 2, 4, 6, 8, 10, 12 };
            for (int i = 0; i < 7; ++i)
            {
                const float ang = a1 - (a1 - a0) * dbs[i] / 12.0f;   // 0 dB at the right, reduction swings left
                const P p0 = at (ang, rad - 12), p1 = at (ang, rad - (i % 2 == 0 ? 20.0f : 16.0f));
                g.setColour (i == 0 ? c : juce::Colours::white.withAlpha (0.5f)); g.drawLine (p0.x, p0.y, p1.x, p1.y, 1.0f);
                if (i % 2 == 0) { const P lp = at (ang, rad - 4); text (g, juce::String ((int) dbs[i]), font (Face::Regular, 7.5f), grey, R (lp.x - 8, lp.y - 5, 16, 9), juce::Justification::centred); }
            }
            const float ang = a1 - (a1 - a0) * juce::jlimit (0.0f, 12.0f, grDb) / 12.0f;
            const P tip = at (ang, rad - 10);
            g.saveState(); g.reduceClipRegion (r.reduced (3).toNearestInt());
            g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawLine (pv.x + 1, pv.y + 1, tip.x + 1, tip.y + 1, 2.2f);
            g.setColour (c.interpolatedWith (juce::Colours::white, 0.3f)); g.drawLine (pv.x, pv.y, tip.x, tip.y, 1.4f);
            g.restoreState();
            text (g, cap, font (Face::CondMedium, 7.5f, 1.6f), grey, R (r.getX(), r.getBottom() - 14, r.getWidth(), 9), juce::Justification::centred);
        };
        auto bigKnobs = [&] (float x, float y, float w, std::initializer_list<std::tuple<const char*, juce::String, float>> ks)
        {
            const int n = (int) ks.size(); const float cw = w / (float) n, sz = 50.0f; int i = 0;
            for (auto& kk : ks)
            {
                const float cx = x + cw * ((float) i++ + 0.5f);
                text (g, std::get<0> (kk), font (Face::CondMedium, 8.5f, 1.5f), ink2, R (cx - cw * 0.5f, y, cw, 10), juce::Justification::centred);
                helperKnob (g, R (cx - sz * 0.5f, y + 14, sz, sz), std::get<2> (kk), c, std::get<1> (kk));
                text (g, std::get<1> (kk), font (Face::Light, 12.0f, 0.3f), ink, R (cx - cw * 0.5f, y + 14 + sz + 3, cw, 14), juce::Justification::centred);
            }
        };
        for (int side = 0; side < 2; ++side)
        {
            const float x = side == 0 ? k.getX() : k.getCentreX() + 20, w = half;
            if (side == 0) header (x, w, top, "DE-ESSER", U8 ("SPLIT  \xc2\xb7  ONLY THE ESSES"), red > 0.5f, MINUS + juce::String (red, 1) + " dB");
            else           header (x, w, top, "POP", U8 ("PLOSIVES  \xc2\xb7  BELOW 150 HZ"), true, MINUS + "5.2 dB");
            const R m (x, top + 36, 150, 74);
            vu (m, side == 0 ? red : 5.2f, side == 0 ? "ESS  GR" : "POP  GR");
            glassButton (g, R (x, top + 116, 72, 20), "LISTEN", false);
            glassButton (g, R (x + 78, top + 116, 72, 20), side == 0 ? "SPLIT" : "SOFT", true, true);
            if (side == 0) bigKnobs (x + 160, top + 40, w - 160, { { "FREQ", "6.5 kHz", 0.6f }, { "AMOUNT", MINUS + "6 dB", 0.5f }, { "RANGE", MINUS + "8 dB", 0.55f } });
            else           bigKnobs (x + 160, top + 40, w - 160, { { "SENSE", "60 %", 0.6f }, { "FREQ", "150 Hz", 0.4f }, { "AMOUNT", MINUS + "9 dB", 0.6f } });
        }
        divider (136);
        return;
    }
    if (style == 2)
    {   // C: WIDE SCREENS - each tool's picture across its whole half, controls as slim glass sliders under it (no knobs)
        auto slider = [&] (R r, const char* label, const juce::String& v, float pos)
        {
            fillRound (g, r, r.getHeight() * 0.5f, col ("#000000", 0.32f));
            strokeRound (g, r, r.getHeight() * 0.5f, juce::Colours::white.withAlpha (0.1f));
            const R fr = r.reduced (2).withWidth ((r.getWidth() - 4) * pos);
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.08f), fr.getX(), 0, c.withAlpha (0.42f), fr.getRight(), 0, false));
            g.fillRoundedRectangle (fr, (r.getHeight() - 4) * 0.5f);
            g.setColour (c.interpolatedWith (juce::Colours::white, 0.5f)); g.fillRoundedRectangle (R (fr.getRight() - 1.5f, r.getY() + 5, 3, r.getHeight() - 10), 1.5f);
            text (g, label, font (Face::CondMedium, 8.0f, 1.4f), ink2, R (r.getX() + 10, r.getY(), 70, r.getHeight()), juce::Justification::centredLeft);
            text (g, v, font (Face::Regular, 10.5f), ink, R (r.getRight() - 70, r.getY(), 62, r.getHeight()), juce::Justification::centredRight);
        };
        for (int side = 0; side < 2; ++side)
        {
            const float x = side == 0 ? k.getX() : k.getCentreX() + 20, w = half;
            if (side == 0) header (x, w, top, "DE-ESSER", U8 ("SPLIT  \xc2\xb7  ONLY THE ESSES"), red > 0.5f, MINUS + juce::String (red, 1) + " dB");
            else           header (x, w, top, "POP", U8 ("PLOSIVES  \xc2\xb7  BELOW 150 HZ"), true, MINUS + "5.2 dB");
            const R sc (x, top + 34, w, 56);
            screen (sc);
            if (side == 0) essScreen (sc.reduced (10, 6).withTrimmedBottom (10)); else popScreen (sc.reduced (10, 6).withTrimmedBottom (10), false);
            const float y = top + 100, sw = (w - 12) / 3.0f;
            if (side == 0)
            {
                slider (R (x, y, sw, 22), "FREQ", "6.5 kHz", 0.6f);
                slider (R (x + sw + 6, y, sw, 22), "AMOUNT", MINUS + "6 dB", 0.5f);
                slider (R (x + 2 * (sw + 6), y, sw, 22), "RANGE", MINUS + "8 dB", 0.55f);
            }
            else
            {
                slider (R (x, y, sw, 22), "SENSE", "60 %", 0.6f);
                slider (R (x + sw + 6, y, sw, 22), "FREQ", "150 Hz", 0.4f);
                slider (R (x + 2 * (sw + 6), y, sw, 22), "AMOUNT", MINUS + "9 dB", 0.6f);
            }
            glassButton (g, R (x, y + 28, 70, 18), "LISTEN", false);
            glassButton (g, R (x + 76, y + 28, 76, 18), side == 0 ? "SPLIT" : "SOFT", true, true);
        }
        divider (146);
        return;
    }
    // ---- DE-ESSER: the top of the spectrum where the esses live, its band, and what it's taking out right now
    {
        const float x = k.getX(), w = half;
        const float sNow = juce::jlimit (0.0f, 1.0f, (float) std::sin (tms / 260.0 + 4.4) * 1.3f - 0.1f);
        const float red = sNow * 5.5f;
        header (x, w, top, "DE-ESSER", U8 ("SPLIT  \xc2\xb7  ONLY THE ESSES"), red > 0.5f, MINUS + juce::String (red, 1) + " dB");
        const R sc (x, top + 36, scW, scH);
        screen (sc);
        const R a = sc.reduced (8, 8).withTrimmedBottom (12);
        auto fx = [&] (float hz) { return a.getX() + std::log2 (hz / 2000.0f) / 3.0f * a.getWidth(); };
        for (float hz : { 2000.0f, 4000.0f, 8000.0f, 16000.0f })
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (fx (hz), a.getY(), 1.0f, a.getHeight());
            text (g, juce::String ((int) (hz / 1000)) + "k", font (Face::Regular, 7.5f), grey, R (fx (hz) - 10, a.getBottom() + 3, 20, 9), juce::Justification::centred);
        }
        const float f0 = 6500.0f;
        g.setColour (c.withAlpha (0.13f)); g.fillRect (R (fx (f0 / 1.45f), a.getY(), fx (f0 * 1.45f) - fx (f0 / 1.45f), a.getHeight()));
        juce::Path in, out; juce::Random rnd (3);
        for (int i = 0; i <= 90; ++i)
        {
            const float hz = 2000.0f * std::pow (8.0f, (float) i / 90.0f);
            const float o = std::log2 (hz / 7000.0f);
            const float lvl = 0.22f + 0.5f * std::exp (-o * o * 2.0f) * (0.45f + sNow * 0.8f) + 0.05f * rnd.nextFloat();
            const float ob = std::log2 (hz / f0), cut = red / 6.0f * 0.5f * std::exp (-ob * ob * 3.5f);
            const P pi (fx (hz), a.getBottom() - juce::jmin (1.0f, lvl) * a.getHeight());
            const P po (fx (hz), a.getBottom() - juce::jlimit (0.0f, 1.0f, lvl - cut) * a.getHeight());
            if (i == 0) { in.startNewSubPath (pi); out.startNewSubPath (po); } else { in.lineTo (pi); out.lineTo (po); }
        }
        g.setColour (juce::Colours::white.withAlpha (0.28f)); g.strokePath (in, juce::PathStrokeType (1.0f));
        juce::Path f (out); f.lineTo (a.getRight(), a.getBottom()); f.lineTo (a.getX(), a.getBottom()); f.closeSubPath();
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.38f), 0, a.getY(), c.withAlpha (0.03f), 0, a.getBottom(), false)); g.fillPath (f);
        g.setColour (juce::Colours::white.withAlpha (0.92f)); g.strokePath (out, juce::PathStrokeType (1.4f));
        const float kx = sc.getRight() + 10, kw = w - scW - 10;
        knobRow (kx, top + 36, kw, { { "FREQ", "6.5 kHz", 0.6f }, { "AMOUNT", MINUS + "6 dB", 0.5f }, { "RANGE", MINUS + "8 dB", 0.55f } });
        glassButton (g, R (kx + 8, top + 112, kw * 0.5f - 12, 20), "LISTEN", false);
        glassButton (g, R (kx + kw * 0.5f + 4, top + 112, kw * 0.5f - 12, 20), "SPLIT", true, true);
    }
    {   // a fine divider
        juce::ColourGradient dg (juce::Colours::white.withAlpha (0.0f), 0, top, juce::Colours::white.withAlpha (0.0f), 0, top + 132, false); dg.addColour (0.5, juce::Colours::white.withAlpha (0.1f));
        g.setGradientFill (dg); g.fillRect (k.getCentreX() - 0.5f, top, 1.0f, 132.0f);
    }
    // ---- POP: the low end over time - thumps from P's and B's poke over its line and get flattened
    {
        const float x = k.getCentreX() + 20, w = half;
        header (x, w, top, "POP", U8 ("PLOSIVES  \xc2\xb7  BELOW 150 HZ"), true, MINUS + "5.2 dB");
        const R sc (x, top + 36, scW, scH);
        screen (sc);
        const R a = sc.reduced (8, 8).withTrimmedBottom (12);
        const float thrY = a.getY() + a.getHeight() * 0.4f;
        juce::Random rnd (11);
        for (int i = 0; i < (int) a.getWidth(); i += 2)
        {
            const float t = (float) i;
            float v = 0.15f + 0.2f * rnd.nextFloat();
            for (float pk : { 14.0f, 58.0f, 100.0f, 128.0f })
            { const float d = t - pk; if (d > 0 && d < 16) v = juce::jmax (v, 0.97f * std::exp (-d / 4.5f)); }
            const float y = a.getBottom() - v * a.getHeight(), yo = juce::jmax (y, thrY);
            if (y < thrY) { g.setColour (juce::Colours::white.withAlpha (0.16f)); g.fillRect (a.getX() + t, y, 1.4f, yo - y); }
            g.setColour (y < thrY ? c.withAlpha (0.9f) : c.withAlpha (0.38f)); g.fillRect (a.getX() + t, yo, 1.4f, a.getBottom() - yo);
        }
        g.setColour (c.withAlpha (0.75f)); g.fillRect (a.getX(), thrY, a.getWidth(), 1.0f);
        text (g, "CAUGHT  4", font (Face::CondMedium, 7.5f, 1.2f), grey, R (a.getX(), a.getBottom() + 3, 80, 9));
        text (g, "time", font (Face::Regular, 7.5f), grey, R (a.getRight() - 40, a.getBottom() + 3, 40, 9), juce::Justification::centredRight);
        const float kx = sc.getRight() + 10, kw = w - scW - 10;
        knobRow (kx, top + 36, kw, { { "SENSE", "60 %", 0.6f }, { "FREQ", "150 Hz", 0.4f }, { "AMOUNT", MINUS + "9 dB", 0.6f } });
        glassButton (g, R (kx + 8, top + 112, kw * 0.5f - 12, 20), "LISTEN", false);
        glassButton (g, R (kx + kw * 0.5f + 4, top + 112, kw * 0.5f - 12, 20), "SOFT", true, true);
    }
}

void Canvas::paintScDrawer (juce::Graphics& g, bool open)
{
    const float x0 = 214.0f, x1 = PW - 214.0f;
    // the glass edge with its handle, like the knee's - lying along the bottom centre when closed, on top of the drawer when open
    auto scEdge = [&] (float ey, bool isOpen)
    {
        const R strip (x0, ey - 7, x1 - x0, 14);
        juce::ColourGradient hg (juce::Colours::white.withAlpha (0.0f), strip.getX(), 0, juce::Colours::white.withAlpha (0.0f), strip.getRight(), 0, false);
        hg.addColour (0.12, juce::Colours::white.withAlpha (0.05f)); hg.addColour (0.88, juce::Colours::white.withAlpha (0.05f));
        g.setGradientFill (hg); g.fillRect (strip);
        juce::ColourGradient lg (juce::Colours::white.withAlpha (0.0f), strip.getX(), 0, juce::Colours::white.withAlpha (0.0f), strip.getRight(), 0, false);
        lg.addColour (0.12, cleanC.withAlpha (0.45f)); lg.addColour (0.5, juce::Colours::white.withAlpha (0.55f)); lg.addColour (0.88, cleanC.withAlpha (0.45f));
        // the line, with a gap in the middle where the name sits (no bubble)
        const R h = gHelperTab ? R (PW * 0.5f - 72, ey - 12, 144, 24) : R (PW * 0.5f - 62, ey - 12, 124, 24);
        dz (h.expanded (10, 4), Hit::PDrawer, 3);
        g.setGradientFill (lg);
        g.fillRect (strip.getX(), strip.getCentreY() - 0.5f, h.getX() - 6 - strip.getX(), 1.0f);
        g.fillRect (h.getRight() + 6, strip.getCentreY() - 0.5f, strip.getRight() - h.getRight() - 6, 1.0f);
        const bool near = h.expanded (40, 30).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_SC") != nullptr;
        g.beginTransparencyLayer (near ? 1.0f : 0.35f);
        for (int i = 0; i < 2; ++i)
        {
            const float cx = h.getX() + 18 + (float) i * 10, cy = h.getCentreY();
            juce::Path c;
            if (isOpen) { c.startNewSubPath (cx - 4, cy - 3); c.lineTo (cx, cy + 2); c.lineTo (cx + 4, cy - 3); }   // down: tuck it away
            else        { c.startNewSubPath (cx - 4, cy + 3); c.lineTo (cx, cy - 2); c.lineTo (cx + 4, cy + 3); }   // up: bring it up
            g.setColour (ink); g.strokePath (c, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        text (g, gHelperTab ? U8 ("DE-ESS  \xc2\xb7  POP") : juce::String ("SIDECHAIN"), font (Face::CondMedium, 9.5f, 1.8f), ink, R (h.getX() + 38, h.getY(), h.getWidth() - 46, h.getHeight()), juce::Justification::centredLeft);
        g.endTransparencyLayer();
    };
    if (! open) { scEdge (652.0f, false); return; }
    const float edgeY = 498.0f;   // a short band along the bottom
    if (gHelperTab)
    {   // HELPERS: the De-esser and Pop on a glass window, the graph fading behind it
        const R k (x0, edgeY, x1 - x0, 668.0f - edgeY);
        {   // frosted glass: the graph behind goes soft and dark from the edge down
            juce::ColourGradient fg (col ("#111218", 0.0f), 0, edgeY - 36, col ("#111218", 0.93f), 0, edgeY + 18, false);
            juce::Path pnl; pnl.addRectangle (R (x0 + 8, edgeY - 36, x1 - x0 - 16, 716.0f - edgeY));
            g.setGradientFill (fg); g.fillPath (pnl);
            juce::ColourGradient tint (helperC.withAlpha (0.0f), 0, edgeY, helperC.withAlpha (0.05f), 0, 668.0f, false);
            g.setGradientFill (tint); g.fillPath (pnl);
        }
        const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
        paintHelperDrawerContent (g, k.reduced (24, 0), tms);
        scEdge (edgeY, true);
        return;
    }
    const R rowR0 (PW * 0.5f - 380.0f, 650.0f - 26.0f, 760, 50);
    const bool onControls = rowR0.expanded (10).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_ROW") != nullptr;
    const float eqFwd = onControls ? 0.3f : 1.0f;
    // the EQ: drawn into its own picture, then faded in from the glass edge downwards
    {
        const R k (x0, edgeY, x1 - x0, 668.0f - edgeY);
        const float sc = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const int iw = juce::roundToInt (k.getWidth() * sc), ih = juce::roundToInt (k.getHeight() * sc);
        juce::Image im (juce::Image::ARGB, iw, ih, true);
        {
            juce::Graphics ig (im);
            ig.addTransform (juce::AffineTransform::translation (-k.getX(), -k.getY()).scaled (sc));
            paintScEq (ig, k.reduced (24, 0).withTrimmedTop (26).withTrimmedBottom (56));
        }
        {
            juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < ih; ++y)
            {
                const float t = (float) y / (float) (ih - 1);
                for (int x = 0; x < iw; ++x)
                {
                    const float u = (float) x / (float) (iw - 1);
                    reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->multiplyAlpha (eqFwd * smoothStep (t / 0.2f) * smoothStep ((1.0f - t) / 0.3f) * smoothStep (u / 0.05f) * smoothStep ((1.0f - u) / 0.05f));   // and fades towards the bottom
                }
            }
        }
        g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / sc).translated (k.getX(), k.getY()));
    }
    // the horizontal glass edge, fading out at both ends, with its handle (arrows down: tuck it away)
    scEdge (edgeY, true);
    // the controls, a row along the bottom centre
    {
        const float cy = 650.0f;   // nearly on the bottom bar
        auto flip = [&] (R r, const char* now) { glassButton (g, r, now, true, true); };
        auto toggle = [&] (R r, const char* t, bool on) { glassButton (g, r, t, on); };
        const R rowR (PW * 0.5f - 380.0f, cy - 26, 760, 50);
        const bool nearRow = rowR.expanded (10).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_ROW") != nullptr;
        g.beginTransparencyLayer (nearRow ? 1.0f : 0.14f);
        float x = PW * 0.5f - 372.0f;
        const bool pro = penv ("PRESS_PRO") != nullptr, sync = pro && penv ("PRESS_PRO_SYNC") != nullptr;
        // LE-PRO's sources: Internal, External, MIDI, Host Sync (a pump in time with the song)
        if (sync) flip (R (x, cy - 11, 100, 22), "HOST SYNC");
        else { flip (R (x, cy - 11, 100, 22), pchoice ("cScSrc").toUpperCase().toRawUTF8()); pctl (R (x, cy - 11, 100, 22), "cScSrc", 1); }
        x += 112;
        toggle (R (x, cy - 11, 80, 22), "LISTEN", pon ("cScListen")); pctl (R (x, cy - 11, 80, 22), "cScListen", 1); x += 96;
        if (sync)
        {
            valueBar (g, R (x - 4, cy - 21, 76, 42), "OFFSET", "1/16", 0.25f);
            valueBar (g, R (x + 78, cy - 21, 76, 42), "LENGTH", "1/4", 0.5f);
            x += 164;
        }
        // sidechain gain: a short fader (up and down)
        else
        {
            const R tr (x + 6, cy - 22, 4, 44);
            fillRound (g, tr, 2, juce::Colours::white.withAlpha (0.1f));
            const float pos = pn ("cScGain"), hy = tr.getBottom() - tr.getHeight() * pos;
            fillRound (g, R (tr.getX(), hy, 4, tr.getBottom() - hy), 2, cleanC);
            fillRound (g, R (tr.getCentreX() - 9, hy - 4, 18, 8), 3, col ("#e9e5dd"));
            text (g, "SC GAIN", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (x + 20, cy - 12, 60, 11));
            text (g, pfmt ("cScGain"), font (Face::Regular, 11), ink, R (x + 20, cy + 1, 60, 13));
            pctl (R (x - 4, cy - 24, 84, 48), "cScGain", 0);
            x += 92;
        }
        // stereo link: a sideways slider - right is linked, left is unlinked
        {
            const R tr (x, cy + 4, sync ? 90.0f : 150.0f, 4);
            text (g, "LINK", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (x, cy - 16, 60, 11));
            const float lk = pn ("cScLink");
            text (g, pfmt ("cScLink") + (sync ? "" : (lk > 0.99f ? "  linked" : lk < 0.01f ? "  unlinked" : "")), font (Face::Regular, 10.5f), ink, R (x, cy - 17, tr.getWidth(), 13), juce::Justification::centredRight);
            fillRound (g, tr, 2, juce::Colours::white.withAlpha (0.1f));
            fillRound (g, tr.withWidth (tr.getWidth() * lk), 2, cleanC);
            fillRound (g, R (tr.getX() + tr.getWidth() * lk - 4, tr.getCentreY() - 8, 8, 16), 3, col ("#e9e5dd"));
            pctl (R (x - 4, cy - 18, tr.getWidth() + 8, 36), "cScLink", 0);
            text (g, "unlinked", font (Face::Regular, 8.5f), grey, R (x, cy + 11, 60, 10));
            if (! sync) text (g, "linked", font (Face::Regular, 8.5f), grey, R (x + 90, cy + 11, 60, 10), juce::Justification::centredRight);
            x += tr.getWidth() + 20;
        }
        // mid / side and look-ahead: buttons whose values you drag
        valueBar (g, R (x, cy - 21, 96, 42), "M / S", pchoice ("cScMs"), pn ("cScMs")); pctl (R (x, cy - 21, 96, 42), "cScMs", 1); x += 108;
        valueBar (g, R (x, cy - 21, 96, 42), "LOOK-AHEAD", pfmt ("cLook") + (pro ? U8 (" \xc2\xb7 max 10") : juce::String()), pn ("cLook"));
        pctl (R (x, cy - 21, 96, 42), "cLook", 0);
        g.endTransparencyLayer();
    }
}

// the detector EQ, big: frequency grid, what the sidechain sounds like (faint), the curve and its three points
void Canvas::paintScEq (juce::Graphics& g, R e)
{
    auto fxS = [&] (float hz) { return e.getX() + e.getWidth() * std::log10 (hz / 20.0f) / 3.0f; };
    const float midY = e.getY() + e.getHeight() * 0.42f, dbPx = 4.5f;
    auto fs = font (Face::Regular, 9.5f);
    for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        g.setColour (juce::Colours::white.withAlpha (hz == 100.0f || hz == 1000.0f || hz == 10000.0f ? 0.08f : 0.045f));
        g.fillRect (std::round (fxS (hz)), e.getY(), 1.0f, e.getHeight());
        text (g, hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz), fs, grey, R (fxS (hz) - 15, e.getBottom() + 4, 30, 11), juce::Justification::centred);
    }
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (e.getX(), midY, e.getWidth(), 1.0f);
    {
        juce::Path sp; sp.startNewSubPath (e.getX(), e.getBottom());
        for (int i = 0; i <= 160; ++i)
        {
            const float t = (float) i / 160.0f, hz = 20.0f * std::pow (1000.0f, t);
            const float lvl = -10.0f - 14.0f * std::abs (std::log10 (hz / 300.0f)) + 3.0f * std::sin ((float) i * 0.9f);
            sp.lineTo (fxS (hz), juce::jlimit (e.getY(), e.getBottom(), midY - (lvl + 12.0f) * 3.4f));
        }
        sp.lineTo (e.getRight(), e.getBottom()); sp.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, midY - 40, juce::Colours::white.withAlpha (0.0f), 0, e.getBottom(), false));
        g.fillPath (sp);
    }
    const bool band = penv ("PRESS_DS1_BAND") != nullptr;
    const bool pro = false;   // (V1: every engine's detector has the same three bands - the 580E's six come later)
    const float hpHz = proc.param ("cScHp"), bellHz = proc.param ("cScBellHz"), bellDb = proc.param ("cScBellDb"), lpHz = proc.param ("cScLp");
    auto respDb = [band, pro, hpHz, bellHz, bellDb, lpHz] (float hz)
    {
        if (pro)   // six bands: brickwall low and high cuts, four bells between
        {
            const float hp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (60.0f / hz, 24.0f)));
            const float lp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hz / 16000.0f, 24.0f)));
            auto bell = [hz] (float f, float db, float w) { const float o = std::log2 (hz / f); return db * std::exp (-o * o * w); };
            return hp + lp + bell (150.0f, -3.0f, 2.0f) + bell (450.0f, 2.0f, 2.5f) + bell (2000.0f, -2.5f, 2.0f) + bell (6000.0f, 4.0f, 1.6f);
        }
        if (band)   // a band-pass around the bell's frequency, an octave wide: only this part of the sound is compressed
        {
            const float oct = std::log2 (hz / bellHz);
            return 8.0f * std::exp (-oct * oct * 2.2f) - 14.0f * (1.0f - std::exp (-oct * oct * 2.2f));
        }
        const float hp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hpHz / hz, 4.0f)));
        const float lp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hz / lpHz, 4.0f)));
        const float oct = std::log2 (hz / bellHz);
        return hp + lp + bellDb * std::exp (-oct * oct * 1.4f);
    };
    juce::Path c;
    for (int i = 0; i <= 300; ++i)
    {
        const float hz = 20.0f * std::pow (1000.0f, (float) i / 300.0f);
        const float y = juce::jlimit (e.getY() + 2, e.getBottom() + 80.0f, midY - respDb (hz) * dbPx);   // the low end drops all the way down
        if (i == 0) c.startNewSubPath (fxS (hz), y); else c.lineTo (fxS (hz), y);
    }
    // bolder: a dark edge, a violet glow, then a bright line
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.strokePath (c, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (cleanC.withAlpha (0.3f)); g.strokePath (c, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (cleanC.interpolatedWith (juce::Colours::white, 0.5f)); g.strokePath (c, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    auto hzTxt = [] (float hz) { return hz >= 1000.0f ? juce::String (hz / 1000.0f, hz < 10000.0f ? 1 : 0).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " kHz" : juce::String ((int) std::round (hz)) + " Hz"; };
    const juce::String nt[3] { "HP " + hzTxt (hpHz), hzTxt (bellHz) + "  " + (bellDb >= 0 ? "+" : "") + juce::String (bellDb, 1) + " dB", "LP " + hzTxt (lpHz) };
    const struct { float hz; const char* t; } nodes[3] { { hpHz, nt[0].toRawUTF8() }, { bellHz, nt[1].toRawUTF8() }, { lpHz, nt[2].toRawUTF8() } };
    const juce::String bt = "BAND-PASS  " + hzTxt (bellHz) + U8 ("  \xc2\xb7  1 oct");
    const struct { float hz; const char* t; } bandNode[1] { { bellHz, bt.toRawUTF8() } };
    if (band)
    {
        const float x = fxS (bandNode[0].hz), y = juce::jlimit (e.getY() + 10, e.getBottom() - 10, midY - respDb (bandNode[0].hz) * dbPx);
        g.setColour (cleanC.withAlpha (0.4f)); g.fillEllipse (x - 11, y - 11, 22, 22);
        g.setColour (cleanC.interpolatedWith (juce::Colours::white, 0.2f)); g.fillEllipse (x - 7.5f, y - 7.5f, 15, 15);
        text (g, juce::String::fromUTF8 (bandNode[0].t), font (Face::Medium, 11), juce::Colours::white, R (x - 110, y - 26, 220, 13), juce::Justification::centred);
        text (g, "drag for frequency", font (Face::Regular, 9.5f), grey, R (x - 150, y - 42, 300, 12), juce::Justification::centred);
        pctl (R (x - 14, y - 14, 28, 28), "cScBellHz", 4);
    }
    const struct { float hz; const char* t; } proNodes[6] { { 60.0f, "60 Hz brickwall" }, { 150.0f, "150 Hz" }, { 450.0f, "450 Hz" }, { 2000.0f, "2 kHz" }, { 6000.0f, "6 kHz  +4 dB" }, { 16000.0f, "16 kHz brickwall" } };
    for (int i = 0; i < (pro ? 6 : 0); ++i)
    {
        const float x = fxS (proNodes[i].hz), y = juce::jlimit (e.getY() + 10, e.getBottom() - 10, midY - respDb (proNodes[i].hz) * dbPx + (i == 0 || i == 5 ? 0.0f : 0.0f));
        const float yy = (i == 0 || i == 5) ? midY : y;
        g.setColour (cleanC.withAlpha (0.4f)); g.fillEllipse (x - 11, yy - 11, 22, 22);
        g.setColour (cleanC.interpolatedWith (juce::Colours::white, 0.2f)); g.fillEllipse (x - 7.5f, yy - 7.5f, 15, 15);
        text (g, juce::String (i + 1), font (Face::SemiBold, 9), col ("#16161a"), R (x - 6.5f, yy - 6.5f, 13, 13), juce::Justification::centred);
        if (i == 0 || i == 4 || i == 5) text (g, proNodes[i].t, font (Face::Medium, 10.5f), juce::Colours::white, R (x - 55, yy - 28, 110, 13), juce::Justification::centred);
    }
    for (int i = 0; i < (band || pro ? 0 : 3); ++i)
    {
        const float x = fxS (nodes[i].hz), y = juce::jlimit (e.getY() + 10, e.getBottom() - 10, midY - respDb (nodes[i].hz) * dbPx);
        g.setColour (cleanC.withAlpha (0.4f)); g.fillEllipse (x - 11, y - 11, 22, 22);
        g.setColour (cleanC.interpolatedWith (juce::Colours::white, 0.2f)); g.fillEllipse (x - 7.5f, y - 7.5f, 15, 15);
        text (g, juce::String (i + 1), font (Face::SemiBold, 9), col ("#16161a"), R (x - 6.5f, y - 6.5f, 13, 13), juce::Justification::centred);
        text (g, juce::String::fromUTF8 (nodes[i].t), font (Face::Medium, 11), juce::Colours::white, R (x - 60, y + 13, 120, 13), juce::Justification::centred);
        const R nz (x - 14, y - 14, 28, 28);
        if (i == 1) { pctl (nz, "cScBellHz", 5); pz.push_back ("cScBellDb"); }   // drag across for frequency, up and down for gain
        else pctl (nz, i == 0 ? "cScHp" : "cScLp", 4);
    }
    text (g, U8 ("DETECTOR EQ  \xc2\xb7  what the compressor listens to"), font (Face::CondMedium, 9.5f, 1.6f), col ("#b9b3a7"), R (e.getX(), e.getY() - 22, 400, 12));
}

// the engine, in the arch at the top of the screen: its name, with arrows either side - slide (or click an arrow) to
// move between LE-RC and LE-DS1; two dots show which of the two it is
void Canvas::paintEngineSlide (juce::Graphics& g)
{
    const float cx = W * 0.5f, cy = 30.0f;   // up in the arch, above the strip that slides down
    const bool rv = penv ("PRESS_RVOX") != nullptr;
    const bool ds = penv ("PRESS_DS1") != nullptr;
    const bool ml = penv ("PRESS_MLA") != nullptr;
    const bool pr = penv ("PRESS_PRO") != nullptr;
    if (gCompMode)
    {
        const int unit = juce::jlimit (0, kVinUnits - 1, penv ("PRESS_VUNIT") ? std::atoi (penv ("PRESS_VUNIT")) : preview);
        static const char* extra[8] { "LE-8X", "LE-160", "LE-7", "LE-760", "LE-SHM", "LE-2A", "LE-1B", "LE-VSC" };
        const juce::String nm = unit < numCompUnits ? juce::String (units()[(size_t) unit].name) : juce::String (extra[unit - numCompUnits]);
        text (g, nm, font (Face::Light, 15, 2.6f), ink, R (cx - 80, cy - 10, 160, 18), juce::Justification::centred);
        static const char* subs[15] { "VINTAGE FET", "VARI-MU TUBE", "TUBE LEVELER", "BUS COMPRESSOR", "CONSOLE GLUE", "TUBE MASTERING", "DIODE BRIDGE", "DISTRESSOR",
                                     "VCA PUNCH", "TAPE SIMULATOR", "COMP / LIMIT / GATE", "MASTERING DUAL", "OPTICAL TUBE", "TUBE OPTICAL", "DISCRETE VCA" };
        text (g, subs[unit], font (Face::CondMedium, 8.5f, 2.2f), juce::Colours::white.withAlpha (0.9f), R (cx - 90, cy + 9, 180, 11), juce::Justification::centred);
        const float half = textWidth (font (Face::Light, 15, 2.6f), nm) * 0.5f + 12.0f;
        auto chv = [&] (float x, bool left)
        {
            juce::Path c;
            if (left) { c.startNewSubPath (x + 3, cy - 5); c.lineTo (x - 2, cy); c.lineTo (x + 3, cy + 5); }
            else      { c.startNewSubPath (x - 3, cy - 5); c.lineTo (x + 2, cy); c.lineTo (x - 3, cy + 5); }
            g.setColour (cleanC.withAlpha (0.8f)); g.strokePath (c, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        chv (cx - half, true); chv (cx + half, false);
        zone (R (cx - half - 14, cy - 14, 24, 28), Hit::PEngine, -1); zone (R (cx + half - 10, cy - 14, 24, 28), Hit::PEngine, 1);
        for (int i = 0; i < kVinUnits; ++i)
        {
            const P d (cx - (float) (kVinUnits - 1) * 4.0f + (float) i * 8, cy + 26);
            g.setColour (i == unit ? cleanC : juce::Colours::white.withAlpha (0.2f)); g.fillEllipse (d.x - 2, d.y - 2, 4, 4);
        }
        return;
    }
    const juce::String nm = rv ? "LE-755C" : ds ? "LE-DS1" : ml ? "LE-MLA" : pr ? "LE-580E" : "LE-11/25E";
    // what each one is for, under its name
    const juce::String sub = rv ? "VOCAL GRADE" : (ds || ml || pr) ? "MASTERING GRADE" : "MIX GRADE";
    text (g, nm, font (Face::Light, 15, 2.6f), ink, R (cx - 80, cy - 10, 160, 18), juce::Justification::centred);
    auto chev = [&] (float x, bool left)
    {
        juce::Path c;
        if (left) { c.startNewSubPath (x + 3, cy - 5); c.lineTo (x - 2, cy); c.lineTo (x + 3, cy + 5); }
        else      { c.startNewSubPath (x - 3, cy - 5); c.lineTo (x + 2, cy); c.lineTo (x - 3, cy + 5); }
        g.setColour (cleanC.withAlpha (0.8f)); g.strokePath (c, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    text (g, sub, font (Face::CondMedium, 8.5f, 2.2f), juce::Colours::white.withAlpha (0.9f), R (cx - 90, cy + 9, 180, 11), juce::Justification::centred);
    const float half = textWidth (font (Face::Light, 15, 2.6f), nm) * 0.5f + 12.0f;   // the arrows sit just outside the name, however long
    chev (cx - half, true); chev (cx + half, false);
    zone (R (cx - half - 14, cy - 14, 24, 28), Hit::PEngine, -1); zone (R (cx + half - 10, cy - 14, 24, 28), Hit::PEngine, 1);
    for (int i = 0; i < 5; ++i)   // LE-RC, RVox, LE-DS1, LE-MLA, LE-PRO
    {
        const P d (cx - 16 + (float) i * 8, cy + 26);
        g.setColour (i == (rv ? 1 : ds ? 2 : ml ? 3 : pr ? 4 : 0) ? cleanC : juce::Colours::white.withAlpha (0.2f)); g.fillEllipse (d.x - 2, d.y - 2, 4, 4);
    }
}

// ---- the envelope drawer: how fast - a test hit, the reduction diving in at the attack and climbing back at the release
void Canvas::paintEnvOverlay (juce::Graphics& g, R k, float alpha)
{
    const float sc = juce::jlimit (0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int iw = juce::roundToInt (k.getWidth() * sc), ih = juce::roundToInt (k.getHeight() * sc);
    juce::Image im (juce::Image::ARGB, juce::jmax (1, iw), juce::jmax (1, ih), true);
    {
        juce::Graphics ig (im);
        ig.addTransform (juce::AffineTransform::translation (-k.getX(), -k.getY()).scaled (sc));
        if (gHelperTab) paintDynEqContent (ig, k); else if (gCompMode) paintCharContent (ig, k); else paintEnvContent (ig, k);
    }
    {
        juce::Image::BitmapData bd (im, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < ih; ++y)
        {
            const float t = (float) y / (float) juce::jmax (1, ih - 1);
            const float f = juce::jlimit (0.0f, 1.0f, juce::jmin (t / 0.16f, (1.0f - t) / 0.16f)) * alpha;
            const float e = f * f * (3.0f - 2.0f * f);
            for (int x = 0; x < iw; ++x) reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->multiplyAlpha (e);
        }
    }
    g.drawImageTransformed (im, juce::AffineTransform::scale (1.0f / sc).translated (k.getX(), k.getY()));
}

void Canvas::paintEnvContent (juce::Graphics& g, R k)
{
    const R a = k.reduced (44, 96);
    const bool ds1 = penv ("PRESS_DS1") != nullptr, mla = penv ("PRESS_MLA") != nullptr,
               c755 = penv ("PRESS_RVOX") != nullptr;
    // each engine's timing (ms)
    const float att = ds1 ? 3.0f : mla ? 0.16f : c755 ? 2.0f : 8.0f;
    const float rel = ds1 ? 200.0f : mla ? 800.0f : c755 ? 80.0f : 120.0f, relSlow = 1200.0f;
    const float hit = 400.0f, depth = 12.0f;                           // the test: a 400 ms hit that wants 12 dB of reduction
    // time runs in two parts: the hit, zoomed in (log, so a fast attack shows), then the release in plain time
    const float tmin = 0.1f, split = 0.46f;
    const float relSpan = ds1 ? 2600.0f : juce::jmax (600.0f, rel * 5.0f);
    const float tmax = hit + relSpan;
    auto px = [&] (float ms)
    {
        if (ms <= hit) return a.getX() + std::log (juce::jmax (tmin, ms) / tmin) / std::log (hit / tmin) * a.getWidth() * split;
        return a.getX() + a.getWidth() * (split + (1.0f - split) * juce::jmin (1.0f, (ms - hit) / relSpan));
    };
    auto py = [&] (float db) { return a.getY() + 20.0f + (-db) / 24.0f * (a.getHeight() - 20.0f); };   // 0 dB near the top, reduction hangs down
    auto grAt = [&] (float ms, int which)   // which: 0 the real curve, 1 fast only, 2 slow only (LE-DS1)
    {
        const float g0 = -depth * (1.0f - std::exp (-hit / att));
        if (ms <= hit) return -depth * (1.0f - std::exp (-ms / att));
        const float x = ms - hit;
        if (! ds1 || which == 1) return g0 * std::exp (-x / rel);
        if (which == 2) return g0 * std::exp (-x / relSlow);
        return g0 * (0.65f * std::exp (-x / rel) + 0.35f * std::exp (-x / relSlow));   // average: fast for the peak, slow for the body
    };
    auto fs = font (Face::Regular, 9);
    // grid: time across (log), reduction down
    {
        const float ticks[] { 1, 10, 100 };
        const char* tl[] { "1 ms", "10 ms", "100 ms" };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (px (ticks[i]), a.getY(), 1.0f, a.getHeight());
            text (g, tl[i], fs, grey, R (px (ticks[i]) - 24, a.getBottom() + 6, 48, 10), juce::Justification::centred);
        }
        // the release side, in plain time after the hit ends
        const float step = relSpan > 2000.0f ? 1000.0f : relSpan > 900.0f ? 500.0f : 200.0f;
        for (float t = step; t <= relSpan + 1.0f; t += step)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (px (hit + t), a.getY(), 1.0f, a.getHeight());
            text (g, "+" + (t >= 1000.0f ? juce::String (t / 1000.0f, 1) + " s" : juce::String ((int) t) + " ms"), fs, grey, R (px (hit + t) - 24, a.getBottom() + 6, 48, 10), juce::Justification::centred);
        }
        // where the hit stops: a slightly brighter divider
        g.setColour (juce::Colours::white.withAlpha (0.14f)); g.fillRect (px (hit), a.getY(), 1.0f, a.getHeight());
        for (int db = 0; db >= -24; db -= 6)
        {
            g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.12f : 0.05f)); g.fillRect (a.getX(), py ((float) db), a.getWidth(), 1.0f);
            if (db < 0) text (g, MINUS + juce::String (-db), fs, grey, R (a.getX() + 4, py ((float) db) + 2, 24, 10), juce::Justification::centredLeft);
        }
    }
    // the hit itself: a soft band along the bottom while the sound is loud
    {
        const R hb (px (tmin), a.getBottom() - 16, px (hit) - px (tmin), 10);
        g.setColour (cleanC.withAlpha (0.16f)); g.fillRoundedRectangle (hb, 3);
        text (g, "THE HIT  400 ms", font (Face::CondMedium, 8.5f, 1.4f), grey, hb.translated (0, -14).withHeight (11), juce::Justification::centred);
    }
    // markers: thin glass lines where the attack lands and where the release gets back
    auto marker = [&] (float ms, const juce::String& lab, float labY)
    {
        const float x = px (ms);
        juce::ColourGradient gg (juce::Colours::white.withAlpha (0.0f), x, a.getY(), juce::Colours::white.withAlpha (0.0f), x, a.getBottom(), false);
        gg.addColour (0.5, juce::Colours::white.withAlpha (0.28f));
        g.setGradientFill (gg); g.fillRect (x - 0.5f, a.getY(), 1.0f, a.getHeight());
        text (g, lab, font (Face::CondMedium, 9, 1.4f), cleanC.interpolatedWith (juce::Colours::white, 0.3f), R (x + 5, labY, 120, 11), juce::Justification::centredLeft);
    };
    auto fmt = [] (float ms) { return ms >= 1000.0f ? juce::String (ms / 1000.0f, 1) + " s" : ms < 1.0f ? juce::String (ms, 2) + " ms" : juce::String (ms, ms < 10.0f ? 1 : 0) + " ms"; };
    marker (att, "ATTACK  " + fmt (att), py (-depth) + 8);
    marker (hit + rel, (ds1 ? "REL FAST  " : "RELEASE  ") + fmt (rel), py (-depth) + 8);
    if (ds1) marker (hit + relSlow, "REL SLOW  " + fmt (relSlow), py (-depth) + 24);
    // the curve(s)
    auto pathOf = [&] (int which)
    {
        juce::Path pth;
        for (int i = 0; i <= 300; ++i)
        {
            const float f = (float) i / 300.0f;
            const float ms = f <= split ? tmin * std::pow (hit / tmin, f / split) : hit + relSpan * (f - split) / (1.0f - split);
            const P pt (px (ms), py (grAt (ms, which)));
            if (i == 0) pth.startNewSubPath (pt); else pth.lineTo (pt);
        }
        return pth;
    };
    if (ds1)   // the fast and slow releases on their own, faint, either side of the real (average) curve
    {
        const float dl[] { 3.0f, 4.0f };
        for (int w = 1; w <= 2; ++w)
        {
            juce::Path d; juce::PathStrokeType (1.0f).createDashedStroke (d, pathOf (w), dl, 2);
            g.setColour (juce::Colours::white.withAlpha (0.3f)); g.fillPath (d);
        }
    }
    const auto main = pathOf (0);
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.strokePath (main, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.95f)); g.strokePath (main, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // the dot, running along the curve again and again, with its rainbow sparkle trail (like the knee)
    {
        const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
        const float u = penv ("PRESS_ENV_U") ? (float) std::atof (penv ("PRESS_ENV_U")) : (float) std::fmod (tms / 3000.0, 1.0);
        auto at = [&] (float uu) { const float ms = uu <= split ? tmin * std::pow (hit / tmin, uu / split) : hit + relSpan * (uu - split) / (1.0f - split); return P (px (ms), py (grAt (ms, 0))); };
        auto lift = [&] (P c) { return 0.15f + 0.85f * smoothStep ((a.getBottom() - c.y) / (a.getHeight() * 0.8f)); };
        juce::Random rnd (7);
        for (int i = 40; i >= 1; --i)
        {
            const float uu = u - (float) i * 0.006f; if (uu < 0.0f) continue;
            const float age = (float) i / 40.0f;
            const P c = at (uu);
            const float jx = (rnd.nextFloat() - 0.5f) * 20.0f * age, jy = (rnd.nextFloat() - 0.5f) * 20.0f * age;
            const auto rc = juce::Colour::fromHSV (std::fmod ((float) i * 0.045f + u, 1.0f), 0.7f, 1.0f, 1.0f);
            g.setColour (rc.withAlpha (0.7f * (1.0f - age) * lift (c)));
            const float r = 1.2f + 3.0f * (1.0f - age);
            g.fillEllipse (c.x + jx - r * 0.5f, c.y + jy - r * 0.5f, r, r);
        }
        const P c = at (u);
        const auto gold = col ("#ffd27a");
        const float l = lift (c);
        g.setGradientFill (juce::ColourGradient (gold.withAlpha (0.6f * l), c.x, c.y, gold.withAlpha (0.0f), c.x + 16, c.y, true));
        g.fillEllipse (c.x - 16, c.y - 16, 32, 32);
        g.setColour (gold.interpolatedWith (juce::Colours::white, 0.45f).withAlpha (0.3f + 0.7f * l)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9, 9);
        g.setColour (juce::Colours::white.withAlpha (0.25f + 0.75f * l)); g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
    }
    // what it says
    text (g, "ENVELOPE", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 100, 12));
    juce::String rd = mla ? juce::String (U8 ("0.02 ms/dB  \xc2\xb7  0.10 s/dB  \xc2\xb7  follows the music"))
                    : ds1 ? "attack " + fmt (att) + U8 ("  \xc2\xb7  ") + fmt (rel) + " / " + fmt (relSlow) + U8 ("  \xc2\xb7  average")
                          : "attack " + fmt (att) + U8 ("  \xc2\xb7  release ") + fmt (rel);
    text (g, rd, font (Face::Regular, 10.5f), ink, R (a.getRight() - 260, a.getY() - 31, 260, 13), juce::Justification::centredRight);
    if (c755) text (g, "FIXED", font (Face::CondMedium, 9, 1.6f), cleanC, R (a.getX() + 70, a.getY() - 30, 60, 12));
    text (g, "TIME", font (Face::CondMedium, 9, 1.6f), grey, R (a.getRight() - 30, a.getBottom() + 16, 30, 10), juce::Justification::centredRight);
    text (g, "GR", font (Face::CondMedium, 9, 1.6f), grey, R (a.getX() + 6, a.getY() + 4, 30, 10), juce::Justification::centredLeft);
}

void Canvas::paintEnvEdge (juce::Graphics& g, bool open)
{
    const float ex = open ? PW - 614.0f : PW - 214.0f;   // the mirror of the knee's edge
    const R strip (ex, 0, 14, 670);
    {
        juce::ColourGradient vg (juce::Colours::white.withAlpha (0.0f), 0, strip.getY(), juce::Colours::white.withAlpha (0.0f), 0, strip.getBottom(), false);
        vg.addColour (0.12, juce::Colours::white.withAlpha (0.05f)); vg.addColour (0.88, juce::Colours::white.withAlpha (0.05f));
        g.setGradientFill (vg); g.fillRect (strip);
        juce::ColourGradient lg (juce::Colours::white.withAlpha (0.0f), 0, strip.getY(), juce::Colours::white.withAlpha (0.0f), 0, strip.getBottom(), false);
        lg.addColour (0.12, cleanC.withAlpha (0.45f)); lg.addColour (0.5, juce::Colours::white.withAlpha (0.55f)); lg.addColour (0.88, cleanC.withAlpha (0.45f));
        g.setGradientFill (lg); g.fillRect (strip.getCentreX() - 0.5f, strip.getY(), 1.0f, strip.getHeight());
    }
    {
        const R h (strip.getCentreX() - 12, 310, 24, 60);
        dz (h.expanded (8, 6), Hit::PDrawer, 1);
        const bool near = h.expanded (30, 40).contains (mouse - display.getPosition()) || penv ("PRESS_HOVER_ENV") != nullptr;
        g.beginTransparencyLayer (near ? 1.0f : 0.18f);
        juce::Path p; p.addRoundedRectangle (h, 12);
        cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 10, { 0, 3 }), p);
        g.setGradientFill (juce::ColourGradient (col ("#2c2b38", 0.85f), h.getX(), h.getY(), col ("#17171d", 0.85f), h.getRight(), h.getBottom(), false));
        g.fillPath (p);
        g.setColour (cleanC.withAlpha (0.7f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        for (int i = 0; i < 2; ++i)
        {
            const float cy = h.getCentreY() - 6 + (float) i * 12, cx = h.getCentreX();
            juce::Path c;   // closed: points left (pull it out across the screen); open: points right (tuck it back)
            if (open) { c.startNewSubPath (cx - 3, cy - 4); c.lineTo (cx + 2, cy); c.lineTo (cx - 3, cy + 4); }
            else      { c.startNewSubPath (cx + 3, cy - 4); c.lineTo (cx - 2, cy); c.lineTo (cx + 3, cy + 4); }
            g.setColour (ink); g.strokePath (c, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.endTransparencyLayer();
    }
}

// ---- the CHARACTER drawer (VINTAGE): what the unit does to the tone - its saturation curve and the harmonics it adds
void Canvas::paintCharContent (juce::Graphics& g, R k)
{
    const R a = k.reduced (44, 96);
    const float thd = penv ("PRESS_THD") ? (float) std::atof (penv ("PRESS_THD")) / 100.0f : 0.35f;
    const float drive = 0.15f + 0.85f * thd;   // THD bends the curve and grows the harmonics
    const R tc (a.getX(), a.getY() + 10, a.getWidth(), a.getHeight() * 0.5f);
    // THD lives here, with what it does: a slim slider whose fill glows brighter amber the further it goes (like the EQs')
    {
        const float v = thd, cyT = tc.getBottom() + 34.0f;
        auto fm = font (Face::CondMedium, 10, 1.2f);
        const R box (a.getX() + a.getWidth() * 0.5f - 110, cyT - 12, 220, 24);
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (box, 12);
        g.setColour (juce::Colours::white.withAlpha (0.14f)); g.drawRoundedRectangle (box.reduced (0.5f), 12, 1.0f);
        text (g, "THD", fm, col ("#b9b3a7"), R (box.getX() + 14, box.getY(), 30, 24));
        const R tr (box.getX() + 48, cyT - 1.5f, 120, 3);
        fillRound (g, tr, 1.5f, juce::Colours::white.withAlpha (0.1f));
        const auto glow = col ("#7a5418").interpolatedWith (col ("#ffcf7a"), v);
        if (v > 0.001f)
        {
            const R on = tr.withWidth (tr.getWidth() * v);
            g.setColour (glow.withAlpha (0.12f + 0.3f * v)); g.fillRoundedRectangle (on.expanded (1.5f + 2.5f * v), 3.5f);
            fillRound (g, on, 1.5f, glow);
        }
        const float kx = tr.getX() + tr.getWidth() * v;
        g.setColour (glow.withAlpha (0.25f * v)); g.fillEllipse (kx - 8, cyT - 8, 16, 16);
        g.setColour (col ("#f4ede0")); g.fillEllipse (kx - 4.5f, cyT - 4.5f, 9, 9);
        g.setColour (glow); g.fillEllipse (kx - 2, cyT - 2, 4, 4);
        text (g, juce::String ((int) std::round (v * 100.0f)) + "%", font (Face::Regular, 10.5f), v > 0.5f ? col ("#ffcf7a") : col ("#b9b3a7"), R (tr.getRight() + 10, box.getY(), 40, 24));
    }
    auto fs = font (Face::Regular, 9);
    for (int i = 0; i <= 4; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (i == 2 ? 0.1f : 0.05f));
        g.fillRect (tc.getX() + tc.getWidth() * (float) i / 4.0f, tc.getY(), 1.0f, tc.getHeight());
        g.fillRect (tc.getX(), tc.getY() + tc.getHeight() * (float) i / 4.0f, tc.getWidth(), 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.14f)); g.drawLine (tc.getX(), tc.getBottom(), tc.getRight(), tc.getY(), 1.0f);   // a clean, straight line
    auto curveY = [&] (float x) { const float k2 = 1.0f + 3.0f * drive; return juce::jlimit (-1.05f, 1.05f, std::tanh (k2 * x) / std::tanh (k2) + 0.06f * drive * x * x); };
    auto at = [&] (float x) { return P (tc.getX() + (x + 1.0f) * 0.5f * tc.getWidth(), tc.getCentreY() - curveY (x) * 0.5f * tc.getHeight()); };
    juce::Path cv;
    for (int i = 0; i <= 200; ++i) { const P pt = at (-1.0f + 2.0f * (float) i / 200.0f); if (i == 0) cv.startNewSubPath (pt); else cv.lineTo (pt); }
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.strokePath (cv, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.95f)); g.strokePath (cv, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    {
        const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
        const float ph = penv ("PRESS_CHAR_X") ? (float) std::atof (penv ("PRESS_CHAR_X")) : (float) std::sin (tms / 900.0) * 0.78f;
        juce::Random rnd (3);
        for (int i = 24; i >= 1; --i)
        {
            const float age = (float) i / 24.0f;
            const P c = at (ph - (float) i * 0.012f);
            const auto rc = juce::Colour::fromHSV (std::fmod ((float) i * 0.05f, 1.0f), 0.7f, 1.0f, 1.0f);
            g.setColour (rc.withAlpha (0.7f * (1.0f - age)));
            const float r = 1.2f + 3.0f * (1.0f - age), jx = (rnd.nextFloat() - 0.5f) * 16.0f * age, jy = (rnd.nextFloat() - 0.5f) * 16.0f * age;
            g.fillEllipse (c.x + jx - r * 0.5f, c.y + jy - r * 0.5f, r, r);
        }
        const P c = at (ph);
        const auto gold = col ("#ffd27a");
        g.setGradientFill (juce::ColourGradient (gold.withAlpha (0.6f), c.x, c.y, gold.withAlpha (0.0f), c.x + 16, c.y, true));
        g.fillEllipse (c.x - 16, c.y - 16, 32, 32);
        g.setColour (gold.interpolatedWith (juce::Colours::white, 0.45f)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9, 9);
        g.setColour (juce::Colours::white); g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
    }
    text (g, "IN", font (Face::CondMedium, 9, 1.6f), grey, R (tc.getRight() - 20, tc.getBottom() + 4, 20, 10), juce::Justification::centredRight);
    text (g, "OUT", font (Face::CondMedium, 9, 1.6f), grey, R (tc.getX() + 6, tc.getY() + 4, 30, 10), juce::Justification::centredLeft);
    {
        const R hb (a.getX(), tc.getBottom() + 84, a.getWidth(), a.getBottom() - tc.getBottom() - 96);
        const float base[7] { 0.0f, 0.62f, 0.48f, 0.22f, 0.16f, 0.08f, 0.05f };   // (mock: the FET's mix - more 2nd and 3rd)
        float lv[7]; for (int i = 0; i < 7; ++i) lv[i] = base[i] * (0.25f + 1.2f * thd) * (i > 2 ? 0.6f + thd : 1.0f);
        const char* nm[7] { "", "2nd", "3rd", "4th", "5th", "6th", "7th" };
        const float bwid = hb.getWidth() / 6.0f;
        for (int i = 1; i < 7; ++i)
        {
            const float h = hb.getHeight() * lv[i];
            const R bar (hb.getX() + bwid * (float) (i - 1) + 8, hb.getBottom() - h, bwid - 16, h);
            g.setGradientFill (juce::ColourGradient (cleanC.withAlpha (0.75f), 0, bar.getY(), cleanC.withAlpha (0.12f), 0, bar.getBottom(), false));
            g.fillRoundedRectangle (bar, 3);
            g.setColour (juce::Colours::white.withAlpha (0.5f)); g.fillRect (bar.getX(), bar.getY(), bar.getWidth(), 1.0f);
            text (g, nm[i], fs, i <= 2 ? ink : grey, R (bar.getCentreX() - 20, hb.getBottom() + 6, 40, 10), juce::Justification::centred);
        }
        text (g, "HARMONICS", font (Face::CondMedium, 9, 1.6f), grey, R (hb.getX(), hb.getY() - 14, 100, 10));
        text (g, U8 ("even = warm  \xc2\xb7  odd = edge"), fs, grey, R (hb.getRight() - 160, hb.getY() - 14, 160, 10), juce::Justification::centredRight);
    }
    text (g, "CHARACTER", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 110, 12));
    static const char* cr[15] { "FET colour", "vari-mu tube colour", "tube colour", "transformer punch", "console sheen", "tube & iron colour", "diode & iron colour", "Dist 2 / Dist 3 colour",
                                "VCA bite", "tape & tranny colour", "discrete colour", "transformer colour", "opto tube colour", "tube colour", "discrete VCA colour" };
    const int cu = juce::jlimit (0, kVinUnits - 1, penv ("PRESS_VUNIT") ? std::atoi (penv ("PRESS_VUNIT")) : 0);
    text (g, juce::String (cr[cu]) + U8 ("  \xc2\xb7  THD ") + juce::String ((int) std::round (thd * 100.0f)) + "%", font (Face::Regular, 10.5f), ink, R (a.getRight() - 240, a.getY() - 31, 240, 13), juce::Justification::centredRight);
}

// ---- HELPER's TONE drawer: the high pass and Lift's two bands on a frequency graph; the bands rise and settle with the music

// UPWARD mode: each band as a level curve - below its threshold quiet parts are lifted (up to its Lift), above it untouched
static void paintUpwardCurves (juce::Graphics& g, R a, juce::Colour liC, double tms)
{
    // UPWARD mode: for each band, how much it lifts at every input level - the full Lift on quiet parts, easing to none at its threshold
    const float gap = 36.0f, bw = (a.getWidth() - gap) * 0.5f;
    struct Band { const char* name; const char* hz; float thr, lift, ratio, lvl; double per, ph; };
    const Band bands[] {
        { "LOW",  "120 Hz", -30.0f, 4.0f, 1.2f, -40.0f + 9.0f * (float) std::sin (tms / 700.0), 700.0, 0.0 },
        { "HIGH", "6 kHz",  -34.0f, 3.0f, 1.2f, -44.0f + 10.0f * (float) std::sin (tms / 530.0 + 1.3), 530.0, 1.3 } };
    auto gainAt = [] (const Band& b, float in)
    {
        const float under = b.thr - in, kn = 6.0f;
        float u = under <= -kn * 0.5f ? 0.0f : under >= kn * 0.5f ? under : (under + kn * 0.5f) * (under + kn * 0.5f) / (2.0f * kn);
        return juce::jmin (b.lift, u * (1.0f - 1.0f / b.ratio));
    };
    auto fs = font (Face::Regular, 9);
    for (int bi = 0; bi < 2; ++bi)
    {
        const Band& b = bands[bi];
        const R box (a.getX() + (bi == 0 ? 0.0f : bw + gap), a.getY() + 14, bw, a.getHeight() - 14);
        auto px = [&] (float db) { return box.getX() + (db + 60.0f) / 60.0f * box.getWidth(); };
        auto py = [&] (float lift) { return box.getBottom() - lift / 6.0f * box.getHeight(); };
        for (int db = -60; db <= 0; db += 12)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (px ((float) db), box.getY(), 1.0f, box.getHeight());
            text (g, db == 0 ? juce::String ("0") : MINUS + juce::String (-db), fs, grey, R (px ((float) db) - 14, box.getBottom() + 6, 28, 10), juce::Justification::centred);
        }
        for (int l = 0; l <= 6; l += 2)
        {
            g.setColour (juce::Colours::white.withAlpha (l == 0 ? 0.16f : 0.05f)); g.fillRect (box.getX(), py ((float) l), box.getWidth(), 1.0f);
            if (l > 0) text (g, "+" + juce::String (l), fs, grey, R (box.getX() + 4, py ((float) l) - 11, 24, 10));
        }
        // its threshold
        g.setColour (liC.withAlpha (0.5f)); g.fillRect (px (b.thr) - 0.5f, box.getY(), 1.0f, box.getHeight());
        text (g, "THRESH " + MINUS + juce::String ((int) -b.thr), font (Face::CondMedium, 8.5f, 1.2f), liC.withAlpha (0.9f), R (px (b.thr) + 6, box.getY() + 2, 80, 10));
        // how much it can lift, dashed, at the top of the shelf
        { juce::Path l, d; l.startNewSubPath (box.getX(), py (b.lift)); l.lineTo (px (b.thr), py (b.lift));
          const float dl[] { 3.0f, 4.0f }; juce::PathStrokeType (1.0f).createDashedStroke (d, l, dl, 2);
          g.setColour (liC.withAlpha (0.45f)); g.fillPath (d);
          text (g, "LIFT +" + juce::String ((int) b.lift) + " dB", font (Face::CondMedium, 8.5f, 1.2f), liC, R (box.getX() + 30, py (b.lift) - 13, 90, 10)); }
        // the curve and the lift under it
        juce::Path c;
        for (int i = 0; i <= 240; ++i)
        {
            const float in = -60.0f + 60.0f * (float) i / 240.0f;
            const P pt (px (in), py (gainAt (b, in)));
            if (i == 0) c.startNewSubPath (pt); else c.lineTo (pt);
        }
        juce::Path fill (c); fill.lineTo (px (0), py (0)); fill.lineTo (px (-60), py (0)); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (liC.withAlpha (0.32f), 0, py (b.lift), liC.withAlpha (0.04f), 0, py (0), false)); g.fillPath (fill);
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.strokePath (c, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.95f)); g.strokePath (c, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // the band's level right now riding the curve, leaving a trail
        for (int i = 12; i >= 1; --i)
        {
            const float age = (float) i / 12.0f;
            const float lv = b.lvl - 5.0f * age * (float) std::cos (tms / b.per + b.ph);
            const P tp (px (lv), py (gainAt (b, lv)));
            g.setColour (liC.interpolatedWith (juce::Colours::white, 0.3f).withAlpha (0.55f * (1.0f - age)));
            const float r = 1.5f + 3.2f * (1.0f - age); g.fillEllipse (tp.x - r * 0.5f, tp.y - r * 0.5f, r, r);
        }
        const float gnow = gainAt (b, b.lvl);
        const P d (px (b.lvl), py (gnow));
        g.setColour (liC.withAlpha (0.35f)); g.drawLine (d.x, d.y + 6, d.x, py (0), 1.0f);
        g.setGradientFill (juce::ColourGradient (liC.withAlpha (0.6f), d.x, d.y, liC.withAlpha (0.0f), d.x + 14, d.y, true));
        g.fillEllipse (d.x - 14, d.y - 14, 28, 28);
        g.setColour (liC.interpolatedWith (juce::Colours::white, 0.5f)); g.fillEllipse (d.x - 4.5f, d.y - 4.5f, 9, 9);
        g.setColour (juce::Colours::white); g.fillEllipse (d.x - 2, d.y - 2, 4, 4);
        text (g, "+" + juce::String (gnow, 1) + " dB", font (Face::CondMedium, 9, 1.2f), ink, R (d.x + 10, d.y - 16, 70, 11));
        text (g, juce::String (b.name) + U8 ("  \xc2\xb7  ") + b.hz, font (Face::CondMedium, 9.5f, 2.0f), ink, R (box.getRight() - 110, box.getCentreY() - 6, 104, 11), juce::Justification::centredRight);
        text (g, "IN", fs, grey, R (box.getRight() - 20, box.getBottom() + 17, 20, 10), juce::Justification::centredRight);
    }
}

void Canvas::paintDynEqContent (juce::Graphics& g, R k)
{
    const R a = k.reduced (44, 96);
    if (! liftDynEq())
    {
        const double tms0 = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
        paintUpwardCurves (g, a, helperC, tms0);
        text (g, "TONE", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 110, 12));
        text (g, U8 ("Lift  \xc2\xb7  upward, 2 bands"), font (Face::Regular, 10.5f), ink, R (a.getRight() - 240, a.getY() - 31, 240, 13), juce::Justification::centredRight);
        return;
    }
    const auto hpC = col ("#7fd3ff"), liC = helperC;
    auto fx = [&] (float hz) { return a.getX() + std::log10 (hz / 20.0f) / 3.0f * a.getWidth(); };
    auto fy = [&] (float db) { return a.getCentreY() - db / 15.0f * a.getHeight() * 0.5f; };
    auto fs = font (Face::Regular, 9);
    {
        const float hzs[] { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
        const char* hl[] { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
        for (int i = 0; i < 8; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (fx (hzs[i]), a.getY(), 1.0f, a.getHeight());
            text (g, hl[i], fs, grey, R (fx (hzs[i]) - 16, a.getBottom() + 6, 32, 10), juce::Justification::centred);
        }
        for (int db = -12; db <= 12; db += 6)
        {
            g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.14f : 0.05f)); g.fillRect (a.getX(), fy ((float) db), a.getWidth(), 1.0f);
            if (db != 0) text (g, (db > 0 ? "+" : MINUS) + juce::String (std::abs (db)), fs, grey, R (a.getX() + 4, fy ((float) db) - 11, 26, 10));
        }
    }
    const double tms = penv ("PRESS_BREATH") ? std::atof (penv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
    // how much each band is lifting right now (mock: breathing with the music)
    const float l1 = 4.0f * (0.55f + 0.45f * (float) std::sin (tms / 700.0)), l2 = 3.0f * (0.55f + 0.45f * (float) std::sin (tms / 530.0 + 1.3));
    auto bell = [] (float hz, float f0, float db, float w) { const float o = std::log2 (hz / f0); return db * std::exp (-o * o * w); };
    auto hp = [] (float) { return 0.0f; };   // (the high pass is gone from HELPER)
    auto curve = [&] (std::function<float (float)> f)
    {
        juce::Path p;
        for (int i = 0; i <= 240; ++i)
        {
            const float hz = 20.0f * std::pow (1000.0f, (float) i / 240.0f);
            const P pt (fx (hz), juce::jlimit (a.getY(), a.getBottom(), fy (f (hz))));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        return p;
    };
    // the bands' reach: where each can lift to, faint
    {
        auto reach = curve ([&] (float hz) { return bell (hz, 120.0f, 4.0f, 1.2f) + bell (hz, 6000.0f, 3.0f, 1.0f); });
        const float dl[] { 3.0f, 4.0f };
        juce::Path d; juce::PathStrokeType (1.0f).createDashedStroke (d, reach, dl, 2);
        g.setColour (liC.withAlpha (0.35f)); g.fillPath (d);
    }
    // the whole thing right now: high pass + both bands as they're lifting this moment - bright, with a glow under it
    {
        auto total = curve ([&] (float hz) { return hp (hz) + bell (hz, 120.0f, l1, 1.2f) + bell (hz, 6000.0f, l2, 1.0f); });
        juce::Path fill (total); fill.lineTo (a.getRight(), fy (0)); fill.lineTo (a.getX(), fy (0)); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (liC.withAlpha (0.22f), 0, fy (5), liC.withAlpha (0.0f), 0, fy (0), false)); g.fillPath (fill);
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.strokePath (total, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.95f)); g.strokePath (total, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // each band's dot, riding up and down with its lift, an arrow showing which way it moves, a little sparkle behind
    auto bandDot = [&] (float f0, float lift, float maxLift, const char* name)
    {
        const P c (fx (f0), fy (hp (f0) + lift));
        const P base (fx (f0), fy (hp (f0)));
        g.setColour (liC.withAlpha (0.4f)); g.drawLine (base.x, base.y, c.x, c.y + 6, 1.0f);
        juce::Random rnd ((int) f0);
        for (int i = 1; i <= 10; ++i)
        {
            const float age = (float) i / 10.0f;
            const P sp (c.x + (rnd.nextFloat() - 0.5f) * 14.0f * age, c.y + 4.0f + 16.0f * age);
            const auto sc = liC.interpolatedWith (juce::Colour::fromHSV (std::fmod ((float) i * 0.07f, 1.0f), 0.6f, 1.0f, 1.0f), 0.4f);
            g.setColour (sc.withAlpha (0.7f * (1.0f - age))); const float r = 1.0f + 2.4f * (1.0f - age);
            g.fillEllipse (sp.x - r * 0.5f, sp.y - r * 0.5f, r, r);
        }
        g.setGradientFill (juce::ColourGradient (liC.withAlpha (0.6f), c.x, c.y, liC.withAlpha (0.0f), c.x + 14, c.y, true));
        g.fillEllipse (c.x - 14, c.y - 14, 28, 28);
        g.setColour (liC.interpolatedWith (juce::Colours::white, 0.5f)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9, 9);
        g.setColour (juce::Colours::white); g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
        text (g, juce::String (name) + "  +" + juce::String (lift, 1) + " dB", font (Face::CondMedium, 9, 1.2f), ink, R (c.x - 60, c.y - 28, 120, 11), juce::Justification::centred);
        juce::ignoreUnused (maxLift);
    };
    bandDot (120.0f, l1, 4.0f, "LOW");
    bandDot (6000.0f, l2, 3.0f, "HIGH");
    text (g, "TONE", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 110, 12));
    text (g, U8 ("Lift  \xc2\xb7  dynamic EQ, 2 bands"), font (Face::Regular, 10.5f), ink, R (a.getRight() - 240, a.getY() - 31, 240, 13), juce::Justification::centredRight);
}
