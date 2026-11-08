// Pressure - the dynamics screens (mock): a scrolling picture of the level, newest on the right.
// HELPER: the rider, breath control and peak catcher. CLEAN: a modern compressor drawn on the screen.
// VINTAGE: the same picture over the chained hardware.

namespace
{
    inline float dynY (float db) { return 40.0f + juce::jlimit (0.0f, 62.0f, -db) / 60.0f * 590.0f; }
    const juce::Colour helperC = col ("#2bc2b0"), peakC = col ("#ffb347");
    // gain reduction: red, except LE-DS1, whose display is black with grey bars like the real DS1-MK3
    inline juce::Colour grColour() { return std::getenv ("PRESS_MLA") != nullptr ? col ("#8fcaff") : std::getenv ("PRESS_DS1") != nullptr ? col ("#a7abb1") : col ("#ff5a6a"); }
    // each CLEAN engine wears the colour of the original it comes from
    juce::Colour engineColour()
    {
        if (std::getenv ("PRESS_RVOX") != nullptr) return col ("#e8a98f");   // LE-755C: rose gold
        if (std::getenv ("PRESS_DS1")  != nullptr) { if (auto* c = std::getenv ("PRESS_DS1_COL")) return col (c); return col ("#efe5cf"); }   // LE-DS1: white gold (the DS1-MK3's white face, in the gold family)
        if (std::getenv ("PRESS_MLA")  != nullptr) return col ("#ecd59c");   // LE-MLA: pale gold over an ivory wave - Maselec MLA-2: silver and black, like the DS1 look - its blue light is in the meters and the reduction
        if (std::getenv ("PRESS_PRO")  != nullptr) { if (auto* c = std::getenv ("PRESS_PRO_COL")) return col (c); return col ("#c9a24e"); }   // LE-PRO: antique brass   // Pro-C 3: its yellow
        if (std::getenv ("PRESS_VIOLET") != nullptr) return col ("#a99bff");
        if (auto* c = std::getenv ("PRESS_RC_COL")) return col (c);
        return col ("#a8916a");   // LE-RC: smoked gold                                               // LE-RC (Renaissance Comp): its amber faders
    }
}
#define cleanC engineColour()
#define grC grColour()

void Canvas::pushHistory()
{
    const int n = (int) ((PW - 50.0f) / 2.0f);
    if ((int) hIn.size() != n) { hIn.assign ((size_t) n, -90.0f); hOut = hIn; hGr.assign ((size_t) n, 0.0f); hRide = hGr; hBreath = hGr; hPos = 0; }
    const float in = juce::Decibels::gainToDecibels (juce::jmax (proc.inMeterL.load(), proc.inMeterR.load()), -90.0f);
    // the compressor (soft knee) the screen draws
    const bool rvox0 = std::getenv ("PRESS_RVOX") != nullptr && std::getenv ("PRESS_RVOX_DEMO") == nullptr;
    const bool heavy = std::getenv ("PRESS_GR_HEAVY") != nullptr;   // (mock: pushed hard, to show lots of gain reduction)
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
    hGr[(size_t) hPos] = tab == 0 ? 0.0f : compEnv;
    hRide[(size_t) hPos] = ride;
    hBreath[(size_t) hPos] = breathEnv;
    hOut[(size_t) hPos] = tab == 0 ? juce::jmin (-6.0f, in + ride + breathEnv) : in - compEnv;
    hPos = (hPos + 1) % n;
}

void Canvas::paintDynScreen (juce::Graphics& g, int kind)
{
    const int n = (int) hIn.size();
    if (n < 2) return;
    const float xR = PW - 50.0f;
    auto tc = kind == 0 ? helperC : kind == 1 ? cleanC : amber;
    if (kind == 1 && std::getenv ("PRESS_RVOX") != nullptr) tc = col ("#dcc4b8");   // LE-755C: a warm blush wave under the rose gold
    if (kind == 1 && std::getenv ("PRESS_PRO") != nullptr) tc = col ("#b8a57f");    // LE-PRO: a bronze-tinted wave under the brass
    if (kind == 1 && std::getenv ("PRESS_MLA") != nullptr) tc = col ("#ebe7de");    // LE-MLA: ivory wave
    if (kind == 1 && std::getenv ("PRESS_DS1") != nullptr) tc = col ("#6e6a63");    // LE-DS1: a black wave (its black display)
    if (kind == 1 && std::getenv ("PRESS_RVOX") == nullptr && std::getenv ("PRESS_DS1") == nullptr && std::getenv ("PRESS_MLA") == nullptr && std::getenv ("PRESS_PRO") == nullptr)
        tc = col ("#7e7567");                                                     // LE-RC: a slate wave under the smoked gold
    if (kind == 1) if (auto* w = std::getenv ("PRESS_WAVE_COL")) tc = col (w);   // (mock: try a separate waveform colour)
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
    if (kind == 1 && std::getenv ("PRESS_RVOX") != nullptr && std::getenv ("PRESS_RVOX_DEMO") != nullptr)   // gate on only
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
        g.setColour (helperC.withAlpha (0.25f)); g.strokePath (rp, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (helperC); g.strokePath (rp, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (int a = 0; a < n; ++a)
            if (at (hBreath, a) < -2.0f)
            {
                const float x = xR - (float) a * 2.0f;
                g.setColour (col ("#7fb8ff", 0.18f + 0.03f * -at (hBreath, a))); g.fillRect (x - 1.0f, 600.0f, 2.0f, 26.0f);
            }
        const float py = cy - hOf (-6.0f);
        const float dashes[2] { 6.0f, 5.0f };
        g.setColour (peakC.withAlpha (0.8f));
        g.drawDashedLine (juce::Line<float> (0.0f, py, xR, py), dashes, 2, 1.2f);
        g.drawDashedLine (juce::Line<float> (0.0f, cy + hOf (-6.0f), xR, cy + hOf (-6.0f)), dashes, 2, 1.2f);
        auto f = font (Face::CondMedium, 10, 1.4f);
        auto pill = [&] (float x, float y, const juce::String& t, juce::Colour c)
        {
            const float w = textWidth (f, t) + 18;
            const R b (x - w, y - 10, w, 20);
            fillRound (g, b, 10, col ("#121216", 0.9f)); strokeRound (g, b, 10, c.withAlpha (0.6f));
            text (g, t, f, c, b, juce::Justification::centred);
        };
        const float ride = at (hRide, 0);
        pill (xR - 180, mid - ride * 10.0f - 18, "RIDER  " + juce::String (ride > 0 ? "+" : ride < 0 ? MINUS : juce::String()) + juce::String (std::abs (ride), 1) + " dB", helperC);
        pill (xR - 180, py - 16, U8 ("1964 PEAK  \xe2\x88\x92") + "6.0 dB", peakC);
        pill (xR - 180, 588, U8 ("BREATHS  \xe2\x88\x92") + "8 dB", col ("#7fb8ff"));
        return;
    }
    // gain reduction, hanging from the top (1 dB = 9 px)
    {
        juce::Path fill, edge;
        fill.startNewSubPath (xR, 40.0f);
        for (int a = 0; a < n; ++a) { const float x = xR - (float) a * 2.0f, y = 40.0f + juce::jmin (24.0f, at (hGr, a)) * 9.0f; fill.lineTo (x, y); if (a == 0) edge.startNewSubPath (x, y); else edge.lineTo (x, y); }
        fill.lineTo (xR - (float) (n - 1) * 2.0f, 40.0f); fill.closeSubPath();
        if (kind == 1 && std::getenv ("PRESS_RVOX") == nullptr)
        {
            // LE-RC, LE-DS1, LE-MLA and LE-PRO: just a red line, like Pro-C 3 - no fill
            g.setColour (col ("#000000", 0.45f)); g.strokePath (edge, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col ("#ff4d5e")); g.strokePath (edge, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (kind == 1 && std::getenv ("PRESS_DS1") != nullptr)
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
static int gKnobSteps = 0;   // > 0: a stepped knob (like the Maselec's precision switches) - detent dots, not ticks
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
    if (std::getenv ("PRESS_DS1") != nullptr) { mThr = -14.0f; mRatio = 2.0f; mKnee = 17.0f; }
    const bool mla = std::getenv ("PRESS_MLA") != nullptr;   // LE-MLA: the Maselec MLA-2 - drive into a fixed threshold, stepped controls
    if (mla) { mThr = -20.0f; mRatio = 2.0f; mKnee = 8.0f; }   // LE-DS1: Goliath's defaults, soft knee 0.6 (~17 dB)
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
    // RVox: Goliath's one-knob vocal leveler - pushing COMPRESS levels the vocal and turns it up (like RVox);
    // LEVEL MATCH holds the level even instead, so you hear the compression, not the volume
    if (std::getenv ("PRESS_RVOX") != nullptr)
    {
        {
            const R s (0, top, pw, ph);
            panel (s, false);
            const float cx = s.getCentreX();
            float y = s.getY() + 70;
            const bool demo = std::getenv ("PRESS_RVOX_DEMO") != nullptr;   // (mock: a pushed setting, to show it working)
            y = bigKnob (cx + 4, y, 118, "COMPRESS", demo ? MINUS + "18.0 dB" : juce::String ("0.0 dB"), demo ? 0.62f : 0.0f) + 30;
            // RVox's gate (Goliath's soft one, about 1:4 below its level): keeps the quiet gaps quiet as you push Compress
            y = bigKnob (cx, y, 70, "GATE", demo ? MINUS + "48 dB" : juce::String ("Off"), demo ? 0.2f : 0.0f) + 8;
            text (g, "keeps the gaps between", font (Face::Regular, 10), grey, R (s.getX() + 10, y, pw - 20, 13), juce::Justification::centred);
            text (g, "phrases quiet", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 14, pw - 20, 13), juce::Justification::centred);
        }
        {
            const R s (PW - pw, top, pw, ph);
            panel (s, true);
            const float cx = s.getCentreX();
            float y = s.getY() + 70;
            y = bigKnob (cx, y, 96, "GAIN", "+0.0 dB", 0.5f) + 20;
            {
                const R br (s.getX() + 18, y, pw - 36, 22);
                // LEVEL MATCH: off, pushing Compress turns it up (like RVox); on, the level stays even
                glassButton (g, br, "LEVEL MATCH", false);
                text (g, "off: pushing Compress", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 28, pw - 20, 13), juce::Justification::centred);
                text (g, "turns the level up", font (Face::Regular, 10), grey, R (s.getX() + 10, y + 42, pw - 20, 13), juce::Justification::centred);
                y += 72;
            }
            y = bigKnob (cx, y, 64, "MIX", "100%", 1.0f);
        }
        // the knee drawer stays: RVox's own curve - a fixed 3:1 with a very soft 10 dB knee, display only
        {
            const float keepT = mThr, keepR = mRatio, keepK = mKnee;
            mThr = std::getenv ("PRESS_RVOX_DEMO") ? -18.0f : 0.0f; mRatio = 3.0f; mKnee = 10.0f;
            const double since = juce::Time::getMillisecondCounterHiRes() - kneeTouch;
            const float alpha = (kneeOn || std::getenv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
            if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);
            paintKneeEdge (g, kneeOn || std::getenv ("PRESS_KNEE") != nullptr);
            mThr = keepT; mRatio = keepR; mKnee = keepK;
        }
        paintTopStrip (g, std::getenv ("PRESS_TOPOFF") == nullptr);
        paintScDrawer (g, std::getenv ("PRESS_SC") != nullptr);
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
                const bool red = std::getenv ("PRESS_MLA_LAMPOFF") == nullptr;
                const auto lc = red ? col ("#ff4a3d") : col ("#4ade80");
                const P l (cx + 62, y + 6);
                g.setGradientFill (juce::ColourGradient (lc.withAlpha (0.55f), l.x, l.y, lc.withAlpha (0.0f), l.x + 11, l.y, true));
                g.fillEllipse (l.x - 11, l.y - 11, 22, 22);
                g.setColour (lc.interpolatedWith (juce::Colours::white, 0.25f)); g.fillEllipse (l.x - 3.5f, l.y - 3.5f, 7, 7);
                g.setColour (juce::Colours::white.withAlpha (0.7f)); g.fillEllipse (l.x - 1.5f, l.y - 2.2f, 2.2f, 2.2f);
            }
            gKnobSteps = 21; y = bigKnob (cx, y, 104, "INPUT GAIN", MINUS + "8 dB", 12.0f / 20.0f) + 22;
            gKnobSteps = 9;  y = bigKnob (cx, y, 80, "RATIO", "2 : 1", 3.0f / 8.0f) + 24;   // 1.2 1.4 1.7 2 2.5 3 5 8 inf
            gKnobSteps = 0;
            // ImageLink: links the two sides, but lets a short peak on one side through on its own
            glassButton (g, R (s.getX() + 18, y, pw - 36, 24), "IMAGELINK", true);
            text (g, "links L / R, lets short peaks through", font (Face::Regular, 9.5f), grey, R (s.getX() + 8, y + 30, pw - 16, 12), juce::Justification::centred);
            ratioY = y;
        }
        else {
        { const float thr = std::getenv ("PRESS_DS1") != nullptr ? -14.0f : mThr;
          y = bigKnob (cx, y, 104, "THRESHOLD", MINUS + juce::String (-thr, 1) + " dB", 1.0f - (-thr) / 60.0f) + 22; }
        ratioY = y;
        y = bigKnob (cx, y, 80, "RATIO", (std::getenv ("PRESS_DS1") != nullptr ? juce::String ("2") : juce::String (mRatio, 1).trimCharactersAtEnd (".0")) + " : 1", std::getenv ("PRESS_DS1") != nullptr ? 0.3f : 0.35f) + 10;
        y += 14;   // the ratio picks are gone - the wheel does it all (LE-DS1's wheel turns past 1:1 into expansion)
        // the knee: its knob (the graph appears big over the screen while you touch it)
        if (std::getenv ("PRESS_DS1") != nullptr)
        {
            // the real DS1's soft-knee: a variable amount, 0 (hard) to 1 (from 0 dBFS down to twice the threshold)
            y = bigKnob (cx, y, 64, "SOFT KNEE", "0.6", 0.6f);
        }
        else
            y = bigKnob (cx, y, 64, "KNEE", juce::String (mKnee, 0) + " dB", mKnee / 24.0f);
        if (std::getenv ("PRESS_PRO") != nullptr)
        {
            // LE-PRO's STYLE (like Pro-C 3): one compressor, many characters - click the arrows or the name for the list
            const R b (s.getX() + 18, 650.0f - 21.0f, pw - 36, 42);
            g.setColour (cleanC.withAlpha (0.08f)); g.fillRoundedRectangle (b, 6);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, b.getY(), juce::Colours::white.withAlpha (0.0f), 0, b.getCentreY(), false));
            g.fillRoundedRectangle (b.reduced (1), 5);
            g.setColour (cleanC.withAlpha (0.45f)); g.drawRoundedRectangle (b.reduced (0.5f), 6, 1.0f);
            text (g, "STYLE", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (b.getX() + 8, b.getY() + 5, 60, 11));
            text (g, "CLEAN", font (Face::Regular, 13.5f, 1.2f), ink, R (b.getX() + 22, b.getY() + 17, b.getWidth() - 44, 17), juce::Justification::centred);
            auto chev = [&] (float x, bool left)
            {
                juce::Path c; const float yy = b.getY() + 25.5f;
                if (left) { c.startNewSubPath (x + 3, yy - 5); c.lineTo (x - 2, yy); c.lineTo (x + 3, yy + 5); }
                else      { c.startNewSubPath (x - 3, yy - 5); c.lineTo (x + 2, yy); c.lineTo (x - 3, yy + 5); }
                g.setColour (cleanC.withAlpha (0.85f)); g.strokePath (c, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };
            chev (b.getX() + 14, true); chev (b.getRight() - 14, false);
            text (g, "1 / 14", font (Face::CondMedium, 8.5f, 1.2f), grey, R (b.getRight() - 58, b.getY() + 5, 50, 11), juce::Justification::centredRight);
        }
        }
        // the knee window's tab (replaced by the drawer's glass edge - kept off)
        if (false)
        {
            const bool on = kneeOn || std::getenv ("PRESS_KNEE") != nullptr;
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
        const float alpha = (kneeOn || std::getenv ("PRESS_KNEE") != nullptr) ? 1.0f : (float) juce::jlimit (0.0, 1.0, (4000.0 - since) / 400.0);
        if (alpha > 0.01f) paintKneeOverlay (g, R (218.0f, 94.5f, 359.0f, 474.0f).expanded (44, 96), alpha);   // the Figma size
        paintKneeEdge (g, kneeOn || std::getenv ("PRESS_KNEE") != nullptr);
    }
    // right: attack and release (with auto release), then make-up and mix side by side, and auto gain
    {
        const R s (PW - pw, top, pw, ph);
        panel (s, true);
        const float cx = s.getCentreX();
        float y = s.getY() + 22;
        const bool ds1 = std::getenv ("PRESS_DS1") != nullptr;
        if (mla)
        {
            // attack in ms per dB and release in s per dB, stepped - and both follow the music on their own
            gKnobSteps = 9; y = bigKnob (cx, y, 76, "ATTACK", "0.02 ms/dB", 3.0f / 8.0f) + 10;
            gKnobSteps = 9; y = bigKnob (cx, y, 76, "RELEASE", "0.10 s/dB", 4.0f / 8.0f) + 4;
            gKnobSteps = 0;
            text (g, "both follow the program", font (Face::Regular, 9.5f), grey, R (s.getX() + 8, y, pw - 16, 12), juce::Justification::centred);
            y += 32;
        }
        else if (ds1)
        {
            y = bigKnob (cx, y, 70, "ATTACK", "3.0 ms", 0.3f) + 8;
            // two releases, like the real DS1: fast for the peaks, slow for the average level
            const float sz = 54;
            const struct { const char* lab; const char* val; float pos; float x; } rk[2] { { "REL FAST", "200 ms", 0.35f, cx - 46 }, { "REL SLOW", "1.2 s", 0.6f, cx + 46 } };
            for (auto& k : rk)
            {
                title (R (k.x - 46, y, 92, 12), k.lab);
                modernKnob (g, R (k.x - sz * 0.5f, y + 20, sz, sz), k.pos, cleanC);
                text (g, k.val, font (Face::Light, 14, 0.5f), ink, R (k.x - 46, y + 20 + sz + 8, 92, 18), juce::Justification::centred);
            }
            y += 20 + sz + 8 + 18 + 12;
            valueBar (g, R (s.getX() + 18, y, pw - 36, 42), "AVERAGE", "30 ms", 0.3f);
            y += 42 + 22;
        }
        else
        {
            y = bigKnob (cx, y, 76, "ATTACK", "8.0 ms", 0.4f) + 10;
            y = bigKnob (cx, y, 76, "RELEASE", "120 ms", 0.45f) + 6;
        }
        auto lit = [&] (float yy, const char* t)
        {
            // faint until the mouse comes over it (like Range and Hold)
            const R br (s.getX() + 18, yy, pw - 36, 22);
            const bool near = br.expanded (8).contains (mouse - display.getPosition()) || std::getenv ("PRESS_HOVER_RH") != nullptr;
            g.beginTransparencyLayer (near ? 1.0f : 0.14f);
            glassButton (g, br, t, true);
            g.endTransparencyLayer();
        };
        if (! ds1 && ! mla) { lit (y, "AUTO RELEASE"); y += 36; }
        // LE-PRO: make-up, then separate dry and wet levels (linked so they cross over, like Pro-C 3)
        if (std::getenv ("PRESS_PRO") != nullptr)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (s.getX() + 18, y - 10, pw - 36, 1.0f);
            const float sz = 44;
            const struct { const char* lab; const char* val; float pos; float x; } ks[3] { { "MAKE-UP", "+4.0 dB", 0.6f, cx - 58 }, { "DRY", "\xe2\x88\x92\xe2\x88\x9e", 0.0f, cx }, { "WET", "0.0 dB", 1.0f, cx + 58 } };
            for (auto& k : ks)
            {
                title (R (k.x - 30, y, 60, 12), k.lab);
                modernKnob (g, R (k.x - sz * 0.5f, y + 20, sz, sz), k.pos, cleanC);
                text (g, U8 (k.val), font (Face::Light, 12.5f, 0.3f), ink, R (k.x - 32, y + 20 + sz + 8, 64, 16), juce::Justification::centred);
            }
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
            const struct { const char* lab; const char* val; float pos; float x; } ks[2] { { mla ? "OUTPUT" : "MAKE-UP", "+4.0 dB", 0.6f, cx - 46 }, { "MIX", "100%", 1.0f, cx + 46 } };
            for (auto& k : ks)
            {
                title (R (k.x - 46, y, 92, 12), k.lab);
                modernKnob (g, R (k.x - sz * 0.5f, y + 20, sz, sz), k.pos, cleanC);
                text (g, k.val, font (Face::Light, 14, 0.5f), ink, R (k.x - 46, y + 20 + sz + 8, 92, 18), juce::Justification::centred);
            }
            y += 20 + sz + 8 + 18 + 14;
        }
        lit (y, "AUTO GAIN");
        // range and hold: slim value bars level with the sidechain's look-ahead row - faint until the mouse comes over them
        // (not on LE-MLA: its timing is the program's)
        if (! mla)
        {
            const float bw = (pw - 36 - 10) * 0.5f, ry = 650.0f - 21.0f;
            const R both (s.getX() + 18, ry, pw - 36, 42);
            const bool near = both.expanded (10).contains (mouse - display.getPosition()) || std::getenv ("PRESS_HOVER_RH") != nullptr;
            g.beginTransparencyLayer (near ? 1.0f : 0.14f);
            valueBar (g, R (s.getX() + 18, ry, bw, 42), "RANGE", "24 dB", 0.8f);
            valueBar (g, R (s.getX() + 18 + bw + 10, ry, bw, 42), "HOLD", "0 ms", 0.0f);
            g.endTransparencyLayer();
        }
    }
    paintTopStrip (g, std::getenv ("PRESS_TOPOFF") == nullptr);
    paintScDrawer (g, std::getenv ("PRESS_SC") != nullptr);
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
    auto panel = [&] (R s, bool right)
    {
        juce::Path p; p.addRoundedRectangle (s.getX(), s.getY(), s.getWidth(), s.getHeight(), 10, 10, right, ! right, right, ! right);
        cachedShadow (g, juce::DropShadow (juce::Colours::black.withAlpha (0.2f), 18, { right ? -6 : 6, 0 }), p);
        g.setColour (col ("#1d1d20", 0.55f)); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    };
    auto f11 = font (Face::Regular, 11), fv = font (Face::Regular, 10), fh = font (Face::CondMedium, 10, 1.6f);
    auto section = [&] (float x, float& y, float w, const char* name, juce::Colour c, bool on)
    {
        g.setColour (on ? c : col ("#3a3936")); g.fillEllipse (x, y + 3, 7, 7);
        text (g, name, fh, on ? ink : grey, R (x + 13, y, w - 30, 13));
        icon (g, Icon::Power, R (x + w - 14, y, 13, 13), on ? c : col ("#3a3936"), 1.2f);
        y += 22;
    };
    auto knob = [&] (float x, float& y, float w, const char* lab, const juce::String& val, float pos)
    {
        neonKnob (g, { x, y }, pos, 34.0f);
        text (g, lab, f11, soft, R (x + 44, y + 3, w - 44, 14));
        text (g, val, fv, col ("#b9b3a7"), R (x + 44, y + 18, w - 44, 13));
        y += 42;
    };
    const float pw = 166.0f, ph = 470.0f, top = display.getY() + (display.getHeight() - ph) * 0.5f - display.getY();
    // left: peak catcher, rider, breath
    {
        const R s (0, top, pw, ph);
        panel (s, false);
        const float x = 12, w = pw - 24;
        float y = s.getY() + 14;
        section (x, y, w, "1964 PEAK", peakC, true);
        knob (x, y, w, "Drive", "+3.0 dB", 0.35f);
        knob (x, y, w, "Output", U8 ("\xe2\x88\x92") + "3.0 dB  linked", 0.45f);
        y += 10;
        section (x, y, w, "RIDER", helperC, true);
        knob (x, y, w, "Range", U8 ("\xc2\xb1") + "6 dB", 0.5f);
        knob (x, y, w, "Target", U8 ("\xe2\x88\x92") + "16 dB", 0.6f);
        knob (x, y, w, "Speed", "Medium", 0.5f);
        y += 10;
        section (x, y, w, "BREATH", col ("#7fb8ff"), true);
        knob (x, y, w, "Amount", U8 ("\xe2\x88\x92") + "8 dB", 0.33f);
    }
    // right: Flow (Zerem's multiband styles or Perach's adaptive tone)
    {
        const R s (PW - pw, top, pw, ph);
        panel (s, true);
        const float x = s.getX() + 12, w = pw - 24;
        float y = s.getY() + 14;
        section (x, y, w, "FLOW", col ("#c08bff"), true);
        {
            const R seg (x, y, w, 20);
            fillRound (g, seg, 5, col ("#26262a"));
            for (int i = 0; i < 2; ++i)
            {
                const R c (x + (float) i * w * 0.5f, y, w * 0.5f, 20);
                if (i == 0) fillRound (g, c.reduced (1), 4, col ("#3a3a40"));
                text (g, i == 0 ? "Zerem" : "Perach", fv, i == 0 ? soft : col ("#8b8984"), c, juce::Justification::centred);
            }
            y += 28;
            const R dd (x, y, w, 22);
            fillRound (g, dd, 6, col ("#1d1e22")); strokeRound (g, dd, 6, col ("#34353b"));
            text (g, "Vocal Glue", f11, ink, dd.reduced (9, 0).withTrimmedRight (12));
            icon (g, Icon::TriDown, R (dd.getRight() - 16, dd.getCentreY() - 4, 8, 8), grey);
            y += 34;
        }
        knob (x, y, w, "Amount", "50%", 0.5f);
        knob (x, y, w, "Mix", "100%", 1.0f);
        // the five bands at work
        text (g, "BANDS", fh, grey, R (x, y + 2, w, 12));
        y += 18;
        const char* bn[5] { "LOW", "LO-MID", "MID", "PRES", "AIR" };
        const float red[5] { 1.5f, 3.2f, 2.0f, 4.1f, 1.0f };
        for (int i = 0; i < 5; ++i)
        {
            const R row (x, y, w, 14);
            text (g, bn[i], fv, col ("#b9b3a7"), R (x, y, 50, 14));
            fillRound (g, R (x + 54, y + 4, w - 54, 6), 3, col ("#2a2a30"));
            fillRound (g, R (x + 54, y + 4, (w - 54) * red[i] / 6.0f, 6), 3, col ("#c08bff"));
            y += 20;
        }
        y += 6;
        text (g, U8 ("Sheen  +1.0 dB \xc2\xb7 Delta"), fv, grey, R (x, y, w, 14));
    }
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
        const double tms = std::getenv ("PRESS_BREATH") ? std::atof (std::getenv ("PRESS_BREATH")) : juce::Time::getMillisecondCounterHiRes();
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
    text (g, "KNEE", font (Face::CondMedium, 10, 2.0f), col ("#b9b3a7"), R (a.getX(), a.getY() - 30, 80, 12));
    if (std::getenv ("PRESS_MLA") != nullptr)   // LE-MLA: the threshold is fixed - Input Gain drives into it - and so is the knee
        text (g, U8 ("drive \xe2\x88\x92" "8 dB  \xc2\xb7  2:1  \xc2\xb7  soft knee"), font (Face::Regular, 10.5f), ink, R (a.getRight() - 214, a.getY() - 31, 214, 13), juce::Justification::centredRight);
    else
    text (g, MINUS + juce::String (-mThr, 1) + U8 (" dB  \xc2\xb7  ") + juce::String (mRatio, 1).trimCharactersAtEnd (".0") + U8 (":1  \xc2\xb7  ") + (std::getenv ("PRESS_DS1") != nullptr ? juce::String ("soft knee 0.6") : mKnee < 0.5f ? juce::String ("hard knee") : "knee " + juce::String (mKnee, 0) + " dB"),
          font (Face::Regular, 10.5f), ink, R (a.getRight() - 214, a.getY() - 31, 214, 13), juce::Justification::centredRight);
    if (std::getenv ("PRESS_RVOX") != nullptr || std::getenv ("PRESS_MLA") != nullptr)
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
    if (! open) { paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 1, 44, 14), 1, {}); return; }
    const R s (x0, 0, x1 - x0, 42);
    drawGlass (g, s, [] (float u, float t) { return (0.3f + 0.7f * smoothStep ((1.0f - t) / 0.6f)) * smoothStep (u / 0.06f) * smoothStep ((1.0f - u) / 0.06f); });
    const float cy = 16.0f;
    // flip buttons show what they're set to (click to change); toggles light up
    auto flip = [&] (R r, const char* now, const char*) { glassButton (g, r, now, true, true); };
    auto toggle = [&] (R r, const char* t, bool on) { glassButton (g, r, t, on); };
    const float bw = 96, gap = 14;
    if (std::getenv ("PRESS_PRO") != nullptr)   // LE-PRO: Character (off / warm / punchy) with its drive, Peak / RMS, Delta
    {
        // Character: Tube / Diode / Bright, its Drive, and whether it colours before or after the compressor;
        // Auto Threshold keeps the compression the same when the level going in changes
        const float total = 140 + 96 + 76 + 130 + 76 + gap * 4;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flip (R (x, cy - 11, 140, 22), "CHARACTER  TUBE", "DIODE"); x += 140 + gap;
        toggle (R (x, cy - 11, 96, 22), "DRIVE  30%", true); x += 96 + gap;
        flip (R (x, cy - 11, 76, 22), "PRE", "POST"); x += 76 + gap;
        toggle (R (x, cy - 11, 130, 22), "AUTO THRESHOLD", false); x += 130 + gap;
        toggle (R (x, cy - 11, 76, 22), "DELTA", false);
        paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 31, 44, 14), 0, {});
        return;
    }
    if (std::getenv ("PRESS_MLA") != nullptr)   // LE-MLA: its meter switch (gain reduction or VU) and Delta
    {
        const float total = 120 + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flip (R (x, cy - 11, 120, 22), "METER  GR", "METER  VU"); x += 120 + gap;
        toggle (R (x, cy - 11, 76, 22), "DELTA", false);
        paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 31, 44, 14), 0, {});
        return;
    }
    if (std::getenv ("PRESS_DS1") != nullptr)   // LE-DS1: full range or band-selective (only the chosen band is compressed), and Delta
    {
        const float total = 120 + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flip (R (x, cy - 11, 120, 22), std::getenv ("PRESS_DS1_BAND") ? "BAND" : "FULL RANGE", "BAND"); x += 120 + gap;
        toggle (R (x, cy - 11, 76, 22), "DELTA", false);
        paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 31, 44, 14), 0, {});
        return;
    }
    if (std::getenv ("PRESS_RVOX") != nullptr)   // RVox's timing and tone are fixed: only Peak / RMS and Delta apply
    {
        const float total = bw + 76 + gap;
        float x = (x0 + x1) * 0.5f - total * 0.5f;
        flip (R (x, cy - 11, bw, 22), "PEAK", "RMS"); x += bw + gap;
        toggle (R (x, cy - 11, 76, 22), "DELTA", false);
        paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 31, 44, 14), 0, {});
        return;
    }
    const float total = bw * 3 + 104 + 76 + gap * 4;
    float x = (x0 + x1) * 0.5f - total * 0.5f;
    flip (R (x, cy - 11, bw, 22), "GLIDE", "SNAP"); x += bw + gap;
    flip (R (x, cy - 11, bw, 22), "COLOR", "PURE"); x += bw + gap;
    flip (R (x, cy - 11, bw, 22), "PEAK", "RMS"); x += bw + gap;
    toggle (R (x, cy - 11, 104, 22), "AUTO ATTACK", false); x += 104 + gap;
    toggle (R (x, cy - 11, 76, 22), "DELTA", false);
    paintArrowHandle (g, R ((x0 + x1) * 0.5f - 22, 31, 44, 14), 0, {});
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
void Canvas::paintScDrawer (juce::Graphics& g, bool open)
{
    const float x0 = 214.0f, x1 = PW - 214.0f;
    if (! open) return;   // (its handle is in the bottom bar, beside Look-ahead)
    const float edgeY = 498.0f;   // a short band along the bottom
    const R rowR0 (PW * 0.5f - 380.0f, 650.0f - 26.0f, 760, 50);
    const bool onControls = rowR0.expanded (10).contains (mouse - display.getPosition()) || std::getenv ("PRESS_HOVER_ROW") != nullptr;
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
    {
        const R strip (x0, edgeY - 7, x1 - x0, 14);
        juce::ColourGradient hg (juce::Colours::white.withAlpha (0.0f), strip.getX(), 0, juce::Colours::white.withAlpha (0.0f), strip.getRight(), 0, false);
        hg.addColour (0.12, juce::Colours::white.withAlpha (0.05f)); hg.addColour (0.88, juce::Colours::white.withAlpha (0.05f));
        g.setGradientFill (hg); g.fillRect (strip);
        juce::ColourGradient lg (juce::Colours::white.withAlpha (0.0f), strip.getX(), 0, juce::Colours::white.withAlpha (0.0f), strip.getRight(), 0, false);
        lg.addColour (0.12, cleanC.withAlpha (0.45f)); lg.addColour (0.5, juce::Colours::white.withAlpha (0.55f)); lg.addColour (0.88, cleanC.withAlpha (0.45f));
        g.setGradientFill (lg); g.fillRect (strip.getX(), strip.getCentreY() - 0.5f, strip.getWidth(), 1.0f);
    }
    // the controls, a row along the bottom centre
    {
        const float cy = 650.0f;   // nearly on the bottom bar
        auto flip = [&] (R r, const char* now) { glassButton (g, r, now, true, true); };
        auto toggle = [&] (R r, const char* t, bool on) { glassButton (g, r, t, on); };
        const R rowR (PW * 0.5f - 380.0f, cy - 26, 760, 50);
        const bool nearRow = rowR.expanded (10).contains (mouse - display.getPosition()) || std::getenv ("PRESS_HOVER_ROW") != nullptr;
        g.beginTransparencyLayer (nearRow ? 1.0f : 0.14f);
        float x = PW * 0.5f - 372.0f;
        const bool pro = std::getenv ("PRESS_PRO") != nullptr, sync = pro && std::getenv ("PRESS_PRO_SYNC") != nullptr;
        // LE-PRO's sources: Internal, External, MIDI, Host Sync (a pump in time with the song)
        flip (R (x, cy - 11, 100, 22), sync ? "HOST SYNC" : "INTERNAL"); x += 112;
        toggle (R (x, cy - 11, 80, 22), "LISTEN", false); x += 96;
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
            const float pos = 0.5f, hy = tr.getBottom() - tr.getHeight() * pos;
            fillRound (g, R (tr.getX(), hy, 4, tr.getBottom() - hy), 2, cleanC);
            fillRound (g, R (tr.getCentreX() - 9, hy - 4, 18, 8), 3, col ("#e9e5dd"));
            text (g, "SC GAIN", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (x + 20, cy - 12, 60, 11));
            text (g, "0.0 dB", font (Face::Regular, 11), ink, R (x + 20, cy + 1, 60, 13));
            x += 92;
        }
        // stereo link: a sideways slider - right is linked, left is unlinked
        {
            const R tr (x, cy + 4, sync ? 90.0f : 150.0f, 4);
            text (g, "LINK", font (Face::CondMedium, 9, 1.4f), col ("#b9b3a7"), R (x, cy - 16, 60, 11));
            text (g, sync ? "100%" : "100%  linked", font (Face::Regular, 10.5f), ink, R (x, cy - 17, tr.getWidth(), 13), juce::Justification::centredRight);
            fillRound (g, tr, 2, juce::Colours::white.withAlpha (0.1f));
            fillRound (g, tr, 2, cleanC);
            fillRound (g, R (tr.getRight() - 4, tr.getCentreY() - 8, 8, 16), 3, col ("#e9e5dd"));
            text (g, "unlinked", font (Face::Regular, 8.5f), grey, R (x, cy + 11, 60, 10));
            if (! sync) text (g, "linked", font (Face::Regular, 8.5f), grey, R (x + 90, cy + 11, 60, 10), juce::Justification::centredRight);
            x += tr.getWidth() + 20;
        }
        // mid / side and look-ahead: buttons whose values you drag
        valueBar (g, R (x, cy - 21, 96, 42), "M / S", "Stereo", 0.5f); x += 108;
        valueBar (g, R (x, cy - 21, 96, 42), "LOOK-AHEAD", pro ? juce::String (U8 ("2.0 ms \xc2\xb7 max 10")) : juce::String ("2.0 ms"), 0.2f);
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
    const bool band = std::getenv ("PRESS_DS1_BAND") != nullptr;
    const bool pro = std::getenv ("PRESS_PRO") != nullptr;
    auto respDb = [band, pro] (float hz)
    {
        if (pro)   // six bands: brickwall low and high cuts, four bells between
        {
            const float hp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (60.0f / hz, 24.0f)));
            const float lp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hz / 16000.0f, 24.0f)));
            auto bell = [hz] (float f, float db, float w) { const float o = std::log2 (hz / f); return db * std::exp (-o * o * w); };
            return hp + lp + bell (150.0f, -3.0f, 2.0f) + bell (450.0f, 2.0f, 2.5f) + bell (2000.0f, -2.5f, 2.0f) + bell (6000.0f, 4.0f, 1.6f);
        }
        if (band)   // a band-pass around 5 kHz, an octave wide: only this part of the sound is compressed
        {
            const float oct = std::log2 (hz / 5000.0f);
            return 8.0f * std::exp (-oct * oct * 2.2f) - 14.0f * (1.0f - std::exp (-oct * oct * 2.2f));
        }
        const float hp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (80.0f / hz, 4.0f)));
        const float lp = 10.0f * std::log10 (1.0f / (1.0f + std::pow (hz / 14000.0f, 4.0f)));
        const float oct = std::log2 (hz / 3000.0f);
        return hp + lp + 3.0f * std::exp (-oct * oct * 1.4f);
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
    const struct { float hz; const char* t; } nodes[3] { { 80.0f, "HP 80 Hz" }, { 3000.0f, "3 kHz  +3 dB" }, { 14000.0f, "LP 14 kHz" } };
    const struct { float hz; const char* t; } bandNode[1] { { 5000.0f, "BAND-PASS  5 kHz  \xc2\xb7  1 oct" } };
    if (band)
    {
        const float x = fxS (bandNode[0].hz), y = juce::jlimit (e.getY() + 10, e.getBottom() - 10, midY - respDb (bandNode[0].hz) * dbPx);
        g.setColour (cleanC.withAlpha (0.4f)); g.fillEllipse (x - 11, y - 11, 22, 22);
        g.setColour (cleanC.interpolatedWith (juce::Colours::white, 0.2f)); g.fillEllipse (x - 7.5f, y - 7.5f, 15, 15);
        text (g, U8 (bandNode[0].t), font (Face::Medium, 11), juce::Colours::white, R (x - 110, y - 26, 220, 13), juce::Justification::centred);
        text (g, U8 ("drag for frequency \xc2\xb7 scroll for width \xc2\xb7 click for LP / BP / HP"), font (Face::Regular, 9.5f), grey, R (x - 150, y - 42, 300, 12), juce::Justification::centred);
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
        text (g, nodes[i].t, font (Face::Medium, 11), juce::Colours::white, R (x - 50, y + 13, 100, 13), juce::Justification::centred);
    }
    text (g, U8 ("DETECTOR EQ  \xc2\xb7  what the compressor listens to"), font (Face::CondMedium, 9.5f, 1.6f), col ("#b9b3a7"), R (e.getX(), e.getY() - 22, 400, 12));
}

// the engine, in the arch at the top of the screen: its name, with arrows either side - slide (or click an arrow) to
// move between LE-RC and LE-DS1; two dots show which of the two it is
void Canvas::paintEngineSlide (juce::Graphics& g)
{
    const float cx = W * 0.5f, cy = 30.0f;   // up in the arch, above the strip that slides down
    const bool rv = std::getenv ("PRESS_RVOX") != nullptr;
    const bool ds = std::getenv ("PRESS_DS1") != nullptr;
    const bool ml = std::getenv ("PRESS_MLA") != nullptr;
    const bool pr = std::getenv ("PRESS_PRO") != nullptr;
    const juce::String nm = rv ? "LE-755C" : ds ? "LE-DS1" : ml ? "LE-MLA" : pr ? "LE-PRO" : "LE-RC", sub = "PROGRAM COMPRESSOR";
    text (g, nm, font (Face::Light, 15, 2.6f), ink, R (cx - 80, cy - 10, 160, 18), juce::Justification::centred);
    auto chev = [&] (float x, bool left)
    {
        juce::Path c;
        if (left) { c.startNewSubPath (x + 3, cy - 5); c.lineTo (x - 2, cy); c.lineTo (x + 3, cy + 5); }
        else      { c.startNewSubPath (x - 3, cy - 5); c.lineTo (x + 2, cy); c.lineTo (x - 3, cy + 5); }
        g.setColour (cleanC.withAlpha (0.8f)); g.strokePath (c, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    chev (cx - 44, true); chev (cx + 44, false);   // closer in
    for (int i = 0; i < 5; ++i)   // LE-RC, RVox, LE-DS1, LE-MLA, LE-PRO
    {
        const P d (cx - 16 + (float) i * 8, cy + 12);
        g.setColour (i == (rv ? 1 : ds ? 2 : ml ? 3 : pr ? 4 : 0) ? cleanC : juce::Colours::white.withAlpha (0.2f)); g.fillEllipse (d.x - 2, d.y - 2, 4, 4);
    }
    juce::ignoreUnused (sub);
}
