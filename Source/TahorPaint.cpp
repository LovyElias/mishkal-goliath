// Mishkal Tahor - the screen, drawn with the numbers of the approved Claude Design canvas (Tabs.dc.html, Units.dc.html)
#include "TahorEditor.h"

using namespace tahor;
using namespace tahor::ui;

namespace
{
const juce::Colour ink = col ("#ece6da"), grey = col ("#8d8a84"), dim = col ("#5f5c57"), amber = col ("#f3a93c"), red = col ("#ef6c6c");
const juce::Colour crimson = col ("#b3262f"), crimsonLine = col ("#c0303c"), soft = col ("#ece9e4");
constexpr float pi = juce::MathConstants<float>::pi;

juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }
const juce::String MINUS = U8 ("\xe2\x88\x92"), DOT = U8 (" \xc2\xb7 ");

void fillRound (juce::Graphics& g, R r, float rad, juce::Colour c) { g.setColour (c); g.fillRoundedRectangle (r, rad); }
void strokeRound (juce::Graphics& g, R r, float rad, juce::Colour c, float w = 1.0f) { g.setColour (c); g.drawRoundedRectangle (r.reduced (w * 0.5f), rad, w); }

// V7: the band panel - a soft arch rising to a peak under its band, corners sweeping into the bottom,
// a thin glow in the band's colour along the curved top, over dark smoked glass
void bandPanel (juce::Graphics& g, R b, float peakX, juce::Colour c)
{
    const float x0 = b.getX(), x1 = b.getRight(), y0 = b.getY(), y1 = b.getBottom();
    const float r = 14.0f, rb = 20.0f, rise = 11.0f;
    const float span = juce::jmin (95.0f, (b.getWidth() - 2 * r) * 0.5f - 4.0f);
    const float px = juce::jlimit (x0 + r + span, x1 - r - span, peakX);
    juce::Path top;   // the top edge, left corner to right corner
    top.startNewSubPath (x0, y0 + r);
    top.cubicTo (x0, y0 + r * 0.45f, x0 + r * 0.45f, y0, x0 + r, y0);
    top.lineTo (px - span, y0);
    top.cubicTo (px - span * 0.45f, y0, px - span * 0.5f, y0 - rise, px, y0 - rise);
    top.cubicTo (px + span * 0.5f, y0 - rise, px + span * 0.45f, y0, px + span, y0);
    top.lineTo (x1 - r, y0);
    top.cubicTo (x1 - r * 0.45f, y0, x1, y0 + r * 0.45f, x1, y0 + r);
    juce::Path body (top);
    body.lineTo (x1, y1 - rb);
    body.cubicTo (x1, y1 - rb * 0.2f, x1 - rb * 0.2f, y1, x1 - rb * 1.3f, y1);   // the corners sweep into the bottom
    body.lineTo (x0 + rb * 1.3f, y1);
    body.cubicTo (x0 + rb * 0.2f, y1, x0, y1 - rb * 0.2f, x0, y1 - rb);
    body.closeSubPath();
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 26, { 0, 10 }).drawForPath (g, body);
    // V8: worn and a little dirty - warm smoky metal, brushed grain, smudges, scuffs, rubbed edges
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xf5221d18), 0, y0 - rise, juce::Colour (0xf80e0c0a), 0, y1, false));
    g.fillPath (body);
    {
        g.saveState(); g.reduceClipRegion (body);
        juce::Random rnd ((juce::int64) (b.getWidth() * 131.0f + b.getHeight() * 7.0f));   // the same wear every time
        // brushed grain: fine horizontal strokes of slightly lighter and darker metal
        for (float yy = y0 - rise; yy < y1; yy += 1.5f)
        {
            const float a = 0.02f + rnd.nextFloat() * 0.045f;
            const float xs = x0 + rnd.nextFloat() * b.getWidth() * 0.4f, xe = x1 - rnd.nextFloat() * b.getWidth() * 0.4f;
            g.setColour ((rnd.nextBool() ? juce::Colour (0xfffff2dc) : juce::Colours::black).withAlpha (a));
            g.fillRect (xs, yy, xe - xs, 0.7f);
        }
        // smudges: soft dark and pale blotches
        for (int i = 0; i < 6; ++i)
        {
            const float cx = x0 + rnd.nextFloat() * b.getWidth(), cy2 = y0 + rnd.nextFloat() * b.getHeight(), rr = 14.0f + rnd.nextFloat() * 34.0f;
            const auto sc = (i % 3 == 0 ? juce::Colour (0xffd8c6a6) : juce::Colours::black).withAlpha (i % 3 == 0 ? 0.05f : 0.22f);
            g.setGradientFill (juce::ColourGradient (sc, cx, cy2, sc.withAlpha (0.0f), cx + rr, cy2, true));
            g.fillEllipse (cx - rr, cy2 - rr * 0.7f, rr * 2, rr * 1.4f);
        }
        // scuffs: short thin scratches
        for (int i = 0; i < 22; ++i)
        {
            const float sx = x0 + rnd.nextFloat() * b.getWidth(), sy = y0 + rnd.nextFloat() * b.getHeight();
            const float ang = (rnd.nextFloat() - 0.5f) * 0.9f, len = 5.0f + rnd.nextFloat() * 18.0f;
            g.setColour (juce::Colour (0xfffff4e0).withAlpha (0.04f + rnd.nextFloat() * 0.07f));
            g.drawLine (sx, sy, sx + len * std::cos (ang), sy + len * std::sin (ang), 0.6f);
        }
        // a warm, smoky haze and a breath of the band's colour under the arch
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.10f), px, y0 - rise, c.withAlpha (0.0f), px, y0 + (y1 - y0) * 0.7f, true));
        g.fillRect (b.withTop (y0 - rise));
        // rubbed edges: darker all round, with paler worn spots where hands touch the corners
        g.setColour (juce::Colours::black.withAlpha (0.28f)); g.strokePath (body, juce::PathStrokeType (7.0f));
        g.setColour (juce::Colours::black.withAlpha (0.22f)); g.strokePath (body, juce::PathStrokeType (3.0f));
        for (auto pt : { juce::Point<float> (x0 + 6, y1 - 6), juce::Point<float> (x1 - 6, y1 - 6), juce::Point<float> (x0 + 8, y0 + 8) })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9d8b8).withAlpha (0.07f), pt.x, pt.y, juce::Colour (0xffe9d8b8).withAlpha (0.0f), pt.x + 22, pt.y, true));
            g.fillEllipse (pt.x - 22, pt.y - 22, 44, 44);
        }
        g.restoreState();
    }
    g.setColour (juce::Colour (0xffd8c6a6).withAlpha (0.08f)); g.strokePath (body, juce::PathStrokeType (1.0f));
    // the glow along the curved top - a touch dimmer and warmer, like an old lamp
    g.setColour (c.withAlpha (0.14f)); g.strokePath (top, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.30f)); g.strokePath (top, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.interpolatedWith (juce::Colour (0xffffe2b0), 0.2f).withAlpha (0.85f)); g.strokePath (top, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void shadow (juce::Graphics& g, R r, float rad, float dy, float blur, juce::Colour c)
{
    juce::Path p; p.addRoundedRectangle (r.translated (0, dy), rad);
    juce::DropShadow (c, juce::roundToInt (blur), {}).drawForPath (g, p);
}

// little vector icons for the symbols the fonts don't carry
enum class Icon { Undo, Redo, Star, Down, Power, Swap, Check, Cross, Grip, Left, Right, Up, TriDown, TriRight, TriLeft, Dot };
void icon (juce::Graphics& g, Icon k, R b, juce::Colour c, float w = 1.4f)
{
    g.setColour (c);
    const auto cx = b.getCentreX(), cy = b.getCentreY();
    const float s = juce::jmin (b.getWidth(), b.getHeight());
    juce::Path p;
    switch (k)
    {
        case Icon::Undo: case Icon::Redo:
        {
            const float r = s * 0.28f, dir = k == Icon::Undo ? 1.0f : -1.0f;
            p.addCentredArc (cx, cy + s * 0.06f, r, r, 0.0f, dir * -pi * 0.15f, dir * pi * 1.15f, true);
            g.strokePath (p, juce::PathStrokeType (w * 1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const float ax = cx - dir * r * 0.95f, ay = cy - r * 0.55f;
            juce::Path h; h.addTriangle (ax - dir * s * 0.13f, ay - s * 0.02f, ax + dir * s * 0.1f, ay - s * 0.16f, ax + dir * s * 0.06f, ay + s * 0.14f);
            g.fillPath (h);
            return;
        }
        case Icon::Star:
        {
            p.addStar ({ cx, cy }, 5, s * 0.2f, s * 0.46f, 0.0f);
            g.strokePath (p, juce::PathStrokeType (w * 0.8f, juce::PathStrokeType::mitered));
            return;
        }
        case Icon::Down: case Icon::TriDown:
            p.addTriangle (cx - s * 0.32f, cy - s * 0.18f, cx + s * 0.32f, cy - s * 0.18f, cx, cy + s * 0.22f); g.fillPath (p); return;
        case Icon::Up:
            p.addTriangle (cx - s * 0.32f, cy + s * 0.18f, cx + s * 0.32f, cy + s * 0.18f, cx, cy - s * 0.22f); g.fillPath (p); return;
        case Icon::TriRight:
            p.addTriangle (cx - s * 0.2f, cy - s * 0.3f, cx - s * 0.2f, cy + s * 0.3f, cx + s * 0.26f, cy); g.fillPath (p); return;
        case Icon::TriLeft:
            p.addTriangle (cx + s * 0.2f, cy - s * 0.3f, cx + s * 0.2f, cy + s * 0.3f, cx - s * 0.26f, cy); g.fillPath (p); return;
        case Icon::Power:
        {
            const float r = s * 0.34f;
            p.addCentredArc (cx, cy + s * 0.04f, r, r, 0.0f, pi * 0.22f, pi * 1.78f, true);
            g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine (cx, cy - s * 0.42f, cx, cy - s * 0.02f, w);
            return;
        }
        case Icon::Swap:
        {
            const float l = cx - s * 0.36f, r = cx + s * 0.36f, y1 = cy - s * 0.17f, y2 = cy + s * 0.17f;
            g.drawLine (l, y1, r - s * 0.1f, y1, w); g.drawLine (l + s * 0.1f, y2, r, y2, w);
            juce::Path a; a.addTriangle (r, y1, r - s * 0.2f, y1 - s * 0.13f, r - s * 0.2f, y1 + s * 0.13f);
            a.addTriangle (l, y2, l + s * 0.2f, y2 - s * 0.13f, l + s * 0.2f, y2 + s * 0.13f); g.fillPath (a);
            return;
        }
        case Icon::Check:
            p.startNewSubPath (cx - s * 0.3f, cy); p.lineTo (cx - s * 0.08f, cy + s * 0.24f); p.lineTo (cx + s * 0.32f, cy - s * 0.26f);
            g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); return;
        case Icon::Cross:
            g.drawLine (cx - s * 0.25f, cy - s * 0.25f, cx + s * 0.25f, cy + s * 0.25f, w);
            g.drawLine (cx + s * 0.25f, cy - s * 0.25f, cx - s * 0.25f, cy + s * 0.25f, w); return;
        case Icon::Grip:
            for (int i = 0; i < 2; ++i) for (int j = 0; j < 3; ++j)
                g.fillEllipse (cx - s * 0.16f + (float) i * s * 0.32f - 0.8f, cy - s * 0.3f + (float) j * s * 0.3f - 0.8f, 1.6f, 1.6f);
            return;
        case Icon::Left: case Icon::Right:
        {
            const float d = k == Icon::Left ? 1.0f : -1.0f;
            p.startNewSubPath (cx + d * s * 0.12f, cy - s * 0.24f); p.lineTo (cx - d * s * 0.12f, cy); p.lineTo (cx + d * s * 0.12f, cy + s * 0.24f);
            g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); return;
        }
        case Icon::Dot: g.fillEllipse (cx - s * 0.3f, cy - s * 0.3f, s * 0.6f, s * 0.6f); return;
    }
}

// CSS filter maths for the faceplate metal (sepia, saturate, contrast)
juce::Colour cssFilter (juce::Colour c, float sep, float sat, float con)
{
    float r = c.getFloatRed(), gg = c.getFloatGreen(), b = c.getFloatBlue();
    const float t = 1.0f - sep;
    float r1 = (0.393f + 0.607f * t) * r + (0.769f - 0.769f * t) * gg + (0.189f - 0.189f * t) * b;
    float g1 = (0.349f - 0.349f * t) * r + (0.686f + 0.314f * t) * gg + (0.168f - 0.168f * t) * b;
    float b1 = (0.272f - 0.272f * t) * r + (0.534f - 0.534f * t) * gg + (0.131f + 0.869f * t) * b;
    r = juce::jlimit (0.0f, 1.0f, r1); gg = juce::jlimit (0.0f, 1.0f, g1); b = juce::jlimit (0.0f, 1.0f, b1);
    float r2 = (0.213f + 0.787f * sat) * r + (0.715f - 0.715f * sat) * gg + (0.072f - 0.072f * sat) * b;
    float g2 = (0.213f - 0.213f * sat) * r + (0.715f + 0.285f * sat) * gg + (0.072f - 0.072f * sat) * b;
    float b2 = (0.213f - 0.213f * sat) * r + (0.715f - 0.715f * sat) * gg + (0.072f + 0.928f * sat) * b;
    auto k = [con] (float v) { return juce::jlimit (0.0f, 1.0f, (juce::jlimit (0.0f, 1.0f, v) - 0.5f) * con + 0.5f); };
    return juce::Colour::fromFloatRGBA (k (r2), k (g2), k (b2), c.getFloatAlpha());
}

// CSS linear-gradient geometry (angle in degrees, clockwise from "to top")
juce::ColourGradient cssLinear (R box, float deg)
{
    const float a = deg * pi / 180.0f;
    const float dx = std::sin (a), dy = -std::cos (a);
    const float L = std::abs (box.getWidth() * dx) + std::abs (box.getHeight() * dy);
    const auto c = box.getCentre();
    juce::ColourGradient gr; gr.isRadial = false;
    gr.point1 = { c.x - dx * L * 0.5f, c.y - dy * L * 0.5f };
    gr.point2 = { c.x + dx * L * 0.5f, c.y + dy * L * 0.5f };
    return gr;
}
float cssLinearLength (R box, float deg)
{
    const float a = deg * pi / 180.0f;
    return std::abs (box.getWidth() * std::sin (a)) + std::abs (box.getHeight() * std::cos (a));
}
// a radial ellipse (rx, ry) at (cx, cy) with stops (position 0..1 of the radius)
void fillEllipseGrad (juce::Graphics& g, R box, float cx, float cy, float rx, float ry, std::initializer_list<std::pair<float, juce::Colour>> stops)
{
    juce::ColourGradient gr; gr.isRadial = true;
    gr.point1 = { cx, cy }; gr.point2 = { cx + rx, cy };
    for (auto& s : stops) gr.addColour (s.first, s.second);
    juce::FillType ft (gr);
    ft.transform = juce::AffineTransform::scale (1.0f, ry / juce::jmax (0.01f, rx), cx, cy);
    g.setFillType (ft);
    g.fillRect (box);
}

juce::Colour resoShade (int shape)   // V6: one red family, a shade per kind of shape
{
    switch (shape)
    {
        case 0: case 1: case 5: return col ("#ef6c6c");   // bell, bandpass, notch
        case 4:                 return col ("#ff7a45");   // tilt
        default:                return col ("#c0303c");   // shelves and cuts
    }
}
// the shape's little curve, for the dropdown
void shapeIcon (juce::Graphics& g, int shape, R b, juce::Colour c)
{
    juce::Path p;
    const float x0 = b.getX(), x1 = b.getRight(), yb = b.getBottom() - 1, yt = b.getY() + 1, xm = b.getCentreX(), w = b.getWidth();
    switch (shape)
    {
        case 0: p.startNewSubPath (x0, yb); p.cubicTo (xm - w * 0.15f, yb, xm - w * 0.2f, yt, xm, yt); p.cubicTo (xm + w * 0.2f, yt, xm + w * 0.15f, yb, x1, yb); break;
        case 1: p.startNewSubPath (x0, yb); p.lineTo (xm, yt); p.lineTo (x1, yb); break;
        case 2: p.startNewSubPath (x0, yt); p.lineTo (xm - w * 0.1f, yt); p.cubicTo (xm, yt, xm, yb, xm + w * 0.1f, yb); p.lineTo (x1, yb); break;
        case 3: p.startNewSubPath (x0, yb); p.lineTo (xm - w * 0.1f, yb); p.cubicTo (xm, yb, xm, yt, xm + w * 0.1f, yt); p.lineTo (x1, yt); break;
        case 4: p.startNewSubPath (x0, yb); p.lineTo (x1, yt); break;
        case 5: p.startNewSubPath (x0, yt); p.lineTo (xm - 2, yt); p.lineTo (xm, yb); p.lineTo (xm + 2, yt); p.lineTo (x1, yt); break;
        case 6: p.startNewSubPath (x0 + 2, yb); p.cubicTo (xm - w * 0.15f, yt, xm, yt, x1, yt); break;
        default: p.startNewSubPath (x0, yt); p.cubicTo (xm, yt, xm + w * 0.15f, yt, x1 - 2, yb); break;
    }
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

float dbRange (TahorProcessor& p) { static const float r[4] { 3.0f, 6.0f, 12.0f, 30.0f }; return r[juce::jlimit (0, 3, (int) p.param ("range"))] * 1.5f; }
}

//==============================================================================
void Canvas::paint (juce::Graphics& g)
{
    zones.clear();
    glassRects.clear();
    revealRects.clear();
    soloHzUi = 0.0f; soloQUi = 1.0f;   // V9: the tab on show sets it (Reso or Character solo)
    // page background: linear-gradient(180deg, #121215, #0b0b0d)
    g.setGradientFill (juce::ColourGradient (col ("#121215"), 0, 0, col ("#0b0b0d"), 0, H, false));
    g.fillAll();

    paintHeader (g);
    paintBottomBar (g);      // V5: before the screen, so the screen's arches can open into the header and the bar
    paintDisplay (g);
    paintMeters (g);
    if (tab == 0)
    {
        const bool off = proc.param ("resoOn") < 0.5f;   // V9: the panels dim too (still adjustable)
        if (off) g.beginTransparencyLayer (0.45f);
        paintResoStrip (g); if (advOpen) paintAdvanced (g);
        if (off) g.endTransparencyLayer();
    }
    if (tab != 2) { charSolo = -1; addOpen = false; }
    if (tab != 0) resoSolo = -1;
    proc.charSoloHz.store (soloHzUi); proc.charSoloQ.store (soloQUi);

    // the resize corner: diagonal stripes
    {
        const R c (W - 17, H - 17, 14, 14);
        g.saveState(); g.reduceClipRegion (c.toNearestInt());
        g.setColour (col ("#5f5c57"));
        for (float o = -14.0f; o < 28.0f; o += 4.0f) g.drawLine (c.getX() + o, c.getBottom(), c.getX() + o + 14.0f, c.getY(), 1.0f);
        g.restoreState();
        zone (c.expanded (4), Hit::Resize);
    }
    if (addOpen) paintAddGallery (g);
    if (browseOpen) paintBrowser (g);
}

// V9: a band on solo - a lit column over the range you're hearing (as wide as its Q), the rest of the screen dimmed
void Canvas::paintSoloColumn (juce::Graphics& g)
{
    float hz = 0.0f, q = 1.0f; juce::Colour c = amber;
    if (tab == 1)
    {
        for (int b = 0; b < numBands; ++b)
            if (proc.param (bid (b, "Solo")) > 0.5f && proc.param (bid (b, "On")) > 0.5f)
            { hz = proc.param (bid (b, "Hz")); q = juce::jlimit (0.3f, 12.0f, proc.param (bid (b, "Q"))); c = bandColour (b); break; }
    }
    else if (tab == 0 && resoSolo >= 0 && proc.param (nid (resoSolo, "On")) > 0.5f)
    {
        hz = proc.param (nid (resoSolo, "Hz")); q = juce::jlimit (0.3f, 8.0f, proc.param (nid (resoSolo, "Q")) * 1.4f);
        c = resoShade ((int) proc.param (nid (resoSolo, "Shape")));
    }
    else if (tab == 2 && charSolo >= 0 && charSoloUnit == preview)
    {
        float v[unitValues]; proc.getUnitValues (preview, v, proc.unitSplit (preview) ? charSide : 0);
        const auto& nodes = unitNodes (preview);
        if (charSolo < (int) nodes.size())
        {
            const auto& nd = nodes[(size_t) charSolo];
            hz = nodeHz (nd, v); q = nd.qv >= 0 ? 0.5f + v[nd.qv] * 3.0f : 1.2f;
            c = col (units()[(size_t) preview].col);
        }
    }
    if (hz <= 0.0f) return;
    const float bw = 2.0f * std::asinh (1.0f / (2.0f * q)) / std::log (2.0f);   // octaves between the -3 dB points
    const float x0 = fx (hz * std::pow (2.0f, -bw * 0.5f)), x1 = fx (hz * std::pow (2.0f, bw * 0.5f));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (-40.0f, -80.0f, x0 + 40.0f, 900.0f);
    g.fillRect (x1, -80.0f, PW + 40.0f - x1, 900.0f);
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.10f), (x0 + x1) * 0.5f, 0, c.withAlpha (0.02f), x1, 0, true));
    g.fillRect (x0, -80.0f, x1 - x0, 900.0f);
    g.setColour (c.withAlpha (0.55f)); g.fillRect (x0 - 0.5f, -80.0f, 1.0f, 900.0f); g.fillRect (x1 - 0.5f, -80.0f, 1.0f, 900.0f);
    auto f = font (Face::CondMedium, 10, 1.6f);
    const juce::String t = "SOLO  " + hzText (hz);
    const float tw = textWidth (f, t) + 16;
    const R tb ((x0 + x1) * 0.5f - tw * 0.5f, 24, tw, 16);
    fillRound (g, tb, 8, col ("#0e0e10", 0.85f)); strokeRound (g, tb, 8, c.withAlpha (0.7f));
    text (g, t, f, c, tb, juce::Justification::centred);
}

// V9: the preset browser - tags along the top, the presets under them; click one to load it
void Canvas::paintBrowser (juce::Graphics& g)
{
    zone (R (0, 0, W, H), Hit::BrowseClose);
    const auto& fac = factoryPresets();
    const auto users = proc.userPresets();
    struct Item { juce::String name, tag; int id; };
    std::vector<Item> all;
    for (int i = 0; i < (int) fac.size(); ++i) all.push_back ({ juce::String::fromUTF8 (fac[(size_t) i].name), TahorProcessor::factoryTag (i), i });
    for (int i = 0; i < users.size(); ++i) all.push_back ({ users[i].getFileNameWithoutExtension(), proc.presetTagOf (users[i]), 1000 + i });
    juce::StringArray tags { "All" };
    for (auto& t : presetTagNames()) for (auto& it : all) if (it.tag == t) { tags.add (t); break; }
    if (! tags.contains (browseTag)) browseTag = "All";
    std::vector<Item> shown;
    for (auto& it : all) if (browseTag == "All" || it.tag == browseTag) shown.push_back (it);
    const int cols = 4, rows = juce::jmax (1, ((int) shown.size() + cols - 1) / cols);   // V11: a bigger library
    auto fchip = font (Face::Regular, 10.5f);
    const float pw = 880, rowH = 26;
    int chipRows = 1; { float cx = 18; for (auto& t : tags) { const float w = textWidth (fchip, t) + 20; if (cx + w > pw - 18) { ++chipRows; cx = 18; } cx += w + 6; } }
    const float listTop = 40 + (float) chipRows * 28 + 8;
    const float ph = listTop + (float) rows * rowH + 16;
    const R panel (presetRect.getX() - 30, 52, pw, ph);
    {
        juce::Path p; p.addRoundedRectangle (panel, 12);
        juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 24, { 0, 10 }).drawForPath (g, p);
        g.setColour (col ("#121215", 0.96f)); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.1f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    }
    zone (panel, Hit::Glass);
    text (g, "PRESETS", font (Face::CondMedium, 11, 1.8f), soft, R (panel.getX() + 18, panel.getY() + 14, 200, 16));
    text (g, "click to load " + juce::String (U8 ("\xc2\xb7")) + " save from the menu", font (Face::Regular, 10), grey, R (panel.getRight() - 260, panel.getY() + 14, 242, 16), juce::Justification::centredRight);
    // tag chips
    {
        auto f = fchip;
        float x = panel.getX() + 18, y = panel.getY() + 40;
        for (int i = 0; i < tags.size(); ++i)
        {
            const bool sel = tags[i] == browseTag;
            const float w = textWidth (f, tags[i]) + 20;
            if (x + w > panel.getRight() - 18) { x = panel.getX() + 18; y += 28; }
            const R c (x, y, w, 22);
            fillRound (g, c, 11, sel ? col ("#3a2c14") : col ("#1b1c20")); strokeRound (g, c, 11, sel ? col ("#6b4f1f") : col ("#34353b"));
            text (g, tags[i], f, sel ? amber : ink, c, juce::Justification::centred);
            zone (c, Hit::BrowseTag, i == 0 ? 0 : presetTagNames().indexOf (tags[i]) + 1);
            x += w + 6;
        }
    }
    const float cw = (pw - 36) / (float) cols;
    for (size_t i = 0; i < shown.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols;
        const R cell (panel.getX() + 18 + (float) c * cw, panel.getY() + listTop + (float) r * rowH, cw - 8, rowH - 4);
        const bool cur = shown[i].name == proc.presetName;
        const bool hv = cell.contains (mouse);
        if (cur || hv) fillRound (g, cell, 6, cur ? col ("#3a2c14", 0.9f) : col ("#1f2024"));
        text (g, shown[i].name, font (Face::Regular, 11), cur ? amber : ink, cell.reduced (10, 0));   // V11: the folder is the chip above
        zone (cell, Hit::BrowsePick, shown[i].id);
    }
}

// V8: + Add EQ opens the gear itself - every unit's faceplate, a small name under each, on a see-through panel
void Canvas::paintAddGallery (juce::Graphics& g)
{
    zone (R (0, 0, W, H), Hit::AddClose);   // a click anywhere else closes it
    const float sc = 0.3f, cw = 800 * sc, gapX = 18, gapY = 26, cols = 4;   // V9: four across (sixteen units)
    const int n = numUnits;
    const int rows = (n + (int) cols - 1) / (int) cols;
    std::vector<float> rowH ((size_t) rows, 92 * sc), rowY ((size_t) rows, 0);   // V10: a row is as tall as its tallest gear
    for (int u = 0; u < n; ++u) rowH[(size_t) (u / (int) cols)] = juce::jmax (rowH[(size_t) (u / (int) cols)], unitPlateH (u) * sc);
    float acc = 0; for (int r = 0; r < rows; ++r) { rowY[(size_t) r] = acc; acc += rowH[(size_t) r] + gapY; }
    const float pw = cols * cw + (cols - 1) * gapX + 40, ph = 44 + acc + 10;
    const R panel (W * 0.5f - pw * 0.5f, display.getY() + 60, pw, ph);
    {
        juce::Path p; p.addRoundedRectangle (panel, 14);
        g.setColour (col ("#0b0b0d", 0.3f)); g.fillPath (p);   // mostly see-through: the screen shows behind
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    }
    zone (panel, Hit::Glass);
    text (g, "ADD AN EQ", font (Face::CondMedium, 11, 1.8f), soft, R (panel.getX() + 20, panel.getY() + 14, 200, 16));
    const auto chain = proc.chain();
    const bool full = (int) chain.size() >= maxChain;
    text (g, full ? "chain full - remove one first" : "click the gear", font (Face::Regular, 10), grey, R (panel.getRight() - 220, panel.getY() + 14, 200, 16), juce::Justification::centredRight);
    for (int u = 0; u < n; ++u)
    {
        const int r = u / (int) cols, c = u % (int) cols;
        const float ch = unitPlateH (u) * sc;
        const R cell (panel.getX() + 20 + (float) c * (cw + gapX), panel.getY() + 44 + rowY[(size_t) r] + rowH[(size_t) r] - ch, cw, ch);
        const bool in = std::find (chain.begin(), chain.end(), u) != chain.end();
        const bool hv = cell.contains (mouse) && ! in && ! full;
        if (hv) { g.setColour (col (units()[(size_t) u].col).withAlpha (0.35f)); g.drawRoundedRectangle (cell.expanded (3), 5, 1.5f); }
        const auto z0 = zones.size();
        g.saveState();
        g.addTransform (juce::AffineTransform::scale (sc).translated (cell.getX(), cell.getY()));
        paintFaceplate (g, R (0, 0, 800, unitPlateH (u)), u);
        g.restoreState();
        zones.resize (z0);   // the plate's own controls don't work in here
        if (in || full) { g.setColour (col ("#0b0b0d", 0.55f)); g.fillRoundedRectangle (cell, 4); }
        text (g, juce::String (units()[(size_t) u].name) + juce::String (U8 (" \xc2\xb7 ")) + unitHint (u) + (in ? juce::String (U8 (" \xc2\xb7 in the chain")) : juce::String()),
              font (Face::Regular, 9, 0.6f), in ? col ("#6b6864") : col ("#9a958c"), R (cell.getX(), cell.getBottom() + 4, cw, 11), juce::Justification::centred);
        if (! in && ! full) zone (cell, Hit::AddPick, u);
    }
}

//==============================================================================
void Canvas::paintHeader (juce::Graphics& g)
{
    // V8: a shorter, sleeker header (44 px): everything scaled down, the screen gets the space back
    const float cy = 22.0f;
    text (g, "Tahor", font (Face::Light, 15, 3.6f), ink, R (22, 8, 100, 17));
    text (g, "EQUALIZER", font (Face::Regular, 6, 2.6f), grey, R (22, 26, 100, 8));
    float x = 22 + 66 + 8;
    for (int i = 0; i < 2; ++i)
    {
        R b (x + (float) i * 21.0f, cy - 9.5f, 19, 19);
        fillRound (g, b, 5, col ("#17181b"));
        const bool en = i == 0 ? proc.canUndo() : proc.canRedo();
        icon (g, i == 0 ? Icon::Undo : Icon::Redo, b.withSizeKeepingCentre (11, 11), en ? ink : col ("#6b6864"), 1.2f);
        zone (b, i == 0 ? Hit::Undo : Hit::Redo);
    }
    {   // V9: the history - jump back several steps
        R b (x + 42, cy - 9.5f, 19, 19);
        fillRound (g, b, 5, col ("#17181b"));
        g.setColour (proc.historyNames().size() > 1 ? ink : col ("#6b6864"));
        for (int i = 0; i < 3; ++i) g.fillRoundedRectangle (b.getX() + 5.5f, b.getY() + 6 + (float) i * 3.2f, 8.0f - (i == 2 ? 3.0f : 0.0f), 1.3f, 0.6f);
        zone (b, Hit::History);
    }
    x += 61 + 8;
    // preset: < pill >
    {
        const R lb (x, cy - 9.5f, 15, 19);
        icon (g, Icon::Left, lb.withSizeKeepingCentre (8, 8), grey, 1.3f);
        zone (lb, Hit::PresetStep, -1);
        x += 17;
        R b (x, cy - 10, 124, 20);
        fillRound (g, b, 10, col ("#141518")); strokeRound (g, b, 10, col ("#34353b"));
        icon (g, Icon::Star, R (b.getX() + 7, cy - 4.5f, 9, 9), browseOpen ? amber : grey, 1.0f);
        icon (g, Icon::Down, R (b.getRight() - 14, cy - 3.5f, 7, 7), grey);
        const auto name = proc.presetName + (proc.presetDirty ? " *" : "");
        g.setColour (ink); g.setFont (font (Face::Regular, 9.5f));
        g.drawFittedText (name, R (b.getX() + 19, cy - 6, 86, 12).toNearestInt(), juce::Justification::centred, 1, 1.0f);
        zone (b, Hit::Preset);
        zone (R (b.getX(), b.getY(), 20, b.getHeight()), Hit::Browse);   // V9: the star opens the preset browser
        presetRect = b;
        x += 124 + 2;
        const R rb (x, cy - 9.5f, 15, 19);
        icon (g, Icon::Right, rb.withSizeKeepingCentre (8, 8), grey, 1.3f);
        zone (rb, Hit::PresetStep, 1);
        x += 15 + 8;
    }
    // A > B
    {
        auto f = font (Face::CondMedium, 10.5f, 1.3f);
        const juce::String t = proc.abSlot == 0 ? "A > B" : "B > A";
        const float w = textWidth (f, t);
        text (g, t, f, col ("#b9b3a7"), R (x, cy - 7, w + 4, 14));
        zone (R (x - 4, cy - 10, w + 8, 20), Hit::AB);
        x += w + 8;
    }
    // MATCH, BYPASS
    {
        auto f = font (Face::CondMedium, 9.5f, 1.0f);
        const char* t[2] { "MATCH", "BYPASS" };
        for (int i = 0; i < 2; ++i)
        {
            const bool on = proc.param (i == 0 ? "match" : "bypass") > 0.5f;
            const float w = 6 + 5 + 4 + textWidth (f, t[i]) + 6;
            R b (x, cy - 9, w, 18);
            strokeRound (g, b, 5, on ? col ("#6b4f1f") : col ("#34353b"));
            if (on) fillRound (g, b.reduced (1), 4, col ("#3a2c14", 0.6f));
            icon (g, Icon::Dot, R (b.getX() + 6, cy - 2.5f, 5, 5), on ? amber : col ("#b9b3a7"));
            text (g, t[i], f, on ? ink : col ("#b9b3a7"), R (b.getX() + 15, cy - 6, w, 12));
            zone (b, i == 0 ? Hit::Match : Hit::Bypass);
            x += w + 4;
        }
        archTop[0] = x + 8;
    }
    // V9: the logo - "Mishkal", "by Lovy Elias", and the scale at the end
    float rx = W - 22;
    {
        auto fm = font (Face::Light, 13, 2.2f), fb = font (Face::Regular, 6.5f, 1.1f);
        const float lw = juce::jmax (textWidth (fm, "Mishkal"), textWidth (fb, "by Lovy Elias"));
        const float k = 0.62f, iw = 34 * k;
        {   // the scale
            const float sx = rx - iw, sy = cy - 11 * k - 0.5f;
            g.saveState();
            g.addTransform (juce::AffineTransform::scale (k).translated (sx, sy));
            g.setColour (ink.withAlpha (0.85f));
            g.drawLine (17, 3, 17, 19, 1.6f); g.drawLine (4, 6, 30, 6, 1.6f); g.drawLine (11, 19.5f, 23, 19.5f, 1.8f);
            juce::Path pa; pa.addTriangle (1, 13, 4, 6, 7, 13); pa.addTriangle (27, 13, 30, 6, 33, 13);
            g.strokePath (pa, juce::PathStrokeType (1.3f));
            g.setColour (amber); g.fillEllipse (17 - 2.0f, 3 - 2.0f, 4.0f, 4.0f);
            g.restoreState();
        }
        const float lx = rx - iw - 8 - lw;
        text (g, "Mishkal", fm, ink, R (lx, cy - 11, lw + 4, 15), juce::Justification::centredRight);
        text (g, "by Lovy Elias", fb, grey, R (lx, cy + 5, lw + 4, 8), juce::Justification::centredRight);
        g.setColour (col ("#26272c")); g.fillRect (lx - 11, cy - 12, 1.0f, 24.0f);
        rx = lx - 11 - 11;
    }
    // V9: AUTO GAIN, by the logo
    {
        auto f = font (Face::CondMedium, 9.5f, 1.0f);
        const bool on = proc.param ("autoGain") > 0.5f;
        const juce::String t = "AUTO GAIN";
        const float ag = proc.autoGainDb.load();
        const juce::String v = on ? (ag < -0.05f ? MINUS : juce::String ("+")) + juce::String (std::abs (ag), 1) : juce::String();
        const float w = 6 + 5 + 4 + textWidth (f, t) + (on ? 4 + textWidth (font (Face::Regular, 9), v) : 0.0f) + 6;
        R b (rx - w, cy - 9, w, 18);
        strokeRound (g, b, 5, on ? col ("#6b4f1f") : col ("#34353b"));
        if (on) fillRound (g, b.reduced (1), 4, col ("#3a2c14", 0.6f));
        icon (g, Icon::Dot, R (b.getX() + 6, cy - 2.5f, 5, 5), on ? amber : col ("#b9b3a7"));
        text (g, t, f, on ? ink : col ("#b9b3a7"), R (b.getX() + 15, cy - 6, w, 12));
        if (on) text (g, v, font (Face::Regular, 9), amber, R (b.getX() + 15 + textWidth (f, t) + 4, cy - 6, 40, 12));
        zone (b, Hit::AutoGain);
    }
    // the compact selector
    {
        auto f = font (Face::CondMedium, 10, 1.3f);
        const char* names[3] { "RESO", "TAHOR", "CHARACTER" };
        const juce::Colour cols[3] { red, ink, amber };
        const char* pw[3] { "resoOn", "tahorOn", "charOn" };
        const float labW = textWidth (f, "CHARACTER");
        const float w = 16 + 6 + 5 + 5 + labW + 7 + 9 + 6 + 16;
        const float half = juce::jmin (180.0f, W * 0.5f - archTop[0]);
        const R box (W * 0.5f + half + 20, cy - 10, w, 20);   // V9: just past the arch, in the open space
        archTop[1] = box.getX() - 8;
        const auto c = cols[tab];
        const bool power = proc.param (pw[tab]) > 0.5f;
        fillRound (g, box, 6, col ("#141518"));
        g.saveState(); { juce::Path cl; cl.addRoundedRectangle (box, 6); g.reduceClipRegion (cl); }
        g.setColour (c); g.fillRect (box.getX() + 16, box.getBottom() - 2, box.getWidth() - 32, 2.0f);
        g.setColour (col ("#26272c")); g.fillRect (box.getX() + 16, box.getY() + 4, 1.0f, 12.0f); g.fillRect (box.getRight() - 17, box.getY() + 4, 1.0f, 12.0f);
        g.restoreState();
        strokeRound (g, box, 6, col ("#34353b"));
        const R la (box.getX(), box.getY(), 16, 20), ra (box.getRight() - 16, box.getY(), 16, 20);
        const auto ac = grey.interpolatedWith (ink, 0.25f + 0.55f * pulse);   // V10: bigger, breathing gently - you can flip through
        icon (g, Icon::Left, la.withSizeKeepingCentre (12, 12), ac, 1.6f);
        icon (g, Icon::Right, ra.withSizeKeepingCentre (12, 12), ac, 1.6f);
        zone (la, Hit::TabCycle, 2); zone (ra, Hit::TabCycle, 1);
        float tx = box.getX() + 16 + 6;
        g.setColour (power ? c : c.withAlpha (0.25f)); g.fillEllipse (tx, cy - 2.5f, 5, 5);
        for (int i = 0; i < 3; ++i) { g.setColour (i == tab ? cols[i] : juce::Colours::white.withAlpha (0.18f)); g.fillEllipse (tx - 1.5f + (float) i * 3.4f, cy + 4.5f, 2.0f, 2.0f); }
        tx += 5 + 5;
        text (g, names[tab], f, ink, R (tx, cy - 6, labW + 6, 12));
        zone (R (box.getX() + 16, box.getY(), labW + 22, 20), Hit::TabCycle, 1);
        const R pr (ra.getX() - 6 - 9, cy - 4.5f, 9, 9);
        icon (g, Icon::Power, pr, power ? c : col ("#3a3936"), 1.1f);
        zone (pr.expanded (4), Hit::TabPower, tab);
    }
}

//==============================================================================
// V5: the screen - edge to edge, rounded corners, arching up into the header (between BYPASS and the selector) and
// down into the bottom bar (between its two groups)
float Canvas::archHalf() const
{
    // V7: both arches centred and the same width - up to 360, as wide as the controls either side allow
    const float c = W * 0.5f;
    return juce::jlimit (0.0f, 180.0f, juce::jmin (c - juce::jmax (archTop[0], archBot[0]), juce::jmin (archTop[1], archBot[1]) - c));
}

juce::Path Canvas::screenPath() const
{
    const float top = screenTop, bot = 764.0f, r = 14.0f, up = 8.0f, down = 794.0f, s = 30.0f;   // V8: the header is 44 px
    const float c = W * 0.5f;
    const float half = archHalf();
    const float aT[2] { c - half, c + half }, aB[2] { c - half, c + half };
    juce::Path p;
    p.startNewSubPath (0, top + r);
    p.quadraticTo (0, top, r, top);
    if (aT[1] - aT[0] > 3 * s)
    {
        p.lineTo (aT[0], top);
        p.cubicTo (aT[0] + s * 0.55f, top, aT[0] + s * 0.45f, up, aT[0] + s, up);
        p.lineTo (aT[1] - s, up);
        p.cubicTo (aT[1] - s * 0.45f, up, aT[1] - s * 0.55f, top, aT[1], top);
    }
    p.lineTo (W - r, top);
    p.quadraticTo (W, top, W, top + r);
    p.lineTo (W, bot - r);
    p.quadraticTo (W, bot, W - r, bot);
    if (aB[1] - aB[0] > 3 * s)
    {
        p.lineTo (aB[1], bot);
        p.cubicTo (aB[1] - s * 0.55f, bot, aB[1] - s * 0.45f, down, aB[1] - s, down);
        p.lineTo (aB[0] + s, down);
        p.cubicTo (aB[0] + s * 0.45f, down, aB[0] + s * 0.55f, bot, aB[0], bot);
    }
    p.lineTo (r, bot);
    p.quadraticTo (0, bot, 0, bot - r);
    p.closeSubPath();
    return p;
}

void Canvas::paintDisplay (juce::Graphics& g)
{
    const R d = display;
    g.saveState();
    const auto shape = screenPath();
    g.reduceClipRegion (shape);   // V5: the arched screen
    g.addTransform (juce::AffineTransform::translation (d.getX(), d.getY()));
    paintBackground (g);
    paintSpectrum (g);
    paintResoLayer (g);
    paintCurves (g);

    // freq labels
    {
        auto f = font (Face::Regular, 11);
        const float fs[8] { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
        const char* t[8] { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
        for (int i = 0; i < 8; ++i) text (g, t[i], f, col ("#6b6864"), R (fx (fs[i]) - 6, 670 - 8 - 14, 40, 14));
    }
    paintSoloColumn (g);   // V9: Pro-Q style - what you're hearing, top to bottom
    if (tab == 0) paintResoTab (g);
    if (tab == 1) paintTahorTab (g);
    if (tab == 2) paintCharTab (g);
    paintLegend (g);
    g.restoreState();
    // V4: the screen fades softly into the header and the bottom bar
    {
        // a soft inner fade along every edge (it follows the arches), then a faint lit rim
        g.saveState(); g.reduceClipRegion (shape);
        for (float w : { 34.0f, 20.0f, 8.0f }) { g.setColour (col ("#0d0d10", 0.12f)); g.strokePath (shape, juce::PathStrokeType (w)); }
        g.restoreState();
        // V8: no outline - the screen simply fades into the header and the bottom bar, arches and all
    }
}

// V2: a livelier display - the design's dark gradient, two slow, soft glows drifting behind, a dB grid and a vignette
void Canvas::paintBackground (juce::Graphics& g)
{
    const R d (-display.getX(), -70.0f, W, 800.0f);   // V5: under the arches too   // V3: the whole screen between the header and the bottom bar
    fillEllipseGrad (g, d, PW * 0.5f, -40, d.getWidth() * 0.5f * 1.4142f, 760 * 1.4142f, { { 0.0f, col ("#1c1826") }, { 0.7f, col ("#0f0f13") }, { 1.0f, col ("#0d0d10") } });
    const float t = drift * juce::MathConstants<float>::twoPi;
    const juce::Colour acc = tab == 0 ? col ("#b3262f") : tab == 1 ? col ("#6d5bd0") : col ("#c27a1c");
    // two big soft glows that wander very slowly (one minute round): the room breathes without pulling the eye
    fillEllipseGrad (g, d, 300 + 120 * std::sin (t), 120 + 40 * std::cos (t * 2), 520, 300,
                     { { 0.0f, col ("#6d28d9", 0.13f) }, { 0.55f, col ("#6d28d9", 0.04f) }, { 1.0f, col ("#6d28d9", 0.0f) } });
    fillEllipseGrad (g, d, PW * 0.75f - 140 * std::cos (t), 520 + 50 * std::sin (t * 2), 560, 300,
                     { { 0.0f, col ("#1d4ed8", 0.10f) }, { 0.55f, col ("#1d4ed8", 0.03f) }, { 1.0f, col ("#1d4ed8", 0.0f) } });
    fillEllipseGrad (g, d, PW * 0.5f, -40, 700, 220, { { 0.0f, acc.withAlpha (0.10f) }, { 1.0f, acc.withAlpha (0.0f) } });
    // the grid: frequencies (1-2-5 per decade, the decades a touch stronger) and dB (every 6 dB of the range)
    for (float f : { 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f,
                     2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f, 20000.0f })
    { g.setColour (juce::Colours::white.withAlpha (f == 50 || f == 200 || f == 500 || f == 2000 || f == 5000 ? 0.06f : 0.025f)); g.fillRect (std::round (fx (f)), -70.0f, 1.0f, 800.0f); }
    g.setColour (juce::Colours::white.withAlpha (0.075f));
    for (float f : { 100.0f, 1000.0f, 10000.0f }) g.fillRect (std::round (fx (f)), -70.0f, 1.0f, 800.0f);
    const float rng = dbRange (proc);
    const float step = rng <= 4.5f ? 1.0f : rng <= 9.0f ? 2.0f : rng <= 18.0f ? 6.0f : 12.0f;
    auto fl = font (Face::Regular, 10);
    for (float db = -std::floor (rng / step) * step; db <= rng; db += step)
    {
        const float y = 335.0f - db / rng * 335.0f;
        if (y < 30 || y > 640) continue;
        g.setColour (juce::Colours::white.withAlpha (std::abs (db) < 0.01f ? 0.0f : 0.035f));
        g.fillRect (d.getX(), std::round (y), d.getWidth(), 1.0f);
        if (tab != 0) text (g, (db > 0 ? "+" : db < 0 ? MINUS : juce::String()) + juce::String ((int) std::abs (db)), fl, col ("#4f4c48"),
                            R (PW - 40, y - 7, 32, 14), juce::Justification::centredRight);
    }
    // V6: a very fine grain (one speck per real pixel on Retina) so the soft glows don't step into bands
    {
        static juce::Image grain = []
        {
            juce::Image im (juce::Image::ARGB, 256, 256, true);
            juce::Random rnd (1234);
            for (int y = 0; y < 256; ++y)
                for (int x = 0; x < 256; ++x)
                {
                    const int v = rnd.nextInt (256);
                    im.setPixelAt (x, y, v < 128 ? juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, (juce::uint8) (14 + v % 10))
                                                 : juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, (juce::uint8) (5 + v % 6)));
                }
            return im;
        }();
        juce::FillType ft (grain, juce::AffineTransform::scale (0.5f));
        g.setFillType (ft);
        g.fillRect (d);
    }
    // a soft vignette at the edges
    fillEllipseGrad (g, d, d.getCentreX(), d.getCentreY(), d.getWidth() * 0.5f * 1.4142f, 345 * 1.4142f, { { 0.0f, juce::Colours::transparentBlack }, { 0.72f, juce::Colours::transparentBlack }, { 1.0f, juce::Colours::black.withAlpha (0.35f) } });
}

// V2: the analyzer, Pro-Q 4 style - smooth arches (1/6-octave smoothing, curves through the points), Pre as a deep,
// darker layer behind, Post bright in front with a lit edge
void Canvas::paintSpectrum (juce::Graphics& g)
{
    const int mode = (int) proc.param ("analyzer");
    if (mode == 3) return;
    if (anaAlpha < 0.01f) return;   // V10: nothing playing - nothing shown
    g.beginTransparencyLayer (juce::jmin (1.0f, anaAlpha));
    paintSpectrumInner (g, mode);
    g.endTransparencyLayer();
}

void Canvas::paintSpectrumInner (juce::Graphics& g, int mode)
{
    const float sr = (float) proc.getRate();
    const float binHz = sr / 8192.0f;   // V5: 8192-point FFT
    auto columns = [&] (const std::vector<float>& db, std::vector<float>& out)
    {
        const int nc = (int) (PW / 3.0f) + 1;
        out.resize ((size_t) nc);
        for (int i = 0; i < nc; ++i)
        {
            const float x = (float) i * 3.0f, hz = xf (x);
            const float lo = hz * 0.891f, hi = hz * 1.122f;   // +-1/6 octave (a third-octave window)
            const int k0 = juce::jlimit (1, 4095, (int) std::floor (lo / binHz)), k1 = juce::jlimit (k0 + 1, 4096, (int) std::ceil (hi / binHz));
            double pw = 0.0; int nb = 0;
            if (k1 - k0 <= 2)
            {
                const float kf = juce::jlimit (1.0f, 4095.0f, hz / binHz);
                const int k = (int) kf; const float t = kf - (float) k;
                pw = std::pow (10.0, (db[(size_t) k] * (1.0f - t) + db[(size_t) k + 1] * t) / 10.0); nb = 1;
            }
            else for (int k = k0; k <= k1; ++k) { pw += std::pow (10.0, db[(size_t) k] / 10.0); ++nb; }
            const float m = (float) (10.0 * std::log10 (pw / juce::jmax (1, nb) + 1.0e-14)) + 3.0f * std::log2 (hz / 1000.0f);   // +3 dB/oct
            const float y = juce::jlimit (0.0f, 670.0f, (-m - 22.0f) / 72.0f * 670.0f);
            out[(size_t) i] = out[(size_t) i] == 0.0f ? y : out[(size_t) i];
            out[(size_t) i] = y;
        }
    };
    auto shape = [] (const std::vector<float>& c, bool close)
    {
        juce::Path p;
        p.startNewSubPath (0, c[0]);
        for (size_t i = 1; i < c.size(); ++i)
        {
            const float x0 = (float) (i - 1) * 3.0f, x1 = (float) i * 3.0f;
            p.quadraticTo (x0, c[i - 1], (x0 + x1) * 0.5f, (c[i - 1] + c[i]) * 0.5f);
        }
        p.lineTo (PW, c.back());
        if (close) { p.lineTo (PW, 730); p.lineTo (0, 730); p.closeSubPath(); }   // V6: down into the bottom arch
        return p;
    };
    auto topOf = [] (const std::vector<float>& c) { float t = 670; for (float v : c) t = juce::jmin (t, v); return t; };
    if (mode == 0 || mode == 2)
    {
        columns (preDb, preCol);
        const auto p = shape (preCol, true);
        const float top = topOf (preCol);
        // the depth: Pre sits behind, a deep blue-violet that sinks into the floor
        g.setGradientFill (juce::ColourGradient (col ("#3b2f7a", mode == 2 ? 0.55f : 0.42f), 0, top, col ("#101426", 0.05f), 0, 730, false));
        g.fillPath (p);
        g.setGradientFill (juce::ColourGradient (col ("#6d5bd0", 0.28f), 0, 560, col ("#6d5bd0", 0.0f), 0, 664, false));   // V9: fades out at the bottom
        g.strokePath (shape (preCol, false), juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }
    if (mode == 0 || mode == 1)
    {
        columns (postDb, postCol);
        const auto p = shape (postCol, true);
        const float top = topOf (postCol);
        g.setGradientFill (juce::ColourGradient (col ("#8b5cf6", 0.42f), 0, top, col ("#1e3a5f", 0.10f), 0, 730, false));
        g.fillPath (p);
        const auto edge = shape (postCol, false);
        auto fade = [&g] (juce::Colour c) { g.setGradientFill (juce::ColourGradient (c, 0, 560, c.withAlpha (0.0f), 0, 664, false)); };   // V9: no flat line at the bottom
        fade (col ("#a78bfa", 0.07f)); g.strokePath (edge, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        fade (col ("#a78bfa", 0.14f)); g.strokePath (edge, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        fade (col ("#d4c9ff", 0.6f));  g.strokePath (edge, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void Canvas::paintResoLayer (juce::Graphics& g)
{
    const float op = (tab == 0 ? 1.0f : 0.4f) * (proc.param ("resoOn") > 0.5f ? 1.0f : 0.35f);   // V9: dull when off
    const float y0 = 14.0f, k = 17.0f;   // V8: the line Reso hangs from, right at the top of the graph
    // V8: under the top arch the line disappears and the reduction rises on up into the arch
    const float half = archHalf(), s = 30.0f, c = W * 0.5f - display.getX();
    const float a0 = c - half, a1 = c + half;
    const bool arch = a1 - a0 > 3 * s;
    const float ceil_ = -display.getY();   // the window's top: the arch clips it
    auto topAt = [&] (float x)
    {
        if (! arch || x <= a0 || x >= a1) return y0;
        const float t = juce::jmin (juce::jlimit (0.0f, 1.0f, (x - a0) / s), juce::jlimit (0.0f, 1.0f, (a1 - x) / s));
        const float e = t * t * (3 - 2 * t);
        return y0 + (ceil_ - y0) * e;
    };
    // the hanging line, fading out where the arch opens
    {
        auto seg = [&] (float from, float to, float fadeFrom, float fadeTo)
        {
            g.setGradientFill (juce::ColourGradient (red.withAlpha (0.45f * op), fadeFrom, y0, red.withAlpha (0.0f), fadeTo, y0, false));
            g.fillRect (from, y0, to - from, 1.0f);
        };
        if (arch) { seg (0.0f, a0 + s * 0.6f, a0, a0 + s * 0.6f); seg (a1 - s * 0.6f, PW, a1, a1 - s * 0.6f); }
        else { g.setColour (red.withAlpha (0.45f * op)); g.fillRect (0.0f, y0, PW, 1.0f); }
    }
    juce::Path curve, fill;
    float deepest = y0;
    for (int x = 0; x <= (int) PW; ++x)
    {
        const float y = y0 - resoShown[(size_t) x] * k;
        deepest = juce::jmax (deepest, y);
        if (x == 0) curve.startNewSubPath (0, y); else curve.lineTo ((float) x, y);
    }
    fill = curve;
    for (int x = (int) PW; x >= 0; x -= 2) fill.lineTo ((float) x, topAt ((float) x));
    fill.closeSubPath();
    {
        juce::ColourGradient gr (crimson.withAlpha (0.0f), 0, ceil_, crimson.withAlpha (0.7f * op), 0, juce::jmax (deepest, y0 + 6), false);
        gr.addColour ((y0 - ceil_) / (juce::jmax (deepest, y0 + 6) - ceil_), crimson.withAlpha (0.14f * op));
        g.setGradientFill (gr);
        g.fillPath (fill);
    }
    g.setColour (crimsonLine.withAlpha (op));
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
}

void Canvas::paintCurves (juce::Graphics& g)
{
    const float rng = dbRange (proc);
    auto yOf = [rng] (float db) { return 335.0f - juce::jlimit (-rng * 0.95f, rng * 0.95f, db) / rng * 335.0f; };
    // zero line
    if (tab != 0) { g.setColour (juce::Colours::white.withAlpha (0.16f)); g.fillRect (0.0f, 335.0f, PW, 1.0f); }
    // Tahor's curve
    if (tab != 0)
    {
        const auto bands = proc.tahorBands();
        juce::Path p;
        for (int x = 0; x <= (int) PW; ++x)   // V5: every pixel
        {
            const float hz = xf ((float) x);
            double db = 0.0;
            for (int b = 0; b < numBands; ++b)
            {
                const auto& bd = bands[(size_t) b];
                if (! bd.on) continue;
                db += vox::tahorAnalogBandDb (bd, bd.freq, bd.gain, bd.q, hz);   // V10: the set gain (solid); the live movement is drawn apart
            }
            const float y = yOf ((float) db);
            if (x == 0) p.startNewSubPath (0, y); else p.lineTo ((float) x, y);
        }
        const bool on = proc.param ("tahorOn") > 0.5f;
        bool split = false;
        for (int b = 0; b < numBands; ++b) if (bands[(size_t) b].on && bands[(size_t) b].place >= 3) split = true;
        if (! split)
        {
            g.setColour (juce::Colours::white.withAlpha ((tab == 1 ? 1.0f : 0.45f) * (on ? 1.0f : 0.35f)));
            g.strokePath (p, juce::PathStrokeType (tab == 1 ? 3.0f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {   // V8: left and right drawn apart, each in its own tint, so you see both sides as you move them
            const juce::Colour lc = col ("#7fd3ff"), rc = col ("#ff8fb8");
            for (int side = 0; side < 2; ++side)
            {
                juce::Path q;
                for (int x = 0; x <= (int) PW; ++x)
                {
                    const float hz = xf ((float) x);
                    double db = 0.0;
                    for (int b = 0; b < numBands; ++b)
                    {
                        const auto& bd = bands[(size_t) b];
                        if (! bd.on || bd.place == (side == 0 ? 4 : 3)) continue;
                        db += vox::tahorAnalogBandDb (bd, bd.freq, bd.gain, bd.q, hz);
                    }
                    const float y = yOf ((float) db);
                    if (x == 0) q.startNewSubPath (0, y); else q.lineTo ((float) x, y);
                }
                const auto cc = side == 0 ? lc : rc;
                const float a = (tab == 1 ? 1.0f : 0.45f) * (on ? 1.0f : 0.35f);
                g.setColour (cc.withAlpha (0.18f * a)); g.strokePath (q, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (cc.withAlpha (a)); g.strokePath (q, juce::PathStrokeType (tab == 1 ? 2.4f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                if (tab == 1)
                {
                    const float ye = juce::jlimit (30.0f, 600.0f, q.getCurrentPosition().y);
                    fillRound (g, R (PW - 46, ye - (side == 0 ? 20.0f : -4.0f), 16, 14), 4, col ("#0e0e10", 0.85f));
                    text (g, side == 0 ? "L" : "R", font (Face::SemiBold, 10), cc, R (PW - 46, ye - (side == 0 ? 20.0f : -4.0f), 16, 14), juce::Justification::centred);
                }
            }
        }
    }
    // V10: Pro-Q style - each dynamic band's live movement, pulling (or pushing) from the curve it sits on
    if (tab != 0)
    {
        const auto bands = proc.tahorBands();
        bool split = false;
        for (auto& b : bands) if (b.on && b.place >= 3) split = true;
        for (int bi = 0; bi < numBands; ++bi)
        {
            const auto& bd = bands[(size_t) bi];
            const float dd = proc.dynDb[(size_t) bi].load();
            if (! bd.on || ! bd.dyn || ! bd.hasGain()) continue;
            const float rgDb = bd.dynRange;   // V11: the furthest it may move - drawn as a dashed outline
            const int side = ! split ? -1 : (bd.place == 4 ? 1 : 0);
            const float lo = bd.freq / 8.0f, hi = bd.freq * 8.0f;
            const int x0 = juce::jmax (0, (int) fx (lo)), x1 = juce::jmin ((int) PW, (int) fx (hi));
            juce::Path top, fill;
            std::vector<float> ys, yd, yr;
            for (int x = x0; x <= x1; ++x)
            {
                const float hz = xf ((float) x);
                double st = 0.0;
                for (int b = 0; b < numBands; ++b)
                {
                    const auto& o = bands[(size_t) b];
                    if (! o.on || (side == 0 && o.place == 4) || (side == 1 && o.place == 3)) continue;
                    st += vox::tahorAnalogBandDb (o, o.freq, o.gain, o.q, hz);
                }
                const double own = vox::tahorAnalogBandDb (bd, bd.freq, bd.gain, bd.q, hz);
                const double mv = st - own + vox::tahorAnalogBandDb (bd, bd.freq, bd.gain + dd, bd.q, hz);
                const double mr = st - own + vox::tahorAnalogBandDb (bd, bd.freq, bd.gain + rgDb, bd.q, hz);
                ys.push_back (yOf ((float) st)); yd.push_back (yOf ((float) mv)); yr.push_back (yOf ((float) mr));
            }
            if (ys.size() < 2) continue;
            fill.startNewSubPath ((float) x0, ys[0]);
            for (size_t i = 1; i < ys.size(); ++i) fill.lineTo ((float) x0 + (float) i, ys[i]);
            for (size_t i = yd.size(); i-- > 0;) fill.lineTo ((float) x0 + (float) i, yd[i]);
            fill.closeSubPath();
            top.startNewSubPath ((float) x0, yd[0]);
            for (size_t i = 1; i < yd.size(); ++i) top.lineTo ((float) x0 + (float) i, yd[i]);
            const auto c = bandColour (bi);
            const float a = tab == 1 ? 1.0f : 0.5f;
            {   // the range: a faint dashed outline and a whisper of fill - the room it has to work in
                juce::Path rp, rf;
                rp.startNewSubPath ((float) x0, yr[0]);
                for (size_t i = 1; i < yr.size(); ++i) rp.lineTo ((float) x0 + (float) i, yr[i]);
                rf.startNewSubPath ((float) x0, ys[0]);
                for (size_t i = 1; i < ys.size(); ++i) rf.lineTo ((float) x0 + (float) i, ys[i]);
                for (size_t i = yr.size(); i-- > 0;) rf.lineTo ((float) x0 + (float) i, yr[i]);
                rf.closeSubPath();
                g.setColour (c.withAlpha (0.06f * a)); g.fillPath (rf);
                juce::Path dashed; const float dl[2] { 4.0f, 4.0f };
                juce::PathStrokeType (1.0f).createDashedStroke (dashed, rp, dl, 2);
                g.setColour (c.withAlpha (0.45f * a)); g.fillPath (dashed);
            }
            if (std::abs (dd) < 0.05f) continue;
            g.setColour (c.withAlpha (0.22f * a)); g.fillPath (fill);
            g.setColour (c.withAlpha (0.7f * a)); g.strokePath (top, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved));
        }
    }
    if (tab != 2) return;

    // CHARACTER: one neon line per unit + the bright combined line
    const int NP = (int) PW;   // V5: a point per pixel
    const auto ch = proc.chain();
    const double sr = juce::jmax (44100.0, proc.getRate());
    std::vector<std::vector<float>> per;
    auto curveOf = [&] (int u)
    {
        float v[unitValues]; proc.getUnitValues (u, v);
        const auto d = vox::VocalEQ::design (unitSettings (u, v), units()[(size_t) u].type, sr);
        std::vector<float> c ((size_t) NP + 1);
        for (int i = 0; i <= NP; ++i) c[(size_t) i] = vox::VocalEQ::magnitudeDb (d, xf ((float) i), sr);
        return c;
    };
    auto pathOf = [&] (const std::vector<float>& c)
    {
        juce::Path p;
        for (int i = 0; i <= NP; ++i) { const float y = yOf (c[(size_t) i]); if (i == 0) p.startNewSubPath (0, y); else p.lineTo ((float) i, y); }
        return p;
    };
    auto neon = [&] (const juce::Path& p, juce::Colour c, float w, float op)
    {
        g.setColour (c.withAlpha (op * 0.12f)); g.strokePath (p, juce::PathStrokeType (w * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (op * 0.16f)); g.strokePath (p, juce::PathStrokeType (w * 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (op));         g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    std::vector<float> sum ((size_t) NP + 1, 0.0f), sumR ((size_t) NP + 1, 0.0f);
    bool anySplit = false;
    std::vector<std::vector<float>> perR;   // V9: a split unit's right side
    auto curveOfSide = [&] (int u, int side)
    {
        float v[unitValues]; proc.getUnitValues (u, v, side);
        const auto d = vox::VocalEQ::design (unitSettings (u, v), units()[(size_t) u].type, sr);
        std::vector<float> c ((size_t) NP + 1);
        for (int i = 0; i <= NP; ++i) c[(size_t) i] = vox::VocalEQ::magnitudeDb (d, xf ((float) i), sr);
        return c;
    };
    for (int u : ch)
    {
        per.push_back (curveOf (u));
        const bool sp = proc.unitSplit (u);
        anySplit = anySplit || sp;
        perR.push_back (sp ? curveOfSide (u, 1) : per.back());
        for (int i = 0; i <= NP; ++i) { sum[(size_t) i] += per.back()[(size_t) i]; sumR[(size_t) i] += perR.back()[(size_t) i]; }
    }
    // the fill under the combined line
    {
        auto p = pathOf (sum);
        p.lineTo (PW, 335); p.lineTo (0, 335); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col ("#fff4dc", 0.16f), 0, 0, col ("#fff4dc", 0.03f), 0, 670, false));
        g.fillPath (p);
    }
    const bool on = proc.param ("charOn") > 0.5f;
    // V6: each unit's own coloured shadow, between its curve and the zero line (Pro-Q 4 style)
    for (size_t i = 0; i < ch.size(); ++i)
    {
        const auto c = col (units()[(size_t) ch[i]].col);
        auto p = pathOf (per[i]);
        p.lineTo (PW, 335); p.lineTo (0, 335); p.closeSubPath();
        float mn = 335, mx = 335;
        for (float v : per[i]) { mn = juce::jmin (mn, yOf (v)); mx = juce::jmax (mx, yOf (v)); }
        const float a = (hoverUnit == ch[i] ? 0.30f : 0.18f) * (on ? 1.0f : 0.4f);
        juce::ColourGradient gr (c.withAlpha (a), 0, mn, c.withAlpha (a * 0.15f), 0, 335, false);
        if (mx > 336) { gr = juce::ColourGradient (c.withAlpha (a * 0.15f), 0, 335, c.withAlpha (a), 0, mx, false); if (mn < 334) { gr = juce::ColourGradient (c.withAlpha (a), 0, mn, c.withAlpha (a), 0, mx, false); gr.addColour ((335.0 - mn) / juce::jmax (1.0, (double) (mx - mn)), c.withAlpha (a * 0.15f)); } }
        g.setGradientFill (gr); g.fillPath (p);
    }
    for (size_t i = 0; i < ch.size(); ++i)
    {
        const int u = ch[i];
        neon (pathOf (per[i]), col (units()[(size_t) u].col), hoverUnit == u ? 2.6f : 2.0f, (hoverUnit == u ? 1.0f : 0.5f) * (on ? 1.0f : 0.4f));
    }
    for (size_t i = 0; i < ch.size(); ++i)   // V9: a split unit's right side, fainter
        if (proc.unitSplit (ch[i]))
        {
            const int u = ch[i];
            neon (pathOf (perR[i]), col (units()[(size_t) u].col).interpolatedWith (col ("#ff8fb8"), 0.4f), hoverUnit == u ? 2.2f : 1.6f, 0.45f * (on ? 1.0f : 0.4f));
        }
    if (std::find (ch.begin(), ch.end(), preview) == ch.end()) neon (pathOf (curveOf (preview)), col (units()[(size_t) preview].col), 1.5f, 0.45f);
    if (! anySplit) neon (pathOf (sum), col ("#fff4dc"), 3.5f, on ? 1.0f : 0.4f);
    else
    {   // V9: left and right combined, apart
        neon (pathOf (sum), col ("#cfeeff"), 3.0f, on ? 1.0f : 0.4f);
        neon (pathOf (sumR), col ("#ffd6e6"), 3.0f, on ? 1.0f : 0.4f);
        const float yl = juce::jlimit (30.0f, 600.0f, yOf (sum.back())), yr = juce::jlimit (30.0f, 600.0f, yOf (sumR.back()));
        fillRound (g, R (PW - 46, yl - 20, 16, 14), 4, col ("#0e0e10", 0.85f)); text (g, "L", font (Face::SemiBold, 10), col ("#7fd3ff"), R (PW - 46, yl - 20, 16, 14), juce::Justification::centred);
        fillRound (g, R (PW - 46, yr + 4, 16, 14), 4, col ("#0e0e10", 0.85f)); text (g, "R", font (Face::SemiBold, 10), col ("#ff8fb8"), R (PW - 46, yr + 4, 16, 14), juce::Justification::centred);
    }

    // peak dots: each unit's biggest boost (the number only on hover)
    for (size_t i = 0; i < ch.size(); ++i)
    {
        const int u = ch[i];
        float best = 0.0f; int bi = 250;
        for (int k = 0; k <= NP; ++k) if (per[i][(size_t) k] > best) { best = per[i][(size_t) k]; bi = k; }
        if (best <= 0.05f) continue;
        const float x = juce::jmin (PW - 40.0f, (float) bi), y = yOf (best);
        const bool hv = hoverUnit == u;
        const auto c = col (units()[(size_t) u].col);
        const float s = hv ? 10.0f : 8.0f;
        g.setColour (c.withAlpha (0.35f)); g.fillEllipse (x - s * 0.5f - (hv ? 6 : 3), y - s * 0.5f - (hv ? 6 : 3), s + (hv ? 12 : 6), s + (hv ? 12 : 6));
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (x - s * 0.5f - 2, y - s * 0.5f - 2, s + 4, s + 4);
        g.setColour (c.withAlpha (hv ? 1.0f : 0.75f)); g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
        zone (R (x - 8, y - 8, 16, 16).translated (display.getX(), display.getY()), Hit::PeakDot, u);
        if (hv)
        {
            const auto t = "+" + juce::String (best, 1) + " dB";
            auto f = font (Face::Medium, 11);
            const float w = textWidth (f, t) + 14;
            R b (x + 10, y - 26, w, 18);
            fillRound (g, b, 9, col ("#0e0e10", 0.85f)); strokeRound (g, b, 9, c.withAlpha ((float) 0x66 / 255.0f));
            text (g, t, f, c, b, juce::Justification::centred);
        }
    }
}

void Canvas::paintLegend (juce::Graphics& g)
{
    g.saveState(); g.addTransform (juce::AffineTransform::translation (0.0f, -22.0f));   // V8: up into the space the header gave back
    struct L { int kind; juce::Colour c; juce::String t; };   // kind 0 = block (8 tall), 1 = line 3, 2 = line 2
    std::vector<L> items;
    if (tab == 0) items.push_back ({ 0, red.withAlpha (0.7f), "What Reso removes" });
    else if (tab == 1) { items.push_back ({ 1, juce::Colours::white, "Tahor" }); items.push_back ({ 0, red.withAlpha (0.4f), "Reso (view only)" }); }
    else
    {
        items.push_back ({ 1, col ("#fff4dc"), "Character (combined)" });
        for (int u : proc.chain()) items.push_back ({ 2, col (units()[(size_t) u].col), units()[(size_t) u].name });
        items.push_back ({ 2, juce::Colours::white.withAlpha (0.45f), "Tahor" });
        items.push_back ({ 0, red.withAlpha (0.4f), "Reso" });
    }
    auto f = font (Face::Regular, 10);   // V4: much smaller
    float x = 16;
    for (auto& it : items)
    {
        const float h = it.kind == 0 ? 8.0f : it.kind == 1 ? 3.0f : 2.0f;
        const R sw (x, 7 + 6.0f - h * 0.5f, 13, h * 0.8f);
        if (it.kind != 0 || tab == 0) { g.setColour (it.c.withAlpha (it.c.getFloatAlpha() * 0.5f)); g.fillRoundedRectangle (sw.expanded (2), 2); }
        g.setColour (it.c); g.fillRoundedRectangle (sw, it.kind == 0 ? 0.0f : h * 0.5f);
        text (g, it.t, f, col ("#9a958c"), R (x + 18, 7, 300, 12));
        x += 18 + textWidth (f, it.t) + 12;
    }
    // V5: Clean / Subtle / Warm in the top row (Character only), small to match
    if (tab == 2)
    {
        auto fm = font (Face::CondMedium, 10, 1.2f);
        const char* modes[3] { "CLEAN", "SUBTLE", "WARM" };
        float mw[3], tot = 0;
        for (int i = 0; i < 3; ++i) { mw[i] = textWidth (fm, modes[i]) + 16; tot += mw[i]; }
        const R seg (x + 6, 4, tot, 18);
        fillRound (g, seg, 9, col ("#17181b", 0.9f)); strokeRound (g, seg, 9, juce::Colours::white.withAlpha (0.12f));
        const int mode = (int) proc.param ("charMode");
        float sx = seg.getX();
        for (int i = 0; i < 3; ++i)
        {
            const R c (sx, 4, mw[i], 18);
            if (mode == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg.reduced (1), 8); g.reduceClipRegion (cp); g.setColour (col ("#3a2c14", 0.95f)); g.fillRect (c); g.restoreState(); }
            text (g, modes[i], fm, mode == i ? amber : col ("#b9b3a7"), c, juce::Justification::centred);
            zone (c.translated (display.getX(), display.getY() - 22.0f), Hit::CharMode, i);
            sx += mw[i];
        }
        // V7: THD - a slim slider; the fill glows brighter amber the further it goes
        {
            const float v = juce::jlimit (0.0f, 1.0f, proc.param ("charThd") / 100.0f);
            float tx = seg.getRight() + 14;
            text (g, "THD", fm, col ("#b9b3a7"), R (tx, 4, 30, 18));
            tx += textWidth (fm, "THD") + 8;
            const R tr (tx, 11.5f, 96, 3);
            fillRound (g, tr, 1.5f, juce::Colours::white.withAlpha (0.1f));
            const auto glow = col ("#7a5418").interpolatedWith (col ("#ffcf7a"), v);
            if (v > 0.001f)
            {
                const R on = tr.withWidth (tr.getWidth() * v);
                g.setColour (glow.withAlpha (0.12f + 0.3f * v)); g.fillRoundedRectangle (on.expanded (1.5f + 2.5f * v), 3.5f);
                fillRound (g, on, 1.5f, glow);
            }
            const float kx = tr.getX() + tr.getWidth() * v;
            g.setColour (glow.withAlpha (0.25f * v)); g.fillEllipse (kx - 8, 13 - 8, 16, 16);
            g.setColour (col ("#f4ede0")); g.fillEllipse (kx - 4.5f, 13 - 4.5f, 9, 9);
            g.setColour (glow); g.fillEllipse (kx - 2, 13 - 2, 4, 4);
            text (g, juce::String ((int) std::round (v * 100.0f)) + "%", font (Face::Regular, 10), v > 0.5f ? amber : col ("#b9b3a7"), R (tr.getRight() + 10, 4, 40, 18));
            zone (R (tr.getX() - 6, 2, tr.getWidth() + 12, 22).translated (display.getX(), display.getY() - 22.0f), Hit::Thd);
        }
    }
    const juce::String hint = tab == 0 ? U8 ("drag the red edges to focus \xc2\xb7 dots to push or relax")
                            : tab == 1 ? U8 ("double-click to add a band \xc2\xb7 drag across to sketch")
                                       : U8 ("each unit draws its own colour \xc2\xb7 the bright line is the combined sound");
    {
        const float hiX = tab == 0 ? fx (proc.param ("rsHigh")) - 12.0f : PW - 16 - 26;   // V8: Reso's hint stays in the lit part
        const float hw = textWidth (f, hint) + 4;
        text (g, hint, f, col ("#77746e"), R (hiX - hw, 7, hw, 12), juce::Justification::centredRight);
    }
    // V2: full screen
    {
        const R b (PW - 16 - 18, 4, 18, 18);
        const bool hv = b.translated (display.getX(), display.getY() - 22.0f).contains (mouse);
        fillRound (g, b, 5, col ("#17181b", hv ? 0.95f : 0.7f)); strokeRound (g, b, 5, juce::Colours::white.withAlpha (hv ? 0.3f : 0.14f));
        g.setColour (hv ? ink : grey);
        const float c = 5.0f, l = 3.5f;
        const R r = b.reduced (4.5f);
        juce::Path pth;
        auto corner = [&] (float x, float y, float sx, float sy) { pth.startNewSubPath (x + sx * l, y); pth.lineTo (x, y); pth.lineTo (x, y + sy * l); };
        if (! fullScreen) { corner (r.getX(), r.getY(), 1, 1); corner (r.getRight(), r.getY(), -1, 1); corner (r.getX(), r.getBottom(), 1, -1); corner (r.getRight(), r.getBottom(), -1, -1); }
        else { const float m = 4.0f; corner (r.getX() + m, r.getY() + m, -1, -1); corner (r.getRight() - m, r.getY() + m, 1, -1); corner (r.getX() + m, r.getBottom() - m, -1, 1); corner (r.getRight() - m, r.getBottom() - m, 1, 1); }
        juce::ignoreUnused (c);
        g.strokePath (pth, juce::PathStrokeType (1.4f, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
        zone (b.translated (display.getX(), display.getY() - 22.0f), Hit::FullScreen);
    }
    g.restoreState();
}

//==============================================================================
// the 52 px knob of Reso: a bold silver ring, a dark disc, a small white dot
void Canvas::neonKnob (juce::Graphics& g, P tl, float pos, float size)
{
    if (std::abs (size - 52.0f) > 0.01f)   // V5: drawn as vectors at any size (crisp)
    {
        g.saveState();
        g.addTransform (juce::AffineTransform::scale (size / 52.0f).translated (tl.x, tl.y));
        neonKnob (g, { 0, 0 }, pos, 52.0f);
        g.restoreState();
        return;
    }
    const auto c = tl + P (26, 26);
    const float a = 300.0f * juce::jlimit (0.0f, 1.0f, pos);
    const float a0 = 210.0f * pi / 180.0f;
    const float rMid = 24.9f, w = 2.6f;
    juce::Path track; track.addCentredArc (c.x, c.y, rMid, rMid, 0.0f, a0 + a * pi / 180.0f, a0 + 300.0f * pi / 180.0f, true);
    g.setColour (juce::Colours::white.withAlpha (0.14f)); g.strokePath (track, juce::PathStrokeType (w));
    if (a > 0.5f)
    {
        juce::Path arc; arc.addCentredArc (c.x, c.y, rMid, rMid, 0.0f, a0, a0 + a * pi / 180.0f, true);
        g.setColour (col ("#eef0f2")); g.strokePath (arc, juce::PathStrokeType (w));
    }
    // the disc (inset 11): shadow, radial body, a faint top highlight
    const R disc (tl.x + 11, tl.y + 11, 30, 30);
    { juce::Path p; p.addEllipse (disc.translated (0, 2)); juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 4, {}).drawForPath (g, p); }
    g.setGradientFill (juce::ColourGradient (col ("#34343a"), disc.getX() + 0.45f * 30, disc.getY() + 0.35f * 30, col ("#222226"),
                                             disc.getX() + 0.45f * 30 + 30 * 0.7f * 0.95f, disc.getY() + 0.35f * 30, true));
    g.fillEllipse (disc);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawEllipse (disc.reduced (0.5f).translated (0, 0.5f), 1.0f);
    const float d = (a - 150.0f) * pi / 180.0f;
    g.setColour (col ("#fff1e2"));
    g.fillEllipse (c.x + 18.0f * std::sin (d) - 3, c.y - 18.0f * std::cos (d) - 3, 6, 6);
}

// the Tahor pop-up knob: a coloured pie (conic from 225, 270 degrees) with a turning disc and pointer
void Canvas::bandKnob (juce::Graphics& g, R b, float pos, juce::Colour c)
{
    const float s = b.getWidth(), a = 270.0f * juce::jlimit (0.0f, 1.0f, pos);
    const auto ctr = b.getCentre();
    const float a0 = 225.0f * pi / 180.0f;
    juce::Path rest; rest.addPieSegment (b, a0 + a * pi / 180.0f, a0 + 270.0f * pi / 180.0f, 0.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillPath (rest);
    if (a > 0.3f) { juce::Path on; on.addPieSegment (b, a0, a0 + a * pi / 180.0f, 0.0f); g.setColour (c); g.fillPath (on); }
    const float i = std::round (s * 0.1f);
    const R disc = b.reduced (i);
    { juce::Path p; p.addEllipse (disc.translated (0, 2)); juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 4, {}).drawForPath (g, p); }
    g.setGradientFill (juce::ColourGradient (col ("#2c2d33"), disc.getX() + disc.getWidth() * 0.4f, disc.getY() + disc.getHeight() * 0.35f,
                                             col ("#141518"), disc.getX() + disc.getWidth() * 0.4f + disc.getWidth() * 0.7f * 0.9f, disc.getY() + disc.getHeight() * 0.35f, true));
    g.fillEllipse (disc);
    const float rot = (a - 135.0f) * pi / 180.0f;
    juce::Path ptr; ptr.addRoundedRectangle (-1.0f, -disc.getHeight() * 0.5f + 3.0f, 2.0f, std::round (s * 0.24f), 1.0f);
    g.setColour (col ("#f4ede0"));
    g.fillPath (ptr, juce::AffineTransform::rotation (rot).translated (ctr.x, ctr.y));
}

//==============================================================================
void Canvas::paintResoTab (juce::Graphics& g)
{
    // V9: Reso switched off - its whole screen goes dull, like Tahor and Character
    const bool resoOff = proc.param ("resoOn") < 0.5f;
    if (resoOff) g.beginTransparencyLayer (0.4f);
    paintResoTabInner (g);
    if (resoOff) g.endTransparencyLayer();
}

void Canvas::paintResoTabInner (juce::Graphics& g)
{
    const float lo = fx (proc.param ("rsLow")), hi = fx (proc.param ("rsHigh"));
    // the focus range: dark outside, red edges
    g.setColour (col ("#000408", 0.55f));
    g.fillRect (0.0f, -70.0f, lo + 1.0f, 800.0f);   // V8: from the very top (the header is shorter)
    g.fillRect (hi + 1.0f, -70.0f, PW - hi, 800.0f);
    g.setColour (crimson.withAlpha (0.5f)); g.fillRect (lo - 1.0f, -70.0f, 4.0f, 800.0f); g.fillRect (hi - 1.0f, -70.0f, 4.0f, 800.0f);
    g.setColour (crimson); g.fillRect (lo, -70.0f, 2.0f, 800.0f); g.fillRect (hi, -70.0f, 2.0f, 800.0f);
    const P o (display.getX(), display.getY());
    zone (R (lo - 5, 30, 12, 600).translated (o.x, o.y), Hit::FocusEdge, 0);
    zone (R (hi - 5, 30, 12, 600).translated (o.x, o.y), Hit::FocusEdge, 1);

    // the baseline of the shaping bands
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        const float dashes[2] { 2.0f, 6.0f };
        g.drawDashedLine (juce::Line<float> (lo + 2, 330.5f, hi, 330.5f), dashes, 2, 1.0f);
    }
    zone (R (0, 0, PW, 566).translated (o.x, o.y), Hit::ResoBg);   // double-click: a new band

    const auto rs = proc.resoSettings();
    if (! rs.nodes[(size_t) juce::jlimit (0, maxResoNodes - 1, selNode)].on)
        for (int i = 0; i < maxResoNodes; ++i) if (rs.nodes[(size_t) i].on) { selNode = i; break; }
    if (resoSolo >= 0 && ! rs.nodes[(size_t) resoSolo].on) resoSolo = -1;
    if (resoSolo >= 0) { soloHzUi = rs.nodes[(size_t) resoSolo].hz; soloQUi = juce::jlimit (0.3f, 8.0f, rs.nodes[(size_t) resoSolo].q * 1.4f); }
    // the selected band's shape
    const auto& sn = rs.nodes[(size_t) selNode];
    if (sn.on && reveal > 0.01f)
    {
        g.beginTransparencyLayer (reveal);   // V3: the band's shape only while the tab is showing
        juce::Path p; bool started = false;
        const bool isMask = sn.shape >= 5;
        for (int x = (int) lo + 2; x <= (int) hi; x += 2)
        {
            const float f = xf ((float) x);
            float db = isMask ? -(1.0f - nodeMask (sn, f)) * 4.0f : nodeDb (sn, f);
            const float y = 330.0f - db * 40.0f;
            if (! started) { p.startNewSubPath ((float) x, 330.0f); started = true; }
            p.lineTo ((float) x, y);
        }
        p.lineTo (hi, 330); p.closeSubPath();
        const auto sh = resoShade (sn.shape);
        g.setColour (sh.withAlpha (0.12f)); g.fillPath (p);
        g.setColour (sh.withAlpha (0.9f)); g.strokePath (p, juce::PathStrokeType (2.0f));
        g.endTransparencyLayer();
    }
    // the dots: faint until touched; the touched one lights up with its label
    const char* shapeNames[8] { "Bell", "Bandpass", "Low shelf", "High shelf", "Tilt", "Notch", "Low cut", "High cut" };
    for (int i = 0; i < maxResoNodes; ++i)
    {
        const auto& n = rs.nodes[(size_t) i];
        if (! n.on) continue;
        const float x = fx (n.hz), y = 330.0f - n.amount * 40.0f;
        if (i == selNode)
        {
            const auto red = resoShade (n.shape);
            g.setColour (red.withAlpha (0.2f)); g.fillEllipse (x - 21, y - 21, 42, 42);
            { juce::Path p; p.addEllipse (x - 14, y - 12, 28, 28); juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 8, {}).drawForPath (g, p); }
            g.setColour (juce::Colours::white); g.fillEllipse (x - 14, y - 14, 28, 28);
            g.setColour (red); g.fillEllipse (x - 11, y - 11, 22, 22);
            const juce::String t = juce::String (i + 1) + DOT + shapeNames[n.shape] + DOT + hzText (n.hz) + DOT + num (n.amount) + " dB";   // V11
            auto f = font (Face::Regular, 12);
            R b (x - 34, y - 52, textWidth (f, t) + 20, 22.4f);
            if (reveal > 0.01f)
            {
                g.beginTransparencyLayer (reveal);
                fillRound (g, b, 12, col ("#0e0e10", 0.9f)); strokeRound (g, b, 12, red.withAlpha (0.6f));
                text (g, t, f, juce::Colours::white, b.reduced (10, 0));
                g.endTransparencyLayer();
            }
        }
        else
        {
            g.setColour (resoShade (n.shape).withAlpha (0.4f)); g.fillEllipse (x - 6, y - 6, 12, 12);
            g.setColour (juce::Colours::white.withAlpha (0.3f)); g.drawEllipse (x - 5.5f, y - 5.5f, 11, 11, 1.0f);
        }
        if (const int pl = (int) proc.param (nid (i, "Place")); pl != 0)   // V9: the side it works on
        {
            const R tg (x + 9, y - 22, 12, 11);
            fillRound (g, tg, 3, pl == 1 ? col ("#7fd3ff") : col ("#ff8fb8"));
            text (g, pl == 1 ? "L" : "R", font (Face::SemiBold, 8.5f), col ("#101014"), tg, juce::Justification::centred);
        }
        zone (R (x - 14, y - 14, 28, 28).translated (o.x, o.y), Hit::ResoNode, i);
    }
    // V6: the band panel - Amount and Width on top, the shape dropdown under them (Pro-Q 4 style); at the bottom,
    // sliding along to sit under the band you touch
    if (sn.on)
    {
        const float pw = 300, ph = 134, k = 0.84f;
        const auto sh = resoShade (sn.shape);
        const float nx = fx (sn.hz);
        const float px = juce::jlimit (8.0f, PW - 8.0f - pw * k, nx - pw * k * 0.5f), py = 662.0f - ph * k;
        const auto tr = juce::AffineTransform::scale (k).translated (px, py);
        const R b (0, 0, pw, ph);
        const R bs = R (px, py, pw * k, ph * k).translated (o.x, o.y);
        if (reveal > 0.05f) revealRects.push_back (bs);
        const auto z0 = zones.size();
        g.beginTransparencyLayer (reveal);
        g.saveState(); g.addTransform (tr);
        bandPanel (g, b, (nx - px) / k, sh);
        g.setColour (sh); g.fillEllipse (14, 16, 8, 8);
        text (g, "Band " + juce::String (selNode + 1), font (Face::Regular, 13), soft, R (28, 12, 200, 16));
        {   // V9: which side it works on (L / R makes a partner on the other side), and Solo
            const int pl = (int) proc.param (nid (selNode, "Place"));
            const char* ps[3] { "ST", "L", "R" };
            const R seg (pw - 14 - 30 - 8 - 90, 10, 90, 20);
            strokeRound (g, seg, 5, col ("#34353b"));
            for (int i = 0; i < 3; ++i)
            {
                const R cc (seg.getX() + (float) i * 30, seg.getY(), 30, 20);
                if (pl == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg.reduced (1), 4); g.reduceClipRegion (cp); g.setColour (col ("#2b2c31")); g.fillRect (cc); g.restoreState(); }
                text (g, ps[i], font (Face::Regular, 11), pl == i ? (i == 1 ? col ("#7fd3ff") : i == 2 ? col ("#ff8fb8") : soft) : col ("#8b8984"), cc, juce::Justification::centred);
                zone (cc.translated (o.x, o.y), Hit::ResoPlace, i);
            }
            const bool so = resoSolo == selNode;
            const R sb (pw - 14 - 30, 8, 30, 24);
            fillRound (g, sb, 6, so ? col ("#3a1418") : col ("#232327")); strokeRound (g, sb, 6, so ? sh : col ("#34353b"));
            text (g, "S", font (Face::SemiBold, 12), so ? sh : col ("#8b8984"), sb, juce::Justification::centred);
            zone (sb.translated (o.x, o.y), Hit::BandSolo, 2);
        }
        const juce::String vals[2] { num (sn.amount) + " dB", sn.shape == 4 ? juce::String (sn.q < 0.8f ? "wide" : sn.q < 2.0f ? "medium" : "narrow") : "Q " + juce::String (sn.q, 1) };
        const juce::String labs[2] { "Amount", "Width" };
        auto fl = font (Face::Regular, 12), fv = font (Face::Regular, 15);
        float x = 14;
        for (int i = 0; i < 2; ++i)
        {
            auto* p = proc.apvts.getParameter (nid (selNode, i == 0 ? "Amt" : "Q"));
            neonKnob (g, { x, 36 }, p->getValue());
            text (g, labs[i], fl, soft, R (x + 60, 44, 80, 15));
            text (g, vals[i], fv, soft, R (x + 60, 62, 80, 19));
            zone (R (x, 36, 52, 52).translated (o.x, o.y), Hit::ResoNodeKnob, i);
            x += 52 + 8 + 80 + 6;
        }
        // the shape dropdown
        {
            const char* shapeNames[8] { "Bell", "Bandpass", "Low shelf", "High shelf", "Tilt", "Notch", "Low cut", "High cut" };
            const R dd (14, 98, pw - 28, 24);
            fillRound (g, dd, 6, col ("#232327")); strokeRound (g, dd, 6, col ("#34353b"));
            shapeIcon (g, sn.shape, R (dd.getX() + 10, dd.getY() + 6, 22, 12), sh);
            text (g, shapeNames[sn.shape], font (Face::Regular, 12), soft, R (dd.getX() + 40, dd.getY(), 200, 24));
            icon (g, Icon::TriDown, R (dd.getRight() - 18, dd.getCentreY() - 4, 8, 8), grey);
            zone (dd.translated (o.x, o.y), Hit::ResoShape, -1);
        }
        g.restoreState();
        for (auto i = z0; i < zones.size(); ++i) zones[i].r = zones[i].r.translated (-o.x, -o.y).transformedBy (tr).translated (o.x, o.y);
        g.endTransparencyLayer();
        std::vector<Zone> mine;
        for (auto i = z0; i < zones.size(); ++i) mine.push_back (zones[i]);
        zone (bs, Hit::Glass);
        for (auto& z : mine) zones.push_back (z);
        if (reveal < 0.05f) zones.resize (z0);
    }
}

// V6: Reso's main controls slide out from the left, in the space below 50 Hz that Reso never touches; its tab on the edge
void Canvas::paintResoStrip (juce::Graphics& g)
{
    const float pw = 156.0f, ph = 452.0f, top = display.getY() + (display.getHeight() - ph) * 0.5f;   // V7: centred top to bottom
    const R s (display.getX(), top, pw, ph);
    const R tabR = stripOpen ? R (s.getRight(), top + ph * 0.5f - 30, 14, 60) : R (display.getX(), top + ph * 0.5f - 30, 14, 60);
    auto f10 = font (Face::Regular, 10, 0.5f), f9 = font (Face::Regular, 9, 1.0f), f11 = font (Face::Regular, 11), fv = font (Face::Regular, 10);
    if (stripOpen)
    {
        {
            juce::Path p; p.addRoundedRectangle (s.getX(), s.getY(), s.getWidth(), s.getHeight(), 10, 10, false, true, false, true);
            juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 18, { 6, 0 }).drawForPath (g, p);
            g.setColour (col ("#1d1d20")); g.fillPath (p);
            g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        }
        const float x = s.getX() + 10, w = pw - 20;
        float y = s.getY() + 10;
        // Soft / Hard
        {
            const R seg (x, y, w, 20);
            fillRound (g, seg, 5, col ("#26262a"));
            const int mode = (int) proc.param ("rsMode");
            const char* sh[2] { "Soft", "Hard" };
            for (int i = 0; i < 2; ++i)
            {
                const R c (x + (float) i * w * 0.5f, y, w * 0.5f, 20);
                if (mode == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg, 5); g.reduceClipRegion (cp); g.setColour (col ("#3a3a40")); g.fillRect (c); g.restoreState(); }
                text (g, sh[i], f10, mode == i ? soft : col ("#8b8984"), c, juce::Justification::centred);
                zone (c, Hit::ResoMode, i);
            }
            y += 26;
            const bool ll = proc.param ("rsLowLat") > 0.5f;
            const R tg (x, y, 22, 12);
            fillRound (g, tg, 6, ll ? red.withAlpha (0.45f) : col ("#2c2c31"));
            g.setColour (ll ? soft : col ("#6b6a67")); g.fillEllipse (tg.getX() + (ll ? 12.0f : 2.0f), tg.getY() + 2, 8, 8);
            text (g, "Low latency", f10, ll ? soft : col ("#8b8984"), R (x + 28, y, 100, 12));
            zone (R (x, y - 2, w, 16), Hit::LowLat);
            y += 22;
        }
        // the seven knobs, stacked: knob, then its name over its value
        static const char* labels[7] { "Depth", "Detail", "Attack", "Release", "Max cut", "Mix", "Trim" };
        static const char* pids[7] { "rsDepth", "rsDetail", "rsAttack", "rsRelease", "rsMaxCut", "rsMix", "rsTrim" };
        juce::String vals[7];
        vals[0] = juce::String (proc.param ("rsDepth"), 1);
        vals[1] = juce::String ((int) std::round (proc.param ("rsDetail"))) + "%";
        { const float a = proc.param ("rsAttack"); vals[2] = (a < 10 ? juce::String (a, a < 1 ? 1 : (std::abs (a - std::round (a)) < 0.05f ? 0 : 1)) : juce::String ((int) std::round (a))) + " ms"; }
        vals[3] = juce::String ((int) std::round (proc.param ("rsRelease"))) + " ms";
        vals[4] = (proc.param ("rsMaxCut") > 0.05f ? MINUS : juce::String()) + juce::String ((int) std::round (proc.param ("rsMaxCut"))) + " dB";
        vals[5] = juce::String ((int) std::round (proc.param ("rsMix"))) + "%";
        { const float t = proc.param ("rsTrim"); vals[6] = (t < -0.05f ? MINUS : t > 0.05f ? juce::String ("+") : juce::String()) + juce::String (std::abs (t), 1) + " dB"; }
        for (int i = 0; i < 7; ++i)
        {
            neonKnob (g, { x, y }, proc.apvts.getParameter (pids[i])->getValue(), 34.0f);
            text (g, labels[i], f11, soft, R (x + 44, y + 3, w - 44, 14));
            text (g, vals[i], fv, col ("#b9b3a7"), R (x + 44, y + 18, w - 44, 13));
            zone (R (x, y, 34, 34), Hit::ResoKnob, i);
            y += 42;
        }
        y += 4;
        // the reduction meter
        {
            const float redDb = -activeResoReduction();
            text (g, "Reduction", fv, col ("#9a9792"), R (x, y, w, 12));
            const juce::String v = (redDb > 0.05f ? MINUS : juce::String()) + juce::String (redDb, 1) + " dB";
            text (g, v, fv, soft, R (x, y, w, 12), juce::Justification::centredRight);
            const int lit = juce::jlimit (0, 16, juce::roundToInt (redDb / 10.0f * 16.0f));
            const float sw = (w - 30.0f) / 16.0f;
            for (int i = 0; i < 16; ++i)
            {
                juce::Colour c = col ("#2c2c31");
                if (i < lit) c = lit == 1 ? col ("#ef6c6c") : col ("#6e1720").interpolatedWith (col ("#ef6c6c"), (float) i / (float) (lit - 1));
                fillRound (g, R (x + (float) i * (sw + 2), y + 16, sw, 10), 2, c);
            }
            y += 36;
        }
        // Stereo / Mid / Side, Delta, Sidechain
        {
            const char* ms[3] { "Stereo", "Mid", "Side" };
            const R seg (x, y, w, 19);
            fillRound (g, seg, 5, col ("#26262a"));
            const int m = (int) proc.param ("rsMs");
            for (int i = 0; i < 3; ++i)
            {
                const R c (x + (float) i * w / 3.0f, y, w / 3.0f, 19);
                if (m == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg, 5); g.reduceClipRegion (cp); g.setColour (col ("#3a3a40")); g.fillRect (c); g.restoreState(); }
                text (g, ms[i], f9, m == i ? soft : col ("#8b8984"), c, juce::Justification::centred);
                zone (c, Hit::ResoMs, i);
            }
            y += 24;
            const float hw = (w - 4) * 0.5f;
            const bool dl = proc.param ("rsDelta") > 0.5f, sc = proc.param ("rsKey") > 0.5f;
            const R d1 (x, y, hw, 19), d2 (x + hw + 4, y, hw, 19);
            fillRound (g, d1, 5, dl ? col ("#3a3a40") : col ("#26262a"));
            fillRound (g, d2, 5, sc ? col ("#1f2a33") : col ("#26262a"));
            text (g, "Delta", f9, dl ? soft : col ("#8b8984"), d1, juce::Justification::centred);
            text (g, "Sidechain", f9, sc ? col ("#8ec5ff") : col ("#8b8984"), d2, juce::Justification::centred);
            zone (d1, Hit::Delta); zone (d2, Hit::ResoSc);
        }
    }
    // its tab on the edge
    {
        juce::Path p; p.addRoundedRectangle (tabR.getX(), tabR.getY(), tabR.getWidth(), tabR.getHeight(), 6, 6, false, true, false, true);
        g.setColour (col ("#1d1d20")); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        icon (g, stripOpen ? Icon::Left : Icon::Right, tabR.withSizeKeepingCentre (10, 12), col ("#9a9792"), 1.4f);
        zone (tabR.expanded (2), Hit::Strip);
    }
    // ADV's own tab on the right edge (the panel slides out from the right)
    {
        const R a = advOpen ? R (display.getRight() - 156 - 14, top + ph * 0.5f - 30, 14, 60) : R (display.getRight() - 14, top + ph * 0.5f - 30, 14, 60);
        juce::Path p; p.addRoundedRectangle (a.getX(), a.getY(), a.getWidth(), a.getHeight(), 6, 6, true, false, true, false);
        g.setColour (col ("#1d1d20")); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        const auto c = advOpen ? soft : col ("#9a9792");
        auto fa = font (Face::CondMedium, 9, 0.5f);
        const char* t[3] { "A", "D", "V" };
        for (int i = 0; i < 3; ++i) text (g, t[i], fa, c, R (a.getX(), a.getY() + 10 + (float) i * 10, 14, 10), juce::Justification::centred);
        icon (g, advOpen ? Icon::Right : Icon::Left, R (a.getX() + 2, a.getBottom() - 18, 10, 10), c, 1.3f);
        zone (a.expanded (2), Hit::Adv);
    }
}

void Canvas::paintAdvanced (juce::Graphics& g)
{
    // V7: the same size and compact stacked style as the Depth panel on the left, mirrored, centred top to bottom
    const float pw = 156.0f, ph = 452.0f;
    const R s (display.getRight() - pw, display.getY() + (display.getHeight() - ph) * 0.5f, pw, ph);
    {
        juce::Path p; p.addRoundedRectangle (s.getX(), s.getY(), s.getWidth(), s.getHeight(), 10, 10, true, false, true, false);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 18, { -6, 0 }).drawForPath (g, p);
        g.setColour (col ("#1d1d20")); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    }
    const float x = s.getX() + 10, w = pw - 20;
    auto f11 = font (Face::Regular, 11), fv = font (Face::Regular, 10), fh = font (Face::CondMedium, 9.5f, 1.4f), f9 = font (Face::Regular, 9, 1.0f);
    float y = s.getY() + 12;
    text (g, "ADVANCED", font (Face::CondMedium, 11, 1.8f), soft, R (x, y, w, 14));
    y += 26;
    auto heading = [&] (const char* t)
    {
        text (g, t, fh, col ("#8b8984"), R (x, y, w, 11));
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (x + textWidth (fh, t) + 8, y + 5.5f, w - textWidth (fh, t) - 8, 1.0f);
        y += 18;
    };
    auto row = [&] (const char* label, const char* pid, const juce::String& value, Hit h, int a)
    {
        neonKnob (g, { x, y }, proc.apvts.getParameter (pid)->getValue(), 34.0f);
        text (g, label, f11, soft, R (x + 44, y + 3, w - 44, 14));
        text (g, value, fv, col ("#b9b3a7"), R (x + 44, y + 18, w - 44, 13));
        zone (R (x, y, 34, 34), h, a);
        y += 42;
    };
    auto pct = [this] (const char* pid) { const float v = proc.param (pid); return (v > 0.5f ? juce::String ("+") : v < -0.5f ? MINUS : juce::String()) + juce::String ((int) std::round (std::abs (v))) + "%"; };
    heading ("TILT");
    row ("Detail tilt", "rsDTilt", pct ("rsDTilt"), Hit::TiltKnob, 0);
    row ("Attack tilt", "rsATilt", pct ("rsATilt"), Hit::TiltKnob, 1);
    row ("Release tilt", "rsRTilt", pct ("rsRTilt"), Hit::TiltKnob, 2);
    text (g, "leans each to the lows or highs", font (Face::Regular, 8.5f), col ("#6b6864"), R (x, y - 4, w, 11));
    y += 22;
    heading ("STEREO LINK");
    row ("Link", "rsLink", juce::String ((int) std::round (proc.param ("rsLink"))) + "%", Hit::LinkKnob, 0);
    text (g, "how much L and R react together", font (Face::Regular, 8.5f), col ("#6b6864"), R (x, y - 4, w, 11));
    y += 22;
    heading ("PHASE");
    {
        const R seg (x, y, w, 20);
        fillRound (g, seg, 5, col ("#26262a"));
        const int ph_ = (int) proc.param ("rsPhase");
        for (int i = 0; i < 2; ++i)
        {
            const R c (x + (float) i * w * 0.5f, y, w * 0.5f, 20);
            if (ph_ == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg, 5); g.reduceClipRegion (cp); g.setColour (col ("#3a3a40")); g.fillRect (c); g.restoreState(); }
            text (g, i == 0 ? "Minimum" : "Linear", f9, ph_ == i ? soft : col ("#8b8984"), c, juce::Justification::centred);
            zone (c, Hit::ResoPhase, i);
        }
        y += 26;
        text (g, "linear keeps parallel and M/S work in phase", font (Face::Regular, 8.5f), col ("#6b6864"), R (x, y, w, 11));
    }
    glassRects.push_back (s);
}

//==============================================================================
void Canvas::paintTahorTab (juce::Graphics& g)
{
    const P o (display.getX(), display.getY());
    zone (R (0, 0, PW, 600).translated (o.x, o.y), Hit::TahorBg);
    const float rng = dbRange (proc);
    auto yOf = [rng] (float db) { return 335.0f - juce::jlimit (-rng * 0.95f, rng * 0.95f, db) / rng * 335.0f; };
    const auto bands = proc.tahorBands();
    if (proc.param (bid (selBand, "On")) < 0.5f)
        for (int b = 0; b < numBands; ++b) if (bands[(size_t) b].on) { selBand = b; break; }
    auto nodeY = [&] (int b) { const auto& bd = bands[(size_t) b]; return bd.hasGain() ? yOf (bd.gain) : 335.0f; };   // V10: the set gain
    // nodes
    for (int b = 0; b < numBands; ++b)
    {
        if (! bands[(size_t) b].on) continue;
        const float x = fx (bands[(size_t) b].freq), y = nodeY (b);
        const auto c = bandColour (b);
        const auto t = juce::String (b + 1);
        if (bands[(size_t) b].dyn && bands[(size_t) b].hasGain())   // V10: a faint ring rides the live gain
        {
            const float yd = yOf (bands[(size_t) b].gain + proc.dynDb[(size_t) b].load());
            if (std::abs (yd - y) > 1.0f) { g.setColour (c.withAlpha (0.55f)); g.drawEllipse (x - 6, yd - 6, 12, 12, 1.3f); }
        }
        if (b == selBand)
        {
            g.setColour (c.withAlpha (0.16f)); g.fillEllipse (x - 19, y - 19, 38, 38);
            g.setColour (col ("#fff3dc")); g.fillEllipse (x - 11, y - 11, 22, 22);
            g.setColour (c); g.fillEllipse (x - 9, y - 9, 18, 18);
            text (g, t, font (Face::SemiBold, 11), col ("#1a1208"), R (x - 11, y - 11, 22, 22), juce::Justification::centred);
            if (reveal > 0.01f)   // V11: the bubble - shape, frequency, gain - live as you move it (like Character's)
            {
                static const char* types[6] { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" };
                const auto& bd = bands[(size_t) b];
                juce::String lab = juce::String (types[(int) bd.type]) + DOT + hzText (bd.freq);
                if (bd.hasGain()) lab << DOT << num (bd.gain) << " dB";
                else if (bd.isCut()) { static const char* sl[6] { "6", "12", "18", "24", "36", "48" }; lab << DOT << sl[juce::jlimit (0, 5, bd.slope)] << " dB/oct"; }
                auto f = font (Face::Regular, 12);
                const R bb (juce::jlimit (4.0f, PW - textWidth (f, lab) - 24, x - 30), y - 50 < 4 ? y + 22 : y - 50, textWidth (f, lab) + 20, 22.4f);
                g.beginTransparencyLayer (reveal);
                fillRound (g, bb, 12, col ("#0e0e10", 0.9f)); strokeRound (g, bb, 12, c.withAlpha (0.6f));
                text (g, lab, f, juce::Colours::white, bb.reduced (10, 0));
                g.endTransparencyLayer();
            }
        }
        else
        {
            g.setColour (col ("#141418")); g.fillEllipse (x - 9, y - 9, 18, 18);
            g.setColour (c); g.drawEllipse (x - 7.75f, y - 7.75f, 15.5f, 15.5f, 2.5f);
            text (g, t, font (Face::Regular, 10), c, R (x - 9, y - 9, 18, 18), juce::Justification::centred);
        }
        if (bands[(size_t) b].place >= 3)   // V8: the side it works on
        {
            const bool lft = bands[(size_t) b].place == 3;
            const R tg (x + 8, y - 20, 12, 11);
            fillRound (g, tg, 3, lft ? col ("#7fd3ff") : col ("#ff8fb8"));
            text (g, lft ? "L" : "R", font (Face::SemiBold, 8.5f), col ("#101014"), tg, juce::Justification::centred);
        }
        zone (R (x - 12, y - 12, 24, 24).translated (o.x, o.y), Hit::TahorNode, b);
    }
    // the band pop-up (glass)
    if (bands[(size_t) selBand].on)
    {
        const auto& bd = bands[(size_t) selBand];
        const float nx = fx (bd.freq), ny = nodeY (selBand);
        juce::ignoreUnused (ny);
        // V5: small (Pro-Q 4 style), at the bottom of the screen, sliding along to sit under the band you touch
        const int modeW = (proc.param (bid (selBand, "Spec")) > 0.5f || proc.param (bid (selBand, "Dyn")) > 0.5f) ? 1 : 0;
        const float k = 0.84f, pw = modeW == 0 ? 500.0f : 692.0f;   // V9: wider for the dynamic view: 20% bigger, narrower (one dropdown for the four switches)
        const float px = juce::jlimit (8.0f, PW - 8.0f - pw * k, nx - pw * 0.5f * k), py = 662.0f - 104.0f * k;
        const auto tr = juce::AffineTransform::scale (k).translated (px, py);
        const R b (0, 0, pw, 104.0f);
        const R bs = R (px, py, pw * k, 104.0f * k).translated (o.x, o.y);
        if (reveal > 0.05f) revealRects.push_back (bs);
        const auto z0 = zones.size();
        g.beginTransparencyLayer (reveal);
        g.saveState(); g.addTransform (tr);
        const auto c = bandColour (selBand);
        bandPanel (g, b, (nx - px) / k, c);
        float x = b.getX() + 12;
        const float cy = b.getCentreY();
        // column: name, type, placement
        {
            const float top = cy - (15.6f + 7 + 24 + 7 + 22) * 0.5f;
            g.setColour (c); g.fillEllipse (x, top + 4, 8, 8);
            text (g, "Band " + juce::String (selBand + 1), font (Face::Regular, 13), ink, R (x + 14, top, 100, 15.6f));
            const R ty (x, top + 22.6f, 112, 24);
            strokeRound (g, ty, 6, col ("#34353b"));
            static const char* types[6] { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" };
            text (g, types[(int) bd.type], font (Face::Regular, 12), ink, ty.reduced (8, 0));
            icon (g, Icon::TriDown, R (ty.getRight() - 18, ty.getCentreY() - 4, 8, 8), grey);
            zone (ty.translated (o.x, o.y), Hit::TahorType);
            const R pl (x, top + 53.6f, 112, 22);
            strokeRound (g, pl, 6, col ("#34353b"));
            const char* ps[5] { "ST", "M", "S", "L", "R" };
            const float pw = 112.0f / 5.0f;
            for (int i = 0; i < 5; ++i)
            {
                const R cc (x + (float) i * pw, pl.getY(), pw, 22);
                if (bd.place == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (pl.reduced (1), 5); g.reduceClipRegion (cp); g.setColour (col ("#2b2c31")); g.fillRect (cc); g.restoreState(); }
                text (g, ps[i], font (Face::Regular, 11), bd.place == i ? ink : grey, cc, juce::Justification::centred);
                zone (cc.translated (o.x, o.y), Hit::TahorPlace, i);
            }
            x += 112 + 14;
        }
        // V9: the panel flips with the band's mode - EQ: FREQ, GAIN, Q; Dynamic: THRESHOLD, RANGE, ATTACK, RELEASE, Q and a
        // meter of what it's pulling right now; Spectral: the same plus SELECT (how narrow it listens)
        const bool dynM = proc.param (bid (selBand, "Dyn")) > 0.5f, specM = proc.param (bid (selBand, "Spec")) > 0.5f;
        const int mode = (specM || dynM) ? 1 : 0;   // V10: EQ or Dynamic (no Spectral)
        {
            struct KS { int idx; const char* lab; juce::String val; };
            std::vector<KS> ks;
            auto ms = [] (float v) { return v < 10.0f ? juce::String (v, 1) + " ms" : juce::String ((int) std::round (v)) + " ms"; };
            if (mode == 0)
            {
                const float f = bd.freq;
                ks.push_back ({ 0, "FREQ", f >= 1000.0f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String ((int) std::round (f)) + " Hz" });
                ks.push_back ({ 1, "GAIN", bd.hasGain() ? num (bd.gain) + " dB" : juce::String (U8 ("\xe2\x80\x94")) });
                ks.push_back ({ 2, "Q", juce::String (bd.q, 2) });
            }
            else
            {
                ks.push_back ({ 3, "THRESH", num (proc.param (bid (selBand, "Thr"))) + " dB" });
                ks.push_back ({ 4, "RANGE", num (proc.param (bid (selBand, "Rng"))) + " dB" });
                ks.push_back ({ 5, "ATTACK", ms (proc.param (bid (selBand, "Att"))) });
                ks.push_back ({ 6, "RELEASE", ms (proc.param (bid (selBand, "Rel"))) });
                ks.push_back ({ 2, "Q", juce::String (bd.q, 2) });
            }
            const auto kc = mode == 0 ? amber : mode == 1 ? col ("#7fd3ff") : col ("#c08bff");
            for (auto& k : ks)
            {
                static const char* pid[8] { "Hz", "Gain", "Q", "Thr", "Rng", "Att", "Rel", "Sel" };
                const float top = cy - (42 + 4 + 14.4f + 4 + 12) * 0.5f;
                bandKnob (g, R (x + 5, top, 42, 42), proc.apvts.getParameter (bid (selBand, pid[k.idx]))->getValue(), kc);
                text (g, k.val, font (Face::Regular, 12), ink, R (x - 10, top + 46, 72, 14.4f), juce::Justification::centredTop);
                text (g, k.lab, font (Face::Regular, 10, 1), grey, R (x - 10, top + 64.4f, 72, 12), juce::Justification::centredTop);
                zone (R (x + 5, top, 42, 42).translated (o.x, o.y), Hit::TahorKnob, k.idx);
                x += 52 + 14;
            }
            if (mode != 0)   // the live meter: how many dB it's pulling (or pushing) right now
            {
                const float dd = proc.dynDb[(size_t) selBand].load();
                const float rng = juce::jmax (1.0f, std::abs (proc.param (bid (selBand, "Rng"))));
                const R m (x + 2, cy - 34, 12, 60);
                fillRound (g, m, 3, col ("#0b0b0d", 0.9f)); strokeRound (g, m, 3, col ("#34353b"));
                const float fr = juce::jlimit (0.0f, 1.0f, std::abs (dd) / rng);
                if (fr > 0.001f)
                {
                    const R on = dd < 0 ? m.reduced (2).withHeight ((m.getHeight() - 4) * fr) : m.reduced (2).withTrimmedTop ((m.getHeight() - 4) * (1 - fr));
                    fillRound (g, on, 2, kc);
                }
                text (g, (dd < -0.05f ? MINUS : dd > 0.05f ? juce::String ("+") : juce::String()) + juce::String (std::abs (dd), 1),
                      font (Face::Medium, 10), kc, R (x - 12, cy + 30, 40, 12), juce::Justification::centredTop);
                x += 30;
            }
        }
        // V6: Dynamic, Spectral, Sidechain and Solo in one dropdown (Pro-Q 4 style); it shows what's on
        {
            const bool dyn = proc.param (bid (selBand, "Dyn")) > 0.5f, solo = proc.param (bid (selBand, "Solo")) > 0.5f;
            juce::StringArray on;
            on.add (mode == 2 ? "SPECTRAL" : mode == 1 ? "DYNAMIC" : "EQ");   // V9: the band's mode
            if (bd.sc) on.add ("SC");
            juce::String t = on.isEmpty() ? juce::String ("OPTIONS") : on.joinIntoString (U8 (" \xc2\xb7 "));
            if (dyn && dragging && down.kind == Hit::TahorChip) t = "THR " + num (proc.param (bid (selBand, "Thr")));
            const R dd (x, cy - 13, b.getRight() - 12 - 32 - x, 26);
            {   // V8: Solo, right on the panel
                const R sb (b.getRight() - 12 - 26, cy - 13, 26, 26);
                fillRound (g, sb, 6, solo ? col ("#3a2c14") : col ("#232327")); strokeRound (g, sb, 6, solo ? amber : col ("#34353b"));
                text (g, "S", font (Face::SemiBold, 12), solo ? amber : grey, sb, juce::Justification::centred);
                zone (sb.translated (o.x, o.y), Hit::BandSolo, 0);
            }
            const bool any = mode != 0 || bd.sc;
            fillRound (g, dd, 6, any ? col ("#3a2c14") : col ("#232327")); strokeRound (g, dd, 6, any ? col ("#6b4f1f") : col ("#34353b"));
            text (g, t, font (Face::Regular, 10, 0.5f), any ? amber : grey, dd.withTrimmedRight (16).reduced (8, 0), juce::Justification::centredLeft);
            icon (g, Icon::TriDown, R (dd.getRight() - 16, dd.getCentreY() - 4, 8, 8), any ? amber : grey);
            zone (dd.translated (o.x, o.y), Hit::TahorChip, 9);
        }
        g.restoreState();
        for (auto i = z0; i < zones.size(); ++i) zones[i].r = zones[i].r.translated (-o.x, -o.y).transformedBy (tr).translated (o.x, o.y);
        g.endTransparencyLayer();
        std::vector<Zone> mine;
        for (auto& z : zones) if (bs.contains (z.r.getCentre()) && (z.kind == Hit::TahorType || z.kind == Hit::TahorPlace || z.kind == Hit::TahorKnob || z.kind == Hit::TahorChip || z.kind == Hit::BandSolo)) mine.push_back (z);
        zone (bs, Hit::Glass);
        for (auto& z : mine) zones.push_back (z);
        if (reveal < 0.05f) zones.resize (z0);
    }
}

//==============================================================================
// the faceplate metal + its age (the same random layers as the design, seeded the same way)
const juce::Image& Canvas::metalImage (int unit, int w, int h)
{
    auto it = metal.find (unit * 1000 + h);
    if (it != metal.end()) return it->second;
    const auto& U = units()[(size_t) unit];
    const float wear = U.wear;
    const int sc = 3;   // V5: drawn at 3x so it stays sharp at every size
    juce::Image img (juce::Image::ARGB, w * sc, h * sc, true);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale ((float) sc));
    const R box (0, 0, (float) w, (float) h);

    // the metal (opacity 0.7, CSS filter by wear)
    {
        const float sep = 0.55f * wear * wear, sat = 1.0f - 0.25f * wear, con = 1.0f + 0.08f * wear;
        const juce::Colour c0 = cssFilter (col (U.bg[0]), sep, sat, con), c1 = cssFilter (col (U.bg[1]), sep, sat, con), c2 = cssFilter (col (U.bg[2]), sep, sat, con);
        juce::Image m (juce::Image::ARGB, w, h, true);
        { juce::Graphics mg (m);
          for (int x = 0; x < w; ++x)
          {
              const int k = x % 4;
              const juce::Colour c = k == 0 ? c0 : k == 1 ? c0.interpolatedWith (c1, 0.5f) : k == 2 ? c1 : c1.interpolatedWith (c2, 0.5f);
              mg.setColour (c); mg.fillRect (x, 0, 1, h);
          }
          mg.setColour (cssFilter (col (U.line), sep, sat, con)); mg.fillRect (0, 0, w, 1); }
        g.setOpacity (0.7f);
        g.drawImage (m, box);
        g.setOpacity (1.0f);
    }
    // the age layer (opacity 0.7): built in the design's order, drawn bottom-up
    juce::int64 seed = 11 + 17 * unit;
    auto rnd = [&seed] { seed = (seed * 9301 + 49297) % 233280; return (float) seed / 233280.0f; };
    std::vector<std::function<void (juce::Graphics&)>> L;
    auto pt = [&] { const float a = std::round (rnd() * 100.0f), b = std::round (rnd() * 100.0f); return P (a / 100.0f * (float) w, b / 100.0f * (float) h); };
    const int nScr = juce::roundToInt (2.0f + wear * 8.0f);
    for (int j = 0; j < nScr; ++j)
    {
        const float ang = std::round (rnd() * 180.0f), at = std::round ((20.0f + rnd() * 60.0f) * 10.0f) / 10.0f;
        const float a = 0.12f + 0.3f * wear;
        L.push_back ([=] (juce::Graphics& gg)
        {
            auto gr = cssLinear (box, ang);
            gr.addColour (0.0, juce::Colours::transparentWhite);
            gr.addColour (at / 100.0, juce::Colours::transparentWhite);
            gr.addColour ((at + 0.15f) / 100.0, juce::Colours::white.withAlpha (a));
            gr.addColour ((at + 0.4f) / 100.0, juce::Colours::transparentWhite);
            gr.addColour (1.0, juce::Colours::transparentWhite);
            gg.setGradientFill (gr); gg.fillRect (box);
        });
    }
    if (wear > 0.7f)
    {
        const float ra = 0.35f + 0.4f * (wear - 0.7f) / 0.3f;
        const auto rust = [] (float a) { return juce::Colour ((juce::uint8) 140, (juce::uint8) 68, (juce::uint8) 22, a); };
        L.push_back ([=] (juce::Graphics& gg) { fillEllipseGrad (gg, box, 0, (float) h, 90, 40, { { 0.0f, rust (ra) }, { 0.7f, rust (0.0f) }, { 1.0f, rust (0.0f) } }); });
        L.push_back ([=] (juce::Graphics& gg) { fillEllipseGrad (gg, box, (float) w, 0, 70, 34, { { 0.0f, juce::Colour ((juce::uint8) 130, (juce::uint8) 62, (juce::uint8) 20, ra) }, { 0.7f, juce::Colour ((juce::uint8) 130, (juce::uint8) 62, (juce::uint8) 20, 0.0f) }, { 1.0f, juce::Colours::transparentBlack } }); });
        const int nr = juce::roundToInt (2.0f + 4.0f * wear);
        for (int j = 0; j < nr; ++j)
        {
            const float rx = std::round (10 + rnd() * 24), ry = std::round (6 + rnd() * 12);
            const auto p = pt();
            const float a = 0.4f + 0.3f * rnd();
            L.push_back ([=] (juce::Graphics& gg) { fillEllipseGrad (gg, box, p.x, p.y, rx, ry, { { 0.0f, juce::Colour ((juce::uint8) 135, (juce::uint8) 64, (juce::uint8) 20, a) }, { 1.0f, juce::Colour ((juce::uint8) 135, (juce::uint8) 64, (juce::uint8) 20, 0.0f) } }); });
        }
    }
    const int nChip = juce::roundToInt (wear * 9.0f);
    for (int j = 0; j < nChip; ++j)
    {
        const float rx = std::round (6 + rnd() * 30), ry = std::round (4 + rnd() * 12);
        const auto p = pt();
        const auto bare = [] (float a) { return juce::Colour ((juce::uint8) 196, (juce::uint8) 196, (juce::uint8) 190, a); };
        const float a0 = 0.35f + 0.35f * wear, a1 = 0.2f * wear;
        L.push_back ([=] (juce::Graphics& gg) { fillEllipseGrad (gg, box, p.x, p.y, rx, ry, { { 0.0f, bare (a0) }, { 0.55f, bare (a1) }, { 0.75f, bare (0.0f) }, { 1.0f, bare (0.0f) } }); });
    }
    {
        const float e = std::round (3 + 6 * wear);
        const auto ed = [] (float a) { return juce::Colour ((juce::uint8) 200, (juce::uint8) 200, (juce::uint8) 195, a); };
        L.push_back ([=] (juce::Graphics& gg)
        {
            juce::ColourGradient gr (ed (0.5f * wear), 0, 0, ed (0.45f * wear), 0, (float) h, false);
            gr.clearColours();
            gr.addColour (0.0, ed (0.5f * wear)); gr.addColour (e / (float) h, ed (0.0f));
            gr.addColour (1.0 - e / (float) h, ed (0.0f)); gr.addColour (1.0, ed (0.45f * wear));
            gg.setGradientFill (gr); gg.fillRect (box);
        });
    }
    const int nGrime = 2 + juce::roundToInt (wear * 4.0f);
    for (int j = 0; j < nGrime; ++j)
    {
        const float rx = std::round (50 + rnd() * 90), ry = std::round (25 + rnd() * 30);
        const auto p = pt();
        const float a = (0.25f + rnd() * 0.25f) * wear;
        L.push_back ([=] (juce::Graphics& gg) { fillEllipseGrad (gg, box, p.x, p.y, rx, ry, { { 0.0f, juce::Colour ((juce::uint8) 28, (juce::uint8) 16, (juce::uint8) 6, a) }, { 1.0f, juce::Colour ((juce::uint8) 28, (juce::uint8) 16, (juce::uint8) 6, 0.0f) } }); });
    }
    auto stripes = [box] (float deg, float period, juce::Colour c)
    {
        return [=] (juce::Graphics& gg)
        {
            // stripes perpendicular to the gradient direction, 1 px of colour every period
            const float ang = deg * pi / 180.0f;
            const P dir (std::sin (ang), -std::cos (ang)), perp (-dir.y, dir.x);
            const float Ln = cssLinearLength (box, deg);
            const auto c0 = box.getCentre() - dir * (Ln * 0.5f);
            const float reach = box.getWidth() + box.getHeight();
            gg.saveState(); gg.reduceClipRegion (box.toNearestInt());
            gg.setColour (c);
            for (float s = 0.5f; s < Ln; s += juce::jmax (2.0f, period))
            {
                const auto m = c0 + dir * s;
                gg.drawLine (juce::Line<float> (m - perp * reach, m + perp * reach), 1.0f);
            }
            gg.restoreState();
        };
    };
    { const float ang = std::round (10 + rnd() * 30), per = std::round (6 + rnd() * 8); L.push_back (stripes (ang, per, juce::Colours::white.withAlpha (0.14f * wear))); }
    { const float ang = std::round (140 + rnd() * 30), per = std::round (10 + rnd() * 12); L.push_back (stripes (ang, per, juce::Colours::black.withAlpha (0.16f * wear))); }
    L.push_back ([=] (juce::Graphics& gg)
    {
        gg.setGradientFill (juce::ColourGradient (juce::Colour ((juce::uint8) 170, (juce::uint8) 125, (juce::uint8) 40, 0.3f * wear * wear), 0, 0,
                                                  juce::Colour ((juce::uint8) 120, (juce::uint8) 85, (juce::uint8) 25, 0.24f * wear * wear), 0, (float) h, false));
        gg.fillRect (box);
    });
    {
        const float inner = std::round (68 - 26 * wear) / 100.0f, a = 0.15f + 0.6f * wear;
        L.push_back ([=] (juce::Graphics& gg)
        {
            const auto v = [] (float al) { return juce::Colour ((juce::uint8) 22, (juce::uint8) 13, (juce::uint8) 5, al); };
            fillEllipseGrad (gg, box, (float) w * 0.5f, (float) h * 0.5f, (float) w * 0.5f * 1.4142f, (float) h * 0.5f * 1.4142f,
                             { { 0.0f, v (0.0f) }, { inner, v (0.0f) }, { 1.0f, v (a) } });
        });
    }
    juce::Image age (juce::Image::ARGB, w * sc, h * sc, true);
    { juce::Graphics ag (age); ag.addTransform (juce::AffineTransform::scale ((float) sc));
      for (auto i = L.rbegin(); i != L.rend(); ++i) (*i) (ag); }
    g.setOpacity (0.7f);
    g.drawImage (age, box);
    return metal.emplace (unit * 1000 + h, img).first->second;
}

// one vintage knob: printed tick ring + its body (chicken / console / neve / stack / skirt / knurl)
void Canvas::vintageKnob (juce::Graphics& g, P tl, int unit, int knob, float pos, float pos2)
{
    const auto& U = units()[(size_t) unit];
    const float s = U.kind == KnobKind::Neve ? 42.0f : 40.0f, Rr = s + 14.0f;
    const auto c = tl + P (Rr * 0.5f, Rr * 0.5f);
    const float ww = U.wear * 0.7f;
    // the wear halo, then the twelve ticks
    fillEllipseGrad (g, R (tl.x, tl.y, Rr, Rr), c.x, c.y, Rr * 0.5f * 1.4142f, Rr * 0.5f * 1.4142f,
                     { { 0.5f, juce::Colour ((juce::uint8) 205, (juce::uint8) 205, (juce::uint8) 198, 0.55f * ww) },
                       { 0.62f, juce::Colour ((juce::uint8) 205, (juce::uint8) 205, (juce::uint8) 198, 0.25f * ww) },
                       { 0.74f, juce::Colours::transparentBlack }, { 1.0f, juce::Colours::transparentBlack } });
    {
        g.saveState(); juce::Path clip; clip.addEllipse (tl.x, tl.y, Rr, Rr); g.reduceClipRegion (clip);
        g.setColour (col (U.tick));
        for (int k = 0; k < 12; ++k)
        {
            juce::Path w; w.addPieSegment (tl.x - 2, tl.y - 2, Rr + 4, Rr + 4, (float) k * pi / 6.0f, (float) k * pi / 6.0f + 2.4f * pi / 180.0f, 0.0f);
            g.fillPath (w);
        }
        g.restoreState();
    }
    const float a = (270.0f * juce::jlimit (0.0f, 1.0f, pos) - 135.0f) * pi / 180.0f;
    const float inner = U.kind == KnobKind::Console ? std::round (s * 0.8f) : U.kind == KnobKind::Stack ? std::round (s * 0.6f) : s;
    const float off = 7.0f + (s - inner) * 0.5f;
    const R body (tl.x + off, tl.y + off, inner, inner);
    const auto rot = juce::AffineTransform::rotation (a, body.getCentreX(), body.getCentreY());
    auto radial = [&] (R r, float fx, float fy, juce::Colour c0, juce::Colour c1, float stop, juce::Colour c2 = {}, bool three = false)
    {
        juce::ColourGradient gr (c0, r.getX() + r.getWidth() * fx, r.getY() + r.getHeight() * fy, three ? c2 : c1,
                                 r.getX() + r.getWidth() * fx + r.getWidth() * 0.5f * 1.5f, r.getY() + r.getHeight() * fy, true);
        if (three) gr.addColour (stop, c1); else gr.addColour (stop, c1);
        g.setGradientFill (gr);
        g.fillEllipse (r);
    };
    auto rim = [&] (R r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (r.expanded (0.5f), 1.0f);
        g.saveState(); juce::Path cl; cl.addEllipse (r); g.reduceClipRegion (cl);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawEllipse (r.translated (0, 1.5f).expanded (0.5f), 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawEllipse (r.translated (0, -2.5f).expanded (1.5f), 3.0f);
        g.restoreState();
    };
    // the stacked knob's outer ring (it doesn't turn with the inner one)
    if (U.kind == KnobKind::Stack)
    {
        const R ob (tl.x + 7, tl.y + 7, s, s);
        { juce::Path p; p.addEllipse (ob.translated (0, 5)); juce::DropShadow (juce::Colours::black.withAlpha (0.85f), 6, {}).drawForPath (g, p); }
        g.saveState(); juce::Path cl; cl.addEllipse (ob); g.reduceClipRegion (cl);
        for (int k = 0; k < 72; ++k)
        {
            juce::Path w; w.addPieSegment (ob.expanded (1), (float) k * 5.0f * pi / 180.0f, (float) (k + 1) * 5.0f * pi / 180.0f, 0.0f);
            g.setColour (col (k % 2 == 0 ? U.outer[0] : U.outer[1])); g.fillPath (w);
        }
        const float b = (270.0f * juce::jlimit (0.0f, 1.0f, pos2) - 135.0f) * pi / 180.0f;
        juce::Path mk; mk.addPieSegment (ob.expanded (1), b - 3.0f * pi / 180.0f, b + 3.0f * pi / 180.0f, 0.0f);
        g.setColour (juce::Colours::white); g.fillPath (mk);
        g.restoreState();
        rim (ob);
    }
    { juce::Path p; p.addEllipse (body.translated (0, 6)); juce::DropShadow (juce::Colours::black.withAlpha (0.85f), 6, {}).drawForPath (g, p); }
    g.saveState();
    g.addTransform (rot);
    const juce::String capCol = U.caps[knob] != nullptr ? U.caps[knob] : (U.cap[0] != nullptr ? U.cap[0] : "#888888");
    const auto cap0 = col (capCol.toRawUTF8());
    const auto white = juce::Colours::white;
    auto line = [&] (float wpx, float topPct, float hPct, juce::Colour c, bool glow)
    {
        const R l (body.getCentreX() - wpx * 0.5f, body.getY() + body.getHeight() * topPct, wpx, body.getHeight() * hPct);
        if (glow) { g.setColour (white.withAlpha (0.45f)); g.fillRoundedRectangle (l.expanded (1.5f), 1.5f); }
        g.setColour (c); g.fillRect (l);
    };
    switch (U.kind)
    {
        case KnobKind::Console:
            radial (body, 0.45f, 0.35f, col ("#3a3a3a"), col ("#070707"), 0.7f);
            g.setColour (col ("#1a1a1a")); g.drawEllipse (body.reduced (1), 2.0f);
            rim (body);
            radial (body.reduced (inner * 0.22f), 0.4f, 0.3f, white.withAlpha (0.45f), cap0, 0.55f, cap0, true);
            line (3, 0.0f, 0.3f, white, true);
            break;
        case KnobKind::Neve:
            for (int k = 0; k < 90; ++k)
            {
                juce::Path w; w.addPieSegment (body, (float) k * 4.0f * pi / 180.0f, (float) (k + 1) * 4.0f * pi / 180.0f, 0.0f);
                g.setColour (col (k % 2 == 0 ? "#6f7479" : "#9ca2a7")); g.fillPath (w);
            }
            g.setColour (col ("#50555a")); g.drawEllipse (body.reduced (1.5f), 3.0f);
            rim (body);
            radial (body.reduced (inner * 0.27f), 0.4f, 0.3f, white.withAlpha (0.4f), cap0, 0.55f, cap0, true);
            line (3, 0.0f, 0.27f, col ("#111111"), false);
            break;
        case KnobKind::Stack:
        {
            radial (body, 0.4f, 0.3f, col (U.cap[0]), col (U.cap[1]), 0.75f, col (U.cap[1]), true);
            g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (body.expanded (1), 2.0f);
            rim (body);
            line (3, 0.04f, 0.36f, white, true);
            break;
        }
        case KnobKind::Chicken:
        {
            const R capR = body.reduced (inner * 0.12f);
            radial (capR, 0.4f, 0.3f, col ("#3a3633"), col ("#0d0c0b"), 0.75f, col ("#0d0c0b"), true);
            const R head (body.getX() + inner * 0.33f, body.getY() - inner * 0.06f, inner * 0.34f, inner * 1.12f);
            g.setGradientFill (juce::ColourGradient::horizontal (col ("#0c0b0a"), head.getX(), col ("#0c0b0a"), head.getRight()));
            { juce::ColourGradient gr (col ("#0c0b0a"), head.getX(), 0, col ("#0c0b0a"), head.getRight(), 0, false); gr.addColour (0.45, col ("#2c2926")); g.setGradientFill (gr); }
            g.fillRoundedRectangle (head, head.getWidth() * 0.42f);
            g.setColour (white.withAlpha (0.3f)); g.drawRoundedRectangle (head.reduced (0.5f), head.getWidth() * 0.42f, 1.0f);
            line (3, -0.02f, 0.4f, white, true);
            break;
        }
        case KnobKind::Skirt:
        {
            for (int k = 0; k < 72; ++k)
            {
                juce::Path w; w.addPieSegment (body, (float) k * 5.0f * pi / 180.0f, (float) (k + 1) * 5.0f * pi / 180.0f, 0.0f);
                g.setColour (col (k % 2 == 0 ? "#1b1b1b" : "#3a3a3a")); g.fillPath (w);
            }
            g.setColour (white.withAlpha (0.08f)); g.drawEllipse (body.reduced (0.5f), 1.0f);
            rim (body);
            const R head = body.reduced (inner * 0.24f);
            radial (head, 0.4f, 0.3f, col (U.cap[0]), col (U.cap[1]), 0.72f, col (U.cap[1]), true);
            line (3, 0.02f, 0.3f, white, true);
            break;
        }
        case KnobKind::Knurl:
        {
            for (int k = 0; k < 120; ++k)
            {
                juce::Path w; w.addPieSegment (body, (float) k * 3.0f * pi / 180.0f, (float) (k + 1) * 3.0f * pi / 180.0f, 0.0f);
                g.setColour (k % 2 == 0 ? cap0 : juce::Colours::black.withAlpha (0.55f)); g.fillPath (w);
            }
            rim (body);
            const R head = body.reduced (inner * 0.14f);
            juce::ColourGradient gr (white.withAlpha (0.55f), head.getX() + head.getWidth() * 0.38f, head.getY() + head.getHeight() * 0.3f,
                                     juce::Colours::black.withAlpha (0.35f), head.getX() + head.getWidth() * 0.38f + head.getWidth() * 0.75f, head.getY() + head.getHeight() * 0.3f, true);
            gr.addColour (0.45, cap0);
            g.setGradientFill (gr); g.fillEllipse (head);
            const R l (body.getCentreX() - 2, body.getY() + inner * 0.06f, 4, inner * 0.36f);
            g.setColour (white.withAlpha (0.35f)); g.fillRect (l.expanded (1));
            g.setColour (col ("#050505")); g.fillRect (l);
            break;
        }
    }
    g.restoreState();
}

void Canvas::paintFaceplate (juce::Graphics& g, R plate, int unit)
{
    const auto& U = units()[(size_t) unit];
    const auto ch = proc.chain();
    const auto it = std::find (ch.begin(), ch.end(), unit);
    const int slot = it == ch.end() ? -1 : (int) (it - ch.begin());
    // the Tron outline (a unit that isn't in the chain yet), breathing in its own colour
    if (slot < 0)
    {
        const auto c = col (U.col);
        const R o = plate.expanded (3.75f);
        const float t = pulse;
        const auto edge = c.withAlpha (0.35f).interpolatedWith (c.interpolatedWith (juce::Colours::white, 0.3f).withAlpha (0.7f), t);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (c.withAlpha ((0.06f + 0.16f * t) / (float) i));
            g.drawRoundedRectangle (o.expanded ((float) i * (2.0f + 4.0f * t)), 9.0f + (float) i * 3.0f, 2.0f + (float) i * 3.0f * t);
        }
        g.setColour (edge); g.drawRoundedRectangle (o, 9.0f, 1.5f);
    }
    shadow (g, plate, 5, 6, 18, juce::Colours::black.withAlpha (0.35f));
    g.saveState();
    { juce::Path cl; cl.addRoundedRectangle (plate, 5); g.reduceClipRegion (cl); }
    // the rack ears + screws
    for (int side = 0; side < 2; ++side)
    {
        const R ear (side == 0 ? plate.getX() : plate.getRight() - 14, plate.getY(), 14, plate.getHeight());
        juce::ColourGradient gr (juce::Colour ((juce::uint8) 69, (juce::uint8) 66, (juce::uint8) 61, 0.75f), side == 0 ? ear.getX() : ear.getRight(), 0,
                                 juce::Colour ((juce::uint8) 42, (juce::uint8) 40, (juce::uint8) 37, 0.75f), side == 0 ? ear.getRight() : ear.getX(), 0, false);
        g.setGradientFill (gr); g.fillRect (ear);
        for (int k = 0; k < 2; ++k)
        {
            const R sc (ear.getX() + 3, ear.getY() + (k == 0 ? 9.0f : plate.getHeight() - 17.0f), 8, 8);
            const char* sc0 = U.wear > 0.85f ? "#9a6236" : U.wear > 0.5f ? "#6b665e" : "#b9bcc0";
            const char* sc1 = U.wear > 0.85f ? "#4a2a14" : U.wear > 0.5f ? "#2c2925" : "#55595e";
            juce::ColourGradient sg (col (sc0), sc.getX() + 3.2f, sc.getY() + 2.8f, col (sc1), sc.getX() + 3.2f + 6.0f, sc.getY() + 2.8f, true);
            g.setGradientFill (sg); g.fillEllipse (sc);
            const float ang = (20.0f + (float) unit * 37.0f) * pi / 180.0f;
            g.setColour (col ("#141210"));
            g.drawLine (juce::Line<float> (sc.getCentre() - P (std::cos (ang), std::sin (ang)) * 3.6f, sc.getCentre() + P (std::cos (ang), std::sin (ang)) * 3.6f), 1.3f);
        }
    }
    const R panel (plate.getX() + 14, plate.getY(), 772, plate.getHeight());
    g.drawImage (metalImage (unit, 772, (int) plate.getHeight()), panel);
    g.restoreState();
    // the faceplate's own items: name, knobs, switches, slot / ADD (space-between, padding 0 18 0 22)
    auto fName = font (Face::CondMedium, 20, 2), fDesc = font (Face::Regular, 10), fLab = font (Face::CondSemiBold, 10, 1.2f), fVal = font (Face::Medium, 10);
    const float s = U.kind == KnobKind::Neve ? 42.0f : 40.0f, Rr = s + 14;
    float v[unitValues]; proc.getUnitValues (unit, v, unit == preview && proc.unitSplit (unit) ? charSide : 0);   // V9: a split unit's side on show
    if (plate.getHeight() > 120.0f && ! unitLayout2U (unit).empty())
    {   // V11: two-unit gear, laid out like the original - big gain knobs on top, the frequency selectors under them
        auto stext2 = [&] (const juce::String& t, const juce::Font& f, juce::Colour c, R box, juce::Justification j)
        { text (g, t, f, juce::Colours::black.withAlpha (0.6f), box.translated (0, 1), j); text (g, t, f, c, box, j); };
        const auto& lay = unitLayout2U (unit);
        float maxCol = 0; for (auto& k : lay) maxCol = juce::jmax (maxCol, k.col);
        const float x0 = plate.getX() + 14 + 22, cyN = plate.getCentreY();
        {   // the name block, centred top to bottom
            const float top = cyN - (24 + 3 + 12) * 0.5f;
            stext2 (U.name, fName, col (U.ink), R (x0, top, 200, 24), juce::Justification::centredLeft);
            const auto led = col (U.led);
            g.setColour (led.withAlpha (0.4f)); g.fillEllipse (x0 - 3, top + 27 + 2 - 3, 14, 14);
            g.setColour (led); g.fillEllipse (x0, top + 27 + 2, 8, 8);
            stext2 (U.desc, fDesc, col (U.sub), R (x0 + 14, top + 27, 150, 12), juce::Justification::centredLeft);
        }
        const juce::String slotText2 = U8 ("slot ") + juce::String (slot + 1);
        const float lastW2 = slot >= 0 ? 10 + 12 + 4 + textWidth (font (Face::Regular, 11), slotText2) + 10 : 40 + 8 + textWidth (font (Face::SemiBold, 12, 1), "ADD");
        const float areaL = x0 + 150, areaR = plate.getX() + 14 + 772 - 18 - lastW2 - 16;
        const float colW = (areaR - areaL) / (maxCol + 1.0f);
        const float rowY[2] { plate.getY() + 6.0f, plate.getY() + plate.getHeight() * 0.5f + 2.0f };
        for (auto& k : lay)
        {
            const float cx = areaL + colW * (k.col + 0.5f);
            const float top = rowY[k.row];
            stext2 (k.label, fLab, col (U.sub), R (cx - 50, top, 100, 12), juce::Justification::centredTop);
            const P kt (cx - Rr * 0.5f, top + 13);
            vintageKnob (g, kt, unit, (int) k.col, v[k.v], 0.5f);
            stext2 (valueTextFor (unit, k.v, v), fVal, col (U.ink), R (cx - 50, top + 14 + Rr, 100, 12), juce::Justification::centredTop);
            zone (R (kt.x, kt.y, Rr, Rr), Hit::CharKnob, k.v);
        }
        const float xs = areaR + 16;
        if (slot >= 0)
        {
            const R b (xs, cyN - 13, lastW2, 26);
            strokeRound (g, b, 13, juce::Colours::white.withAlpha (0.25f));
            icon (g, Icon::Check, R (b.getX() + 10, cyN - 6, 12, 12), col (U.ink), 1.5f);
            text (g, slotText2, font (Face::Regular, 11), col (U.ink), R (b.getX() + 26, cyN - 8, 100, 16));
        }
        else
        {
            const R c (xs, cyN - 20, 40, 40);
            g.setColour (amber.withAlpha (0.2f)); g.fillEllipse (c.expanded (6));
            g.setColour (amber); g.fillEllipse (c);
            g.setColour (col ("#1a1208")); g.fillRect (c.getCentreX() - 6.5f, c.getCentreY() - 1.0f, 13.0f, 2.0f); g.fillRect (c.getCentreX() - 1.0f, c.getCentreY() - 6.5f, 2.0f, 13.0f);
            text (g, "ADD", font (Face::SemiBold, 12, 1), amber, R (xs + 48, cyN - 8, 60, 16));
            zone (R (xs - 6, cyN - 26, lastW2 + 12, 52), Hit::UnitAdd);
        }
        return;
    }
    std::vector<float> widths;
    widths.push_back (130);
    std::vector<juce::String> vals;
    for (int k = 0; k < (int) U.knobs.size(); ++k)
    {
        vals.push_back (unitValueText (unit, k, v));
        widths.push_back (juce::jmax (Rr, textWidth (fLab, U.knobs[(size_t) k].label), textWidth (fVal, vals.back())));
    }
    float swW = 0;
    if (! U.switches.empty()) { for (auto* t : U.switches) swW = juce::jmax (swW, 14 + 6 + textWidth (font (Face::CondSemiBold, 10, 1), t)); widths.push_back (swW); }
    const juce::String slotText = U8 ("slot ") + juce::String (slot + 1);
    const float lastW = slot >= 0 ? 10 + 12 + 4 + textWidth (font (Face::Regular, 11), slotText) + 10 : 40 + 8 + textWidth (font (Face::SemiBold, 12, 1), "ADD");
    widths.push_back (lastW);
    float total = 0; for (float w : widths) total += w;
    const float gap = (772 - 40 - total) / (float) (widths.size() - 1);
    float x = plate.getX() + 14 + 22;
    const float cy = plate.getBottom() - 46.0f;   // V10: the controls live in the bottom unit
    // shadowed text helper
    auto stext = [&] (const juce::String& t, const juce::Font& f, juce::Colour c, R box, juce::Justification j)
    {
        text (g, t, f, juce::Colours::black.withAlpha (0.6f), box.translated (0, 1), j);
        text (g, t, f, c, box, j);
    };
    {
        const float top = cy - (24 + 3 + 12) * 0.5f;
        stext (U.name, fName, col (U.ink), R (x, top, 200, 24), juce::Justification::centredLeft);
        const auto led = col (U.led);
        g.setColour (led.withAlpha (0.4f)); g.fillEllipse (x - 3, top + 27 + 2 - 3, 14, 14);
        g.setColour (led); g.fillEllipse (x, top + 27 + 2, 8, 8);
        stext (U.desc, fDesc, col (U.sub), R (x + 14, top + 27, 200, 12), juce::Justification::centredLeft);
        x += 130 + gap;
    }
    for (int k = 0; k < (int) U.knobs.size(); ++k)
    {
        const float w = widths[(size_t) k + 1];
        const float top = cy - (12 + 1 + Rr + 1 + 12) * 0.5f;
        stext (U.knobs[(size_t) k].label, fLab, col (U.sub), R (x - 10, top, w + 20, 12), juce::Justification::centredTop);
        const P kt (x + (w - Rr) * 0.5f, top + 13);
        const auto& kd = U.knobs[(size_t) k];
        vintageKnob (g, kt, unit, k, v[kd.v], kd.v2 >= 0 ? v[kd.v2] : 0.5f);
        stext (vals[(size_t) k], fVal, col (U.ink), R (x - 14, top + 14 + Rr, w + 28, 12), juce::Justification::centredTop);
        zone (R (kt.x, kt.y, Rr, Rr), Hit::UnitKnob, k, kd.v2 >= 0 ? 1 : 0);
        x += w + gap;
    }
    if (! U.switches.empty())
    {
        const float top = cy - (3 * 12 + 2 * 4) * 0.5f;
        for (int i = 0; i < (int) U.switches.size(); ++i)
        {
            const bool on = v[8 + i] > 0.5f;
            const R b (x, top + (float) i * 16 + 1.5f, 14, 9);
            if (on) { g.setColour (col (U.led).withAlpha (0.5f)); g.fillRoundedRectangle (b.expanded (2.5f), 4); }
            g.setColour (on ? col (U.led) : col ("#333333")); g.fillRoundedRectangle (b, 2);
            g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (b.reduced (0.5f), 2, 1);
            text (g, U.switches[(size_t) i], font (Face::CondSemiBold, 10, 1), col (U.sub), R (x + 20, top + (float) i * 16, 100, 12));
            zone (R (x, top + (float) i * 16, swW, 12), Hit::UnitSwitch, i);
        }
        x += swW + gap;
    }
    if (slot >= 0)
    {
        const R b (x, cy - 13, lastW, 26);
        strokeRound (g, b, 13, juce::Colours::white.withAlpha (0.25f));
        icon (g, Icon::Check, R (b.getX() + 10, cy - 6, 12, 12), col (U.ink), 1.5f);
        text (g, slotText, font (Face::Regular, 11), col (U.ink), R (b.getX() + 26, cy - 8, 100, 16));
    }
    else
    {
        const R c (x, cy - 20, 40, 40);
        g.setColour (amber.withAlpha (0.2f)); g.fillEllipse (c.expanded (6));
        g.setColour (amber); g.fillEllipse (c);
        g.setColour (col ("#1a1208")); g.fillRect (c.getCentreX() - 6.5f, c.getCentreY() - 1.0f, 13.0f, 2.0f); g.fillRect (c.getCentreX() - 1.0f, c.getCentreY() - 6.5f, 2.0f, 13.0f);
        text (g, "ADD", font (Face::SemiBold, 12, 1), amber, R (x + 48, cy - 8, 60, 16));
        zone (R (x - 6, cy - 26, lastW + 12, 52), Hit::UnitAdd);
    }
}

void Canvas::paintCharTab (juce::Graphics& g)
{
    if (charSoloUnit != preview) { charSolo = -1; charSoloUnit = -1; }
    if (! proc.unitSplit (preview)) charSide = 0;
    const P o (display.getX(), display.getY());
    const auto ch = proc.chain();
    const auto& U = units()[(size_t) preview];
    const auto uc = col (U.col);

    // V2: the nodes of the unit on show, on its curve - drag them like Tahor's bands (locked to the unit's own steps)
    // V9: a split unit shows both sides' nodes (tagged L / R); a boost and a cut on one frequency sit a little apart
    {
        const float rng = dbRange (proc);
        auto yOf = [rng] (float db) { return 335.0f - juce::jlimit (-rng * 0.95f, rng * 0.95f, db) / rng * 335.0f; };
        const bool split = proc.unitSplit (preview);
        const auto& nodes = unitNodes (preview);
        for (int pass = 0; pass < (split ? 2 : 1); ++pass)
        {
            const int side = split ? (pass == 0 ? 1 - charSide : charSide) : 0;   // the side on show drawn last (on top)
            const bool front = side == charSide;
            float v[unitValues]; proc.getUnitValues (preview, v, side);
            const auto tagC = side == 0 ? col ("#7fd3ff") : col ("#ff8fb8");
            // the boost / cut pairs: a thin line joins the two dots
            for (int i = 0; i < (int) nodes.size(); ++i)
            {
                const float off = nodePairOffset (preview, i);
                if (off <= 0) continue;
                const auto& nd = nodes[(size_t) i];
                const float x = juce::jlimit (6.0f, 1094.0f, fx (nodeHz (nd, v)));
                for (int j = 0; j < (int) nodes.size(); ++j)
                    if (nodePairOffset (preview, j) < 0 && nodes[(size_t) j].fv == nd.fv)
                    {
                        g.setColour (uc.withAlpha (front ? 0.45f : 0.2f));
                        g.drawLine (x + off, yOf (nodeDb (nd, v)), x + nodePairOffset (preview, j), yOf (nodeDb (nodes[(size_t) j], v)), 1.2f);
                    }
            }
            for (int i = 0; i < (int) nodes.size(); ++i)
            {
                const auto& nd = nodes[(size_t) i];
                const float x = juce::jlimit (6.0f, 1094.0f, fx (nodeHz (nd, v))) + nodePairOffset (preview, i), y = yOf (nodeDb (nd, v));
                const bool off = nodeOff (nd, v), sel = i == selCharNode && front;
                const auto t = juce::String (i + 1);
                if (! front) g.beginTransparencyLayer (0.55f);
                if (sel)
                {
                    g.setColour (uc.withAlpha (0.16f)); g.fillEllipse (x - 19, y - 19, 38, 38);
                    g.setColour (col ("#fff3dc")); g.fillEllipse (x - 11, y - 11, 22, 22);
                    g.setColour (uc); g.fillEllipse (x - 9, y - 9, 18, 18);
                    text (g, t, font (Face::SemiBold, 11), col ("#1a1208"), R (x - 11, y - 11, 22, 22), juce::Justification::centred);
                    juce::String val = nd.gv < 0 ? (off ? juce::String ("off") : hzText (nodeHz (nd, v)))
                                                 : (nd.fv >= 0 ? hzText (nodeHz (nd, v)) + DOT : juce::String()) + num (nodeDb (nd, v)) + " dB";
                    const auto lab = juce::String (nd.name) + DOT + val;
                    auto f = font (Face::Regular, 12);
                    const R b (x - 30, y - 50, textWidth (f, lab) + 20, 22.4f);
                    if (reveal > 0.01f)
                    {
                        g.beginTransparencyLayer (reveal);
                        fillRound (g, b, 12, col ("#0e0e10", 0.9f)); strokeRound (g, b, 12, uc.withAlpha (0.6f));
                        text (g, lab, f, juce::Colours::white, b.reduced (10, 0));
                        g.endTransparencyLayer();
                    }
                }
                else
                {
                    g.setColour (col ("#141418")); g.fillEllipse (x - 9, y - 9, 18, 18);
                    g.setColour (uc.withAlpha (off ? 0.4f : 1.0f)); g.drawEllipse (x - 7.75f, y - 7.75f, 15.5f, 15.5f, 2.5f);
                    text (g, t, font (Face::Regular, 10), uc.withAlpha (off ? 0.5f : 1.0f), R (x - 9, y - 9, 18, 18), juce::Justification::centred);
                }
                if (split)
                {
                    const R tg (x + 8, y - 20, 12, 11);
                    fillRound (g, tg, 3, tagC);
                    text (g, side == 0 ? "L" : "R", font (Face::SemiBold, 8.5f), col ("#101014"), tg, juce::Justification::centred);
                }
                if (! front) g.endTransparencyLayer();
                zone (R (x - 12, y - 12, 24, 24).translated (o.x, o.y), Hit::CharNode, i, side);
            }
        }
    }

    const float sc = 0.4f;                      // V4: the hardware at 40% of its size, solid, shown only while a band is touched
    const float plateH = unitPlateH (preview);
    const float bw = 905.0f * sc, bh = (plateH + 9.0f) * sc;   // V10: two-unit gear is taller (the extra unit on top)
    const float kp = 0.84f;                     // V6: the node panel, 20% bigger, at the bottom
    const float hwTop = 662.0f - 76.0f * kp - 8.0f - bh;   // the hardware always on show, just above the panels' row
    auto zChar = zones.size();
    // the hardware: < faceplate >, 60% of its size, very see-through until the mouse is over it
    {
        const float left = PW * 0.5f - bw * 0.5f;
        const R area (left, hwTop, bw, bh);
        const R areaS = area.translated (o.x, o.y).expanded (4);
        const bool hv = areaS.contains (mouse) || (dragging && areaS.contains (downAt));
        g.beginTransparencyLayer (1.0f);   // V5: always on screen, solid
        g.saveState();
        const auto tr = juce::AffineTransform::scale (sc).translated (left, hwTop);
        g.addTransform (tr);
        const auto n0 = zones.size();
        hwArrowA += ((hv ? 1.0f : 0.0f) - hwArrowA) * 0.25f;   // V10: the arrows only while the mouse is over the hardware
        if (hwArrowA > 0.02f)
            for (int side = 0; side < 2; ++side)
            {
                const R b (side == 0 ? 0.0f : 905.0f - 36.0f, 4.5f + (plateH - 92.0f) * 0.5f, 36, 92);   // centred on the unit
                g.beginTransparencyLayer (hwArrowA);
                fillRound (g, b, 8, col ("#17181b", 0.75f)); strokeRound (g, b, 8, juce::Colours::white.withAlpha (0.14f));
                icon (g, side == 0 ? Icon::Left : Icon::Right, b.withSizeKeepingCentre (22, 22), ink, 1.6f);
                g.endTransparencyLayer();
                if (hv) zone (b, side == 0 ? Hit::UnitPrev : Hit::UnitNext);
            }
        paintFaceplate (g, R (36 + 12 + 4.5f, 4.5f, 800, plateH), preview);
        g.restoreState();
        {   // V9: a one-word hint, just above the hardware
            auto fh = font (Face::CondMedium, 10, 1.6f);   // V10: plain text, like CLEAN / SUBTLE / WARM
            text (g, juce::String (unitHint (preview)).toUpperCase(), fh, col ("#b9b3a7", 0.55f), R (PW * 0.5f - 100, hwTop - 26, 200, 13), juce::Justification::centred);   // V11: higher, quieter
        }
        for (auto i = n0; i < zones.size(); ++i) zones[i].r = zones[i].r.transformedBy (tr).translated (o.x, o.y);
        g.endTransparencyLayer();
        // the hardware's own zone sits under its controls (so hovering anywhere on it lights it up)
        zones.insert (zones.begin() + (long) n0, Zone { areaS, Hit::Hardware });
    }
    zChar = zones.size();
    // V3: the selected node's panel, fixed at the bottom (like Tahor's and Reso's)
    {
        float v[unitValues]; proc.getUnitValues (preview, v, proc.unitSplit (preview) ? charSide : 0);
        const auto& nodes = unitNodes (preview);
        const int ni = juce::jlimit (0, (int) nodes.size() - 1, selCharNode < 0 ? 0 : selCharNode);
        const auto& nd = nodes[(size_t) ni];
        struct K { const char* lab; int vi; juce::String val; };
        std::vector<K> ks;
        if (nd.fv >= 0) ks.push_back ({ "FREQ", nd.fv, nodeOff (nd, v) ? juce::String ("off") : hzText (nodeHz (nd, v)) });
        if (nd.gv >= 0) ks.push_back ({ "GAIN", nd.gv, num (nodeDb (nd, v)) + " dB" });
        if (nd.qv >= 0) ks.push_back ({ "WIDTH", nd.qv, dial (v[nd.qv] * 10.0f) });
        const float pw = 220 + (float) ks.size() * 66 + 12 + 38 + 82;   // V8: + Solo   // V9: + ST / L / R
        const float nx = juce::jlimit (6.0f, PW - 6.0f, fx (nodeHz (nd, v)));
        // V5: small, at the bottom of the screen, sliding along to sit under the node you touch
        const float px = juce::jlimit (8.0f, PW - 8.0f - pw * kp, nx - pw * kp * 0.5f), py = 662.0f - 76.0f * kp;
        const auto tr = juce::AffineTransform::scale (kp).translated (px, py);
        const R b (0, 0, pw, 76);
        const R bs = R (px, py, pw * kp, 76 * kp).translated (o.x, o.y);
        if (reveal > 0.05f) revealRects.push_back (bs);
        const auto zp = zones.size();
        g.beginTransparencyLayer (reveal);
        g.saveState(); g.addTransform (tr);
        bandPanel (g, b, (nx - px) / kp, uc);
        const float cy = b.getCentreY();
        g.setColour (uc); g.fillEllipse (b.getX() + 14, cy - 13, 8, 8);
        text (g, juce::String (U.name) + DOT + juce::String (ni + 1) + DOT + nd.name, font (Face::Regular, 13), ink, R (b.getX() + 28, cy - 18, 200, 16));
        text (g, nd.fv < 0 && nd.gv >= 0 ? "fixed frequency, the hardware's gain" : "locked to the hardware's steps", font (Face::Regular, 11), grey, R (b.getX() + 14, cy + 2, 200, 14));
        float x = b.getX() + 220;
        for (auto& k : ks)
        {
            const float top = cy - (42 + 4 + 14.4f + 4 + 12) * 0.5f;
            bandKnob (g, R (x + 5, top, 42, 42), v[k.vi], uc);
            text (g, k.val, font (Face::Regular, 12), ink, R (x - 10, top + 46, 72, 14.4f), juce::Justification::centredTop);
            text (g, k.lab, font (Face::Regular, 10, 1), grey, R (x - 10, top + 64.4f, 72, 12), juce::Justification::centredTop);
            zone (R (x + 5, top, 42, 42).translated (o.x, o.y), Hit::CharKnob, k.vi);
            x += 66;
        }
        {   // V9: ST / L / R - split the whole unit into left and right, each with its own settings
            const int cs = proc.unitSplit (preview) ? 1 + charSide : 0;
            const char* ps[3] { "ST", "L", "R" };
            const R seg (b.getRight() - 12 - 28 - 8 - 72, cy - 11, 72, 22);
            strokeRound (g, seg, 5, col ("#34353b"));
            for (int i = 0; i < 3; ++i)
            {
                const R cc (seg.getX() + (float) i * 24, seg.getY(), 24, 22);
                if (cs == i) { g.saveState(); juce::Path cp; cp.addRoundedRectangle (seg.reduced (1), 4); g.reduceClipRegion (cp); g.setColour (col ("#2b2c31")); g.fillRect (cc); g.restoreState(); }
                text (g, ps[i], font (Face::Regular, 11), cs == i ? (i == 1 ? col ("#7fd3ff") : i == 2 ? col ("#ff8fb8") : ink) : grey, cc, juce::Justification::centred);
                zone (cc.translated (o.x, o.y), Hit::CharSide, i);
            }
        }
        {   // V8: Solo - hear just this node's range
            const bool so = charSolo == ni && charSoloUnit == preview;
            const R sb (b.getRight() - 12 - 28, cy - 14, 28, 28);
            fillRound (g, sb, 6, so ? col ("#3a2c14") : col ("#232327")); strokeRound (g, sb, 6, so ? amber : col ("#34353b"));
            text (g, "S", font (Face::SemiBold, 12), so ? amber : grey, sb, juce::Justification::centred);
            zone (sb.translated (o.x, o.y), Hit::BandSolo, 1, ni);
            if (so) { soloHzUi = juce::jlimit (20.0f, 20000.0f, nodeHz (nd, v)); soloQUi = nd.qv >= 0 ? 0.5f + v[nd.qv] * 3.0f : 1.2f; }
        }
        g.restoreState();
        for (auto i = zp; i < zones.size(); ++i) zones[i].r = zones[i].r.translated (-o.x, -o.y).transformedBy (tr).translated (o.x, o.y);
        g.endTransparencyLayer();
    }
    if (reveal < 0.05f) zones.resize (zChar);

    // V4: the chain chips stack down the left side, faint until the mouse is over them
    {
        auto f = font (Face::Regular, 11), fo = font (Face::Regular, 9, 1);
        float cw = 0;
        for (size_t j = 0; j < ch.size(); ++j)
            cw = juce::jmax (cw, 10 + 10 + 7 + textWidth (f, juce::String ((int) j + 1) + DOT + juce::String (units()[(size_t) ch[j]].name).replace ("LE-", "")) + 7 + 16 + 6);
        const float x = 10, y0 = 40;
        const R col_ (x - 4, y0 - 4, cw + 8, (float) ch.size() * 30.0f + 22);
        const bool hv = col_.translated (o.x, o.y).contains (mouse) || (dragging && down.kind == Hit::ChainChip);
        g.beginTransparencyLayer (hv ? 1.0f : 0.35f);
        for (size_t j = 0; j < ch.size(); ++j)
        {
            const int u = ch[j];
            const bool sel = u == preview;
            const auto t = juce::String ((int) j + 1) + DOT + juce::String (units()[(size_t) u].name).replace ("LE-", "");
            const float y = y0 + (float) j * 30.0f;
            const R c (x, y, cw, 24);
            fillRound (g, c, 12, sel ? col ("#3a2c14", 0.9f) : col ("#17181b", 0.85f));
            strokeRound (g, c, 12, sel ? col ("#6b4f1f") : juce::Colours::white.withAlpha (0.12f));
            g.setColour (col (units()[(size_t) u].col)); g.fillEllipse (x + 8, y + 9, 6, 6);
            icon (g, Icon::Grip, R (x + 16, y + 7, 10, 10), grey);
            text (g, t, f, sel ? amber : ink, R (x + 30, y, 200, 24));
            const R xb (c.getRight() - 5 - 15, y + 4.5f, 15, 15);
            g.setColour (juce::Colours::white.withAlpha (0.1f)); g.fillEllipse (xb);
            icon (g, Icon::Cross, xb.reduced (4), ink, 1.2f);
            zone (c.translated (o.x, o.y), Hit::ChainChip, (int) j);
            zone (xb.translated (o.x, o.y), Hit::ChainRemove, (int) j);
        }
        text (g, U8 ("\xe2\x86\x93 OUT"), fo, grey, R (x + 10, y0 + (float) ch.size() * 30.0f - 2, 80, 14));
        g.endTransparencyLayer();
        // V7: + Add EQ, always on show
        {
            const bool full = (int) ch.size() >= maxChain;
            const R ab (x, y0 + (float) ch.size() * 30.0f + 18, juce::jmax (cw, 96.0f), 24);
            const bool hvA = ab.translated (o.x, o.y).contains (mouse);
            fillRound (g, ab, 12, full ? col ("#17181b", 0.7f) : hvA ? col ("#3a2c14", 0.95f) : col ("#241c10", 0.92f));
            strokeRound (g, ab, 12, full ? juce::Colours::white.withAlpha (0.1f) : col (hvA ? "#c08a35" : "#6b4f1f"));
            auto fa = font (Face::Medium, 11);
            const juce::String t = full ? juce::String ("Chain full") : juce::String ("+  Add EQ");
            text (g, t, fa, full ? grey : amber, ab, juce::Justification::centred);
            zone (ab.translated (o.x, o.y), Hit::AddEq);
        }
    }
}

//==============================================================================
// V2: the input meter on the left of the display, the output meter on the right
void Canvas::paintMeters (juce::Graphics& g)
{
    // V10: peak (held) and average readouts at the bottom of each meter - input left, output right
    for (int side = 0; side < 2; ++side)
    {
        auto f = font (Face::Medium, 9), fl = font (Face::Regular, 7.5f, 0.8f);
        auto t = [] (float db) { return db <= -89.0f ? juce::String (U8 ("\xe2\x88\x92\xe2\x88\x9e")) : (db < -0.05f ? MINUS : juce::String()) + juce::String (std::abs (db), 1); };
        const auto j = side == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight;
        const R a (side == 0 ? 4.0f : W - 64.0f, 743, 60, 10), b2 (side == 0 ? 4.0f : W - 64.0f, 753, 60, 10);
        const bool hot = pkHold[side] > -0.3f;
        text (g, (side == 0 ? "PK " : "") + t (pkHold[side]) + (side == 1 ? " PK" : ""), f, hot ? col ("#ef4444") : ink, a, j);
        text (g, (side == 0 ? "AVG " : "") + t (avgDb[side]) + (side == 1 ? " AVG" : ""), fl, grey, b2, j);
    }
    for (int side = 0; side < 2; ++side)
    {
        const R box (side == 0 ? 3.0f : W - 15.0f, 70, 12, 670);   // V3: thin, at the very edges
        for (int c = 0; c < 2; ++c)
        {
            const float db = meterDb[side * 2 + c];
            const R bar (box.getX() + 1 + (float) c * 6, box.getY(), 4, box.getHeight());
            fillRound (g, bar, 2, juce::Colours::white.withAlpha (0.05f));
            const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            if (frac > 0.0f)
            {
                const R fill (bar.getX(), bar.getBottom() - bar.getHeight() * frac, 4, bar.getHeight() * frac);
                g.saveState(); juce::Path cl; cl.addRoundedRectangle (bar, 2); g.reduceClipRegion (cl);
                juce::ColourGradient gr (col ("#4ade80"), 0, fill.getBottom(), col ("#ef4444"), 0, fill.getY(), false);
                gr.addColour (0.7, col ("#4ade80")); gr.addColour (0.92, col ("#f3a93c"));
                g.setGradientFill (gr); g.fillRect (fill);
                g.restoreState();
            }
        }
    }
}

void Canvas::paintBottomBar (juce::Graphics& g)
{
    g.setColour (col ("#0e0e10")); g.fillRect (0.0f, 764.0f, W, 36.0f);   // V4: a slim bottom bar (36 px)
    const float cy = 782.0f;
    auto f11 = font (Face::Regular, 10.5f), f11m = font (Face::Medium, 10.5f, 0.3f), f9 = font (Face::Regular, 9), f11b = font (Face::Medium, 10.5f);
    auto dbText = [] (float v) { return (v < -0.05f ? MINUS : v > 0.05f ? juce::String ("+") : juce::String()) + juce::String (std::abs (v), 1) + " dB"; };
    const bool learning = proc.autoLearning.load();
    const bool sc = proc.param ("scExt") > 0.5f;
    const int ph = (int) proc.param ("phase");
    const char* names[3] { "Natural Phase", "Linear Phase", "Linear Phase HQ" };
    const char* lats[3] { "low latency", "medium latency", "high latency" };
    const char* osL[6] { "Off", "2x", "4x", "8x", "16x", "32x" };
    const char* osR[6] { "Same", "2x", "4x", "8x", "16x", "32x" };
    const char* an[4] { "Pre + Post", "Post", "Pre", "Off" };
    static const float scl[5] { 0.7f, 0.85f, 1.0f, 1.2f, 1.4f };
    static const char* snm[5] { "Mini", "Small", "Medium", "Large", "Extra Large" };
    juce::String sz = fullScreen ? juce::String ("Full screen") : juce::String ("Custom");
    if (! fullScreen) for (int i = 0; i < 5; ++i) if (std::abs (currentScale - scl[i]) < 0.01f) sz = snm[i];

    // the pieces, measured: kind 0 = a label, 1 = a framed box, 2 = a button (label + value), 3 = the phase button, 4 = "Label: value"
    struct Item { int kind; juce::String a, b; Hit h; juce::Colour border, colour; float w = 0; };
    auto measure = [&] (Item& it)
    {
        switch (it.kind)
        {
            case 0: it.w = textWidth (f11, it.a); break;
            case 1: it.w = 14 + textWidth (f11, it.a); break;
            case 2: it.w = 8 + textWidth (f11, it.a) + 5 + textWidth (f11b, it.b) + 8; break;
            case 3: it.w = 9 + 11 + 6 + textWidth (f11m, it.a) + 6 + textWidth (f9, it.b) + 9; break;
            default: it.w = 4 + textWidth (f11, it.a) + textWidth (f11, it.b) + 4; break;
        }
    };
    auto draw = [&] (Item& it, float x)
    {
        switch (it.kind)
        {
            case 0: text (g, it.a, f11, grey, R (x, cy - 7, it.w + 2, 14)); break;
            case 1: { const R b (x, cy - 9, it.w, 18); strokeRound (g, b, 4, it.border); text (g, it.a, f11, it.colour, b, juce::Justification::centred); zone (b, it.h); break; }
            case 2: { const R b (x, cy - 10, it.w, 20); fillRound (g, b, 5, col ("#1b1c20")); strokeRound (g, b, 5, col ("#34353b"));
                      text (g, it.a, f11, grey, R (x + 8, cy - 7, 200, 14)); text (g, it.b, f11b, ink, R (x + 8 + textWidth (f11, it.a) + 5, cy - 7, 100, 14)); zone (b, it.h); break; }
            case 3: { const R b (x, cy - 10, it.w, 20); fillRound (g, b, 5, col ("#1b1c20")); strokeRound (g, b, 5, col ("#34353b"));
                      icon (g, Icon::Swap, R (x + 9, cy - 5.5f, 11, 11), amber, 1.2f);
                      text (g, it.a, f11m, ink, R (x + 26, cy - 7, 200, 14)); text (g, it.b, f9, grey, R (x + 26 + textWidth (f11m, it.a) + 6, cy - 6, 120, 12)); zone (b, it.h); break; }
            default: { text (g, it.a, f11, grey, R (x + 4, cy - 7, 200, 14)); text (g, it.b, f11, ink, R (x + 4 + textWidth (f11, it.a), cy - 7, 200, 14)); zone (R (x, cy - 11, it.w, 22), it.h); break; }
        }
    };
    const auto border = col ("#34353b");
    // V5: two groups, either side of the arch - left: IN, AUTO, phase, oversampling; right: analyzer, range, size, SC, OUT
    std::vector<Item> left {
        { 0, "IN", {}, Hit::None, {}, {} },
        { 1, dbText (proc.param ("inGain")), {}, Hit::InBox, border, ink },
        { 1, "AUTO", {}, Hit::Auto, learning ? col ("#6b4f1f") : border, learning ? amber : grey },
        { 3, names[ph], lats[ph], Hit::Phase, {}, {} },
        { 2, "Oversampling", osL[(int) proc.param ("os")], Hit::Os, {}, {} },
        { 2, "Render", osR[(int) proc.param ("osRender")], Hit::OsRender, {}, {} } };
    std::vector<Item> right {
        { 4, "Analyzer: ", an[(int) proc.param ("analyzer")], Hit::Analyzer, {}, {} },
        { 4, "Range: ", proc.apvts.getParameter ("range")->getCurrentValueAsText(), Hit::Range, {}, {} },
        { 4, "Size: ", sz, Hit::Size, {}, {} },
        { 0, "SC", {}, Hit::None, {}, {} },
        { 1, "Ext", {}, Hit::ScExt, sc ? col ("#2f4a63") : border, sc ? col ("#8ec5ff") : grey },
        { 0, "OUT", {}, Hit::None, {}, {} },
        { 1, dbText (proc.param ("outGain")), {}, Hit::OutBox, border, ink } };
    const float gaps[] { 5, 5, 12, 5, 5 };
    float x = 20;
    for (size_t i = 0; i < left.size(); ++i) { measure (left[i]); draw (left[i], x); x += left[i].w + (i < 5 ? gaps[i] : 0); }
    archBot[0] = x + 16;
    float tot = 0;
    const float rg[] { 4, 10, 12, 5, 10, 5 };
    for (size_t i = 0; i < right.size(); ++i) { measure (right[i]); tot += right[i].w + (i + 1 < right.size() ? rg[i] : 0); }
    x = W - 20 - tot;
    archBot[1] = x - 16;
    for (size_t i = 0; i < right.size(); ++i) { draw (right[i], x); x += right[i].w + (i + 1 < right.size() ? rg[i] : 0); }
}

float Canvas::activeResoReduction() { return proc.activeReso().reductionNow.load(); }
