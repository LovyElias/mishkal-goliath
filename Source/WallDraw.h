#pragma once
// the mock's little drawing words (text, fillR, strokeR, rgba, mix ...) in JUCE, so the screen is drawn with the same numbers
#include "WallEditor.h"
#include "BinaryData.h"

namespace wallui
{
inline juce::Colour hex (juce::uint32 rgb, float a = 1.0f) { return juce::Colour (0xff000000u | rgb).withAlpha (a); }
inline juce::Colour mixc (juce::Colour a, juce::Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t)); }
inline juce::Colour mixc (juce::uint32 a, juce::uint32 b, float t) { return mixc (hex (a), hex (b), t); }

// the mock's colours
inline const juce::Colour ink = hex (0xece6da), grey = hex (0x8d8a84), dim = hex (0x5f5c57), amber = hex (0xf3a93c), ivory = hex (0xf4efe4),
                          red = hex (0xff5a4a), coral = hex (0xff7a5c), label = hex (0xb9b3a7), faint = hex (0x77746e), soft = hex (0x9a968f),
                          orange = hex (0xff9a2e), gold = hex (0xffcf7a), green = hex (0x4ade80), mint = hex (0x7cf0a8), spot = hex (0x1ed760);

inline juce::Typeface::Ptr typeface (Face f)
{
    static juce::Typeface::Ptr t[6] {
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansLight_ttf, (size_t) BinaryData::FiraSansLight_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansRegular_ttf, (size_t) BinaryData::FiraSansRegular_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansMedium_ttf, (size_t) BinaryData::FiraSansMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansSemiBold_ttf, (size_t) BinaryData::FiraSansSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedMedium_ttf, (size_t) BinaryData::BarlowCondensedMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, (size_t) BinaryData::BarlowCondensedSemiBold_ttfSize) };
    return t[(int) f];
}
inline juce::Font font (Face f, float px, float ls = 0.0f)
{
    juce::Font fo (juce::FontOptions (typeface (f)).withPointHeight (px));
    if (ls != 0.0f) fo.setExtraKerningFactor (ls / px);
    return fo;
}
inline float tw (const juce::String& s, Face f, float px, float ls = 0.0f)
{
    juce::GlyphArrangement ga; ga.addLineOfText (font (f, px, ls), s, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}
// text with its middle on y ('l', 'c' or 'r' on x), as the mock's text()
inline void text (juce::Graphics& g, const juce::String& s, float x, float y, Face f, float px, juce::Colour c, char al = 'l', float ls = 0.0f)
{
    g.setColour (c); g.setFont (font (f, px, ls));
    const float h = px * 2.0f;
    if (al == 'c') g.drawText (s, R (x - 600.0f, y - h * 0.5f, 1200.0f, h), juce::Justification::centred, false);
    else if (al == 'r') g.drawText (s, R (x - 1200.0f, y - h * 0.5f, 1200.0f, h), juce::Justification::centredRight, false);
    else g.drawText (s, R (x, y - h * 0.5f, 1200.0f, h), juce::Justification::centredLeft, false);
}
// a soft glow behind text (the mock's shadowBlur)
inline void glowText (juce::Graphics& g, const juce::String& s, float x, float y, Face f, float px, juce::Colour c, juce::Colour glow, char al = 'l', float ls = 0.0f)
{
    for (auto [dx, dy] : { std::pair { -1.2f, 0.0f }, { 1.2f, 0.0f }, { 0.0f, -1.2f }, { 0.0f, 1.2f } })
        text (g, s, x + dx, y + dy, f, px, glow.withMultipliedAlpha (0.35f), al, ls);
    text (g, s, x, y, f, px, c, al, ls);
}
inline void fillR (juce::Graphics& g, float x, float y, float w, float h, float r, juce::Colour c) { g.setColour (c); g.fillRoundedRectangle (x, y, w, h, std::min (r, std::min (w, h) * 0.5f)); }
inline void strokeR (juce::Graphics& g, float x, float y, float w, float h, float r, juce::Colour c, float lw = 1.0f)
{
    g.setColour (c); g.drawRoundedRectangle (x + lw * 0.5f, y + lw * 0.5f, w - lw, h - lw, std::min (r, std::min (w, h) * 0.5f), lw);
}
inline void dot (juce::Graphics& g, float x, float y, float r, juce::Colour c) { g.setColour (c); g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f); }
inline void radial (juce::Graphics& g, float x, float y, float r, juce::Colour c0, juce::Colour c1)
{
    g.setGradientFill (juce::ColourGradient (c0, x, y, c1, x + r, y, true));
    g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
}
inline float seed (float i) { const double x = std::sin (i * 12.9898) * 43758.5453; return (float) (x - std::floor (x)); }

// a status light: dark > orange > red
inline void lamp (juce::Graphics& g, float x, float y, float amt, bool big)
{
    const auto c = amt < 0.5f ? mixc (0x7a3a10, 0xff9a2e, amt * 2.0f) : mixc (0xff9a2e, 0xff2d2d, (amt - 0.5f) * 2.0f);
    const float Rr = big ? 10.0f : 3.4f;
    if (amt > 0.05f) radial (g, x, y, Rr * (big ? 3.2f : 3.4f), c.withAlpha (0.55f), c.withAlpha (0.0f));
    dot (g, x, y, Rr, amt > 0.05f ? c : hex (0x2a2a2e));
    if (amt > 0.05f) dot (g, x - Rr * 0.25f, y - Rr * 0.25f, Rr * 0.38f, hex (0xfff4e0, 0.85f));
}
// green when calm, then orange, then red (Gold's clip light)
inline void greenLamp (juce::Graphics& g, float x, float y, float amt)
{
    const auto c = amt < 0.4f ? mixc (0x3ddc84, 0x9be15d, amt / 0.4f) : (amt < 0.7f ? mixc (0xffb347, 0xff8a2a, (amt - 0.4f) / 0.3f) : mixc (0xff8a2a, 0xff2d2d, (amt - 0.7f) / 0.3f));
    radial (g, x, y, 8.0f * 3.2f, c.withAlpha (0.55f), c.withAlpha (0.0f));
    dot (g, x, y, 8.0f, c); dot (g, x - 2.0f, y - 2.0f, 8.0f * 0.38f, juce::Colours::white.withAlpha (0.8f));
}
inline void miniGraph (juce::Graphics& g, float x, float y, float w, float h, const juce::String& lab, bool glass = false)
{
    if (glass)   // V7: a glass window (see-through, a soft highlight at the top, a brighter edge)
    {
        fillR (g, x, y, w, h, 10, hex (0x121016, 0.10f));
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path p; p.addRoundedRectangle (x, y, w, h, 10.0f); g.reduceClipRegion (p);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0, y, juce::Colours::white.withAlpha (0.0f), 0, y + 30, false));
            g.fillRect (x, y, w, 30.0f);
        }
        strokeR (g, x, y, w, h, 10, juce::Colours::white.withAlpha (0.16f));
        g.setColour (juce::Colours::white.withAlpha (0.22f)); g.fillRect (x + 14, y, w - 28, 1.0f);
        if (lab.isNotEmpty()) text (g, lab, x + 10, y + 11, Face::CondMedium, 8, faint, 'l', 1.8f);
        return;
    }
    fillR (g, x, y, w, h, 10, juce::Colours::black.withAlpha (0.28f)); strokeR (g, x, y, w, h, 10, juce::Colours::white.withAlpha (0.07f));
    if (lab.isNotEmpty()) text (g, lab, x + 10, y + 11, Face::CondMedium, 8, faint, 'l', 1.8f);
}
// frosted glass (`clear`: higher = more see-through)
inline void glassPanel (juce::Graphics& g, float x, float y, float w, float h, float clear)
{
    fillR (g, x, y, w, h, 14, hex (0x121016, 0.30f - 0.17f * clear));   // V4: more glass
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path p; p.addRoundedRectangle (x, y, w, h, 14.0f); g.reduceClipRegion (p);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.06f), 0, y, juce::Colours::white.withAlpha (0.0f), 0, y + 40, false));
        g.fillRect (x, y, w, 40.0f);
    }
    strokeR (g, x, y, w, h, 14, juce::Colours::white.withAlpha (0.18f));
    g.setColour (juce::Colours::white.withAlpha (0.24f)); g.fillRect (x + 20, y, w - 40, 1.0f);
}
inline void panelTitle (juce::Graphics& g, float x, float y, const juce::String& name, const juce::String& sub, juce::Colour c, float lampAmt = -1.0f)
{
    float tx = x;
    if (lampAmt >= 0.0f) { lamp (g, x + 8, y, lampAmt, false); tx += 20; } else { dot (g, x + 6, y, 3, c); tx += 16; }
    text (g, name, tx, y, Face::CondMedium, 12, ink, 'l', 2.6f);
    if (sub.isNotEmpty()) text (g, sub, tx + tw (name, Face::CondMedium, 12, 2.6f) + 12, y + 0.5f, Face::CondMedium, 8.5f, grey, 'l', 1.8f);
}
inline juce::String dbs (float v, int dec = 1, bool plus = false)
{
    juce::String s = juce::String (std::abs (v), dec);
    if (v < -0.0001f && s.getFloatValue() != 0.0f) return juce::String::fromUTF8 ("\xe2\x88\x92") + s;
    return (plus && v > 0.0001f ? "+" : "") + s;
}
inline const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92"), middot = juce::String::fromUTF8 ("\xc2\xb7");
}
