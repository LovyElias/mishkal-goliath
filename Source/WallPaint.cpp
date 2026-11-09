// Mishkal Wall - the drawing. A port of the approved mock (wall.html): same places, same sizes, same colours.
#include "WallDraw.h"

using namespace wallui;
using wall::dbToGain; using wall::gainToDb;

namespace
{
    constexpr float archHalf = 170.0f;
    juce::PathStrokeType stroke (float w, bool round = false) { return juce::PathStrokeType (w, juce::PathStrokeType::curved, round ? juce::PathStrokeType::rounded : juce::PathStrokeType::butt); }
    void dashH (juce::Graphics& g, float x0, float x1, float y, float on, float off, juce::Colour c, float lw)
    {
        const float d[2] { on, off }; g.setColour (c); g.drawDashedLine ({ x0, y, x1, y }, d, 2, lw);
    }
    void dashV (juce::Graphics& g, float x, float y0, float y1, float on, float off, juce::Colour c, float lw)
    {
        const float d[2] { on, off }; g.setColour (c); g.drawDashedLine ({ x, y0, x, y1 }, d, 2, lw);
    }
    juce::String choiceName (WallProcessor& p, const char* id)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.par (id))) return c->getCurrentChoiceName();
        return {};
    }
    const char* bandName (int b, int nb)
    {
        if (nb == 2) return b == 0 ? "LOW" : "HIGH";
        if (nb == 3) return b == 0 ? "LOW" : b == 1 ? "MID" : "HIGH";
        static const char* n4[] { "LOW", "LOW MID", "HIGH MID", "HIGH" };
        return n4[juce::jlimit (0, 3, b)];
    }
    juce::Colour bandColour (int b) { static const juce::uint32 c[] { 0xf3a93c, 0xf4efe4, 0xffb27a, 0xffd9a8 }; return hex (c[juce::jlimit (0, 3, b)]); }
}

// ============================================================================================================
// the screen's outline: it arches up into the header and down into the bottom bar
juce::Path WallCanvas::screenPath() const
{
    const float top = 50, bot = 764, ah = 36, ab = 34, s = 30, c = C;
    juce::Path p;
    p.startNewSubPath (14, top + 10); p.quadraticTo (14, top, 24, top);
    p.lineTo (c - archHalf - s, top); p.cubicTo (c - archHalf, top, c - archHalf, top - ah, c - archHalf + s, top - ah);
    p.lineTo (c + archHalf - s, top - ah); p.cubicTo (c + archHalf, top - ah, c + archHalf, top, c + archHalf + s, top);
    p.lineTo (W - 24, top); p.quadraticTo (W - 14, top, W - 14, top + 10);
    p.lineTo (W - 14, bot - 10); p.quadraticTo (W - 14, bot, W - 24, bot);
    p.lineTo (c + archHalf + s, bot); p.cubicTo (c + archHalf, bot, c + archHalf, bot + ab, c + archHalf - s, bot + ab);
    p.lineTo (c - archHalf + s, bot + ab); p.cubicTo (c - archHalf, bot + ab, c - archHalf, bot, c - archHalf - s, bot);
    p.lineTo (24, bot); p.quadraticTo (14, bot, 14, bot - 10); p.closeSubPath();
    return p;
}

// Pressure's knob (the same drawing as Tahor V18's)
void WallCanvas::knob (juce::Graphics& g, float cx, float cy, float d, float pos, juce::Colour c, bool bip)
{
    const float r = d / 2, a0 = -135.0f * juce::MathConstants<float>::pi / 180.0f, a1 = -a0, av = a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, pos);
    const float tk = juce::jlimit (0.6f, 1.0f, r / 24.0f);
    auto P = [&] (float rad, float t) { return juce::Point<float> (cx + rad * std::sin (t), cy - rad * std::cos (t)); };
    for (int i = 0; i <= 10; ++i)
    {
        const float t = a0 + (a1 - a0) * (float) i / 10.0f;
        g.setColour (juce::Colours::white.withAlpha (i % 5 == 0 ? 0.45f : 0.22f));
        g.drawLine ({ P (r + 3 * tk, t), P (r + (i % 5 == 0 ? 8.0f : 6.0f) * tk, t) }, 1.0f);
    }
    auto arc = [&] (float from, float to, float lw, juce::Colour col)
    {
        juce::Path p; p.addCentredArc (cx, cy, r - 1.5f, r - 1.5f, 0.0f, from, to, true);
        g.setColour (col); g.strokePath (p, stroke (lw, true));
    };
    arc (a0, a1, 3 * tk, juce::Colours::black.withAlpha (0.55f));
    if (bip) { if (std::abs (pos - 0.5f) > 0.01f) { const float s0 = std::min (0.0f, av), s1 = std::max (0.0f, av); arc (s0, s1, 7 * tk, c.withAlpha (0.25f)); arc (s0, s1, 2.6f * tk, c); } }
    else if (pos > 0.005f) { arc (a0, av, 7 * tk, c.withAlpha (0.25f)); arc (a0, av, 2.6f * tk, c); }
    const float br = r - 5 * tk;
    radial (g, cx, cy + 3 * tk, br + 6 * tk, juce::Colours::black.withAlpha (0.55f), juce::Colours::black.withAlpha (0.0f));   // its shadow
    g.setGradientFill (juce::ColourGradient (hex (0x3a3b41), cx - br, cy - br, hex (0x121316), cx + br, cy + br, false));
    g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 48; ++i) { const float t = (float) i / 48.0f * juce::MathConstants<float>::twoPi + av; g.drawLine ({ P (br - 2.5f * tk, t), P (br, t) }, 0.8f); }
    const float cr = br * 0.88f;
    g.setGradientFill (juce::ColourGradient (hex (0x2e2f35), 0, cy - cr, hex (0x18191d), 0, cy + cr, false));
    g.fillEllipse (cx - cr, cy - cr, cr * 2, cr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (cx - cr + 0.5f, cy - cr + 0.5f, cr * 2 - 1, cr * 2 - 1, 1.0f);
    g.setColour (hex (0xf4f1ea));
    juce::Path ln; ln.startNewSubPath (P (cr * 2 * 0.12f, av)); ln.lineTo (P (cr * 2 * 0.46f, av));
    g.strokePath (ln, stroke (2.2f * tk, true));
}

// a knob with its name and number. style 0: the side knobs (bigKnob), 1: a row in a stage panel (kRow)
void WallCanvas::paramKnob (juce::Graphics& g, const juce::String& pid, float cx, float y, float d, const juce::String& lab, const juce::String& val,
                            juce::Colour c, bool bip, int style, const char* bipL, const char* bipR, float valSize)
{
    const float pos = norm (pid);
    float ky, vy; Face vf; float vs;
    if (style == 0)
    {
        text (g, lab, cx, y + 6, Face::CondMedium, 10, label, 'c', 2);
        ky = y + 24 + d / 2; vy = y + 24 + d + 22; vf = Face::Light; vs = valSize > 0 ? valSize : (d > 90 ? 20.0f : 16.0f);
    }
    else
    {
        text (g, lab, cx, y, Face::CondMedium, 9, label, 'c', 1.6f);
        ky = y + 16 + d / 2; vy = y + 16 + d + 14; vf = Face::Regular; vs = valSize > 0 ? valSize : 11.0f;
    }
    const bool hot = (dragging && down.kind == Hit::Knob && down.pid == pid);
    knob (g, cx, ky, d, pos, hot ? c.brighter (0.25f) : c, bip);
    if (typeIn == nullptr || typeInPid != pid) text (g, val, cx, vy, vf, vs, ink, 'c', style == 0 ? 0.5f : 0.0f);
    if (bip && bipL != nullptr)
    {
        text (g, bipL, cx - d / 2 - 4, ky + d / 2 - 2 - (style == 0 ? 0.0f : 0.0f), Face::CondMedium, 7, faint, 'r', 1);
        text (g, bipR, cx + d / 2 + 4, ky + d / 2 - 2, Face::CondMedium, 7, faint, 'l', 1);
    }
    zone (R (cx - d / 2 - 4, ky - d / 2 - 4, d + 8, d + 8), Hit::Knob, pid, 0);
    zone (R (cx - std::max (34.0f, d / 2 + 8), vy - vs * 0.75f, std::max (68.0f, d + 16), vs * 1.5f), Hit::Knob, pid, 1);   // (its number: click to type)
}

// one tab that steps to the next choice each time you press it
void WallCanvas::cycleTab (juce::Graphics& g, float cx, float y, const juce::String& lab, float w, float sc, const juce::String& pid)
{
    const float k = sc, hh = 24 * k, ww = (w > 0 ? w : tw (lab, Face::CondMedium, 10, 1.8f) + 56) * k;
    const bool over = R (cx - ww / 2, y, ww, hh).transformedBy (zoneXf).contains (mouse);
    fillR (g, cx - ww / 2, y, ww, hh, hh / 2, juce::Colours::white.withAlpha (over ? 0.2f : 0.14f));
    strokeR (g, cx - ww / 2, y, ww, hh, hh / 2, juce::Colours::white.withAlpha (0.75f), 1.2f);
    text (g, "<", cx - ww / 2 + 12 * k, y + hh / 2 - 0.5f, Face::Regular, 12 * k, juce::Colours::white.withAlpha (0.85f), 'c');
    text (g, ">", cx + ww / 2 - 12 * k, y + hh / 2 - 0.5f, Face::Regular, 12 * k, juce::Colours::white.withAlpha (0.85f), 'c');
    text (g, lab, cx, y + hh / 2 + 0.5f, Face::CondSemiBold, 10.5f * k, juce::Colours::white, 'c', 1.8f * k);
    zone (R (cx - ww / 2, y, ww, hh), Hit::CycleTab, pid);
}

// Gold's light: the converter's curve with the signal riding it, warmer and wider the harder it's hit
void WallCanvas::goldLight (juce::Graphics& g, float cx, float ty, float tw2, float th, float d, bool bold)
{
    const float tx = cx - tw2 / 2, k2 = 1 + 2 * (0.15f + 0.85f * d) + 9 * d * d;
    auto cy2 = [&] (float x) { return juce::jlimit (-1.05f, 1.05f, std::tanh (k2 * x) / std::tanh (k2) + 0.1f * d * x * x); };
    auto at = [&] (float x) { return juce::Point<float> (tx + (x + 1) * 0.5f * tw2, ty - cy2 (x) * 0.5f * th); };
    juce::Path p; for (int i = 0; i <= 120; ++i) { const auto q = at (-1 + 2 * (float) i / 120); if (i) p.lineTo (q); else p.startNewSubPath (q); }
    juce::ColourGradient fd (juce::Colours::white.withAlpha (0.0f), tx, 0, juce::Colours::white.withAlpha (0.0f), tx + tw2, 0, false);
    fd.addColour (0.25, juce::Colours::white.withAlpha (0.5f)); fd.addColour (0.75, juce::Colours::white.withAlpha (0.5f));
    if (bold) { g.setColour (hex (0xffcf7a, 0.18f)); g.strokePath (p, stroke (8.0f, true)); }
    g.setGradientFill (fd); g.strokePath (p, stroke (bold ? 2.8f : 1.2f, true));
    const float ph = 0.45f;
    for (int i = 24; i >= 1; --i)
    {
        const float age = (float) i / 24.0f;
        const auto q = at (ph - (float) i * 0.012f);
        const float hue = std::fmod (std::fmod (0.11f - 0.11f * d + (float) i * (0.012f + 0.05f * d), 1.0f) + 1.0f, 1.0f);
        const auto cc = juce::Colour::fromHSL (hue, (35 + 60 * d) / 100.0f, 0.62f, (0.45f + 0.35f * d) * (1 - age));
        const float rr = (1 + (2.4f + 1.6f * d) * (1 - age)) * (bold ? 1.8f : 1.0f), sp = 4 + 12 * d;
        dot (g, q.x + (seed ((float) i * 3) - 0.5f) * sp * age, q.y + (seed ((float) i * 7) - 0.5f) * sp * age * 0.6f, rr * 0.5f + 0.5f, cc);
    }
    const auto p0 = at (ph);
    const std::pair<juce::uint32, float> rgb[] { { 0xff3040, 0.0f }, { 0x3dff9a, 2.094f }, { 0x4a7dff, 4.19f } };
    for (auto [cc, an] : rgb)
    {
        const float off = 1.2f + 5 * d, gr2 = 7 + 4 * d;
        radial (g, p0.x + off * std::cos (an + 1), p0.y + off * std::sin (an + 1), gr2, hex (cc, 0.45f * d), hex (cc, 0.0f));
    }
    const auto core = mixc (mixc (0xffd27a, 0xff8a2a, std::min (1.0f, d * 1.6f)), hex (0xff2d3d), std::max (0.0f, d * 1.6f - 0.6f));
    radial (g, p0.x, p0.y, th * (bold ? 0.85f : 0.6f), core.withAlpha (0.8f), core.withAlpha (0.0f));
    dot (g, p0.x, p0.y, bold ? 6.0f : 3.5f, mixc (core, juce::Colours::white, 0.3f));
    dot (g, p0.x, p0.y, bold ? 2.6f : 1.5f, juce::Colours::white);
}

// Gold's drive: a stepped slider (Off, 2, 3, 4, 6 dB), glowing brighter the further it goes
void WallCanvas::goldSlider (juce::Graphics& g, float cx, float cy, float w)
{
    static const char* steps[] { "Off", "2", "3", "4", "6 dB" };
    const int step = (int) pv ("goldDrive");
    const float x0 = cx - w / 2, t = (float) step / 4.0f;
    text (g, "DRIVE", x0 - 12, cy, Face::CondMedium, 9.5f, label, 'r', 1.6f);
    fillR (g, x0, cy - 1.5f, w, 3, 1.5f, juce::Colours::white.withAlpha (0.1f));
    const auto glow = mixc (0x7a5418, 0xffcf7a, t);
    if (t > 0) { fillR (g, x0 - 2, cy - 4, w * t + 4, 8, 4, glow.withAlpha (0.12f + 0.3f * t)); fillR (g, x0, cy - 1.5f, w * t, 3, 1.5f, glow); }
    for (int i = 0; i < 5; ++i)
    {
        const float x = x0 + w * (float) i / 4.0f;
        dot (g, x, cy, 2, i <= step ? glow : juce::Colours::white.withAlpha (0.25f));
        text (g, steps[i], x, cy + 14, Face::Regular, 8.5f, i == step ? amber : faint, 'c');
        zone (R (x - w / 8, cy - 10, w / 4, 32), Hit::GoldStep, "goldDrive", i);
    }
    const float kx = x0 + w * t;
    dot (g, kx, cy, 8, glow.withAlpha (0.25f)); dot (g, kx, cy, 4.5f, hex (0xf4ede0)); dot (g, kx, cy, 2, glow);
}

// ============================================================================================================
void WallCanvas::paint (juce::Graphics& g)
{
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    zones.clear(); zoneXf = {};
    // the background (frame, screen, grid): drawn once per size and reused
    const float want = juce::jlimit (1.0f, 4.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor());
    if (! bgCache.isValid() || std::abs (bgScale - want) > 0.01f)
    {
        bgScale = want;
        bgCache = juce::Image (juce::Image::RGB, juce::roundToInt (W * want), juce::roundToInt (H * want), true);
        juce::Graphics b (bgCache);
        b.addTransform (juce::AffineTransform::scale (want));
        b.fillAll (hex (0x0d0d10));
        juce::Graphics::ScopedSaveState s (b);
        b.reduceClipRegion (screenPath());
        juce::ColourGradient bg (hex (0x16131c), 0, 14, hex (0x0e0d12), 0, 800, false); bg.addColour (0.5, hex (0x110f16));
        b.setGradientFill (bg); b.fillRect (R (0, 0, W, H));
        juce::ColourGradient rg (juce::Colour (120, 70, 60).withAlpha (0.16f), C, 330, juce::Colours::transparentBlack, C + 640, 330, true);
        b.setGradientFill (rg); b.fillRect (R (0, 0, W, H));
        const float gy0 = 96, gy1 = 592;
        for (int i = 0; i <= 8; ++i) { b.setColour (juce::Colours::white.withAlpha (0.035f)); b.fillRect (14.0f, gy0 + (gy1 - gy0) * (float) i / 8, W - 28, 1.0f); }
        for (int i = 0; i <= 16; ++i) { b.setColour (juce::Colours::white.withAlpha (0.025f)); b.fillRect (14 + (W - 28) * (float) i / 16, 14.0f, 1.0f, 760.0f); }
    }
    g.drawImage (bgCache, R (0, 0, W, H));

    paintScreen (g);
    paintMeters (g);
    paintHeader (g);
    paintGrMeter (g);
    paintCrestMeter (g);
    paintSides (g);
    const bool main = view == 0 && stage < 0;
    if ((main || view == 2) && stage < 0) paintLoudStrip (g);
    paintBottom (g);
    if (main) paintMainLights (g);
    if (stage >= 0 && view == 0) paintStage (g);
    if (view == 1) { zoneXf = {}; paintBands (g); zoneXf = {}; }
    if (view == 2) paintLoudView (g);
    paintPreviewBanner (g);
    if (pv ("power") < 0.5f)
    {
        juce::Graphics::ScopedSaveState s (g); g.reduceClipRegion (screenPath());
        g.setColour (hex (0x0d0d10, 0.45f)); g.fillRect (R (0, 0, W, H));
        text (g, "BYPASSED", C, 300, Face::Light, 22, ink.withAlpha (0.8f), 'c', 6);
    }
    lastPaintMs = juce::Time::getMillisecondCounterHiRes() - t0;
}

// ============================================================================================================
// the screen: the scrolling wave (input smoke, output ivory, gain reduction hanging from the top), the ceiling, the rider
void WallCanvas::paintScreen (juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (screenPath());
    paintWave (g);
    // the "now" edge on the right, the left edge fading
    g.setGradientFill (juce::ColourGradient (hex (0x0d0d10, 0.0f), W - 90, 0, hex (0x0d0d10, 0.85f), W - 14, 0, false)); g.fillRect (W - 90, 56.0f, 76.0f, 560.0f);
    g.setGradientFill (juce::ColourGradient (hex (0x0d0d10, 0.9f), 14, 0, hex (0x0d0d10, 0.0f), 120, 0, false)); g.fillRect (14.0f, 56.0f, 106.0f, 560.0f);
    for (float x : { 14.0f, W - 216 })
    {
        const bool Lf = x < 100;
        juce::ColourGradient sg (hex (0x0e0d12, Lf ? 0.5f : 0.0f), x, 0, hex (0x0e0d12, Lf ? 0.0f : 0.5f), x + 202, 0, false);
        sg.addColour (Lf ? 0.82 : 0.18, hex (0x0e0d12, 0.28f));
        g.setGradientFill (sg); g.fillRect (x, 40.0f, 202.0f, 560.0f);
    }
    // the view switch, top left of the screen
    const float x = 48, y = 58;
    fillR (g, x, y, 186, 20, 10, hex (0x17181b, 0.9f)); strokeR (g, x, y, 186, 20, 10, juce::Colours::white.withAlpha (0.12f));
    static const char* vn[] { "WAVE", "BANDS", "LOUDNESS" };
    for (int i = 0; i < 3; ++i)
    {
        const float xx = x + 62.0f * (float) i;
        if (view == i) fillR (g, xx + 1, y + 1, 60, 18, 9, amber.withAlpha (0.22f));
        text (g, vn[i], xx + 31, y + 10.5f, Face::CondMedium, 9.5f, view == i ? amber : soft, 'c', 1.6f);
        zone (R (xx, y, 62, 20), Hit::View, {}, i);
    }
    {
        static const char* sp[] { "FAST", "SLOW", "INFINITE", "OFF" };
        const juce::String t = juce::String ("DISPLAY  ") + middot + "  " + sp[juce::jlimit (0, 3, proc.ui ("dispSpeed"))];
        const float w = tw (t, Face::CondMedium, 9, 1.4f) + 22;
        fillR (g, x + 194, y, w, 20, 10, hex (0x17181b, 0.9f)); strokeR (g, x + 194, y, w, 20, 10, juce::Colours::white.withAlpha (0.12f));
        text (g, t, x + 194 + w / 2, y + 10.5f, Face::CondMedium, 9, soft, 'c', 1.4f);
        zone (R (x + 194, y, w, 20), Hit::Display);
    }
}

void WallCanvas::paintWave (juce::Graphics& g)
{
    const float yc = 404, amp = 300, ceilDb = pv ("ceiling");
    const float ceil = 0.58f;
    const float range = (float) juce::jlimit (12, 48, proc.ui ("dispRange"));
    auto f = [&] (float lin) { return juce::jlimit (0.0f, 1.05f, ceil * (1.0f + (gainToDb (lin) - ceilDb) / range)); };
    auto xs = [] (int i) { return 14.0f + (W - 28.0f) * (float) i / (float) (cols - 1); };
    const int speed = proc.ui ("dispSpeed");   // V2: 0 fast, 1 slow, 2 infinite (the whole song), 3 off
    if (speed == 3) { text (g, juce::String ("WAVE OFF  ") + middot + "  right-click the screen to bring it back", C, 404, Face::CondMedium, 9.5f, faint, 'c', 1.6f); return; }
    std::array<float, cols> in {}, out {}, gr {}, lv {};
    auto merge = [] (wall::Frame& a, const wall::Frame& b) { a.inPk = std::max (a.inPk, b.inPk); a.outPk = std::max (a.outPk, b.outPk); a.grDb = std::max (a.grDb, b.grDb); a.levDb = b.levDb; };
    for (int i = 0; i < cols; ++i)
    {
        wall::Frame fr {};
        if (speed == 2)
        {
            const int n = (int) song.size();
            const int a = n * i / cols, b = std::max (a + 1, n * (i + 1) / cols);
            for (int k = a; k < b && k < n; ++k) merge (fr, song[(size_t) k]);
        }
        else
        {
            const int per = speed == 1 ? 4 : 1;
            for (int k = 0; k < per; ++k) merge (fr, wave[(size_t) ((wavePos - (cols - i) * per + k + rawN * 8) % rawN)]);
        }
        in[(size_t) i] = f (fr.inPk); out[(size_t) i] = std::min (f (fr.outPk), f (dbToGain (ceilDb)));
        gr[(size_t) i] = fr.grDb; lv[(size_t) i] = fr.levDb;
    }
    // (a touch of smoothing so the envelope reads like the mock's, not like noise)
    auto smooth = [] (std::array<float, cols>& a) { auto b = a; for (int i = 1; i < cols - 1; ++i) a[(size_t) i] = std::max (b[(size_t) i], (b[(size_t) i - 1] + b[(size_t) i] + b[(size_t) i + 1]) / 3.0f); };
    smooth (in); smooth (out);
    auto shape = [&] (const std::array<float, cols>& a)
    {
        juce::Path p; p.startNewSubPath (xs (0), yc - a[0] * amp);
        for (int i = 1; i < cols; ++i) p.lineTo (xs (i), yc - a[(size_t) i] * amp);
        for (int i = cols - 1; i >= 0; --i) p.lineTo (xs (i), yc + a[(size_t) i] * amp);
        p.closeSubPath(); return p;
    };
    {
        juce::ColourGradient wg (juce::Colour (170, 150, 130).withAlpha (0.16f), 0, yc - amp, juce::Colour (170, 150, 130).withAlpha (0.16f), 0, yc + amp, false);
        wg.addColour (0.5, juce::Colour (170, 150, 130).withAlpha (0.04f));
        g.setGradientFill (wg); g.fillPath (shape (in));
    }
    {
        juce::ColourGradient wg (juce::Colour (236, 224, 204).withAlpha (0.36f), 0, yc - amp, juce::Colour (236, 224, 204).withAlpha (0.36f), 0, yc + amp, false);
        wg.addColour (0.5, juce::Colour (236, 224, 204).withAlpha (0.07f));
        g.setGradientFill (wg); g.fillPath (shape (out));
        g.setColour (juce::Colour (246, 238, 224).withAlpha (0.7f));
        for (float sg : { -1.0f, 1.0f })
        {
            juce::Path p; p.startNewSubPath (xs (0), yc + sg * out[0] * amp);
            for (int i = 1; i < cols; ++i) p.lineTo (xs (i), yc + sg * out[(size_t) i] * amp);
            g.strokePath (p, stroke (1.0f));
        }
    }
    // gain reduction, hanging from the top of the screen
    {
        const float g0 = 72;
        auto grY = [&] (int i) { return g0 + juce::jlimit (0.0f, 300.0f, gr[(size_t) i] * 14.0f); };
        juce::Path p; p.startNewSubPath (xs (0), g0);
        for (int i = 0; i < cols; ++i) p.lineTo (xs (i), grY (i));
        p.lineTo (xs (cols - 1), g0); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (255, 90, 74).withAlpha (0.06f), 0, g0, juce::Colour (255, 90, 74).withAlpha (0.42f), 0, g0 + 90, false));
        g.fillPath (p);
        juce::Path l; l.startNewSubPath (xs (0), grY (0)); for (int i = 1; i < cols; ++i) l.lineTo (xs (i), grY (i));
        g.setColour (coral); g.strokePath (l, stroke (1.6f));
        g.setColour (juce::Colour (255, 122, 92).withAlpha (0.35f)); g.fillRect (14.0f, g0, W - 28, 1.0f);
        // V2: peak labels - the moments of most gain reduction, marked
        if (proc.ui ("peakLabels") != 0)
        {
            float mx = 0, mean = 0; for (auto v : gr) { mx = std::max (mx, v); mean += v / (float) cols; }
            const float thr = std::max ({ 1.0f, mean + 1.0f, mx * 0.85f });   // (only the moments that stand out)
            int shownN = 0;
            for (int i = 20; i < cols - 30 && shownN < 6; ++i)
            {
                const float v = gr[(size_t) i];
                if (v < thr) continue;
                bool top = true;
                for (int k = std::max (0, i - 30); k < std::min (cols, i + 30) && top; ++k) if (gr[(size_t) k] > v || (gr[(size_t) k] == v && k < i)) top = false;
                if (! top) continue;
                const float x = xs (i), y = grY (i) + 14;
                const juce::String t = minus + juce::String (v, 1);
                const float w = tw (t, Face::Medium, 8.5f) + 12;
                fillR (g, x - w / 2, y - 7, w, 14, 7, hex (0x0e0e10, 0.85f)); strokeR (g, x - w / 2, y - 7, w, 14, 7, coral.withAlpha (0.6f));
                text (g, t, x, y + 0.5f, Face::Medium, 8.5f, hex (0xff8a6e), 'c');
                ++shownN;
            }
        }
    }
    // the ceiling, both sides
    for (float sg : { -1.0f, 1.0f }) dashH (g, 14, W - 14, yc + sg * ceil * amp, 6, 5, amber.withAlpha (0.75f), 1.2f);
    // the leveler's ride through the middle: faint everywhere, alive in its own view
    {
        const bool LV = stage == wall::stLeveler && view == 0;
        auto py = [&] (int i) { return yc - juce::jlimit (-12.0f, 12.0f, lv[(size_t) i]) * 6.0f; };
        juce::Path p; p.startNewSubPath (xs (0), py (0)); for (int i = 2; i < cols; i += 2) p.lineTo (xs (i), py (i));
        if (pv ("levOn") > 0.5f)
        {
            if (LV) { g.setColour (orange.withAlpha (0.25f)); g.strokePath (p, stroke (7.0f, true)); }
            g.setColour (LV ? orange : orange.withAlpha (0.28f)); g.strokePath (p, stroke (LV ? 2.2f : 1.2f));
            if (LV)
            {
                g.setColour (orange.withAlpha (0.08f)); g.fillRect (14.0f, yc - 40, W - 28, 80.0f);
                g.setColour (orange.withAlpha (0.35f)); g.fillRect (14.0f, yc, W - 28, 1.0f);
                const int ii = juce::roundToInt ((W - 300 - 14) / (W - 28) * (float) (cols - 1));
                const float ex = W - 300, ey = py (ii);
                dot (g, ex, ey, 4, orange);
                fillR (g, ex - 52, ey - 32, 104, 18, 9, hex (0x0e0e10, 0.85f)); strokeR (g, ex - 52, ey - 32, 104, 18, 9, orange.withAlpha (0.7f));
                text (g, "RIDING  " + dbs (lv[(size_t) ii], 1, true) + " dB", ex, ey - 22.5f, Face::CondMedium, 9.5f, orange, 'c', 1.2f);
            }
        }
    }
    if (view == 0 && stage < 0)
    {
        const float y = yc - ceil * amp;
        fillR (g, W - 330, y - 22, 108, 17, 8.5f, hex (0x0e0e10, 0.8f)); strokeR (g, W - 330, y - 22, 108, 17, 8.5f, amber.withAlpha (0.6f));
        text (g, "CEILING  " + dbs (pv ("ceiling")) + (pv ("tp") > 0.5f ? " dBTP" : " dB"), W - 276, y - 13.5f, Face::CondMedium, 9.5f, amber, 'c', 1);
    }
}

// the meters at the window's edges: input left, output right
void WallCanvas::paintMeters (juce::Graphics& g)
{
    // V2: the scale - peak over 60 or 24 dB, or K-20 / K-14 / K-12 (RMS, 0 at -K dBFS, amber to +4, red above)
    const int ms = proc.ui ("meterScale");
    const int K = ms == 2 ? 20 : ms == 3 ? 14 : ms == 4 ? 12 : 0;
    const float lo = K > 0 ? -(float) K - 24.0f : (ms == 1 ? -24.0f : -60.0f);
    auto meter = [&] (float x, float db)
    {
        fillR (g, x, 70, 4, 670, 2, juce::Colours::white.withAlpha (0.05f));
        const float fr = juce::jlimit (0.0f, 1.0f, (db - lo) / -lo), h = 670 * fr;
        juce::ColourGradient mg (green, 0, 740, hex (0xef4444), 0, 70, false);
        if (K > 0) { const float k0 = (-(float) K - lo) / -lo, k4 = (-(float) K + 4 - lo) / -lo; mg.addColour (k0, green); mg.addColour (std::min (0.99, (double) k0 + 0.01), amber); mg.addColour (k4, amber); mg.addColour (std::min (0.995, (double) k4 + 0.01), hex (0xef4444)); }
        else { mg.addColour (0.7, green); mg.addColour (0.92, amber); }
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path p; p.addRoundedRectangle (x, 70.0f, 4.0f, 670.0f, 2.0f); g.reduceClipRegion (p);
            g.setGradientFill (mg); g.fillRect (x, 740 - h, 4.0f, h);
        }
        if (K > 0) { g.setColour (juce::Colours::white.withAlpha (0.7f)); g.fillRect (x - 1, 740 - 670 * (-(float) K - lo) / -lo, 6.0f, 1.0f); }
    };
    if (K > 0)
    {
        const float ri = gainToDb (proc.inRms.load()), ro = gainToDb (proc.outRms.load());
        meter (4, ri); meter (10, ri); meter (W - 16, ro); meter (W - 10, ro);
        text (g, "K-" + juce::String (K) + " " + dbs (std::max (ri + (float) K, -40.0f), 1, true), 4, 748, Face::Medium, 9, ink);
        text (g, "RMS", 4, 758, Face::Regular, 7.5f, grey);
        text (g, dbs (std::max (ro + (float) K, -40.0f), 1, true) + " K-" + juce::String (K), W - 4, 748, Face::Medium, 9, ink, 'r');
        text (g, "RMS", W - 4, 758, Face::Regular, 7.5f, grey, 'r');
        return;
    }
    meter (4, shown[0]); meter (10, shown[1]); meter (W - 16, shown[2]); meter (W - 10, shown[3]);
    text (g, "PK " + dbs (std::max (pkHold[0], lo)), 4, 748, Face::Medium, 9, ink);
    text (g, "AVG " + dbs (std::max (gainToDb (proc.inRms.load()), -60.0f)), 4, 758, Face::Regular, 7.5f, grey);
    text (g, dbs (std::max (pkHold[1], lo)) + " PK", W - 4, 748, Face::Medium, 9, ink, 'r');
    text (g, dbs (std::max (gainToDb (proc.outRms.load()), -60.0f)) + " AVG", W - 4, 758, Face::Regular, 7.5f, grey, 'r');
}

// the mastering GR meter, left (in every view): fine at the top where mastering lives
void WallCanvas::paintGrMeter (juce::Graphics& g)
{
    const float h = 360, x = 26, y0 = 407 - h / 2, bw = 5, gap = 3, mx = 12;
    fillR (g, x - 8, y0 - 30, 40, h + 70, 10, hex (0x0e0d12, 0.26f)); strokeR (g, x - 8, y0 - 30, 40, h + 70, 10, juce::Colours::white.withAlpha (0.10f));
    glowText (g, "GR", x + 6, y0 - 17, Face::CondSemiBold, 13, juce::Colours::white, coral, 'c', 2.4f);
    auto yOf = [&] (float db) { return y0 + h * std::pow (juce::jlimit (0.0f, mx, db) / mx, 0.75f); };
    for (float db : { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f })
    {
        const float y = yOf (db); const bool major = db == 0 || db == 1 || db == 2 || db == 3 || db == 6 || db == 12;
        g.setColour (juce::Colours::white.withAlpha (major ? 0.4f : 0.18f)); g.fillRect (x + 2 * bw + gap + 2, y, major ? 6.0f : 3.0f, 1.0f);
        if (major) text (g, db > 0 ? minus + juce::String ((int) db) : juce::String ("0"), x + 2 * bw + gap + 11, y + 0.5f, Face::Regular, 7.5f, grey);
    }
    // left: now; right: where it has been sitting (3 s)
    const float vals[2] { grShown, grAvg };
    for (int ch = 0; ch < 2; ++ch)
    {
        const float bx = x + (float) ch * (bw + gap);
        fillR (g, bx, y0, bw, h, 2, juce::Colours::white.withAlpha (0.05f));
        const int steps = (int) std::floor (yOf (vals[ch]) - y0);
        for (int yy = 0; yy < steps; yy += 3)
        {
            const float db = mx * std::pow ((float) yy / h, 1.0f / 0.75f);
            g.setColour ((db < 2 ? amber : db < 6 ? coral : hex (0xff3b30)).withMultipliedAlpha (ch == 1 ? 0.7f : 1.0f));
            g.fillRect (bx, y0 + (float) yy, bw, 2.0f);
        }
    }
    g.setColour (juce::Colours::white); g.fillRect (x - 1, yOf (grPeak), 2 * bw + gap + 2, 1.5f);   // peak hold
    glowText (g, (grShown > 0.05f ? minus : juce::String()) + juce::String (grShown, 1), x + 6, y0 + h + 21, Face::Medium, 15, hex (0xff8a6e), coral, 'c');
    text (g, "dB", x + 6, y0 + h + 33, Face::Regular, 7.5f, faint, 'c');
}

// the crest meter, right: how alive the peaks still are over the loudness (PLR)
void WallCanvas::paintCrestMeter (juce::Graphics& g)
{
    const float h = 360, x = W - 26 - 13, y0 = 407 - h / 2, bw = 13, mx = 20;
    fillR (g, x - 14, y0 - 30, 40, h + 70, 10, hex (0x0e0d12, 0.26f)); strokeR (g, x - 14, y0 - 30, 40, h + 70, 10, juce::Colours::white.withAlpha (0.10f));
    glowText (g, "CREST", x + 6, y0 - 17, Face::CondSemiBold, 11, juce::Colours::white, hex (0x6ee7a0), 'c', 1.8f);
    auto yOf = [&] (float db) { return y0 + h - h * juce::jlimit (0.0f, mx, db) / mx; };
    const std::tuple<float, float, juce::Colour> zonesC[] { { 0.0f, 6.0f, hex (0xff3b30) }, { 6.0f, 9.0f, amber }, { 9.0f, 20.0f, green } };
    for (auto& [a, b2, col] : zonesC) { g.setColour (col.withAlpha (0.10f)); g.fillRect (x, yOf (b2), bw, yOf (a) - yOf (b2)); }
    for (int db : { 0, 3, 6, 9, 12, 15, 20 }) { const float y = yOf ((float) db); g.setColour (juce::Colours::white.withAlpha (0.35f)); g.fillRect (x - 8, y, 5.0f, 1.0f); text (g, juce::String (db), x - 10, y + 0.5f, Face::Regular, 7.5f, grey, 'r'); }
    const bool live = proc.lufsS.load() > -60.0f;
    const float v = live ? crest : 0.0f;
    const auto col = v < 6 ? hex (0xff3b30) : v < 9 ? amber : green;
    if (live)
        for (float yy = yOf (v); yy < y0 + h; yy += 3)
        {
            const float db = (y0 + h - yy) / h * mx;
            g.setColour (db < 6 ? hex (0xff3b30) : db < 9 ? amber : green); g.fillRect (x, yy, bw, 2.0f);
        }
    if (live) { g.setColour (juce::Colours::white); g.fillRect (x - 1, yOf (crestAvg), bw + 2, 1.5f); }
    glowText (g, live ? juce::String (v, 1) : juce::String ("--"), x + 6, y0 + h + 21, Face::Medium, 15, col, col, 'c');
    text (g, "dB PLR", x + 6, y0 + h + 33, Face::Regular, 7.5f, faint, 'c');
}

// ============================================================================================================
void WallCanvas::paintHeader (juce::Graphics& g)
{
    text (g, "Wall", 22, 20, Face::Light, 19, ink, 'l', 3.4f);
    text (g, "LIMITER", 23, 37, Face::CondMedium, 7.5f, faint, 'l', 2.6f);
    // undo, redo, history
    const std::tuple<const char*, float, Hit, bool> ur[] { { "<", 96.0f, Hit::Undo, proc.canUndo() }, { ">", 118.0f, Hit::Redo, proc.canRedo() }, { "=", 140.0f, Hit::History, true } };
    for (auto& [t, x, k, en] : ur)
    {
        fillR (g, x - 9, 10, 20, 20, 6, hex (0x17181b)); strokeR (g, x - 9, 10, 20, 20, 6, hex (0x2a2b30));
        if (k == Hit::History) { g.setColour (soft); for (int i = 0; i < 3; ++i) g.fillRect (x - 4, 16.0f + (float) i * 3.5f, 10.0f, 1.2f); }
        else
        {
            juce::Path a; const float s = k == Hit::Undo ? -1.0f : 1.0f;
            a.addCentredArc (x + 1, 21, 4.5f, 4.0f, 0.0f, -2.4f * s, 0.9f * s, true);
            g.setColour (en ? soft : soft.withAlpha (0.35f)); g.strokePath (a, stroke (1.3f, true));
            const float ax = x + 1 + 4.5f * std::sin (-2.4f * s), ay = 21 - 4.0f * std::cos (-2.4f * s);
            juce::Path h; h.addTriangle (ax - 2.6f, ay - 1.0f, ax + 2.6f, ay - 1.0f, ax, ay + 2.6f);
            g.fillPath (h, juce::AffineTransform::rotation (s * 0.6f, ax, ay));
        }
        zone (R (x - 9, 10, 20, 20), k);
    }
    // the preset
    text (g, "<", 170, 20, Face::Regular, 13, grey, 'c'); zone (R (162, 10, 16, 20), Hit::PresetPrev);
    fillR (g, 182, 10, 124, 20, 10, hex (0x141518)); strokeR (g, 182, 10, 124, 20, 10, hex (0x34353b));
    text (g, juce::String::fromUTF8 ("\xe2\x98\x86"), 194, 20.5f, Face::Regular, 10, grey, 'c'); zone (R (184, 10, 20, 20), Hit::SavePreset);
    {
        juce::String n = proc.presetName + (proc.presetDirty ? " *" : "");
        float fs = 10; while (fs > 7 && tw (n, Face::Regular, fs) > 84) fs -= 0.5f;
        text (g, n, 243, 20.5f, Face::Regular, fs, ink, 'c');
    }
    { juce::Path tri; tri.addTriangle (291, 18.5f, 297, 18.5f, 294, 22.5f); g.setColour (grey); g.fillPath (tri); }
    zone (R (204, 10, 102, 20), Hit::Preset);
    text (g, ">", 316, 20, Face::Regular, 13, grey, 'c'); zone (R (308, 10, 16, 20), Hit::PresetNext);
    text (g, proc.abSlot == 0 ? "A > B" : "B > A", 332, 20.5f, Face::CondMedium, 10, label, 'l', 1.2f); zone (R (328, 10, 34, 20), Hit::AB);
    {
        const std::tuple<const char*, float, Hit, bool> bt[] { { "MATCH", 366.0f, Hit::Match, false }, { "BYPASS", 426.0f, Hit::Power, pv ("power") < 0.5f } };
        for (auto& [t, x, k, on] : bt)
        {
            const float w = tw (t, Face::CondMedium, 9.5f, 1) + 22;
            if (on) fillR (g, x, 11, w, 18, 6, amber.withAlpha (0.18f));
            strokeR (g, x, 11, w, 18, 6, on ? amber.withAlpha (0.7f) : hex (0x34353b));
            dot (g, x + 8, 20, 2.2f, on ? amber : label);
            text (g, t, x + 14, 20.5f, Face::CondMedium, 9.5f, on ? amber : label, 'l', 1);
            zone (R (x, 11, w, 18), k);
        }
    }
    // top centre: the limiter's style between amber arrows, a line under it, page dots
    {
        const int si = (int) pv ("limStyle");
        const auto& st = wall::styles()[(size_t) si];
        const juce::String name (st.name);
        text (g, name, C, 19, Face::Light, 17, ink, 'c', 4);
        const float sw = tw (name, Face::Light, 17, 4) / 2 + 18;
        text (g, "<", C - sw, 18, Face::Light, 17, amber, 'c'); text (g, ">", C + sw, 18, Face::Light, 17, amber, 'c');
        zone (R (C - sw - 12, 4, 24, 28), Hit::StylePrev); zone (R (C + sw - 12, 4, 24, 28), Hit::StyleNext);
        text (g, juce::String (st.blurb).toUpperCase().replace (", ", juce::String ("  ") + middot + "  ").replace (" & ", juce::String (" & ")), C, 37, Face::CondMedium, 8, hex (0xd8d1c4), 'c', 2.6f);
        for (int i = 0; i < 9; ++i)
        {
            const float x = C + (float) (i - 4) * 9;
            dot (g, x, 48, i == si ? 2.0f : 1.5f, i == si ? amber : juce::Colours::white.withAlpha (0.2f));
            zone (R (x - 4.5f, 43, 9, 10), Hit::StyleDot, {}, i);
        }
    }
    // right: preview, power, unity, logo
    {
        const float x = W - 22;
        if (logo.isValid()) g.drawImage (logo, R (W - 4 - 117, 4, 117, 36));
        g.setColour (hex (0x26272c)); g.fillRect (x - 112, 8.0f, 1.0f, 24.0f);
        const float ux = x - 205; const bool un = pv ("unity") > 0.5f;
        strokeR (g, ux, 11, 82, 18, 5, un ? hex (0xb07c2a) : hex (0x6b4f1f)); fillR (g, ux + 1, 12, 80, 16, 4, un ? hex (0x7a5a20, 0.75f) : hex (0x3a2c14, 0.6f));
        dot (g, ux + 9, 20, 2.5f, un ? amber : amber.withAlpha (0.35f));
        text (g, "UNITY GAIN", ux + 17, 20.5f, Face::CondMedium, 9.5f, un ? ink : label, 'l', 1);
        zone (R (ux, 11, 82, 18), Hit::Unity);
        const float px = ux - 112; const bool on = pv ("power") > 0.5f;
        fillR (g, px, 9, 100, 22, 6, ivory.withAlpha (on ? 0.12f : 0.03f)); strokeR (g, px, 9, 100, 22, 6, ivory.withAlpha (on ? 0.6f : 0.2f));
        {
            juce::Path a; a.addCentredArc (px + 15, 20, 4.5f, 4.5f, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
            g.setColour (on ? ivory : grey); g.strokePath (a, stroke (1.4f)); g.drawLine (px + 15, 14, px + 15, 19.5f, 1.4f);
        }
        text (g, on ? "WALL  ON" : "WALL  OFF", px + 28, 20.5f, Face::CondMedium, 10, on ? ink : grey, 'l', 1.3f);
        zone (R (px, 9, 100, 22), Hit::Power);
        // the streaming preview
        const int pvIdx = (int) pv ("preview"), where = (int) pv ("previewWhere");
        const bool PV = pvIdx > 0;
        static const char* plat[] { "SPOTIFY", "APPLE", "YOUTUBE", "TIDAL", "AMAZON" };
        static const char* lvl[] { "-14", "-16", "-14", "-14", "-14" };
        const int shownP = PV ? pvIdx - 1 : where;
        const float sx2 = px - 112, sw2 = 102;
        fillR (g, sx2, 9, sw2, 22, 11, PV ? hex (0x1ed760, 0.16f) : hex (0x17181b, 0.9f)); strokeR (g, sx2, 9, sw2, 22, 11, PV ? hex (0x1ed760, 0.8f) : hex (0x34353b));
        { juce::Path tri; tri.addTriangle (sx2 + 11, 15, sx2 + 18, 20, sx2 + 11, 25); g.setColour (PV ? spot : soft); g.fillPath (tri); }
        text (g, plat[shownP], sx2 + 24, 20.5f, Face::CondMedium, 9.5f, PV ? juce::Colours::white : label, 'l', 1.2f);
        text (g, juce::String (lvl[shownP]).replace ("-", minus), sx2 + sw2 - 24, 20.5f, Face::Regular, 8.5f, PV ? spot : faint, 'l');
        zone (R (sx2, 9, sw2, 22), Hit::Preview);
    }
}

// ============================================================================================================
// the side controls (Pressure's layout): input + learn + ceiling left; release, character, mix, lookahead, link right
void WallCanvas::paintSides (juce::Graphics& g)
{
    const float cxL = 110, cxR = W - 140;
    float yL = 96;
    paramKnob (g, "input", cxL, yL, 104, "INPUT", dbs (pv ("input"), 1, true) + " dB", amber, false, 0);
    yL += 24 + 104 + 12 + 22;
    {
        const float w = 124, x = cxL - w / 2, y = yL + 4;
        const bool L = proc.learning.load();
        const float pulse = L ? 0.5f + 0.5f * std::sin (animT * 6.0f) : 0.0f;
        fillR (g, x, y, w, 22, 11, hex (0x3a2c14, 0.55f + 0.3f * pulse)); strokeR (g, x, y, w, 22, 11, amber.withAlpha (0.7f + 0.3f * pulse));
        text (g, L ? juce::String ("LISTENING") + juce::String::fromUTF8 ("\xe2\x80\xa6") : "LEARN  " + middot + "  " + dbs (pv ("target"), 0) + " LUFS", cxL, y + 11.5f, Face::CondMedium, 9.5f, amber, 'c', 1.2f);
        zone (R (x, y, w, 22), Hit::Learn, "target");
        yL = y + 40;
    }
    paramKnob (g, "ceiling", cxL, yL, 80, "CEILING", dbs (pv ("ceiling")) + (pv ("tp") > 0.5f ? " dBTP" : " dB"), ivory, false, 0, nullptr, nullptr, 16);
    zones.back().kind = Hit::Knob;   // (the number: click to type; right-click the knob for the platforms)
    float yR = 96;
    {
        const float rel = pv ("limRel");
        const juce::String rv = (pv ("limArc") > 0.5f ? "ARC " + middot + " " : juce::String()) + (rel >= 100 ? juce::String (juce::roundToInt (rel)) : juce::String (rel, rel < 10 ? 1 : 0)) + " ms";
        paramKnob (g, "limRel", cxR, yR, 76, "RELEASE", rv, ivory, false, 0, nullptr, nullptr, 16);
    }
    yR += 24 + 76 + 12 + 22;
    {
        const float y = yR + 2;
        const bool arc = pv ("limArc") > 0.5f, tp = pv ("tp") > 0.5f;
        strokeR (g, cxR - 60, y, 56, 18, 9, arc ? amber.withAlpha (0.7f) : hex (0x34353b)); if (arc) fillR (g, cxR - 59, y + 1, 54, 16, 8, hex (0x3a2c14, 0.6f));
        text (g, "ARC", cxR - 32, y + 9.5f, Face::CondMedium, 9.5f, arc ? amber : label, 'c', 1.4f);
        strokeR (g, cxR + 4, y, 56, 18, 9, tp ? amber.withAlpha (0.7f) : hex (0x34353b)); if (tp) fillR (g, cxR + 5, y + 1, 54, 16, 8, hex (0x3a2c14, 0.6f));
        text (g, "TP", cxR + 32, y + 9.5f, Face::CondMedium, 9.5f, tp ? amber : label, 'c', 1.4f);
        zone (R (cxR - 60, y, 56, 18), Hit::Toggle, "limArc"); zone (R (cxR + 4, y, 56, 18), Hit::Toggle, "tp");
        yR = y + 34;
    }
    paramKnob (g, "chAmt", cxR - 44, yR, 56, "CHARACTER", juce::String (pv ("chAmt"), 1), pv ("chOn") > 0.5f ? amber : amber.withAlpha (0.4f), false, 0, nullptr, nullptr, 14);
    paramKnob (g, "mix", cxR + 44, yR, 56, "MIX", juce::String (juce::roundToInt (pv ("mix"))) + "%", ivory, false, 0, nullptr, nullptr, 14);
    yR += 24 + 56 + 12 + 22 + 10;
    paramKnob (g, "limLook", cxR - 44, yR, 44, "LOOKAHEAD", juce::String (pv ("limLook"), 1) + " ms", ivory, false, 0, nullptr, nullptr, 12);
    paramKnob (g, "limLinkT", cxR + 44, yR, 44, "LINK", "T " + juce::String (juce::roundToInt (pv ("limLinkT"))) + " " + middot + " R " + juce::String (juce::roundToInt (pv ("limLinkR"))), ivory, false, 0, nullptr, nullptr, 12);
}

// ============================================================================================================
// the loudness strip (Pro-L 2's meters, Mishkal glass)
void WallCanvas::paintLoudStrip (juce::Graphics& g)
{
    const float x = 268, w = W - 268 * 2, y = 622, h = 126;
    fillR (g, x, y, w, h, 12, hex (0x101014, 0.36f));   // V4: more glass
    {
        juce::Graphics::ScopedSaveState s (g); juce::Path p; p.addRoundedRectangle (x, y, w, h, 12.0f); g.reduceClipRegion (p);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), 0, y, juce::Colours::white.withAlpha (0.0f), 0, y + 26, false)); g.fillRect (x, y, w, 26.0f);
    }
    strokeR (g, x, y, w, h, 12, juce::Colours::white.withAlpha (0.10f));
    text (g, "LOUDNESS", x + 16, y + 16, Face::CondMedium, 9, faint, 'l', 2.4f);
    zone (R (x + 10, y + 6, 80, 20), Hit::LoudReset);
    {
        const float t = pv ("target");
        static const char* stdN[] { "", "Streaming", "Apple Music", "EBU R128", "ATSC A/85", "TR-B32", "Club" };
        const int ms = matchedStandard();
        const juce::String what = ms > 0 ? juce::String (stdN[ms]) : juce::String (t <= -14 ? "streaming" : t <= -11 ? "pop" : t <= -8 ? "club" : "loud");
        const juce::String tt = "target  " + dbs (t, t == std::round (t) ? 0 : 1) + " LUFS  " + middot + "  " + what;
        const float tw2 = tw (tt, Face::Regular, 9.5f) + 16;
        const bool over = R (x + w - 16 - tw2, y + 6, tw2 + 8, 20).contains (mouse);
        if (over) fillR (g, x + w - 20 - tw2, y + 6, tw2 + 12, 20, 10, juce::Colours::white.withAlpha (0.06f));
        text (g, tt + juce::String::fromUTF8 ("  \xe2\x96\xbe"), x + w - 16, y + 16, Face::Regular, 9.5f, over ? ink : grey, 'r');
        zone (R (x + w - 20 - tw2, y + 6, tw2 + 12, 20), Hit::LoudMenu);
    }
    const bool rel = proc.ui ("loudRel") != 0; const float tgt = pv ("target");
    auto lv = [&] (float v) { return v <= -69.5f ? juce::String (minus + juce::String::fromUTF8 ("\xe2\x88\x9e")) : rel ? dbs (v - tgt, 1, true) : dbs (v); };
    auto lvTp = [] (float v) { return v <= -69.5f ? juce::String (minus + juce::String::fromUTF8 ("\xe2\x88\x9e")) : dbs (v); };
    const char* lu = rel ? "LU" : "LUFS";
    const std::tuple<const char*, juce::String, const char*, juce::Colour> cells[] {
        { "INTEGRATED", lv (proc.lufsI.load()), lu, amber }, { "SHORT-TERM", lv (proc.lufsS.load()), lu, ink }, { "MOMENTARY", lv (proc.lufsM.load()), lu, ink },
        { "TRUE PEAK", lvTp (std::max (-70.0f, proc.tpMaxDb.load())), "dBTP", ink }, { "RANGE", juce::String (proc.lra.load(), 1), "LU", ink }, { "GAIN REDUCTION", (grShown > 0.05f ? minus : juce::String()) + juce::String (grShown, 1), "dB", coral } };
    const float cw = (w - 32) / 6.0f;
    int i = 0;
    for (auto& [l, v, u, col] : cells)
    {
        const float cx = x + 16 + cw * (float) i + cw / 2;
        text (g, l, cx, y + 44, Face::CondMedium, 8.5f, soft, 'c', 1.4f);
        const float vw = tw (v, Face::Light, 24), uw = tw (u, Face::Regular, 9), x0v = cx - (vw + 4 + uw) / 2;
        text (g, v, x0v, y + 70, Face::Light, 24, col); text (g, u, x0v + vw + 4, y + 74, Face::Regular, 9, grey);
        if (i) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x + 16 + cw * (float) i, y + 36, 1.0f, 56.0f); }
        ++i;
    }
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x + 16, y + 92, w - 32, 1.0f);
    // correlation (left), true-peak overs (right)
    const float ry = y + 110, bx = x + 120, bw2 = 300;
    text (g, "CORRELATION", x + 16, ry, Face::CondMedium, 8.5f, soft, 'l', 1.4f);
    fillR (g, bx, ry - 3, bw2, 6, 3, juce::Colours::white.withAlpha (0.06f));
    {
        juce::ColourGradient cg (hex (0xff3b30, 0.27f), bx, 0, hex (0x4ade80, 0.3f), bx + bw2, 0, false);
        cg.addColour (0.45, hex (0xf3a93c, 0.17f)); cg.addColour (0.6, hex (0x4ade80, 0.17f));
        g.setGradientFill (cg); g.fillRoundedRectangle (bx, ry - 3, bw2, 6, 3);
    }
    g.setColour (juce::Colours::white.withAlpha (0.4f)); g.fillRect (bx + bw2 / 2, ry - 6, 1.0f, 12.0f);
    text (g, minus + "1", bx - 6, ry + 0.5f, Face::Regular, 8, faint, 'r'); text (g, "+1", bx + bw2 + 6, ry + 0.5f, Face::Regular, 8, faint, 'l');
    const auto cc = corrShown < 0 ? hex (0xff6b5e) : corrShown < 0.3f ? amber : mint;
    fillR (g, bx + bw2 * (corrLo + 1) / 2, ry - 3, std::max (2.0f, bw2 * (corrHi - corrLo) / 2), 6, 3, cc.withAlpha (0.35f));
    const float mx2 = bx + bw2 * (corrShown + 1) / 2;
    radial (g, mx2, ry, 10, cc.withAlpha (0.5f), cc.withAlpha (0.0f));
    fillR (g, mx2 - 2, ry - 7, 4, 14, 2, cc);
    text (g, (corrShown >= 0 ? "+" : minus) + juce::String (std::abs (corrShown), 2), bx + bw2 + 28, ry + 0.5f, Face::Medium, 10, cc, 'l');
    const float ix = x + w - 16 - 196;
    const int isp = proc.ispCount.load();
    const auto ic = isp == 0 ? green : hex (0xff5a4a);
    text (g, "TRUE-PEAK OVERS", ix, ry, Face::CondMedium, 8.5f, soft, 'l', 1.4f);
    fillR (g, ix + 108, ry - 10, 88, 20, 10, ic.withAlpha (0.14f)); strokeR (g, ix + 108, ry - 10, 88, 20, 10, ic.withAlpha (0.7f));
    dot (g, ix + 120, ry, 3, ic);
    text (g, "ISP  " + juce::String (isp), ix + 156, ry + 0.5f, Face::Medium, 10, isp == 0 ? mint : hex (0xff8a7e), 'c');
    zone (R (ix + 108, ry - 10, 88, 20), Hit::LoudReset);
}

// ============================================================================================================
void WallCanvas::paintBottom (juce::Graphics& g)
{
    g.setColour (hex (0x0e0e10)); g.fillRect (0.0f, 764.0f, W, 36.0f);
    {
        juce::Graphics::ScopedSaveState s (g); g.reduceClipRegion (screenPath());
        g.setColour (hex (0x0e0d12)); g.fillRect (C - archHalf - 30, 764.0f, 2 * archHalf + 60, 40.0f);
    }
    const float cy = 782;
    text (g, "IN", 20, cy, Face::Regular, 10.5f, grey);
    strokeR (g, 38, cy - 9, 56, 18, 4, hex (0x34353b)); text (g, dbs (pv ("trim"), 1, true) + " dB", 66, cy + 0.5f, Face::Regular, 10, ink, 'c');
    zone (R (38, cy - 9, 56, 18), Hit::Knob, "trim", 1);
    const bool at = proc.autoTrim;
    strokeR (g, 100, cy - 9, 44, 18, 4, at ? amber : hex (0x34353b)); text (g, "AUTO", 122, cy + 0.5f, Face::Regular, 10, at ? amber : grey, 'c');
    zone (R (100, cy - 9, 44, 18), Hit::Auto);
    auto chip = [&] (float x, float w, const juce::String& l, const juce::String& v, bool on, Hit k, const char* pid)
    {
        fillR (g, x, cy - 8, w, 16, 4, on ? hex (0x4ade80, 0.12f) : hex (0x1b1c20)); strokeR (g, x, cy - 8, w, 16, 4, on ? hex (0x4ade80, 0.6f) : hex (0x34353b));
        if (v.isNotEmpty()) { text (g, l, x + 5, cy + 0.5f, Face::Regular, 7.5f, grey); text (g, v, x + w - 5, cy + 0.5f, Face::Medium, 7.5f, on ? mint : ink, 'r'); }
        else text (g, l, x + w / 2, cy + 0.5f, Face::Medium, 7.5f, on ? mint : ink, 'c');
        zone (R (x, cy - 8, w, 16), k, pid);
    };
    chip (150, 44, "OS", choiceName (proc, "os"), false, Hit::Choice, "os");
    chip (198, 50, "Dither", choiceName (proc, "dither") + ((int) pv ("dither") > 0 && (int) pv ("ditherShape") == 2 ? "+" : ""), false, Hit::DitherMenu, "dither");
    chip (252, 24, "DC", {}, pv ("dc") > 0.5f, Hit::Toggle, "dc");
    chip (280, 58, "MONO", "<" + juce::String (juce::roundToInt (pv ("monoHz"))), pv ("mono") > 0.5f, Hit::Toggle, "mono");
    zones.push_back ({ R (308, cy - 8, 30, 16), Hit::Knob, "monoHz", 1 });   // (its number: drag or type)
    paintChain (g);
    float x = W - 20;
    strokeR (g, x - 56, cy - 9, 56, 18, 4, hex (0x34353b)); text (g, dbs (pv ("out")) + " dB", x - 28, cy + 0.5f, Face::Regular, 10, ink, 'c');
    zone (R (x - 56, cy - 9, 56, 18), Hit::Knob, "out", 1);
    x -= 62; text (g, "OUT", x, cy, Face::Regular, 10.5f, grey, 'r'); x -= 34;
    const bool dl = pv ("delta") > 0.5f;
    strokeR (g, x - 46, cy - 9, 46, 18, 4, dl ? amber : hex (0x34353b)); text (g, "Delta", x - 23, cy + 0.5f, Face::Regular, 10, dl ? amber : grey, 'c');
    zone (R (x - 46, cy - 9, 46, 18), Hit::Delta);
    x -= 54;
    static const char* sizes[] { "Mini", "Small", "Medium", "Large", "Extra Large" };
    static const float sc[] { 0.7f, 0.85f, 1.0f, 1.2f, 1.4f };
    int si = 2; float best = 9; for (int i = 0; i < 5; ++i) if (std::abs (sc[i] - currentScale) < best) { best = std::abs (sc[i] - currentScale); si = i; }
    const juce::String sn = fullScreen ? juce::String ("Full") : juce::String (sizes[si]);
    text (g, sn, x - 4, cy, Face::Medium, 7.5f, ink, 'r');
    zone (R (x - 4 - tw (sn, Face::Medium, 7.5f) - 4, cy - 9, tw (sn, Face::Medium, 7.5f) + 8, 18), Hit::Size);
}

// the chain: the six stages in their order, then the limiter. Click a block: its controls. Its dot: on / off. Drag: move it.
void WallCanvas::paintChain (juce::Graphics& g)
{
    const float cy = 782, bw = 76, aw = 7, lw = tw ("CHAIN", Face::CondMedium, 9, 2) + 10, total = 7 * bw + 6 * aw;
    const float x0 = C - total / 2 + lw / 2 + 34;
    text (g, "CHAIN", x0 - lw, cy, Face::CondMedium, 9, faint, 'l', 2);
    const auto ord = proc.order();
    auto value = [&] (int st) -> juce::String
    {
        switch (st)
        {
            case wall::stPunch:     return pv ("pUpOn") > 0.5f && pv ("bandsOn") < 0.5f ? dbs (pv ("pUp"), 0, true) : juce::String (juce::roundToInt (pv ("pTrans"))) + "%";
            case wall::stCharacter: return {};
            case wall::stBands:     return pv ("bandsOn") > 0.5f ? juce::String ((int) pv ("bCount")) : juce::String ("off");
            case wall::stLeveler:   return dbs (levShown, 1, true);
            case wall::stGold:      return choiceName (proc, "goldDrive");
            case wall::stClipper:   return clipShown > 0.05f ? minus + juce::String (clipShown, 1) : juce::String ("0.0");
            default:                return (grShown > 0.05f ? minus : juce::String()) + juce::String (limShown, 1);
        }
    };
    const juce::Colour cols6[] { ink, amber, amber, orange, gold, orange, coral };
    auto lampOf = [&] (int st) -> float
    {
        switch (st)
        {
            case wall::stLeveler: return juce::jlimit (0.0f, 1.0f, std::abs (levShown) / std::max (1.0f, pv ("lRange")));
            case wall::stGold:    return goldShown;
            case wall::stClipper: return juce::jlimit (0.0f, 1.0f, clipShown / 3.0f);
            case 6:               return juce::jlimit (0.0f, 1.0f, limShown / 6.0f);
            default:              return -1.0f;
        }
    };
    for (int slot = 0; slot < 7; ++slot)
    {
        const int st = slot < 6 ? ord[(size_t) slot] : 6;
        float x = x0 + (float) slot * (bw + aw);
        if (chainDrag >= 0 && slot == chainDrag) x = juce::jlimit (x0, x0 + 5 * (bw + aw), chainDragX - bw / 2);
        const bool on = st == 6 ? pv ("limOn") > 0.5f : pv (wall::stageOnId (st)) > 0.5f;
        const bool sel = (st == wall::stBands && view == 1) || (stage == st && view == 0) || (st == 6 && stage == 6 && view == 0);
        const auto col = cols6[st];
        const float la = lampOf (st);
        fillR (g, x, cy - 10, bw, 20, 5, on ? col.withAlpha (sel ? 0.24f : 0.10f) : hex (0x17181b, 0.85f));
        strokeR (g, x, cy - 10, bw, 20, 5, on ? col.withAlpha (sel ? 0.9f : 0.45f) : juce::Colours::white.withAlpha (0.1f));
        if (sel) { g.setColour (col); g.fillRect (x + 8, cy + 8, bw - 16, 1.5f); }
        if (la >= 0 && on) lamp (g, x + 9, cy, la, false); else dot (g, x + 9, cy, 2, on ? mixc (col, juce::Colours::white, 0.35f) : juce::Colours::white.withAlpha (0.18f));
        const juce::String n = st == 6 ? juce::String ("LIMITER") : juce::String (wall::stageName (st));
        text (g, n, x + 16, cy + 0.5f, Face::CondMedium, 8.5f, on ? ink : grey, 'l', 1);
        text (g, value (st), x + bw - 6, cy + 0.5f, Face::Regular, 8.5f, on ? label : hex (0x5b5955), 'r');
        if (slot < 6 && chainDrag != slot) zone (R (x, cy - 10, bw, 20), Hit::ChainBlock, {}, slot);
        if (slot == 6) zone (R (x, cy - 10, bw, 20), Hit::ChainBlock, {}, 6);
        zone (R (x, cy - 10, 17, 20), Hit::ChainDot, {}, slot);
        if (slot < 6)
        {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            const float ax = x0 + (float) slot * (bw + aw) + bw + aw / 2;
            juce::Path a; a.startNewSubPath (ax - 3, cy); a.lineTo (ax + 2, cy); a.startNewSubPath (ax - 0.5f, cy - 2.5f); a.lineTo (ax + 2, cy); a.lineTo (ax - 0.5f, cy + 2.5f);
            g.strokePath (a, stroke (1.0f));
        }
    }
}

// the main view's lights: leveler, clipper, limiter, and Gold with its drive
void WallCanvas::paintMainLights (juce::Graphics& g)
{
    const float y = 552;
    const std::tuple<const char*, float, float, float, bool> ls[] {
        { "LEVELER", juce::jlimit (0.0f, 1.0f, std::abs (levShown) / std::max (1.0f, pv ("lRange"))), std::abs (levShown), C - 330, pv ("levOn") > 0.5f },
        { "CLIPPER", juce::jlimit (0.0f, 1.0f, clipShown / 3.0f), clipShown, C + 200, pv ("clipOn") > 0.5f },
        { "LIMITER", juce::jlimit (0.0f, 1.0f, limShown / 6.0f), limShown, C + 330, pv ("limOn") > 0.5f } };
    int k = 0;
    for (auto& [n, a2, v, x, on] : ls)
    {
        lamp (g, x - 30, y, on ? a2 : 0.0f, true);
        text (g, n, x - 12, y - 4, Face::CondMedium, 9.5f, on ? ink : grey, 'l', 1.6f);
        text (g, on ? juce::String (v, 1) + " dB" : juce::String ("off"), x - 12, y + 9, Face::Regular, 9, grey, 'l');
        static const int st[] { wall::stLeveler, wall::stClipper, 6 };
        zone (R (x - 44, y - 14, 96, 28), Hit::ChainBlock, "lamp", st[k++]);
    }
    const bool gOn = pv ("goldOn") > 0.5f;
    goldLight (g, C, 532, 280, 36, gOn ? goldShown : 0.0f, false);
    text (g, "GOLD", C, 560, Face::CondMedium, 10.5f, gOn ? gold : grey, 'c', 3);
    zone (R (C - 140, 506, 280, 64), Hit::ChainBlock, "lamp", wall::stGold);
    goldSlider (g, C, 590, 190);
}

// ============================================================================================================
// a stage's controls, over the wave
void WallCanvas::paintStage (juce::Graphics& g)
{
    switch (stage)
    {
        case wall::stPunch: paintPunchChar (g, true); break;
        case wall::stCharacter: paintPunchChar (g, false); break;
        case wall::stLeveler: paintLeveler (g); break;
        case wall::stGold: paintGold (g); break;
        case wall::stClipper: paintClipper (g); break;
        case 6: paintLimiter (g); break;
        default: break;
    }
}

void WallCanvas::paintPunchChar (juce::Graphics& g, bool isP)
{
    const float PX = C - 372, PW2 = 744, PY = 556, PH = 196;
    // V5: Punch 20% smaller (anchored at the bottom centre, like the Leveler)
    const float sc = isP ? 0.8f : 1.0f, ax = PX + PW2 / 2, ay = PY + PH;
    glassPanel (g, ax - (ax - PX) * sc, ay - (ay - PY) * sc, PW2 * sc, PH * sc, 0.0f);
    juce::Graphics::ScopedSaveState sst (g);
    zoneXf = juce::AffineTransform::translation (-ax, -ay).scaled (sc).translated (ax, ay);
    g.addTransform (zoneXf);
    const bool on = pv (isP ? "punchOn" : "chOn") > 0.5f;
    panelTitle (g, PX + 20, PY + 20, isP ? "PUNCH" : "CHARACTER", on ? (isP ? juce::String ("TRANSIENTS ") + middot + " UPWARD" : juce::String ("DENSITY ") + middot + " HARMONICS " + middot + " ENHANCE") : juce::String ("OFF ") + middot + " CLICK ITS DOT IN THE CHAIN", isP ? ivory : amber);
    float gx = PX + 20, gy = PY + 38, gw = 270, gh = 138;
    if (! isP) { gw = 243; gh = 124; gx = (PX + (C - 43)) / 2 - gw / 2; gy = PY + PH / 2 - gh / 2 + 8; }
    miniGraph (g, gx, gy, gw, gh, isP ? "ONE HIT" : "HARMONICS ADDED");
    if (isP)
    {
        // it breathes: slowly brighter and back (about every 4 s)
        const float br = 0.5f - 0.5f * std::cos (animT * juce::MathConstants<float>::twoPi / 4.0f);
        {
            juce::Graphics::ScopedSaveState s (g); juce::Path cp; cp.addRoundedRectangle (gx, gy, gw, gh, 10.0f); g.reduceClipRegion (cp);
            radial (g, gx + 60, gy + 40, 200, hex (0xfff1d6, 0.16f * br), hex (0xfff1d6, 0.0f));
        }
        const float tr = pv ("pTrans") * 0.01f, up = pv ("pUpOn") > 0.5f && pv ("bandsOn") < 0.5f ? pv ("pUp") : 0.0f;
        auto hit = [] (float t, float a, float s2, float u) { return t < 0 ? u : std::min (1.0f, std::exp (-t * a) * (1 + s2) + u); };
        const std::tuple<juce::Colour, float, float, float> curves[] { { juce::Colour (170, 150, 130).withAlpha (0.55f), 0.0f, 0.05f, 1.0f },
                                                                       { ivory, 0.35f * tr / 0.4f, 0.05f + up * 0.055f, 1.6f } };
        for (auto& [col, s2, u, lw] : curves)
        {
            juce::Path p;
            for (int i = 0; i <= 160; ++i)
            {
                const float t = (float) i / 160 * 6 - 0.6f, v = hit (t, 1.4f, s2, u);
                const float x = gx + 12 + (gw - 24) * (float) i / 160, yy = gy + gh - 12 - v * (gh - 34) * 0.85f;
                if (i) p.lineTo (x, yy); else p.startNewSubPath (x, yy);
            }
            if (lw > 1) { g.setColour (hex (0xfff1d6, 0.25f * br)); g.strokePath (p, stroke (lw + 6 * br, true)); g.setColour (mixc (hex (0xe9e0d0), juce::Colours::white, br)); g.strokePath (p, stroke (lw + 1.2f * br)); }
            else { g.setColour (col); g.strokePath (p, stroke (lw)); }
        }
        text (g, "attack " + dbs (tr * 4.5f, 1, true) + " dB", gx + 70, gy + 30, Face::Regular, 9, amber);
        text (g, "tail " + dbs (up, 1, true) + " dB", gx + gw - 14, gy + gh - 30, Face::Regular, 9, amber, 'r');
    }
    else
    {
        // the real harmonics of the curve as it's set now (a sine through it, measured)
        float hv[7] {};
        {
            const float amt = pv ("chAmt"), eo = pv ("chEO") * 0.01f; const int flav = (int) pv ("chFlav");
            const float drive = 0.15f + amt * 0.22f, bias = eo < 0 ? -eo * 0.22f : 0.0f, oddMul = 1.0f + std::max (0.0f, eo) * 0.8f;
            const int N = 256; double re[8] {}, im[8] {};
            for (int i = 0; i < N; ++i)
            {
                const double ph = 2.0 * juce::MathConstants<double>::pi * i / N;
                const float x = 0.7f * (float) std::sin (ph);
                const float y = wall::Character::shape (flav, (x + bias) * drive * oddMul) - wall::Character::shape (flav, bias * drive * oddMul);
                for (int k = 1; k <= 7; ++k) { re[k] += y * std::cos (k * ph); im[k] += y * std::sin (k * ph); }
            }
            const double f1 = std::sqrt (re[1] * re[1] + im[1] * im[1]) + 1e-12;
            for (int k = 1; k <= 7; ++k)
            {
                const double rel = std::sqrt (re[k] * re[k] + im[k] * im[k]) / f1;
                hv[k - 1] = k == 1 ? 1.0f : (float) juce::jlimit (0.0, 0.6, (60.0 + 20.0 * std::log10 (rel + 1e-9)) / 60.0 * 0.6);
            }
        }
        const int n = 7; const float bw = 22, step = (gw - 36) / n;
        const float ph = std::fmod (animT, 3.0f) / 3.0f * (n + 3) - 1.5f;   // a light sweeps across every 3 s, like a crowd doing the wave
        {
            juce::Graphics::ScopedSaveState s (g); juce::Path cp; cp.addRoundedRectangle (gx, gy, gw, gh, 10.0f); g.reduceClipRegion (cp);
            radial (g, gx + 18 + ph * step + bw / 2, gy + gh - 30, 60, hex (0xffd696, 0.22f), hex (0xffd696, 0.0f));
        }
        for (int i = 0; i < n; ++i)
        {
            const float lift = std::exp (-std::pow ((float) i - ph, 2.0f) / 1.1f);
            const float x = gx + 18 + (float) i * step, hgt = (gh - 44) * hv[i] + 14 * lift;
            const auto col = i == 0 ? hex (0xd8d1c4) : (i % 2 ? amber : hex (0xffb27a));
            const auto c2 = mixc (col, juce::Colours::white, 0.45f * lift).withAlpha (i == 0 ? 0.45f + 0.4f * lift : 0.85f + 0.15f * lift);
            if (lift > 0.05f) fillR (g, x - 3, gy + gh - 18 - hgt - 3, bw + 6, hgt + 6, 5, mixc (col, juce::Colours::white, 0.4f).withAlpha (0.2f * lift));
            fillR (g, x, gy + gh - 18 - hgt, bw, hgt, 3, c2);
            text (g, i == 0 ? juce::String ("1st") : juce::String (i + 1), x + bw / 2, gy + gh - 8, Face::Regular, 8, lift > 0.4f ? hex (0xd8d1c4) : faint, 'c');
        }
    }
    const float kc = PX + PW2 - 215;
    if (isP)
    {
        const float tr = pv ("pTrans");
        static const char* spd[] { "Slow", "Medium", "Fast" };
        const bool upOn = pv ("pUpOn") > 0.5f && pv ("bandsOn") < 0.5f;
        const float x0 = kc - 200, y = PY + 30;
        paramKnob (g, "pTrans", x0 + 50, y, 50, "TRANSIENTS", dbs (tr, 0, true) + "%", amber, true, 1, "SOFT", "HARD");
        paramKnob (g, "pUp", x0 + 150, y, 50, "UPWARD", pv ("bandsOn") > 0.5f ? juce::String ("in BANDS") : upOn ? dbs (pv ("pUp"), 1, true) + " dB" : juce::String ("off"), upOn ? ivory : ivory.withAlpha (0.35f), false, 1);
        zone (R (x0 + 110, y - 8, 80, 16), Hit::Toggle, "pUpOn");   // (click its name: upward on / off)
        paramKnob (g, "pThr", x0 + 250, y, 50, "THRESHOLD", dbs (pv ("pThr"), 0) + " dB", ivory, false, 1);
        paramKnob (g, "pSpeed", x0 + 350, y, 50, "SPEED", spd[juce::jlimit (0, 2, (int) pv ("pSpeed"))], ivory, false, 1);
        cycleTab (g, kc, PY + 156, choiceName (proc, "pMode").toUpperCase(), 130, 1.0f, "pMode");
    }
    else
    {
        const float eo = pv ("chEO");
        const juce::String eoS = std::abs (eo) < 8 ? juce::String ("balanced") : eo < 0 ? (eo < -55 ? juce::String ("warmer") : juce::String ("warm")) : (eo > 55 ? juce::String ("edgier") : juce::String ("edge"));
        const float x0 = kc - 200, y = PY + 30;
        paramKnob (g, "chAmt", x0 + 50, y, 50, "CHARACTER", juce::String (pv ("chAmt"), 1), amber, false, 1);
        paramKnob (g, "chDens", x0 + 150, y, 50, "DENSITY", juce::String (juce::roundToInt (pv ("chDens"))) + "%", amber, false, 1);
        paramKnob (g, "chEO", x0 + 250, y, 50, juce::String ("EVEN ") + middot + " ODD", eoS, amber, true, 1, "EVEN", "ODD");
        paramKnob (g, "chEnh", x0 + 350, y, 50, "ENHANCE", dbs (pv ("chEnh"), 1, true), ivory, false, 1);
        cycleTab (g, kc, PY + 156, choiceName (proc, "chFlav").toUpperCase(), 130, 1.0f, "chFlav");
    }
    zoneXf = {};
}

void WallCanvas::paintLeveler (juce::Graphics& g)
{
    const float PX = C - 372, PW2 = 744, sc = 0.8f, bx = PX, by = 566, bwid = PW2, bh = 180;
    const float ax = bx + bwid / 2, ay = by + bh;
    glassPanel (g, ax - (ax - bx) * sc, ay - (ay - by) * sc, bwid * sc, bh * sc, 0.75f);
    juce::Graphics::ScopedSaveState s (g);
    zoneXf = juce::AffineTransform::translation (-ax, -ay).scaled (sc).translated (ax, ay);
    g.addTransform (zoneXf);
    const float PY = 566;
    const float amt = juce::jlimit (0.0f, 1.0f, std::abs (levShown) / std::max (1.0f, pv ("lRange")));
    panelTitle (g, PX + 20, PY + 22, "LEVELER", juce::String ("RIDES THE LEVEL ") + middot + " THE ORANGE LINE ON THE WAVE", orange, pv ("levOn") > 0.5f ? amt : 0.0f);
    static const char* spd[] { "Slow", "Medium", "Fast" };
    const float y = PY + 46, kw = 150, x0 = C - kw * 2;
    paramKnob (g, "lTarget", x0 + kw * 0.5f, y, 62, "TARGET", dbs (pv ("lTarget"), 1) + " dB", orange, false, 1);
    paramKnob (g, "lRange", x0 + kw * 1.5f, y, 62, "RANGE", juce::String::fromUTF8 ("\xc2\xb1") + juce::String (pv ("lRange"), 1) + " dB", orange, false, 1);
    paramKnob (g, "lSpeed", x0 + kw * 2.5f, y, 62, "SPEED", spd[juce::jlimit (0, 2, (int) pv ("lSpeed"))], ivory, false, 1);
    paramKnob (g, "lFreeze", x0 + kw * 3.5f, y, 62, "FREEZE", dbs (pv ("lFreeze"), 0) + " dB", ivory, false, 1);
    zoneXf = {};
}

void WallCanvas::paintGold (juce::Graphics& g)
{
    const float gx = C - 250, gy = 540, gw = 500, gh = 190;
    glassPanel (g, gx, gy, gw, gh, 0.75f);
    const bool on = pv ("goldOn") > 0.5f;
    goldLight (g, C - 20, 610, 320, 56, on ? goldShown : 0.0f, true);
    text (g, "GOLD", C - 20, 658, Face::CondMedium, 13, on ? gold : grey, 'c', 4);
    goldSlider (g, C - 20, 698, 240);
    // its clip light: green when calm, orange then red as Gold is pushed into clipping
    greenLamp (g, gx + gw - 36, gy + gh / 2, on ? goldShown : 0.0f);
    text (g, "CLIP", gx + gw - 36, gy + gh / 2 + 26, Face::CondMedium, 8, grey, 'c', 1.6f);
}

void WallCanvas::paintClipper (juce::Graphics& g)
{
    const float PX = C - 372, PW2 = 744, sc = 0.7f, by = 488, bh = 262;
    const float ax = PX + PW2 / 2, ay = by + bh;
    glassPanel (g, ax - (ax - PX) * sc, ay - (ay - by) * sc, PW2 * sc, bh * sc, 0.0f);
    juce::Graphics::ScopedSaveState s (g);
    zoneXf = juce::AffineTransform::translation (-ax, -ay).scaled (sc).translated (ax, ay);
    g.addTransform (zoneXf);
    const float PY = 488;
    const bool on = pv ("clipOn") > 0.5f;
    const float HIT = on ? juce::jlimit (0.0f, 1.0f, clipShown / 3.0f) : 0.0f;
    panelTitle (g, PX + 20, PY + 22, "CLIPPER", on ? "SHAVES THE FASTEST PEAKS" : juce::String ("OFF ") + middot + " CLICK ITS DOT IN THE CHAIN", orange, HIT);
    const float gw = 200, gh = 104, gy = PY + 40, gap = 20, gx0 = C - (gw * 3 + gap * 2) / 2;
    {
        const float gx = gx0; miniGraph (g, gx, gy, gw, gh, "SHAVING NOW");
        lamp (g, gx + 52, gy + gh / 2 + 6, HIT, true);
        text (g, (clipShown > 0.05f ? minus : juce::String()) + juce::String (on ? clipShown : 0.0f, 1) + " dB", gx + 130, gy + gh / 2 + 6, Face::Light, 22, ink, 'c');
    }
    auto& cd = proc.clipperDsp();
    const int curve = (int) pv ("cCurve"); const float knee = pv ("cKnee") * 0.01f, amt = pv ("cAmt");
    const float floorG = 1.0f / dbToGain (std::max (0.0f, amt)), xr = 1.15f * dbToGain (std::max (0.5f, amt));
    auto cf = [&] (float x) { const float a = x * xr; float y = wall::Clipper::clip (a, curve, knee); if (std::abs (y) < std::abs (a) * floorG) y = a * floorG; return juce::jlimit (-1.2f, 1.2f, y) * 0.82f; };
    const int sp = cd.scopePos.load();
    {
        const float gx = gx0 + gw + gap; miniGraph (g, gx, gy, gw, gh, "CURVE");
        const float cx2 = gx + gw / 2, cy3 = gy + gh / 2 + 6, hw = 60, hh = 36;
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawLine (cx2 - hw, cy3 + hh, cx2 + hw, cy3 - hh, 1.0f);
        // cold > warm > red the harder it's hit
        const auto hc = HIT < 0.35f ? mixc (0x5ab4ff, 0x9fd8ff, HIT / 0.35f) : (HIT < 0.7f ? mixc (0xffcf7a, 0xff8a2a, (HIT - 0.35f) / 0.35f) : mixc (0xff8a2a, 0xff2d2d, (HIT - 0.7f) / 0.3f));
        {
            juce::Graphics::ScopedSaveState s2 (g); juce::Path cp; cp.addRoundedRectangle (gx, gy, gw, gh, 10.0f); g.reduceClipRegion (cp);
            radial (g, cx2 + hw * 0.7f, cy3 - hh * 0.7f, 40 + 40 * HIT, hc.withAlpha (0.22f + 0.3f * HIT), hc.withAlpha (0.0f));
        }
        juce::Path cpth; for (int i = 0; i <= 100; ++i) { const float x = (float) i / 100 * 2 - 1, y = cf (x); if (i) cpth.lineTo (cx2 + x * hw, cy3 - y * hh); else cpth.startNewSubPath (cx2 + x * hw, cy3 - y * hh); }
        g.setColour (hc.withAlpha (0.25f)); g.strokePath (cpth, stroke (6 + 10 * HIT, true));
        g.setColour (hc); g.strokePath (cpth, stroke (2.2f + 1.4f * HIT, true));
        g.setColour (mixc (hc, juce::Colours::white, 0.45f)); g.strokePath (cpth, stroke (1.0f));
        // phosphor: the last moment's samples, each a fading dot on the curve - dense where the music lives
        for (int i = 0; i < 260; ++i)
        {
            const int k = ((sp - 1 - i) % wall::Clipper::scopeN + wall::Clipper::scopeN) % wall::Clipper::scopeN;
            const float a = cd.scope[(size_t) (k * 2)].load() / xr;
            if (std::abs (a) > 1.0f) continue;
            const float age = (float) i / 260.0f;
            dot (g, cx2 + a * hw, cy3 - cf (a) * hh, 0.9f + 1.1f * (1 - age), mixc (hc, juce::Colours::white, 0.3f).withAlpha ((0.05f + 0.28f * (1 - age)) * (0.6f + 0.4f * HIT)));
        }
        // the comet: the signal now, riding the curve with a tail
        for (int j = 18; j >= 0; --j)
        {
            const int k = ((sp - 1 - j * 2) % wall::Clipper::scopeN + wall::Clipper::scopeN) % wall::Clipper::scopeN;
            const float a = juce::jlimit (-1.0f, 1.0f, std::abs (cd.scope[(size_t) (k * 2)].load()) / xr);
            dot (g, cx2 + a * hw, cy3 - cf (a) * hh, j ? (3.2f * (1 - (float) j / 18) + 0.6f) : 3 + 2 * HIT, j ? hc.withAlpha (0.5f * (1 - (float) j / 18)) : juce::Colours::white);
        }
        // sparks where the curve bends over (it's shaving): more and hotter the harder it's hit
        const float kneeX = std::min (0.95f, 1.0f / xr * (1.0f - knee * 0.5f)), kx = cx2 + kneeX * hw, ky = cy3 - cf (kneeX) * hh;
        const int ns = on && clipShown > 0.05f ? juce::roundToInt (6 + 22 * HIT) : 0;
        const float tt = std::floor (animT * 12.0f);
        for (int i = 0; i < ns; ++i)
        {
            const float an = -juce::MathConstants<float>::halfPi + (seed ((float) i * 2.3f + tt) - 0.5f) * 2.4f, len = 6 + 26 * seed ((float) i * 4.1f + tt) * (0.5f + HIT);
            const float x1 = kx + (seed ((float) i * 6.7f + tt) - 0.3f) * 30, y1 = ky + (seed ((float) i * 8.9f + tt) - 0.5f) * 8;
            const float x2 = x1 + std::cos (an) * len, y2 = y1 + std::sin (an) * len;
            g.setGradientFill (juce::ColourGradient (mixc (hc, juce::Colours::white, 0.5f).withAlpha (0.9f), x1, y1, hc.withAlpha (0.0f), x2, y2, false));
            g.drawLine (x1, y1, x2, y2, 1.1f);
        }
        dashV (g, kx, gy + 18, gy + gh - 8, 2, 3, hc.withAlpha (0.45f), 1.0f);
    }
    {
        const float gx = gx0 + 2 * (gw + gap); miniGraph (g, gx, gy, gw, gh, "PEAKS SHAVED");
        const float mid = gy + gh / 2 + 6;
        juce::Path pin, pout;
        for (int i = 0; i <= 180; ++i)
        {
            const int k = ((sp - 181 + i) % wall::Clipper::scopeN + wall::Clipper::scopeN) % wall::Clipper::scopeN;
            const float a = juce::jlimit (-1.5f, 1.5f, cd.scope[(size_t) (k * 2)].load()), y = juce::jlimit (-1.5f, 1.5f, cd.scope[(size_t) (k * 2 + 1)].load());
            const float x = gx + 10 + (gw - 20) * (float) i / 180;
            if (i) { pin.lineTo (x, mid - a * 0.7f * 34); pout.lineTo (x, mid - y * 0.7f * 34); } else { pin.startNewSubPath (x, mid - a * 0.7f * 34); pout.startNewSubPath (x, mid - y * 0.7f * 34); }
        }
        { juce::Graphics::ScopedSaveState s2 (g); g.reduceClipRegion (R (gx, gy + 18, gw, gh - 22).toNearestInt());
          g.setColour (hex (0xff9a2e, 0.35f)); g.strokePath (pin, stroke (1.0f)); g.setColour (ivory); g.strokePath (pout, stroke (1.2f)); }
        for (float sg : { -1.0f, 1.0f }) dashH (g, gx + 10, gx + gw - 10, mid - sg * 0.7f * 34, 4, 4, orange.withAlpha (0.7f), 1.0f);
    }
    static const char* cn[] { "Hard", "Soft", "Analog", "Tube" };
    const float x0 = C - 90 - 96 * 1.5f, y = PY + 156;
    paramKnob (g, "cAmt", x0 + 48, y, 44, "AMOUNT", juce::String (pv ("cAmt"), 1) + " dB", orange, false, 1);
    paramKnob (g, "cKnee", x0 + 144, y, 44, "KNEE", juce::String (cn[curve]) + " " + juce::String (juce::roundToInt (pv ("cKnee"))) + "%", ivory, false, 1);
    paramKnob (g, "cCeil", x0 + 240, y, 44, "CEILING", dbs (pv ("cCeil")) + " dB", ivory, false, 1);
    cycleTab (g, C + 210, PY + 190, juce::String (cn[curve]).toUpperCase(), 130, 1.2f, "cCurve");
    zoneXf = {};
}

void WallCanvas::paintLimiter (juce::Graphics& g)
{
    const float PX = C - 372, PW2 = 744, PY = 548, PH = 202;
    // V5: the Limiter panel 20% smaller (anchored at the bottom centre)
    const float sc = 0.8f, ax = C, ay = PY + PH;
    glassPanel (g, ax - (ax - (PX + 80)) * sc, ay - (ay - PY) * sc, (PW2 - 160) * sc, PH * sc, 0.75f);
    juce::Graphics::ScopedSaveState sst (g);
    zoneXf = juce::AffineTransform::translation (-ax, -ay).scaled (sc).translated (ax, ay);
    g.addTransform (zoneXf);
    const bool on = pv ("limOn") > 0.5f;
    panelTitle (g, PX + 100, PY + 22, "LIMITER", on ? juce::String() : juce::String ("OFF ") + middot + " THE CEILING STILL HOLDS", orange, on ? juce::jlimit (0.0f, 1.0f, limShown / 6.0f) : 0.0f);
    const int si = (int) pv ("limStyle");
    cycleTab (g, C, PY + 26, wall::styles()[(size_t) si].name, 190, 1.2f, "limStyle");
    text (g, wall::styles()[(size_t) si].blurb, C, PY + 66, Face::Regular, 9.5f, grey, 'c');
    const float kw = 104, x0 = C - kw * 2.5f, y = PY + 82;
    paramKnob (g, "limAtk", x0 + kw * 0.5f, y, 54, "ATTACK", juce::String (pv ("limAtk"), pv ("limAtk") < 10 ? 1 : 0) + " ms", coral, false, 1);
    // V2: the external sidechain - the limiter listens to the louder of the music and the key (stem mastering)
    {
        const bool sc = pv ("scOn") > 0.5f, has = proc.keyActive;
        const juce::String t = sc ? (has ? juce::String ("SIDECHAIN  ON") : juce::String ("SIDECHAIN  ") + middot + "  NO KEY") : juce::String ("SIDECHAIN  OFF");
        const float w = tw (t, Face::CondMedium, 9, 1.4f) + 28, sx = PX + PW2 - 100 - w;
        fillR (g, sx, PY + 14, w, 18, 9, sc ? (has ? hex (0x4ade80, 0.14f) : amber.withAlpha (0.14f)) : hex (0x1b1c20));
        strokeR (g, sx, PY + 14, w, 18, 9, sc ? (has ? hex (0x4ade80, 0.7f) : amber.withAlpha (0.7f)) : hex (0x34353b));
        dot (g, sx + 10, PY + 23, 2.5f, sc ? (has ? green : amber) : grey);
        text (g, t, sx + 16, PY + 23.5f, Face::CondMedium, 9, sc ? (has ? mint : amber) : label, 'l', 1.4f);
        zone (R (sx, PY + 14, w, 18), Hit::Toggle, "scOn");
        paramKnob (g, "scGain", x0 + kw * 4.5f, y, 54, "SC GAIN", dbs (pv ("scGain"), 1, true) + " dB", sc ? hex (0x4ade80) : ivory.withAlpha (0.4f), true, 1);
    }
    paramKnob (g, "limStages", x0 + kw * 1.5f, y, 54, "STAGES", juce::String ((int) pv ("limStages")), coral, false, 1);
    paramKnob (g, "limLook", x0 + kw * 2.5f, y, 54, "LOOKAHEAD", juce::String (pv ("limLook"), 1) + " ms", ivory, false, 1);
    paramKnob (g, "limLinkT", x0 + kw * 3.5f, y, 54, juce::String ("LINK T ") + middot + " R", juce::String (juce::roundToInt (pv ("limLinkT"))) + " " + middot + " " + juce::String (juce::roundToInt (pv ("limLinkR"))), ivory, false, 1);
    zoneXf = {};
}

// ============================================================================================================
// BANDS: free-standing bands low on the screen over the wave (like Pro-MB), the touched one lit, its dials under it
void WallCanvas::paintBands (juce::Graphics& g)
{
    const float PX = C - 372, PW2 = 744, sc = 0.8f, bx = C - 372, by = 470, bwid = 744, bh = 300;
    const float ax = bx + bwid / 2, ay = by + bh;
    juce::Graphics::ScopedSaveState s (g);
    zoneXf = juce::AffineTransform::translation (-ax, -ay).scaled (sc).translated (ax, ay);
    g.addTransform (zoneXf);
    // a soft shade behind, so the bands read over the wave
    radial (g, C, 600, 440, hex (0x0a090c, 0.6f), hex (0x0a090c, 0.0f));
    const float BX0 = PX + 10, BX1 = PX + PW2 - 10;
    auto fxp = [&] (float hz) { return BX0 + (BX1 - BX0) * std::log (hz / 20.0f) / std::log (1000.0f); };
    const float Z = 578, T = 530;
    const bool on = pv ("bandsOn") > 0.5f;
    const std::pair<float, const char*> fl[] { { 50, "50" }, { 100, "100" }, { 200, "200" }, { 500, "500" }, { 1000, "1k" }, { 2000, "2k" }, { 5000, "5k" }, { 10000, "10k" } };
    for (auto& [hz, t] : fl) text (g, t, fxp (hz), 632, Face::Regular, 8.5f, faint, 'c');
    const int nb = juce::jlimit (2, 4, (int) pv ("bCount"));
    selBand = juce::jlimit (0, nb - 1, selBand);
    float xo[3] { pv ("x1"), pv ("x2"), pv ("x3") };
    for (int k = 1; k < 3; ++k) xo[k] = std::max (xo[k], xo[k - 1] * 1.5f);
    auto& bd = proc.bandsDsp();
    for (int b = 0; b < nb; ++b)
    {
        const float lo = b == 0 ? 20.0f : xo[b - 1], hi = b == nb - 1 ? 20000.0f : xo[b];
        const float x0 = fxp (lo) + (lo > 20 ? 5.0f : 0.0f), x1 = fxp (hi) - (hi < 20000 ? 5.0f : 0.0f);
        const bool sel = b == selBand; const auto c = bandColour (b);
        const bool byp = pv (wall::bandId (b, "Byp")) > 0.5f, solo = pv (wall::bandId (b, "Solo")) > 0.5f;
        const float mv = on && ! byp ? bd.meterDb[(size_t) b].load() : 0.0f;
        juce::Graphics::ScopedSaveState s2 (g);
        g.setOpacity (sel ? 1.0f : 0.5f);
        const float a = (on ? 1.0f : 0.45f) * (byp ? 0.5f : 1.0f);
        {
            juce::Path body; body.addRoundedRectangle (x0, T, x1 - x0, 90, 12, 12, true, true, false, false);
            g.setGradientFill (juce::ColourGradient (c.withAlpha ((sel ? 0.16f : 0.10f) * a), 0, T, c.withAlpha (0.0f), 0, T + 90, false)); g.fillPath (body);
            juce::Path top; top.startNewSubPath (x0, T + 22); top.quadraticTo (x0, T, x0 + 12, T); top.lineTo (x1 - 12, T); top.quadraticTo (x1, T, x1, T + 22);
            if (sel) { g.setColour (c.withAlpha (0.25f * a)); g.strokePath (top, stroke (6.0f, true)); }
            g.setColour (c.withAlpha ((sel ? 0.85f : 0.5f) * a)); g.strokePath (top, stroke (sel ? 2.0f : 1.2f));
            for (float ex : { x0, x1 }) { g.setGradientFill (juce::ColourGradient (c.withAlpha ((sel ? 0.6f : 0.35f) * a), 0, T + 22, c.withAlpha (0.0f), 0, T + 90, false)); g.fillRect (ex - (ex == x0 ? 0.0f : 1.0f), T + 22, 1.0f, 68.0f); }
        }
        // the live movement: compression pulls down (coral, shaded), expansion and upward push up (dashed)
        auto bandLine = [&] (float yd) { juce::Path p; for (int i = 0; i <= 40; ++i) { const float t = (float) i / 40, e = std::max (0.0f, std::sin (juce::MathConstants<float>::pi * t)), y = Z + (yd - Z) * std::pow (e, 0.35f); if (i) p.lineTo (x0 + (x1 - x0) * t, y); else p.startNewSubPath (x0, y); } return p; };
        if (mv < -0.05f)
        {
            const float yd = Z + juce::jlimit (0.0f, 12.0f, -mv) * 6;
            juce::Path p = bandLine (yd); p.lineTo (x1 - 1, Z); p.lineTo (x0 + 1, Z); p.closeSubPath();
            g.setGradientFill (juce::ColourGradient (coral.withAlpha (0.05f), 0, Z, coral.withAlpha (0.4f), 0, yd, false)); g.fillPath (p);
            g.setColour (coral); g.strokePath (bandLine (yd), stroke (1.5f));
        }
        if (mv > 0.05f)
        {
            const float ye = Z - juce::jlimit (0.0f, 12.0f, mv) * 6;
            juce::Path p = bandLine (ye), d; const float dl[2] { 3, 3 };
            stroke (1.5f).createDashedStroke (d, p, dl, 2);
            g.setColour (red.withAlpha (0.8f)); g.fillPath (d);
        }
        g.setColour (juce::Colours::white.withAlpha (0.16f)); g.fillRect (x0, Z, x1 - x0, 1.0f);
        const float cx = (x0 + x1) / 2; const juce::String nm (bandName (b, nb));
        const float w = tw (nm, Face::CondMedium, 9.5f, 1.8f) + 34;
        fillR (g, cx - w / 2, T - 10, w, 20, 10, hex (0x0e0e10, 0.9f)); strokeR (g, cx - w / 2, T - 10, w, 20, 10, c.withAlpha (sel ? 0.85f : 0.45f));
        dot (g, cx - w / 2 + 11, T, 3, solo ? juce::Colours::white : c);
        text (g, nm, cx + 5, T + 0.5f, Face::CondMedium, 9.5f, sel ? ink : label, 'c', 1.8f);
        if (! sel) text (g, (mv < 0 ? minus : juce::String ("+")) + juce::String (std::abs (mv), 1) + " dB", cx, T + 22, Face::Regular, 10, soft, 'c');
        zone (R (x0, T - 10, x1 - x0, 100), Hit::BandBody, {}, b);
    }
    // crossover grips (drag them)
    for (int k = 0; k < nb - 1; ++k)
    {
        const float hz = xo[k], x = fxp (hz);
        fillR (g, x - 2, T + 20, 4, 40, 2, amber.withAlpha (0.75f));
        fillR (g, x - 24, T - 34, 48, 16, 8, hex (0x0e0e10, 0.85f)); strokeR (g, x - 24, T - 34, 48, 16, 8, amber.withAlpha (0.55f));
        text (g, hz >= 1000 ? juce::String (hz / 1000.0f, hz >= 10000 ? 0 : 1).trimCharactersAtEnd (".0") + "k" : juce::String (juce::roundToInt (hz)) + " Hz", x, T - 25.5f, Face::Medium, 9, amber, 'c');
        zone (R (x - 24, T - 36, 48, 100), Hit::BandXover, juce::String ("x") + juce::String (k + 1), k);
    }
    // + / - a band (2 to 4)
    if (nb < 4) { fillR (g, BX1 - 26, T - 34, 24, 16, 8, juce::Colours::white.withAlpha (0.08f)); text (g, "+", BX1 - 14, T - 26.5f, Face::Medium, 12, ink, 'c'); zone (R (BX1 - 26, T - 34, 24, 16), Hit::BandAdd); }
    if (nb > 2) { fillR (g, BX1 - 54, T - 34, 24, 16, 8, juce::Colours::white.withAlpha (0.08f)); text (g, minus, BX1 - 42, T - 26.5f, Face::Medium, 12, ink, 'c'); zone (R (BX1 - 54, T - 34, 24, 16), Hit::BandRemove); }
    // on / off, and the general controls, top left
    {
        const float x = BX0, y = 730;
        fillR (g, x, y, 64, 16, 8, on ? hex (0x4ade80, 0.12f) : hex (0x1b1c20)); strokeR (g, x, y, 64, 16, 8, on ? hex (0x4ade80, 0.6f) : hex (0x34353b));
        text (g, on ? "BANDS ON" : "BANDS OFF", x + 32, y + 8.5f, Face::CondMedium, 8.5f, on ? mint : ink, 'c', 1.2f);
        zone (R (x, y, 64, 16), Hit::Toggle, "bandsOn");
        const juce::String ln = "LINK  " + choiceName (proc, "bLink").toUpperCase();
        const float lw2 = tw (ln, Face::CondMedium, 8.5f, 1.2f) + 18;
        fillR (g, x + 70, y, lw2, 16, 8, juce::Colours::white.withAlpha (0.08f)); text (g, ln, x + 70 + lw2 / 2, y + 8.5f, Face::CondMedium, 8.5f, ink, 'c', 1.2f);
        zone (R (x + 70, y, lw2, 16), Hit::CycleTab, "bLink");
        const bool scb = pv ("bSc") > 0.5f;
        fillR (g, x + 76 + lw2, y, 44, 16, 8, scb ? amber.withAlpha (0.2f) : juce::Colours::white.withAlpha (0.08f)); text (g, "S/C +", x + 98 + lw2, y + 8.5f, Face::CondMedium, 8.5f, scb ? amber : ink, 'c', 1.2f);
        zone (R (x + 76 + lw2, y, 44, 16), Hit::Toggle, "bSc");
    }
    // the touched band's dials, floating right under it
    {
        const int b = selBand;
        const float lo = b == 0 ? 20.0f : xo[b - 1], hi = b == nb - 1 ? 20000.0f : xo[b];
        const float cxb = juce::jlimit (BX0 + 250, BX1 - 230, (fxp (lo) + fxp (hi)) / 2 - 20), y = 650;
        const float ratio = pv (wall::bandId (b, "Ratio"));
        const juce::String rs = std::abs (ratio) < 0.01f ? juce::String ("1:1") : ratio < 0 ? juce::String (1.4f + (-ratio) * 4.6f, 1) + ":1" : "1:" + juce::String (1.0f + ratio * 0.8f, 1) + " exp";
        const float kx0 = cxb - 160;
        paramKnob (g, wall::bandId (b, "Ratio"), kx0 + 40, y, 40, "RATIO", rs, ivory, true, 1, "COMP", "EXP");
        paramKnob (g, wall::bandId (b, "Up"), kx0 + 120, y, 40, "UPWARD", dbs (pv (wall::bandId (b, "Up")), 1, true) + " dB", ivory, false, 1);
        paramKnob (g, wall::bandId (b, "Lim"), kx0 + 200, y, 40, "LIMIT", (pv (wall::bandId (b, "Lim")) > 0.05f ? minus : juce::String()) + juce::String (pv (wall::bandId (b, "Lim")), 1) + " dB", ivory, false, 1);
        paramKnob (g, wall::bandId (b, "Gain"), kx0 + 280, y, 40, "GAIN", dbs (pv (wall::bandId (b, "Gain")), 1, true) + " dB", ivory, true, 1);
        // the MLA meter: expansion lights red going up, compression green going down
        const float mx = cxb + 176, my = y - 2, mh = 78, mv = on ? bd.meterDb[(size_t) b].load() : 0.0f;
        for (int i = 0; i < 16; ++i)
        {
            const float yy = my + (float) i * (mh / 16); const bool isExp = i < 6;
            const bool lit = isExp ? (mv > 0 && (5 - i) < (int) std::ceil (mv / 1.0f)) : (mv < 0 && (i - 6) < (int) std::ceil (-mv / 1.0f));
            fillR (g, mx, yy, 10, mh / 16 - 1.4f, 1.4f, lit ? (isExp ? hex (0xff5a4a) : green) : juce::Colours::white.withAlpha (0.07f));
        }
        const bool so = pv (wall::bandId (b, "Solo")) > 0.5f, by2 = pv (wall::bandId (b, "Byp")) > 0.5f;
        fillR (g, mx + 18, y + 8, 18, 18, 9, so ? ivory.withAlpha (0.85f) : juce::Colours::white.withAlpha (0.1f)); text (g, "S", mx + 27, y + 17.5f, Face::SemiBold, 9, so ? hex (0x1a1208) : hex (0xd8d1c4), 'c');
        fillR (g, mx + 18, y + 32, 18, 18, 9, by2 ? amber.withAlpha (0.85f) : juce::Colours::white.withAlpha (0.1f)); text (g, "B", mx + 27, y + 41.5f, Face::SemiBold, 9, by2 ? hex (0x1a1208) : hex (0xd8d1c4), 'c');
        zone (R (mx + 18, y + 8, 18, 18), Hit::BandSolo, {}, b); zone (R (mx + 18, y + 32, 18, 18), Hit::BandByp, {}, b);
        // the bands' shared threshold
        paramKnob (g, "bThr", BX0 + 40, y, 40, "THRESHOLD", dbs (pv ("bThr"), 1) + " dB", amber, false, 1);
    }
}

// ============================================================================================================
// LOUDNESS: the song's loudness so far, with the integrated, your target and the platform's, and the loud stretches
void WallCanvas::paintLoudView (juce::Graphics& g)
{
    {
        juce::Graphics::ScopedSaveState s (g); juce::Path p; p.addRoundedRectangle (236, 126, W - 236 - 250, 372, 14.0f); g.reduceClipRegion (p);
        g.setColour (hex (0x0c0b0f, 0.26f)); g.fillRect (R (0, 0, W, H));   // V4: more glass
    }
    strokeR (g, 236, 126, W - 236 - 250, 372, 14, juce::Colours::white.withAlpha (0.08f));
    const float X0 = 262, X1 = W - 268, Y0 = 140, Y1 = 470;
    const float tgt = pv ("target"), span18 = (float) (proc.ui ("loudRange") >= 36 ? 36 : 18), hiL = std::ceil ((std::max (tgt, proc.lufsI.load()) + span18 * 4 / 18) / 2) * 2, loL = hiL - span18;   // (V3: room for the song when it runs above the target)
    auto ly = [&] (float l) { return Y1 - (juce::jlimit (loL, hiL, l) - loL) / (hiL - loL) * (Y1 - Y0); };
    for (float l = loL + 2; l <= hiL; l += (span18 > 20 ? 8.0f : 4.0f)) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (X0, ly (l), X1 - X0, 1.0f); text (g, dbs (l, 0), X0 - 8, ly (l) + 0.5f, Face::Regular, 8.5f, faint, 'r'); }
    text (g, "LUFS", X0 - 8, Y0 - 14, Face::CondMedium, 8, faint, 'r', 1.4f);
    const auto& h = proc.lufsHistory;
    const int n = (int) h.size();
    const float I = proc.lufsI.load();
    const int span = std::max (600, n);   // at least a minute across
    auto xt = [&] (int i) { return X0 + (X1 - X0) * (float) i / (float) (span - 1); };
    // the time
    for (int sec = 0; sec <= span / 10; sec += (span > 3000 ? 60 : span > 1200 ? 30 : 10))
    {
        const float x = xt (sec * 10);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (x, Y0, 1.0f, Y1 - Y0);
        text (g, juce::String (sec / 60) + ":" + juce::String (sec % 60).paddedLeft ('0', 2), x, Y1 + 16, Face::CondMedium, 8.5f, faint, 'c', 1.2f);
    }
    if (n > 1)
    {
        // the stretches more than 3 LU over the song
        if (I > -69.0f)
        {
            int start = -1;
            for (int i = 0; i <= n; ++i)
            {
                const bool over = i < n && h[(size_t) i] > I + 3.0f;
                if (over && start < 0) start = i;
                if (! over && start >= 0)
                {
                    if (i - start > 20)
                    {
                        float mx = -99; for (int j = start; j < i; ++j) mx = std::max (mx, h[(size_t) j]);
                        g.setColour (hex (0xff5a4a, 0.10f)); g.fillRect (xt (start), Y0, xt (i) - xt (start), Y1 - Y0);
                        text (g, "+" + juce::String (mx - I, 1) + " LU over the song", (xt (start) + xt (i)) / 2, Y0 + 12, Face::Regular, 9, coral, 'c');
                    }
                    start = -1;
                }
            }
        }
        juce::Path p; bool st = false;
        for (int i = 0; i < n; ++i) { if (h[(size_t) i] <= -69.0f) { st = false; continue; } const float x = xt (i), y = ly (h[(size_t) i]); if (st) p.lineTo (x, y); else { p.startNewSubPath (x, y); st = true; } }
        g.setColour (hex (0xf4efe4)); g.strokePath (p, stroke (1.8f));
        const float xp = xt (n - 1);
        g.setColour (juce::Colours::white.withAlpha (0.6f)); g.fillRect (xp, Y0 - 6, 1.5f, Y1 - Y0 + 6); dot (g, xp, Y0 - 6, 3, juce::Colours::white);
    }
    else text (g, "play the song - its loudness draws here", (X0 + X1) / 2, (Y0 + Y1) / 2, Face::Regular, 11, grey, 'c');
    auto tag = [&] (float y, const juce::String& t, juce::Colour col)
    {
        const float w = tw (t, Face::CondMedium, 9, 1.2f) + 18;
        fillR (g, X0 + 8, y - 9, w, 18, 9, hex (0x0e0e10, 0.85f)); strokeR (g, X0 + 8, y - 9, w, 18, 9, col.withAlpha (0.7f));
        text (g, t, X0 + 8 + w / 2, y + 0.5f, Face::CondMedium, 9, col, 'c', 1.2f);
    };
    if (I > -69.0f) { g.setColour (amber); g.fillRect (X0, ly (I), X1 - X0, 1.5f); tag (ly (I) - 14, "INTEGRATED " + dbs (I), amber); }
    dashH (g, X0, X1, ly (tgt), 3, 4, amber.withAlpha (0.6f), 1.0f);
    {
        static const char* plat[] { "SPOTIFY", "APPLE MUSIC", "YOUTUBE", "TIDAL", "AMAZON" };
        static const float lv[] { -14, -16, -14, -14, -14 };
        const int w = (int) pv ("preview") > 0 ? (int) pv ("preview") - 1 : (int) pv ("previewWhere");
        dashH (g, X0, X1, ly (lv[w]), 6, 5, spot.withAlpha (0.8f), 1.2f);
        tag (ly (lv[w]) + 14, juce::String (plat[w]) + " " + dbs (lv[w], 0), spot);
    }
    fillR (g, X1 - 60, Y0 - 26, 56, 16, 8, juce::Colours::white.withAlpha (0.08f)); text (g, "RESET", X1 - 32, Y0 - 17.5f, Face::CondMedium, 8.5f, ink, 'c', 1.4f);
    zone (R (X1 - 60, Y0 - 26, 56, 16), Hit::LoudReset);
}

// what the streaming preview is doing right now
void WallCanvas::paintPreviewBanner (juce::Graphics& g)
{
    if (const int li = proc.learnIndex.load(); li >= 0)
    {
        auto ps = proc.getParameters();
        const juce::String t = "MIDI LEARN  " + middot + "  move a knob on your controller for " + (juce::isPositiveAndBelow (li, ps.size()) ? ps[li]->getName (40).toUpperCase() : juce::String());
        const float w = tw (t, Face::CondMedium, 10, 1.4f) + 40;
        fillR (g, C - w / 2, 124, w, 24, 12, amber.withAlpha (0.16f)); strokeR (g, C - w / 2, 124, w, 24, 12, amber.withAlpha (0.8f));
        dot (g, C - w / 2 + 14, 136, 3.5f, amber.withAlpha (0.5f + 0.5f * std::sin (animT * 6.0f)));
        text (g, t, C + 6, 136.5f, Face::CondMedium, 10, juce::Colours::white, 'c', 1.4f);
    }
    const int pvIdx = (int) pv ("preview");
    if (pvIdx <= 0) return;
    static const char* plat[] { "SPOTIFY", "APPLE MUSIC", "YOUTUBE", "TIDAL", "AMAZON" };
    static const char* nice[] { "Spotify", "Apple Music", "YouTube", "Tidal", "Amazon" };
    const float d = proc.previewDb.load();
    const juce::String t = juce::String (plat[pvIdx - 1]) + " PREVIEW  " + middot + "  " + (d < -0.05f ? "playing " + juce::String (-d, 1) + " dB quieter, as " + nice[pvIdx - 1] + " will" : juce::String ("no turn-down: it's under ") + nice[pvIdx - 1] + "'s level");
    const float w = tw (t, Face::CondMedium, 10, 1.4f) + 40;
    const float by = 404 - 12;   // V5: in the middle of the screen
    fillR (g, C - w / 2, by, w, 24, 12, hex (0x0e0e10, 0.55f));   // (so it reads over the wave)
    fillR (g, C - w / 2, by, w, 24, 12, hex (0x1ed760, 0.14f)); strokeR (g, C - w / 2, by, w, 24, 12, hex (0x1ed760, 0.7f));
    dot (g, C - w / 2 + 14, by + 12, 3.5f, spot);
    text (g, t, C + 6, by + 12.5f, Face::CondMedium, 10, juce::Colours::white, 'c', 1.4f);
}
