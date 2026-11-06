#include "PluginEditor.h"
#include "BinaryData.h"

using namespace vox;
using namespace vox::ui;

// Goliath typefaces (built in, SIL Open Font Licence): Fira Sans for the hardware legends, Barlow for the display
static juce::Typeface::Ptr goliathFace (int which)
{
    static const juce::Typeface::Ptr faces[] {
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansMedium_ttf,   (size_t) BinaryData::FiraSansMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansSemiBold_ttf, (size_t) BinaryData::FiraSansSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowRegular_ttf,    (size_t) BinaryData::BarlowRegular_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowMedium_ttf,     (size_t) BinaryData::BarlowMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiBold_ttf,   (size_t) BinaryData::BarlowSemiBold_ttfSize) };
    return faces[juce::jlimit (0, 4, which)];
}

juce::Font vox::ui::font (float size, bool bold)
{
    if (isGoliath())
        return juce::Font (juce::FontOptions().withTypeface (goliathFace (bold ? 1 : 0)).withHeight (size * 1.12f));
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font vox::ui::screenFont (float size, int weight)
{
    return juce::Font (juce::FontOptions().withTypeface (goliathFace (2 + juce::jlimit (0, 2, weight))).withHeight (size * 1.12f));
}

//==============================================================================
// V13 modern switches
namespace v13
{
    const juce::Colour coral (MISHKAL_ACCENT), teal (MISHKAL_NEON);

    // segmented pill: dark capsule, the chosen segment raised with a coral underline and soft glow
    static void pill (juce::Graphics& g, juce::Rectangle<float> r, const juce::StringArray& names, int index, bool vertical)
    {
        const int n = juce::jmax (1, names.size());
        g.setColour (juce::Colour (0xff111215));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (juce::Colour (0xff34363c));
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
        auto inner = r.reduced (3.0f);
        const float step = (vertical ? inner.getHeight() : inner.getWidth()) / (float) n;
        float size = 12.0f;
        for (auto& t : names)
            while (size > 8.5f && juce::GlyphArrangement::getStringWidth (font (size), t) > (vertical ? inner.getWidth() : step) - 6.0f) size -= 0.5f;
        for (int i = 0; i < n; ++i)
        {
            auto seg = vertical ? juce::Rectangle<float> (inner.getX(), inner.getY() + step * (float) i, inner.getWidth(), step)
                                : juce::Rectangle<float> (inner.getX() + step * (float) i, inner.getY(), step, inner.getHeight());
            const bool sel = i == index;
            if (sel)
            {
                auto hi = seg.reduced (vertical ? 0.0f : 1.0f, vertical ? 1.0f : 0.0f);
                g.setColour (juce::Colour (0xff2c2e33));
                g.fillRoundedRectangle (hi, 5.0f);
                auto line = juce::Rectangle<float> (hi.getWidth() - 8.0f, 2.0f).withCentre ({ hi.getCentreX(), hi.getBottom() - 1.5f });
                g.setColour (coral.withAlpha (0.3f));
                g.fillRoundedRectangle (line.expanded (1.5f, 2.0f), 2.0f);
                g.setColour (coral);
                g.fillRoundedRectangle (line, 1.0f);
            }
            g.setColour (sel ? juce::Colour (0xfff2f0eb) : juce::Colour (0xff7e7e84));
            g.setFont (font (size));
            g.drawText (names[i], seg.withTrimmedBottom (sel ? 2.0f : 0.0f), juce::Justification::centred, false);
        }
    }

    // slide toggle: a capsule that fills coral with the knob slid right when on, grey when off; label to its right
    static void slide (juce::Graphics& g, juce::Rectangle<float> r, bool on, bool highlighted, const juce::String& label)
    {
        const float h = juce::jmin (18.0f, r.getHeight() - 2.0f), w = h * 1.85f;
        float size = 12.0f;
        while (size > 8.5f && juce::GlyphArrangement::getStringWidth (font (size), label) > r.getWidth() - w - 8.0f) size -= 0.5f;
        const float tw = label.isEmpty() ? 0.0f : juce::GlyphArrangement::getStringWidth (font (size), label) + 7.0f;
        const float x0 = r.getCentreX() - (w + tw) * 0.5f;
        auto track = juce::Rectangle<float> (x0, r.getCentreY() - h * 0.5f, w, h);
        if (on)
        {
            g.setColour (coral.withAlpha (0.25f));
            g.fillRoundedRectangle (track.expanded (2.5f), h * 0.5f + 2.5f);
        }
        g.setColour (on ? coral : juce::Colour (highlighted ? 0xff393b41 : 0xff2c2e33));
        g.fillRoundedRectangle (track, h * 0.5f);
        const float d = h - 5.0f;
        auto thumb = juce::Rectangle<float> (d, d).withCentre ({ on ? track.getRight() - h * 0.5f : track.getX() + h * 0.5f, track.getCentreY() });
        g.setColour (on ? juce::Colours::white : juce::Colour (0xff6b6d74));
        g.fillEllipse (thumb);
        if (label.isNotEmpty())
        {
            g.setColour (on || highlighted ? juce::Colour (0xfff2f0eb) : juce::Colour (0xff9da0a8));
            g.setFont (font (size));
            g.drawText (label.toLowerCase(), juce::Rectangle<float> (track.getRight() + 7.0f, r.getY(), tw + 4.0f, r.getHeight()), juce::Justification::centredLeft, false);
        }
    }

    // neon power button: dark capsule, power symbol + word; outline and symbol glow teal when on
    static void power (juce::Graphics& g, juce::Rectangle<float> r, bool on, bool highlighted, const juce::String& offText)
    {
        auto btn = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 28.0f)).reduced (1.5f);
        const float rad = btn.getHeight() * 0.5f;
        g.setColour (juce::Colour (0xff141518));
        g.fillRoundedRectangle (btn, rad);
        if (on)
        {
            for (auto [ex, a] : { std::pair { 5.0f, 0.07f }, std::pair { 3.0f, 0.14f }, std::pair { 1.5f, 0.25f } })
            {
                g.setColour (teal.withAlpha (a));
                g.drawRoundedRectangle (btn.expanded (ex * 0.5f), rad + ex * 0.5f, ex);
            }
            g.setColour (teal.withAlpha (0.10f));
            g.fillRoundedRectangle (btn, rad);
        }
        g.setColour (on ? teal : juce::Colour (highlighted ? 0xff4a4c52 : 0xff34363c));
        g.drawRoundedRectangle (btn, rad, 1.5f);
        const auto col = on ? teal : juce::Colour (0xff6b6d74);
        juce::ignoreUnused (offText);
        const juce::String word = on ? "on" : "off";
        const float fs = 11.5f;
        const float iconD = 11.0f, tw = juce::GlyphArrangement::getStringWidth (font (fs), word);
        const bool showWord = iconD + 5.0f + tw <= btn.getWidth() - 10.0f;
        const float total = showWord ? iconD + 5.0f + tw : iconD;
        const float x0 = btn.getCentreX() - total * 0.5f;
        const auto c = juce::Point<float> (x0 + iconD * 0.5f, btn.getCentreY());
        juce::Path icon;
        icon.addCentredArc (c.x, c.y, iconD * 0.5f, iconD * 0.5f, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        icon.startNewSubPath (c.x, c.y - iconD * 0.62f);
        icon.lineTo (c.x, c.y - iconD * 0.05f);
        g.setColour (col);
        g.strokePath (icon, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (showWord)
        {
            g.setColour (on ? juce::Colour (0xffe9fffb) : juce::Colour (0xff7e7e84));
            g.setFont (font (fs));
            g.drawText (word, juce::Rectangle<float> (x0 + iconD + 5.0f, btn.getY(), tw + 4.0f, btn.getHeight()), juce::Justification::centredLeft, false);
        }
    }
}

void vox::ui::drawBatToggle (juce::Graphics& g, juce::Rectangle<float> r, bool up, const juce::String& top, const juce::String& bottom)
{
    if (MISHKAL_V13)   // V13: a two-way segmented pill instead of the bat switch (stacked when the space is narrow)
    {
        const bool vert = r.getWidth() < 96.0f;
        auto pr = vert ? r.reduced (1.0f).withSizeKeepingCentre (juce::jmin (r.getWidth() - 2.0f, 70.0f), juce::jmin (r.getHeight() - 2.0f, 58.0f))
                       : r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 30.0f));
        v13::pill (g, pr, { top, bottom }, up ? 0 : 1, vert);
        return;
    }
    const float labelH = 16.0f;
    auto topR = r.removeFromTop (labelH), botR = r.removeFromBottom (labelH);
    g.setFont (font (12.5f));
    g.setColour (up ? goliathLegend() : goliathLabel().withAlpha (0.8f));
    g.drawText (top, topR.expanded (12.0f, 0.0f), juce::Justification::centred, false);
    g.setColour (up ? goliathLabel().withAlpha (0.8f) : goliathLegend());
    g.drawText (bottom, botR.expanded (12.0f, 0.0f), juce::Justification::centred, false);

    const auto c = r.getCentre();
    const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    auto circle = [&c] (float rad) { return juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c); };
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (circle (R).translated (0.0f, 2.0f));
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (circle (R));
    const float nut = R * 0.84f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4c), c.x - nut * 0.3f, c.y - nut * 0.4f,
                                             juce::Colour (0xff121213), c.x + nut * 0.5f, c.y + nut * 0.8f, true));
    g.fillEllipse (circle (nut));
    g.setColour (juce::Colours::black);
    g.drawEllipse (circle (nut), 1.0f);
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillEllipse (circle (nut * 0.5f));
    g.setColour (juce::Colour (0xff2c2c2e));
    g.drawEllipse (circle (nut * 0.5f), 1.0f);

    // the bat: a tapered chrome lever from the centre to a rounded tip
    const float dir = up ? -1.0f : 1.0f;
    const float len = R * 1.0f, wBase = R * 0.36f, wTip = R * 0.28f;
    juce::Path bat;
    bat.startNewSubPath (c.x - wBase * 0.5f, c.y);
    bat.lineTo (c.x - wTip * 0.5f, c.y + dir * len);
    bat.lineTo (c.x + wTip * 0.5f, c.y + dir * len);
    bat.lineTo (c.x + wBase * 0.5f, c.y);
    bat.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6e6e70), c.x - wBase * 0.5f, 0.0f, juce::Colour (0xff5a5a5c), c.x + wBase * 0.5f, 0.0f, false));
    g.fillPath (bat);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.0f), c.x - wBase * 0.5f, 0.0f, juce::Colours::white.withAlpha (0.0f), c.x + wBase * 0.5f, 0.0f, false));
    {
        juce::ColourGradient cg (juce::Colour (0xff6e6e70), c.x - wBase * 0.5f, 0.0f, juce::Colour (0xff5a5a5c), c.x + wBase * 0.5f, 0.0f, false);
        cg.addColour (0.45, juce::Colour (0xffe9e9ea));
        g.setGradientFill (cg);
        g.fillPath (bat);
    }
    const auto tip = juce::Rectangle<float> (wTip * 1.15f, wTip * 0.85f).withCentre ({ c.x, c.y + dir * len });
    g.setColour (up ? juce::Colour (0xfff4f4f4) : juce::Colour (0xffbdbdbd));
    g.fillEllipse (tip);
}

//==============================================================================
// Shared drawing helpers
void vox::ui::drawGlowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width)
{
    using PST = juce::PathStrokeType;
    g.setColour (c.withAlpha (0.10f)); g.strokePath (p, PST (width * 3.2f, PST::curved, PST::rounded));
    g.setColour (c.withAlpha (0.22f)); g.strokePath (p, PST (width * 1.9f, PST::curved, PST::rounded));
    g.setColour (c);                   g.strokePath (p, PST (width,        PST::curved, PST::rounded));
}

void vox::ui::drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
{
    const auto area = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (area.translated (0.0f, 0.8f).expanded (0.6f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9c4ba), c.x - r * 0.5f, c.y - r * 0.6f,
                                             juce::Colour (0xff5b5852), c.x + r * 0.6f, c.y + r * 0.8f, true));
    g.fillEllipse (area);
    juce::Path slot;
    slot.addRectangle (-r * 0.75f, -0.6f, r * 1.5f, 1.2f);
    g.setColour (juce::Colour (0xff2a2826));
    g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (c));
}

void vox::ui::drawScreenBackground (juce::Graphics& g, juce::Rectangle<float> r)
{
    if (isGoliath())   // part of the black display: just a fine frame
    {
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRect (r);
        g.setColour (juce::Colour (0xff1f1f22));
        g.drawRect (r, 1.0f);
        return;
    }
    g.setGradientFill (juce::ColourGradient (pal::screen2, r.getX(), r.getY(), pal::screen, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.0f);

    if (isRack())   // faint fine grid texture
    {
        g.setColour (juce::Colours::white.withAlpha (0.012f));
        for (float y = r.getY() + 2.0f; y < r.getBottom(); y += 4.0f)
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    }
    else if (isHardware())   // faint scanlines + inner shadow, like an old analyzer screen
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        for (float y = r.getY() + 1.0f; y < r.getBottom(); y += 3.0f)
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    }
    else if (isModern())
    {
        g.setGradientFill (juce::ColourGradient (pal::clean.withAlpha (0.06f), r.getCentreX(), r.getY(),
                                                 juce::Colours::transparentBlack, r.getCentreX(), r.getY() + r.getHeight() * 0.6f, false));
        g.fillRoundedRectangle (r, 6.0f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (pal::io.withAlpha (0.05f), r.getCentreX(), r.getBottom(),
                                                 juce::Colours::transparentBlack, r.getCentreX(), r.getY(), false));
        g.fillRoundedRectangle (r, 6.0f);
    }

    // inner shadow along the top edge
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.45f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 10.0f, false));
    g.fillRoundedRectangle (r.withHeight (10.0f), 6.0f);
}

void vox::ui::drawSegmented (juce::Graphics& g, juce::Rectangle<float> r, const juce::StringArray& names,
                             int index, juce::Colour accent)
{
    r = r.reduced (0.5f);

    if (isGoliath() && MISHKAL_V13 && ! names.isEmpty() && ! names[0].startsWith ("    "))   // V13: segmented pill
    {
        v13::pill (g, r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 28.0f)), names, index, false);
        return;
    }
    if (isGoliath())
    {
        // LED selector: one amber LED per position with an engraved label. Padded names (type bars) carry their own LEDs.
        const float w = r.getWidth() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
            const bool sel = i == index;
            const bool ownLed = ! names[i].startsWith ("    ");
            const auto txt = names[i].trim();
            float size = 11.5f;
            const float ledD = juce::jmin (9.0f, r.getHeight() - 6.0f);
            const float avail = seg.getWidth() - (ownLed ? ledD + 8.0f : 22.0f);
            while (size > 8.0f && juce::GlyphArrangement::getStringWidth (font (size, true), txt) > avail) size -= 0.5f;
            const float tw = juce::GlyphArrangement::getStringWidth (font (size, true), txt);
            if (ownLed)
            {
                const float total = ledD + 5.0f + tw;
                const float x0 = seg.getCentreX() - total * 0.5f;
                drawGoliathLed (g, { x0, seg.getCentreY() - ledD * 0.5f, ledD, ledD }, goliathAmber(), sel);
                g.setColour (sel ? goliathLegend() : goliathLabel().withAlpha (0.8f));
                g.setFont (font (size, true));
                g.drawText (txt, juce::Rectangle<float> (x0 + ledD + 5.0f, seg.getY(), tw + 4.0f, seg.getHeight()), juce::Justification::centredLeft);
            }
            else
            {
                if (sel)
                {
                    g.setColour (juce::Colour (0xff0b0b0c));
                    g.fillRoundedRectangle (seg.reduced (1.0f), 3.0f);
                    g.setColour (goliathAmber().withAlpha (0.55f));
                    g.drawRoundedRectangle (seg.reduced (1.5f), 3.0f, 1.0f);
                }
                g.setColour (sel ? goliathLegend() : goliathLabel().withAlpha (0.8f));
                g.setFont (font (11.0f, true));
                g.drawText (names[i], seg, juce::Justification::centred);
            }
        }
        return;
    }

    if (isRack())
    {
        // Slide switch: recessed slot with a brushed-metal thumb on the selected position
        g.setColour (juce::Colour (0xff0b0b0d));
        g.fillRoundedRectangle (r, 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.8f), 0.0f, r.getY(),
                                                 juce::Colours::transparentBlack, 0.0f, r.getY() + 6.0f, false));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.translated (0.0f, 0.6f), 3.0f, 0.8f);

        const float w = r.getWidth() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
            if (i == index)
            {
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.fillRoundedRectangle (seg.translated (0.0f, 1.5f), 2.5f);
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e6e8), 0.0f, seg.getY(),
                                                         juce::Colour (0xff8c8c91), 0.0f, seg.getBottom(), false));
                g.fillRoundedRectangle (seg, 2.5f);
                auto light = juce::Rectangle<float> (seg.getX() + 5.0f, seg.getBottom() - 3.5f, seg.getWidth() - 10.0f, 2.0f);
                g.setColour (accent.withAlpha (0.35f));
                g.fillRoundedRectangle (light.expanded (1.5f, 1.0f), 2.0f);
                g.setColour (accent);
                g.fillRoundedRectangle (light, 1.0f);
                g.setColour (juce::Colour (0xff1a1a1c));
            }
            else
                g.setColour (pal::dim.withAlpha (0.85f));
            g.setFont (font (11.0f, true));
            g.drawText (names[i], seg.withTrimmedBottom (1.0f), juce::Justification::centred);
        }
        return;
    }

    const float rad = isHardware() ? 3.0f : 6.0f;

    // Recessed track
    g.setColour (isHardware() ? (isLightPanel() ? pal::panel.darker (0.3f) : juce::Colour (0xff1c1b1a)) : pal::bg.darker (0.2f));
    g.fillRoundedRectangle (r, rad);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 6.0f, false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (isRoyal() ? pal::io.withAlpha (0.35f) : pal::line);
    g.drawRoundedRectangle (r, rad, 1.0f);

    const float w = r.getWidth() / (float) names.size();
    for (int i = 0; i < names.size(); ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
        const bool sel = i == index;

        if (isHardware())
        {
            // little push buttons; the selected one is lit
            const bool lp = isLightPanel();
            g.setGradientFill (juce::ColourGradient (sel ? accent.brighter (0.25f) : (lp ? pal::panelHi : juce::Colour (0xff4a4744)), 0.0f, seg.getY(),
                                                     sel ? accent.darker (0.35f) : (lp ? pal::panel.darker (0.08f) : juce::Colour (0xff302e2c)),
                                                     0.0f, seg.getBottom(), false));
            g.fillRoundedRectangle (seg, 2.5f);
            g.setColour (juce::Colours::white.withAlpha (sel ? 0.35f : 0.08f));
            g.drawHorizontalLine ((int) seg.getY() + 1, seg.getX() + 2.0f, seg.getRight() - 2.0f);
            if (sel)
            {
                g.setColour (accent.withAlpha (0.25f));
                g.drawRoundedRectangle (seg.expanded (1.5f), 3.5f, 2.0f);
            }
        }
        else if (sel)
        {
            if (isModern())
            {
                g.setColour (accent.withAlpha (0.25f));
                g.fillRoundedRectangle (seg.expanded (2.0f), rad);
            }
            g.setGradientFill (juce::ColourGradient (accent.brighter (0.15f), 0.0f, seg.getY(),
                                                     accent.darker (0.15f), 0.0f, seg.getBottom(), false));
            g.fillRoundedRectangle (seg, rad - 1.5f);
        }

        g.setColour (sel ? (isHardware() || isLightPanel() ? juce::Colour (0xff1d1b19) : pal::bg) : pal::dim);
        if (sel && isLightPanel() && ! isHardware()) g.setColour (juce::Colours::white);
        g.setFont (font (11.0f, true));
        g.drawText (names[i], seg, juce::Justification::centred);
    }
}

void vox::ui::drawRackHole (juce::Graphics& g, juce::Rectangle<float> r)
{
    // Oval rack-ear slot: dark hole with an inner shadow and a lit lower lip
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.0f, r.getY(), juce::Colour (0xff1c1c1f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (1.0f), r.getHeight() * 0.5f - 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r.expanded (0.5f).translated (0.0f, 0.6f), r.getHeight() * 0.5f, 0.8f);
}

void vox::ui::drawGoliathLed (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour col, bool on)
{
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    auto bezel = area.withSizeKeepingCentre (d, d);
    auto lens = bezel.reduced (d * 0.17f);
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (bezel);
    if (on)
    {
        g.setColour (col.withAlpha (0.28f));
        g.fillEllipse (bezel.expanded (d * 0.35f));
        g.setColour (col.withAlpha (0.45f));
        g.fillEllipse (bezel.expanded (d * 0.12f));
        g.setGradientFill (juce::ColourGradient (col.brighter (0.9f), lens.getCentreX(), lens.getCentreY(),
                                                 col.darker (0.15f), lens.getRight(), lens.getBottom(), true));
    }
    else
        g.setGradientFill (juce::ColourGradient (col.withMultipliedBrightness (0.26f).withMultipliedSaturation (0.55f), lens.getCentreX(), lens.getY(),
                                                 juce::Colour (0xff141210), lens.getCentreX(), lens.getBottom(), false));
    g.fillEllipse (lens);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.55f : 0.12f));
    g.fillEllipse (lens.getX() + lens.getWidth() * 0.25f, lens.getY() + lens.getHeight() * 0.14f, lens.getWidth() * 0.32f, lens.getHeight() * 0.24f);
}

void vox::ui::drawGoliathScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
{
    const auto area = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black);
    g.fillEllipse (area.expanded (1.2f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4d), c.x - r * 0.5f, c.y - r * 0.6f,
                                             juce::Colour (0xff101011), c.x + r * 0.6f, c.y + r * 0.8f, true));
    g.fillEllipse (area);
    juce::Path cross;
    cross.addRoundedRectangle (-r * 0.62f, -r * 0.13f, r * 1.24f, r * 0.26f, r * 0.1f);
    cross.addRoundedRectangle (-r * 0.13f, -r * 0.62f, r * 0.26f, r * 1.24f, r * 0.1f);
    g.setColour (juce::Colour (0xff050505));
    g.fillPath (cross, juce::AffineTransform::rotation (angle).translated (c));
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (area.reduced (0.5f), 0.8f);
}

juce::String vox::ui::compactValue (const juce::String& text)
{
    const auto t = text.trim();
    // first number in the text, with sign
    int start = -1, end = -1;
    for (int i = 0; i < t.length(); ++i)
    {
        const auto ch = t[i];
        if (juce::CharacterFunctions::isDigit (ch) || ((ch == '-' || ch == '+' || ch == '.') && i + 1 < t.length()
                                                       && juce::CharacterFunctions::isDigit (t[i + 1])))
        {
            if (start < 0) start = i;
            end = i + 1;
        }
        else if (start >= 0) break;
    }
    if (start < 0) return t;   // e.g. "Off"

    const auto numText = t.substring (start, end);
    const double v = numText.getDoubleValue();
    const bool khz = t.containsIgnoreCase ("khz");
    auto fmt = [] (double x, bool plus)
    {
        juce::String out;
        if (std::abs (x) >= 10.0 || std::abs (x - std::round (x)) < 0.05) out = juce::String ((int) std::round (x));
        else out = juce::String (x, 1);
        return (plus && x > 0.0 ? "+" : "") + out;
    };
    auto out = fmt (v, numText.startsWithChar ('+'));
    if (khz) out << "k";
    else if (t.endsWith ("us")) out << "u";
    return out;
}

//==============================================================================
LookAndFeel::LookAndFeel() { applyColours(); }

void LookAndFeel::applyColours()
{
    setColour (juce::Slider::textBoxTextColourId, pal::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, pal::line);
    setColour (juce::Label::textColourId, pal::text);
    setColour (juce::TextEditor::backgroundColourId, pal::panelHi);
    setColour (juce::TextEditor::textColourId, pal::text);
    setColour (juce::TextEditor::highlightColourId, pal::line);
    setColour (juce::CaretComponent::caretColourId, pal::text);
    setColour (juce::ComboBox::backgroundColourId, isLightPanel() ? pal::panelHi : pal::bg);
    setColour (juce::ComboBox::textColourId, pal::text);
    setColour (juce::ComboBox::outlineColourId, isRoyal() ? pal::io.withAlpha (0.4f) : pal::line);
    setColour (juce::ComboBox::arrowColourId, pal::dim);
    setColour (juce::PopupMenu::backgroundColourId, pal::panel);
    setColour (juce::PopupMenu::textColourId, pal::text);
    setColour (juce::PopupMenu::headerTextColourId, pal::dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::line);
    setColour (juce::PopupMenu::highlightedTextColourId, pal::text);
    setColour (juce::TooltipWindow::backgroundColourId, pal::panelHi);
    setColour (juce::TooltipWindow::textColourId, pal::text);
}

#if MISHKAL_V48
// V48: linked controls - where the output really ends up: a marker on the ring, dots running toward it,
// and (on the flat knobs) the real value in the middle. Coral = turned down by the link, cyan = turned up.
static void drawLinkGhost (juce::Graphics& g, juce::Slider& s, juce::Point<float> c, float ringR, float ringW, float discR,
                           float a0, float a1, float angle, bool showValue)
{
    if (! s.isEnabled() || ! s.getProperties().contains ("autoDb") || std::abs ((float) s.getProperties()["autoDb"]) <= 0.05f) return;
    const float ad = (float) s.getProperties()["autoDb"];
    const float ph = (float) s.getProperties()["linkPhase"];
    const double v2 = juce::jlimit (s.getMinimum(), s.getMaximum(), s.getValue() + (double) ad);
    const float angle2 = a0 + (float) s.valueToProportionOfLength (v2) * (a1 - a0);
    const auto col = ad < 0.0f ? juce::Colour (0xffff5e7a) : juce::Colour (0xff4fe3ff);
    juce::Path span;
    span.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, juce::jmin (angle, angle2), juce::jmax (angle, angle2), true);
    g.setColour (col.withAlpha (0.22f));
    g.strokePath (span, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    for (int i = 0; i < 4; ++i)   // dots travelling from the set value to the real one
    {
        const float t = std::fmod (ph + (float) i / 4.0f, 1.0f);
        const auto pt = c.getPointOnCircumference (ringR, angle + (angle2 - angle) * t);
        const float fade = std::sin (t * juce::MathConstants<float>::pi);
        g.setColour (col.withAlpha (0.3f * fade));
        g.fillEllipse (juce::Rectangle<float> (ringW * 2.4f, ringW * 2.4f).withCentre (pt));
        g.setColour (col.brighter (0.3f).withAlpha (fade));
        g.fillEllipse (juce::Rectangle<float> (ringW * 1.1f, ringW * 1.1f).withCentre (pt));
    }
    const auto mk = c.getPointOnCircumference (ringR, angle2);
    g.setColour (col.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (ringW * 3.6f, ringW * 3.6f).withCentre (mk));
    g.setColour (juce::Colour (0xff111215));
    g.fillEllipse (juce::Rectangle<float> (ringW * 2.0f, ringW * 2.0f).withCentre (mk));
    g.setColour (col);
    g.drawEllipse (juce::Rectangle<float> (ringW * 2.0f, ringW * 2.0f).withCentre (mk), 1.6f);
    if (! showValue) return;
    const float fs = juce::jlimit (9.0f, 13.0f, discR * 0.42f);
    g.setColour (col);
    g.setFont (font (fs, true));
    g.drawText ((v2 > 0.0 ? "+" : "") + juce::String (v2, 1), juce::Rectangle<float> (discR * 2.0f, fs + 2.0f).withCentre ({ c.x, c.y - 2.0f }),
                juce::Justification::centred);
    g.setColour (col.withAlpha (0.6f));
    g.setFont (font (juce::jmax (7.5f, fs * 0.7f)));
    g.drawText ("auto", juce::Rectangle<float> (discR * 2.0f, fs).withCentre ({ c.x, c.y + fs * 0.75f }), juce::Justification::centred);
}
#endif

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float a0, float a1, juce::Slider& s)
{
    // Classic-hardware knob for the selected mode
    {
        const int style = s.getProperties()["knobStyle"];
        if (style != 0)
        {
            const auto cap  = juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["knobCap"]);
            const auto text = juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["knobText"]);
            if (drawHardwareKnob (g, s, juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f), pos, a0, a1, (KnobStyle) style, cap, text))
            {
               #if MISHKAL_V48
                const auto hb = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
                const float hr = juce::jmin (hb.getWidth(), hb.getHeight()) * 0.5f;
                drawLinkGhost (g, s, hb.getCentre(), hr * 0.97f, juce::jmax (2.5f, hr * 0.07f), 0.0f, a0, a1, a0 + pos * (a1 - a0), false);
               #endif
                return;
            }
        }
    }

    if (isGoliath() && (bool) s.getProperties()["minimal"])
    {
        // V3 minimal knob: a flat dark disc inside a thin ring; the ring fills with the value, a small dot marks it.
        // The value sits in a small box underneath.
        auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
        if (! (bool) s.getProperties()["minimalNoBox"])   // V58: the big knob shows no value box (like the Figma design)
        {
            auto boxArea = area.removeFromBottom (22.0f);
            area.removeFromBottom (4.0f);
            const auto txt = s.getTextFromValue (s.getValue());
            const float tw = juce::jmax (48.0f, juce::GlyphArrangement::getStringWidth (font (12.0f), txt) + 14.0f);
            auto box = boxArea.withSizeKeepingCentre (juce::jmin (tw, boxArea.getWidth()), 20.0f);
            g.setColour (juce::Colour (0xff111215));
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (juce::Colour (0xff34363c));
            g.drawRoundedRectangle (box.reduced (0.5f), 4.0f, 1.0f);
            g.setColour (juce::Colour (0xffe9e7e2));
            g.setFont (font (12.0f));
            g.drawText (txt, box, juce::Justification::centred);
        }
        const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const float ringW = juce::jmax (3.0f, R * 0.09f);
        const float ringR = R - ringW * 0.5f;
        const float angle = a0 + pos * (a1 - a0);
        const auto accent = ! s.isEnabled() ? juce::Colour (0xff4a4a50)
                          : s.getProperties().contains ("minimalColour") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["minimalColour"])
                          : juce::Colour (MISHKAL_ACCENT);
        juce::Path track, value;
        track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a1, true);
        value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, angle, true);
        g.setColour (juce::Colour (0xff2c2e33));
        g.strokePath (track, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // V11: "glowLevel" (0..1, set from the signal level) scales the glow; without it the glow is always full
        const float glow = s.getProperties().contains ("glowLevel") ? (float) s.getProperties()["glowLevel"] : 1.0f;
        const bool neon = s.isEnabled() && (bool) s.getProperties()["minimalGlow"] && glow > 0.01f;
        if (neon)   // V10: neon tube - wide soft halo layers under the ring, then a bright core on top
        {
            for (auto [extra, alpha] : { std::pair { 14.0f, 0.06f }, std::pair { 9.0f, 0.10f }, std::pair { 5.0f, 0.18f }, std::pair { 2.5f, 0.32f } })
            {
                g.setColour (accent.withAlpha (alpha * glow));
                g.strokePath (value, juce::PathStrokeType (ringW + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
        if (s.isEnabled() && (bool) s.getProperties()["velvet"])
            for (auto [extra, alpha] : { std::pair { 8.0f, 0.05f }, std::pair { 4.0f, 0.09f } })
            {
                g.setColour (accent.withAlpha (alpha));
                g.strokePath (value, juce::PathStrokeType (ringW + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (neon)
        {
            g.setColour (accent.interpolatedWith (juce::Colours::white, 0.55f).withAlpha (glow));
            g.strokePath (value, juce::PathStrokeType (juce::jmax (1.0f, ringW * 0.35f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const float discR = ringR - ringW * 1.6f;
        if ((bool) s.getProperties()["velvet"])
        {
            // V16 velvet: a plush matte disc - soft light from above fading to deep shadow, a faint sheen
            // along the top edge and a fine pile texture; the ring gets a soft, diffused bloom (no hard neon)
            const auto disc = juce::Rectangle<float> (discR * 2.0f, discR * 2.0f).withCentre (c);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillEllipse (disc.expanded (2.0f).translated (0.0f, 1.5f));
            const float tint = MISHKAL_V17 ? 0.16f : 0.0f;   // V17: the velvet takes a little of the ring's colour
            juce::ColourGradient plush (juce::Colour (0xff2b2a2e).interpolatedWith (accent, tint), c.x, c.y - discR * 0.75f,
                                        juce::Colour (0xff0c0c0e).interpolatedWith (accent, tint * 0.25f), c.x, c.y + discR, true);
            plush.addColour (0.55, juce::Colour (0xff18171a).interpolatedWith (accent, tint * 0.5f));
            g.setGradientFill (plush);
            g.fillEllipse (disc);
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path clip; clip.addEllipse (disc);
                g.reduceClipRegion (clip);
                juce::Random rng (1234);   // fixed seed: the pile never shimmers between repaints
                const int dots = (int) (discR * discR * 0.9f);
                for (int i = 0; i < dots; ++i)
                {
                    const float px = disc.getX() + rng.nextFloat() * disc.getWidth();
                    const float py = disc.getY() + rng.nextFloat() * disc.getHeight();
                    g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.035f + rng.nextFloat() * 0.035f));
                    g.fillRect (px, py, 1.0f, 1.0f);
                }
            }
            juce::Path sheen;
            sheen.addCentredArc (c.x, c.y, discR - 1.0f, discR - 1.0f, 0.0f, -1.1f, 1.1f, true);
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.strokePath (sheen, juce::PathStrokeType (1.5f));
        }
        else
        {
            g.setColour (juce::Colour (0xff1a1b1f));
            g.fillEllipse (juce::Rectangle<float> (discR * 2.0f, discR * 2.0f).withCentre (c));
        }
       #if MISHKAL_V48
        drawLinkGhost (g, s, c, ringR, ringW, discR, a0, a1, angle, true);
       #endif
       #if MISHKAL_V84
        if (s.isEnabled() && s.getProperties().contains ("inDb"))
        {
            // V84: the signal on the threshold's own scale - the outer arc is what goes in, the inner one what comes out
            // after the compressor; the part of the input past the threshold glows hot (that is what's being squeezed)
            auto angleFor = [&] (float db)
            {
                const double lo = s.getMinimum(), hi = s.getMaximum();
                if (db <= (float) lo) return a0;
                return a0 + (float) s.valueToProportionOfLength (juce::jlimit (lo, hi, (double) db)) * (a1 - a0);
            };
            const float inDb = (float) s.getProperties()["inDb"], outDb = (float) s.getProperties()["outDb"];
            const float aIn = angleFor (inDb), aOut = angleFor (outDb);
            const float rIn = discR * 0.86f, rOut = discR * 0.74f, w = juce::jmax (2.0f, ringW * 0.42f);
            auto arc = [&] (float r, float from, float to, juce::Colour col, float width)
            {
                if (to <= from + 0.001f) return;
                juce::Path p;
                p.addCentredArc (c.x, c.y, r, r, 0.0f, from, to, true);
                g.setColour (col.withAlpha (col.getFloatAlpha() * 0.25f));
                g.strokePath (p, juce::PathStrokeType (width + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (col);
                g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };
            // faint tracks so the scale reads even in silence
            arc (rIn,  a0, a1, juce::Colour (0xff26272c), w);
            arc (rOut, a0, a1, juce::Colour (0xff26272c), w);
            const auto inCol = juce::Colour (0xffd9d4c7), hot = juce::Colour (0xffff5a3c);
            arc (rIn, a0, juce::jmin (aIn, angle), inCol.withAlpha (0.75f), w);
            if (aIn > angle) arc (rIn, angle, aIn, hot, w);   // over the threshold: being compressed
            arc (rOut, a0, aOut, accent, w);
            // tiny labels at the start of each arc
            g.setFont (font (9.0f, true));
            const auto tagAt = [&] (float r, const char* t, juce::Colour col)
            {
                const auto p = c.getPointOnCircumference (r, a0 - 0.22f);
                g.setColour (col);
                g.drawText (t, juce::Rectangle<float> (22.0f, 10.0f).withCentre (p), juce::Justification::centred);
            };
            tagAt (rIn, "in", inCol.withAlpha (0.8f));
            tagAt (rOut, "out", accent.withAlpha (0.9f));
        }
       #endif
        const auto dot = c.getPointOnCircumference (discR * 0.72f, angle);
        if (neon)
        {
            g.setColour (accent.withAlpha (0.25f * glow));
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (dot));
            g.setColour (accent.withAlpha (0.5f * glow));
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (dot));
        }
        g.setColour (juce::Colour (0xffe9e7e2));
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (dot));
        return;
    }

    if (isGoliath())
    {
        // 500-series knob: numbered dot scale around a black cap with a white index line
        const bool enabled = s.isEnabled();
        auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (1.0f);
        const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const int labels = s.getProperties().contains ("scaleLabels") ? (int) s.getProperties()["scaleLabels"]
                         : R >= 48.0f ? 5 : R >= 30.0f ? 3 : 0;
        const bool big = R >= 64.0f;
        const float numR = R - (big ? 10.0f : 8.0f);
        const float knobR = labels > 0 ? numR - (big ? 16.0f : 11.0f) : R - 6.0f;
        const auto legend = goliathLegend().withAlpha (enabled ? 0.92f : 0.35f);

        // numbers
        g.setFont (font (big ? 14.0f : R >= 44.0f ? 11.0f : 9.5f));
        for (int i = 0; i < labels; ++i)
        {
            const float prop = (float) i / (float) (labels - 1);
            const auto txt = compactValue (s.getTextFromValue (s.proportionOfLengthToValue (prop)));
            const auto pt = c.getPointOnCircumference (numR, a0 + (a1 - a0) * prop);
            g.setColour (legend);
            g.drawText (txt, juce::Rectangle<float> (40.0f, 13.0f).withCentre (pt), juce::Justification::centred);
        }
        // dots between the numbers (or all round on small knobs)
        const int dots = labels > 0 ? (labels - 1) * 2 : 11;
        const float dotR = labels > 0 ? numR : knobR + 3.5f;
        for (int i = 0; i <= dots; ++i)
        {
            if (labels > 0 && i % 2 == 0) continue;   // a number sits there
            const auto pt = c.getPointOnCircumference (dotR, a0 + (a1 - a0) * (float) i / (float) dots);
            const float dd = big ? 3.4f : 2.6f;
            g.setColour (legend);
            g.fillEllipse (juce::Rectangle<float> (dd, dd).withCentre (pt));
        }

        auto circle = [&c] (float r) { return juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };
        // shadow + skirt
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (circle (knobR + 1.5f).translated (0.0f, knobR * 0.12f + 1.5f));
        g.setColour (juce::Colour (0xff060607));
        g.fillEllipse (circle (knobR));
        // cap: soft top light falling to black
        const float capR = knobR * 0.9f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3e), c.x, c.y - capR, juce::Colour (0xff0c0c0d), c.x, c.y + capR * 0.7f, false));
        g.fillEllipse (circle (capR));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), c.x, c.y - capR,
                                                 juce::Colours::transparentWhite, c.x, c.y, false));
        g.fillEllipse (circle (capR * 0.92f).translated (0.0f, -capR * 0.05f));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), c.x, c.y - capR,
                                                 juce::Colours::transparentWhite, c.x, c.y - capR * 0.2f, false));
        g.drawEllipse (circle (capR - 0.5f), 1.0f);

        // index line
        const float angle = a0 + pos * (a1 - a0);
        const float lw = juce::jmax (2.0f, knobR * 0.075f);
        juce::Path line;
        line.addRoundedRectangle (-lw * 0.5f, -capR * 0.96f, lw, capR * 0.58f, lw * 0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (line, juce::AffineTransform::rotation (angle).translated (c.x, c.y + 1.0f));
        g.setColour (enabled ? juce::Colour (0xfff2f2ee) : juce::Colour (0xff6a6a6a));
        g.fillPath (line, juce::AffineTransform::rotation (angle).translated (c));
        return;
    }

    const auto accentRaw = s.findColour (juce::Slider::rotarySliderFillColourId);
    const bool bipolar = s.getProperties()["bipolar"];
    const bool enabled = s.isEnabled();
    const auto accent = enabled ? accentRaw : pal::dim.withAlpha (0.45f);

    auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float radius = d * 0.5f;
    const float angle = a0 + pos * (a1 - a0);
    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    using PST = juce::PathStrokeType;

    auto circle = [&c] (float r) { return juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };

    if (isRack())
    {
        // Numbered scale around a colour-coded grip knob
        // Scale numbers only where there is room: 5 on big knobs, 3 on medium, none on small
        const int labels = radius >= 48.0f ? 5 : radius >= 36.0f ? 3 : 0;
        const bool big = labels == 5;
        const float scaleR = radius - (labels > 0 ? 4.0f : 0.0f);
        g.setFont (font (big ? 9.5f : 9.0f, true));
        for (int i = 0; i < labels; ++i)
        {
            const float prop = (float) i / (float) (labels - 1);
            const double v = s.proportionOfLengthToValue (prop);
            const auto txt = compactValue (s.getTextFromValue (v));
            const auto pt = c.getPointOnCircumference (scaleR, a0 + (a1 - a0) * prop);
            g.setColour (pal::text.withAlpha (enabled ? 0.85f : 0.35f));
            g.drawText (txt, juce::Rectangle<float> (34.0f, 11.0f).withCentre (pt), juce::Justification::centred);
        }

        const float tickR = labels > 0 ? scaleR - (big ? 8.0f : 6.5f) : radius - 3.0f;
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.0f;
            const auto p1 = c.getPointOnCircumference (tickR - 2.5f, a);
            const auto p2 = c.getPointOnCircumference (tickR + 1.5f, a);
            g.setColour ((isOnyx() ? pal::text : accent).withAlpha (enabled ? 0.9f : 0.4f));
            g.drawLine ({ p1, p2 }, 1.4f);
        }

        const float knobR = tickR - 3.0f;

        if (isOnyx())
        {
            // Vintage black skirted knob: wide skirt, knurled cap, white index line
            const auto line = enabled ? juce::Colours::white : juce::Colours::white.withAlpha (0.35f);

            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.fillEllipse (circle (knobR).translated (0.0f, 3.5f).expanded (2.0f));

            // skirt
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e31), c.x, c.y - knobR,
                                                     juce::Colour (0xff060606), c.x, c.y + knobR, false));
            g.fillEllipse (circle (knobR));
            g.setColour (juce::Colour (0xff4a4a4f));   // gunmetal rim separates the skirt from the panel
            g.drawEllipse (circle (knobR + 0.5f), 1.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), c.x, c.y - knobR,
                                                     juce::Colours::transparentWhite, c.x, c.y, false));
            g.drawEllipse (circle (knobR - 0.6f), 1.2f);

            // index line across the skirt
            juce::Path skirtLine;
            skirtLine.addRoundedRectangle (-1.5f, -knobR + 1.5f, 3.0f, knobR * 0.36f, 1.2f);
            g.setColour (line);
            g.fillPath (skirtLine, juce::AffineTransform::rotation (angle).translated (c));

            // cap shadow + knurled edge
            const float capR = knobR * 0.66f;
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillEllipse (circle (capR).translated (0.0f, 1.5f).expanded (1.0f));
            g.setColour (juce::Colour (0xff121213));
            g.fillEllipse (circle (capR));
            constexpr int knurls = 44;
            for (int i = 0; i < knurls; ++i)
            {
                const float th = juce::MathConstants<float>::twoPi * (float) i / (float) knurls;
                const float light = 0.5f + 0.5f * std::cos (th + 2.2f);   // lit from the upper left
                g.setColour (juce::Colours::white.withAlpha (0.05f + 0.20f * light));
                g.drawLine ({ c.getPointOnCircumference (capR * 0.86f, th), c.getPointOnCircumference (capR - 0.4f, th) }, 1.0f);
            }

            // cap top: satin black with a soft highlight
            const float topR = capR * 0.84f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2f), c.x, c.y - topR,
                                                     juce::Colour (0xff0b0b0c), c.x, c.y + topR, false));
            g.fillEllipse (circle (topR));
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), c.x - topR * 0.35f, c.y - topR * 0.65f,
                                                     juce::Colours::transparentWhite, c.x, c.y + topR * 0.1f, true));
            g.fillEllipse (circle (topR).reduced (topR * 0.06f));

            // index line on the cap
            juce::Path capLine;
            capLine.addRoundedRectangle (-1.3f, -topR + 1.0f, 2.6f, topR * 0.62f, 1.2f);
            g.setColour (line);
            g.fillPath (capLine, juce::AffineTransform::rotation (angle).translated (c));
            return;
        }

        const auto body = enabled ? accentRaw : accentRaw.withSaturation (0.0f).darker (0.4f);

        // drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (circle (knobR).translated (0.0f, 3.0f).expanded (1.5f));

        // grip ring with scalloped edge
        juce::Path grip;
        constexpr int ridges = 26;
        for (int i = 0; i <= ridges * 8; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) (ridges * 8);
            const float rr = knobR * (1.0f - 0.05f * (0.5f + 0.5f * std::cos ((float) ridges * th)));
            const auto pt = c.getPointOnCircumference (rr, th);
            if (i == 0) grip.startNewSubPath (pt); else grip.lineTo (pt);
        }
        grip.closeSubPath();
        g.setGradientFill (juce::ColourGradient (body.brighter (0.35f), c.x - knobR * 0.6f, c.y - knobR * 0.8f,
                                                 body.darker (0.65f), c.x + knobR * 0.6f, c.y + knobR, false));
        g.fillPath (grip);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (grip, PST (1.0f));

        // domed cap
        const float capR = knobR * 0.74f;
        g.setGradientFill (juce::ColourGradient (body.brighter (0.25f), c.x, c.y - capR,
                                                 body.darker (0.35f), c.x, c.y + capR, false));
        g.fillEllipse (circle (capR));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), c.x - capR * 0.35f, c.y - capR * 0.7f,
                                                 juce::Colours::transparentWhite, c.x, c.y + capR * 0.2f, true));
        g.fillEllipse (circle (capR).reduced (capR * 0.08f));
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (circle (capR), 0.8f);

        // white pointer
        juce::Path pointer;
        pointer.addRoundedRectangle (-1.6f, -knobR + 2.0f, 3.2f, knobR * 0.62f, 1.6f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.translated (0.0f, 1.0f)));
        g.setColour (enabled ? juce::Colours::white : juce::Colours::white.withAlpha (0.4f));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c));
        return;
    }

    if (isHardware())
    {
        // Tick scale
        const float tickIn = radius * 0.86f, tickOut = radius * 0.99f;
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.0f;
            const float len = (i % 5 == 0) ? 1.0f : 0.6f;
            juce::Point<float> p1 = c.getPointOnCircumference (tickIn + (tickOut - tickIn) * (1.0f - len), a);
            juce::Point<float> p2 = c.getPointOnCircumference (tickOut, a);
            g.setColour (pal::text.withAlpha (enabled ? 0.55f : 0.25f));
            g.drawLine ({ p1, p2 }, 1.1f);
        }

        // Thin lamp-coloured value arc inside the ticks
        juce::Path val;
        val.addCentredArc (c.x, c.y, radius * 0.80f, radius * 0.80f, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (accent.withAlpha (0.9f));
        g.strokePath (val, PST (2.0f, PST::curved, PST::rounded));

        // Skirt
        const float skirt = radius * 0.72f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (circle (skirt).translated (0.0f, 2.0f).expanded (1.0f));
        g.setGradientFill (juce::ColourGradient (pal::skirtTop, c.x, c.y - skirt, pal::skirtBot, c.x, c.y + skirt, false));
        g.fillEllipse (circle (skirt));
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (circle (skirt - 0.5f), 1.0f);

        // Bakelite cap with a soft specular highlight
        const float cap = skirt * 0.70f;
        g.setGradientFill (juce::ColourGradient (pal::capTop, c.x, c.y - cap, pal::capBot, c.x, c.y + cap, false));
        g.fillEllipse (circle (cap));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), c.x - cap * 0.3f, c.y - cap * 0.6f,
                                                 juce::Colours::transparentWhite, c.x, c.y, true));
        g.fillEllipse (circle (cap).reduced (cap * 0.12f));

        // Cream pointer line across skirt and cap
        juce::Path pointer;
        pointer.addRoundedRectangle (-1.3f, -skirt + 1.5f, 2.6f, skirt * 0.62f, 1.3f);
        g.setColour (enabled ? pal::pointer : pal::pointer.withAlpha (0.4f));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c));
        return;
    }

    const float track = isModern() ? 2.0f : 3.0f;
    const float arcR = radius - track * 2.0f;

    juce::Path bgArc;
    bgArc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
    g.setColour (pal::line.withAlpha (isModern() ? 0.9f : 1.0f));
    g.strokePath (bgArc, PST (track, PST::curved, PST::rounded));

    juce::Path valArc;
    valArc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    if (enabled) drawGlowStroke (g, valArc, accent, track + 0.8f);
    else { g.setColour (accent); g.strokePath (valArc, PST (track, PST::curved, PST::rounded)); }

    // Knob body
    const float bodyR = arcR - track * 2.6f - 2.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (circle (bodyR).translated (0.0f, 2.0f).expanded (1.5f));

    if (isModern())
    {
        g.setGradientFill (juce::ColourGradient (pal::panelHi.brighter (0.25f), c.x, c.y - bodyR,
                                                 pal::bg, c.x, c.y + bodyR, false));
        g.fillEllipse (circle (bodyR));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), c.x, c.y - bodyR,
                                                 juce::Colours::transparentWhite, c.x, c.y + bodyR * 0.3f, false));
        g.drawEllipse (circle (bodyR - 0.5f), 1.0f);

        // Glowing dot pointer
        const auto dot = c.getPointOnCircumference (bodyR * 0.62f, angle);
        g.setColour (accent.withAlpha (enabled ? 0.3f : 0.0f));
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (dot));
        g.setColour (enabled ? accent.brighter (0.4f) : pal::dim);
        g.fillEllipse (juce::Rectangle<float> (4.5f, 4.5f).withCentre (dot));
    }
    else   // Royal
    {
        g.setGradientFill (juce::ColourGradient (pal::panelHi.brighter (0.3f), c.x - bodyR * 0.4f, c.y - bodyR,
                                                 pal::bg, c.x + bodyR * 0.4f, c.y + bodyR, false));
        g.fillEllipse (circle (bodyR));
        g.setGradientFill (juce::ColourGradient (pal::io.withAlpha (0.85f), c.x, c.y - bodyR,
                                                 pal::io.withAlpha (0.25f), c.x, c.y + bodyR, false));
        g.drawEllipse (circle (bodyR - 0.5f), 1.2f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.4f, -bodyR + 3.0f, 2.8f, bodyR * 0.5f, 1.4f);
        g.setColour (enabled ? pal::io : pal::dim);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c));
    }
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);

    if (isGoliath())   // LED + engraved label; the power switch is a bare LED
    {
        const auto lampCol = b.getProperties().contains ("ledColour") ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["ledColour"])
                                                                      : goliathAmber();
        if (MISHKAL_V13 && (bool) b.getProperties()["slideOnly"])   // a bare slide switch (its name is a separate label)
        {
            v13::slide (g, r, on, highlighted, {});
            return;
        }
        if (b.getButtonText().isEmpty())   // module power: a lit push button, ON (amber lamp) or BYPASS (dark)
        {
            if (MISHKAL_V13)   // V13: neon power button
            {
                if (b.getProperties().contains ("caption"))
                {
                    g.setColour (on ? goliathLegend() : goliathLabel());
                    g.setFont (font (11.0f, true).withExtraKerningFactor (0.08f));
                    auto cb = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 30.0f));
                    g.drawText (b.getProperties()["caption"].toString(), cb.withY (cb.getY() - 18.0f).withHeight (14.0f).expanded (10.0f, 0.0f),
                                juce::Justification::centred, false);
                }
                v13::power (g, r, on, highlighted, b.getProperties().getWithDefault ("offText", "BYPASS").toString());
                return;
            }
            auto btn = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 30.0f));
            if (b.getProperties().contains ("caption"))   // which stage this button switches
            {
                g.setColour (on ? goliathLegend() : goliathLabel());
                g.setFont (font (11.0f, true).withExtraKerningFactor (0.08f));
                g.drawText (b.getProperties()["caption"].toString(), btn.withY (btn.getY() - 18.0f).withHeight (14.0f).expanded (10.0f, 0.0f),
                            juce::Justification::centred, false);
            }
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (btn.translated (0.0f, 1.5f), 4.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (highlighted ? 0xff3a3a3e : 0xff313134), 0.0f, btn.getY(),
                                                     juce::Colour (0xff18181a), 0.0f, btn.getBottom(), false));
            g.fillRoundedRectangle (btn, 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawHorizontalLine ((int) btn.getY() + 1, btn.getX() + 3.0f, btn.getRight() - 3.0f);
            g.setColour (juce::Colours::black);
            g.drawRoundedRectangle (btn, 4.0f, 1.0f);
            if (on)
            {
                g.setColour (lampCol.withAlpha (0.18f));
                g.fillRoundedRectangle (btn.reduced (2.0f), 3.0f);
            }
            auto lamp = juce::Rectangle<float> (btn.getWidth() - 16.0f, 3.0f).withCentre ({ btn.getCentreX(), btn.getY() + 6.0f });
            g.setColour (on ? lampCol : juce::Colour (0xff2a2620));
            g.fillRoundedRectangle (lamp, 1.5f);
            if (on)
            {
                g.setColour (lampCol.withAlpha (0.35f));
                g.fillRoundedRectangle (lamp.expanded (2.0f, 2.0f), 2.5f);
            }
            g.setColour (on ? goliathLegend() : goliathLabel());
            g.setFont (font (on ? 12.0f : 10.5f, true).withExtraKerningFactor (0.06f));
            g.drawText (on ? juce::String ("ON") : b.getProperties().getWithDefault ("offText", "BYPASS").toString(), btn.withTrimmedTop (8.0f), juce::Justification::centred);
            return;
        }
        if (b.getProperties().contains ("batTop"))   // two-way switches: bat toggle
        {
            const juce::String top = b.getButtonText().isEmpty() ? "On" : b.getProperties()["batTop"].toString();
            const juce::String bottom = b.getButtonText().isEmpty() ? "Bypass" : b.getProperties()["batBottom"].toString();
            drawBatToggle (g, b.getLocalBounds().toFloat(), on, top, bottom);
            return;
        }
        if (MISHKAL_V13)   // V13: slide toggle
        {
            v13::slide (g, r, on, highlighted, b.getButtonText());
            return;
        }
        const float d = juce::jmin (11.0f, r.getHeight() - 4.0f);
        float size = 11.5f;
        while (size > 8.5f && juce::GlyphArrangement::getStringWidth (font (size, true), b.getButtonText()) > r.getWidth() - d - 8.0f) size -= 0.5f;
        const float tw = juce::GlyphArrangement::getStringWidth (font (size, true), b.getButtonText());
        const float x0 = r.getCentreX() - (d + 6.0f + tw) * 0.5f;
        drawGoliathLed (g, { x0, r.getCentreY() - d * 0.5f, d, d }, lampCol, on);
        g.setColour (on || highlighted ? goliathLegend() : goliathLabel());
        g.setFont (font (size, true));
        g.drawText (b.getButtonText(), juce::Rectangle<float> (x0 + d + 6.0f, r.getY(), tw + 4.0f, r.getHeight()), juce::Justification::centredLeft);
        return;
    }

    if (b.getButtonText().isEmpty())   // power switch
    {
        auto circle = r.withSizeKeepingCentre (r.getHeight(), r.getHeight());

        if (isMetal())   // jewel lamp in a metal bezel
        {
            const auto lampCol = isRack() ? pal::alert : accent;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffbdb7ac), circle.getX(), circle.getY(),
                                                     juce::Colour (0xff4a4743), circle.getRight(), circle.getBottom(), false));
            g.fillEllipse (circle);
            auto lens = circle.reduced (2.5f);
            if (on)
            {
                g.setColour (lampCol.withAlpha (0.35f));
                g.fillEllipse (circle.expanded (3.0f));
                g.setGradientFill (juce::ColourGradient (lampCol.brighter (0.8f), lens.getCentreX() - 1.5f, lens.getCentreY() - 1.5f,
                                                         lampCol.darker (0.4f), lens.getRight(), lens.getBottom(), true));
            }
            else
                g.setGradientFill (juce::ColourGradient (lampCol.darker (1.6f).withSaturation (0.4f), lens.getCentreX(), lens.getCentreY(),
                                                         juce::Colour (0xff141312), lens.getRight(), lens.getBottom(), true));
            g.fillEllipse (lens);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.6f : 0.2f));
            g.fillEllipse (lens.getX() + lens.getWidth() * 0.22f, lens.getY() + lens.getHeight() * 0.15f,
                           lens.getWidth() * 0.3f, lens.getHeight() * 0.22f);
            return;
        }

        if (on)
        {
            g.setColour (accent.withAlpha (isModern() ? 0.28f : 0.2f));
            g.fillEllipse (circle.expanded (2.0f));
        }
        g.setColour (on ? accent.withAlpha (0.2f) : pal::panelHi);
        g.fillEllipse (circle);
        g.setColour (on ? accent : (highlighted ? pal::text : pal::dim));
        g.drawEllipse (circle.reduced (0.5f), 1.0f);

        auto icon = circle.reduced (circle.getWidth() * 0.28f);
        juce::Path p;
        p.addCentredArc (icon.getCentreX(), icon.getCentreY(), icon.getWidth() * 0.5f, icon.getHeight() * 0.5f,
                         0.0f, 0.75f, juce::MathConstants<float>::twoPi - 0.75f, true);
        p.startNewSubPath (icon.getCentreX(), icon.getY() - 1.0f);
        p.lineTo (icon.getCentreX(), icon.getCentreY());
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    const float rad = isMetal() ? 3.0f : r.getHeight() * 0.5f;

    if (isRack())   // dark hardware button; lit legend and LED when on
    {
        const auto litCol = isOnyx() ? pal::meter : accent;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), rad);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff424246), 0.0f, r.getY(), juce::Colour (0xff27272b), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.18f : 0.1f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r, rad, 1.0f);

        auto led = juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ r.getX() + 10.0f, r.getCentreY() });
        if (on)
        {
            g.setColour (litCol.withAlpha (0.35f));
            g.fillEllipse (led.expanded (2.5f));
        }
        g.setColour (on ? litCol.brighter (0.3f) : juce::Colour (0xff141416));
        g.fillEllipse (led);
        g.setColour (on ? litCol.brighter (0.2f) : pal::text.withAlpha (0.75f));
        g.setFont (font (11.0f, true));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (12.0f), juce::Justification::centred);
        return;
    }

    if (isHardware())   // square push button, lit when on
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), rad);
        const bool lp = isLightPanel();
        g.setGradientFill (juce::ColourGradient (on ? accent.brighter (0.3f) : (lp ? pal::panelHi : juce::Colour (0xff57544f)), 0.0f, r.getY(),
                                                 on ? accent.darker (0.3f) : (lp ? pal::panel.darker (0.12f) : juce::Colour (0xff35332f)),
                                                 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        if (on)
        {
            g.setColour (accent.withAlpha (0.3f));
            g.drawRoundedRectangle (r.expanded (1.5f), rad + 1.5f, 2.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (on ? 0.4f : 0.12f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
        g.setColour (on ? juce::Colour (0xff1d1b19) : pal::text);
    }
    else
    {
        if (on && isModern())
        {
            g.setColour (accent.withAlpha (0.25f));
            g.fillRoundedRectangle (r.expanded (2.0f), rad + 2.0f);
        }
        if (on)
            g.setGradientFill (juce::ColourGradient (accent.brighter (0.15f), 0.0f, r.getY(), accent.darker (0.15f), 0.0f, r.getBottom(), false));
        else
            g.setColour (pal::panelHi);
        g.fillRoundedRectangle (r, rad);
        g.setColour (on ? accent : (highlighted ? pal::text : (isRoyal() ? pal::io.withAlpha (0.35f) : pal::line)));
        g.drawRoundedRectangle (r, rad, 1.0f);
        g.setColour (on ? pal::bg : pal::text);
    }

    g.setFont (font (11.0f, true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        // Long black slot with a brushed-silver ribbed cap
        const auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
        const bool enabled = s.isEnabled();
        const bool bipolar = s.getProperties()["bipolar"];
        const auto col = enabled ? s.findColour (juce::Slider::trackColourId) : pal::dim.withAlpha (0.4f);
        const float cy = area.getCentreY();
        const float left = area.getX(), right = area.getRight();

        // recessed slot
        auto slot = juce::Rectangle<float> (left - 8.0f, cy - 6.0f, right - left + 16.0f, 12.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));                       // lit lower lip
        g.fillRoundedRectangle (slot.translated (0.0f, 1.0f), 6.0f);
        g.setColour (juce::Colour (0xff040405));
        g.fillRoundedRectangle (slot, 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.0f, slot.getY(),
                                                 juce::Colour (0xff1b1c1f), 0.0f, slot.getBottom(), false));
        g.fillRoundedRectangle (slot.reduced (1.0f), 5.0f);

        // Optional live level meter in the slot (gate): red below the threshold (gated), green above (open)
        if (s.getProperties().contains ("meterDb"))
        {
            const double lvl  = (double) s.getProperties()["meterDb"];
            const double peak = (double) s.getProperties()["meterPeakDb"];
            auto xForDb = [&] (double db)
            {
                const double v = juce::jlimit (s.getMinimum(), s.getMaximum(), db);
                return left + (right - left) * (float) s.valueToProportionOfLength (v);
            };
            const float lx = xForDb (lvl);
            auto inner = slot.reduced (2.0f, 3.0f);
            if (lx > inner.getX())
            {
                const bool gateOff = s.getValue() <= s.getMinimum() + 0.2;
                const auto open = juce::Colour (0xff3fcf6e), shut = juce::Colour (0xffd9342b);
                auto bar = inner.withRight (juce::jmin (lx, inner.getRight()));
                if (gateOff)
                {
                    g.setColour (open.withAlpha (enabled ? 0.55f : 0.25f));
                    g.fillRoundedRectangle (bar, 3.0f);
                }
                else
                {
                    auto below = bar.withRight (juce::jmin (bar.getRight(), pos));
                    g.setColour (shut.withAlpha (enabled ? 0.75f : 0.3f));
                    g.fillRoundedRectangle (below, 3.0f);
                    if (bar.getRight() > pos)
                    {
                        g.setColour (open.withAlpha (enabled ? 0.85f : 0.3f));
                        g.fillRoundedRectangle (bar.withLeft (pos), 3.0f);
                    }
                }
            }
            const float px = xForDb (peak);   // peak-hold tick
            if (px > inner.getX() + 1.0f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.fillRect (px - 1.0f, inner.getY(), 2.0f, inner.getHeight());
            }
        }

        // Optional spectrum meter in the slot (de-esser Freq): one LED cell per slice of the frequency scale.
        // Left of the cap is dim (not listened to); right of the cap is what the de-esser hears,
        // green while it passes and red while it is turning the "s" down.
        if (auto* cells = s.getProperties()["spectrum"].getArray())
        {
            const int n = cells->size();
            const float reducing = std::abs ((float) s.getProperties()["reducingDb"]);
            const float redAmt = juce::jlimit (0.0f, 1.0f, (reducing - 0.3f) / 3.0f);
            auto inner = slot.reduced (3.0f, 3.5f);
            const float cw = inner.getWidth() / (float) juce::jmax (1, n);
            const auto pass = juce::Colour (0xff3fcf6e), cut = juce::Colour (0xffd9342b), idle = juce::Colour (0xff8a9099);
            for (int c = 0; c < n; ++c)
            {
                const float db = (float) (*cells)[c];
                const float lvl = juce::jlimit (0.0f, 1.0f, (db + 72.0f) / 54.0f);   // -72 dB dark ... -18 dB full
                auto cell = juce::Rectangle<float> (inner.getX() + cw * (float) c + 0.6f, inner.getY(), cw - 1.2f, inner.getHeight());
                const bool heard = cell.getCentreX() >= pos;
                g.setColour (juce::Colours::white.withAlpha (0.035f));   // unlit LED
                g.fillRoundedRectangle (cell, 1.0f);
                if (lvl <= 0.02f) continue;
                const auto lit = heard ? pass.interpolatedWith (cut, redAmt) : idle;
                g.setColour (lit.withAlpha ((heard ? 0.25f + 0.7f * lvl : 0.12f + 0.4f * lvl) * (enabled ? 1.0f : 0.4f)));
                g.fillRoundedRectangle (cell, 1.0f);
            }
        }

        // Optional vocal-rider display (Rider slider): a bar from the centre of the slot showing the live ride,
        // to the right when it's pushing the vocal up, to the left when it's pulling it down (full slot = +/-12 dB)
        if (s.getProperties().contains ("rideDb"))
        {
            const float ride = juce::jlimit (-12.0f, 12.0f, (float) s.getProperties()["rideDb"]);
            auto inner = slot.reduced (3.0f, 3.5f);
            const float mid = inner.getCentreX();
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.fillRect (mid - 0.5f, inner.getY() - 1.0f, 1.0f, inner.getHeight() + 2.0f);   // 0 dB mark
            if (std::abs (ride) > 0.05f && s.getValue() > 0.2)
            {
                const float bw = inner.getWidth() * 0.5f * std::abs (ride) / 12.0f;
                auto bar = ride > 0.0f ? juce::Rectangle<float> (mid, inner.getY(), bw, inner.getHeight())
                                       : juce::Rectangle<float> (mid - bw, inner.getY(), bw, inner.getHeight());
                const auto rideCol = ride > 0.0f ? juce::Colour (0xff3fcf6e) : pal::eq;   // green = up, yellow = down
                g.setColour (rideCol.withAlpha (enabled ? 0.85f : 0.3f));
                g.fillRoundedRectangle (bar, 2.0f);
            }
        }

        // thin colour line up to the cap (mode identity)
        const float from = bipolar ? (left + right) * 0.5f : left - 4.0f;
        auto line = juce::Rectangle<float> (juce::jmin (from, pos), cy - 1.0f, std::abs (pos - from), 2.0f);
        if (line.getWidth() > 0.5f && ! s.getProperties().contains ("meterDb")
                                   && ! s.getProperties().contains ("spectrum")
                                   && ! s.getProperties().contains ("rideDb"))   // a meter replaces the line
        {
            if (enabled)
            {
                g.setColour (col.withAlpha (0.25f));
                g.fillRoundedRectangle (line.expanded (0.0f, 2.0f), 2.0f);
            }
            g.setColour (col);
            g.fillRoundedRectangle (line, 1.0f);
        }

        // silver cap with grip ridges
        auto cap = juce::Rectangle<float> (18.0f, juce::jmin (22.0f, area.getHeight() - 2.0f)).withCentre ({ pos, cy });
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (cap.translated (1.0f, 2.5f).expanded (1.0f), 2.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff1f2f3), cap.getX(), cap.getY(),
                                                 juce::Colour (0xff8a8e93), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 2.5f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (cap, 2.5f, 1.0f);
        for (float dx : { -5.0f, -2.5f, 2.5f, 5.0f })   // ridges
        {
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawVerticalLine ((int) (pos + dx), cap.getY() + 3.0f, cap.getBottom() - 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawVerticalLine ((int) (pos + dx) + 1, cap.getY() + 3.0f, cap.getBottom() - 3.0f);
        }
        g.setColour (juce::Colour (0xff1a1a1c));   // index line
        g.fillRect (pos - 0.75f, cap.getY() + 2.0f, 1.5f, cap.getHeight() - 4.0f);
        if (! enabled)
        {
            g.setColour (pal::bg.withAlpha (0.5f));
            g.fillRoundedRectangle (cap, 2.5f);
        }
        return;
    }

    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0.0f, 0.0f, style, s);
        return;
    }

    // Modern fader: slim rounded track, gradient fill with a soft glow, white pill handle
    const auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    const bool enabled = s.isEnabled();
    const bool bipolar = s.getProperties()["bipolar"];
    const auto col = enabled ? s.findColour (juce::Slider::trackColourId) : pal::dim.withAlpha (0.4f);
    const float cx = area.getCentreX();
    const float top = area.getY(), bottom = area.getBottom();
    constexpr float trackW = 8.0f;

    // faint min / max at the ends
    g.setFont (font (8.5f, true));
    g.setColour (pal::text.withAlpha (enabled ? 0.35f : 0.2f));
    if (std::abs (pos - top) > 12.0f)      // hide an end label while the handle sits on it
        g.drawText (compactValue (s.getTextFromValue (s.getMaximum())), juce::Rectangle<float> (cx + 9.0f, top - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    if (std::abs (pos - bottom) > 12.0f)
        g.drawText (compactValue (s.getTextFromValue (s.getMinimum())), juce::Rectangle<float> (cx + 9.0f, bottom - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);

    // track with an inner shadow
    auto track = juce::Rectangle<float> (cx - trackW * 0.5f, top - 4.0f, trackW, bottom - top + 8.0f);
    g.setColour (juce::Colour (0xff08080a));
    g.fillRoundedRectangle (track, trackW * 0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.6f), cx - trackW * 0.5f, 0.0f,
                                             juce::Colours::transparentBlack, cx + trackW * 0.5f, 0.0f, false));
    g.fillRoundedRectangle (track, trackW * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (track.reduced (0.5f), trackW * 0.5f, 1.0f);

    // gradient fill with glow
    const float from = bipolar ? (top + bottom) * 0.5f : bottom + 4.0f;
    auto fill = juce::Rectangle<float> (cx - trackW * 0.5f, juce::jmin (from, pos), trackW, std::abs (from - pos));
    if (fill.getHeight() > 0.5f)
    {
        if (enabled)
        {
            g.setColour (col.withAlpha (0.18f));
            g.fillRoundedRectangle (fill.expanded (4.0f, 2.0f), trackW);
        }
        g.setGradientFill (juce::ColourGradient (col.brighter (0.25f), cx, fill.getY(), col.darker (0.45f), cx, fill.getBottom(), false));
        g.fillRoundedRectangle (fill, trackW * 0.5f);
    }

    // white pill handle with a coloured halo
    auto handle = juce::Rectangle<float> (24.0f, 10.0f).withCentre ({ cx, pos });
    if (enabled)
    {
        g.setColour (col.withAlpha (0.35f));
        g.fillRoundedRectangle (handle.expanded (3.0f), 8.0f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (handle.translated (0.0f, 2.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, cx, handle.getY(), juce::Colour (0xffc9ccd2), cx, handle.getBottom(), false));
    g.fillRoundedRectangle (handle, 5.0f);
    g.setColour (col.withAlpha (enabled ? 1.0f : 0.5f));   // thin colour line through the handle
    g.fillRoundedRectangle (handle.withSizeKeepingCentre (12.0f, 2.0f), 1.0f);
}

juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (12.5f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
            const juce::String& titleText, const juce::Colour& accentRef, bool bipolar)
    : Knob (state, paramId, titleText, ColourFn ([&accentRef] { return accentRef; }), bipolar) {}

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
            const juce::String& titleText, ColourFn accentFn, bool bipolar)
    : accent (std::move (accentFn))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    slider.getProperties().set ("bipolar", bipolar);
    addAndMakeVisible (slider);

    rawTitle = titleText;
    title.setText (isGoliath() ? titleText : titleText.toUpperCase(), juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    slider.addMouseListener (this, false);
    slider.onValueChange = [this] { if (hovering || slider.isMouseButtonDown()) showValueInTitle (true); };
    slider.onDragEnd = [this] { if (! hovering) showValueInTitle (false); };
    title.setFont (font (10.5f, true));
    title.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (title);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);

    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    lookAndFeelChanged();
}

void Knob::bind (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                 const juce::String& newTitle, int bipolar)
{
    if (newTitle.isNotEmpty()) { rawTitle = newTitle; title.setText (isGoliath() ? newTitle : newTitle.toUpperCase(), juce::dontSendNotification); }
    if (bipolar >= 0) slider.getProperties().set ("bipolar", bipolar == 1);
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    repaint();
}

void Knob::setLook (KnobStyle style, juce::Colour cap, juce::Colour text, juce::Colour titleColour)
{
    lookStyle = style;
    lookText = text;
    lookTitle = titleColour;
    slider.getProperties().set ("knobStyle", (int) style);
    slider.getProperties().set ("knobCap", (juce::int64) cap.getARGB());
    slider.getProperties().set ("knobText", (juce::int64) text.getARGB());
    lookAndFeelChanged();
}

void Knob::setFader (bool fader)
{
    // Fader = long horizontal slot with a silver cap: label left, value right
    slider.setSliderStyle (fader ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (fader ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow, false, fader ? 56 : 80, 16);
    title.setJustificationType (fader ? juce::Justification::centredLeft : juce::Justification::centred);
    resized();
    repaint();
}

void Knob::mouseEnter (const juce::MouseEvent&) { hovering = true; showValueInTitle (true); }
void Knob::mouseExit (const juce::MouseEvent&)  { hovering = false; if (! slider.isMouseButtonDown()) showValueInTitle (false); }

void Knob::showValueInTitle (bool show)
{
    // Goliath knobs have no value box: the label shows the value while you hover or turn the knob
    if (! isGoliath() || slider.getSliderStyle() == juce::Slider::LinearHorizontal || (bool) slider.getProperties()["minimal"]) return;
    title.setText (show ? slider.getTextFromValue (slider.getValue()) : rawTitle, juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, show ? goliathLegend() : goliathLabel());
}

void Knob::lookAndFeelChanged()
{
    const bool hw = lookStyle != KnobStyle::Skin;
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent());
    slider.setColour (juce::Slider::textBoxTextColourId, hw ? lookText : pal::text);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    title.setColour (juce::Label::textColourId, hw ? lookTitle : isGoliath() ? goliathLabel() : isOnyx() ? pal::text.withAlpha (0.78f) : isRack() ? accent() : pal::dim);
    if (isGoliath()) slider.setColour (juce::Slider::textBoxTextColourId, goliathLegend());
    title.setFont (isGoliath() ? font (13.0f) : font (10.5f, true));
    if (rawTitle.isNotEmpty()) title.setText (isGoliath() ? rawTitle : rawTitle.toUpperCase(), juce::dontSendNotification);
    if (isGoliath() && (bool) slider.getProperties()["minimal"])   // minimal knobs: their own name colour (e.g. on a hardware plate)
    {
        title.setFont (font (12.5f));
        title.setColour (juce::Label::textColourId, slider.getProperties().contains ("minimalTitle")
                                                        ? juce::Colour ((juce::uint32) (juce::int64) slider.getProperties()["minimalTitle"])
                                                        : juce::Colour (0xff9da0a8));
    }
    repaint();
}

void Knob::resized()
{
    auto r = getLocalBounds();
    if (slider.getSliderStyle() == juce::Slider::LinearHorizontal)
    {
        title.setBounds (r.removeFromLeft (r.getWidth() > 300 ? 84 : 70));
        slider.setBounds (r);
        return;
    }
    if (isGoliath() && (bool) slider.getProperties()["minimal"])   // V3 minimal: label on top, value in a box underneath
    {
        if (slider.getTextBoxPosition() != juce::Slider::NoTextBox)
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);   // the value box is drawn with the knob
        title.setFont (font (12.5f));
        title.setColour (juce::Label::textColourId, slider.getProperties().contains ("minimalTitle")
                                                        ? juce::Colour ((juce::uint32) (juce::int64) slider.getProperties()["minimalTitle"])
                                                        : juce::Colour (0xff9da0a8));
        title.setBounds (r.removeFromTop (17));
        slider.setBounds (r);
        return;
    }
    if (isGoliath())   // hardware style: no value box, the name engraved under the knob
    {
        if (slider.getTextBoxPosition() != juce::Slider::NoTextBox)
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        title.setBounds (r.removeFromBottom (17));
        slider.setBounds (r);
        return;
    }
    if (slider.getTextBoxPosition() == juce::Slider::NoTextBox)
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    title.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

//==============================================================================
ModeSelector::ModeSelector (juce::RangedAudioParameter& p, const juce::StringArray& n, const juce::Colour& a)
    : names (n), accent (&a),
      attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
}

void ModeSelector::paint (juce::Graphics& g)
{
    if (isGoliath() && pills)   // V60: caption + a compact row of pills, the chosen one lit
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        if (pillCaption.isNotEmpty())
        {
            g.setColour (goliathLabel());
            g.setFont (font (11.5f));
            g.drawText (pillCaption, r.removeFromLeft (pillCaptionWidth()), juce::Justification::centredLeft);
        }
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (juce::Colour (0xff2c2d31));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        const float w = r.getWidth() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
            const auto col = colourFor ? colourFor (i) : *accent;
            if (i == index)
            {
                g.setColour (col.withAlpha (0.22f));
                g.fillRoundedRectangle (seg, seg.getHeight() * 0.5f);
                g.setColour (col.withAlpha (0.85f));
                g.drawRoundedRectangle (seg, seg.getHeight() * 0.5f, 1.0f);
            }
            g.setColour (i == index ? col.brighter (0.35f) : juce::Colour (0xff8a8c93));
            g.setFont (font (11.0f, i == index));
            g.drawText (names[i], seg, juce::Justification::centred);
        }
        return;
    }
    if (isGoliath() && hwButtons > 0)   // V76: caption above a row of hardware push-buttons, the chosen one pressed and lit
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (hwLegend);
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.14f));
        g.drawText (hwCaption, r.removeFromTop (12.0f), juce::Justification::centred);
        r.removeFromTop (1.0f);
        const bool api = hwButtons == 1;
        const int n = juce::jmax (1, names.size());
        const float w = r.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            auto b = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f, 1.0f);
            const bool on = i == index;
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (b.translated (0.0f, on ? 0.6f : 1.6f), 2.5f);
            auto cap = on ? b.translated (0.0f, 1.0f) : b;
            if (api)   // light grey square caps
                g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffdedbd2) : juce::Colour (0xfff3f1ea), 0.0f, cap.getY(),
                                                         on ? juce::Colour (0xffaeaba3) : juce::Colour (0xffc8c5bd), 0.0f, cap.getBottom(), false));
            else       // dark console caps
                g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff3a3b3f) : juce::Colour (0xff4b4d52), 0.0f, cap.getY(),
                                                         on ? juce::Colour (0xff1c1d20) : juce::Colour (0xff2a2b2f), 0.0f, cap.getBottom(), false));
            g.fillRoundedRectangle (cap, 2.5f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawRoundedRectangle (cap, 2.5f, 1.0f);
            // lamp: a small lit window at the left of the cap
            const auto lampCol = api ? juce::Colour (0xff3ee07a) : juce::Colour (0xffff3b30);
            auto lamp = juce::Rectangle<float> (5.0f, cap.getHeight() * 0.42f).withCentre ({ cap.getX() + 7.0f, cap.getCentreY() });
            g.setColour (on ? lampCol : lampCol.withSaturation (0.25f).withBrightness (api ? 0.45f : 0.3f));
            g.fillRoundedRectangle (lamp, 1.5f);
            if (on)
            {
                g.setColour (lampCol.withAlpha (0.35f));
                g.fillRoundedRectangle (lamp.expanded (2.0f), 2.5f);
            }
            g.setColour (api ? juce::Colour (0xff15171a) : juce::Colour (0xffeeeeee));
            g.setFont (font (10.5f, on));
            g.drawText (names[i].toUpperCase(), cap.withTrimmedLeft (10.0f), juce::Justification::centred);
        }
        return;
    }
    if (isGoliath() && batLabels.size() == 2)
    {
        drawBatToggle (g, getLocalBounds().toFloat(), index == 0, batLabels[0], batLabels[1]);
        return;
    }
    if (isGoliath() && stepperLabel.isNotEmpty())   // LCD window like the Zerem display: caption, value, arrows either side
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const auto lcd = goliathAmber();
        g.setColour (juce::Colour (0xff050608));
        g.fillRoundedRectangle (r, 5.0f);
        g.setGradientFill (juce::ColourGradient (lcd.withAlpha (0.08f), r.getCentreX(), r.getCentreY(),
                                                 juce::Colours::transparentBlack, r.getX(), r.getY(), true));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        auto arrow = [&] (juce::Rectangle<float> a, bool left, bool enabled)
        {
            const auto c = a.getCentre();
            juce::Path p;
            if (left) p.addTriangle (c.x - 4.0f, c.y, c.x + 3.0f, c.y - 5.0f, c.x + 3.0f, c.y + 5.0f);
            else      p.addTriangle (c.x + 4.0f, c.y, c.x - 3.0f, c.y - 5.0f, c.x - 3.0f, c.y + 5.0f);
            g.setColour (lcd.withAlpha (enabled ? 0.9f : 0.25f));
            g.fillPath (p);
        };
        arrow (r.removeFromLeft (20.0f), true, index > 0);
        arrow (r.removeFromRight (20.0f), false, index < names.size() - 1);
        g.setColour (lcd.withAlpha (0.55f));
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.12f));
        g.drawText (stepperLabel, r.removeFromTop (r.getHeight() * 0.42f), juce::Justification::centredBottom);
        g.setColour (lcd);
        g.setFont (font (15.0f, true).withExtraKerningFactor (0.04f));
        g.drawText (names[index].toUpperCase(), r, juce::Justification::centredTop);
        return;
    }
    if (isGoliath() && dropdownLabel.isNotEmpty())   // compact dropdown: caption on the left, value + arrow on the right
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (isMouseOver() ? juce::Colour (0xff4a4a4e) : juce::Colour (0xff323236));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        auto t = r.reduced (10.0f, 0.0f);
        g.setColour (goliathLabel());
        g.setFont (font (11.5f, true));
        g.drawText (dropdownLabel, t, juce::Justification::centredLeft);
        juce::Path arrow;
        const auto c = juce::Point<float> (r.getRight() - 13.0f, r.getCentreY());
        arrow.addTriangle (c.x - 4.5f, c.y - 2.5f, c.x + 4.5f, c.y - 2.5f, c.x, c.y + 3.0f);
        g.fillPath (arrow);
        g.setColour (index == 0 ? goliathLabel() : goliathLegend());
        g.setFont (font (13.0f, true));
        g.drawText (names[index], t.withTrimmedRight (20.0f), juce::Justification::centredRight);
        return;
    }
    if (isGoliath() && vertical)   // column of LEDs, label to the right of each
    {
        const float rowH = (float) getHeight() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto row = juce::Rectangle<float> (0.0f, rowH * (float) i, (float) getWidth(), rowH);
            const float d = juce::jmin (13.0f, rowH - 4.0f);
            drawGoliathLed (g, juce::Rectangle<float> (d, d).withCentre ({ d * 0.5f + 1.0f, row.getCentreY() }), goliathAmber(), i == index);
            g.setColour (i == index ? goliathLegend() : goliathLabel().withAlpha (0.85f));
            g.setFont (font (13.0f));
            g.drawText (names[i], row.withTrimmedLeft (d + 8.0f), juce::Justification::centredLeft);
        }
        return;
    }
    if (isGoliath() && screenTabs)   // text tabs on the display, the selected one lit
    {
        const float w = (float) getWidth() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            g.setColour (i == index ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff76767c));
            g.setFont (screenFont (12.5f, 1).withExtraKerningFactor (0.12f));
            g.drawText (names[i].toUpperCase(), juce::Rectangle<float> (w * (float) i, 0.0f, w, (float) getHeight()), juce::Justification::centred);
        }
        return;
    }
    drawSegmented (g, getLocalBounds().toFloat(), names, index, colourFor ? colourFor (index) : *accent);
}

void ModeSelector::mouseDown (const juce::MouseEvent& e)
{
    if (isGoliath() && hwButtons > 0)   // V76: the button under the mouse
    {
        const int n = juce::jmax (1, names.size());
        const int next = juce::jlimit (0, n - 1, (int) ((float) e.x * (float) n / (float) juce::jmax (1, getWidth())));
        if (next != index) attachment.setValueAsCompleteGesture ((float) next);
        return;
    }
    if (isGoliath() && stepperLabel.isNotEmpty())   // left arrow steps back, right arrow (or the middle) steps on
    {
        int next = index;
        if (e.x < 22) next = juce::jmax (0, index - 1);
        else if (e.x > getWidth() - 22) next = juce::jmin (names.size() - 1, index + 1);
        else next = (index + 1) % names.size();
        if (next != index) attachment.setValueAsCompleteGesture ((float) next);
        return;
    }
    if (isGoliath() && dropdownLabel.isNotEmpty())
    {
        juce::PopupMenu m;
        for (int i = 0; i < names.size(); ++i) m.addItem (1 + i, names[i], true, i == index);
        juce::Component::SafePointer<ModeSelector> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                         [safe] (int r) { if (safe != nullptr && r > 0) safe->attachment.setValueAsCompleteGesture ((float) (r - 1)); });
        return;
    }
    if (isGoliath() && batLabels.size() == 2)   // flip the switch
    {
        attachment.setValueAsCompleteGesture ((float) (index == 0 ? 1 : 0));
        return;
    }
    if (isGoliath() && pills)
    {
        const float x0 = pillCaption.isNotEmpty() ? pillCaptionWidth() : 0.0f;
        const int i = juce::jlimit (0, names.size() - 1, (int) (((float) e.x - x0) / ((float) getWidth() - x0) * (float) names.size()));
        if (e.x >= x0 && i != index) attachment.setValueAsCompleteGesture ((float) i);
        return;
    }
    const int i = isGoliath() && vertical
                    ? juce::jlimit (0, names.size() - 1, (int) ((float) e.y / (float) getHeight() * (float) names.size()))
                    : juce::jlimit (0, names.size() - 1, (int) ((float) e.x / (float) getWidth() * (float) names.size()));
    if (i != index)
        attachment.setValueAsCompleteGesture ((float) i);
}

//==============================================================================
GrMeter::GrMeter (std::atomic<float>& src, const juce::Colour& a, juce::String l)
    : source (src), accent (&a), label (std::move (l))
{
    startTimerHz (30);
}

void GrMeter::timerCallback()
{
    const float v = source.load();
    shown = v > shown ? v : shown * 0.85f + v * 0.15f;
    repaint();
}

void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal::dim);
    g.setFont (font (10.0f, true));
    g.drawText (label, r.removeFromTop (14.0f), juce::Justification::centred);

    auto txt = r.removeFromBottom (16.0f);
    auto bar = r.reduced (3.0f, 2.0f);
    constexpr float range = 24.0f;
    const float frac = juce::jlimit (0.0f, 1.0f, shown / range);
    const auto col = *accent;

    if (isGoliath() && MISHKAL_V4)   // like the GAIN RED bars on the display: a solid red bar hanging from the top
    {
        constexpr float range = 24.0f;
        auto well = bar.withSizeKeepingCentre (juce::jmin (bar.getWidth(), 22.0f), bar.getHeight());
        auto marks = juce::Rectangle<float> (well.getRight() + 1.0f, well.getY(), bar.getRight() - well.getRight(), well.getHeight());
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRect (well.expanded (2.0f, 2.0f));
        g.setColour (juce::Colour (0xff1e1e20));
        for (float d : { 6.0f, 12.0f, 18.0f })
            g.drawHorizontalLine ((int) (well.getY() + well.getHeight() * d / range), well.getX(), well.getRight());
        const float frac = juce::jlimit (0.0f, 1.0f, shown / range);
        if (frac > 0.002f)
        {
            g.setColour (juce::Colour (0xffe2343e));
            g.fillRect (well.withHeight (juce::jmax (2.0f, well.getHeight() * frac)));
        }
        juce::ignoreUnused (marks);
        g.setColour (shown >= 0.05f ? juce::Colour (0xffe2343e) : goliathLegend());
        g.setFont (screenFont (11.5f, 1));
        g.drawText (shown >= 0.05f ? "-" + juce::String (shown, 1) : "0.0", txt, juce::Justification::centred);
        return;
    }
    if (isGoliath())   // (old) column of round LEDs, lit from the top down
    {
        constexpr int segs = 12;
        const float pitch = bar.getHeight() / (float) segs;
        const float d = juce::jmin (bar.getWidth() - 4.0f, pitch - 2.0f, 11.0f);
        g.setColour (juce::Colour (0xff09090a));
        g.fillRoundedRectangle (bar.withSizeKeepingCentre (d + 8.0f, bar.getHeight() + 4.0f), 3.0f);
        for (int i = 0; i < segs; ++i)
        {
            const auto cc = juce::Point<float> (bar.getCentreX(), bar.getY() + pitch * ((float) i + 0.5f));
            const bool lit = frac * segs > (float) i + 0.25f;
            drawGoliathLed (g, juce::Rectangle<float> (d, d).withCentre (cc), i < 4 ? goliathAmber() : juce::Colour (0xffff4a3d), lit);
        }
        g.setColour (goliathLegend());
        g.setFont (font (10.5f));
        g.drawText (shown >= 0.05f ? "-" + juce::String (shown, 1) : "0.0", txt, juce::Justification::centred);
        return;
    }

    const auto ledCol = colourFor ? colourFor() : isOnyx() ? pal::alert : isRack() ? pal::meter : col;
    g.setColour (isMetal() ? juce::Colour (0xff0d0d0e) : pal::bg.darker (0.2f));
    g.fillRoundedRectangle (bar, 3.0f);

    if (isMetal())   // LED ladder: 12 segments, lit from the top down
    {
        constexpr int segs = 12;
        const float gap = 2.0f, segH = (bar.getHeight() - 4.0f - gap * (segs - 1)) / segs;
        for (int i = 0; i < segs; ++i)
        {
            auto seg = juce::Rectangle<float> (bar.getX() + 3.0f, bar.getY() + 2.0f + i * (segH + gap), bar.getWidth() - 6.0f, segH);
            const bool lit = frac * segs > (float) i + 0.25f;
            g.setColour (lit ? ledCol : ledCol.withAlpha (0.12f));
            g.fillRoundedRectangle (seg, 1.5f);
            if (lit)
            {
                g.setColour (ledCol.withAlpha (0.25f));
                g.drawRoundedRectangle (seg.expanded (1.0f), 2.0f, 1.5f);
            }
        }
    }
    else
    {
        auto fill = bar.withHeight (bar.getHeight() * frac);
        if (frac > 0.0f)
        {
            if (isModern())
            {
                g.setColour (col.withAlpha (0.25f));
                g.fillRoundedRectangle (fill.expanded (2.0f, 1.0f), 4.0f);
            }
            g.setGradientFill (juce::ColourGradient (col.darker (0.3f), 0.0f, fill.getY(), col.brighter (0.25f), 0.0f, fill.getBottom(), false));
            g.fillRoundedRectangle (fill, 3.0f);
        }
        g.setColour (pal::line.withAlpha (0.7f));
        for (float db : { 6.0f, 12.0f, 18.0f })
            g.drawHorizontalLine ((int) (bar.getY() + bar.getHeight() * db / range), bar.getX(), bar.getRight());
    }

    g.setColour (pal::text);
    g.setFont (font (10.5f));
    g.drawText (shown >= 0.05f ? "-" + juce::String (shown, 1) : "0.0", txt, juce::Justification::centred);
}

//==============================================================================
GateLed::GateLed (std::atomic<float>& src) : source (src) { startTimerHz (30); }

void GateLed::timerCallback()
{
    const float v = juce::jlimit (0.0f, 1.0f, source.load() / 20.0f);
    if (std::abs (v - shown) > 0.01f) { shown = v; repaint(); }
}

void GateLed::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto dot = r.removeFromLeft (r.getHeight()).reduced (3.0f);
    g.setColour (pal::bg.darker (0.3f));
    g.fillEllipse (dot);
    if (shown > 0.05f)
    {
        g.setColour (juce::Colour (0xffd9342b).withAlpha (0.3f * shown));
        g.fillEllipse (dot.expanded (2.5f));
    }
    g.setColour (juce::Colour (0xffd9342b).withAlpha (0.2f + 0.8f * shown));
    g.fillEllipse (dot.reduced (1.5f));
    if (r.getWidth() > 12.0f)   // room for a label
    {
        g.setColour (pal::dim);
        g.setFont (font (10.0f, true));
        g.drawText ("GATE", r.withTrimmedLeft (3.0f), juce::Justification::centredLeft);
    }
}

//==============================================================================
void TapeVu::timerCallback()
{
    // VU ballistics: about 300 ms to settle. 0 VU = -18 dBFS RMS
    const float target = juce::jlimit (-22.0f, 4.0f, source.load() + 18.0f);
    needle += (target - needle) * 0.18f;
    if (needle >= peakHold) { peakHold = needle; holdFrames = 45; }          // peak marker holds 1.5 s
    else if (--holdFrames < 0) peakHold = juce::jmax (needle, peakHold - 0.6f);
    repaint();
}

// LED VU ladder, like a tape deck's: glowing segments on real VU steps (yellow up to 0 VU, red above),
// a printed scale, a peak-hold LED and the live reading underneath
void TapeVu::paint (juce::Graphics& g)
{
    const ScreenColours screenColours;
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (r, 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.8f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 10.0f, false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    const bool live = needle > -21.5f;
    auto inner = r.reduced (6.0f, 6.0f);
    auto readout = inner.removeFromBottom (13.0f);
    inner.removeFromBottom (3.0f);

    // classic tape-deck VU steps, top to bottom
    static constexpr float steps[] { 3.0f, 2.0f, 1.0f, 0.0f, -1.0f, -2.0f, -3.0f, -5.0f, -7.0f, -10.0f, -15.0f, -20.0f };
    constexpr int n = (int) (sizeof (steps) / sizeof (steps[0]));
    auto ledCol = inner.removeFromLeft (16.0f);
    inner.removeFromLeft (5.0f);
    const float gap = 1.5f;
    const float h = (ledCol.getHeight() - gap * (float) (n - 1)) / (float) n;
    // the peak-hold LED: the highest step at or below the held peak
    int holdIdx = -1;
    if (live) for (int i = 0; i < n; ++i) if (peakHold >= steps[i] - 0.5f) { holdIdx = i; break; }

    for (int i = 0; i < n; ++i)
    {
        auto led = juce::Rectangle<float> (ledCol.getX(), ledCol.getY() + (float) i * (h + gap), ledCol.getWidth(), h);
        const auto col = steps[i] > 0.0f ? pal::alert : pal::eq;
        const bool lit = live && needle >= steps[i] - 0.5f;
        if (lit || i == holdIdx)
        {
            g.setColour (col.withAlpha (0.3f));
            g.fillRoundedRectangle (led.expanded (2.0f, 1.5f), 2.5f);   // glow
            g.setColour (col);
        }
        else
            g.setColour (col.withAlpha (steps[i] > 0.0f ? 0.2f : 0.1f));
        g.fillRoundedRectangle (led, 1.5f);

        // scale: print the landmark steps
        if (steps[i] == 3.0f || steps[i] == 0.0f || steps[i] == -3.0f || steps[i] == -7.0f || steps[i] == -20.0f)
        {
            g.setColour (steps[i] > 0.0f ? pal::alert.withAlpha (0.85f) : (steps[i] == 0.0f ? pal::text : pal::dim));
            g.setFont (font (8.0f, true));
            g.drawText ((steps[i] > 0.0f ? "+" : "") + juce::String ((int) steps[i]),
                        juce::Rectangle<float> (inner.getX(), led.getCentreY() - 5.0f, inner.getWidth(), 10.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    // live reading
    g.setColour (live ? (needle > 0.0f ? pal::alert : pal::text) : pal::dim.withAlpha (0.6f));
    g.setFont (font (10.5f, true));
    const auto txt = live ? (needle >= 0.0f ? "+" : "") + juce::String (needle, 1) + " VU" : juce::String ("-- VU");
    g.drawText (txt, readout, juce::Justification::centred, false);
}

//==============================================================================
IoStrip::IoStrip (std::atomic<float> (&p)[2]) : peaks (p) { startTimerHz (30); }

void IoStrip::timerCallback()
{
    for (size_t c = 0; c < 2; ++c)
    {
        const float lin = peaks[c].exchange (0.0f);
        const float db = juce::Decibels::gainToDecibels (lin, -100.0f);
        shown[c] = db > shown[c] ? db : juce::jmax (db, shown[c] - 0.9f);        // instant rise, ~27 dB/s fall
        if (db >= hold[c]) { hold[c] = db; holdFrames[c] = 45; }                // peak line holds 1.5 s
        else if (--holdFrames[c] < 0) hold[c] = juce::jmax (db, hold[c] - 0.9f);
        maxDb = juce::jmax (maxDb, db);
        if (lin >= 0.999f) clip[c] = true;                                      // 0 dBFS: latch until clicked
    }
    repaint();
}

void IoStrip::mouseDown (const juce::MouseEvent& e)
{
    if (loudness != nullptr && lufsArea.contains (e.getPosition()))
    {
        loudness->resetRequest = true;   // start a fresh integrated reading
        return;
    }
    if (meterArea.contains (e.getPosition()))
    {
        maxDb = -100.0f;
        clip = {};
        hold = shown;
        repaint();
    }
}

void IoStrip::resized()
{
    if (compact)
    {
        auto r = getLocalBounds().reduced (2, 0);
        if (knob != nullptr) knob->setBounds (r.removeFromTop (100));
        r.removeFromTop (4);
        if (button != nullptr)
        {
            button->setBounds (r.removeFromTop (20).reduced (4, 0));
            r.removeFromTop (6);
        }
        if (loudness != nullptr)
        {
            lufsArea = r.removeFromBottom (36).reduced (4, 0);
            r.removeFromBottom (6);
        }
        meterArea = r.reduced (4, 0);
        return;
    }
    auto r = getLocalBounds().reduced (8, 0);
    r.removeFromTop (16);
    if (knob != nullptr) knob->setBounds (r.removeFromTop (100));
    r.removeFromTop (8);
    if (button != nullptr)
    {
        button->setBounds (r.removeFromTop (24));
        r.removeFromTop (10);
    }
    r.removeFromBottom (18);
    if (loudness != nullptr)
    {
        lufsArea = r.removeFromBottom (40);
        r.removeFromBottom (6);
    }
    meterArea = r;
}

void IoStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float rad = isMetal() ? 3.0f : 10.0f;

    // panel, in the same finish as the sections (Goliath: sits straight on the black display)
    if (! isGoliath())
    {
    g.setGradientFill (juce::ColourGradient (pal::panelHi, 0.0f, r.getY(), pal::panel.darker (0.2f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    if (isRack())
    {
        setBrushFill (g, isOnyx() ? 0.45f : 1.0f);
        g.fillRoundedRectangle (r, rad);
    }
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawHorizontalLine (1, r.getX() + 3.0f, r.getRight() - 3.0f);
    g.setColour (isMetal() ? juce::Colours::black.withAlpha (0.7f) : pal::line);
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    if (isMetal())
    {
        const float in = 9.0f;
        drawScrew (g, { r.getX() + in, r.getY() + in }, 3.4f, 0.6f);
        drawScrew (g, { r.getRight() - in, r.getY() + in }, 3.4f, 2.1f);
        drawScrew (g, { r.getX() + in, r.getBottom() - in }, 3.4f, 1.2f);
        drawScrew (g, { r.getRight() - in, r.getBottom() - in }, 3.4f, 0.2f);
    }
    }

    // meter layout: clip lights / two bars + scale / peak readout
    auto m = meterArea.toFloat();
    auto readout = m.removeFromBottom (20.0f);
    m.removeFromBottom (6.0f);
    auto clipRow = m.removeFromTop (7.0f);
    m.removeFromTop (4.0f);

    constexpr float barW = 11.0f, barGap = 3.0f, pad = 3.0f;
    auto well = m.withWidth (barW * 2.0f + barGap + pad * 2.0f);
    auto scale = m.withTrimmedLeft (well.getWidth() + 4.0f);

    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (well, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (well.reduced (0.5f), 3.0f, 1.0f);

    constexpr float minDb = -60.0f;
    auto bars = well.reduced (pad);
    auto yFor = [&] (float db) { return bars.getBottom() - bars.getHeight() * juce::jlimit (0.0f, 1.0f, (db - minDb) / -minDb); };
    auto colourFor = [] (float db)
    {
        return db > -3.0f ? juce::Colour (0xffd9342b) : db > -12.0f ? juce::Colour (0xffe8c547) : juce::Colour (0xff3fcf6e);
    };

    const float segH = isGoliath() ? 3.5f : 3.0f, segGap = isGoliath() ? 2.0f : 1.0f;
    const int segs = (int) ((bars.getHeight() + segGap) / (segH + segGap));
    const float alpha = isEnabled() ? 1.0f : 0.4f;
    for (int c = 0; c < 2; ++c)
    {
        const float x = bars.getX() + (float) c * (barW + barGap);
        for (int i = 0; i < segs; ++i)
        {
            const float y = bars.getBottom() - (float) (i + 1) * (segH + segGap) + segGap;
            const float segDb = minDb + -minDb * ((float) i + 0.5f) / (float) segs;
            const bool lit = shown[(size_t) c] >= segDb;
            g.setColour (lit ? colourFor (segDb).withAlpha (0.92f * alpha) : juce::Colours::white.withAlpha (0.045f));
            if (isGoliath())
            {
                if (lit) { g.setColour (colourFor (segDb).withAlpha (0.25f * alpha)); g.fillRoundedRectangle (x - 1.0f, y - 1.0f, barW + 2.0f, segH + 2.0f, 2.0f);
                           g.setColour (colourFor (segDb).brighter (0.2f).withAlpha (alpha)); }
                else g.setColour (colourFor (segDb).withMultipliedBrightness (0.22f));
                g.fillRoundedRectangle (x, y, barW, segH, 1.5f);
            }
            else
                g.fillRect (x, y, barW, segH);
        }
        if (hold[(size_t) c] > minDb)   // peak-hold line
        {
            g.setColour (colourFor (hold[(size_t) c]).brighter (0.3f));
            g.fillRect (x, yFor (hold[(size_t) c]) - 1.0f, barW, 2.0f);
        }
        // clip light
        auto led = juce::Rectangle<float> (x, clipRow.getY(), barW, clipRow.getHeight());
        g.setColour (clip[(size_t) c] ? juce::Colour (0xffff3b30) : juce::Colour (0xff2a1414));
        g.fillRoundedRectangle (led, 1.5f);
        if (clip[(size_t) c])
        {
            g.setColour (juce::Colour (0xffff3b30).withAlpha (0.35f));
            g.fillRoundedRectangle (led.expanded (2.0f), 2.5f);
        }
    }

    // dB scale
    g.setFont (font (9.0f, true));
    for (int db : { 0, -3, -6, -12, -18, -24, -36, -48, -60 })
    {
        const float y = yFor ((float) db);
        g.setColour (pal::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (scale.getX() + 1.0f, y - 6.0f, scale.getWidth() - 3.0f, 12.0f),
                    juce::Justification::centredLeft, false);
    }

    // peak readout (the highest level since the last click)
    g.setColour (juce::Colour (0xff0b0b0d));
    g.fillRoundedRectangle (readout, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (readout.reduced (0.5f), 3.0f, 1.0f);
    const bool clipped = clip[0] || clip[1];
    g.setColour (clipped ? juce::Colour (0xffff3b30) : screenText());
    g.setFont (font (11.0f, true));
    g.drawText (maxDb <= -99.0f ? juce::String ("-inf") : juce::String (maxDb, 1), readout, juce::Justification::centred);

    // LUFS: short-term (last 3 s) and integrated (since reset; click to reset)
    if (loudness != nullptr)
    {
        auto box = lufsArea.toFloat();
        g.setColour (juce::Colour (0xff0b0b0d));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (box.reduced (0.5f), 3.0f, 1.0f);
        auto inner = box.reduced (5.0f, 3.0f);
        g.setColour (screenDim());
        g.setFont (font (8.0f, true));
        g.drawText ("LUFS", inner.removeFromTop (10.0f), juce::Justification::centred);
        auto line = [&] (const char* tag, float v, bool main)
        {
            auto row = inner.removeFromTop (12.0f);
            g.setColour (screenDim());
            g.setFont (font (8.5f, true));
            g.drawText (tag, row.removeFromLeft (12.0f), juce::Justification::centredLeft);
            g.setColour (main ? screenText() : screenText().withAlpha (0.8f));
            g.setFont (font (main ? 11.0f : 10.0f, true));
            g.drawText (v <= -69.0f ? juce::String ("--") : juce::String (v, 1), row, juce::Justification::centredRight);
        };
        line ("S", loudness->shortTerm.load(), false);
        line ("I", loudness->integrated.load(), true);
    }
}

//==============================================================================
LevelMeter::LevelMeter (std::atomic<float>& src) : source (src) { startTimerHz (30); }

void LevelMeter::timerCallback()
{
    const float v = source.load();
    shown = v > shown ? v : juce::jmax (v, shown - 1.2f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (isMetal() ? juce::Colour (0xff0d0d0e) : pal::bg.darker (0.3f));
    g.fillRoundedRectangle (r, 2.0f);
    const float frac = juce::jlimit (0.0f, 1.0f, (shown + 60.0f) / 60.0f);

    // green -> amber -> red (Rack: amber -> orange -> red, like big VU ladders)
    juce::ColourGradient grad (isRack() ? pal::meter.darker (0.35f) : pal::comp, 0.0f, r.getBottom(),
                               isRack() ? (isOnyx() ? pal::alert : juce::Colour (0xffff4a2a)) : pal::limiter, 0.0f, r.getY(), false);
    grad.addColour (0.75, isRack() ? pal::meter.brighter (0.2f) : pal::eq);
    g.setGradientFill (grad);

    if (isMetal())
    {
        constexpr int segs = 14;
        const float gap = 1.5f, segH = (r.getHeight() - gap * (segs - 1)) / segs;
        for (int i = 0; i < segs; ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + 1.0f, r.getBottom() - (i + 1) * segH - i * gap, r.getWidth() - 2.0f, segH);
            if (frac * segs > (float) i + 0.5f) g.fillRect (seg);
        }
    }
    else
        g.fillRoundedRectangle (r.withTrimmedTop (r.getHeight() * (1.0f - frac)), 2.0f);
}

//==============================================================================
TypeBar::TypeBar (juce::AudioProcessorValueTreeState& s, juce::StringArray n,
                  std::function<juce::String (int)> onId, std::function<juce::Colour (int)> colours,
                  juce::Identifier prop, int defaultTab, juce::String tip, juce::String orderId)
    : state (s), names (std::move (n)), onParamId (std::move (onId)), colourFor (std::move (colours)),
      tabProperty (prop)
{
    orderParamId = std::move (orderId);
    selected = juce::jlimit (0, names.size() - 1, (int) state.state.getProperty (tabProperty, defaultTab));
    setTooltip (tip);
    startTimerHz (30);
}

std::vector<int> TypeBar::order() const
{
    std::vector<int> o;
    if (orderParamId.isNotEmpty())
    {
        const int idx = juce::roundToInt (state.getRawParameterValue (orderParamId)->load());
        if (names.size() == 4) { auto p = permutationFromIndex<4> (idx); o.assign (p.begin(), p.end()); }
        if (names.size() == 3) { auto p = permutationFromIndex<3> (idx); o.assign (p.begin(), p.end()); }
        if (names.size() == 9)   // V75: nine compressors; 0 = the older four-unit order, then the new ones
        {
            if (idx > 0) { auto p = permutationFromIndexFast<9> (idx); o.assign (p.begin(), p.end()); }
            else if (auto* old4 = state.getRawParameterValue ("compOrder4"))
            {
                auto p = permutationFromIndex<4> (juce::jlimit (0, 23, juce::roundToInt (old4->load())));
                o.assign (p.begin(), p.end());
                for (int i = 4; i < 9; ++i) o.push_back (i);
            }
        }
        if (names.size() == 8)   // V74: eight character EQs; 0 = the older four-EQ order, then the new ones
        {
            if (idx > 0) { auto p = permutationFromIndexFast<8> (idx); o.assign (p.begin(), p.end()); }
            else if (auto* old4 = state.getRawParameterValue ("eqOrder"))
            {
                auto p = permutationFromIndex<4> (juce::jlimit (0, 23, juce::roundToInt (old4->load())));
                o.assign (p.begin(), p.end());
                for (int i = 4; i < 8; ++i) o.push_back (i);
            }
        }
    }
    if ((int) o.size() != names.size())
        for (int i = 0; i < names.size(); ++i) o.push_back (i);
    return o;
}

void TypeBar::setOrder (const std::vector<int>& o)
{
    if (orderParamId.isEmpty()) return;
    int idx = 0;
    if (names.size() == 4) { std::array<int, 4> p {}; std::copy (o.begin(), o.end(), p.begin()); idx = indexFromPermutation (p); }
    if (names.size() == 3) { std::array<int, 3> p {}; std::copy (o.begin(), o.end(), p.begin()); idx = indexFromPermutation (p); }
    if (names.size() == 8 && o.size() == 8) { std::array<int, 8> p {}; std::copy (o.begin(), o.end(), p.begin()); idx = indexFromPermutationFast (p); }
    if (names.size() == 9 && o.size() == 9) { std::array<int, 9> p {}; std::copy (o.begin(), o.end(), p.begin()); idx = indexFromPermutationFast (p); }
    if (auto* p = state.getParameter (orderParamId))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
    repaint();
}

bool TypeBar::isActive (int t) const
{
    return state.getRawParameterValue (onParamId (t))->load() > 0.5f;
}

void TypeBar::setSelected (int t)
{
    selected = juce::jlimit (0, names.size() - 1, t);
    state.state.setProperty (tabProperty, selected, nullptr);   // remembered with the session
    repaint();
    if (onSelect) onSelect (selected);
}

int TypeBar::slotAt (float x) const
{
    if (isGoliath() && vertical) return juce::jlimit (0, names.size() - 1, (int) (x / ((float) getHeight() / (float) names.size())));
    return juce::jlimit (0, names.size() - 1, (int) (x / ((float) getWidth() / (float) names.size())));
}

float TypeBar::ledX (int slot) const
{
    if (isGoliath() && vertical) { juce::ignoreUnused (slot); return 7.0f; }
    // the LED sits just left of the centred label
    const float w = (float) getWidth() / (float) names.size();
    const float textW = juce::GlyphArrangement::getStringWidth (font (11.0f, true), names[order()[(size_t) slot]]);
    return w * (float) slot + w * 0.5f - textW * 0.5f - 9.0f;
}

void TypeBar::paint (juce::Graphics& g)
{
    if (isGoliath() && vertical)
    {
        // a column like the hardware: light (in / out) + name; the one being edited is lit up
        const auto o = order();
        const float rowH = (float) getHeight() / (float) o.size();
        for (int slot = 0; slot < (int) o.size(); ++slot)
        {
            const int t = o[(size_t) slot];
            auto row = juce::Rectangle<float> (0.0f, rowH * (float) slot, (float) getWidth(), rowH);
            const float d = juce::jmin (13.0f, rowH - 4.0f);
            drawGoliathLed (g, juce::Rectangle<float> (d, d).withCentre ({ ledX (slot), row.getCentreY() }), goliathAmber(), isActive (t));
            const bool sel = t == selected;
            g.setColour (sel ? goliathLegend() : goliathLabel().withAlpha (0.85f));
            g.setFont (font (13.0f, sel));
            g.drawText (names[t].fromFirstOccurrenceOf ("LE-", false, false), row.withTrimmedLeft (d + 10.0f), juce::Justification::centredLeft);
            if (dragging && t == dragUnit)
            {
                g.setColour (goliathAmber().withAlpha (0.6f));
                g.drawRoundedRectangle (row.reduced (0.5f), 3.0f, 1.0f);
            }
        }
        return;
    }
    const auto o = order();
    juce::StringArray padded;   // names in processing order, padded so the LED sits to their left
    int selectedSlot = 0;
    for (int slot = 0; slot < (int) o.size(); ++slot)
    {
        padded.add ("    " + names[o[(size_t) slot]]);
        if (o[(size_t) slot] == selected) selectedSlot = slot;
    }
    drawSegmented (g, getLocalBounds().toFloat(), padded, selectedSlot, colourFor (selected));

    const float w = (float) getWidth() / (float) o.size();
    for (int slot = 0; slot < (int) o.size(); ++slot)
    {
        const int t = o[(size_t) slot];
        const auto lit = colourFor (t);   // each unit's light in its own colour
        auto led = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ ledX (slot), (float) getHeight() * 0.5f });
        if (isGoliath())
            drawGoliathLed (g, led.expanded (1.5f), goliathAmber(), isActive (t));
        else if (isActive (t))
        {
            g.setColour (lit.withAlpha (0.45f));
            g.fillEllipse (led.expanded (3.0f));
            g.setGradientFill (juce::ColourGradient (lit.brighter (0.35f), led.getCentreX() - 1.0f, led.getCentreY() - 1.5f,
                                                     lit.darker (0.15f), led.getRight(), led.getBottom(), true));   // deep lens, small hot spot
        }
        else
            g.setColour (juce::Colour (0xff1a1a1c));
        if (! isGoliath())
        {
            g.fillEllipse (led);
            g.setColour (isActive (t) ? juce::Colours::black.withAlpha (0.6f) : pal::dim.withAlpha (0.7f));   // unlit LEDs keep a visible rim
            g.drawEllipse (led, 1.0f);
        }

        // order number + the tab being dragged gets an outline
        if (orderParamId.isNotEmpty() && o.size() > 1)
        {
            g.setColour (pal::dim.withAlpha (0.6f));
            g.setFont (font (8.5f, true));
            g.drawText (juce::String (slot + 1), juce::Rectangle<float> (w * (float) slot + 4.0f, 2.0f, 10.0f, 10.0f), juce::Justification::centred);
        }
        if (dragging && t == dragUnit)
        {
            g.setColour (lit.withAlpha (0.8f));
            g.drawRoundedRectangle (juce::Rectangle<float> (w * (float) slot, 0.0f, w, (float) getHeight()).reduced (1.5f), 4.0f, 1.5f);
        }
    }
}

void TypeBar::mouseDown (const juce::MouseEvent& e)
{
    downSlot = slotAt ((float) (isGoliath() && vertical ? e.y : e.x));
    dragUnit = order()[(size_t) downSlot];
    dragging = false;
}

void TypeBar::mouseDrag (const juce::MouseEvent& e)
{
    if (orderParamId.isEmpty() || downSlot < 0) return;
    const bool vert = isGoliath() && vertical;
    if (! dragging && std::abs (vert ? e.getDistanceFromDragStartY() : e.getDistanceFromDragStartX()) > 6)
    {
        dragging = true;
        setMouseCursor (vert ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
        if (auto* p = state.getParameter (orderParamId)) p->beginChangeGesture();
    }
    if (! dragging) return;

    // move the dragged unit to the slot under the mouse; the others shift along
    auto o = order();
    const int from = (int) (std::find (o.begin(), o.end(), dragUnit) - o.begin());
    const int to = slotAt ((float) (vert ? e.y : e.x));
    if (from != to)
    {
        o.erase (o.begin() + from);
        o.insert (o.begin() + to, dragUnit);
        setOrder (o);
    }
    repaint();
}

void TypeBar::mouseUp (const juce::MouseEvent& e)
{
    if (dragging)
    {
        dragging = false;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        if (auto* p = state.getParameter (orderParamId)) p->endChangeGesture();
        repaint();
        return;
    }
    if (downSlot < 0) return;

    const int t = order()[(size_t) downSlot];
    if (std::abs ((float) e.getMouseDownX() - ledX (downSlot)) < 12.0f)   // the light: switch this unit in/out
        toggleUnit (t);
    else
        setSelected (t);
    downSlot = -1;
}

//==============================================================================
Section::Section (juce::AudioProcessorValueTreeState& state, const juce::String& t,
                  const juce::String& powerParamId, const juce::Colour& a)
    : accent (&a), title (t), powerAttachment (state, powerParamId, power)
{
    power.setTooltip ("Section on/off");
    addAndMakeVisible (power);
    lookAndFeelChanged();
}

void Section::lookAndFeelChanged()
{
    power.setColour (juce::ToggleButton::tickColourId, *accent);
    resized();
    repaint();
}

void Section::resized()
{
    if (isGoliath() && compact) { power.setBounds (12, 42 + (getHeight() - 52 - 74) / 2, 52, 74); return; }   // V36: centred in the slim unit
    if (isGoliath()) { power.setBounds (12, MISHKAL_V52 ? 54 : 90, 52, 74); return; }   // On / Bypass bat toggle, top left of the module body
    const int left = isMetal() ? 22 : 12;   // room for the faceplate screws / rack holes
    power.setBounds (getLocalBounds().withTrimmedLeft (left).removeFromTop (38).reduced (0, 9).removeFromLeft (20));
}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float rad = isMetal() ? 4.0f : 10.0f;
    const int titleX = isMetal() ? 50 : 40;

    if (isGoliath())
    {
        // 500-series module: brushed black faceplate, four cross-head screws, sage title on a black tab
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff232325), 0.0f, r.getY(), juce::Colour (0xff161618), 0.0f, r.getBottom(), false));
        g.fillRect (r);
        setBrushFill (g, 0.7f);
        g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawHorizontalLine (0, r.getX(), r.getRight());
        g.drawVerticalLine (0, r.getY(), r.getBottom());
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawVerticalLine ((int) r.getRight() - 1, r.getY(), r.getBottom());
        g.drawHorizontalLine ((int) r.getBottom() - 1, r.getX(), r.getRight());

        if (! compact && ! MISHKAL_V52)   // V52: no screws - more room, less clutter
        {
            const float sy0 = 22.0f, sy1 = r.getBottom() - 22.0f;
            struct ScrewPos { float fx, fy, angle; };
            const ScrewPos screws[] { { 0.25f, sy0, 0.4f }, { 0.75f, sy0, 1.1f }, { 0.25f, sy1, 0.9f }, { 0.75f, sy1, 0.2f } };
            for (const auto& sp : screws)
                drawGoliathScrew (g, { r.getWidth() * sp.fx, sp.fy }, 11.0f, sp.angle);
        }

        auto tab = juce::Rectangle<float> (r.getX(), compact ? 4.0f : MISHKAL_V52 ? 8.0f : 44.0f, r.getWidth(), compact ? 30.0f : 32.0f);
        g.setColour (juce::Colour (0xff0c0c0d));
        g.fillRect (tab);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine ((int) tab.getBottom(), tab.getX(), tab.getRight());
        if (const int step = MISHKAL_V83 ? 0 : (int) getProperties().getWithDefault ("stepNumber", 0); step > 0)   // V42: where it sits in the signal flow (V83: off, the order can change)
        {
            const auto badge = juce::Rectangle<float> (18.0f, 18.0f).withCentre ({ tab.getX() + 21.0f, tab.getCentreY() });
            g.setColour (juce::Colour (0xff2c2e33));
            g.fillEllipse (badge);
            g.setColour (juce::Colours::white);
            g.setFont (font (11.0f, true));
            g.drawText (juce::String (step), badge, juce::Justification::centred);
            tab = tab.withTrimmedLeft (24.0f);
        }
        float size = 19.0f;
        const auto shown = getProperties().getWithDefault ("goliathTitle", title).toString();
        while (size > 11.0f && juce::GlyphArrangement::getStringWidth (font (size, true).withExtraKerningFactor (0.04f), shown) > tab.getWidth() - 24.0f) size -= 0.5f;
        g.setColour (goliathSage());
        g.setFont (font (size, true).withExtraKerningFactor (0.04f));
        const auto sub = getProperties().getWithDefault ("goliathSub", {}).toString();
        if (sub.isNotEmpty())   // V80: the name, with its meaning in small letters under it
        {
            auto t = tab.withTrimmedLeft (12.0f);
            auto top = t.removeFromTop (t.getHeight() * 0.64f);
            g.drawText (shown, top.translated (0.0f, 3.0f), juce::Justification::bottomLeft);
            g.setColour (goliathSage().withAlpha (0.6f));
            g.setFont (font (10.5f).withExtraKerningFactor (0.12f));
            g.drawText (sub, t.translated (1.0f, 1.0f), juce::Justification::topLeft);
            return;
        }
        g.drawText (shown, tab.withTrimmedLeft (12.0f), juce::Justification::centredLeft);
        return;
    }

    if (look.active)   // classic-hardware faceplate for the selected mode
    {
        paintFaceplate (g, r, look, isMetal() ? 3.0f : 10.0f);
        if (isMetal() && ! look.modern)
        {
            const float in = 9.0f;
            if (screwsLeft)  { drawScrew (g, { r.getX() + in, r.getY() + in }, 3.4f, 0.6f); drawScrew (g, { r.getX() + in, r.getBottom() - in }, 3.4f, 1.2f); }
            if (screwsRight) { drawScrew (g, { r.getRight() - in, r.getY() + in }, 3.4f, 2.1f); drawScrew (g, { r.getRight() - in, r.getBottom() - in }, 3.4f, 0.2f); }
        }
        auto groove = [&] (float xTop, float xBottom)
        {
            g.setColour (look.groove.withAlpha (0.85f));
            g.drawLine (xTop, r.getY() + 3.0f, xBottom, r.getBottom() - 3.0f, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (look.lightPlate ? 0.35f : 0.09f));
            g.drawLine (xTop + 1.5f, r.getY() + 3.0f, xBottom + 1.5f, r.getBottom() - 3.0f, 1.0f);
        };
        if (isRack())
        {
            for (int x : dividers) groove ((float) x + 10.0f, (float) x - 10.0f);
            if (grooveLeft) groove (16.0f, 2.0f);
        }
        g.setFont (font (13.0f, true));
        g.setColour (look.lightPlate ? juce::Colours::white.withAlpha (0.4f) : juce::Colours::black.withAlpha (0.6f));
        g.drawText (title, juce::Rectangle<int> (titleX, 1, 300, 38), juce::Justification::centredLeft);
        g.setColour (look.text);
        g.drawText (title, juce::Rectangle<int> (titleX, 0, 300, 38), juce::Justification::centredLeft);
        return;
    }

    if (isRack())
    {
        if (isScreenUnit)
        {
            // Black anodised rack unit holding the analyzer screen
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2d), 0.0f, r.getY(), juce::Colour (0xff18181b), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 4.0f);
            setBrushFill (g, 0.5f);
            g.fillRoundedRectangle (r, 4.0f);
            const float hw = 16.0f, hh = 8.0f;
            drawRackHole (g, { r.getX() + 6.0f, r.getY() + 9.0f, hw, hh });
            drawRackHole (g, { r.getRight() - 6.0f - hw, r.getY() + 9.0f, hw, hh });
            drawRackHole (g, { r.getX() + 6.0f, r.getBottom() - 9.0f - hh, hw, hh });
            drawRackHole (g, { r.getRight() - 6.0f - hw, r.getBottom() - 9.0f - hh, hw, hh });
        }
        else
        {
            // Charcoal faceplate
            g.setGradientFill (juce::ColourGradient (pal::panelHi, 0.0f, r.getY(), pal::panel.darker (0.2f), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.0f);
            setBrushFill (g, isOnyx() ? 0.45f : 1.0f);   // Onyx: matte finish
            g.fillRoundedRectangle (r, 3.0f);
            const float in = 9.0f;
            if (screwsLeft)  { drawScrew (g, { r.getX() + in, r.getY() + in }, 3.4f, 0.6f); drawScrew (g, { r.getX() + in, r.getBottom() - in }, 3.4f, 1.2f); }
            if (screwsRight) { drawScrew (g, { r.getRight() - in, r.getY() + in }, 3.4f, 2.1f); drawScrew (g, { r.getRight() - in, r.getBottom() - in }, 3.4f, 0.2f); }
        }

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine (1, r.getX() + 3.0f, r.getRight() - 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

        // Slanted grooves between bands / sections
        auto groove = [&g, &r] (float xTop, float xBottom)
        {
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawLine (xTop, r.getY() + 3.0f, xBottom, r.getBottom() - 3.0f, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.09f));
            g.drawLine (xTop + 1.5f, r.getY() + 3.0f, xBottom + 1.5f, r.getBottom() - 3.0f, 1.0f);
        };
        for (int x : dividers) groove ((float) x + 10.0f, (float) x - 10.0f);
        if (grooveLeft) groove (16.0f, 2.0f);

        g.setFont (font (13.0f, true));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (title, juce::Rectangle<int> (titleX, 1, 300, 38), juce::Justification::centredLeft);
        g.setColour (pal::text);
        g.drawText (title, juce::Rectangle<int> (titleX, 0, 300, 38), juce::Justification::centredLeft);
        return;
    }

    if (isHardware())
    {
        // Brushed faceplate with a bevel and four screws
        g.setGradientFill (juce::ColourGradient (pal::panelHi, 0.0f, r.getY(), pal::panel.darker (0.15f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawHorizontalLine (1, r.getX() + 3.0f, r.getRight() - 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        const float in = 9.0f;
        drawScrew (g, { r.getX() + in, r.getY() + in }, 3.4f, 0.6f);
        drawScrew (g, { r.getRight() - in, r.getY() + in }, 3.4f, 2.1f);
        drawScrew (g, { r.getX() + in, r.getBottom() - in }, 3.4f, 1.2f);
        drawScrew (g, { r.getRight() - in, r.getBottom() - in }, 3.4f, 0.2f);

        // Engraved title
        g.setFont (font (13.0f, true));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (title, juce::Rectangle<int> (titleX, 1, 240, 38), juce::Justification::centredLeft);
        g.setColour (pal::text);
        g.drawText (title, juce::Rectangle<int> (titleX, 0, 240, 38), juce::Justification::centredLeft);
        return;
    }

    if (isModern())
    {
        // Glass panel: soft top light, accent-tinted rim
        g.setGradientFill (juce::ColourGradient (pal::panelHi.withAlpha (0.92f), 0.0f, r.getY(),
                                                 pal::panel.withAlpha (0.92f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        g.setGradientFill (juce::ColourGradient (accent->withAlpha (0.45f), r.getX(), r.getY(),
                                                 pal::line.withAlpha (0.6f), r.getX() + r.getWidth() * 0.5f, r.getBottom(), false));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (1, r.getX() + rad, r.getRight() - rad);

        juce::Path bar;
        bar.startNewSubPath (r.getX() + 14.0f, r.getY() + 1.5f);
        bar.lineTo (r.getX() + 52.0f, r.getY() + 1.5f);
        drawGlowStroke (g, bar, *accent, 2.5f);
    }
    else   // Royal
    {
        g.setGradientFill (juce::ColourGradient (pal::panelHi, 0.0f, r.getY(), pal::panel.darker (0.25f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad);
        g.setGradientFill (juce::ColourGradient (pal::io.withAlpha (0.55f), r.getCentreX(), r.getY(),
                                                 pal::io.withAlpha (0.12f), r.getCentreX(), r.getBottom(), false));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        g.setColour (pal::io.withAlpha (0.14f));
        g.drawHorizontalLine (37, r.getX() + 12.0f, r.getRight() - 12.0f);

        // gold-tipped accent bar
        g.setGradientFill (juce::ColourGradient (*accent, r.getX() + 14.0f, 0.0f, pal::io, r.getX() + 52.0f, 0.0f, false));
        g.fillRoundedRectangle (juce::Rectangle<float> (r.getX() + 14.0f, r.getY(), 38.0f, 3.0f), 1.5f);
    }

    g.setColour (pal::text);
    g.setFont (font (13.0f, true));
    g.drawText (title, juce::Rectangle<int> (titleX, 0, 240, 38), juce::Justification::centredLeft);
}

//==============================================================================
VoxStripEditor::VoxStripEditor (VoxStripProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      cleanSection (p.apvts, "RESO + TAHOR EQ", id::cleanOn, pal::clean),
      eqSection   (p.apvts, "CHARACTER EQ", id::eqOn, pal::eq),
      compSection (p.apvts, "COMPRESSOR", id::compOn, pal::comp),
      dsSection   (p.apvts, "DE-ESSER",   id::dsOn,   pal::deess),
      flowSection (p.apvts, "ZEREM",   id::flowOn, pal::eq),
      tapeSection (p.apvts, "TAPE",       id::tapeOn, pal::eq),
      limSection  (p.apvts, "LIMITER",    id::limOn,  pal::limiter),
      flowDisplay (p),
      eqTypes  (p.apvts, eqTypeNames, [] (int t) { return charEqId (t, "On"); },
                [] (int t) { return eqModeColour ((EqType) t); }, "eqTab", 1,
                "Click the light to switch an EQ in or out. Click the name to edit it. Drag a tab to change the order.", MISHKAL_V74 ? id::eqOrder8 : id::eqOrder),
      compTypes (p.apvts, MISHKAL_V52 ? compTypeNames : compTypeNames3, [] (int u) { return compId (u, "On"); },
                 [] (int u) { return compModeColour ((CompType) u); }, "compTab", 0,
                 "Click the light to switch a compressor in or out. Click the name to edit it. Drag a tab to change the order.",
                 MISHKAL_V75 ? id::compOrder9 : MISHKAL_V52 ? id::compOrder4 : id::compOrder),
      limMode  (*p.apvts.getParameter (id::limMode),  limModeNames,  pal::limiter),
      dsMode   (*p.apvts.getParameter (id::dsMode),   dsModeNames,   pal::deess),
      eqPosition (*p.apvts.getParameter (id::eqAfterComp), juce::StringArray { "Before Comp", "After Comp" }, pal::eq),
      tapeStyleSel (*p.apvts.getParameter (id::tapeStyle), tapeStyleNames, pal::eq),
      tapeWidthSel (*p.apvts.getParameter (id::tapeWidth), tapeWidthNames, pal::eq),
      scHpfSel (*p.apvts.getParameter (id::compScHpf), juce::StringArray { "SC off", "80", "120", "160" }, pal::comp),
      limSoftSel (*p.apvts.getParameter (id::limSoft), juce::StringArray { "Soft off", "2 dB", "3 dB", "4 dB", "6 dB" }, pal::limiter),
      tapeOsSel (*p.apvts.getParameter (id::tapeOs), juce::StringArray { "Off", "2x", "4x" }, pal::eq),
      limOsSel  (*p.apvts.getParameter (id::limOs),  juce::StringArray { "Off", "2x", "4x" }, pal::limiter),
      fetRevSel (*p.apvts.getParameter (compId (0, "Rev")), juce::StringArray { "Purple", "Blue Stripe", "Black" }, pal::limiter),
      flowModelSel (*p.apvts.getParameter (id::flowModel), juce::StringArray { "Zerem", "Perach" }, pal::limiter),
      tapeModelSel (*p.apvts.getParameter (id::tapeModel), juce::StringArray { MISHKAL_V40 ? "Slil" : "Tape", "Esh" }, pal::limiter),
      eshCharSel (*p.apvts.getParameter (id::eshChar), juce::StringArray { "Clear", "Warm", "Round", "Glow", "Ember" }, pal::limiter),
      eshToneSel (*p.apvts.getParameter (id::eshTone), juce::StringArray { "warm", "flat", "bright" }, pal::limiter),
      rcModelSel (*p.apvts.getParameter (compId (1, "Model")), MISHKAL_V32 ? juce::StringArray { "LE-RC", "LE-RV" } : juce::StringArray { "RComp", "RVox" }, pal::limiter),
      tubeModelSel (*p.apvts.getParameter (compId (2, "Model")), juce::StringArray { "LE-660", "LE-STA" }, pal::limiter),
      staModeSel (*p.apvts.getParameter (compId (MISHKAL_V52 ? 3 : 2, "StaMode")), juce::StringArray { "Single", "Double", "Triple" }, pal::limiter),
      staRelSel (*p.apvts.getParameter (compId (MISHKAL_V52 ? 3 : 2, "StaRel")), juce::StringArray { "Fast", "Medium", "Slow", "Very Slow" }, pal::limiter),
      tahorPhaseSel (*p.apvts.getParameter (id::cleanPhase), juce::StringArray { "Natural", "Linear", "Linear HQ" }, pal::clean),
      tahorOsSel    (*p.apvts.getParameter (id::cleanOs),    juce::StringArray { "Off", "2x", "4x" }, pal::clean),
      rcBehavior    (*p.apvts.getParameter (compId (1, "Behavior")),  juce::StringArray { "Glide", "Snap" },  pal::comp),
      rcCharacter   (*p.apvts.getParameter (compId (1, "Character")), juce::StringArray { "Color", "Pure" },  pal::comp),
      cleanGraph (p), bandPanel (p.apvts), resoPanel (p.apvts),
      compMeter (p.meters().compGr, pal::comp),
      dsMeter   (p.meters().dsGr,   pal::deess),
      flowMeter (p.meters().flowGr, pal::eq),
      limMeter  (p.meters().limGr,  pal::limiter),
      inStrip (p.meters().inPeak), outStrip (p.meters().outPeak),
      tapeVu (p.meters().tapeVuDb),
      limScope (p),
      meterBridge (p),
      p1LowFreqSel (*p.apvts.getParameter (charEqId (0, "LowFreq")), juce::StringArray { "20", "30", "60", "100" }, pal::eq),
      gateLed (p.meters().gateGr), breathLed (p.meters().breathGr),
      listenAttachment (p.apvts, id::dsListen, listen),
      dsAutoAttachment (p.apvts, id::dsAuto, dsAuto),
      arcAttachment (p.apvts, id::limArc, arc)
{
    if (MISHKAL_V56)   // V56: Goliath only - the old skins are gone (a session saved with another skin opens in Goliath)
        applySkin (SkinStyle::Goliath);
    else
    applySkin ((SkinStyle) juce::jlimit (0, skinNames.size() - 1, (int) p.apvts.state.getProperty ("skin", (int) SkinStyle::Goliath)));
    lnf.applyColours();
    setLookAndFeel (&lnf);

    // Top bar
    for (auto* c : std::initializer_list<juce::Component*> { &undoBtn, &redoBtn, &prevBtn, &nextBtn, &presetBox, &saveBtn, &saveAsBtn, &abSwitch, &menuBtn, &trackBtn, &matchBtn, &bypassBtn })
        content.addAndMakeVisible (c);
    bypassBtn.setClickingTogglesState (true);
    bypassBtn.setTooltip ("Bypass the whole plugin (smooth, stays in time). Your DAW's bypass button does the same.");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::masterBypass, bypassBtn);
    trackBtn.setClickingTogglesState (true);
    trackBtn.getProperties().set ("lampColour", (juce::int64) 2);   // green: ready to record
    trackBtn.setTooltip ("Tracking mode: near-zero latency so singers can record while hearing the full chain. "
                         "The resonance suppressor pauses (it needs 21 ms to listen) and the look-ahead stages "
                         "switch to instant versions. Switch it off again for mixing. Best changed while stopped.");
    trackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::trackMode, trackBtn);
    content.addChildComponent (osBtn);
    osBtn.getProperties().set ("lampColour", (juce::int64) 1);
    osBtn.setClickingTogglesState (true);   // so it draws a lamp; the timer keeps the lamp in step with the setting
    osBtn.setTooltip ("Oversampling for the whole plugin: every stage runs at 2x or 4x the session rate, so saturation, "
                      "EQ boosts near the top and limiting stay clean (less aliasing). Costs CPU (about 2x / 4x) and adds a little "
                      "latency, which your DAW compensates for. Switching drops the audio for a moment - best done while stopped. "
                      "TRACK mode turns it off.");
    osBtn.onClick = [this]
    {
        auto* prm = proc.apvts.getParameter (id::globalOs);
        const int next = (juce::roundToInt (proc.apvts.getRawParameterValue (id::globalOs)->load()) + 1) % 3;
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) next));
        prm->endChangeGesture();
    };
    matchBtn.setClickingTogglesState (true);
    matchBtn.getProperties().set ("lampColour", (juce::int64) 1);   // yellow lamp
    matchBtn.setTooltip ("Level match: when you hit BYPASS, the raw signal plays at the same loudness as the processed one, "
                         "so you hear only what the plugin changes. Your processed output is never changed.");
    matchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::levelMatch, matchBtn);
    undoBtn.setTooltip ("Undo");  redoBtn.setTooltip ("Redo");
    undoBtn.onClick = [this] { proc.undoManager.undo(); refreshToolbar(); };
    redoBtn.onClick = [this] { proc.undoManager.redo(); refreshToolbar(); };
    auto stepPreset = [this] (int d)   // V67: inside the current folder when that's switched on (preset menu)
    {
        proc.undoManager.beginNewTransaction();
        if (MISHKAL_V67 && (bool) proc.apvts.state.getProperty ("stepFolder", false)) proc.presets.stepInFolder (d);
        else proc.presets.step (d);
        refreshToolbar();
    };
    prevBtn.onClick = [stepPreset] { stepPreset (-1); };
    nextBtn.onClick = [stepPreset] { stepPreset (1); };
   #if MISHKAL_V67
    presetBox.onStar = [this] { proc.presets.toggleFavourite (proc.presets.getCurrentName()); refreshToolbar(); };
   #endif
    presetBox.onClick = [this] { showPresetMenu(); };
    saveBtn.onClick = [this] { savePreset (false); };
    saveAsBtn.onClick = [this] { savePreset (true); };
    abSwitch.onSelect = [this] (int slot) { proc.undoManager.beginNewTransaction(); proc.presets.selectSlot (slot); refreshToolbar(); };
    abSwitch.onCopy = [this] { proc.presets.copyActiveToOther(); };
    abSwitch.setTooltip ("Compare two settings. Click > to copy the active one to the other slot.");
    menuBtn.onClick = [this] { showMainMenu(); };
    addMouseListener (this, true);   // every click starts a fresh undo step

    for (auto* s : { &cleanSection, &eqSection, &compSection, &dsSection, &flowSection, &tapeSection, &limSection })
        content.addAndMakeVisible (s);

    // Zerem (styled 5-band stage)
    flowSection.addAndMakeVisible (flowDisplay);
    flowSection.addAndMakeVisible (flowMeter);
    fAmount = &add (flowSection, id::flowAmount, "Amount", pal::eq);
    fMix    = &add (flowSection, id::flowMix,    "Mix",    pal::eq);
    {
        const char* ids[] { id::perAmount, id::perLow, id::perBody, id::perPresence, id::perAir, id::perSpeed, id::perMix };
        const char* names[] { "amount", "low", "body", "presence", "air", "speed", "mix" };
        const char* tips[] {
            "Perach amount: how strongly it steers the tone toward balance. Above 7 it also squashes each region against its own level (denser, more finished).",
            "Low (around 110 Hz): ask for more or less weight. Perach only adds it when the source needs it.",
            "Body (around 450 Hz): more or less warmth / fullness.",
            "Presence (around 2.8 kHz): more or less forwardness.",
            "Air (around 10 kHz): more or less top and sheen.",
            "Speed: how quickly it follows the sound - slow for smooth, fast to follow every word.",
            "Mix: how much of Perach's correction is applied." };
        for (int i = 0; i < 7; ++i)
        {
            perKnobs[(size_t) i] = &add (flowSection, ids[i], names[i], pal::eq, i >= 1 && i <= 4);
            perKnobs[(size_t) i]->slider.setTooltip (tips[i]);
            perKnobs[(size_t) i]->setVisible (false);
        }
    }

    // Tape: style in the header, VU | Drive | Mix
    tapeSection.addAndMakeVisible (tapeStyleSel);
    tapeSection.addAndMakeVisible (tapeVu);
    tapeSection.addAndMakeVisible (tapeWidthSel);
    tapeWidthSel.setTooltip ("Tape width.  1/4\": saturates soonest, gritty and vintage.  "
                             "1/2\": the mastering format, cleanest, wide low end.  1\": balanced.  "
                             "2\": the big multitrack sound, punchiest, fullest low end.");
    tapeStyleSel.setTooltip ("Tape speed.  30 ips: best for vocals - smooth highs, gentle glue.  "
                             "15 ips: best for drums - rounded transients, punch and weight.  "
                             "7.5 ips: best for guitars - warm harmonics, smooth top.");
    tapeSection.addAndMakeVisible (tapeNoiseBtn);
    tapeNoiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::tapeNoise, tapeNoiseBtn);
    tapeNoiseBtn.setTooltip ("Tape noise: adds the gentle hiss of real tape. Like the real thing, slower speeds and narrower tape "
                             "hiss more (about -72 dB at 30 ips on 1/2\", about -57 dB at 7.5 ips on 1/4\"). "
                             "It plays even in silence, so switch it off if you print stems.");
    tDrive = &add (tapeSection, id::tapeDrive, "Drive", pal::eq);
    tMix   = &add (tapeSection, id::tapeMix,   "Mix",   pal::eq);
    {
        const char* ids[] { id::eshIn, id::eshProcess, id::eshOut };
        const char* names[] { "input", "process", "output" };
        const char* tips[] { "Esh input: how hot the signal hits the tape (-10 to +10 dB).",
                             "Process: how hard the chosen character works - from a gentle fullness to dense, loud saturation.",
                             "Esh output: level trim after the tape (-6 to +6 dB), for fair comparisons." };
        for (int i = 0; i < 3; ++i)
        {
            eshKnobs[(size_t) i] = &add (tapeSection, ids[i], names[i], pal::eq, i != 1);
            eshKnobs[(size_t) i]->slider.setTooltip (tips[i]);
            eshKnobs[(size_t) i]->setVisible (false);
        }
    }
   #if MISHKAL_V60
    tMsDrive = &add (tapeSection, id::tapeMsDrive, "m/s drive", pal::eq, true);
    tMsDrive->slider.setTooltip ("M/S drive: turn left to drive the centre (vocal, kick, bass) harder, right to drive the sides "
                                 "(reverbs, wide guitars, room) harder for width and excitement. The middle is plain stereo tape. "
                                 "The level and width stay matched, and the tape's low-end bump stays on the centre so the bass stays mono. "
                                 "Pushing the sides hard can thin the mix in mono - check it. Double-click to reset.");
    tMsDrive->setVisible (false);
   #endif
    tDrive->slider.setTooltip ("How hard the tape is hit. 50% is the recommended setting for the chosen speed.");
    tMix->slider.setTooltip ("Blends the tape saturation with the clean signal. The tape's tone stays.");
    fAmount->slider.setTooltip ("How much of the style: 50% is the style as designed");

    inputKnob  = &add (content, id::inputGain,  "Input",  pal::io, true);
    outputKnob = &add (content, id::outputGain, "Output", pal::io, true);
    content.addAndMakeVisible (inStrip);
    content.addAndMakeVisible (outStrip);
    inStrip.setKnob (*inputKnob);     // the knob sits on top of its meter
    peakBtn.setClickingTogglesState (true);
    peakBtn.getProperties().set ("lampColour", (juce::int64) 1);
    peakBtn.setTooltip ("Peak catch: an ultra-fast limiter right after the Input knob (Spectra 610 style). "
                        "It grabs only the sharpest peaks above -6 dB before the rest of the chain sees them. "
                        "Drive the Input harder to catch more. The lamp flashes red when it grabs a peak.");
    peakAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::peakCatch, peakBtn);
    pkInKnob  = &add (cleanSection, id::peakIn,  "PK In",  pal::io, true);
    pkOutKnob = &add (cleanSection, id::peakOut, "PK Out", pal::io, true);
    pkInKnob->slider.setTooltip ("1964 PK input: drives the signal into the peak catcher's -6 dB catch point. "
                                 "With Link on, the output comes down by the same amount so the level stays put.");
    pkOutKnob->slider.setTooltip ("1964 PK output: trims the level after the peak catcher. With Link on it works on top of the automatic make-down.");
    peakLinkBtn.setClickingTogglesState (true);
    peakLinkBtn.setTooltip ("Link: pushing PK In turns the output down by the same amount automatically. Off: In and Out are independent.");
    peakLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::peakLink, peakLinkBtn);
    pkPowerBtn.setClickingTogglesState (true);
    pkPowerBtn.getProperties().set ("offText", "OFF");
    pkPowerBtn.setTooltip (peakBtn.getTooltip());
    pkPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::peakCatch, pkPowerBtn);
    for (auto* c : { (juce::Component*) pkInKnob, (juce::Component*) pkOutKnob, (juce::Component*) &peakLinkBtn, (juce::Component*) &pkPanel, (juce::Component*) &pkPowerBtn, (juce::Component*) &pkLinkLabel })
    {
        cleanSection.addChildComponent (*c);
        c->setVisible (false);
    }
    inStrip.setButton (peakBtn);
    outStrip.setKnob (*outputKnob);
    outStrip.setLoudness (p.loudness);
    inStrip.setTooltip ("Input level (after the Input knob). Click the meter to reset the peak and clip lights.");
    outStrip.setTooltip ("Output level (after the Output knob). Click the meter to reset the peak and clip lights. "
                         "LUFS: S = short-term (last 3 s), I = integrated (whole song since reset, what streaming services measure). "
                         "Click the LUFS box to reset the integrated reading.");

    // Clean-up EQ
    cleanSection.addAndMakeVisible (cleanGraph);
    cleanSection.addAndMakeVisible (bandPanel);
    bandPanel.setSoloTarget (&p.getChain().soloBand);
    cleanSection.addChildComponent (resoPanel);
    cleanSection.addAndMakeVisible (rightTabs);
    for (auto* c : { (juce::Component*) &rcArc, (juce::Component*) &rcBehavior, (juce::Component*) &rcCharacter })
        compSection.addChildComponent (c);
    rcArcAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, compId (1, "Arc"), rcArc);
    rcArc.setTooltip ("AUTO REL (auto release): the release stretches out under sustained compression and stays quick on short peaks, "
                      "so it never pumps. The Release knob sets where it starts.");
    rcBehavior.setTooltip ("Glide: the release slows down as the gain reduction gets small - smooth and musical.  "
                           "Snap: the release speeds up near the threshold - more transparent and louder.");
    rcCharacter.setTooltip ("Color: gentle harmonics that grow as it compresses.  Pure: clean, no added colour.");
    cleanSection.addAndMakeVisible (tahorPhaseSel);
    cleanSection.addAndMakeVisible (tahorOsSel);
    cleanSection.addAndMakeVisible (tahorOsLabel);
    tahorOsLabel.setFont (font (10.0f, true));
    tahorOsLabel.setJustificationType (juce::Justification::centredRight);
    tahorOsLabel.setBorderSize ({});
    tahorOsLabel.setInterceptsMouseClicks (false, false);
    tahorPhaseSel.setTooltip ("Phase mode.  Natural: zero latency, behaves like analog EQ (a little phase shift around each band).  "
                              "Linear: no phase shift at all, so lows and highs stay perfectly in time - great on drums, bass, "
                              "buses and mastering. Adds about 48 ms of latency.  Linear HQ: finer resolution for steep low cuts "
                              "and narrow low bands, about 176 ms of latency (mixing only).  Linear modes can add a little "
                              "pre-ring on very sharp, heavy boosts.  Best changed while playback is stopped; TRACK uses Natural.");
    tahorOsSel.setTooltip ("Oversampling for Natural mode: keeps bells and shelves near 20 kHz their true shape (no 'cramping'). "
                           "Off = zero latency. Linear modes don't need it: they're built from the exact analog shapes.");
    rightTabs.onChange = [this] (int i) { showRightTab (i); };
    cleanGraph.onSelect = [this] (int band)
    {
        bandPanel.showBand (band);
        if (band >= 0) rightTabs.setIndex (0);   // picking a node shows its controls
    };

    // Character EQ
    eqSection.addAndMakeVisible (eqTypes);
    eqSection.addAndMakeVisible (eqPosition);
    eqPosition.setTooltip ("Run the character EQs before or after the compressors");
    {
        const int t = eqTypes.getSelected();
        hpf   = &add (eqSection, charEqId (t, "HpfFreq"),  "Freq", pal::eqLow);
        bodyF = &add (eqSection, charEqId (t, "BodyFreq"), "Freq", pal::eqLow);
        bodyG = &add (eqSection, charEqId (t, "BodyGain"), "Gain", pal::eqLow, true);
        mudF  = &add (eqSection, charEqId (t, "MudFreq"),  "Freq", pal::eq);
        mudG  = &add (eqSection, charEqId (t, "MudGain"),  "Gain", pal::eq, true);
        mudQ  = &add (eqSection, charEqId (t, "MudQ"),     "Q",    pal::eq);
        presF = &add (eqSection, charEqId (t, "PresFreq"), "Freq", pal::eq);
        presG = &add (eqSection, charEqId (t, "PresGain"), "Gain", pal::eq, true);
        presQ = &add (eqSection, charEqId (t, "PresQ"),    "Q",    pal::eq);
        airF  = &add (eqSection, charEqId (t, "AirFreq"),  "Freq", pal::eqHigh);
        airG  = &add (eqSection, charEqId (t, "AirGain"),  "Gain", pal::eqHigh, true);
        silk  = &add (eqSection, charEqId ((int) EqType::Airy, "Silk"), "Silk", pal::eqHigh);
    }
    eqTypes.onSelect = [this] (int t) { showCharEq (t); };
    compMeter.colourFor = [this] { return compModeColour ((CompType) compTypes.getSelected()); };
    flowMeter.colourFor = [] { return isOnyx() || isRack() || isGoliath() ? pal::eq : pal::clean; };   // matches the Vox Flow LCD
    silk->slider.setTooltip ("LE-SP1 only: adds soft, tube-like harmonics to the air band");

    for (auto name : { "HIGH-PASS", "BODY", "MUD", "PRESENCE", "AIR" })
    {
        auto* l = eqColumnLabels.add (new juce::Label ({}, name));
        l->setFont (font (11.0f, true));
        l->setColour (juce::Label::textColourId, pal::eq);
        l->setJustificationType (juce::Justification::centred);
        eqSection.addAndMakeVisible (l);
    }

    // Compressor
    compSection.addAndMakeVisible (compTypes);
    compTypes.onSelect = [this] (int u) { showComp (u); };
    compSection.addAndMakeVisible (compMeter);
    cGate    = &add (compSection, id::gateThresh,  "Gate",      pal::comp);
    compSection.addAndMakeVisible (gateLed);
    cBreath  = &add (compSection, id::breathAmount, "Breath",   pal::comp);
    cRider   = &add (compSection, id::riderRange,   "Rider",    pal::comp);
    compSection.addAndMakeVisible (breathLed);
    compSection.addAndMakeVisible (scHpfSel);
    cBreath->slider.setTooltip ("Breath control: turns breaths down by this much, instead of gating them out. "
                                "The light glows while a breath is being turned down.");
    cRider->slider.setTooltip ("Rider: an automatic fader that evens out loud and quiet passages before the compressors (vocals, bass, acoustic instruments), "
                               "up to this far either way. The bar in the slot shows it riding up or down. It holds still in silences.");
    scHpfSel.setTooltip ("Sidechain high-pass for the compressors: the detector ignores the low end, so plosives and "
                         "proximity boom don't cause extra compression. 80 Hz is subtle, 160 Hz is strong.");
    // knobs start bound to FET Purple (the only unit with every control); showComp re-binds them
    cThresh  = &add (compSection, compId (0, "Thresh"),  "Threshold", pal::comp);
    cRatio   = &add (compSection, compId (0, "Ratio"),   "Ratio",     pal::comp);
    cAttack  = &add (compSection, compId (0, "Attack"),  "Attack",    pal::comp);
    cRelease = &add (compSection, compId (0, "Release"), "Release",   pal::comp);
    cTc      = &add (compSection, compId (2, "Tc"),      "Time Const", pal::comp);
    cMakeup  = &add (compSection, compId (0, "Makeup"),  "Make-up",   pal::comp, false);
   #if MISHKAL_V75
    {
        struct SwDef { int unit; const char* field; const char* label; const char* tip; };
        static const SwDef defs[] {
            { 4, "Knee",    "KNEE",      "Knee: Hard bites exactly at the threshold; Med and Soft ease in, smoother and more transparent." },
            { 4, "Thrust",  "THRUST",    "Thrust: the detector hears less low end (Med) or tilts toward the highs (Loud), so the kick and bass punch "
                                         "through instead of pumping the whole mix. Loud is the classic API bus trick." },
            { 4, "Type",    "TYPE",      "New: feed-forward - tight and modern. Old: feedback - it listens to its own output, softer and more vintage." },
            { 6, "Knee",    "KNEE",      "Hard: precise. Soft: eases in over about 10 dB - the transparent mastering setting." },
            { 6, "Detect",  "DETECT",    "Peak: catches every transient. RMS: follows the loudness, smoother and more musical on full mixes." },
            { 6, "RelMode", "RELEASE",   "Single: one release time. Average: recovers fast from short peaks and slowly under sustained level - "
                                         "the trick that keeps a master moving without pumping." },
            { 7, "Bias",    "TUBE BIAS", "Tube Bias: Low is gentle (about 1.5:1) and clean; Max is about 4:1 with thick tube harmonics." },
            { 7, "Rect",    "RECTIFIER", "Silicon: fast and firm. Germanium: softer knee, a little slower. LED: the softest, slowest, smoothest." },
            { 7, "Airbass", "AIRBASS",   "Airbass: keeps the lows and the highs out of the detector, so the compression rides the mids and the "
                                         "master keeps its weight and air." },
            { 8, "LimRel",  "LIM REC",   "Limiter recovery: 50-800 ms, or A1 / A2 auto (fast then slow)." },
            { 8, "LimAtt",  "LIM ATT",   "Limiter attack: Fast (2 ms) catches more; Slow (4 ms) lets a little more punch through." } };
        for (auto& d : defs)
            if (auto* pp = p.apvts.getParameter (compId (d.unit, d.field)))
            {
                auto* ch = dynamic_cast<juce::AudioParameterChoice*> (pp);
                auto sel = std::make_unique<vox::ui::ModeSelector> (*pp, ch != nullptr ? ch->choices : juce::StringArray { "Off", "On" }, pal::comp);
                sel->stepperLabel = d.label;
               #if MISHKAL_V76 && ! MISHKAL_V77
                if (d.unit == 4) { sel->hwButtons = 1; sel->hwCaption = d.label; sel->hwLegend = juce::Colour (0xff15171a); }
               #endif
                sel->setTooltip (d.tip);
                compSection.addChildComponent (*sel);
                mcompSw.push_back ({ d.unit, std::move (sel) });
            }
    }
   #endif
    cMix     = &add (compSection, compId (0, "Mix"),     "Mix",       pal::comp);
    voxLinkBtn.setClickingTogglesState (true);
    voxLinkBtn.getProperties().set ("slideOnly", true);
    voxLinkBtn.setTooltip ("Link: as you pull the Threshold down, the Output comes up automatically so the level stays up (the classic leveler). "
                           "Off: the Threshold only compresses, and the Output is yours to set.");
    voxLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, compId (1, "VoxLink"), voxLinkBtn);
    compSection.addChildComponent (voxLinkBtn);
    compSection.addChildComponent (voxLinkLabel);
    for (auto* k : { cGate, cBreath, cRider, cThresh, cRatio, cAttack, cRelease, cTc, cMakeup })
        k->setFader (true);   // compressor controls are long sliders; Mix stays a knob
    compInfo.setFont (font (11.5f));
    compInfo.setColour (juce::Label::textColourId, pal::dim);
    for (auto* lb : { &tapeOsLabel, &limOsLabel, &tahorOsLabel }) lb->setColour (juce::Label::textColourId, pal::dim);
    compInfo.setJustificationType (juce::Justification::topLeft);
    compInfo.setMinimumHorizontalScale (1.0f);
    compSection.addAndMakeVisible (compInfo);

    // De-esser
    dsSection.addAndMakeVisible (dsMeter);
    dFreq   = &add (dsSection, id::dsFreq,   "Freq",      pal::deess);
    dThresh = &add (dsSection, id::dsThresh, "Threshold", pal::deess);
    dRange  = &add (dsSection, id::dsRange,  "Range",     pal::deess);
    dFreq->setFader (true);   // Freq is a slider; Threshold and Range are knobs
    tapeNoiseBtn.setColour (juce::ToggleButton::tickColourId, pal::eq);
    rcArc.setColour (juce::ToggleButton::tickColourId, pal::eq);
    listen.setColour (juce::ToggleButton::tickColourId, pal::deess);
    listen.setTooltip ("Hear only the sibilance band");
    dsSection.addAndMakeVisible (listen);
    dsSection.addAndMakeVisible (dsMode);
    dsMode.setTooltip ("Split: only the harsh band is turned down. Wide: the whole signal dips briefly on each hit (esses, cymbals, harsh peaks).");
    dsAuto.setTooltip ("Threshold-less: follows the signal's own level. Threshold becomes sensitivity.");
    dsSection.addAndMakeVisible (dsAuto);
   #if MISHKAL_V64
    dsKeyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::dsScExt, dsKeyBtn);
    dsKeyBtn.setColour (juce::ToggleButton::tickColourId, pal::deess);
    dsKeyBtn.setTooltip ("External key: the de-esser listens to the track routed into the plugin's sidechain input instead of this one "
                         "(e.g. de-ess a reverb or a doubles bus from the lead vocal). Set the sidechain in your DAW first.");
    dsSection.addAndMakeVisible (dsKeyBtn);
    {
        static const juce::Colour keyAccent (0xffffb83d);
        compKeySel = std::make_unique<vox::ui::ModeSelector> (*p.apvts.getParameter (id::compScExt), juce::StringArray { "int", "ext" }, keyAccent);
        compKeySel->pills = true;
        compKeySel->pillCaption = "key";
        compKeySel->setTooltip ("Key: what the compressors listen to. int = this track. ext = the track routed into the plugin's sidechain "
                                "input in your DAW (duck a beat under the vocal, a bass under the kick). The sidechain high-pass works on either.");
        compSection.addChildComponent (*compKeySel);
    }
   #endif
   #if MISHKAL_V65
    compDeltaAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::compDelta, compDeltaBtn);
    limDeltaAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::limDelta, limDeltaBtn);
    compDeltaBtn.setColour (juce::ToggleButton::tickColourId, pal::eq);
    limDeltaBtn.setColour (juce::ToggleButton::tickColourId, pal::limiter);
    compDeltaBtn.setTooltip ("Delta: hear only what the compressors change (the level is matched, so you hear the squeeze, not the volume). "
                             "Switch it off again before you bounce.");
    limDeltaBtn.setTooltip ("Delta: hear only what the limiter takes off. If you can hear the music clearly in here, it's working too hard. "
                            "Switch it off again before you bounce.");
    compSection.addChildComponent (compDeltaBtn);
    limSection.addChildComponent (limDeltaBtn);
   #endif

    // Limiter
    limSection.addAndMakeVisible (limMeter);
    limSection.addAndMakeVisible (limMode);
    limSection.addAndMakeVisible (limSoftSel);
    const juce::String osTip = "Oversampling: runs this stage at 2x or 4x the sample rate so pushed saturation doesn't alias "
                               "(no grainy, metallic fizz on bright sources). Costs more CPU and adds a little latency, which your "
                               "DAW compensates for. Off = no oversampling. Best changed while playback is stopped. TRACK mode always turns it off.";
    limSection.addAndMakeVisible (limOsSel);
    limSection.addAndMakeVisible (limOsLabel);
    limOsSel.setTooltip (osTip + " Here it oversamples the soft saturation (the limiter itself is smooth enough not to need it), "
                                 "so it matters most with Soft at 4 or 6 dB.");
    tapeSection.addAndMakeVisible (tapeOsSel);
    tapeSection.addAndMakeVisible (tapeOsLabel);
    tapeOsSel.setTooltip (osTip);
    for (auto* lb : { &tapeOsLabel, &limOsLabel })
    {
        lb->setFont (font (10.0f, true));
        lb->setJustificationType (juce::Justification::centredRight);
        lb->setBorderSize ({});
        lb->setInterceptsMouseClicks (false, false);
    }
    limSoftSel.setTooltip ("Soft saturation, in the style of Lavry AD converters: rounds peaks off instantly on a smooth curve "
                           "(no attack or release), leaves everything below the threshold untouched, then adds that much gain. "
                           "3 dB starts at -6 dBFS; 6 dB starts at -12 dBFS. The brickwall limiter catches anything left.");
    limSoftKnob = &add (limSection, id::limSoft, "Soft", pal::limiter);
    limSoftKnob->slider.setTooltip (limSoftSel.getTooltip());
    lThresh  = &add (limSection, id::limThresh,  "Threshold", pal::limiter);
    lCeiling = &add (limSection, id::limCeiling, "Ceiling", pal::limiter);
    lRelease = &add (limSection, id::limRelease, "Release", pal::limiter);
    lMix     = &add (limSection, id::limMix,     "Mix",     pal::limiter);
   #if MISHKAL_V66
    autoInput = std::make_unique<vox::ui::AutoInputButton> (p);
    cleanSection.addChildComponent (*autoInput);
   #endif
   #if MISHKAL_V63
    lWidth = &add (limSection, id::outWidth, "width", pal::limiter);
    lWidth->slider.setTooltip ("Stereo width of the whole strip (before the limiter). 100% = as it is, 0% = mono, up to 200% = wider. "
                               "Widening leaves everything below about 120 Hz in the middle, so the bass stays solid. "
                               "Watch the phase meter: if it dips below 0, back off.");
    lWidth->setVisible (false);
    phaseMeter = std::make_unique<vox::ui::PhaseMeter> (p.getChain().meters.correlation);
    limSection.addChildComponent (*phaseMeter);
   #endif
   #if MISHKAL_V61
    lLink    = &add (limSection, id::limLink,    "link",    pal::limiter);
    lLink->slider.setTooltip ("Channel link. 100%: both sides always get the same gain, so the stereo image stays put. "
                              "Lower: a peak on one side (a hard-panned snare, say) is caught mostly on that side, so the other "
                              "side isn't pulled down with it - often louder and more open on masters. Try 50-80%.");
    lLink->setVisible (false);
   #endif
   #if MISHKAL_V69
    limEngine = std::make_unique<vox::ui::LimEngineBox> (p.apvts);
    limEngine->onChange = [this] { resized(); };
    limSection.addChildComponent (*limEngine);
    limStyleSel = std::make_unique<vox::ui::ModeSelector> (*p.apvts.getParameter (id::limMaxStyle), limMaxStyleNames, pal::limiter);
    limStyleSel->dropdownLabel = MISHKAL_V71 ? "XL STYLE" : "MAX STYLE";
   #if MISHKAL_V71
    limEngine->setTooltip ("Limiter engine. Goliath Classic: one clean brickwall. Goliath Multi: splits the lows from the rest so the bass "
                           "doesn't pull the whole mix down (L3-style). Goliath XL: a loudness maximizer with its own styles (pick one under "
                           "XL STYLE), a Character knob for how fast it lets go, and a Transients knob to keep the hits punchy. "
                           "The knobs take each engine's colour.");
   #endif
    limStyleSel->setTooltip (juce::String (MISHKAL_V71 ? "Goliath XL styles" : "Max styles") + ". Smart: adapts to the music, safe on anything. Transient: keeps drums snappy. "
                             "Loud - Pumping: slow and breathing, for EDM. Loud - Balanced: the all-rounder for loud masters. "
                             "Loud - Crisp: fast, bright, a little gritty. Loud - Clip: shaves the tops first for the most level. "
                             "Spectral: also evens out harsh or boomy frequencies as it pushes. "
                             "Bands: four bands, each with its own gain, so a kick only pulls down the lows and the vocal and pads stay steady."
                             + juce::String (MISHKAL_VERSION >= 72 ? " Safe: the gentlest - slow, smooth recovery that only catches the peaks and keeps the dynamics; for vocals, acoustic and anything delicate." : ""));
    limSection.addChildComponent (*limStyleSel);
    lCharacter = &add (limSection, id::limCharacter, "character", pal::limiter);
    lCharacter->slider.setTooltip (juce::String (MISHKAL_V71 ? "Character (Goliath XL)" : "Character (Max engine)") + ": how fast the limiter lets go. Low = fast and loud, with more grit; "
                                   "high = slow and smooth, a little quieter. 5 is the middle. It replaces Release in " + juce::String (MISHKAL_V71 ? "Goliath XL." : "Max."));
    lCharacter->setVisible (false);
    lTransients = &add (limSection, id::limTransients, "transients", pal::limiter);
    lTransients->slider.setTooltip (juce::String (MISHKAL_V71 ? "Transients (Goliath XL)" : "Transients (Max engine)") + ": lifts the front of each hit before it is limited, so kicks and snares "
                                    "stay punchy when you push hard. It costs a little loudness. Try 20-40% on drum-heavy mixes.");
    lTransients->setVisible (false);
   #endif
   #if MISHKAL_V70
    lUpward = &add (limSection, id::limUpward, "upward", pal::limiter);
    lUpward->slider.setTooltip ("Upward compression: lifts the quiet parts (verses, breaths, reverb tails, fades) up toward the loud ones "
                                "before the limiter, so the track feels denser and louder without squashing the peaks harder. "
                                "The loudest 6 dB is left alone and silence or hiss is never lifted. Try 2-4 dB on masters; "
                                "much more flattens the dynamics. Works in every limiter engine.");
    lUpward->setVisible (false);
   #endif
    arc.setColour (juce::ToggleButton::tickColourId, pal::limiter);
    arc.setTooltip ("AUTO REL (auto release): the limiter's release adapts to the music - quick on short peaks, slower under sustained limiting. Turns the Release knob off.");
    limSection.addAndMakeVisible (arc);

    // Goliath: module names, two-way switches and display tabs
    cleanSection.getProperties().set ("goliathTitle", juce::String (juce::CharPointer_UTF8 ("TAHOR EQ \xc2\xb7 RESO")));
    for (auto* vt : { &tahorViewTab, &resoViewTab }) cleanSection.addChildComponent (vt);
    tahorViewTab.setTooltip ("Show the Tahor EQ band controls");
    resoViewTab.setTooltip ("Show the resonance suppressor controls");
    tahorViewTab.onClick = [this] { rightTabs.setIndex (0); showRightTab (0); };
    resoViewTab.onClick  = [this] { rightTabs.setIndex (1); showRightTab (1); };
    tahorViewTab.isSelected = [this] { return bandPanel.isVisible(); };
    resoViewTab.isSelected  = [this] { return resoPanel.isVisible(); };
    resoPowerBtn.getProperties().set ("offText", "OFF");
    resoPowerBtn.setTooltip ("Resonance suppressor on/off");
    resoPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::resoOn, resoPowerBtn);
    cleanSection.addChildComponent (resoPowerBtn);
    eqSection.getProperties().set ("goliathTitle", "CHARACTER EQS");
    compSection.getProperties().set ("goliathTitle", "DYNAMICS");
    dsSection.getProperties().set ("goliathTitle", "DE-ESS");
    flowSection.getProperties().set ("goliathTitle", MISHKAL_V41 ? "TONE" : "ZEREM");
    tapeSection.getProperties().set ("goliathTitle", "TAPE");
    limSection.getProperties().set ("goliathTitle", juce::String (juce::CharPointer_UTF8 ("LIMITER \xc2\xb7 OUTPUT")));
   #if MISHKAL_V80
    // V80: the modules that will become their own plugins get their names (Hebrew), with what they are under each
    compSection.getProperties().set ("goliathTitle", "LACHATZ");   compSection.getProperties().set ("goliathSub", "dynamics");
    dsSection.getProperties().set ("goliathTitle", "LACHASH");     dsSection.getProperties().set ("goliathSub", "de-esser");
    limSection.getProperties().set ("goliathTitle", "CHOMAH");     limSection.getProperties().set ("goliathSub", juce::String (juce::CharPointer_UTF8 ("limiter \xc2\xb7 output")));
   #endif
    eqPosition.batLabels = { "Pre", "Post" };
    p1LowFreqSel.vertical = true;
    p1LowFreqSel.setTooltip ("LE-EP1 low frequency: 20, 30, 60 or 100 Hz");
    eqSection.addChildComponent (p1LowFreqSel);
    eqSection.addChildComponent (charMoreBtn);
    charMoreBtn.setTooltip ("Show the rest of this EQ's controls (frequencies, Q, bandwidth)");
    charMoreBtn.onClick = [this] { charEqMore = charMoreBtn.getToggleState(); layoutCharEq(); };
   #if MISHKAL_V71
    for (int t = 0; t < eqTypeNames.size(); ++t)   // V71: each character EQ on stereo, mid or side
        if (auto* pp = p.apvts.getParameter (charEqId (t, "Place")))
        {
            auto& sel = charPlaceSel[(size_t) t];
            sel = std::make_unique<vox::ui::ModeSelector> (*pp, juce::StringArray { "stereo", "mid", "side" }, juce::Colour (0xffd8d8dc));
            sel->pills = true;
            sel->pillCaption = "m/s";
            sel->colourFor = [] (int i) { return vox::ui::placeColour (i); };
            sel->setTooltip ("Where this EQ works: Stereo = both sides as usual. Mid = only the centre (lead vocal, kick, bass, snare). "
                             "Side = only the width (reverbs, doubles, wide guitars, room). Try Air or Presence on the side to open up a mix "
                             "without making the vocal harsh, or a little Body on the mid only. Each EQ has its own setting. Stereo tracks only.");
            eqSection.addChildComponent (*sel);
        }
   #endif
    rcBehavior.stepperLabel = "RELEASE SHAPE";
    rcCharacter.stepperLabel = "TONE";
    dsMode.batLabels = { "Split", "Wide" };
    limMode.batLabels = { "Classic", "Multi" };
    for (auto* b : { &rcArc, &arc }) { b->getProperties().set ("batTop", "Auto Rel"); b->getProperties().set ("batBottom", "Off"); }
   #if MISHKAL_V62
    limTpAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id::limTp, limTpBtn);
    limTpBtn.getProperties().set ("batTop", "True Pk"); limTpBtn.getProperties().set ("batBottom", "Off");
    limTpBtn.setColour (juce::ToggleButton::tickColourId, pal::limiter);
    limTpBtn.setTooltip ("True peak: the limiter also checks the peaks that appear between samples once the audio is played "
                         "back or turned into an MP3 / streaming file, so the ceiling holds there too (set the ceiling to -1 dB). "
                         "Use it on masters for streaming.");
    limSection.addAndMakeVisible (limTpBtn);
    limClipSel = std::make_unique<vox::ui::ModeSelector> (*p.apvts.getParameter (id::limClip),
                                                          juce::StringArray { "Off", "1 dB", "2 dB", "3 dB", "4 dB", "6 dB" }, pal::limiter);
    limClipSel->dropdownLabel = "CLIPPER";
    limClipSel->setTooltip ("Clipper: pushes the level up by this much and shaves the tallest peaks off just before the limiter "
                            "(with a short smooth knee), so the limiter works less. Louder, punchier drums and masters; "
                            "too much sounds crunchy. 1-3 dB is the usual range.");
    limSection.addAndMakeVisible (*limClipSel);
   #endif
    tapeNoiseBtn.getProperties().set ("batTop", "Noise");
    tapeNoiseBtn.getProperties().set ("batBottom", "Off");
    goliathNotch.setInterceptsMouseClicks (false, false);
    goliathNotch.painter = [] (juce::Graphics& g)
    {
        const float w = (float) g.getClipBounds().getRight(), h = 14.0f;
        juce::Path p;
        p.startNewSubPath (0.0f, 0.0f);
        p.lineTo (560.0f, 0.0f);
        p.cubicTo (522.0f, 0.0f, 518.0f, h, 486.0f, h);
        p.lineTo (74.0f, h);
        p.cubicTo (42.0f, h, 38.0f, 0.0f, 0.0f, 0.0f);
        p.closeSubPath();
        juce::ignoreUnused (w);
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillPath (p);
    };
    goliathDrawer.painter = [this] (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (0xff070708));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawHorizontalLine (0, 0.0f, (float) goliathDrawer.getWidth());
    };
    drawerOpen = (bool) p.apvts.state.getProperty ("drawerOpen", false);
    drawerBar.onToggle = [this] { setDrawerOpen (! drawerOpen, true); };
    limScope.setVisible (false);
    reparentForSkin();
    showCharEq (eqTypes.getSelected());   // after the group labels exist
    showComp (compTypes.getSelected());
    addAndMakeVisible (content);
    content.painter = [this] (juce::Graphics& gr) { paintContent (gr); };
    const float savedScale = juce::jlimit (0.6f, 1.6f, (float) (double) p.apvts.state.getProperty ("uiScale", MISHKAL_V59 ? 0.75 : 1.0));
    uiScale = savedScale;
    // drag the corner to any size; the shape stays locked so nothing stretches
    setResizable (true, true);
    applyBaseSize();
    uiScale = savedScale;   // (setting the limits may have resized us to the minimum on the way)
    sizeReady = true;
    setSize (juce::roundToInt (baseWidth * uiScale), juce::roundToInt (baseHeight * uiScale));
    sendLookAndFeelChange();   // members built before the skin loaded pick up its colours now
    startTimerHz (30);
}

VoxStripEditor::~VoxStripEditor() { setLookAndFeel (nullptr); }

void VoxStripEditor::showRightTab (int index)
{
    bandPanel.setVisible (index == 0);
    resoPanel.setVisible (index == 1);
    cleanGraph.resoView = isGoliath() && index == 1;   // Goliath: the screen follows (EQ or resonance view)
    cleanGraph.repaint();
    content.repaint();
}

void VoxStripEditor::applyBaseSize()
{
    const int w = isGoliath() ? 1440 : 1140 + 2 * (ioStripWidth + 12);
    const int h = isGoliath() ? 1130 + (MISHKAL_V36 && drawerOpen ? 220 : 0) : 970;   // V36: the drawer adds its height below
    baseWidth = w;
    baseHeight = h;
    const bool wasReady = sizeReady;
    sizeReady = false;   // the limit changes below must not overwrite the saved scale
    const float keep = uiScale;
    setResizeLimits (juce::roundToInt (w * 0.6f), juce::roundToInt (h * 0.6f), juce::roundToInt (w * 1.6f), juce::roundToInt (h * 1.6f));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
    uiScale = keep;
    sizeReady = wasReady;
    if (wasReady) setSize (juce::roundToInt (w * uiScale), juce::roundToInt (h * uiScale));
}

void VoxStripEditor::reparentForSkin()
{
    const bool gol = isGoliath();
    // Goliath: the EQ graph and its phase controls live in the black display at the top
    juce::Component& graphHome = gol ? static_cast<juce::Component&> (content) : static_cast<juce::Component&> (cleanSection);
    for (auto* c : { (juce::Component*) &cleanGraph, (juce::Component*) &tahorPhaseSel, (juce::Component*) &tahorOsSel, (juce::Component*) &tahorOsLabel })
        if (c->getParentComponent() != &graphHome) graphHome.addAndMakeVisible (c);

    // Zerem + Tape: in the drawer (Goliath) or on the main panel
    for (auto* sec : { (juce::Component*) &flowSection, (juce::Component*) &tapeSection })
    {
        juce::Component& home = gol ? static_cast<juce::Component&> (goliathDrawer) : static_cast<juce::Component&> (content);
        if (sec->getParentComponent() != &home) home.addAndMakeVisible (sec);
    }
    for (auto* c : { (juce::Component*) &limScope, (juce::Component*) &meterBridge, (juce::Component*) &goliathNotch,
                     (juce::Component*) &goliathDrawer, (juce::Component*) &drawerBar })
    {
        if (gol) content.addAndMakeVisible (c);
        else c->setVisible (false);
    }
    meterBridge.setActive (gol);
    inStrip.setActive (! gol);
    outStrip.setActive (! gol);
    inStrip.setVisible (! gol);
    outStrip.setVisible (! gol);
    if (gol)
    {
        // Input knob and 1964 PK go into the Tahor module, Output into the Limiter module
        inStrip.detach();
        outStrip.detach();
        cleanSection.addAndMakeVisible (*inputKnob);
        cleanSection.addAndMakeVisible (peakBtn);
        for (auto* c : { (juce::Component*) pkInKnob, (juce::Component*) pkOutKnob, (juce::Component*) &peakLinkBtn, (juce::Component*) &pkPanel, (juce::Component*) &pkPowerBtn, (juce::Component*) &pkLinkLabel })
            c->setVisible (MISHKAL_V24);
        pkPowerBtn.setVisible (MISHKAL_V25);
        pkLinkLabel.setVisible (MISHKAL_V25);
        peakLinkBtn.setButtonText (MISHKAL_V25 ? "" : "link");
        peakBtn.setVisible (! MISHKAL_V25);   // V25: the power button replaces the lamp button
        limSection.addAndMakeVisible (*outputKnob);
        limSection.addAndMakeVisible (ledBlock);
    }
    else
    {
        inStrip.setKnob (*inputKnob);
        inStrip.setButton (peakBtn);
        outStrip.setKnob (*outputKnob);
        ledBlock.setVisible (false);
        for (auto* c : { (juce::Component*) pkInKnob, (juce::Component*) pkOutKnob, (juce::Component*) &peakLinkBtn, (juce::Component*) &pkPanel, (juce::Component*) &pkPowerBtn, (juce::Component*) &pkLinkLabel })
            c->setVisible (false);
        pkFlow.setVisible (false);
        rvFlow.setVisible (false);
    }
    limMeter.setVisible (! gol);
    {
        const bool dd = gol && MISHKAL_V20;   // V20: soft clip and oversampling as dropdowns
        limSoftSel.setVisible (! gol || dd);
        if (limSoftKnob != nullptr) limSoftKnob->setVisible (gol && ! dd);
        limSoftSel.dropdownLabel = dd ? "SOFT CLIP" : "";
        limSoftSel.setNames (dd ? juce::StringArray { "Off", "2 dB", "3 dB", "4 dB", "6 dB" }
                                : juce::StringArray { "Soft off", "2 dB", "3 dB", "4 dB", "6 dB" });
        limOsSel.dropdownLabel = dd ? "OVERSAMPLING" : "";
        limOsLabel.setVisible (! dd);
    }

    // Goliath uses rotary knobs everywhere (no long faders)
    for (auto* k : { cGate, cBreath, cRider, cThresh, cRatio, cAttack, cRelease, cTc, cMakeup, dFreq })
        if (k != nullptr) k->setFader (! gol);
    inStrip.setCompact (false);
    outStrip.setCompact (false);
    compInfo.setVisible (! gol);
    limSoftLabel.setVisible (false);
    tahorPhaseSel.screenTabs = tahorOsSel.screenTabs = gol;
    eqTypes.vertical = gol;
    resoPowerBtn.setVisible (gol);
    tahorViewTab.setVisible (gol);
    resoViewTab.setVisible (gol);
    rightTabs.setVisible (! gol);
    if (! gol) charScreen.setVisible (false);
    if (eqDropdown.getParentComponent() == nullptr) eqSection.addChildComponent (eqDropdown);
    eqDropdown.setVisible (gol);
    eqTypes.setVisible (! gol);
    if (compDropdown.getParentComponent() == nullptr) compSection.addChildComponent (compDropdown);
   #if MISHKAL_V81
    if (eqLane.getParentComponent() == nullptr) { eqSection.addChildComponent (eqLane); compSection.addChildComponent (compLane); }
    eqLane.setVisible (gol);
    compLane.setVisible (gol);
   #endif
    if (grRing.getParentComponent() == nullptr) { compSection.addChildComponent (grRing); compSection.addChildComponent (dynBlock); }
    grRing.setVisible (gol);
    dynBlock.setVisible (gol);
    if (! gol) compMeter.setVisible (true);
    compDropdown.setVisible (gol);
    compTypes.setVisible (! gol);
    scHpfSel.dropdownLabel = gol ? (MISHKAL_V64 ? "SC HPF" : "SIDECHAIN HPF") : "";   // V64: shorter, the key switch sits beside it
    scHpfSel.setNames (gol ? juce::StringArray { "Off", "80 Hz", "120 Hz", "160 Hz" } : juce::StringArray { "SC off", "80", "120", "160" });
    charMoreBtn.setVisible (gol);
    if (! gol) p1LowFreqSel.setVisible (false);
}

void VoxStripEditor::setSkin (int index)
{
    if (MISHKAL_V56) index = (int) SkinStyle::Goliath;   // V56: Goliath only
    applySkin ((SkinStyle) juce::jlimit (0, skinNames.size() - 1, index));
    proc.apvts.state.setProperty ("skin", index, nullptr);   // saved with the session
    reparentForSkin();
    applyBaseSize();
    lnf.applyColours();
    tapeNoiseBtn.setColour (juce::ToggleButton::tickColourId, pal::eq);
    listen.setColour (juce::ToggleButton::tickColourId, pal::deess);
    arc.setColour (juce::ToggleButton::tickColourId, pal::limiter);
    showCharEq (eqTypes.getSelected());   // re-binds the knob titles for this skin and re-colours the group labels
    compInfo.setColour (juce::Label::textColourId, pal::dim);
    for (auto* lb : { &tapeOsLabel, &limOsLabel, &tahorOsLabel }) lb->setColour (juce::Label::textColourId, pal::dim);
    rebuildTexture();
    sendLookAndFeelChange();
    content.repaint();
    bandPanel.showBand (bandPanel.getBand());   // re-tint the band controls
    resized();
    repaint();
}

void VoxStripEditor::rebuildTexture()
{
    texture = {};
    if (! isHardware() || baseWidth <= 0) return;

    // Brushed metal: horizontal streaks of fine noise
    texture = juce::Image (juce::Image::ARGB, baseWidth, baseHeight, true);
    juce::Image::BitmapData bd (texture, juce::Image::BitmapData::writeOnly);
    juce::Random rng (42);
    for (int y = 0; y < baseHeight; ++y)
    {
        float streak = rng.nextFloat() * 2.0f - 1.0f;
        float run = 0.0f;
        for (int x = 0; x < baseWidth; ++x)
        {
            if (rng.nextInt (90) == 0) streak = rng.nextFloat() * 2.0f - 1.0f;
            run = run * 0.9f + (rng.nextFloat() * 2.0f - 1.0f) * 0.1f;
            const float v = streak * 0.6f + run * 2.5f;
            bd.setPixelColour (x, y, v > 0.0f ? juce::Colours::white.withAlpha (juce::jmin (0.07f, v * 0.05f))
                                               : juce::Colours::black.withAlpha (juce::jmin (0.10f, -v * 0.07f)));
        }
    }
}

void VoxStripEditor::applyHardwareLooks()
{
    // Sections keep the skin's look; each mode shows its identity through its light colour only.
    // (The full per-mode panel looks in HardwareLook.cpp are kept but not used.)
    eqSection.setLook ({});
    compSection.setLook ({});
    for (auto* k : { hpf, bodyF, bodyG, mudF, mudG, mudQ, presF, presG, presQ, airF, airG, silk,
                     cGate, cBreath, cRider, cThresh, cRatio, cAttack, cRelease, cTc, cMakeup, cMix })
        k->setLook (KnobStyle::Skin, {}, {}, {});

    const bool p1 = eqGroups.size() == 3;
    const juce::Colour cols5[5] { pal::eqLow, pal::eqLow, pal::eq, pal::eq, pal::eqHigh };
    const juce::Colour cols3[3] { pal::eqLow, pal::eq, pal::eqHigh };
    for (int i = 0; i < eqColumnLabels.size(); ++i)
        eqColumnLabels[i]->setColour (juce::Label::textColourId, p1 ? cols3[juce::jmin (i, 2)] : cols5[i]);
    compInfo.setColour (juce::Label::textColourId, pal::dim);
    for (auto* lb : { &tapeOsLabel, &limOsLabel, &tahorOsLabel }) lb->setColour (juce::Label::textColourId, pal::dim);

    // compressor sliders carry the selected mode's light colour
    const auto mode = (CompType) compTypes.getSelected();
    const auto fillCol = compModeColour (mode);
    for (auto* k : { cGate, cBreath, cRider, cThresh, cRatio, cAttack, cRelease, cTc, cMakeup })
    {
        k->slider.setColour (juce::Slider::trackColourId, fillCol);
        k->repaint();
    }

    // Mix knob matches the outboard unit the mode is modelled on
    const auto outboard = hwLookForCompKnob (mode);
    if (isGoliath())
    {
        cMix->setLook (KnobStyle::Skin, {}, {}, {});
        for (auto* l : eqColumnLabels) l->setColour (juce::Label::textColourId, goliathSage());
       #if MISHKAL_V22
        {
            // V22: faceplate and knobs after the hardware each compressor is modelled on (colours and shapes only, no logos)
            HwLook l;
            l.active = true;
            KnobStyle ks = KnobStyle::BlackChrome;
            juce::Colour cap (0xff111111);
            if (MISHKAL_V75) compPlate.stripe = juce::Colours::transparentBlack;   // V75: only units with a stripe set one
           #if MISHKAL_V76
            compPlate.badge = {};
            compPlate.frameColour = juce::Colours::transparentBlack;
           #endif
            switch (mode)
            {
                case CompType::FetPurple:   // FET limiting amplifier: black anodised plate, white legends, black knobs with chrome rings
                {
                    l.faceTop = juce::Colour (0xff222225); l.faceBottom = juce::Colour (0xff0f0f11);
                    l.text = juce::Colour (0xfff2f2f2); l.label = juce::Colour (0xffd9d9d9);
                    ks = KnobStyle::BlackChrome;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-76  \xc2\xb7  FET LIMITING AMPLIFIER"));
                    compPlate.stripe = juce::Colours::transparentBlack;
                   #if MISHKAL_V27
                    const int rev = juce::roundToInt (proc.apvts.getRawParameterValue (compId (0, "Rev"))->load());
                    shownFetRev = rev;
                    if (rev == 0)        // Purple: deep purple-black plate
                    {
                        l.faceTop = juce::Colour (0xff2c2240); l.faceBottom = juce::Colour (0xff160f22);
                        l.label = juce::Colour (0xffd8ccff);
                    }
                    else if (rev == 1)   // Blue Stripe: brushed silver plate with the blue band, dark legends
                    {
                        l.faceTop = juce::Colour (0xffc4c6c8); l.faceBottom = juce::Colour (0xff9a9da0);
                        l.text = juce::Colour (0xff15171a); l.label = juce::Colour (0xff1c1f24);
                        l.lightPlate = true;
                        compPlate.stripe = juce::Colour (0xff2f6fd6);
                    }
                    // rev 2, Black: the black anodised plate above
                   #endif
                    break;
                }
                case CompType::RComp:       // program compressor: brushed silver plate, dark legends, turned aluminium knobs
                    l.faceTop = juce::Colour (0xffb9b9b4); l.faceBottom = juce::Colour (0xff8e8d88);
                    l.text = juce::Colour (0xff1c1d1f); l.label = juce::Colour (0xff2a2b2e);
                    l.lightPlate = true;
                    ks = KnobStyle::Aluminium; cap = juce::Colour (0xffd0d0d0);
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-RC  \xc2\xb7  PROGRAM COMPRESSOR"));
                   #if MISHKAL_V30
                    if (juce::roundToInt (proc.apvts.getRawParameterValue (compId (1, "Model"))->load()) == 1)
                        compPlate.legend = juce::String (juce::CharPointer_UTF8 (MISHKAL_V32 ? "LE-RV  \xc2\xb7  VOCAL LEVELER" : "LE-RC  \xc2\xb7  VOCAL LEVELER"));
                   #endif
                    break;
               #if MISHKAL_V75
                case CompType::Api2500:     // light grey brushed plate, dark legends, black knobs with blue accents
                    l.faceTop = juce::Colour (0xffc9ccd0); l.faceBottom = juce::Colour (0xffa4a8ad);
                    l.text = juce::Colour (0xff111318); l.label = juce::Colour (0xff1b2a4a);
                    l.lightPlate = true;
                    ks = KnobStyle::BlackChrome;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-2500  \xc2\xb7  STEREO BUS COMPRESSOR"));
                    compPlate.stripe = juce::Colour (0xff2f6bff);
                   #if MISHKAL_V76
                    // V76: no band (that read as a Blue Stripe FET); a warm grey panel, a blue badge, matte knurled black knobs
                    l.faceTop = juce::Colour (0xffd3d1cb); l.faceBottom = juce::Colour (0xffaeaca6);
                    l.text = juce::Colour (0xff15171a); l.label = juce::Colour (0xff15171a);
                    ks = KnobStyle::KnurledBlack;
                    compPlate.stripe = juce::Colours::transparentBlack;
                    compPlate.badge = "LE";
                    compPlate.badgeColour = juce::Colour (0xff1f4fa8);
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("2500  \xc2\xb7  STEREO BUS COMPRESSOR"));
                   #endif
                    break;
                case CompType::SslG:        // dark console grey, white legends, grey knobs (the red stripe of the console)
                    l.faceTop = juce::Colour (0xff3b3d42); l.faceBottom = juce::Colour (0xff26282c);
                    l.text = juce::Colour (0xfff0f0f0); l.label = juce::Colour (0xffe6e6e6);
                    ks = KnobStyle::Aluminium; cap = juce::Colour (0xffbfbfbf);
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-G  \xc2\xb7  BUS COMPRESSOR"));
                    compPlate.stripe = juce::Colour (0xffc62f2f);
                   #if MISHKAL_V76
                    // V76: console charcoal with a printed white frame, grey knobs with coloured caps
                    l.faceTop = juce::Colour (0xff35373b); l.faceBottom = juce::Colour (0xff222327);
                    ks = KnobStyle::CapColour;
                    compPlate.stripe = juce::Colours::transparentBlack;
                    compPlate.frameColour = juce::Colours::white.withAlpha (0.5f);
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-G  \xc2\xb7  STEREO BUS COMPRESSOR"));
                   #endif
                    break;
                case CompType::WeissDs1:    // anthracite and silver, precise
                    l.faceTop = juce::Colour (0xff2e3134); l.faceBottom = juce::Colour (0xff1c1e20);
                    l.text = juce::Colour (0xffdfe3e8); l.label = juce::Colour (0xffcfd3d8);
                    ks = KnobStyle::Aluminium; cap = juce::Colour (0xffd8dbe0);
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-DS1  \xc2\xb7  MASTERING DYNAMICS"));
                    compPlate.stripe = juce::Colours::transparentBlack;
                    break;
                case CompType::SplIron:     // dark iron hammertone, warm bronze legends, black knobs
                    l.faceTop = juce::Colour (0xff3a3632); l.faceBottom = juce::Colour (0xff221f1c);
                    l.text = juce::Colour (0xffe8c9a0); l.label = juce::Colour (0xffd9b48a);
                    l.hammertone = true;
                    ks = KnobStyle::Bakelite;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-IRON  \xc2\xb7  TUBE MASTERING COMPRESSOR"));
                    compPlate.stripe = juce::Colours::transparentBlack;
                    break;
                case CompType::Neve33609:   // Neve blue-grey hammertone, cream legends
                    l.faceTop = juce::Colour (0xff5d6f80); l.faceBottom = juce::Colour (0xff3f4d5a);
                    l.text = juce::Colour (0xfff3ead2); l.label = juce::Colour (0xfff3ead2);
                    l.hammertone = true;
                    ks = KnobStyle::BlackChrome;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-336  \xc2\xb7  COMPRESSOR / LIMITER"));
                    compPlate.stripe = juce::Colours::transparentBlack;
                    break;
               #endif
                case CompType::StaLevel:    // V52: LE-STA - broadcast-blue hammertone plate, cream legends, black knobs
                    l.faceTop = juce::Colour (0xff4f6270); l.faceBottom = juce::Colour (0xff33424d);
                    l.text = juce::Colour (0xfff3ead2); l.label = juce::Colour (0xfff3ead2);
                    l.hammertone = true;
                    ks = KnobStyle::Bakelite;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-STA  \xc2\xb7  PROGRAM LEVEL AMPLIFIER"));
                    break;
                case CompType::VariMu660:   // vari-mu: grey hammertone plate, cream legends, big black bakelite knobs
                default:
                    l.faceTop = juce::Colour (0xff6f7472); l.faceBottom = juce::Colour (0xff4c504f);
                    l.text = juce::Colour (0xfff3ead2); l.label = juce::Colour (0xfff3ead2);
                    l.hammertone = true;
                    ks = KnobStyle::Bakelite;
                    compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-660  \xc2\xb7  VARIABLE-MU LIMITER"));
                   #if MISHKAL_V49
                    if (! MISHKAL_V52 && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1)
                    {
                        // LE-STA: broadcast-blue hammertone plate, cream legends, black knobs
                        l.faceTop = juce::Colour (0xff4f6270); l.faceBottom = juce::Colour (0xff33424d);
                        compPlate.legend = juce::String (juce::CharPointer_UTF8 ("LE-STA  \xc2\xb7  PROGRAM LEVEL AMPLIFIER"));
                    }
                   #endif
                    break;
            }
            compPlate.look = l;
            compPlate.repaint();
            for (auto* k : { cRatio, cAttack, cRelease, cTc, cMakeup, cMix })
                if (! (MISHKAL_V26 && k == cMix))   // V26: Mix keeps the Reso look
                    k->setLook (ks, cap, l.text, l.label);
           #if MISHKAL_V76
            if (mode == CompType::SslG)   // console caps: Ratio red, Attack blue, Release green, Make-up grey
            {
                cRatio->setLook (ks, juce::Colour (0xffc8352c), l.text, l.label);
                cAttack->setLook (ks, juce::Colour (0xff2f62b8), l.text, l.label);
                cRelease->setLook (ks, juce::Colour (0xff2f8f55), l.text, l.label);
                cMakeup->setLook (ks, juce::Colour (0xff9a9da3), l.text, l.label);
            }
           #endif
            if (MISHKAL_V58)   // V58 (Figma): LE-STA's Output is a clean ring knob in the plate's cream
            {
                auto& op = cMakeup->slider.getProperties();
                if (mode == CompType::StaLevel)
                {
                    cMakeup->setLook (KnobStyle::Skin, {}, {}, l.label);
                    op.set ("minimal", true);
                    op.set ("minimalColour", (juce::int64) 0xfff3ead2);
                    op.set ("minimalTitle", (juce::int64) l.label.getARGB());
                }
                else
                {
                    op.remove ("minimal"); op.remove ("minimalColour"); op.remove ("minimalTitle");
                }
                cMakeup->resized();
                cMakeup->slider.repaint();
            }
            voxLinkLabel.setColour (juce::Label::textColourId, l.label);
           #if MISHKAL_V29
            ratioButtons.legend = l.label;
            ratioButtons.repaint();
           #endif
            if (MISHKAL_V26)   // the Reso-style Mix name stays readable on the silver plate
            {
                const auto tc = l.label;   // the plate's own legend colour
                cMix->slider.getProperties().set ("minimalTitle", (juce::int64) tc.getARGB());
                cMix->title.setColour (juce::Label::textColourId, tc);
            }
        }
       #endif
    }
    else
    cMix->setLook (outboard, mode == CompType::RComp ? juce::Colour (0xff4b5158) : juce::Colour (0xff111111),
                   pal::text, pal::text.withAlpha (0.78f));

    dFreq->slider.setColour (juce::Slider::trackColourId, isOnyx() ? pal::eq : pal::deess);   // Onyx: yellow line
    dsAuto.setColour (juce::ToggleButton::tickColourId, isOnyx() ? pal::eq : pal::deess);
    dsMode.colourFor = [] (int) { return isOnyx() ? pal::eq : pal::deess; };
    dFreq->repaint();
}

void VoxStripEditor::showCharEq (int t)
{
    for (auto* k : { hpf, bodyF, bodyG, mudF, mudG, mudQ, presF, presG, presQ, airF, airG, silk })
        k->setVisible (true);

    juce::StringArray names;
    if (t == (int) EqType::TubeP1)
    {
        // Pultec-style panel: LOW (freq, boost, atten) | HIGH BOOST (freq, bandwidth, boost) | HIGH ATTEN (freq, atten)
        bodyF->bind (proc.apvts, charEqId (0, "LowFreq"),   "Freq",      0);
        bodyG->bind (proc.apvts, charEqId (0, "LowBoost"),  "Boost",     0);
        mudG ->bind (proc.apvts, charEqId (0, "LowAtten"),  "Atten",     0);
        presF->bind (proc.apvts, charEqId (0, "HighFreq"),  "Freq",      0);
        presQ->bind (proc.apvts, charEqId (0, "HighBw"),    "Bandwidth", 0);
        presG->bind (proc.apvts, charEqId (0, "HighBoost"), "Boost",     0);
        airF ->bind (proc.apvts, charEqId (0, "AttenFreq"), "Freq",      0);
        airG ->bind (proc.apvts, charEqId (0, "HighAtten"), "Atten",     0);
        for (auto* k : { hpf, mudF, mudQ, silk }) k->setVisible (false);
        eqGroups = { { bodyF, bodyG, mudG }, { presF, presQ, presG }, { airF, airG } };
        names = { "LOW FREQUENCY", "HIGH BOOST", "HIGH ATTEN" };
    }
    else
    {
        Knob* knobsFor[] { hpf, bodyF, bodyG, mudF, mudG, mudQ, presF, presG, presQ, airF, airG };
        const char* fields[] { "HpfFreq", "BodyFreq", "BodyGain", "MudFreq", "MudGain", "MudQ", "PresFreq", "PresGain", "PresQ", "AirFreq", "AirGain" };
        const char* titles[] { "Freq", "Freq", "Gain", "Freq", "Gain", "Q", "Freq", "Gain", "Q", "Freq", "Gain" };
        const bool bip[] { false, false, true, false, true, false, false, true, false, false, true };
        for (size_t i = 0; i < std::size (fields); ++i)
        {
            if (proc.apvts.getParameter (charEqId (t, fields[i])) == nullptr) { knobsFor[i]->setVisible (false); continue; }   // V74 models: screen only
            knobsFor[i]->bind (proc.apvts, charEqId (t, fields[i]), titles[i], bip[i] ? 1 : 0);
        }
        silk->setVisible (t == (int) EqType::Airy);   // Silk belongs to the Airy EQ
        eqGroups = { { hpf }, { bodyF, bodyG }, { mudF, mudG, mudQ }, { presF, presG, presQ }, { airF, airG, silk } };
        names = { "HIGH-PASS", "BODY", "MUD", "PRESENCE", "AIR" };
    }

    for (int i = 0; i < eqColumnLabels.size(); ++i)
    {
        eqColumnLabels[i]->setText (names[i], juce::dontSendNotification);
        eqColumnLabels[i]->setVisible (i < names.size());
    }
    sendLookAndFeelChange();   // refresh knob titles/colours
    layoutCharEq();
}

void VoxStripEditor::layoutCharEq()
{
    if (eqGroups.empty() || eqSection.getWidth() <= 0) return;
    if (isGoliath()) { layoutCharEqGoliath(); return; }

    auto c = eqSection.getContent();
    const int knobW = c.getWidth() / 12;
    int total = 0;
    for (auto& grp : eqGroups) total += (int) grp.size();

    // centre the groups when there are fewer than 12 knobs (Tube P1 has 8, wider spacing)
    const int unit = juce::jmin (c.getWidth() / juce::jmax (1, total), (int) (knobW * 1.35f));
    auto row = c.withSizeKeepingCentre (unit * total, c.getHeight());
    eqSection.dividers.clear();

    const int lowLabelCount = eqColumnLabels.size();
    for (size_t gi = 0; gi < eqGroups.size(); ++gi)
    {
        auto group = row.removeFromLeft (unit * (int) eqGroups[gi].size());
        if (gi > 0) eqSection.dividers.push_back (group.getX());
        if ((int) gi < lowLabelCount) eqColumnLabels[(int) gi]->setBounds (group.removeFromTop (18));
        group.removeFromTop (4);
        auto knobRow = group.withHeight (juce::jmin (group.getHeight(), 128));
        for (auto* k : eqGroups[gi])
            k->setBounds (knobRow.removeFromLeft (unit).reduced (isRack() ? 1 : 6, 0));
    }

    applyHardwareLooks();
    eqSection.repaint();
}

Knob& VoxStripEditor::add (juce::Component& parent, const juce::String& paramId, const char* title,
                           const juce::Colour& accent, bool bipolar)
{
    knobs.push_back (std::make_unique<Knob> (proc.apvts, paramId, title, accent, bipolar));
    parent.addAndMakeVisible (*knobs.back());
    return *knobs.back();
}

void VoxStripEditor::updateDeEssMeter()
{
    proc.getChain().dsTap.pull (dsWin);
    std::copy (dsWin.begin(), dsWin.end(), dsFftData.begin());
    std::fill (dsFftData.begin() + dsFftSize, dsFftData.end(), 0.0f);
    dsWindow.multiplyWithWindowingTable (dsFftData.data(), (size_t) dsFftSize);
    dsFft.performFrequencyOnlyForwardTransform (dsFftData.data());

    // one cell per slice of the slider's own frequency scale, so each cell sits under the Hz it shows
    auto& sl = dFreq->slider;
    const double binHz = proc.getCurrentSampleRate() / dsFftSize;
    juce::Array<juce::var> cells;
    for (int c = 0; c < dsCells; ++c)
    {
        const double lo = sl.proportionOfLengthToValue ((double) c / dsCells);
        const double hi = sl.proportionOfLengthToValue ((double) (c + 1) / dsCells);
        const int b0 = juce::jlimit (1, dsFftSize / 2 - 1, (int) std::floor (lo / binHz));
        const int b1 = juce::jlimit (b0, dsFftSize / 2 - 1, (int) std::ceil (hi / binHz));
        float mag = 0.0f;
        for (int b = b0; b <= b1; ++b) mag = juce::jmax (mag, dsFftData[(size_t) b]);
        const float db = juce::Decibels::gainToDecibels (mag * 4.0f / (float) dsFftSize, -120.0f)
                       + 3.0f * (float) std::log2 (juce::jmax (1.0, (lo + hi) * 0.5) / 1000.0);   // same tilt as the main analyzer
        auto& d = dsCellDb[(size_t) c];
        d = db > d ? db : juce::jmax (db, d - 2.0f);   // fast rise, smooth fall
        cells.add (d);
    }
    sl.getProperties().set ("spectrum", cells);
    sl.getProperties().set ("reducingDb", proc.meters().dsGr.load());
    sl.repaint();
}

void VoxStripEditor::timerCallback()
{
   #if MISHKAL_V48
    // V48: animate the links (1964 PK in -> out, LE-RV threshold -> output)
    if (isGoliath())
    {
        linkPhase = std::fmod (linkPhase + 0.03f, 1.0f);
        auto setAuto = [this] (vox::ui::Knob* k, float autoDb)
        {
            if (k == nullptr) return;
            auto& props = k->slider.getProperties();
            const float was = props.contains ("autoDb") ? (float) props["autoDb"] : 0.0f;
            props.set ("autoDb", autoDb);
            props.set ("linkPhase", linkPhase);
            if (std::abs (autoDb) > 0.05f || std::abs (was) > 0.05f) k->slider.repaint();
        };
        auto raw = [this] (const juce::String& pid) { return proc.apvts.getRawParameterValue (pid)->load(); };
        const float sig = juce::jlimit (0.0f, 1.0f, (juce::jmax (meterBridge.inPk[0], meterBridge.inPk[1]) + 48.0f) / 48.0f);

        // 1964 PK: with Link on, the output drops by whatever the input pushes
        const bool pkOn = raw (id::peakCatch) > 0.5f, pkLink = raw (id::peakLink) > 0.5f;
        const float pkAuto = pkOn && pkLink ? -raw (id::peakIn) : 0.0f;
        setAuto (pkOutKnob, pkAuto);
        pkFlow.linked = pkLink; pkFlow.amount = pkOn ? pkAuto : 0.0f; pkFlow.level = sig; pkFlow.phase = linkPhase;
        if (pkFlow.isVisible()) pkFlow.repaint();

        // LE-RV: with Link on, pulling the threshold down brings the output up (3:1, 10 dB soft knee)
        const bool rvShown = shownCompType == (int) CompType::RComp && juce::roundToInt (raw (compId (1, "Model"))) == 1;
        float rvAuto = 0.0f;
        if (rvShown && raw (compId (1, "VoxLink")) > 0.5f)
        {
            const float over = -raw (compId (1, "Thresh")), knee = 10.0f, ratio = 3.0f;
            rvAuto = over >= knee * 0.5f ? over * (1.0f - 1.0f / ratio)
                   : over > -knee * 0.5f ? (1.0f - 1.0f / ratio) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee) : 0.0f;
        }
        setAuto (cMakeup, rvShown ? rvAuto : 0.0f);
        rvFlow.linked = raw (compId (1, "VoxLink")) > 0.5f; rvFlow.amount = rvAuto;
        rvFlow.result = cMakeup != nullptr ? (float) cMakeup->slider.getValue() + rvAuto : rvAuto; rvFlow.level = sig; rvFlow.phase = linkPhase;
        rvFlow.setVisible (rvShown && voxLinkBtn.isVisible());
        if (rvFlow.isVisible()) rvFlow.repaint();
    }
   #endif
   #if MISHKAL_V11
    // V11: the Input / Output neon glows with the signal going through it (dark when nothing is playing)
    if (isGoliath() && inputKnob != nullptr && outputKnob != nullptr)
    {
        auto driveTo = [] (vox::ui::Knob* k, float target)
        {
            target = juce::jlimit (0.0f, 1.0f, target);
            auto& props = k->slider.getProperties();
            const float was = props.contains ("glowLevel") ? (float) props["glowLevel"] : 0.0f;
            const float now = target > was ? target : was + (target - was) * 0.25f;   // jump up, fall back gently
            if (std::abs (now - was) > 0.004f || ! props.contains ("glowLevel"))
            {
                props.set ("glowLevel", now);
                k->slider.repaint();
            }
        };
        auto level = [] (float peakDb) { return (peakDb + 48.0f) / 48.0f; };
        const float inLevel = level (juce::jmax (meterBridge.inPk[0], meterBridge.inPk[1]));
        driveTo (inputKnob,  inLevel);
        driveTo (outputKnob, level (juce::jmax (meterBridge.outPk[0], meterBridge.outPk[1])));
       #if MISHKAL_V35
        {
            const auto& m = proc.meters();
            const bool flowOn = proc.apvts.getRawParameterValue (id::flowOn)->load() > 0.5f;
            const bool tapeOn = proc.apvts.getRawParameterValue (id::tapeOn)->load() > 0.5f;
            // Zerem: a soft glow with the signal, full while it is working (6 dB of reduction = full)
            const float z = flowOn ? juce::jmax (inLevel * 0.3f, m.flowGr.load() / 6.0f) : 0.0f;
            // Tape: glows with how hard the tape is being hit (its VU, -30..0 dB)
            const float t = tapeOn ? (m.tapeVuDb.load() + 30.0f) / 30.0f : 0.0f;
            driveTo (fAmount, z); driveTo (fMix, z);
           #if MISHKAL_V37
            // Perach glows with how much it is reshaping (3 dB average correction = full)
            const float pz = flowOn ? juce::jmax (inLevel * 0.3f, m.flowGr.load() / 3.0f) : 0.0f;
            for (auto* k : perKnobs) driveTo (k, pz);
           #endif
            driveTo (tDrive, t);  driveTo (tMix, t);
           #if MISHKAL_V39
            for (auto* k : eshKnobs) driveTo (k, t);
           #endif
        }
       #endif
       #if MISHKAL_V84
        if (cThresh != nullptr)
        {
            // the big Threshold knob: glows with the signal and harder while compressing; in / out arcs on its scale
            const bool compIn = proc.apvts.getRawParameterValue (id::compOn)->load() > 0.5f;
            const bool isThreshold = shownCompType != (int) CompType::StaLevel
                                  && ! (shownCompType == (int) CompType::VariMu660 && ! MISHKAL_V52 && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1);
            auto& kp = cThresh->slider.getProperties();
            const float inDb = juce::jmax (meterBridge.inPk[0], meterBridge.inPk[1]);
            const float gr = compIn ? meterBridge.compGr : 0.0f;
            driveTo (cThresh, compIn ? juce::jmax (inLevel * 0.45f, gr / 6.0f) : inLevel * 0.2f);
            if (isThreshold)
            {
                const float wasIn = kp.contains ("inDb") ? (float) kp["inDb"] : -100.0f, wasOut = kp.contains ("outDb") ? (float) kp["outDb"] : -100.0f;
                const float outDb = inDb - gr;
                if (std::abs (wasIn - inDb) > 0.15f || std::abs (wasOut - outDb) > 0.15f)
                {
                    kp.set ("inDb", inDb); kp.set ("outDb", outDb);
                    cThresh->slider.repaint();
                }
            }
            else if (kp.contains ("inDb")) { kp.remove ("inDb"); kp.remove ("outDb"); cThresh->slider.repaint(); }
        }
       #endif
       #if MISHKAL_V15
        // De-ess threshold: a soft glow with the signal, full glow while it is pulling sibilance down (6 dB = full)
        if (dThresh != nullptr && dThresh->slider.getProperties()["minimalGlow"])
            driveTo (dThresh, juce::jmax (inLevel * 0.35f, proc.meters().dsGr.load() / 6.0f));
       #endif
    }
   #endif
    // Tahor EQ oversampling only applies to Natural mode
    {
        const bool natural = tahorPhaseSel.getIndex() == 0;
        if (tahorOsSel.isEnabled() != natural)
        {
            tahorOsSel.setEnabled (natural);
            tahorOsSel.setAlpha (natural ? 1.0f : 0.35f);
            tahorOsLabel.setAlpha (natural ? 1.0f : 0.35f);
        }
    }
    // level match: show how much louder or quieter the processed vocal is than the raw one
    {
        const float d = proc.levelMatchDb.load();
        const auto t = matchBtn.getToggleState() ? "MATCH " + juce::String (d >= 0.0f ? "+" : "") + juce::String (d, 1) : juce::String ("MATCH");
        if (matchBtn.getButtonText() != t) matchBtn.setButtonText (t);
    }
    // master bypass: dim the processing sections so it's obvious the plugin is out
    {
        const float a = bypassBtn.getToggleState() ? 0.38f : 1.0f;
        for (auto* s : { &cleanSection, &eqSection, &compSection, &dsSection, &flowSection, &tapeSection, &limSection })
            if (std::abs (s->getAlpha() - a) > 0.01f) s->setAlpha (a);
    }
    if (compTypes.getSelected() != shownCompType)
        showComp (compTypes.getSelected());

   #if MISHKAL_V42
    if (isGoliath())   // step numbers on the module titles follow the real order (Character EQs / Dynamics can swap)
    {
        const bool post = proc.apvts.getRawParameterValue (id::eqAfterComp)->load() > 0.5f;
        const std::pair<Section*, int> steps[] { { &cleanSection, 1 }, { &eqSection, post ? 3 : 2 }, { &compSection, post ? 2 : 3 },
                                                 { &dsSection, 4 }, { &flowSection, 5 }, { &tapeSection, 6 }, { &limSection, 7 } };
        for (auto& [sec, n] : steps)
            if ((int) sec->getProperties().getWithDefault ("stepNumber", 0) != n) { sec->getProperties().set ("stepNumber", n); sec->repaint(); }
        drawerBar.setTooltip (MISHKAL_V68 ? "The signal path, left to right. Lit = switched in. Drag CHAR EQ, DYNAMICS, DE-ESS, TONE or TAPE to "
                                            "change the order (e.g. tape before the compressors); 'reset order' puts it back. Click TONE or TAPE "
                                            "(or the arrow) to open the drawer; pre / post flips the Character EQs around the dynamics."
                                          : "The signal path, left to right. Lit = switched in. Click TONE or TAPE (or the arrow) to open the drawer; "
                                            "click pre / post to put the Character EQs before or after the dynamics.");
    }
   #endif
   #if MISHKAL_V39
    if (isGoliath() && juce::roundToInt (proc.apvts.getRawParameterValue (id::tapeModel)->load()) != shownTapeModel)
        resized();   // Tape <-> Esh swaps the controls
   #endif
   #if MISHKAL_V60
    if (proc.isMonoTrack() != shownMono)   // the DAW put us on a mono / stereo track: meters and M/S controls follow
    {
        shownMono = proc.isMonoTrack();
        resized();
        bandPanel.resized();
        repaint();
    }
   #endif
   #if MISHKAL_V37
    if (isGoliath() && juce::roundToInt (proc.apvts.getRawParameterValue (id::flowModel)->load()) != shownFlowModel)
        resized();   // Zerem <-> Perach swaps the controls
   #endif
   #if MISHKAL_V30
    if (isGoliath() && shownCompType == (int) CompType::RComp
        && juce::roundToInt (proc.apvts.getRawParameterValue (compId (1, "Model"))->load()) != shownRcModel)
        layoutCompGoliath();   // RComp <-> RVox changes which controls show (this also re-skins the plate)
   #endif
   #if MISHKAL_V49
    if (isGoliath() && ! MISHKAL_V52 && shownCompType == (int) CompType::VariMu660
        && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) != shownTubeModel)
    {
        layoutCompGoliath();   // LE-660 <-> LE-STA swaps the controls
        applyHardwareLooks();
    }
   #endif
   #if MISHKAL_V27
    if (isGoliath() && shownCompType == (int) CompType::FetPurple
        && juce::roundToInt (proc.apvts.getRawParameterValue (compId (0, "Rev"))->load()) != shownFetRev)
        applyHardwareLooks();   // the plate follows the LE-76 revision
   #endif
   #if MISHKAL_V33
    {
        const int os = juce::roundToInt (proc.apvts.getRawParameterValue (id::globalOs)->load());
        const bool track = proc.apvts.getRawParameterValue (id::trackMode)->load() > 0.5f;
        const juce::String t = os == 0 ? "OS OFF" : os == 1 ? "OS 2X" : "OS 4X";
        if (osBtn.getButtonText() != t) osBtn.setButtonText (t);
        if (osBtn.getToggleState() != (os > 0 && ! track)) osBtn.setToggleState (os > 0 && ! track, juce::dontSendNotification);
        osBtn.setAlpha (track ? 0.45f : 1.0f);   // TRACK overrides it
    }
   #endif
    // dim the controls when the compressor being edited is switched out
    {
        const bool active = proc.apvts.getRawParameterValue (compId (shownCompType, "On"))->load() > 0.5f;
        for (auto* k : { cThresh, cRatio, cAttack, cRelease, cTc, cMakeup, cMix })
            k->setAlpha (active ? 1.0f : 0.45f);
    }
    lRelease->setEnabled (! arc.getToggleState());   // auto release takes over the release
   #if MISHKAL_V69
    if (limEngine != nullptr && limEngine->isVisible())   // Max has its own release (Character), so Auto Rel is dimmed
        arc.setAlpha (limEngine->current() == 2 ? 0.35f : 1.0f);
   #endif

    // feed the gate slider's built-in level meter (smoothed, with a peak hold)
    {
        const float in = proc.meters().gateInDb.load();
        gateMeterDb = in > gateMeterDb ? in : juce::jmax (in, gateMeterDb - 1.5f);
        if (in >= gatePeakDb) { gatePeakDb = in; gatePeakHold = 30; }
        else if (--gatePeakHold < 0) gatePeakDb = juce::jmax (in, gatePeakDb - 0.8f);
        cGate->slider.getProperties().set ("meterDb", gateMeterDb);
        cGate->slider.getProperties().set ("meterPeakDb", gatePeakDb);
        cGate->slider.repaint();
    }
    updateDeEssMeter();
    bandPanel.syncSolo();
    // vocal rider: show the live ride in its slider's slot
    {
        const float ride = proc.meters().riderDb.load();
        const float prev = (float) cRider->slider.getProperties().getWithDefault ("rideDb", 0.0f);
        if (std::abs (ride - prev) > 0.05f || ! cRider->slider.getProperties().contains ("rideDb"))
        {
            cRider->slider.getProperties().set ("rideDb", ride);
            cRider->slider.repaint();
        }
    }
    // PEAK lamp flashes red when the catcher grabs a peak (brighter for bigger catches)
    {
        const float gr = proc.meters().peakGr.exchange (0.0f);
        const float hit = peakBtn.getToggleState() && gr > 0.1f ? juce::jlimit (0.35f, 1.0f, gr / 3.0f) : 0.0f;
        const float next = juce::jmax (hit, peakFlash * 0.75f);
        if (std::abs (next - peakFlash) > 0.01f || (next == 0.0f && peakFlash != 0.0f))
        {
            peakFlash = next < 0.02f ? 0.0f : next;
            peakBtn.getProperties().set ("activity", peakFlash);
            peakBtn.repaint();
        }
       #if MISHKAL_V54
        {
            // the 1964 PK light: red flash on each catch, fading out (the lamp button above is hidden from V25)
            const bool on = proc.apvts.getRawParameterValue (id::peakCatch)->load() > 0.5f;
            const float pkHit = on && gr > 0.1f ? juce::jlimit (0.45f, 1.0f, 0.45f + gr / 4.0f) : 0.0f;
            const float pkNext = juce::jmax (pkHit, pkPanel.flash * 0.8f);
            if (std::abs (pkNext - pkPanel.flash) > 0.01f || (pkNext < 0.02f && pkPanel.flash != 0.0f))
            {
                pkPanel.flash = pkNext < 0.02f ? 0.0f : pkNext;
                pkPanel.repaint (0, 0, 40, 34);
            }
        }
       #endif
    }
    {
        dThresh->setTitleText ((isGoliath() && MISHKAL_V12) ? (dsAuto.getToggleState() ? "sensitivity" : "threshold")
                                                             : (dsAuto.getToggleState() ? "Sensitivity" : "Threshold"));
    }
    // dim the knobs when the EQ being edited is switched out
    {
        const bool active = proc.apvts.getRawParameterValue (charEqId (eqTypes.getSelected(), "On"))->load() > 0.5f;
        for (auto* k : { hpf, bodyF, bodyG, mudF, mudG, mudQ, presF, presG, presQ, airF, airG, silk })
            k->setAlpha (active ? 1.0f : 0.45f);
    }
    refreshToolbar();
}

void VoxStripEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal::bg);   // everything else is drawn by the scaled content
}

void VoxStripEditor::setUiScale (float scale)
{
    uiScale = scale;
    proc.apvts.state.setProperty ("uiScale", scale, nullptr);   // remembered with the session
    setSize (juce::roundToInt (baseWidth * uiScale), juce::roundToInt (baseHeight * uiScale));
}

void VoxStripEditor::paintContent (juce::Graphics& g)
{
    const auto r = content.getLocalBounds().toFloat();
    constexpr float barH = 76.0f;

    if (isGoliath())
    {
        // rack frame behind the modules, black display across the top
        g.fillAll (juce::Colour (0xff070708));
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRect (goliathDisplay);

        // spaced lowercase wordmark on the left, brand on the right
        g.setColour (juce::Colour (0xff8e8e93));
        g.setFont (screenFont (24.0f, 0).withExtraKerningFactor (0.38f));
        if (MISHKAL_V47)
        {
            g.setFont (screenFont (21.0f, 0).withExtraKerningFactor (0.16f));
           #if MISHKAL_V78
            g.setFont (screenFont (24.0f, 0).withExtraKerningFactor (0.30f));
            g.drawText ("Goliath", juce::Rectangle<int> (30, 14, 300, 30), juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff6c6c72));
            g.setFont (screenFont (11.5f, 0).withExtraKerningFactor (0.22f));
            g.drawText (juce::String (juce::CharPointer_UTF8 ("MIXING  \xc2\xb7  MASTERING")), juce::Rectangle<int> (31, 44, 300, 16), juce::Justification::centredLeft);
           #else
            g.drawText (MISHKAL_V53 ? "Goliath channel strip" : "goliath channel strip", juce::Rectangle<int> (30, 20, 440, 36), juce::Justification::centredLeft);
           #endif
        }
        else
        g.drawText ("mishkal_goliath", juce::Rectangle<int> (30, 20, 320, 36), juce::Justification::centredLeft);
        g.setFont (screenFont (26.0f, 2).withExtraKerningFactor (0.04f));
        g.drawText ("MISHKAL", juce::Rectangle<int> (1250, 14, 160, 30), juce::Justification::centredRight);
        g.setFont (screenFont (12.5f, 0));
        g.drawText (juce::String ("by Lovy Elias") + (MISHKAL_VERSION > 1 ? juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  v")) + juce::String (MISHKAL_VERSION) : juce::String()),
                    juce::Rectangle<int> (1230, 44, 180, 16), juce::Justification::centredRight);
        g.setColour (juce::Colour (0xff3a3a3d));
        g.drawHorizontalLine (72, 30.0f, r.getRight() - 30.0f);

        // EQ screen title
        const bool resoShown = cleanGraph.resoView;
        const bool stageOn = proc.apvts.getRawParameterValue (resoShown ? id::resoOn : id::cleanOn)->load() > 0.5f;
        auto t = goliathGraphTitle.toFloat();
        drawGoliathLed (g, juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ t.getX() + 8.0f, t.getCentreY() }), goliathAmber(), stageOn);
        g.setColour (juce::Colour (0xffe6e6e6));
        g.setFont (screenFont (15.0f, 1).withExtraKerningFactor (0.14f));
        g.drawText (resoShown ? "RESO" : "TAHOR EQ", t.withTrimmedLeft (26.0f), juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff76767c));
        g.setFont (screenFont (12.0f, 0).withExtraKerningFactor (0.06f));
        g.drawText (resoShown ? (MISHKAL_V14 ? "dips = resonances being tamed" : "red = resonances being tamed") : "double-click to add a band", t, juce::Justification::centredRight);
        return;
    }

    // Background
    if (isRack())
    {
        g.fillAll (pal::bg);
        setBrushFill (g, 0.6f);
        g.fillAll();
    }
    else if (isHardware())
    {
        g.fillAll (pal::bg);
        if (texture.isValid()) g.drawImageAt (texture, 0, 0);
    }
    else if (isModern())
    {
        g.setGradientFill (juce::ColourGradient (pal::bg2, 0.0f, 0.0f, pal::bg, 0.0f, r.getBottom(), false));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (pal::clean.withAlpha (0.10f), r.getWidth() * 0.15f, 0.0f,
                                                 juce::Colours::transparentBlack, r.getWidth() * 0.15f + 420.0f, 0.0f, true));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (pal::reso.withAlpha (0.07f), r.getWidth() * 0.85f, 0.0f,
                                                 juce::Colours::transparentBlack, r.getWidth() * 0.85f + 380.0f, 0.0f, true));
        g.fillAll();
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (pal::bg2, r.getCentreX(), -120.0f,
                                                 pal::bg, r.getCentreX(), r.getBottom() * 0.9f, true));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (pal::io.withAlpha (0.10f), r.getCentreX(), 0.0f,
                                                 juce::Colours::transparentBlack, r.getCentreX(), 260.0f, true));
        g.fillAll();
    }

    // Vignette
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colours::black.withAlpha (isMetal() ? 0.45f : 0.35f), 0.0f, 0.0f, true));
    g.fillAll();

    // Top bar strip
    if (isMetal() && isLightPanel())
    {
        g.setGradientFill (juce::ColourGradient (pal::panelHi, 0.0f, 0.0f, pal::panel.darker (0.12f), 0.0f, barH, false));
        g.fillRect (r.withHeight (barH));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawHorizontalLine ((int) barH, 0.0f, r.getRight());
    }
    else if (isMetal())
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222225), 0.0f, 0.0f, juce::Colour (0xff17171a), 0.0f, barH, false));
        g.fillRect (r.withHeight (barH));
        g.setColour (juce::Colours::black);
        g.drawHorizontalLine ((int) barH, 0.0f, r.getRight());
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine ((int) barH + 1, 0.0f, r.getRight());
    }
    else
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRect (r.withHeight (barH));
        g.setColour (isRoyal() ? pal::io.withAlpha (0.18f) : pal::line.withAlpha (0.6f));
        g.drawHorizontalLine ((int) barH, 0.0f, r.getRight());
    }

    // Section shadows
    const juce::DropShadow shadow (juce::Colours::black.withAlpha (isModern() ? 0.55f : 0.5f), isMetal() ? 10 : 22, { 0, isMetal() ? 4 : 8 });
    for (auto* s : { &cleanSection, &eqSection, &compSection, &dsSection, &flowSection, &tapeSection, &limSection })
        shadow.drawForRectangle (g, s->getBounds());
    for (auto* s : { (juce::Component*) &inStrip, (juce::Component*) &outStrip })
        shadow.drawForRectangle (g, s->getBounds());

    // Logo + name
    drawLogo (g, { 18.0f, 20.0f, 36.0f, 36.0f });
    juce::GlyphArrangement ga;
    ga.addLineOfText (font (22.0f, true), "MISHKAL", 64.0f, 42.0f);
    juce::Path titlePath;
    ga.createPath (titlePath);

    if (isMetal())
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillPath (titlePath, juce::AffineTransform::translation (0.0f, 1.5f));
        g.setGradientFill (juce::ColourGradient (pal::text, 0.0f, 24.0f, pal::text.darker (0.25f), 0.0f, 44.0f, false));
    }
    else if (isModern())
        g.setGradientFill (juce::ColourGradient (pal::clean, 64.0f, 0.0f, pal::reso, 190.0f, 0.0f, false));
    else
        g.setGradientFill (juce::ColourGradient (pal::io.brighter (0.4f), 0.0f, 22.0f, pal::io.darker (0.3f), 0.0f, 44.0f, false));
    g.fillPath (titlePath);

    // Model name beside the brand, letter-spaced small caps
    {
        const float x0 = ga.getBoundingBox (0, -1, true).getRight() + 9.0f;
        juce::GlyphArrangement sub;
        sub.addLineOfText (font (11.0f, true).withExtraKerningFactor (0.16f), "GOLIATH STRIP", x0, 41.0f);
        juce::Path subPath;
        sub.createPath (subPath);
        g.setColour (pal::dim.brighter (0.25f));
        g.fillPath (subPath);
    }

    // "by" in the plain font, the name in calligraphy (Parisienne, built in)
    g.setColour (pal::dim);
    g.setFont (font (11.0f));
    g.drawText ("by", 65, 47, 20, 13, juce::Justification::centredLeft);
    static const auto script = juce::Typeface::createSystemTypefaceFor (BinaryData::ParisienneRegular_ttf,
                                                                       (size_t) BinaryData::ParisienneRegular_ttfSize);
    g.setColour (pal::text);
    g.setFont (juce::Font (juce::FontOptions().withTypeface (script).withHeight (23.0f)));
    g.drawText ("Lovy Elias", 81, 42, 220, 26, juce::Justification::centredLeft);
}

void VoxStripEditor::resized()
{
    content.setTransform ({});
    content.setBounds (0, 0, baseWidth, baseHeight);
    // the scale follows the window (corner drag or the Size menu) and is remembered with the session
    if (sizeReady && getWidth() > 0)
    {
        const float s = juce::jlimit (0.6f, 1.6f, (float) getWidth() / (float) baseWidth);
        if (std::abs (s - uiScale) > 0.001f)
        {
            uiScale = s;
            proc.apvts.state.setProperty ("uiScale", uiScale, nullptr);
        }
    }
    content.setTransform (juce::AffineTransform::scale (uiScale));

    if (isHardware() && (! texture.isValid() || texture.getWidth() != baseWidth || texture.getHeight() != baseHeight))
        rebuildTexture();

    if (isGoliath()) { layoutGoliath(); return; }

    auto r = content.getLocalBounds().reduced (12);

    auto header = r.removeFromTop (64);

    // Top bar controls, centred
    {
        constexpr int barW = 644 + 14 + 86 + 8 + 112 + 8 + 86 - 6;
        // right-aligned so the "MISHKAL GOLIATH STRIP" title has room on the left
        auto bar = header.removeFromRight (barW).withSizeKeepingCentre (barW, 32);
        auto take = [&bar] (int w, int gapAfter) { auto a = bar.removeFromLeft (w); bar.removeFromLeft (gapAfter); return a; };
        undoBtn.setBounds (take (30, 4));
        redoBtn.setBounds (take (30, 14));
        prevBtn.setBounds (take (30, 6));
        presetBox.setBounds (take (190, 6));
        nextBtn.setBounds (take (30, 14));
        saveBtn.setBounds (take (64, 8));
        saveAsBtn.setBounds (take (80, 14));
        abSwitch.setBounds (take (78, 10));
        menuBtn.setBounds (take (30, 14));
        trackBtn.setBounds (take (86, 8));
        matchBtn.setBounds (take (112, 8));
        bypassBtn.setBounds (take (86, 0));
    }
    r.removeFromTop (12);

    // Input meter strip down the left edge, output down the right
    inStrip.setBounds (r.removeFromLeft (ioStripWidth));
    r.removeFromLeft (12);
    outStrip.setBounds (r.removeFromRight (ioStripWidth));
    r.removeFromRight (12);

    // Clean-up EQ: graph + selected band controls
    cleanSection.setBounds (r.removeFromTop (280));
    {
        auto c = cleanSection.getContent();
        auto right = c.removeFromRight (290);
        bandPanel.setBounds (right);
        resoPanel.setBounds (right);
        rightTabs.setBounds (right.getX(), 8, right.getWidth(), 22);   // in the section header row
        // phase mode + oversampling, in the header row just left of the BAND / RESO tabs
        auto head = juce::Rectangle<int> (c.getX(), 8, right.getX() - 16 - c.getX(), 22);
        tahorOsSel.setBounds (head.removeFromRight (100));
        tahorOsLabel.setBounds (head.removeFromRight (24));
        head.removeFromRight (4);
        tahorPhaseSel.setBounds (head.removeFromRight (230));
        c.removeFromRight (12);
        cleanGraph.setBounds (c);
    }

    r.removeFromTop (10);

    // Character EQ: one row of knobs, grouped by band
    eqSection.setBounds (r.removeFromTop (194));
    {
        eqTypes.setBounds (eqSection.getHeaderRight().removeFromLeft (460));
        eqPosition.setBounds (eqSection.getLocalBounds().reduced (12, 0).removeFromTop (38).reduced (0, 8).removeFromRight (220).withTrimmedRight (isMetal() ? 12 : 0));
        layoutCharEq();
    }

    r.removeFromTop (10);

    // Dynamics grid, read left to right then down:
    //   Compressor | De-esser
    //   Vox Flow   | Limiter
    // In Rack skins each row shares one faceplate.
    const int gap = isRack() ? 0 : 10;
    constexpr int leftW = 680, flowW = 406, tapeW = 276;
    auto row3 = r.removeFromTop (210);
    r.removeFromTop (10);
    auto row4 = r;
    compSection.setBounds (row3.removeFromLeft (leftW));
    row3.removeFromLeft (gap);
    dsSection.setBounds (row3);
    flowSection.setBounds (row4.removeFromLeft (flowW));
    row4.removeFromLeft (gap);
    tapeSection.setBounds (row4.removeFromLeft (tapeW));
    row4.removeFromLeft (gap);
    limSection.setBounds (row4);
    compSection.screwsRight = dsSection.screwsLeft = flowSection.screwsRight = limSection.screwsLeft = ! isRack();
    dsSection.grooveLeft = limSection.grooveLeft = isRack();
    cleanSection.isScreenUnit = true;
    for (auto* sec : { &cleanSection, &eqSection, &compSection, &dsSection, &flowSection, &tapeSection, &limSection }) sec->repaint();

    // Vox Flow: LCD style browser | Amount | Mix | GR meter
    {
        auto c = flowSection.getContent().withTrimmedTop (-6);
        flowMeter.setBounds (c.removeFromRight (40));
        c.removeFromRight (8);
        fMix->setBounds (c.removeFromRight (80));
        fAmount->setBounds (c.removeFromRight (80));
        c.removeFromRight (10);
        flowDisplay.setBounds (c.reduced (0, 2));
    }
    // Tape: style selector in the header; VU meter | Drive | Mix
    {
        tapeStyleSel.setBounds (tapeSection.getLocalBounds().reduced (12, 0).removeFromTop (38).reduced (0, 8).removeFromRight (134));
        auto c = tapeSection.getContent().withTrimmedTop (-6);
        {
            auto top = c.removeFromTop (20);   // width | OS
            tapeOsSel.setBounds (top.removeFromRight (90));
            tapeOsLabel.setBounds (top.removeFromRight (22));
            top.removeFromRight (2);
            tapeWidthSel.setBounds (top);
            c.removeFromTop (2);
        }
        {
            auto left = c.removeFromLeft (62);
            tapeNoiseBtn.setBounds (left.removeFromBottom (20));
            left.removeFromBottom (4);
            tapeVu.setBounds (left.withSizeKeepingCentre (62, juce::jmin (left.getHeight(), 98)));
        }
        c.removeFromLeft (6);
        const int w = c.getWidth() / 2;
        tDrive->setBounds (c.removeFromLeft (w).reduced (1, 0));
        tMix->setBounds (c.reduced (1, 0));
    }

    layoutCompressor();

    auto layoutSimple = [] (Section& s, GrMeter& meter, std::initializer_list<Knob*> ks, juce::Component* extra)
    {
        auto c = s.getContent();
        meter.setBounds (c.removeFromRight (40));
        c.removeFromRight (8);
        if (extra != nullptr)
            extra->setBounds (c.removeFromBottom (24).withSizeKeepingCentre (90, 22));
        auto row = c.withSizeKeepingCentre (c.getWidth(), 96);
        const int w = row.getWidth() / (int) ks.size();
        for (auto* k : ks)
            k->setBounds (row.removeFromLeft (w).reduced (2, 0));
    };

    {
        // De-esser: three long sliders above the LISTEN button
        auto c = dsSection.getContent();
        dsMeter.setBounds (c.removeFromRight (40));
        c.removeFromRight (8);
        {
            auto buttons = c.removeFromBottom (24).withSizeKeepingCentre (176, 22);
            listen.setBounds (buttons.removeFromLeft (86));
            dsAuto.setBounds (buttons.removeFromRight (86));
        }
        dsMode.setBounds (dsSection.getLocalBounds().reduced (12, 0).removeFromTop (38).reduced (0, 8).removeFromRight (110));
        // Threshold + Range knobs on top, Freq slider underneath
        auto block = c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 96 + 30));
        auto knobRow = block.removeFromTop (96).withSizeKeepingCentre (juce::jmin (block.getWidth(), 260), 96);
        dThresh->setBounds (knobRow.removeFromLeft (knobRow.getWidth() / 2).reduced (4, 0));
        dRange->setBounds (knobRow.reduced (4, 0));
        dFreq->setBounds (block.removeFromTop (30).reduced (0, 4));
    }
    {
        // Limiter: mode + AUTO REL in the header, four knobs and the meter below
        auto head = limSection.getLocalBounds().reduced (12, 0).removeFromTop (38).reduced (0, 8);
        limMode.setBounds (head.removeFromRight (124));
        head.removeFromRight (8);
        arc.setBounds (head.removeFromRight (92));
        auto c = limSection.getContent().withTrimmedTop (-6);
        limMeter.setBounds (c.removeFromRight (40));
        c.removeFromRight (8);
        {
            auto row = c.removeFromTop (20).reduced (4, 0);   // Lavry-style soft saturation | OS, above the knobs
            limOsSel.setBounds (row.removeFromRight (90));
            limOsLabel.setBounds (row.removeFromRight (22));
            row.removeFromRight (6);
            limSoftSel.setBounds (row);
        }
        c.removeFromTop (2);
        const int w = c.getWidth() / 4;
        for (auto* k : { lThresh, lCeiling, lRelease, lMix })
            k->setBounds (c.removeFromLeft (w).reduced (2, 0));
    }
    juce::ignoreUnused (layoutSimple);
}

void VoxStripEditor::showComp (int unit)
{
    // point the shared controls at this compressor's own parameters
    shownCompType = unit;
    threshOnStaIn = (MISHKAL_V52 && unit == (int) CompType::StaLevel)
                 || (MISHKAL_V49 && ! MISHKAL_V52 && unit == (int) CompType::VariMu660
                     && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1);
    cThresh->bind (proc.apvts, threshOnStaIn ? compId (unit, "StaIn") : compId (unit, "Thresh"), threshOnStaIn ? "Input" : "Threshold");
    cMakeup->bind (proc.apvts, compId (unit, "Makeup"));
    cMix->bind    (proc.apvts, compId (unit, "Mix"));
    if (unit == (int) CompType::FetPurple || unit == (int) CompType::RComp)
    {
        cRatio->bind   (proc.apvts, compId (unit, "Ratio"),   MISHKAL_V75 ? "Ratio" : "");
        cAttack->bind  (proc.apvts, compId (unit, "Attack"),  MISHKAL_V75 ? "Attack" : "");
        cRelease->bind (proc.apvts, compId (unit, "Release"), MISHKAL_V75 ? "Release" : "");
    }
   #if MISHKAL_V75
    if (unit == (int) CompType::VariMu660) cTc->bind (proc.apvts, compId (2, "Tc"), "Time Const");
    if (unit >= (int) CompType::Api2500)   // the new units: the same knobs, on their own parameters
    {
        auto has = [this, unit] (const char* f) { return proc.apvts.getParameter (compId (unit, f)) != nullptr; };
        if (has ("Ratio"))   cRatio->bind   (proc.apvts, compId (unit, "Ratio"), "Ratio");
        if (has ("Attack"))  cAttack->bind  (proc.apvts, compId (unit, "Attack"), "Attack");
        if (has ("Release")) cRelease->bind (proc.apvts, compId (unit, "Release"), unit == (int) CompType::Neve33609 ? "Recovery" : "Release");
        if (has ("LimThresh")) cTc->bind (proc.apvts, compId (unit, "LimThresh"), "Limiter");
    }
   #endif
    layoutCompressor();
}

void VoxStripEditor::layoutCompressor()
{
    if (isGoliath()) { layoutCompGoliath(); return; }
    {
        auto head = compSection.getHeaderRight();
        compTypes.setBounds (head.removeFromLeft (340));
        head.removeFromLeft (10);
        scHpfSel.setBounds (head.removeFromLeft (150));
    }

    auto c = compSection.getContent();
    compMeter.setBounds (c.removeFromRight (40));
    c.removeFromRight (8);

    std::vector<Knob*> visible;
    juce::String info;

    switch ((CompType) shownCompType)
    {
        case CompType::RComp:
            visible = { cThresh, cRatio, cAttack, cRelease, cMakeup };
            info = "Clean, versatile control for any source.";
            break;
        case CompType::VariMu660:
            visible = { cThresh, cTc, cMakeup };
            info = "Tube vari-mu: ratio grows as you push harder. TC 1-4 are fixed timings, "
                   "TC 5-6 release slower the longer the signal is compressed. Warm glue for buses, drums and mastering.";
            break;
        case CompType::FetPurple:
        default:
            visible = { cThresh, cRatio, cAttack, cRelease, cMakeup };
            info = "FET character: lightning attack (down to 20 us), punchy release, and "
                   "grit that grows with gain reduction. Punchy on vocals, drums, bass and guitars.";
            break;
    }

    for (auto* k : { cThresh, cRatio, cAttack, cRelease, cTc, cMakeup })
        k->setVisible (std::find (visible.begin(), visible.end(), k) != visible.end());
    for (auto* k : { cGate, cBreath, cRider }) k->setVisible (true);
    cMix->setVisible (true);

    compInfo.setText (info, juce::dontSendNotification);
    {
        auto bottom = c.removeFromBottom (28);
        const bool rc = shownCompType == (int) CompType::RComp;
        for (auto* comp : { (juce::Component*) &rcArc, (juce::Component*) &rcBehavior, (juce::Component*) &rcCharacter })
            comp->setVisible (rc);
        if (rc)
        {
            auto row = bottom.reduced (0, 3);
            rcArc.setBounds (row.removeFromLeft (92));
            row.removeFromLeft (8);
            rcBehavior.setBounds (row.removeFromLeft (130));
            row.removeFromLeft (8);
            rcCharacter.setBounds (row.removeFromLeft (130));
            row.removeFromLeft (12);
            bottom = row;
        }
        compInfo.setBounds (bottom);
    }

    // Mix: an outboard-style knob in its own column at the right
    auto area = c.withTrimmedTop (4).withTrimmedBottom (4);
    cMix->setBounds (area.removeFromRight (92).withSizeKeepingCentre (92, juce::jmin (area.getHeight(), 128)));
    area.removeFromRight (8);

    // Two columns of long horizontal sliders:
    //   left  = the tools before the compressors (Gate, Breath, Rider)
    //   right = the selected compressor's own controls
    const std::vector<Knob*> pre { cGate, cBreath, cRider };
    const int rows = juce::jmax ((int) pre.size(), (int) visible.size());
    const int rowH = juce::jmin (30, area.getHeight() / juce::jmax (1, rows));
    area = area.withHeight (rowH * rows);
    const int colW = (area.getWidth() - 16) / 2;
    auto place = [&] (const std::vector<Knob*>& list, int col)
    {
        for (int i = 0; i < (int) list.size(); ++i)
        {
            auto cell = juce::Rectangle<int> (area.getX() + col * (colW + 16), area.getY() + i * rowH, colW, rowH);
            list[(size_t) i]->setBounds (cell.withTrimmedLeft (col == 0 ? 16 : 0).reduced (0, 3));   // 16 px for the lights
        }
    };
    place (pre, 0);
    place (visible, 1);

    applyHardwareLooks();

    // Gate light sits just left of the GATE label
    const auto gb = cGate->getBounds();
    gateLed.setBounds (gb.getX() - 17, gb.getCentreY() - 8, 16, 16);
    const auto bb = cBreath->getBounds();
    breathLed.setBounds (bb.getX() - 17, bb.getCentreY() - 8, 16, 16);
}

//==============================================================================
FlowDisplay::FlowDisplay (VoxStripProcessor& p)
    : proc (p),
      attachment (*p.apvts.getParameter (id::flowStyle), [this] (float v) { style = juce::roundToInt (v); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setTooltip ("Click the arrows to step through styles, or the name for the full list");
    startTimerHz (30);
}

void FlowDisplay::setStyle (int i)
{
    attachment.setValueAsCompleteGesture ((float) ((i + numFlowStyles) % numFlowStyles));
}

void FlowDisplay::timerCallback()
{
    bool changed = false;
   #if MISHKAL_V82
    {   // the on switch isn't attached here, so watch it: the screen lights up (or dims) the moment Tone is switched
        const bool onNow = proc.apvts.getRawParameterValue (id::flowOn)->load() > 0.5f;
        if (onNow != lastOn) { lastOn = onNow; changed = true; }
    }
   #endif
    for (int b = 0; b < 5; ++b)
    {
        const float v = proc.getChain().flow.bandGr[(size_t) b].load();
        const float nv = v > shown[b] ? v : shown[b] * 0.85f + v * 0.15f;
        if (std::abs (nv - shown[b]) > 0.02f) { shown[b] = nv; changed = true; }
    }
    if (changed) repaint();
}

void FlowDisplay::mouseDown (const juce::MouseEvent& e)
{
    const float x = (float) e.x, w = (float) getWidth();
    if (e.y < getHeight() * 0.55f && x < 34.0f)           setStyle (style - 1);
    else if (e.y < getHeight() * 0.55f && x > w - 34.0f)  setStyle (style + 1);
    else if (e.y < getHeight() * 0.55f)
    {
        juce::PopupMenu m;
        m.addSectionHeader ("Vocal");
        for (int i = 0; i < numFlowStyles; ++i)
        {
            if (i == (int) FlowStyle::Drums) m.addSectionHeader ("Instruments & Mix");
            m.addItem (1 + i, flowStyleInfo (i).name, true, i == style);
        }
        juce::Component::SafePointer<FlowDisplay> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                         [safe] (int r) { if (safe != nullptr && r > 0) safe->setStyle (r - 1); });
    }
}

void FlowDisplay::paint (juce::Graphics& g)
{
    const ScreenColours screenColours;
    auto r = getLocalBounds().toFloat();
    const auto lcd = (isOnyx() || isRack() || isGoliath() ? pal::eq : pal::clean).brighter (isLightPanel() ? 0.9f : 0.0f);
    const bool on = proc.apvts.getRawParameterValue (id::flowOn)->load() > 0.5f;

    // LCD window: black glass with an inner shadow and a faint glow
    g.setColour (juce::Colour (0xff050608));
    g.fillRoundedRectangle (r, 5.0f);
    g.setGradientFill (juce::ColourGradient (lcd.withAlpha (0.07f), r.getCentreX(), r.getCentreY(),
                                             juce::Colours::transparentBlack, r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 10.0f, false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) r.getBottom(), r.getX() + 5.0f, r.getRight() - 5.0f);

    const auto text = lcd.withAlpha (on ? 1.0f : 0.4f);
    auto content = r.reduced (10.0f, 6.0f);

    // top: < STYLE NAME >
    auto top = content.removeFromTop (26.0f);
    auto arrow = [&] (juce::Rectangle<float> a, bool left)
    {
        juce::Path p;
        const auto c = a.getCentre();
        if (left) p.addTriangle (c.x - 5.0f, c.y, c.x + 4.0f, c.y - 6.0f, c.x + 4.0f, c.y + 6.0f);
        else      p.addTriangle (c.x + 5.0f, c.y, c.x - 4.0f, c.y - 6.0f, c.x - 4.0f, c.y + 6.0f);
        g.setColour (text.withAlpha (on ? 0.85f : 0.35f));
        g.fillPath (p);
    };
    arrow (top.removeFromLeft (20.0f), true);
    arrow (top.removeFromRight (20.0f), false);
    g.setColour (text);
    g.setFont (font (17.0f, true));
    g.drawText (juce::String (flowStyleInfo (style).name).toUpperCase(), top, juce::Justification::centred);

    // description
    g.setColour (text.withAlpha (on ? 0.6f : 0.3f));
    g.setFont (font (10.0f));
    g.drawFittedText (flowStyleInfo (style).description, content.removeFromTop (24.0f).toNearestInt(), juce::Justification::centred, 2, 0.9f);

    // five band gain-reduction bars
    content.removeFromTop (4.0f);
    auto bars = content;
    const char* names[5] { "BOOM", "MUD", "BODY", "HARSH", "AIR" };
    const float bw = bars.getWidth() / 5.0f;
    for (int b = 0; b < 5; ++b)
    {
        auto cell = juce::Rectangle<float> (bars.getX() + bw * (float) b, bars.getY(), bw, bars.getHeight()).reduced (3.0f, 0.0f);
        auto label = cell.removeFromBottom (11.0f);
        auto track = cell.reduced (0.0f, 1.0f).withSizeKeepingCentre (cell.getWidth(), 6.0f);
        g.setColour (text.withAlpha (0.12f));
        g.fillRoundedRectangle (track, 2.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, shown[b] / 12.0f);
        if (frac > 0.0f)
        {
            auto fill = track.withWidth (track.getWidth() * frac);
            g.setColour (text.withAlpha (0.3f));
            g.fillRoundedRectangle (fill.expanded (1.5f), 3.0f);
            g.setColour (text);
            g.fillRoundedRectangle (fill, 2.0f);
        }
        g.setColour (text.withAlpha (on ? 0.55f : 0.3f));
        g.setFont (font (8.0f, true));
        g.drawText (names[b], label.expanded (3.0f, 0.0f), juce::Justification::centred, false);
    }
}

//==============================================================================
// Goliath layout (1440 x 1130), matching the Rack Edition design in Figma:
// a black display (top bar | Tahor EQ screen | meter bridge | limiter screen) over a row of
// brushed 500-series modules, edge to edge with 3 px seams.
namespace goliath
{
    constexpr int width = 1440, height = 1130, displayH = 394, seam = 3, barH = 30, drawerH = 330;
    constexpr int moduleW[] { 250, 360, 312, 150 };   // Tahor, Character, Dynamics, De-ess; the Limiter module takes the rest
}

void VoxStripEditor::layoutGoliath()
{
    using namespace goliath;
    const auto all = MISHKAL_V36 ? juce::Rectangle<int> (0, 0, width, height) : content.getLocalBounds();   // V36: the drawer lives below this

    // ---- top bar: transport of presets and compare in the middle, menu on the right
    {
        auto bar = MISHKAL_V49 ? juce::Rectangle<int> (300, 22, 930, 34) : juce::Rectangle<int> (360, 22, 870, 34);
        struct Item { juce::Component* c; int w, gapAfter; };
        // V49: MATCH gets room for its reading ("MATCH -1.6"), the rest tightens up a little
        const Item items[] { { &undoBtn, 26, 2 }, { &redoBtn, 26, MISHKAL_V49 ? 12 : 18 }, { &prevBtn, 24, 4 },
                             { &presetBox, MISHKAL_V49 ? 176 : MISHKAL_V33 ? 190 : 220, 4 }, { &nextBtn, 24, MISHKAL_V49 ? 14 : 20 },
                             { &saveBtn, 50, 4 }, { &saveAsBtn, 74, MISHKAL_V49 ? 10 : 14 }, { &abSwitch, 66, MISHKAL_V49 ? 12 : 14 },
                             { &matchBtn, MISHKAL_V49 ? 108 : 72, MISHKAL_V49 ? 4 : 2 }, { &bypassBtn, MISHKAL_V49 ? 76 : 82, 2 },
                             { &trackBtn, 70, 10 }, { &menuBtn, 26, 0 } };
        int total = 0;
        for (auto& it : items) total += it.w + it.gapAfter;
        if (MISHKAL_V33) total += 70 + 2;
        int x = bar.getCentreX() - total / 2 + (MISHKAL_V33 && ! MISHKAL_V49 ? 14 : 0);
        for (auto& it : items)
        {
            if (MISHKAL_V33 && it.c == &trackBtn)   // OS sits between BYPASS and TRACK
            {
                osBtn.setBounds (x, bar.getY(), 70, bar.getHeight());
                osBtn.setVisible (true);
                x += 72;
            }
            it.c->setBounds (x, bar.getY(), it.w, bar.getHeight());
            x += it.w + it.gapAfter;
        }
        if (MISHKAL_V33)   // the per-module oversampling controls give way to the one OS button
            for (auto* comp : { (juce::Component*) &tahorOsSel, (juce::Component*) &tahorOsLabel, (juce::Component*) &tapeOsSel,
                                (juce::Component*) &tapeOsLabel, (juce::Component*) &limOsSel, (juce::Component*) &limOsLabel })
                comp->setVisible (false);
    }

    // ---- display screens
    goliathDisplay = { 0, 0, width, displayH };
    {
        auto d = juce::Rectangle<int> (30, 88, width - 60, displayH - 88 - 18);
        limScope.setBounds (d.removeFromRight (432));
        d.removeFromRight (34);
        meterBridge.setBounds (d.removeFromRight (304));
        d.removeFromRight (34);
        goliathGraphTitle = d.removeFromTop (24);
        d.removeFromTop (8);
        auto tabs = d.removeFromBottom (20);
        d.removeFromBottom (10);
        cleanGraph.setBounds (d);
        tahorPhaseSel.setBounds (tabs.withSizeKeepingCentre (330, 20).withX (tabs.getX()));
        tahorOsSel.setBounds (tabs.removeFromRight (150));
        tahorOsLabel.setBounds (tabs.removeFromRight (30));
        tahorOsLabel.setColour (juce::Label::textColourId, juce::Colour (0xff76767c));
        tahorOsLabel.setFont (screenFont (12.5f, 1));
    }

    // ---- deck
    goliathDeck = all.withTrimmedTop (displayH).withTrimmedBottom (barH);
    {
        auto deck = goliathDeck.reduced (seam);
        Section* order[] { &cleanSection, &eqSection, &compSection, &dsSection };
        for (int i = 0; i < 4; ++i) { order[i]->setBounds (deck.removeFromLeft (moduleW[i])); deck.removeFromLeft (seam); }
        limSection.setBounds (deck);
    }
    // Zerem + Tape live in the drawer that slides up from the bottom bar
    drawerBar.setBounds (all.withTop (height - barH));
    drawerBar.open = drawerOpen;
    goliathDrawer.setBounds (drawerBounds (drawerOpen));
    if (MISHKAL_V36)
    {
        flowSection.compact = tapeSection.compact = true;
        goliathDrawer.setVisible (drawerOpen);
    }
    {
        auto d = goliathDrawer.getLocalBounds().withTrimmedTop (seam).reduced (seam, 0);
        flowSection.setBounds (d.removeFromLeft ((d.getWidth() - seam) / 2));
        d.removeFromLeft (seam);
        tapeSection.setBounds (d);
    }
    for (auto* sec : { &cleanSection, &eqSection, &compSection, &dsSection, &flowSection, &tapeSection, &limSection })
    {
        sec->dividers.clear();
        sec->resized();
        sec->repaint();
    }
    constexpr int switchW = 52, switchH = 74;   // the On / Bypass toggle sits top-left in every module

    // ---- TAHOR EQ: BAND / RESO, the band's controls, Input + 1964 PK at the bottom
    {
        auto c = cleanSection.getContent();
        auto top = c.removeFromTop (switchH);
        top.removeFromLeft (switchW + 8);
        resoPowerBtn.setBounds (top.removeFromLeft (switchW));   // Tahor's ON is the module's own; Reso gets its own beside it
        // the names above the two buttons pick which controls show below
        const int tabY = cleanSection.getContent().getY() + 2;   // just inside the module body (V52: higher, no screws)
        tahorViewTab.setBounds (12, tabY, switchW, 18);
        resoViewTab.setBounds (resoPowerBtn.getX(), tabY, switchW, 18);
        tahorViewTab.toFront (false);
        resoViewTab.toFront (false);
        c.removeFromTop (8);
        auto bottom = c.removeFromBottom (MISHKAL_V25 ? 138 : MISHKAL_V24 ? 132 : 96);
        if (MISHKAL_V9)   // V9: Input / Output in the minimal Reso style, in their own teal
            for (auto* k : { inputKnob, outputKnob })
            {
                k->slider.getProperties().set ("minimal", true);
                k->slider.getProperties().set ("minimalColour", (juce::int64) MISHKAL_NEON);
                k->setTitleText (k == inputKnob ? "input" : "output");
                k->slider.getProperties().set ("minimalGlow", (bool) MISHKAL_V10);
            }
        if (MISHKAL_V24)   // V24: Input | 1964 PK panel (on/off + link, its own in / out)
        {
            if (MISHKAL_V66 && autoInput != nullptr && isGoliath())   // V66: Auto Input under the knob
            {
                auto col = bottom.removeFromLeft (76);
                autoInput->setBounds (col.removeFromBottom (20).reduced (8, 1));
                autoInput->setVisible (true);
                inputKnob->setBounds (col.withSizeKeepingCentre (76, juce::jmin (104, col.getHeight())));
            }
            else
            inputKnob->setBounds (bottom.removeFromLeft (76).withSizeKeepingCentre (76, 104));
            bottom.removeFromLeft (4);
            pkPanel.title = MISHKAL_V25 ? "1964 pk" : "";
            pkPanel.dot = juce::Colour (0xff9b6bff);
            pkPanel.setBounds (bottom);
            pkPanel.toBack();
            auto in = bottom.reduced (4, 4);
            if (MISHKAL_V25)   // title + power button on top; in | link | out below
            {
                auto head = in.removeFromTop (26);
                pkPowerBtn.setBounds (head.removeFromRight (54));
                in.removeFromTop (2);
                for (auto* k : { pkInKnob, pkOutKnob })
                {
                    k->slider.getProperties().set ("minimalColour", (juce::int64) 0xff9b6bff);   // violet, like the Limiter and LE-76
                    k->slider.getProperties().set ("minimal", true);
                }
                pkInKnob->setTitleText ("in");
                pkOutKnob->setTitleText ("out");
                peakLinkBtn.getProperties().set ("slideOnly", true);
                const int kw = (in.getWidth() - 36) / 2;
                pkInKnob->setBounds (in.removeFromLeft (kw));
                pkOutKnob->setBounds (in.removeFromRight (kw));
                auto mid = in.withTrimmedTop (17);   // level with the rings (under the knob names)
                mid = mid.withTrimmedBottom (26);
                pkLinkLabel.setFont (font (11.0f));
                pkLinkLabel.setJustificationType (juce::Justification::centred);
                pkLinkLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9da0a8));
                pkLinkLabel.setBounds (mid.getCentreX() - 22, mid.getCentreY() - 22, 44, 16);
                peakLinkBtn.setBounds (mid.getCentreX() - 18, mid.getCentreY() - 4, 36, 20);
                pkInKnob->slider.getProperties().set ("minimal", true);
                pkOutKnob->slider.getProperties().set ("minimal", true);
                pkInKnob->setTitleText ("in");
                pkOutKnob->setTitleText ("out");
                pkLinkLabel.toFront (false);
                peakLinkBtn.toFront (false);
                if (MISHKAL_V48)
                {
                    if (pkFlow.getParentComponent() != peakLinkBtn.getParentComponent() && peakLinkBtn.getParentComponent() != nullptr)
                        peakLinkBtn.getParentComponent()->addAndMakeVisible (pkFlow);
                    pkFlow.setVisible (pkInKnob->isVisible());
                    const auto lb = peakLinkBtn.getBounds();
                    pkFlow.setBounds (lb.getCentreX() - 24, lb.getBottom() + 5, 48, 12);
                    pkFlow.toFront (false);
                }
            }
            else
            {
            auto head = in.removeFromTop (22);
            peakBtn.setBounds (head.removeFromLeft (head.getWidth() * 11 / 20));
            peakLinkBtn.setBounds (head);
            }
            if (! MISHKAL_V25)
            {
            in.removeFromTop (2);
            for (auto* k : { pkInKnob, pkOutKnob })
                k->slider.getProperties().set ("minimal", true);
            pkInKnob->setTitleText ("in");
            pkOutKnob->setTitleText ("out");
            pkInKnob->setBounds (in.removeFromLeft (in.getWidth() / 2));
            pkOutKnob->setBounds (in);
            }
        }
        else
        {
        inputKnob->setBounds (bottom.removeFromLeft (bottom.getWidth() / 2));
        peakBtn.setBounds (bottom.withSizeKeepingCentre (bottom.getWidth(), 22));
        }
        c.removeFromBottom (6);
        bandPanel.setBounds (c);
        resoPanel.setBounds (c);
    }

    layoutCharEq();
    layoutCompressor();

    // ---- DE-ESS: Split / Wide switch, Freq / Thresh / Range stacked, meter alongside, Listen + Auto
    {
        auto c = dsSection.getContent();
        auto top = c.removeFromTop (switchH);
        top.removeFromLeft (switchW + 6);
        dsMode.setBounds (top.withSizeKeepingCentre (switchW, switchH));
        c.removeFromTop (8);
        const bool keyRow = MISHKAL_V64 && isGoliath();
        auto buttons = c.removeFromBottom (keyRow ? 76 : 48);
        listen.setBounds (buttons.removeFromTop (20));
        buttons.removeFromTop (8);
        dsAuto.setBounds (buttons.removeFromTop (20));
        dsKeyBtn.setVisible (keyRow);
        if (keyRow) { buttons.removeFromTop (8); dsKeyBtn.setBounds (buttons.removeFromTop (20)); }
        c.removeFromBottom (6);
        dsMeter.setBounds (c.removeFromRight (24));
        const int h = juce::jmin (130, c.getHeight() / 3);
        if (MISHKAL_V12)   // V12: the Reso look - ring, value box, lowercase name on top
        {
            for (auto* k : { dFreq, dThresh, dRange })
                k->slider.getProperties().set ("minimal", true);
            dFreq->setTitleText ("freq");
            dRange->setTitleText ("range");
            dThresh->setTitleText (dsAuto.getToggleState() ? "sensitivity" : "threshold");
            dThresh->slider.getProperties().set ("minimalGlow", (bool) MISHKAL_V15);
        }
        for (auto* k : { dFreq, dThresh, dRange })
            k->setBounds (c.removeFromTop (h));
    }

    // ---- ZEREM (drawer): power | style LCD | Amount | Mix | meter
    {
        auto c = flowSection.getContent();
        c.removeFromLeft (switchW + 12);
        if (MISHKAL_V34)   // V34: the Reso look - a titled flat panel, minimal knobs
        {
            if (zeremPanel.getParentComponent() != &flowSection) flowSection.addAndMakeVisible (zeremPanel);
            zeremPanel.title = "zerem";
            zeremPanel.dot = juce::Colour (MISHKAL_ACCENT);
            zeremPanel.setBounds (c.withTrimmedRight (2));
            zeremPanel.toBack();
            c = c.reduced (12, 6);
            c.removeFromTop (22);
            for (auto* k : { fAmount, fMix }) k->slider.getProperties().set ("minimal", true);
            fAmount->setTitleText ("amount");
            fMix->setTitleText ("mix");
           #if MISHKAL_V37
            {
                const int model = juce::roundToInt (proc.apvts.getRawParameterValue (id::flowModel)->load());
                shownFlowModel = model;
                if (flowModelSel.getParentComponent() != &flowSection)
                {
                    flowSection.addAndMakeVisible (flowModelSel);
                    flowModelSel.stepperLabel = "MODEL";
                    flowModelSel.setTooltip ("Zerem: the multiband styles. Perach: an adaptive tone shaper - it keeps steering the tone "
                                             "toward balance (or the shape you ask for with Low / Body / Presence / Air), only as much as the sound needs.");
                }
                flowModelSel.setVisible (true);
                flowModelSel.setBounds (c.removeFromLeft (110).withSizeKeepingCentre (110, 44));
                c.removeFromLeft (10);
                const bool per = model == 1;
                zeremPanel.title = per ? "perach" : "zerem";
                zeremPanel.dot = per ? juce::Colour (0xffff5fa8) : juce::Colour (MISHKAL_V35 ? 0xff35d6ff : MISHKAL_ACCENT);
                zeremPanel.repaint();
                for (auto* comp : { (juce::Component*) &flowDisplay, (juce::Component*) fAmount, (juce::Component*) fMix })
                    comp->setVisible (! per);
                for (auto* k : perKnobs)
                {
                    k->setVisible (per);
                    k->slider.getProperties().set ("minimal", true);
                    k->slider.getProperties().set ("minimalColour", (juce::int64) 0xffff5fa8);   // rose: Perach's own colour
                    k->slider.getProperties().set ("minimalGlow", true);
                }
                if (MISHKAL_V38)
                {
                    if (perachGraph == nullptr) { perachGraph = std::make_unique<PerachGraph> (proc); flowSection.addChildComponent (*perachGraph); }
                    perachGraph->setVisible (per);
                    for (int i = 1; i <= 4; ++i) perKnobs[(size_t) i]->setVisible (false);   // the graph replaces the tone knobs
                }
                if (per && MISHKAL_V38)   // graph | amount | speed | mix | meter
                {
                    flowMeter.setBounds (c.removeFromRight (24));
                    c.removeFromRight (8);
                    const int kw = 78;
                    perKnobs[6]->setBounds (c.removeFromRight (kw));
                    perKnobs[5]->setBounds (c.removeFromRight (kw));
                    perKnobs[0]->setBounds (c.removeFromRight (kw));
                    c.removeFromRight (8);
                    perachGraph->setBounds (c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 128)));
                }
                else if (per)
                {
                    flowMeter.setBounds (c.removeFromRight (24));
                    c.removeFromRight (8);
                    const int w = c.getWidth() / 7;
                    for (auto* k : perKnobs) k->setBounds (c.removeFromLeft (w));
                }
            }
           #endif
            if (MISHKAL_V35)   // electric cyan, glowing with Zerem's work
            {
                for (auto* k : { fAmount, fMix })
                {
                    k->slider.getProperties().set ("minimalColour", (juce::int64) 0xff35d6ff);
                    k->slider.getProperties().set ("minimalGlow", true);
                }
                if (! (MISHKAL_V37 && shownFlowModel == 1)) zeremPanel.dot = juce::Colour (0xff35d6ff);
            }
        }
        if (! (MISHKAL_V37 && shownFlowModel == 1))
        {
        flowMeter.setBounds (c.removeFromRight (24));
        c.removeFromRight (8);
        fMix->setBounds (c.removeFromRight (130));
        fAmount->setBounds (c.removeFromRight (130));
        c.removeFromRight (12);
        flowDisplay.setBounds (c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 96)));
        }
    }

    // ---- TAPE (drawer): power | noise | VU | speed / width / OS | Drive | Mix
    {
        auto c = tapeSection.getContent();
        c.removeFromLeft (switchW + 8);
        if (MISHKAL_V34)   // V34: the Reso look
        {
            if (tapePanel.getParentComponent() != &tapeSection) tapeSection.addAndMakeVisible (tapePanel);
            tapePanel.title = "tape";
            tapePanel.dot = juce::Colour (MISHKAL_ACCENT);
            tapePanel.setBounds (c.withTrimmedRight (2));
            tapePanel.toBack();
            c = c.reduced (10, 6);
            c.removeFromTop (22);
            for (auto* k : { tDrive, tMix }) k->slider.getProperties().set ("minimal", true);
            tDrive->setTitleText ("drive");
            tMix->setTitleText ("mix");
            if (MISHKAL_V35)   // warm copper, glowing with the tape's level
            {
                for (auto* k : { tDrive, tMix })
                {
                    k->slider.getProperties().set ("minimalColour", (juce::int64) 0xffff7a3c);
                    k->slider.getProperties().set ("minimalGlow", true);
                }
                tapePanel.dot = juce::Colour (0xffff7a3c);
            }
        }
        bool esh = false;
       #if MISHKAL_V60
        if (tMsDrive != nullptr)   // V60: M/S drive at the right end, for either tape model (stereo tracks only)
        {
            const bool show = isGoliath() && ! proc.isMonoTrack();
            tMsDrive->setVisible (show);
            if (show)
            {
                tMsDrive->slider.getProperties().set ("minimal", true);
                tMsDrive->slider.getProperties().set ("minimalColour", (juce::int64) 0xff5fd7ff);   // the side colour
                tMsDrive->slider.getProperties().set ("minimalGlow", true);
                tMsDrive->setTitleText ("m/s drive");
                tMsDrive->setBounds (c.removeFromRight (82));
                tMsDrive->resized();
                c.removeFromRight (2);
            }
        }
       #endif
       #if MISHKAL_V39
        {
            const int model = juce::roundToInt (proc.apvts.getRawParameterValue (id::tapeModel)->load());
            shownTapeModel = model;
            esh = model == 1;
            if (tapeModelSel.getParentComponent() != &tapeSection)
            {
                for (auto* sel : { &tapeModelSel, &eshCharSel, &eshToneSel }) tapeSection.addChildComponent (*sel);
                tapeModelSel.stepperLabel = "MODEL";
                eshCharSel.stepperLabel = "CHARACTER";
                tapeModelSel.setTooltip (MISHKAL_V40 ? "Slil: the reel-to-reel tape machine - speed, tape width, drive, noise. "
                                                     "Esh: a character tape saturator - pick a character, set Process, tilt it warm / flat / bright."
                                         : "Tape: a tape machine - speed, tape width, drive, noise. "
                                         "Esh: a character tape saturator - pick a character, set Process, tilt it warm / flat / bright.");
                eshCharSel.setTooltip ("Clear: natural, fuller, still open.  Warm: more low end, thicker low-mids.  Round: warm plus mid-range glue.  "
                                       "Glow: brighter and louder.  Ember: the loudest - hot highs and sibilance rounded off smoothly.");
                eshToneSel.setTooltip ("Overall tone after the tape: warm, flat or bright.");
            }
            tapeModelSel.setVisible (true);
            const int modelW = MISHKAL_V60 ? 98 : 110;   // V60: a little tighter, the M/S drive needs the room
            tapeModelSel.setBounds (c.removeFromLeft (modelW).withSizeKeepingCentre (modelW, 44));
            c.removeFromLeft (MISHKAL_V60 ? 8 : 10);
            tapePanel.title = esh ? "esh" : (MISHKAL_V40 ? "slil" : "tape");
            tapePanel.repaint();
            for (auto* comp : { (juce::Component*) &tapeNoiseBtn, (juce::Component*) &tapeStyleSel, (juce::Component*) &tapeWidthSel,
                                (juce::Component*) tDrive, (juce::Component*) tMix })
                comp->setVisible (! esh);
            for (auto* comp : { (juce::Component*) &eshCharSel, (juce::Component*) &eshToneSel }) comp->setVisible (esh);
            for (auto* k : eshKnobs)
            {
                k->setVisible (esh);
                k->slider.getProperties().set ("minimal", true);
                k->slider.getProperties().set ("minimalColour", (juce::int64) 0xffff7a3c);   // the tape's copper
                k->slider.getProperties().set ("minimalGlow", true);
            }
            if (esh)   // VU | character over tone | input | process | output
            {
                tapeVu.setBounds (c.removeFromLeft (MISHKAL_V60 ? 62 : 70).withSizeKeepingCentre (MISHKAL_V60 ? 62 : 70, juce::jmin (c.getHeight(), 110)));
                c.removeFromLeft (MISHKAL_V60 ? 8 : 12);
                eshKnobs[2]->setBounds (c.removeFromRight (MISHKAL_V60 ? 72 : 80));
                eshKnobs[1]->setBounds (c.removeFromRight (MISHKAL_V60 ? 80 : 96));
                eshKnobs[0]->setBounds (c.removeFromRight (MISHKAL_V60 ? 72 : 80));
                c.removeFromRight (MISHKAL_V60 ? 6 : 10);
                auto col = c.withSizeKeepingCentre (c.getWidth(), 78);
                eshCharSel.setBounds (col.removeFromTop (44));
                col.removeFromTop (6);
                eshToneSel.setBounds (col.removeFromTop (28));
            }
        }
       #endif
        if (! esh)
        {
        tapeNoiseBtn.setBounds (c.removeFromLeft (switchW + 8).withHeight (switchH));
        c.removeFromLeft (8);
        tapeVu.setBounds (c.removeFromLeft (MISHKAL_V60 ? 62 : 70).withSizeKeepingCentre (MISHKAL_V60 ? 62 : 70, juce::jmin (c.getHeight(), 110)));
        c.removeFromLeft (MISHKAL_V60 ? 8 : 12);
        tMix->setBounds (c.removeFromRight (MISHKAL_V60 ? 82 : MISHKAL_V39 ? 100 : 124));
        tDrive->setBounds (c.removeFromRight (MISHKAL_V60 ? 82 : MISHKAL_V39 ? 100 : 124));
        c.removeFromRight (MISHKAL_V60 ? 4 : 8);
        auto sel = c.withSizeKeepingCentre (c.getWidth(), 76);
        tapeStyleSel.setBounds (sel.removeFromTop (20));
        sel.removeFromTop (8);
        tapeWidthSel.setBounds (sel.removeFromTop (20));
        sel.removeFromTop (8);
        tapeOsLabel.setBounds (sel.removeFromLeft (30));
        tapeOsSel.setBounds (sel);
        }
    }

    // ---- LIMITER · OUTPUT: switches, LED metering, Ceiling / Soft, Threshold / Release, Mix / Output, OS
    {
        auto c = limSection.getContent();
        auto top = c.removeFromTop (switchH);
        top.removeFromLeft (switchW + 4);
        arc.setBounds (top.removeFromRight (switchW + 10));
       #if MISHKAL_V62
        if (isGoliath())   // V62: True Peak next to Auto Rel
        {
            top.removeFromRight (6);
            limTpBtn.setBounds (top.removeFromRight (switchW + 10));
            limTpBtn.setVisible (true);
        }
        else limTpBtn.setVisible (false);
       #endif
        limMode.setBounds (top.withSizeKeepingCentre (switchW + 10, switchH));
       #if MISHKAL_V69
        const bool maxUi = limEngine != nullptr && isGoliath();   // V69: Classic / Multi / Max LEDs instead of the two-way switch
        const bool maxOn = maxUi && limEngine->current() == 2;
        if (limEngine != nullptr)
        {
            limEngine->setVisible (maxUi);
            limMode.setVisible (! maxUi);
            if (maxUi) limEngine->setBounds (top.withSizeKeepingCentre (juce::jmin (top.getWidth(), MISHKAL_V71 ? 128 : 76), switchH - 8));
        }
        if (limStyleSel != nullptr) limStyleSel->setVisible (maxOn);
        if (lCharacter != nullptr) lCharacter->setVisible (maxOn);
        if (lTransients != nullptr) lTransients->setVisible (maxOn);
        const bool upOn = lUpward != nullptr && isGoliath();   // V70
        if (lUpward != nullptr) lUpward->setVisible (upOn);
       #else
        constexpr bool maxOn = false, upOn = false;
       #endif
        c.removeFromTop (10);
        if (MISHKAL_V19)   // V19: one panel for the whole module - title, meters, knobs, OS
        {
            if (limCtrlPanel.getParentComponent() != &limSection)
                limSection.addAndMakeVisible (limCtrlPanel);
            limCtrlPanel.title = "limiter";
            limCtrlPanel.dot = juce::Colour (0xff9b6bff);
            limCtrlPanel.setBounds (c.withTrimmedBottom (2));
            limCtrlPanel.toBack();
           #if MISHKAL_V65
            if (isGoliath())   // V65: delta at the top right of the panel, like Reso's
            {
                limDeltaBtn.setVisible (true);
                limDeltaBtn.setBounds (limCtrlPanel.getBounds().removeFromTop (30).removeFromRight (78).reduced (4, 4));
                limDeltaBtn.toFront (false);
            }
           #endif
            c = c.reduced (4, 6);
            c.removeFromTop (22);
            c.removeFromBottom (2);
            const bool stereoRow = MISHKAL_V63 && lWidth != nullptr && isGoliath() && ! proc.isMonoTrack();
            ledBlock.setBounds (c.removeFromTop (stereoRow ? 150 : 206));
            c.removeFromTop (4);
        }
        else
        {
        ledBlock.setBounds (c.removeFromTop (MISHKAL_V18 ? 212 : 250));   // V18: shorter bars give the knob panel room
        c.removeFromTop (6);
        }
        if (MISHKAL_V18 && ! MISHKAL_V19)   // V18: the controls sit on a flat rounded panel, titled like the Reso one
        {
            if (limCtrlPanel.getParentComponent() != &limSection)
                limSection.addAndMakeVisible (limCtrlPanel);
            limCtrlPanel.title = "limiter";
            limCtrlPanel.dot = MISHKAL_V17 ? juce::Colour (0xff9b6bff) : juce::Colour (MISHKAL_ACCENT);
            limCtrlPanel.setBounds (c.withTrimmedBottom (2));
            limCtrlPanel.toBack();
            c = c.reduced (4, 6);
            c.removeFromTop (24);
            c.removeFromBottom (2);
        }
        if (MISHKAL_V20)   // V20: two dropdowns side by side along the bottom
        {
            auto dd = c.removeFromBottom (26);
            if (MISHKAL_V62 && limClipSel != nullptr && isGoliath())   // V62: Soft Clip | Clipper
            {
                limSoftSel.setBounds (dd.removeFromLeft (dd.getWidth() / 2 - 3));
                dd.removeFromLeft (6);
                limClipSel->setBounds (dd);
                limClipSel->setVisible (true);
            }
            else if (MISHKAL_V33)   // oversampling moved to the top bar: Soft Clip gets the whole row
                limSoftSel.setBounds (dd.withSizeKeepingCentre (juce::jmin (dd.getWidth(), 220), dd.getHeight()));
            else
            {
            limSoftSel.setBounds (dd.removeFromLeft (dd.getWidth() / 2 - 3));
            dd.removeFromLeft (6);
            limOsSel.setBounds (dd);
            }
            c.removeFromBottom (8);
           #if MISHKAL_V69
            if (maxOn)   // V69: the Max style sits right above
            {
                auto sr = c.removeFromBottom (26);
                limStyleSel->setBounds (sr);
                c.removeFromBottom (6);
            }
           #endif
        }
        else
        {
        limOsSel.setBounds (c.removeFromBottom (20).withTrimmedLeft (34));
        limOsLabel.setBounds (limOsSel.getX() - 34, limOsSel.getY(), 32, 20);
        c.removeFromBottom (6);
        }
        const bool widthRow = MISHKAL_V63 && lWidth != nullptr && isGoliath() && ! proc.isMonoTrack();   // V63: a third row
        if (lWidth != nullptr) lWidth->setVisible (widthRow);
        if (phaseMeter != nullptr) phaseMeter->setVisible (widthRow);
        const int rowH = widthRow ? juce::jmin (98, c.getHeight() / 3) : juce::jmin (112, c.getHeight() / 2);
        const int w = c.getWidth() / 3;
        auto row = [&] (Knob* a, Knob* b, Knob* d) { auto r = c.removeFromTop (rowH); a->setBounds (r.removeFromLeft (w)); b->setBounds (r.removeFromLeft (w)); d->setBounds (r); };
        if (MISHKAL_V16)   // V16: Reso-style knobs with a velvet finish
        {
            const std::pair<Knob*, const char*> names[] { { lCeiling, "ceiling" }, { limSoftKnob, "soft" }, { lThresh, "threshold" },
                                                          { lRelease, "release" }, { lMix, "mix" } };
            for (auto& [k, n] : names)
            {
                k->slider.getProperties().set ("minimal", true);
                if (MISHKAL_V17)
                    k->slider.getProperties().set ("minimalColour", (juce::int64) 0xff9b6bff);   // violet
                k->setTitleText (n);
            }
            for (auto* k : { lCeiling, limSoftKnob, lThresh, lRelease, lMix, outputKnob })
                k->slider.getProperties().set ("velvet", ! MISHKAL_V21);   // V21: flat, like Reso
        }
       #if MISHKAL_V69
        for (auto [k, n] : { std::pair<Knob*, const char*> { lCharacter, "character" }, { lTransients, "transients" }, { lUpward, "upward" } })
            if (k != nullptr)
            {
                k->slider.getProperties().set ("minimal", true);
                k->slider.getProperties().set ("minimalColour", (juce::int64) 0xff9b6bff);
                k->setTitleText (n);
            }
       #endif
        if (MISHKAL_V20)   // soft clip left the knobs: ceiling / threshold / release, then mix / output
        {
            lRelease->setVisible (! maxOn);
            row (lCeiling, lThresh, maxOn ? lCharacter : lRelease);   // V69: Character takes Release's place in Max
            auto r = c.removeFromTop (rowH);
            const bool showLink = lLink != nullptr && isGoliath() && ! proc.isMonoTrack();   // V61: mix / link / output
            if (lLink != nullptr) lLink->setVisible (showLink);
            if (showLink)
            {
                lLink->slider.getProperties().set ("minimal", true);
                lLink->slider.getProperties().set ("minimalColour", (juce::int64) 0xff9b6bff);
                lLink->setTitleText ("link");
                lMix->setBounds (r.removeFromLeft (w));
                lLink->setBounds (r.removeFromLeft (w));
                outputKnob->setBounds (r);
                lLink->resized();
            }
            else if ((maxOn || upOn) && ! widthRow)   // V69 mono: mix / transients / (V70 upward) / output
            {
                const int n = 2 + (maxOn ? 1 : 0) + (upOn ? 1 : 0), wn = r.getWidth() / n;
                lMix->setBounds (r.removeFromLeft (wn));
                if (maxOn) { lTransients->setBounds (r.removeFromLeft (wn)); lTransients->resized(); }
                if (upOn)  { lUpward->setBounds (r.removeFromLeft (wn)); lUpward->resized(); }
                outputKnob->setBounds (r);
            }
            else
            {
            const int w2 = r.getWidth() / 2;
            lMix->setBounds (r.removeFromLeft (w2).withSizeKeepingCentre (w, rowH));
            outputKnob->setBounds (r.withSizeKeepingCentre (w, rowH));
            }
            if (widthRow)   // V63: width | phase meter
            {
                auto r3 = c.removeFromTop (rowH);
                lWidth->slider.getProperties().set ("minimal", true);
                lWidth->slider.getProperties().set ("minimalColour", (juce::int64) 0xff9b6bff);
                lWidth->setTitleText ("width");
                const int n3 = 2 + (maxOn ? 1 : 0) + (upOn ? 1 : 0), w3 = n3 == 2 ? w : r3.getWidth() / n3;   // width / transients / upward / phase
                lWidth->setBounds (r3.removeFromLeft (w3));
                lWidth->resized();
                if (maxOn)   // V69
                {
                    lTransients->setBounds (r3.removeFromLeft (w3));
                    lTransients->resized();
                }
                if (upOn)   // V70
                {
                    lUpward->setBounds (r3.removeFromLeft (w3));
                    lUpward->resized();
                }
                phaseMeter->setBounds (r3.reduced (n3 > 3 ? 2 : 8, 14));
            }
        }
        else
        {
        row (lCeiling, limSoftKnob, lThresh);
        row (lRelease, lMix, outputKnob);
        }
        if (MISHKAL_V9 && ! MISHKAL_V20) outputKnob->setBounds (outputKnob->getBounds().withTrimmedTop (8));   // room under Threshold's name
       #if MISHKAL_V71
        if (limEngine != nullptr && isGoliath())   // V71: the knobs take the engine's colour; mix, link and width keep their own
        {
            const auto modeCol = vox::ui::LimEngineBox::colourFor (limEngine->current());
            auto tint = [] (Knob* k, juce::Colour col)
            {
                if (k == nullptr) return;
                k->slider.getProperties().set ("minimalColour", (juce::int64) col.getARGB());
                k->repaint(); k->slider.repaint();
            };
            for (auto* k : { lCeiling, lThresh, lRelease, lCharacter, lTransients, lUpward, limSoftKnob }) tint (k, modeCol);
            tint (lMix,   juce::Colour (0xff7fb2ff));   // sky blue
            tint (lLink,  juce::Colour (0xffff5fa8));   // pink
            tint (lWidth, juce::Colour (0xffc6e24a));   // lime
            limCtrlPanel.dot = modeCol;
            limCtrlPanel.repaint();
            if (limStyleSel != nullptr) limStyleSel->repaint();
        }
       #endif
    }
    for (auto* lb : { &tapeOsLabel, &limOsLabel }) lb->setColour (juce::Label::textColourId, goliathLabel());
    goliathNotch.setBounds ((width - 560) / 2, displayH - 1, 560, 14);
    goliathNotch.toFront (false);
    goliathDrawer.toFront (false);
    drawerBar.toFront (false);
}

void VoxStripEditor::layoutCharEqGoliath()
{
    // Exactly the Figma panel: On | EQ list | Pre/Post, a 2 x 2 grid of big knobs, the lights + a small knob at the bottom.
    // MORE flips the same places to the EQ's other controls.
    auto c = eqSection.getContent();
    const int t = eqTypes.getSelected();
    const bool p1 = t == (int) EqType::TubeP1;
    {
        auto top = c.removeFromTop (74);
        top.removeFromLeft (60);
        eqPosition.setBounds (top.removeFromRight (56));
        top.removeFromRight (8);
        eqTypes.setVisible (false);
        eqDropdown.setVisible (true);
        eqDropdown.setBounds (top.withSizeKeepingCentre (top.getWidth(), 44));
       #if MISHKAL_V81
        eqDropdown.setBounds (top.withHeight (44).translated (0, 3));
        eqLane.setBounds (top.withTrimmedTop (52).withHeight (22));
       #endif
    }
    c.removeFromTop (12);
    eqSection.dividers.clear();
    for (auto* l : eqColumnLabels) l->setVisible (false);
    for (auto* k : { hpf, bodyF, bodyG, mudF, mudG, mudQ, presF, presG, presQ, airF, airG, silk }) k->setVisible (false);
    p1LowFreqSel.setVisible (false);
    if (MISHKAL_V2)   // V2: one screen, no knobs
    {
        charMoreBtn.setVisible (false);
       #if MISHKAL_V71
        {
            const bool showPlace = ! proc.isMonoTrack();
            for (int k = 0; k < (int) charPlaceSel.size(); ++k)
                if (charPlaceSel[(size_t) k] != nullptr) charPlaceSel[(size_t) k]->setVisible (showPlace && k == t);
            if (showPlace && charPlaceSel[(size_t) t] != nullptr)
            {
                auto row = c.removeFromBottom (24);
                c.removeFromBottom (6);
                charPlaceSel[(size_t) t]->setBounds (row.withSizeKeepingCentre (juce::jmin (row.getWidth(), 250), 22));
            }
        }
       #endif
        if (charScreen.getParentComponent() != &eqSection) eqSection.addAndMakeVisible (charScreen);
        charScreen.setVisible (true);
        charScreen.setBounds (c);
        applyHardwareLooks();
        eqSection.repaint();
        return;
    }
    charMoreBtn.setToggleState (charEqMore, juce::dontSendNotification);

    struct Slot { Knob* k = nullptr; const char* title = ""; };
    std::array<Slot, 4> grid {};
    Slot left, right;
    bool lowFreqLights = false;
    if (p1)
    {
        if (! charEqMore) { grid = { { { bodyG, "Low Boost" }, { mudG, "Low Atten" }, { presG, "High Boost" }, { airG, "High Atten" } } };
                            lowFreqLights = true; right = { presQ, "Bandwidth" }; }
        else                grid = { { { presF, "High Freq" }, { airF, "Atten Freq" }, {}, {} } };
    }
    else
    {
        if (! charEqMore) { grid = { { { bodyG, "Body" }, { mudG, "Mud" }, { presG, "Presence" }, { airG, "Air" } } };
                            left = { hpf, "High-Pass" };
                            if (t == (int) EqType::Airy) right = { silk, "Silk" }; }
        else              { grid = { { { bodyF, "Body Freq" }, { mudF, "Mud Freq" }, { presF, "Pres Freq" }, { airF, "Air Freq" } } };
                            left = { mudQ, "Mud Q" }; right = { presQ, "Pres Q" }; }
    }
    auto place = [] (const Slot& sl, juce::Rectangle<int> area)
    {
        if (sl.k == nullptr) return;
        sl.k->setVisible (true);
        sl.k->setTitleText (sl.title);
        sl.k->setBounds (area);
    };

    auto bottom = c.removeFromBottom (112);
    auto moreRow = c.removeFromBottom (24);
    charMoreBtn.setBounds (moreRow.withSizeKeepingCentre (84, 20));
    const int cellW = c.getWidth() / 2, cellH = juce::jmin (150, c.getHeight() / 2);
    for (int i = 0; i < 4; ++i)
        place (grid[(size_t) i], { c.getX() + (i % 2) * cellW, c.getY() + (i / 2) * cellH, cellW, cellH });

    auto bl = bottom.removeFromLeft (bottom.getWidth() / 2), br = bottom;
    if (lowFreqLights)
    {
        p1LowFreqSel.setVisible (true);
        p1LowFreqSel.setBounds (bl.withSizeKeepingCentre (64, 84));
    }
    else place (left, bl.withSizeKeepingCentre (juce::jmin (bl.getWidth(), 96), bl.getHeight()));
    place (right, br.withSizeKeepingCentre (juce::jmin (br.getWidth(), 96), br.getHeight()));

    applyHardwareLooks();
    eqSection.repaint();
}

void VoxStripEditor::layoutCompGoliath()
{
    auto c = compSection.getContent();
    {
        auto top = c.removeFromTop (74);
        auto leftCol = top.removeFromLeft (60);
       #if MISHKAL_V65
        compDeltaBtn.setVisible (true);   // V65: delta under the power button
        compDeltaBtn.setBounds (leftCol.removeFromBottom (27).withTrimmedRight (2));
       #else
        juce::ignoreUnused (leftCol);
       #endif
        compTypes.setVisible (false);
        compDropdown.setVisible (true);
        compDropdown.setBounds (top.removeFromTop (42));
        top.removeFromTop (5);
        auto scRow = top.removeFromTop (27);
        if (compKeySel != nullptr)   // V64: sidechain high-pass | key int / ext
        {
            compKeySel->setVisible (true);
            compKeySel->setBounds (scRow.removeFromRight (100).reduced (0, 3));
            scRow.removeFromRight (6);
        }
        scHpfSel.setBounds (scRow);
    }
    c.removeFromTop (6);
   #if MISHKAL_V81
    if (! compLane.nameFor) compLane.nameFor = [this] (int u) { return compDropdown.nameOf (u); };
    compLane.setBounds (c.removeFromTop (22).withTrimmedLeft (4));
    c.removeFromTop (4);
   #endif

    std::vector<Knob*> visible;
    switch ((CompType) shownCompType)
    {
        case CompType::Api2500:
        case CompType::SslG:
        case CompType::WeissDs1:  visible = { cThresh, cRatio, cAttack, cRelease, cMakeup }; break;   // V75
        case CompType::SplIron:   visible = { cThresh, cAttack, cRelease, cMakeup }; break;            // V75: Bias sets the ratio
        case CompType::Neve33609: visible = { cThresh, cRatio, cRelease, cTc, cMakeup }; break;       // V75: + the limiter
        case CompType::StaLevel: visible = { cThresh, cMakeup }; break;   // V52: LE-STA - Input (big) + Output; Mode / Recovery are switches
        case CompType::VariMu660:
            if (MISHKAL_V49 && ! MISHKAL_V52 && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1)
                visible = { cThresh, cMakeup };   // LE-STA: Input (the big knob) and Output; Mode / Recovery are switches
            else
                visible = { cThresh, cTc, cMakeup };
            break;
        case CompType::RComp:
            if (MISHKAL_V30 && juce::roundToInt (proc.apvts.getRawParameterValue (compId (1, "Model"))->load()) == 1)
            {
                visible = { cThresh, cMakeup };   // RVox: Threshold (how much it compresses) + Output; everything else is automatic
                break;
            }
            [[fallthrough]];
        case CompType::FetPurple:
        default:                  visible = { cThresh, cRatio, cAttack, cRelease, cMakeup }; break;
    }
    for (auto* k : { cThresh, cRatio, cAttack, cRelease, cTc, cMakeup })
        k->setVisible (std::find (visible.begin(), visible.end(), k) != visible.end());
    for (auto* k : { cGate, cBreath, cRider, cMix }) k->setVisible (true);
    if (MISHKAL_V26)   // V26: Mix in the Reso style
    {
        cMix->slider.getProperties().set ("minimal", true);
        cMix->setTitleText ("mix");
    }
    compInfo.setVisible (false);
    cThresh->slider.getProperties().set ("scaleLabels", 9);   // the big centrepiece gets the full scale
    if (MISHKAL_V58)   // V58 (Figma): the big knob in the clean gold ring style - no scale numbers, no light ring, no value box
    {
        auto& bp = cThresh->slider.getProperties();
        bp.remove ("scaleLabels");
        bp.set ("minimal", true);
        bp.set ("minimalColour", (juce::int64) 0xffffb83d);
        bp.set ("minimalGlow", true);
        bp.set ("minimalNoBox", true);
    }

    // Meter block (In / Gate / GR) on the left, the big Threshold with its gain-reduction ring beside it
    compMeter.setVisible (false);
    auto big = c.removeFromTop (MISHKAL_V23 ? 178 : 184);
    dynBlock.setBounds (big.removeFromLeft (104));
    big.removeFromLeft (4);
    cThresh->setBounds (big);
    {
        const auto sb = cThresh->slider.getBounds() + cThresh->getPosition();
        const float R = (float) juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f - 1.0f;
        const float numR = R - (R >= 64.0f ? 10.0f : 8.0f);
        grRing.knobRadius = numR - (R >= 64.0f ? 16.0f : 11.0f);
        grRing.setBounds (sb);
        grRing.toFront (false);
        grRing.setVisible (! MISHKAL_V58);   // V58: the clean ring knob has no light ring around it
    }

    // the unit's other controls
    std::vector<Knob*> rest (visible.begin() + 1, visible.end());
    if (MISHKAL_V22)   // V22: a faceplate in the style of the unit behind its own controls (rest row + Mix row)
    {
        if (compPlate.getParentComponent() != &compSection)
            compSection.addAndMakeVisible (compPlate);
        compPlate.setBounds (c.getX() - 4, c.getY() - (MISHKAL_V23 ? 0 : 2), c.getWidth() + 8, MISHKAL_V23 ? 92 + 86 + 16 : 98 + 92 + 18);
        compPlate.toBack();
        c.removeFromTop (16);   // room for the plate's legend
    }
    const bool rcUnit = shownCompType == (int) CompType::RComp;
    const bool fetUnit = shownCompType == (int) CompType::FetPurple;
    const int rcModel = juce::roundToInt (proc.apvts.getRawParameterValue (compId (1, "Model"))->load());
    const bool rvox = MISHKAL_V30 && rcUnit && rcModel == 1;
    const bool tubeUnit = MISHKAL_V49 && ! MISHKAL_V52 && shownCompType == (int) CompType::VariMu660;   // V49-V51: 660 / STA share a slot
    const bool staUnit = (MISHKAL_V52 && shownCompType == (int) CompType::StaLevel)
                      || (tubeUnit && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1);
    if (MISHKAL_V49)
    {
        shownTubeModel = juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load());
        if (tubeUnit && staUnit != threshOnStaIn)   // the big knob follows the model: Threshold (660) or Input (STA)
        {
            threshOnStaIn = staUnit;
            cThresh->bind (proc.apvts, staUnit ? compId (2, "StaIn") : compId (2, "Thresh"), staUnit ? "Input" : "Threshold");
            if (! MISHKAL_V58) cThresh->slider.getProperties().set ("scaleLabels", 9);
        }
        for (auto* sel : { &tubeModelSel, &staModeSel, &staRelSel })
            if (sel->getParentComponent() != &compSection) compSection.addChildComponent (*sel);
        if (! MISHKAL_V52 && ! compDropdown.nameFor)
            compDropdown.nameFor = [this] (int u)
            {
                return u == (int) CompType::VariMu660 && juce::roundToInt (proc.apvts.getRawParameterValue (compId (2, "Model"))->load()) == 1
                     ? juce::String ("LE-STA") : juce::String();
            };
        tubeModelSel.stepperLabel = "MODEL";
        staModeSel.stepperLabel = "MODE";
        staRelSel.stepperLabel = "RECOVERY";
        tubeModelSel.setTooltip ("LE-660: the Fairchild-style vari-mu (Threshold + six time constants). "
                                 "LE-STA: the Sta-Level-style program level amplifier - drive the Input into its fixed threshold, "
                                 "thick tube colour, a release that recovers fast then settles slowly.");
        staModeSel.setTooltip ("Single: slow attack and release - transients punch through (bass, drums). "
                               "Double: the original unit's behaviour. Triple: faster attack, still a smooth release (vocals).");
        staRelSel.setTooltip ("How quickly the gain comes back. Slower = smoother and less pumping; Fast = livelier, more movement.");
        tubeModelSel.setVisible (tubeUnit);
        staModeSel.setVisible (staUnit);
        staRelSel.setVisible (staUnit);
    }
    if (MISHKAL_V30)
    {
        shownRcModel = rcModel;
        if (rcModelSel.getParentComponent() != &compSection)
        {
            compSection.addAndMakeVisible (rcModelSel);
            rcModelSel.stepperLabel = "MODEL";
            const juce::String rcName = MISHKAL_V32 ? "LE-RC" : "RComp", rvName = MISHKAL_V32 ? "LE-RV" : "RVox";
            rcModelSel.setTooltip (rcName + ": the full compressor - ratio, attack, release, auto release, release shape and tone. "
                                   + rvName + ": the one-knob vocal leveler - just pull the Threshold down; timing and ratio are fixed and "
                                   "the level is made up automatically so the peaks stay near 0 dBFS. Output trims the result.");
            rcBehavior.stepperLabel = "RELEASE";   // shorter captions so three displays fit
            rcArc.getProperties().remove ("batTop");
            rcArc.getProperties().remove ("batBottom");
        }
        rcModelSel.setVisible (rcUnit);
        cMakeup->setTitleText (rvox ? "Output" : "Make-up");
    }
    if (MISHKAL_V31)
    {
        voxLinkBtn.setVisible (rvox);
        voxLinkLabel.setVisible (rvox);
    }
    if (MISHKAL_V27)   // V27: the LE-76 revision dropdown sits along the bottom of its plate
    {
        if (fetRevSel.getParentComponent() != &compSection)
        {
            compSection.addAndMakeVisible (fetRevSel);
            if (MISHKAL_V28) fetRevSel.stepperLabel = "REVISION";   // V28: arrows either side, like Release Shape / Tone
            else             fetRevSel.dropdownLabel = "REVISION";
            fetRevSel.setTooltip ("Which LE-76 you're hearing. Purple: the modern take (the original LE-76 sound). "
                                  "Blue Stripe: the early revision - grittier, more harmonics, a harder bite. "
                                  "Black: the low-noise revision - clean and smooth until you push it.");
        }
        fetRevSel.setVisible (fetUnit);
    }
    if (MISHKAL_V29)   // the push-buttons and the "Output" name belong to the LE-76 layout only
    {
        ratioButtons.setVisible (false);
        cMakeup->setTitleText (rvox || staUnit ? "Output" : "Make-up");
    }
    const bool masterUnit = MISHKAL_V75 && shownCompType >= (int) CompType::Api2500;
    for (auto& [u, sel] : mcompSw) sel->setVisible (masterUnit && u == shownCompType);
   #if MISHKAL_V76
    if (! masterUnit) gNeedle.setVisible (false);
   #endif
    if (masterUnit)   // V75: the unit's knobs in a row, then its switches (two columns) and Mix
    {
        auto area = c.removeFromTop (92 + 86);
        auto knobRow = area.removeFromTop (100);
        rest.push_back (cMix);   // Mix joins the knobs, so the switches get the full width
        const int w = knobRow.getWidth() / juce::jmax (1, (int) rest.size());
        knobRow = knobRow.withSizeKeepingCentre (w * (int) rest.size(), knobRow.getHeight());
        for (auto* k : rest) k->setBounds (knobRow.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (w, 100), knobRow.getHeight()));
        area.removeFromTop (2);
        area.removeFromRight (2);
        std::vector<vox::ui::ModeSelector*> sws;
        for (auto& [u, sel] : mcompSw) if (u == shownCompType) sws.push_back (sel.get());
        const int cols = sws.size() > 2 ? 2 : juce::jmax (1, (int) sws.size());
        const int rows = ((int) sws.size() + cols - 1) / cols;
        area.removeFromLeft (6);
        const int cw = area.getWidth() / cols, rh = juce::jmin (37, rows > 0 ? (area.getHeight() - 4) / rows : 37);
        for (int k = 0; k < (int) sws.size(); ++k)
            sws[(size_t) k]->setBounds (area.getX() + (k % cols) * cw + 2, area.getY() + (k / cols) * (rh + 3), cw - 6, rh);
       #if MISHKAL_V76
        if (! MISHKAL_V77 && shownCompType == (int) CompType::SslG && vox::ui::isGoliath())   // the console meter in the space under the knobs (V77: removed)
        {
            if (gNeedle.getParentComponent() != &compSection) compSection.addChildComponent (gNeedle);
            gNeedle.src = &proc.getChain().compUnitGr[(size_t) CompType::SslG];
            gNeedle.setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth() - 40, 150), juce::jmin (area.getHeight() - 10, 70)).translated (0, -6));
            gNeedle.setVisible (true);
        }
        else gNeedle.setVisible (false);
       #endif
    }
    else if (MISHKAL_V26 && (! rcUnit || rvox))   // V26: no RC switches, so Mix joins the unit's knobs in one taller row
    {
        rest.push_back (cMix);
        auto row = c.removeFromTop (92 + 86);
        if (rvox)   // V30: RVox - the model selector along the bottom, like the LE-76 revision
        {
            row.removeFromBottom (6);
            rcModelSel.setBounds (row.removeFromBottom (40).withSizeKeepingCentre (juce::jmin (row.getWidth() - 48, 260), 40));
            row.removeFromBottom (4);
        }
        if (tubeUnit || staUnit)   // V49: LE-660 / LE-STA selector along the bottom, like the LE-76 revision (V52: STA has its own unit)
        {
            if (tubeUnit)
            {
                row.removeFromBottom (6);
                tubeModelSel.setBounds (row.removeFromBottom (40).withSizeKeepingCentre (juce::jmin (row.getWidth() - 48, 260), 40));
                row.removeFromBottom (4);
            }
            if (staUnit)   // Mode and Recovery stacked on the left, then Output and Mix
            {
                auto sw = row.removeFromLeft (112).withSizeKeepingCentre (112, 84);
                staModeSel.setBounds (sw.removeFromTop (40));
                sw.removeFromTop (4);
                staRelSel.setBounds (sw.removeFromTop (40));
                row.removeFromLeft (4);
            }
        }
        if (MISHKAL_V27 && fetUnit)
        {
            if (MISHKAL_V28) row.removeFromBottom (6);   // clear of the plate's bottom edge
            fetRevSel.setBounds (MISHKAL_V28 ? row.removeFromBottom (40).withSizeKeepingCentre (juce::jmin (row.getWidth() - 48, 260), 40)
                                             : row.removeFromBottom (26).reduced (24, 0));
            row.removeFromBottom (4);
        }
        if (MISHKAL_V29 && fetUnit)   // V29: the classic FET unit's layout
        {
            // big Output | Attack over Release (small) | ratio push-buttons | Mix
            if (ratioButtons.getParentComponent() != &compSection)
            {
                compSection.addAndMakeVisible (ratioButtons);
                ratioButtons.target = &cRatio->slider;
                ratioButtons.setTooltip ("Ratio: 4, 8, 12 or 20 to 1. (The Ratio knob in other versions still sets anything in between.)");
            }
            ratioButtons.setVisible (true);
            cRatio->setVisible (false);
            row.removeFromTop (2);
            auto mixCol   = row.removeFromRight (74);
            auto ratioCol = row.removeFromRight (56);
            auto outCol   = row.removeFromLeft (juce::jmin (128, row.getWidth() / 2 + 14));
            cMakeup->setBounds (outCol);
            cMakeup->setTitleText ("Output");
            cAttack->setBounds (row.removeFromTop (row.getHeight() / 2));
            cRelease->setBounds (row);
            ratioButtons.setBounds (ratioCol.withSizeKeepingCentre (ratioCol.getWidth(), juce::jmin (ratioCol.getHeight(), 128)));
            cMix->setBounds (mixCol.withSizeKeepingCentre (mixCol.getWidth(), juce::jmin (mixCol.getHeight(), 104)));
        }
        else
        {
        if (MISHKAL_V31 && rvox)   // link (to the big Threshold) | Output | Mix
        {
            row = row.withSizeKeepingCentre (row.getWidth(), juce::jmin (row.getHeight(), 124));
            auto link = row.removeFromLeft (56);
            const int w = row.getWidth() / 2;
            cMakeup->setBounds (row.removeFromLeft (w));
            cMix->setBounds (row);
            voxLinkLabel.setFont (font (11.0f));
            voxLinkLabel.setJustificationType (juce::Justification::centred);
            voxLinkLabel.setBounds (link.getCentreX() - 22, link.getCentreY() - 26, 44, 16);
            voxLinkBtn.setBounds (link.getCentreX() - 18, link.getCentreY() - 8, 36, 20);
            voxLinkLabel.toFront (false);
            voxLinkBtn.toFront (false);
            if (MISHKAL_V48)
            {
                if (rvFlow.getParentComponent() != &compSection) compSection.addChildComponent (rvFlow);
                const auto lb = voxLinkBtn.getBounds();
                rvFlow.setBounds (lb.getCentreX() - 26, lb.getBottom() + 3, 52, 30);
                rvFlow.showValue = true;
                rvFlow.toFront (false);
            }
        }
        else
        {
        row = row.withSizeKeepingCentre (row.getWidth(), juce::jmin (row.getHeight(), 124));
        const int w = row.getWidth() / juce::jmax (1, (int) rest.size());
        row = row.withSizeKeepingCentre (w * (int) rest.size(), row.getHeight());
        for (auto* k : rest) k->setBounds (row.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (w, 124), row.getHeight()));
        }
        }
    }
    else
    {
        auto row = c.removeFromTop (MISHKAL_V23 ? 92 : 98);
        const int w = row.getWidth() / juce::jmax (1, (int) rest.size());
        row = row.withSizeKeepingCentre (w * (int) rest.size(), row.getHeight());
        for (auto* k : rest) k->setBounds (row.removeFromLeft (w));
    }

    // LE-RC switches + Mix
    const bool rc = shownCompType == (int) CompType::RComp;
    for (auto* comp : { (juce::Component*) &rcArc, (juce::Component*) &rcBehavior, (juce::Component*) &rcCharacter })
        comp->setVisible (rc && ! rvox);
    if (MISHKAL_V30 && rcUnit && ! rvox)
    {
        // MODEL display over the Auto Rel switch | Release / Tone displays | Mix
        auto row = c.removeFromTop (86);
        auto left = row.removeFromLeft (98);
        rcModelSel.setBounds (left.removeFromTop (40));
        left.removeFromTop (6);
        rcArc.setBounds (left.removeFromTop (24));
        row.removeFromLeft (4);
        cMix->setBounds (row.removeFromRight (70));
        row.removeFromRight (4);
        auto lcds = row.withSizeKeepingCentre (row.getWidth(), 84);
        rcBehavior.setBounds (lcds.removeFromTop (40));
        lcds.removeFromTop (4);
        rcCharacter.setBounds (lcds.removeFromTop (40));
    }
    else if (! (MISHKAL_V26 && (! rcUnit || rvox)))
    {
        // Auto Rel switch | two LCD selectors stacked (release shape, tone) | Mix
        auto row = c.removeFromTop (MISHKAL_V23 ? 86 : 92);
        rcArc.setBounds (row.removeFromLeft (68).withSizeKeepingCentre (64, 74));
        row.removeFromLeft (6);
        cMix->setBounds (row.removeFromRight (84));
        row.removeFromRight (6);
        auto lcds = row.withSizeKeepingCentre (row.getWidth(), 84);
        rcBehavior.setBounds (lcds.removeFromTop (40));
        lcds.removeFromTop (4);
        rcCharacter.setBounds (lcds.removeFromTop (40));
    }

    // the tools before the compressors
    if (MISHKAL_V23)   // V23: Reso-style knobs on a flat panel of their own
    {
        if (toolsPanel.getParentComponent() != &compSection)
            compSection.addAndMakeVisible (toolsPanel);
        toolsPanel.title = "vocal tools";
        toolsPanel.dot = juce::Colour (MISHKAL_ACCENT);
        c.removeFromTop (2);
        auto area = c.removeFromTop (juce::jmin (140, c.getHeight()));
        toolsPanel.setBounds (area.expanded (4, 0));
        toolsPanel.toBack();
        area.removeFromTop (20);
        area.removeFromBottom (4);
        const int w = area.getWidth() / 3;
        const std::pair<Knob*, const char*> tools[] { { cGate, "gate" }, { cBreath, "breath" }, { cRider, "rider" } };
        for (auto& [k, n] : tools)
        {
            k->slider.getProperties().set ("minimal", true);
            k->setTitleText (n);
            k->setBounds (area.removeFromLeft (w));
        }
    }
    else
    {
        auto row = c.removeFromTop (juce::jmin (106, c.getHeight()));
        const int w = row.getWidth() / 3;
        for (auto* k : { cGate, cBreath, cRider }) k->setBounds (row.removeFromLeft (w));   // Gate sits under the meter block
    }

    applyHardwareLooks();
    const auto gb = cGate->getBounds();
    gateLed.setBounds (gb.getX() + 4, gb.getY() + 2, 14, 14);
    const auto bb = cBreath->getBounds();
    breathLed.setBounds (bb.getX() + 4, bb.getY() + 2, 14, 14);
    if (MISHKAL_V23)   // the activity lights sit beside the lowercase names
    {
        gateLed.setBounds (gb.getCentreX() - 30, gb.getY() + 2, 14, 14);
        breathLed.setBounds (bb.getCentreX() - 36, bb.getY() + 2, 14, 14);
        gateLed.toFront (false);
        breathLed.toFront (false);
    }
}

//==============================================================================
LimiterScope::LimiterScope (VoxStripProcessor& p) : proc (p)
{
    setTooltip ("Output: grey is the level leaving the plugin, red is what the limiter is taking off, the dashed line is the ceiling. "
                "LUFS: S = short-term, I = integrated (click it to reset).");
    startTimerHz (30);
}

void LimiterScope::timerCallback()
{
   #if MISHKAL_V62
    if (targetLufs < 0.0f && ++targetTicks % 15 == 0) stepTarget();   // twice a second
   #endif
    level[(size_t) head] = juce::Decibels::decibelsToGain (proc.meters().outDb.load(), -100.0f);
    reduction[(size_t) head] = proc.meters().limGr.load();
    head = (head + 1) % columns;
    repaint();
}

void LimiterScope::mouseDown (const juce::MouseEvent& e)
{
   #if MISHKAL_V62
    if (e.y < 26 && e.x > getWidth() / 2)   // V62: the LUFS readout opens the loudness target menu
    {
        juce::PopupMenu m;
        m.addSectionHeader ("Loudness target");
        const float targets[] { -14.0f, -11.0f, -9.0f, -7.0f };
        const char* notes[] { "  (Spotify, YouTube, Apple)", "  (loud pop / rock)", "  (loud rap / EDM)", "  (very loud)" };
        for (int i = 0; i < 4; ++i)
            m.addItem (10 + i, juce::String ((int) targets[i]) + " LUFS" + notes[i], true, std::abs (targetLufs - targets[i]) < 0.1f);
        m.addSeparator();
        m.addItem (2, "Stop", targetLufs < 0.0f);
        m.addItem (1, "Reset the integrated reading");
        juce::Component::SafePointer<LimiterScope> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) { safe->proc.loudness.resetRequest = true; return; }
            if (r == 2) { safe->targetLufs = 0.0f; safe->targetMsg = {}; safe->repaint(); return; }
            const float t[] { -14.0f, -11.0f, -9.0f, -7.0f };
            safe->targetLufs = t[r - 10];
            safe->targetTicks = safe->targetGood = 0;
            safe->targetMsg = "play the loudest part";
            if (auto* on = safe->proc.apvts.getParameter (id::limOn)) on->setValueNotifyingHost (1.0f);   // the limiter does the work
        });
        return;
    }
   #endif
    if (e.y < 26) proc.loudness.resetRequest = true;
}

// V62: every half second, move the limiter threshold part of the way toward the target (short-term loudness),
// then stop once it has been on target for a couple of seconds. Needs music playing.
void LimiterScope::stepTarget()
{
    const float st = proc.loudness.shortTerm.load();
    if (st <= -60.0f) { targetMsg = "play the loudest part"; targetGood = 0; return; }
    const float err = targetLufs - st;
    if (std::abs (err) < 0.4f)
    {
        targetMsg = "on target";
        if (++targetGood >= 4) { targetMsg = "set: " + juce::String ((int) targetLufs) + " LUFS"; targetLufs = 0.0f; }
        return;
    }
    targetGood = 0;
    auto* th = proc.apvts.getParameter (id::limThresh);
    if (th == nullptr) return;
    const float now = th->convertFrom0to1 (th->getValue());
    const float next = juce::jlimit (-24.0f, 0.0f, now - juce::jlimit (-2.0f, 2.0f, err * 0.6f));   // lower threshold = louder
    if (std::abs (next - now) < 0.05f) { targetMsg = err > 0 ? "limit reached" : "on target"; targetLufs = err > 0 ? 0.0f : targetLufs; return; }
    th->beginChangeGesture();
    th->setValueNotifyingHost (th->convertTo0to1 (next));
    th->endChangeGesture();
    targetMsg = "aiming for " + juce::String ((int) targetLufs) + " LUFS";
}

void LimiterScope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool on = proc.apvts.getRawParameterValue (id::limOn)->load() > 0.5f;
    if (MISHKAL_V45)
    {
        const juce::Colour cyan (0xff3fd6ff), violet (0xffa070ff), pink (0xffff4fc8), hot (0xffff4a3a);
        const float life = on ? 1.0f : 0.45f;

        // title row: violet lamp, name, LUFS readout in colour
        auto title = r.removeFromTop (24.0f);
        drawGoliathLed (g, juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ title.getX() + 8.0f, title.getCentreY() }), violet, on);
        g.setColour (juce::Colour (0xffe6e6e6));
        g.setFont (screenFont (15.0f, 1).withExtraKerningFactor (0.14f));
        g.drawText (MISHKAL_V80 ? "CHOMAH" : "LIMITER", title.withTrimmedLeft (26.0f), juce::Justification::centredLeft);
        {
            auto lufs = [] (float v) { return v <= -69.0f ? juce::String ("--") : juce::String (v, 1); };
            const auto f = screenFont (12.0f, 1).withExtraKerningFactor (0.08f);
            g.setFont (f);
            const juce::String sTxt = "S " + lufs (proc.loudness.shortTerm.load()), iTxt = "I " + lufs (proc.loudness.integrated.load());
            const float iw = juce::GlyphArrangement::getStringWidth (f, iTxt) + 2.0f;
            const float sw = juce::GlyphArrangement::getStringWidth (f, sTxt) + 2.0f;
            auto tr = title;
            g.setColour (violet.brighter (0.3f).withMultipliedAlpha (life));
            g.drawText (iTxt, tr.removeFromRight (iw), juce::Justification::centredRight);
            tr.removeFromRight (12.0f);
            g.setColour (cyan.withMultipliedAlpha (life));
            g.drawText (sTxt, tr.removeFromRight (sw), juce::Justification::centredRight);
            tr.removeFromRight (8.0f);
            g.setColour (juce::Colour (0xff8e8e93));
            g.drawText ("LUFS", tr, juce::Justification::centredRight);
           #if MISHKAL_V62
            if (targetMsg.isNotEmpty())   // V62: what the loudness target is doing, under the readout
            {
                g.setFont (screenFont (10.5f, 1));
                g.setColour ((targetLufs < 0.0f ? juce::Colour (0xffffb83d) : juce::Colour (0xff55f06e)).withMultipliedAlpha (life));
                g.drawText (targetMsg, juce::Rectangle<float> (title.getX(), title.getBottom() + 1.0f, title.getWidth(), 13.0f), juce::Justification::centredRight);
            }
           #endif
        }
        r.removeFromTop (8.0f);
        auto tabs = r.removeFromBottom (20.0f);
        r.removeFromBottom (10.0f);

        auto labels = r.removeFromRight (40.0f);
        auto plot = r.reduced (0.0f, 6.0f);
        const float cy = plot.getCentreY(), half = plot.getHeight() * 0.5f;

        // a faint violet glow behind the plot
        g.setGradientFill (juce::ColourGradient (violet.withAlpha (0.10f * life), plot.getCentreX(), cy,
                                                 juce::Colours::transparentBlack, plot.getRight(), cy, true));
        g.fillRect (plot);

        g.setFont (screenFont (11.0f, 0));
        for (float a : { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f })
        {
            const float y = cy - a * half;
            g.setColour (juce::Colours::white.withAlpha (a == 0.0f ? 0.14f : 0.05f));
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            g.setColour (juce::Colour (0xff7a7a80));
            const auto txt = a == 0.0f ? juce::String ("0") : (a > 0.0f ? "+" : "-") + juce::String (std::abs (a) == 1.0f ? "1" : "0.5");
            g.drawText (txt, juce::Rectangle<float> (labels.getX(), y - 7.0f, labels.getWidth(), 14.0f), juce::Justification::centredRight);
        }

        // ceiling + a red danger zone above it
        const float ceilDb = proc.apvts.getRawParameterValue (id::limCeiling)->load();
        const float ceilA = juce::Decibels::decibelsToGain (ceilDb);
        const float yc = cy - ceilA * half, ycLow = cy + ceilA * half * 0.92f;
        g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.0f), 0.0f, plot.getY(), hot.withAlpha (0.10f * life), 0.0f, yc, false));
        g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getY(), plot.getWidth(), juce::jmax (0.0f, yc - plot.getY())));

        // the wave: a smooth mirrored shape, oldest on the left
        const float colW = plot.getWidth() / (float) columns;
        juce::Path top, wave;
        std::vector<juce::Point<float>> upper, lower;
        upper.reserve ((size_t) columns); lower.reserve ((size_t) columns);
        for (int i = 0; i < columns; ++i)
        {
            const int idx = (head + i) % columns;
            const float amp = juce::jmin (1.0f, level[(size_t) idx]);
            const float x = plot.getX() + colW * ((float) i + 0.5f);
            upper.push_back ({ x, cy - amp * half });
            lower.push_back ({ x, cy + amp * half * 0.92f });
        }
        top.startNewSubPath (upper.front());
        for (size_t i = 1; i < upper.size(); ++i) top.lineTo (upper[i]);
        wave = top;
        for (size_t i = lower.size(); i-- > 0;) wave.lineTo (lower[i]);
        wave.closeSubPath();
        juce::Path bottom;
        bottom.startNewSubPath (lower.front());
        for (size_t i = 1; i < lower.size(); ++i) bottom.lineTo (lower[i]);

        // colour by height: cyan in the middle, violet, then pink near full scale
        juce::ColourGradient heat (pink.withAlpha (0.85f * life), 0.0f, cy - half, pink.withAlpha (0.85f * life), 0.0f, cy + half, false);
        heat.addColour (0.22, violet.withAlpha (0.75f * life));
        heat.addColour (0.5, cyan.withAlpha (0.55f * life));
        heat.addColour (0.78, violet.withAlpha (0.75f * life));
        g.setGradientFill (heat);
        g.fillPath (wave);
        using PST = juce::PathStrokeType;
        juce::ColourGradient edge (pink.withAlpha (life), 0.0f, cy - half, pink.withAlpha (life), 0.0f, cy + half, false);
        edge.addColour (0.3, violet.brighter (0.3f).withAlpha (life));
        edge.addColour (0.5, cyan.withAlpha (life));
        edge.addColour (0.7, violet.brighter (0.3f).withAlpha (life));
        for (auto [wd, al] : { std::pair<float, float> { 8.0f, 0.10f }, { 4.0f, 0.22f } })
        {
            auto e2 = edge;
            for (int i = 0; i < e2.getNumColours(); ++i) e2.setColour (i, e2.getColour (i).withMultipliedAlpha (al));
            g.setGradientFill (e2);
            g.strokePath (top, PST (wd, PST::curved, PST::rounded));
            g.strokePath (bottom, PST (wd, PST::curved, PST::rounded));
        }
        g.setGradientFill (edge);
        g.strokePath (top, PST (1.4f, PST::curved, PST::rounded));
        g.strokePath (bottom, PST (1.4f, PST::curved, PST::rounded));

        // limiting: a glowing red-orange edge where it cut
        for (int i = 0; i < columns; ++i)
        {
            const int idx = (head + i) % columns;
            const float gr = reduction[(size_t) idx];
            if (gr <= 0.05f) continue;
            const float tipH = juce::jlimit (3.0f, 16.0f, gr * 2.2f);
            const auto u = upper[(size_t) i], l = lower[(size_t) i];
            const float wdt = juce::jmax (1.6f, colW * 1.1f);
            g.setColour (hot.withAlpha (0.16f * life));
            g.drawLine (u.x, u.y - 2.0f, u.x, u.y + tipH + 2.0f, wdt * 3.0f);
            g.drawLine (l.x, l.y + 2.0f, l.x, l.y - tipH - 2.0f, wdt * 3.0f);
            g.setColour (hot.brighter (0.2f).withMultipliedAlpha (life));
            g.drawLine (u.x, u.y, u.x, u.y + tipH, wdt);
            g.drawLine (l.x, l.y, l.x, l.y - tipH, wdt);
        }

        // neon ceiling lines (top and bottom)
        for (float y : { yc, ycLow })
        {
            g.setColour (hot.withAlpha (0.08f * life)); g.fillRect (plot.getX(), y - 2.0f, plot.getWidth(), 4.0f);
            juce::Path ln, dsh;
            ln.startNewSubPath (plot.getX(), y); ln.lineTo (plot.getRight(), y);
            const float dl2[] { 6.0f, 4.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dsh, ln, dl2, 2);
            g.setColour (hot.withAlpha (0.7f * life)); g.fillPath (dsh);
        }
        g.setColour (hot.brighter (0.3f).withMultipliedAlpha (life));
        g.setFont (screenFont (11.0f, 1));
        g.drawText ("CEILING " + juce::String (ceilDb, 1) + (MISHKAL_V62 && proc.apvts.getRawParameterValue (id::limTp)->load() > 0.5f ? " dBTP" : ""),
                    juce::Rectangle<float> (plot.getX() + 4.0f, yc - 16.0f, 160.0f, 13.0f), juce::Justification::centredLeft);

        // legend
        const float grNow = reduction[(size_t) ((head + columns - 1) % columns)];
        g.setFont (screenFont (12.5f, 1).withExtraKerningFactor (0.12f));
        auto t1 = tabs.withWidth (tabs.getWidth() / 3.0f);
        g.setGradientFill (juce::ColourGradient (cyan, t1.getX() + t1.getWidth() * 0.3f, 0.0f, pink, t1.getRight() - t1.getWidth() * 0.3f, 0.0f, false));
        g.drawText ("WAVE", t1, juce::Justification::centred);
        g.setColour (grNow > 0.05f ? hot.brighter (0.2f) : juce::Colour (0xff76767c));
        g.drawText ("GR " + (grNow > 0.05f ? "-" + juce::String (grNow, 1) : juce::String ("0.0")),
                    tabs.withTrimmedLeft (tabs.getWidth() / 3.0f).withWidth (tabs.getWidth() / 3.0f), juce::Justification::centred);
        g.setColour (on ? violet.brighter (0.3f) : juce::Colour (0xff76767c));
        g.drawText (on ? "ACTIVE" : "OFF", tabs.withTrimmedLeft (tabs.getWidth() * 2.0f / 3.0f), juce::Justification::centred);
        return;
    }

    // title row: lamp, name, LUFS readout
    auto title = r.removeFromTop (24.0f);
    drawGoliathLed (g, juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ title.getX() + 8.0f, title.getCentreY() }), goliathAmber(), on);
    g.setColour (juce::Colour (0xffe6e6e6));
    g.setFont (screenFont (15.0f, 1).withExtraKerningFactor (0.14f));
    g.drawText (MISHKAL_V80 ? "CHOMAH" : "LIMITER", title.withTrimmedLeft (26.0f), juce::Justification::centredLeft);
    {
        auto lufs = [] (float v) { return v <= -69.0f ? juce::String ("--") : juce::String (v, 1); };
        g.setFont (screenFont (12.0f, 1).withExtraKerningFactor (0.08f));
        g.setColour (juce::Colour (0xff8e8e93));
        g.drawText ("LUFS  S " + lufs (proc.loudness.shortTerm.load()) + "   I " + lufs (proc.loudness.integrated.load()),
                    title, juce::Justification::centredRight);
    }
    r.removeFromTop (8.0f);
    auto tabs = r.removeFromBottom (20.0f);
    r.removeFromBottom (10.0f);

    // plot with an amplitude scale on the right
    auto labels = r.removeFromRight (40.0f);
    auto plot = r.reduced (0.0f, 6.0f);
    const float cy = plot.getCentreY(), half = plot.getHeight() * 0.5f;
    g.setFont (screenFont (11.0f, 0));
    for (float a : { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f })
    {
        const float y = cy - a * half;
        g.setColour (a == 0.0f ? juce::Colour (0xff3a3a3d) : juce::Colour (0xff1e1e20));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (juce::Colour (0xff7a7a80));
        const auto txt = a == 0.0f ? juce::String ("0") : (a > 0.0f ? "+" : "-") + juce::String (std::abs (a) == 1.0f ? "1" : "0.5");
        g.drawText (txt, juce::Rectangle<float> (labels.getX(), y - 7.0f, labels.getWidth(), 14.0f), juce::Justification::centredRight);
    }

    // envelope, oldest on the left; red where the limiter cut
    const float colW = plot.getWidth() / (float) columns;
    for (int i = 0; i < columns; ++i)
    {
        const int idx = (head + i) % columns;
        const float amp = juce::jmin (1.0f, level[(size_t) idx]);
        if (amp < 0.002f) continue;
        const float x = plot.getX() + colW * ((float) i + 0.5f);
        const float h = amp * half;
        g.setColour (juce::Colour (0xffc8c8c8).withAlpha (on ? 1.0f : 0.5f));
        g.drawLine (x, cy - h, x, cy + h * 0.92f, juce::jmax (1.0f, colW * 0.75f));
        const float gr = reduction[(size_t) idx];
        if (gr > 0.05f)
        {
            const float tipH = juce::jlimit (3.0f, 14.0f, gr * 2.0f);
            g.setColour (juce::Colour (0xffe2343e));
            g.drawLine (x, cy - h, x, cy - h + tipH, juce::jmax (1.3f, colW * 0.85f));
            g.drawLine (x, cy + h * 0.92f, x, cy + h * 0.92f - tipH, juce::jmax (1.3f, colW * 0.85f));
        }
    }

    // ceiling
    const float ceilDb = proc.apvts.getRawParameterValue (id::limCeiling)->load();
    const float yc = cy - juce::Decibels::decibelsToGain (ceilDb) * half;
    juce::Path ceilLine, dash;
    ceilLine.startNewSubPath (plot.getX(), yc);
    ceilLine.lineTo (plot.getRight(), yc);
    const float dl[] { 3.0f, 4.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dash, ceilLine, dl, 2);
    g.setColour (juce::Colour (0xffe2343e).withAlpha (0.5f));
    g.fillPath (dash);
    g.setColour (juce::Colour (0xffe2343e));
    g.setFont (screenFont (11.0f, 1));
    g.drawText ("CEILING " + juce::String (ceilDb, 1), juce::Rectangle<float> (plot.getX() + 4.0f, yc - 16.0f, 140.0f, 13.0f), juce::Justification::centredLeft);

    // legend in the tab row
    const float grNow = reduction[(size_t) ((head + columns - 1) % columns)];
    g.setFont (screenFont (12.5f, 1).withExtraKerningFactor (0.12f));
    g.setColour (juce::Colour (0xffe6e6e6));
    g.drawText ("WAVE", tabs.withWidth (tabs.getWidth() / 3.0f), juce::Justification::centred);
    g.setColour (grNow > 0.05f ? juce::Colour (0xffe2343e) : juce::Colour (0xff76767c));
    g.drawText ("GR " + (grNow > 0.05f ? "-" + juce::String (grNow, 1) : juce::String ("0.0")),
                tabs.withTrimmedLeft (tabs.getWidth() / 3.0f).withWidth (tabs.getWidth() / 3.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff76767c));
    g.drawText (on ? "ACTIVE" : "OFF", tabs.withTrimmedLeft (tabs.getWidth() * 2.0f / 3.0f), juce::Justification::centred);
}

//==============================================================================
MeterBridge::MeterBridge (VoxStripProcessor& p) : proc (p)
{
    setTooltip ("Input and output peak (grey bar, red hold line) and average level (white line), and gain reduction: "
                "C = compressor, L = limiter. Click to reset the readouts.");
}

void MeterBridge::timerCallback()
{
    auto& m = proc.meters();
    auto upd = [] (std::atomic<float> (&src)[2], std::array<float, 2>& pk, std::array<float, 2>& ms, std::array<float, 2>& avg,
                   std::array<float, 2>& hold, std::array<int, 2>& holdT, std::array<float, 2>& mx)
    {
        for (size_t c = 0; c < 2; ++c)
        {
            const float lin = src[c].exchange (0.0f);
            const float db = juce::Decibels::gainToDecibels (lin, -100.0f);
            pk[c] = db > pk[c] ? db : juce::jmax (db, pk[c] - 0.9f);
            ms[c] += 0.12f * (lin * lin * 0.5f - ms[c]);             // ~0.3 s average
            avg[c] = juce::Decibels::gainToDecibels (std::sqrt (ms[c]), -100.0f);
            if (db >= hold[c]) { hold[c] = db; holdT[c] = 45; }
            else if (--holdT[c] < 0) hold[c] = juce::jmax (db, hold[c] - 0.6f);
            mx[c] = juce::jmax (mx[c], db);
        }
    };
    upd (m.inPeak, inPk, inMs, inAvg, inHold, inHoldT, inMax);
    upd (m.outPeak, outPk, outMs, outAvg, outHold, outHoldT, outMax);
    const float lg = m.limGr.load(), cg = m.compGr.load();
    limGr = lg > limGr ? lg : limGr * 0.85f + lg * 0.15f;
    compGr = cg > compGr ? cg : compGr * 0.85f + cg * 0.15f;
    grMax = juce::jmax (grMax, limGr);
    repaint();
}

void MeterBridge::mouseDown (const juce::MouseEvent&)
{
    inMax = outMax = { -100.0f, -100.0f };
    grMax = 0.0f;
    repaint();
}

AutoInputButton::AutoInputButton (VoxStripProcessor& p) : proc (p)
{
    setTooltip ("Auto input: click, then play the loudest part of the track for a few seconds. It sets Input so the strip is hit "
                "at about -18 dBFS average with the peaks under -6 dBFS - the level the presets are made for. Click again to stop.");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AutoInputButton::toggle()
{
    if (listening) { listening = false; text = "auto"; stopTimer(); repaint(); return; }
    auto& m = proc.meters();
    m.rawEnergy.store (0.0f); m.rawSamples.store (0); m.rawPeak.store (0.0f);
    rmsDb.clear(); peak = 0.0f; ticks = idleTicks = 0;
    listening = true;
    text = "play...";
    startTimerHz (10);
    repaint();
}

void AutoInputButton::timerCallback()
{
    if (! listening)
    {
        if (++idleTicks > 40) { text = "auto"; stopTimer(); repaint(); }   // the result shows for 4 s
        return;
    }
    auto& m = proc.meters();
    const float e = m.rawEnergy.exchange (0.0f);
    const int n = m.rawSamples.exchange (0);
    const float pk = m.rawPeak.exchange (0.0f);
    ++ticks;
    if (n > 0)
    {
        const float db = juce::Decibels::gainToDecibels (std::sqrt (e / (float) n), -120.0f);
        if (db > -55.0f) { rmsDb.push_back (db); peak = juce::jmax (peak, pk); }   // ignore silence between phrases
    }
    const int secs = (int) rmsDb.size() / 10;
    text = rmsDb.empty() ? "play..." : "listening " + juce::String (juce::jmax (0, 4 - secs));
    if (rmsDb.size() >= 40 || ticks >= 150) finish();   // 4 s of sound, or give up after 15 s
    repaint();
}

void AutoInputButton::finish()
{
    listening = false;
    idleTicks = 0;
    if (rmsDb.size() < 10) { text = "no signal"; repaint(); return; }
    // the louder half of the phrases decides (quiet tails would pull it too high)
    std::sort (rmsDb.begin(), rmsDb.end());
    double sum = 0.0; int cnt = 0;
    for (size_t i = rmsDb.size() / 2; i < rmsDb.size(); ++i) { sum += std::pow (10.0, rmsDb[i] / 10.0); ++cnt; }
    const float avgDb = (float) (10.0 * std::log10 (sum / juce::jmax (1, cnt)));
    const float peakDb = juce::Decibels::gainToDecibels (peak, -120.0f);
    const float gain = juce::jlimit (-24.0f, 24.0f, juce::jmin (-18.0f - avgDb, -6.0f - peakDb));
    if (auto* p = proc.apvts.getParameter (id::inputGain))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (gain));
        p->endChangeGesture();
    }
    text = "set " + juce::String (gain > 0 ? "+" : "") + juce::String (gain, 1);
    repaint();
}

void AutoInputButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto col = juce::Colour ((juce::uint32) MISHKAL_NEON);
    const bool hot = listening || text != "auto";
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (hot ? col.withAlpha (0.9f) : juce::Colour (0xff3a3b40));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    if (listening)
    {
        g.setColour (col.withAlpha (0.18f + 0.12f * (float) std::sin (ticks * 0.6)));
        g.fillRoundedRectangle (r.reduced (1.5f), r.getHeight() * 0.5f);
    }
    g.setColour (hot ? col.brighter (0.3f) : juce::Colour (0xff9da0a8));
    g.setFont (font (10.5f, hot));
    g.drawText (text, r, juce::Justification::centred);
}

void LimEngineBox::paint (juce::Graphics& g)
{
    static const char* names[] { MISHKAL_V71 ? "Goliath Classic" : "Classic", MISHKAL_V71 ? "Goliath Multi" : "Multi", MISHKAL_V71 ? "Goliath XL" : "Max" };
    const int cur = current();
    const float rowH = (float) getHeight() / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        auto row = juce::Rectangle<float> (0.0f, rowH * (float) i, (float) getWidth(), rowH);
        const float d = juce::jmin (13.0f, rowH - 4.0f);
        drawGoliathLed (g, juce::Rectangle<float> (d, d).withCentre ({ d * 0.5f + 1.0f, row.getCentreY() }), MISHKAL_V71 ? colourFor (i) : goliathAmber(), i == cur);
        g.setColour (i == cur ? goliathLegend() : goliathLabel().withAlpha (0.85f));
        g.setFont (font (13.0f));
        g.drawText (names[i], row.withTrimmedLeft (d + 8.0f), juce::Justification::centredLeft);
    }
}

void PhaseMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff9da0a8));
    g.setFont (font (12.5f));
    g.drawText ("phase", r.removeFromTop (17.0f), juce::Justification::centred);
    auto labels = r.removeFromBottom (14.0f);
    auto bar = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (16.0f, r.getHeight()));
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (bar, 4.0f);
    // red (out of phase) on the left half, green toward +1
    juce::ColourGradient grad (juce::Colour (0xffff3d3d).withAlpha (0.35f), bar.getX(), 0.0f, juce::Colour (0xff55f06e).withAlpha (0.35f), bar.getRight(), 0.0f, false);
    grad.addColour (0.5, juce::Colour (0xffffb43c).withAlpha (0.25f));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bar.reduced (2.0f), 3.0f);
    const float cx = bar.getCentreX();
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawVerticalLine ((int) cx, bar.getY() + 2.0f, bar.getBottom() - 2.0f);
    auto xFor = [&] (float v) { return bar.getX() + 3.0f + (juce::jlimit (-1.0f, 1.0f, v) + 1.0f) * 0.5f * (bar.getWidth() - 6.0f); };
    const auto col = shown < 0.0f ? juce::Colour (0xffff3d3d) : shown < 0.3f ? juce::Colour (0xffffb43c) : juce::Colour (0xff55f06e);
    g.setColour (col.withAlpha (0.35f));
    g.fillRoundedRectangle (juce::Rectangle<float> (9.0f, bar.getHeight() + 2.0f).withCentre ({ xFor (shown), bar.getCentreY() }), 3.0f);
    g.setColour (col);
    g.fillRoundedRectangle (juce::Rectangle<float> (4.0f, bar.getHeight() - 2.0f).withCentre ({ xFor (shown), bar.getCentreY() }), 2.0f);
    g.setColour (juce::Colour (0xffff3d3d).withAlpha (0.8f));   // the lowest it has been lately
    g.fillRect (juce::Rectangle<float> (1.5f, bar.getHeight() - 6.0f).withCentre ({ xFor (low), bar.getCentreY() }));
    g.setColour (juce::Colour (0xff7a7a80));
    g.setFont (font (10.5f));
    g.drawText ("-1", labels, juce::Justification::centredLeft);
    g.drawText ("0", labels, juce::Justification::centred);
    g.drawText ("+1", labels, juce::Justification::centredRight);
}

bool MeterBridge::isMono() const { return MISHKAL_V60 && proc.isMonoTrack(); }

void MeterBridge::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto fmt = [] (float db) { return db <= -99.0f ? juce::String ("-inf") : juce::String (db, 1); };

    // readouts: PEAK (highest since the last click) and AVG, per bar pair
    auto head = r.removeFromTop (40.0f);
    const float xIn = 40.0f, xOut = 126.0f, xGr = 214.0f, bw = 28.0f, gap = 4.0f, gw = 22.0f;
    const bool mono = isMono();   // V60: a mono track gets one wide bar per meter
    g.setFont (screenFont (12.5f, 1).withExtraKerningFactor (0.08f));
    g.setColour (juce::Colour (0xff8e8e93));
    g.drawText ("PEAK", juce::Rectangle<float> (0.0f, head.getY(), 36.0f, 18.0f), juce::Justification::centredRight);
    g.setColour (juce::Colour (0xffe6e6e6));
    g.drawText ("AVG", juce::Rectangle<float> (0.0f, head.getY() + 20.0f, 36.0f, 18.0f), juce::Justification::centredRight);
    g.setFont (screenFont (12.5f, 0));
    auto pair = [&] (float x, const std::array<float, 2>& a, const std::array<float, 2>& b, float w)
    {
        for (int c = 0; c < (mono ? 1 : 2); ++c)
        {
            auto cell = juce::Rectangle<float> (x + (float) c * (w + gap) - 6.0f, head.getY(), (mono ? w * 2.0f + gap : w) + 12.0f, 18.0f);
            g.setColour (juce::Colour (0xff8e8e93));
            g.drawText (fmt (a[(size_t) c]), cell, juce::Justification::centred);
            g.setColour (juce::Colour (0xffe6e6e6));
            g.drawText (fmt (b[(size_t) c]), cell.translated (0.0f, 20.0f), juce::Justification::centred);
        }
    };
    pair (xIn, inMax, inAvg, bw);
    pair (xOut, outMax, outAvg, bw);
    g.setColour (juce::Colour (0xff8e8e93));
    g.drawText (juce::String (compGr, 1), juce::Rectangle<float> (xGr - 6.0f, head.getY(), gw + 12.0f, 18.0f), juce::Justification::centred);
    g.drawText (juce::String (limGr, 1), juce::Rectangle<float> (xGr + gw + gap - 6.0f, head.getY(), gw + 12.0f, 18.0f), juce::Justification::centred);
    r.removeFromTop (6.0f);

    // labels at the bottom
    auto groups = r.removeFromBottom (18.0f);
    auto lr = r.removeFromBottom (18.0f);
    r.removeFromBottom (4.0f);
    const float top = r.getY() + 6.0f, bot = r.getBottom();
    auto yFor = [&] (float db) { return top + (6.0f - juce::jlimit (-30.0f, 6.0f, db)) / 36.0f * (bot - top); };

    // scale + grid
    g.setFont (screenFont (11.5f, 0));
    for (float db : { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f })
    {
        const float y = yFor (db);
        g.setColour (db == 0.0f ? juce::Colour (0xff5a1e22) : juce::Colour (0xff1e1e20));
        g.drawHorizontalLine ((int) y, 32.0f, xGr - 12.0f);
        g.setColour (db == 0.0f ? juce::Colour (0xffe2343e) : juce::Colour (0xff7a7a80));
        g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (0.0f, y - 7.0f, 28.0f, 14.0f), juce::Justification::centredRight);
    }
    auto bar = [&] (float x, float pk, float avg, float hold)
    {
        const float bw = mono ? 28.0f * 2.0f + 4.0f : 28.0f;   // (shadows the pair width on mono tracks)
        const float y0 = yFor (-30.0f);
       #if MISHKAL_V58
        // V58 (Figma): the same glowing green -> yellow -> red bar as the module meters, with a white-hot top edge
        {
            const juce::Colour green (0xff55f06e), yellow (0xffffb43c), red (0xffff3d3d);
            juce::ColourGradient grad (green, 0.0f, y0, red, 0.0f, yFor (6.0f), false);
            auto at = [&] (float db) { return (double) juce::jlimit (0.0f, 1.0f, (y0 - yFor (db)) / juce::jmax (1.0f, y0 - yFor (6.0f))); };
            grad.addColour (at (-12.0f), green);
            grad.addColour (at (-9.0f), yellow);
            grad.addColour (at (-3.0f), yellow);
            grad.addColour (at (-1.0f), red);
            g.setColour (juce::Colour (0xff060607));
            g.fillRect (juce::Rectangle<float> (x, yFor (6.0f), bw, y0 - yFor (6.0f)));
            if (pk > -30.0f)
            {
                const auto b = juce::Rectangle<float> (x, yFor (pk), bw, y0 - yFor (pk));
                const juce::Colour tip = pk >= -1.0f ? red : pk >= -9.0f ? yellow : green;
                for (auto [ex, a] : { std::pair<float, float> { 5.0f, 0.06f }, { 3.0f, 0.12f }, { 1.5f, 0.22f } })
                {
                    auto gg = grad;
                    for (int i = 0; i < gg.getNumColours(); ++i) gg.setColour (i, gg.getColour (i).withAlpha (a));
                    g.setGradientFill (gg);
                    g.fillRoundedRectangle (b.expanded (ex, ex * 0.6f), 2.0f + ex);
                }
                g.setGradientFill (grad);
                g.fillRoundedRectangle (b, 2.0f);
                juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.0f), b.getX(), 0.0f, juce::Colours::white.withAlpha (0.0f), b.getRight(), 0.0f, false);
                gloss.addColour (0.45, juce::Colours::white.withAlpha (0.25f));
                g.setGradientFill (gloss);
                g.fillRoundedRectangle (b, 2.0f);
                const auto cap = juce::Rectangle<float> (b.getX(), b.getY(), bw, 2.0f);
                g.setColour (tip.withAlpha (0.45f)); g.fillRoundedRectangle (cap.expanded (3.0f), 3.0f);
                g.setColour (tip.interpolatedWith (juce::Colours::white, 0.6f)); g.fillRoundedRectangle (cap, 1.0f);
            }
            if (hold > -30.0f)   // peak hold: a thin red line
            {
                g.setColour (juce::Colour (0xffe2343e));
                g.fillRect (juce::Rectangle<float> (x, yFor (hold) - 1.0f, bw, 2.0f));
            }
            juce::ignoreUnused (avg);
            return;
        }
       #endif
        if (pk > -30.0f)
        {
            g.setColour (juce::Colour (0xff3a3a3c));
            g.fillRect (juce::Rectangle<float> (x, yFor (pk), bw, y0 - yFor (pk)));
        }
        if (avg > -30.0f)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcfcfcf), 0.0f, yFor (avg), juce::Colour (0xff8a8a8a), 0.0f, y0, false));
            g.fillRect (juce::Rectangle<float> (x, yFor (avg), bw, y0 - yFor (avg)));
            g.setColour (juce::Colours::white);
            g.fillRect (juce::Rectangle<float> (x, yFor (avg) - 1.0f, bw, 2.0f));
        }
        if (hold > -30.0f)
        {
            g.setColour (juce::Colour (0xffe2343e));
            g.fillRect (juce::Rectangle<float> (x, yFor (hold) - 1.0f, bw, 2.0f));
        }
    };
    for (int c = 0; c < (mono ? 1 : 2); ++c)
    {
        bar (xIn + (float) c * (bw + gap), inPk[(size_t) c], inAvg[(size_t) c], inHold[(size_t) c]);
        bar (xOut + (float) c * (bw + gap), outPk[(size_t) c], outAvg[(size_t) c], outHold[(size_t) c]);
    }

    // gain reduction: hangs down from the top, scale 0 to -30 on the right
    const float gTop = top, gBot = bot;
    auto gy = [&] (float gr) { return gTop + juce::jlimit (0.0f, 30.0f, gr) / 30.0f * (gBot - gTop); };
    g.setFont (screenFont (11.5f, 0));
    for (float d : { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f, 30.0f })
    {
        g.setColour (juce::Colour (0xff1e1e20));
        g.drawHorizontalLine ((int) gy (d), xGr - 6.0f, r.getRight() - 26.0f);
        g.setColour (juce::Colour (0xff7a7a80));
        g.drawText (d == 0.0f ? juce::String ("0") : "-" + juce::String ((int) d), juce::Rectangle<float> (r.getRight() - 26.0f, gy (d) - 7.0f, 26.0f, 14.0f),
                    juce::Justification::centredRight);
    }
    g.setColour (juce::Colour (0xffe2343e));
    if (compGr > 0.05f) g.fillRect (juce::Rectangle<float> (xGr, gTop, gw, gy (compGr) - gTop + 3.0f));
    if (limGr > 0.05f)  g.fillRect (juce::Rectangle<float> (xGr + gw + gap, gTop, gw, gy (limGr) - gTop + 3.0f));

    // labels
    g.setFont (screenFont (12.0f, 0));
    g.setColour (juce::Colour (0xff7a7a80));
    if (mono)
        for (auto [x, w, t] : { std::tuple<float, float, const char*> { xIn, bw * 2.0f + gap, "MONO" }, { xOut, bw * 2.0f + gap, "MONO" },
                                { xGr, gw, "C" }, { xGr + gw + gap, gw, "L" } })
            g.drawText (t, juce::Rectangle<float> (x, lr.getY(), w, lr.getHeight()), juce::Justification::centred);
    else
    for (auto [x, w, t] : { std::tuple<float, float, const char*> { xIn, bw, "L" }, { xIn + bw + gap, bw, "R" }, { xOut, bw, "L" }, { xOut + bw + gap, bw, "R" },
                            { xGr, gw, "C" }, { xGr + gw + gap, gw, "L" } })
        g.drawText (t, juce::Rectangle<float> (x, lr.getY(), w, lr.getHeight()), juce::Justification::centred);
    g.setFont (screenFont (13.0f, 1).withExtraKerningFactor (0.12f));
    g.setColour (juce::Colour (0xffe6e6e6));
    g.drawText ("INPUT", juce::Rectangle<float> (xIn - 10.0f, groups.getY(), bw * 2.0f + gap + 20.0f, groups.getHeight()), juce::Justification::centred);
    g.drawText ("OUTPUT", juce::Rectangle<float> (xOut - 10.0f, groups.getY(), bw * 2.0f + gap + 20.0f, groups.getHeight()), juce::Justification::centred);
    g.drawText ("GAIN RED", juce::Rectangle<float> (xGr - 14.0f, groups.getY(), gw * 2.0f + gap + 28.0f, groups.getHeight()), juce::Justification::centred);
}

//==============================================================================
// V4: gain reduction as a solid red bar hanging from the top (like GAIN RED on the display) instead of LEDs
static void drawGrBar (juce::Graphics& g, juce::Rectangle<float> slot, float grDb, float maxDb)
{
    g.setColour (juce::Colour (0xff060607));
    g.fillRect (slot);
    g.setColour (juce::Colour (0xff1e1e20));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (slot.getY() + slot.getHeight() * (float) i / 4.0f), slot.getX(), slot.getRight());
    const float frac = juce::jlimit (0.0f, 1.0f, grDb / maxDb);
    if (frac > 0.002f)
    {
        g.setColour (juce::Colour (0xffe2343e));
        g.fillRect (slot.withHeight (juce::jmax (2.0f, slot.getHeight() * frac)));
    }
}

// V6: input/output level as a solid bar rising from the bottom, coloured by height green -> yellow -> red
// (the same zones the LEDs use), in the same style as the red gain-reduction bar
static void drawLevelBar (juce::Graphics& g, juce::Rectangle<float> slot, float levelDb, float floorDb, float yellowDb, float redDb)
{
    g.setColour (juce::Colour (0xff060607));
    g.fillRect (slot);
    g.setColour (juce::Colour (0xff1e1e20));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (slot.getY() + slot.getHeight() * (float) i / 4.0f), slot.getX(), slot.getRight());
    auto toFrac = [floorDb] (float db) { return juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb); };
    const float frac = toFrac (levelDb);
    if (frac <= 0.002f)
        return;
    const juce::Colour green (0xff55f06e), yellow (0xffffb43c), red (0xffff3d3d);
    juce::ColourGradient grad (green, 0.0f, slot.getBottom(), red, 0.0f, slot.getY(), false);
    const float fy = toFrac (yellowDb), fr = toFrac (redDb);
    grad.addColour (juce::jmax (0.0, (double) fy - 0.12), green);
    grad.addColour (fy, yellow);
    grad.addColour (juce::jmax ((double) fy, (double) fr - 0.03), yellow);
    grad.addColour (juce::jmin (1.0, (double) fr + 0.02), red);
   #if MISHKAL_V50
    // V50: a modern glow - soft bloom around the bar, a rounded glossy body and a bright, glowing top edge (same colours)
    {
        const auto bar = slot.withTop (slot.getBottom() - juce::jmax (2.0f, slot.getHeight() * frac));
        const juce::Colour tipCol = levelDb >= redDb ? red : levelDb >= yellowDb ? yellow : green;
        for (auto [ex, a] : { std::pair<float, float> { 5.0f, 0.06f }, { 3.0f, 0.12f }, { 1.5f, 0.22f } })
        {
            auto glowGrad = grad;
            for (int i = 0; i < glowGrad.getNumColours(); ++i) glowGrad.setColour (i, glowGrad.getColour (i).withAlpha (a));
            g.setGradientFill (glowGrad);
            g.fillRoundedRectangle (bar.expanded (ex, ex * 0.6f), 2.0f + ex);
        }
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar, 2.0f);
        // glossy centre line: light runs down the middle of the bar
        {
            juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.0f), bar.getX(), 0.0f, juce::Colours::white.withAlpha (0.0f), bar.getRight(), 0.0f, false);
            gloss.addColour (0.45, juce::Colours::white.withAlpha (0.28f));
            g.setGradientFill (gloss);
            g.fillRoundedRectangle (bar, 2.0f);
        }
        // the top edge: a white-hot cap with a bloom in the zone's colour
        const auto cap = juce::Rectangle<float> (bar.getX(), bar.getY(), bar.getWidth(), 2.0f);
        g.setColour (tipCol.withAlpha (0.45f));
        g.fillRoundedRectangle (cap.expanded (3.0f, 3.0f), 3.0f);
        g.setColour (tipCol.interpolatedWith (juce::Colours::white, 0.6f));
        g.fillRoundedRectangle (cap, 1.0f);
        return;
    }
   #endif
    g.setGradientFill (grad);
    g.fillRect (slot.withTop (slot.getBottom() - juce::jmax (2.0f, slot.getHeight() * frac)));
}

void LedMeterBlock::paint (juce::Graphics& g)
{
    // Exactly the Figma "LED Metering" block: a narrow black panel, five columns (In L, In R, GR, Out L, Out R)
    // of fourteen 10 px LEDs with 4 px gaps, flat dark when off, glowing when lit; In / GR / Out underneath.
    constexpr int leds = 14;
    constexpr float d = 10.0f, gapY = MISHKAL_V18 ? 1.0f : 4.0f, colW = 16.0f, gapX = 6.0f, padX = 10.0f, padY = 8.0f, headH = 14.0f, headGap = 4.0f;
    const float blockW = padX * 2.0f + colW * 5.0f + gapX * 4.0f;
    float blockH = padY * 2.0f + headH + headGap + d * leds + gapY * (leds - 1);
    if (MISHKAL_V63 && MISHKAL_V6) blockH = juce::jmin (blockH, (float) getHeight() - 22.0f);   // V63: shorter bars, room for width / phase
    auto all = getLocalBounds().toFloat();
    auto block = juce::Rectangle<float> (blockW, blockH).withCentre ({ all.getCentreX(), all.getY() + blockH * 0.5f });
    auto names = juce::Rectangle<float> (block.getX() - 20.0f, block.getBottom() + 4.0f, blockW + 40.0f, 18.0f);

    g.setColour (juce::Colour (0xff0d0d0e));
    g.fillRoundedRectangle (block, 3.0f);

    static constexpr float thresholds[leds] { -42.0f, -36.0f, -30.0f, -26.0f, -22.0f, -18.0f, -15.0f, -12.0f, -9.0f, -6.0f, -4.0f, -2.0f, -0.5f, 0.0f };
    static constexpr float grSteps[leds] { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 15.0f, 18.0f, 21.0f, 24.0f, 30.0f };
    const juce::Colour onCol[3]  { juce::Colour (0xff55f06e), juce::Colour (0xffffb43c), juce::Colour (0xffff3d3d) };
    const juce::Colour offCol[3] { juce::Colour (0xff1e3322), juce::Colour (0xff3a3224), juce::Colour (0xff3a1d1d) };
    const char* heads[] { "L", "R", "GR", "L", "R" };

    const bool mono = MISHKAL_V60 && MISHKAL_V6 && bridge.isMono();   // V60: one wide In and Out bar on a mono track
    for (int col = 0; col < 5; ++col)
    {
        if (mono && (col == 1 || col == 4)) continue;
        const float x = block.getX() + padX + (float) col * (colW + gapX);
        const float cx = x + colW * 0.5f;
        const float wCol = mono && (col == 0 || col == 3) ? colW * 2.0f + gapX : colW;
        g.setColour (juce::Colour (0xffdcdcd6));
        g.setFont (font (11.0f, true));
        g.drawText (mono && col != 2 ? "M" : heads[col], juce::Rectangle<float> (x - 6.0f, block.getY() + padY, wCol + 12.0f, headH), juce::Justification::centred);

        if (MISHKAL_V5 && col == 2)
        {
            const float top = block.getY() + padY + headH + headGap;
            drawGrBar (g, { x + 1.0f, top, colW - 2.0f, block.getBottom() - padY - top }, bridge.limGr, 30.0f);
            continue;
        }
        if (MISHKAL_V6)
        {
            const float top = block.getY() + padY + headH + headGap;
            const float lvl = col < 2 ? bridge.inPk[(size_t) col] : bridge.outPk[(size_t) col - 3];
            drawLevelBar (g, { x + 1.0f, top, wCol - 2.0f, block.getBottom() - padY - top }, lvl, -48.0f, -9.0f, -2.0f);
            continue;
        }
        for (int i = 0; i < leds; ++i)   // i = 0 is the bottom LED
        {
            const float cy = block.getBottom() - padY - d * 0.5f - (float) i * (d + gapY);
            int tone;
            bool lit;
            if (col == 2)   // gain reduction lights from the top down, all red
            {
                tone = 2;
                lit = bridge.limGr >= grSteps[leds - 1 - i];
            }
            else
            {
                const float lvl = col < 2 ? bridge.inPk[(size_t) col] : bridge.outPk[(size_t) col - 3];
                tone = i >= 12 ? 2 : i >= 9 ? 1 : 0;
                lit = lvl >= thresholds[i];
            }
            const auto dot = juce::Rectangle<float> (d, d).withCentre ({ cx, cy });
            if (lit)
            {
                g.setColour (onCol[tone].withAlpha (0.22f));
                g.fillEllipse (dot.expanded (5.0f));
                g.setColour (onCol[tone].withAlpha (0.45f));
                g.fillEllipse (dot.expanded (2.0f));
                g.setColour (onCol[tone]);
            }
            else
                g.setColour (offCol[tone]);
            g.fillEllipse (dot);
        }
    }

    // In / GR / Out under the block, spaced evenly like the design
    g.setFont (font (13.0f));
    g.setColour (juce::Colour (0xff7e7e7a));
    auto colX = [&] (float c) { return block.getX() + padX + c * (colW + gapX) + colW * 0.5f; };
    g.drawText ("In",  juce::Rectangle<float> (60.0f, names.getHeight()).withCentre ({ colX (0.5f), names.getCentreY() }), juce::Justification::centred);
    g.drawText ("GR",  juce::Rectangle<float> (40.0f, names.getHeight()).withCentre ({ colX (2.0f), names.getCentreY() }), juce::Justification::centred);
    g.drawText ("Out", juce::Rectangle<float> (60.0f, names.getHeight()).withCentre ({ colX (3.5f), names.getCentreY() }), juce::Justification::centred);
}

//==============================================================================
juce::Rectangle<int> VoxStripEditor::drawerBounds (bool open) const
{
    using namespace goliath;
    if (MISHKAL_V36) return { 0, height, width, open ? 220 : 0 };   // below the plugin, under the bar
    const int top = open ? height - barH - drawerH : height - barH;
    return { 0, top, width, drawerH };
}

void VoxStripEditor::setDrawerOpen (bool open, bool animate)
{
    drawerOpen = open;
    proc.apvts.state.setProperty ("drawerOpen", open, nullptr);   // remembered with the session
    drawerBar.open = open;
    drawerBar.repaint();
    if (MISHKAL_V36)   // the window grows / shrinks to make room below; nothing gets covered
    {
        juce::ignoreUnused (animate);
        applyBaseSize();
        goliathDrawer.setBounds (drawerBounds (open));
        goliathDrawer.setVisible (open);
        return;
    }
    const auto target = drawerBounds (open);
    if (animate && isShowing())
        juce::Desktop::getInstance().getAnimator().animateComponent (&goliathDrawer, target, 1.0f, 220, false, 0.2, 0.6);
    else
        goliathDrawer.setBounds (target);
}

void DrawerBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRect (r);
    g.setColour (juce::Colour (0xff2a2a2d));
    g.drawHorizontalLine (0, r.getX(), r.getRight());

   #if MISHKAL_V42
    {
        // the signal path, in the order the audio really flows; each stage lit when it is switched in
        struct Stage { const char* name; const char* onId; juce::uint32 colour; int action; int mid = -1; };
        const bool post = proc.apvts.getRawParameterValue (id::eqAfterComp)->load() > 0.5f;
        const Stage charEq { "CHAR EQ", id::eqOn, 0xffffb43c, 0, 0 }, dyn { MISHKAL_V80 ? "LACHATZ" : "DYNAMICS", id::compOn, 0xffffb43c, 0, 1 };
        std::vector<Stage> list { { "IN", nullptr, 0xffffe7a8, 0 }, { "1964 PK", id::peakCatch, 0xff9b6bff, 0 }, { "RESO", id::resoOn, 0xffffb83d, 0 },
                                  { "TAHOR", id::cleanOn, 0xffffb83d, 0 }, post ? dyn : charEq, { "pre / post", nullptr, 0, 2 }, post ? charEq : dyn,
                                  { MISHKAL_V80 ? "LACHASH" : "DE-ESS", id::dsOn, 0xffffb83d, 0, 2 }, { "TONE", id::flowOn, 0xffff5fa8, 1, 3 }, { "TAPE", id::tapeOn, 0xffff7a3c, 1, 4 },
                                  { MISHKAL_V80 ? "CHOMAH" : "LIMITER", id::limOn, 0xff9b6bff, 0 }, { "OUT", nullptr, 0xffffe7a8, 0 } };
       #if MISHKAL_V68
        if (juce::roundToInt (proc.apvts.getRawParameterValue (id::midOrder)->load()) > 0)   // V68: a custom order + a reset pill
        {
            const Stage mids[5] { charEq, dyn, { MISHKAL_V80 ? "LACHASH" : "DE-ESS", id::dsOn, 0xffffb83d, 0, 2 }, { "TONE", id::flowOn, 0xffff5fa8, 1, 3 }, { "TAPE", id::tapeOn, 0xffff7a3c, 1, 4 } };
            std::vector<Stage> custom (list.begin(), list.begin() + 4);
            for (int m : currentOrder()) custom.push_back (mids[m]);
            custom.push_back ({ "reset order", nullptr, 0, 3 });
            custom.push_back (list[10]); custom.push_back (list[11]);
            list = custom;
        }
       #endif
        const auto f = screenFont (11.5f, 2).withExtraKerningFactor (0.06f), fs = screenFont (10.5f, 2);
        std::vector<float> widths;
        float total = 0.0f;
        const float gap = 16.0f;
        for (const auto& st : list)
        {
            const bool swap = st.action == 2;
            const float w = juce::GlyphArrangement::getStringWidth (swap ? fs : f, st.name) + (swap ? 16.0f : 22.0f);
            widths.push_back (w);
            total += w;
        }
        total += gap * (float) (list.size() - 1);
        // the drawer arrow sits at the right end
        auto arrowCell = r.removeFromRight (64.0f);
        float x = r.getCentreX() - total * 0.5f;
        const float y0 = r.getY() + 4.0f, y1 = r.getBottom() - 4.0f, cy = (y0 + y1) * 0.5f;
        const auto mouse = getMouseXYRelative().toFloat();
        chips.clear();
        int slot = 0;
        for (size_t i = 0; i < list.size(); ++i)
        {
            const auto& st = list[i];
            const float w = widths[i];
            const auto chip = juce::Rectangle<float> (x, y0, w, y1 - y0);
            const bool hot = isMouseOver() && (st.action != 0 || (MISHKAL_V68 && st.mid >= 0)) && chip.contains (mouse);
            if (MISHKAL_V68 && dragging && st.mid >= 0 && slot == dropSlot)   // V68: where the dragged module will land
            {
                g.setColour (juce::Colour (MISHKAL_ACCENT));
                g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, y1 - y0 + 4.0f).withCentre ({ chip.getX() - gap * 0.5f, cy }), 1.5f);
            }
            if (st.action == 2 || st.action == 3)
            {
                const auto pill = chip.reduced (0.0f, 2.0f);
                g.setColour (juce::Colour (MISHKAL_ACCENT).withAlpha (hot ? 0.18f : 0.0f));
                g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
                g.setColour (juce::Colour (MISHKAL_ACCENT));
                g.drawRoundedRectangle (pill, pill.getHeight() * 0.5f, 1.0f);
                g.setFont (fs);
                g.drawText (st.name, pill, juce::Justification::centred);
            }
            else
            {
                const bool on = st.onId == nullptr || proc.apvts.getRawParameterValue (st.onId)->load() > 0.5f;
                const bool drawerStage = st.action == 1;
                g.setColour (juce::Colour (hot ? 0xff26272c : 0xff1b1c20));
                g.fillRoundedRectangle (chip, 4.0f);
                g.setColour (drawerStage ? juce::Colour (0xffff7a3c).withAlpha (hot || open ? 1.0f : 0.7f) : juce::Colour (0xff2c2e33));
                g.drawRoundedRectangle (chip.reduced (0.5f), 4.0f, 1.0f);
                const auto dot = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ chip.getX() + 10.0f, cy });
                const auto col = on ? juce::Colour (st.colour) : juce::Colour (0xff555558);
                if (on) { g.setColour (col.withAlpha (0.3f)); g.fillEllipse (dot.expanded (2.0f)); }
                g.setColour (col);
                g.fillEllipse (dot);
                g.setColour (on ? juce::Colour (0xffe9e7e2) : juce::Colour (0xff5c5e64));
                g.setFont (f);
                g.drawText (st.name, chip.withTrimmedLeft (16.0f), juce::Justification::centredLeft);
            }
            if (MISHKAL_V68 && dragging && st.mid == dragMid)   // the one being dragged glows
            {
                g.setColour (juce::Colour (MISHKAL_ACCENT).withAlpha (0.35f));
                g.drawRoundedRectangle (chip.expanded (2.0f), 5.0f, 2.0f);
            }
            if (st.action != 0 || st.mid >= 0) chips.push_back ({ chip, st.action, st.mid });
            if (st.mid >= 0) ++slot;
            const bool pillHere = st.action == 2 || st.action == 3, pillNext = i + 1 < list.size() && (list[i + 1].action == 2 || list[i + 1].action == 3);
            if (i + 1 < list.size() && ! pillHere && ! pillNext)
            {
                juce::Path chev;
                const float cx = x + w + gap * 0.5f;
                chev.startNewSubPath (cx - 2.0f, cy - 3.5f); chev.lineTo (cx + 2.0f, cy); chev.lineTo (cx - 2.0f, cy + 3.5f);
                g.setColour (juce::Colour (0xff5c5e64));
                g.strokePath (chev, juce::PathStrokeType (1.3f));
            }
            x += w + gap;
        }
        // drawer arrow (open / close)
        {
            const bool hot = isMouseOver() && arrowCell.contains (mouse);
            const auto a = arrowCell.getCentre();
            juce::Path arrow;
            if (open) arrow.addTriangle (a.x - 5.0f, a.y - 2.5f, a.x + 5.0f, a.y - 2.5f, a.x, a.y + 3.5f);
            else      arrow.addTriangle (a.x - 5.0f, a.y + 2.5f, a.x + 5.0f, a.y + 2.5f, a.x, a.y - 3.5f);
            g.setColour (open || hot ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff8e8e93));
            g.fillPath (arrow);
            chips.push_back ({ arrowCell, 1 });
        }
        return;
    }
   #endif

    // centre tab: arrow + name, lit while open
    const bool hot = isMouseOver();
    auto tab = r.withSizeKeepingCentre (240.0f, r.getHeight());
    g.setColour (open ? juce::Colour (0xff1c1c1e) : (hot ? juce::Colour (0xff161618) : juce::Colour (0xff0b0b0c)));
    g.fillRoundedRectangle (tab.withTrimmedTop (1.0f).withTrimmedBottom (-6.0f), 6.0f);
    juce::Path arrow;
    const auto a = juce::Point<float> (tab.getX() + 26.0f, tab.getCentreY());
    if (open) arrow.addTriangle (a.x - 5.0f, a.y - 2.5f, a.x + 5.0f, a.y - 2.5f, a.x, a.y + 3.5f);
    else      arrow.addTriangle (a.x - 5.0f, a.y + 2.5f, a.x + 5.0f, a.y + 2.5f, a.x, a.y - 3.5f);
    g.setColour (open || hot ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff8e8e93));
    g.fillPath (arrow);
    g.setFont (screenFont (13.0f, 2).withExtraKerningFactor (0.16f));
    g.drawText (juce::String (juce::CharPointer_UTF8 (MISHKAL_V41 ? "TONE \xc2\xb7 TAPE" : "ZEREM \xc2\xb7 TAPE")), tab.withTrimmedLeft (40.0f), juce::Justification::centredLeft);

    // status lights on the right: is each stage switched in?
    auto lights = r.removeFromRight (230.0f).reduced (0.0f, 8.0f);
    const std::pair<const char*, const char*> stages[] { { "TAPE", id::tapeOn }, { MISHKAL_V41 ? "TONE" : "ZEREM", id::flowOn } };
    for (const auto& st : stages)
    {
        auto cell = lights.removeFromRight (100.0f);
        const bool on = proc.apvts.getRawParameterValue (st.second)->load() > 0.5f;
        drawGoliathLed (g, juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ cell.getX() + 10.0f, cell.getCentreY() }), goliathAmber(), on);
        g.setColour (on ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff76767c));
        g.setFont (screenFont (12.0f, 1).withExtraKerningFactor (0.12f));
        g.drawText (st.first, cell.withTrimmedLeft (22.0f), juce::Justification::centredLeft);
    }
}

void DrawerBar::doAction (int action)
{
    if (action == 1 && onToggle) onToggle();   // TONE / TAPE / the arrow: open or close the drawer
    if (action == 2)                             // flip Character EQs before / after the dynamics
        if (auto* prm = proc.apvts.getParameter (id::eqAfterComp))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
            prm->endChangeGesture();
        }
   #if MISHKAL_V68
    if (action == 3)                             // V68: back to the standard order
        if (auto* prm = proc.apvts.getParameter (id::midOrder))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (0.0f);
            prm->endChangeGesture();
        }
   #endif
    repaint();
}

std::array<int, 5> DrawerBar::currentOrder() const
{
    const int mo = juce::roundToInt (proc.apvts.getRawParameterValue (id::midOrder)->load());
    if (mo > 0) return permutationFromIndex<5> (mo - 1);
    const bool post = proc.apvts.getRawParameterValue (id::eqAfterComp)->load() > 0.5f;
    return post ? std::array<int, 5> { 1, 0, 2, 3, 4 } : std::array<int, 5> { 0, 1, 2, 3, 4 };
}

void DrawerBar::mouseDown (const juce::MouseEvent& e)
{
   #if MISHKAL_V42
    dragMid = -1; dropSlot = -1; dragging = false; pressedAction = 0;
    for (const auto& c : chips)
        if (c.r.contains (e.position))
        {
            pressedAction = c.action;
            if (MISHKAL_V68) { dragMid = c.mid; if (c.mid < 0) { doAction (c.action); pressedAction = 0; } }   // pills act at once
            else doAction (c.action);
            return;
        }
   #else
    juce::ignoreUnused (e);
    if (onToggle) onToggle();
   #endif
}

void DrawerBar::mouseDrag (const juce::MouseEvent& e)
{
   #if MISHKAL_V68
    if (dragMid < 0) return;
    if (! dragging && e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    // the drop slot = how many movable chips have their centre left of the mouse
    int slotIdx = 0;
    for (const auto& c : chips) if (c.mid >= 0 && c.r.getCentreX() < e.position.x) ++slotIdx;
    dropSlot = slotIdx;
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    repaint();
   #else
    juce::ignoreUnused (e);
   #endif
}

void DrawerBar::mouseUp (const juce::MouseEvent&)
{
   #if MISHKAL_V68
    setMouseCursor (juce::MouseCursor::NormalCursor);
    if (dragging && dragMid >= 0 && dropSlot >= 0)
    {
        auto order = currentOrder();
        std::vector<int> v (order.begin(), order.end());
        const int from = (int) (std::find (v.begin(), v.end(), dragMid) - v.begin());
        v.erase (v.begin() + from);
        int to = dropSlot > from ? dropSlot - 1 : dropSlot;
        v.insert (v.begin() + juce::jlimit (0, (int) v.size(), to), dragMid);
        std::array<int, 5> next {};
        std::copy (v.begin(), v.end(), next.begin());
        if (next != order)
            if (auto* prm = proc.apvts.getParameter (id::midOrder))
            {
                const int idx = indexFromPermutation<5> (next) + 1;
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) idx));
                prm->endChangeGesture();
            }
    }
    else if (pressedAction != 0)
        doAction (pressedAction);   // a click (no drag) on TONE / TAPE still opens the drawer
    dragMid = -1; dropSlot = -1; dragging = false; pressedAction = 0;
    repaint();
   #endif
}

//==============================================================================
void TypeBar::toggleUnit (int t)
{
    if (auto* p = state.getParameter (onParamId (t)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (isActive (t) ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
    repaint();
}

void TypeBar::setUnitOrder (const std::vector<int>& o)
{
    if (orderParamId.isEmpty() || (int) o.size() != names.size()) return;
    if (auto* p = state.getParameter (orderParamId)) p->beginChangeGesture();
    setOrder (o);
    if (auto* p = state.getParameter (orderParamId)) p->endChangeGesture();
    repaint();
}

//==============================================================================
static juce::String ordinal (int n)
{
    const int m = n % 100;
    const char* suf = (m >= 11 && m <= 13) ? "th" : n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th";
    return juce::String (n) + suf;
}

juce::String ChainLane::nameOf (int t, bool shortName) const
{
    auto n = nameFor ? nameFor (t) : juce::String();
    if (n.isEmpty()) n = bar.unitNames()[t];
    return shortName && n.startsWith ("LE-") ? n.substring (3) : n;
}

std::vector<ChainLane::Chip> ChainLane::layoutChips (juce::Font& f, bool& shortNames) const
{
    std::vector<int> act;
    for (int t : bar.getOrder()) if (bar.unitActive (t)) act.push_back (t);
    const float W = (float) getWidth(), H = (float) getHeight(), sep = 12.0f;
    for (int pass = 0; pass < 3; ++pass)
    {
        shortNames = pass >= 1;
        f = font (pass < 2 ? 11.5f : 10.0f, true);
        float total = 0.0f;
        for (int t : act) total += 24.0f + juce::GlyphArrangement::getStringWidth (f, nameOf (t, shortNames)) + 8.0f;
        total += sep * (float) juce::jmax (0, (int) act.size() - 1);
       #if MISHKAL_V83
        if (total > W && pass == 2)   // V83: too many to name them all: the one being edited keeps its name, the others become numbered dots
        {
            const int sel = bar.getSelected();
            const float dot = H - 2.0f, gap = 6.0f;
            std::vector<Chip> chips;
            float x = 0.0f;
            for (int t : act)
            {
                const float w = t == sel ? juce::jmin (W * 0.5f, 24.0f + juce::GlyphArrangement::getStringWidth (f, nameOf (t, true)) + 8.0f) : dot;
                chips.push_back ({ t, { x, 1.0f, w, H - 2.0f } });
                x += w + gap;
            }
            return chips;
        }
       #endif
        if (total <= W || pass == 2)
        {
            const float squeeze = total > W ? W / total : 1.0f;
            std::vector<Chip> chips;
            float x = 0.0f;
            for (int t : act)
            {
                const float w = (24.0f + juce::GlyphArrangement::getStringWidth (f, nameOf (t, shortNames)) + 8.0f) * squeeze;
                chips.push_back ({ t, { x, 1.0f, w, H - 2.0f } });
                x += w + sep * squeeze;
            }
            return chips;
        }
    }
    return {};
}

int ChainLane::dropIndex (float x) const
{
    juce::Font f (juce::FontOptions {});
    bool sn = false;
    const auto chips = layoutChips (f, sn);
    int i = 0;
    for (auto& c : chips) { if (c.unit != dragUnit && x > c.r.getCentreX()) ++i; }
    return i;
}

void ChainLane::paint (juce::Graphics& g)
{
    juce::Font f (juce::FontOptions {});
    bool sn = false;
    const auto chips = layoutChips (f, sn);
    if (chips.empty())
    {
        g.setColour (goliathLabel().withAlpha (0.7f));
        g.setFont (font (11.0f));
        g.drawText ("no " + noun + " in yet: click the light on the menu above", getLocalBounds(), juce::Justification::centredLeft);
        return;
    }
    const int sel = bar.getSelected();
    for (size_t i = 0; i < chips.size(); ++i)
    {
        auto c = chips[i];
        auto r = c.r;
        if (dragging && c.unit == dragUnit) r = r.withX (juce::jlimit (0.0f, (float) getWidth() - r.getWidth(), dragX - r.getWidth() * 0.5f));
        const auto col = colourFor ? colourFor (c.unit) : goliathAmber();
        const bool isSel = c.unit == sel;
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (isSel ? col.withAlpha (0.9f) : juce::Colour (0xff323236));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, isSel ? 1.4f : 1.0f);
        if (isSel) { g.setColour (col.withAlpha (0.14f)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f); }
        // the number: its place in the chain
        const auto badge = juce::Rectangle<float> (r.getHeight() - 4.0f, r.getHeight() - 4.0f).withCentre ({ r.getX() + r.getHeight() * 0.5f, r.getCentreY() });
        g.setColour (col);
        g.fillEllipse (badge);
        g.setColour (juce::Colour (0xff0b0b0c));
        g.setFont (font (11.0f, true));
        g.drawText (juce::String ((int) i + 1), badge, juce::Justification::centred);
        g.setColour (isSel ? goliathLegend() : goliathLabel());
        g.setFont (f);
        if (r.getWidth() > r.getHeight() + 4.0f)   // V83: a numbered dot has no room for its name
            g.drawText (nameOf (c.unit, sn), r.withTrimmedLeft (r.getHeight() + 1.0f).withTrimmedRight (4.0f), juce::Justification::centredLeft, true);
        if (i + 1 < chips.size() && ! dragging && chips[i + 1].r.getX() - c.r.getRight() > 8.0f)
        {
            g.setColour (goliathLabel().withAlpha (0.7f));
            g.setFont (font (12.0f, true));
            g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xba")), juce::Rectangle<float> (c.r.getRight(), 0.0f, chips[i + 1].r.getX() - c.r.getRight(), (float) getHeight()), juce::Justification::centred);
        }
    }
    if (dragging)   // where it will land
    {
        const int at = dropIndex (dragX);
        std::vector<Chip> rest;
        for (auto& c : chips) if (c.unit != dragUnit) rest.push_back (c);
        const float x = rest.empty() ? 0.0f : at >= (int) rest.size() ? rest.back().r.getRight() + 5.0f : rest[(size_t) at].r.getX() - 6.0f;
        g.setColour (goliathAmber());
        g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, (float) getHeight()).withX (x), 1.0f);
    }
}

void ChainLane::mouseDown (const juce::MouseEvent& e)
{
    juce::Font f (juce::FontOptions {});
    bool sn = false;
    dragUnit = -1;
    dragging = false;
    for (auto& c : layoutChips (f, sn))
        if (c.r.contains (e.position)) { dragUnit = c.unit; bar.setSelected (c.unit); break; }
    repaint();
}

void ChainLane::mouseDrag (const juce::MouseEvent& e)
{
    if (dragUnit < 0) return;
    if (! dragging && e.getDistanceFromDragStart() > 4) dragging = true;
    dragX = e.position.x;
    repaint();
}

void ChainLane::mouseUp (const juce::MouseEvent& e)
{
    if (dragging && dragUnit >= 0)
    {
        // the new order of the units that are in, written back into their slots of the full order
        auto full = bar.getOrder();
        std::vector<int> act;
        for (int t : full) if (bar.unitActive (t) && t != dragUnit) act.push_back (t);
        const int at = juce::jlimit (0, (int) act.size(), dropIndex (e.position.x));
        act.insert (act.begin() + at, dragUnit);
        size_t k = 0;
        for (auto& t : full) if (bar.unitActive (t)) t = act[k++];
        bar.setUnitOrder (full);
    }
    dragging = false;
    dragUnit = -1;
    repaint();
}

void TypeBar::moveUnit (int t, int delta)
{
    auto o = order();
    const int from = (int) (std::find (o.begin(), o.end(), t) - o.begin());
    const int to = juce::jlimit (0, (int) o.size() - 1, from + delta);
    if (from == to || orderParamId.isEmpty()) return;
    std::swap (o[(size_t) from], o[(size_t) to]);
    if (auto* p = state.getParameter (orderParamId)) p->beginChangeGesture();
    setOrder (o);
    if (auto* p = state.getParameter (orderParamId)) p->endChangeGesture();
    repaint();
}

void EqDropdown::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const int t = bar.getSelected();
    const bool on = bar.unitActive (t);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (isMouseOver() ? juce::Colour (0xff4a4a4e) : juce::Colour (0xff323236));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    // the light: is this EQ switched in?
    drawGoliathLed (g, juce::Rectangle<float> (13.0f, 13.0f).withCentre ({ r.getX() + 15.0f, r.getCentreY() }), goliathAmber(), on);
    // position in the chain
    const auto o = bar.getOrder();
    const int slot = (int) (std::find (o.begin(), o.end(), t) - o.begin());
    auto text = r.withTrimmedLeft (30.0f).withTrimmedRight (24.0f);
    g.setColour (goliathLegend());
    g.setFont (font (15.0f, true));
    g.drawText (nameOf (t), text.removeFromTop (r.getHeight() * 0.56f), juce::Justification::bottomLeft);
    g.setColour (goliathLabel());
    g.setFont (font (11.0f));
   #if MISHKAL_V81
    {   // its place among the ones that are in: "2nd of 3 in", or "switched out"
        int pos = 0, ins = 0;
        for (int u : o) if (bar.unitActive (u)) { ++ins; if (u == t) pos = ins; }
        juce::ignoreUnused (slot);
        g.drawText (describe (t) + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) + (on ? ordinal (pos) + " of " + juce::String (ins) + " in" : juce::String ("switched out")),
                    text, juce::Justification::topLeft);
    }
   #else
    g.drawText (describe (t) + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) + juce::String (slot + 1) + " of " + juce::String ((int) o.size()), text, juce::Justification::topLeft);
   #endif

    juce::Path arrow;
    const auto c = juce::Point<float> (r.getRight() - 13.0f, r.getCentreY());
    arrow.addTriangle (c.x - 5.0f, c.y - 2.5f, c.x + 5.0f, c.y - 2.5f, c.x, c.y + 3.5f);
    g.setColour (goliathLabel());
    g.fillPath (arrow);
}

void EqDropdown::mouseDown (const juce::MouseEvent& e)
{
    const int sel = bar.getSelected();
    if (e.x < 30) { bar.toggleUnit (sel); repaint(); return; }   // the light

    juce::PopupMenu m;
    m.addSectionHeader ("EDIT");
   #if MISHKAL_V81
    {   // the ones that are in first, numbered in the order they run; then the ones that are out
        int n = 0;
        for (int t : bar.getOrder())
            if (bar.unitActive (t)) m.addItem (1 + t, juce::String (++n) + "   " + nameOf (t) + "   " + describe (t), true, t == sel);
        for (int t : bar.getOrder())
            if (! bar.unitActive (t)) m.addItem (1 + t, juce::String (juce::CharPointer_UTF8 ("\xc2\xb7   ")) + nameOf (t) + "   " + describe (t) + "   (out)", true, t == sel);
    }
   #else
    for (int t : bar.getOrder())
        m.addItem (1 + t, nameOf (t) + "   " + describe (t) + (bar.unitActive (t) ? "" : "   (out)"), true, t == sel);
   #endif
    m.addSeparator();
    m.addItem (100, juce::String ("Switch ") + nameOf (sel) + (bar.unitActive (sel) ? " out" : " in"));
    const auto o = bar.getOrder();
    const int slot = (int) (std::find (o.begin(), o.end(), sel) - o.begin());
    m.addItem (101, "Run " + bar.unitNames()[sel] + " earlier", slot > 0);
    m.addItem (102, "Run " + bar.unitNames()[sel] + " later", slot < (int) o.size() - 1);

    juce::Component::SafePointer<EqDropdown> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                     [safe, sel] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         auto& b = safe->bar;
                         if (r >= 1 && r <= b.unitNames().size()) b.setSelected (r - 1);
                         else if (r == 100) b.toggleUnit (sel);
                         else if (r == 101) b.moveUnit (sel, -1);
                         else if (r == 102) b.moveUnit (sel, 1);
                         safe->repaint();
                     });
}

//==============================================================================
void GrRing::paint (juce::Graphics& g)
{
    // 27 short segments around the knob, 135 to 405 degrees like the scale; amber up to 6 dB, red beyond
    const auto c = getLocalBounds().toFloat().getCentre();
    constexpr int segs = 27;
    constexpr float maxDb = 20.0f;
    const float gr = bridge.compGr;
    const float a0 = juce::MathConstants<float>::pi * 1.2f, a1 = juce::MathConstants<float>::pi * 2.8f;   // the slider's rotary range
    const float r1 = knobRadius + 4.0f, r2 = knobRadius + 8.5f;
    if (MISHKAL_V7)
    {
        // V7: one solid arc like the bar meters, coloured by how far round it reaches: green -> yellow -> red
        const float r = (r1 + r2) * 0.5f, w = r2 - r1;
        juce::Path track;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xff060607));
        g.strokePath (track, juce::PathStrokeType (w + 2.0f));
        const float frac = juce::jlimit (0.0f, 1.0f, gr / maxDb);
        if (frac <= 0.002f)
            return;
        const juce::Colour green (0xff55f06e), yellow (0xffffb43c), red (0xffff3d3d);
        auto colourAt = [&] (float f)   // 0..1 of the full 20 dB: green to ~3 dB, yellow ~6 dB, red from ~10 dB
        {
            const float db = f * maxDb;
            if (db < 2.0f)  return green;
            if (db < 6.0f)  return green.interpolatedWith (yellow, (db - 2.0f) / 4.0f);
            if (db < 10.0f) return yellow.interpolatedWith (red, (db - 6.0f) / 4.0f);
            return red;
        };
        constexpr int slices = 90;
        const int lit = juce::jmax (1, (int) std::ceil (frac * (float) slices));
        const float step = (a1 - a0) / (float) slices;
        for (int pass = 0; pass < 2; ++pass)   // a soft glow first, then the solid arc on top
            for (int i = 0; i < lit; ++i)
            {
                const float s0 = a0 + step * (float) i;
                const float s1 = juce::jmin (a0 + (a1 - a0) * frac, s0 + step * 1.15f);
                if (s1 <= s0) break;
                juce::Path p;
                p.addCentredArc (c.x, c.y, r, r, 0.0f, s0, s1, true);
                const auto col = colourAt (((float) i + 0.5f) / (float) slices);
                g.setColour (pass == 0 ? col.withAlpha (0.25f) : col);
                g.strokePath (p, juce::PathStrokeType (pass == 0 ? w + 5.0f : w));
            }
        return;
    }
    const float span = (a1 - a0) / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float segDb = maxDb * ((float) i + 0.5f) / (float) segs;
        const bool lit = gr >= segDb;
        const auto col = segDb < 6.0f ? juce::Colour (0xffffb43c) : juce::Colour (0xffff3d3d);
        // JUCE angles: 0 = 12 o'clock, clockwise, the same as the knob's scale
        const float s0 = a0 + span * (float) i + span * 0.18f;
        const float s1 = a0 + span * (float) (i + 1) - span * 0.18f;
        juce::Path p;
        p.addCentredArc (c.x, c.y, (r1 + r2) * 0.5f, (r1 + r2) * 0.5f, 0.0f, s0, s1, true);
        if (lit)
        {
            g.setColour (col.withAlpha (0.3f));
            g.strokePath (p, juce::PathStrokeType (r2 - r1 + 4.0f));
        }
        g.setColour (lit ? col : col.withMultipliedBrightness (0.22f).withMultipliedSaturation (0.6f));
        g.strokePath (p, juce::PathStrokeType (r2 - r1));
    }
}

void DynMeterBlock::paint (juce::Graphics& g)
{
    // same style as the Limiter's LED block: In L, In R, Gate, GR
    constexpr int leds = 10;
    constexpr float d = 10.0f, gapY = 3.0f, colW = 16.0f, gapX = 6.0f, padX = 10.0f, padY = 8.0f, headH = 14.0f, headGap = 4.0f;
    const float blockW = padX * 2.0f + colW * 4.0f + gapX * 3.0f;
    const float blockH = padY * 2.0f + headH + headGap + d * leds + gapY * (leds - 1) - (MISHKAL_V50 ? 9.0f : 0.0f);   // V50: room for the names
    auto all = getLocalBounds().toFloat();
    auto block = juce::Rectangle<float> (blockW, blockH).withCentre ({ all.getCentreX(), all.getY() + 2.0f + blockH * 0.5f });
    g.setColour (juce::Colour (0xff0d0d0e));
    g.fillRoundedRectangle (block, 3.0f);

    static constexpr float inSteps[leds]   { -36.0f, -30.0f, -24.0f, -18.0f, -12.0f, -9.0f, -6.0f, -3.0f, -1.0f, 0.0f };
    static constexpr float gateSteps[leds] { 1.0f, 3.0f, 6.0f, 10.0f, 15.0f, 20.0f, 30.0f, 40.0f, 60.0f, 80.0f };
    static constexpr float grSteps[leds]   { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 14.0f, 20.0f };
    const juce::Colour green (0xff55f06e), amber (0xffffb43c), red (0xffff3d3d);
    const char* heads[] { "L", "R", "G", "GR" };
    const bool mono = MISHKAL_V60 && MISHKAL_V6 && bridge.isMono();   // V60: one wide input bar on a mono track
    for (int col = 0; col < 4; ++col)
    {
        if (mono && col == 1) continue;
        const float x = block.getX() + padX + (float) col * (colW + gapX);
        const float cx = x + colW * 0.5f;
        const float wCol = mono && col == 0 ? colW * 2.0f + gapX : colW;
        g.setColour (juce::Colour (0xffdcdcd6));
        g.setFont (font (11.0f, true));
        g.drawText (mono && col == 0 ? "M" : heads[col], juce::Rectangle<float> (x - 6.0f, block.getY() + padY, wCol + 12.0f, headH), juce::Justification::centred);
        if (MISHKAL_V5 && col == 3)
        {
            const float top = block.getY() + padY + headH + headGap;
            drawGrBar (g, { x + 1.0f, top, colW - 2.0f, block.getBottom() - padY - top }, bridge.compGr, 20.0f);
            continue;
        }
        if (MISHKAL_V8 && col == 2)
        {
            // V8: gate attenuation as one solid bar hanging from the top (like GR), coloured by depth
            // green -> yellow -> red, on the same steps the LEDs used
            const float top = block.getY() + padY + headH + headGap;
            const auto slot = juce::Rectangle<float> (x + 1.0f, top, colW - 2.0f, block.getBottom() - padY - top);
            g.setColour (juce::Colour (0xff060607));
            g.fillRect (slot);
            g.setColour (juce::Colour (0xff1e1e20));
            for (int i = 1; i < 4; ++i)
                g.drawHorizontalLine ((int) (slot.getY() + slot.getHeight() * (float) i / 4.0f), slot.getX(), slot.getRight());
            float frac = 0.0f;   // position on the LED scale, interpolated between steps
            if (gateShown >= gateSteps[leds - 1]) frac = 1.0f;
            else if (gateShown > 0.0f)
            {
                float prev = 0.0f;
                for (int i = 0; i < leds; ++i)
                {
                    if (gateShown < gateSteps[i])
                    {
                        frac = ((float) i + (gateShown - prev) / (gateSteps[i] - prev)) / (float) leds;
                        break;
                    }
                    prev = gateSteps[i];
                }
            }
            if (frac > 0.002f)
            {
                juce::ColourGradient grad (green, 0.0f, slot.getY(), red, 0.0f, slot.getBottom(), false);
                grad.addColour (0.25, green);
                grad.addColour (0.5, amber);
                grad.addColour (0.62, amber);
                grad.addColour (0.8, red);
                g.setGradientFill (grad);
                g.fillRect (slot.withHeight (juce::jmax (2.0f, slot.getHeight() * frac)));
            }
            continue;
        }
        if (MISHKAL_V6 && col < 2)
        {
            const float top = block.getY() + padY + headH + headGap;
            drawLevelBar (g, { x + 1.0f, top, wCol - 2.0f, block.getBottom() - padY - top }, bridge.inPk[(size_t) col], -42.0f, -9.0f, -2.0f);
            continue;
        }
        for (int i = 0; i < leds; ++i)
        {
            const float cy = block.getBottom() - padY - d * 0.5f - (float) i * (d + gapY);
            juce::Colour on;
            bool lit;
            if (col < 2)
            {
                on = i >= 8 ? red : i >= 6 ? amber : green;
                lit = bridge.inPk[(size_t) col] >= inSteps[i];
            }
            else
            {
                const int fromTop = leds - 1 - i;   // reduction lights from the top down
                on = col == 2 ? amber : (fromTop >= 6 ? red : amber);
                lit = col == 2 ? gateShown >= gateSteps[fromTop] : bridge.compGr >= grSteps[fromTop];
            }
            const auto dot = juce::Rectangle<float> (d, d).withCentre ({ cx, cy });
            if (lit)
            {
                g.setColour (on.withAlpha (0.22f)); g.fillEllipse (dot.expanded (5.0f));
                g.setColour (on.withAlpha (0.45f)); g.fillEllipse (dot.expanded (2.0f));
                g.setColour (on);
            }
            else
                g.setColour (on.withMultipliedBrightness (0.22f).withMultipliedSaturation (0.6f));
            g.fillEllipse (dot);
        }
    }
    auto names = juce::Rectangle<float> (block.getX() - 10.0f, block.getBottom() + 3.0f, blockW + 20.0f, 16.0f);
    auto colX = [&] (float c) { return block.getX() + padX + c * (colW + gapX) + colW * 0.5f; };
    g.setFont (font (12.0f));
    g.setColour (juce::Colour (0xff7e7e7a));
    const float sp = MISHKAL_V50 ? 3.0f : 0.0f;   // V50: a little air between Gate and GR
    g.drawText ("In",   juce::Rectangle<float> (40.0f, names.getHeight()).withCentre ({ colX (0.5f), names.getCentreY() }), juce::Justification::centred);
    g.drawText ("Gate", juce::Rectangle<float> (40.0f, names.getHeight()).withCentre ({ colX (2.0f) - sp, names.getCentreY() }), juce::Justification::centred);
    g.drawText ("GR",   juce::Rectangle<float> (30.0f, names.getHeight()).withCentre ({ colX (3.0f) + sp, names.getCentreY() }), juce::Justification::centred);
}

//==============================================================================
CharEqScreen::CharEqScreen (VoxStripProcessor& p, TypeBar& t) : proc (p), types (t)
{
    startTimerHz (30);
}

std::vector<CharEqScreen::Band> CharEqScreen::bandsFor (int t) const
{
    if (t == (int) EqType::TubeP1)
        return { { "LOW",        "LowFreq",   "LowBoost",  "LowAtten", nullptr },
                 { "HIGH BOOST", "HighFreq",  "HighBoost", nullptr,    "HighBw" },
                 { "HIGH ATTEN", "AttenFreq", nullptr,     "HighAtten", nullptr } };
    switch ((EqType) t)
    {
        case EqType::Api550:
            return { { "LOW",      "LowFreq",  "LowGain",  nullptr, nullptr, 0.0, { { "SHAPE", "LowShape" } } },
                     { "LOW-MID",  "LmFreq",   "LmGain",   nullptr, nullptr },
                     { "HIGH-MID", "HmFreq",   "HmGain",   nullptr, nullptr },
                     { "HIGH",     "HighFreq", "HighGain", nullptr, nullptr, 0.0, { { "SHAPE", "HighShape" }, { "FILTER", "Filter" } } } };
        case EqType::K9000:
            return { { "HPF",  "HpfFreq", nullptr,   nullptr, nullptr, 0.0, { { "CURVES", "Mode" } } },
                     { "LF",   "LfFreq",  "LfGain",  nullptr, nullptr, 0.0, { { "BELL", "LfBell" } } },
                     { "LMF",  "LmfFreq", "LmfGain", nullptr, "LmfQ" },
                     { "HMF",  "HmfFreq", "HmfGain", nullptr, "HmfQ" },
                     { "HF",   "HfFreq",  "HfGain",  nullptr, nullptr, 0.0, { { "BELL", "HfBell" } } },
                     { "LPF",  "LpfFreq", nullptr,   nullptr, nullptr, 0.0, { { "CURVES", "Mode" } } } };
        case EqType::Helios69:
            return { { "BASS",   "BassFreq", "BassGain",   nullptr, nullptr },
                     { "MID",    "MidFreq",  "MidGain",    nullptr, nullptr },
                     { "TREBLE", nullptr,    "TrebleGain", nullptr, nullptr, 10000.0 } };
        case EqType::Maag4:
            return { { "SUB",   nullptr,   "SubGain", nullptr, nullptr, 20.0 },
                     { "40",    nullptr,   "G40",     nullptr, nullptr, 40.0 },
                     { "160",   nullptr,   "G160",    nullptr, nullptr, 160.0 },
                     { "650",   nullptr,   "G650",    nullptr, nullptr, 650.0 },
                     { "2.5K",  nullptr,   "G2k5",    nullptr, nullptr, 2500.0 },
                     { "AIR",   "AirFreq", "AirGain", nullptr, nullptr } };
        default: break;
    }
    return { { "HIGH-PASS", "HpfFreq",  nullptr,    nullptr, nullptr },
             { "BODY",      "BodyFreq", "BodyGain", nullptr, nullptr },
             { "MUD",       "MudFreq",  "MudGain",  nullptr, "MudQ" },
             { "PRESENCE",  "PresFreq", "PresGain", nullptr, "PresQ" },
             { "AIR",       "AirFreq",  "AirGain",  nullptr, nullptr } };
}

std::vector<CharEqScreen::Field> CharEqScreen::fieldsFor (int t, int band) const
{
    const auto bands = bandsFor (t);
    const auto& b = bands[(size_t) juce::jlimit (0, (int) bands.size() - 1, band)];
    std::vector<Field> f;
    if (b.freq != nullptr) f.push_back ({ "FREQ", b.freq });
    if (t >= (int) EqType::Api550)   // V74 models
    {
        if (b.gain != nullptr) f.push_back ({ "GAIN", b.gain });
        if (b.q != nullptr)    f.push_back ({ "Q", b.q });
        for (auto& m : b.more) f.push_back (m);
        return f;
    }
    if (t == (int) EqType::TubeP1)
    {
        if (b.gain != nullptr)  f.push_back ({ "BOOST", b.gain });
        if (b.gain2 != nullptr) f.push_back ({ "ATTEN", b.gain2 });
        if (b.q != nullptr)     f.push_back ({ "WIDTH", b.q });
    }
    else
    {
        if (b.gain != nullptr) f.push_back ({ "GAIN", b.gain });
        if (b.q != nullptr)    f.push_back ({ "Q", b.q });
        if (t == (int) EqType::Airy && juce::String (b.freq) == "AirFreq") f.push_back ({ "SILK", "Silk" });
    }
    return f;
}

juce::RangedAudioParameter* CharEqScreen::param (const char* field) const
{
    if (field == nullptr) return nullptr;
    return proc.apvts.getParameter (charEqId (types.getSelected(), field));
}

juce::Rectangle<float> CharEqScreen::plotArea() const
{
    return getLocalBounds().toFloat().withTrimmedBottom (78.0f).reduced (1.0f);
}

float CharEqScreen::xForHz (double hz) const
{
    const auto p = plotArea().reduced (8.0f, 0.0f);
    return p.getX() + p.getWidth() * (float) (std::log (juce::jlimit (20.0, 20000.0, hz) / 20.0) / std::log (1000.0));
}

double CharEqScreen::hzForX (float x) const
{
    const auto p = plotArea().reduced (8.0f, 0.0f);
    return 20.0 * std::pow (1000.0, (double) juce::jlimit (0.0f, 1.0f, (x - p.getX()) / p.getWidth()));
}

float CharEqScreen::yForDb (float db) const
{
    const auto p = plotArea().reduced (0.0f, 14.0f);
    return p.getCentreY() - juce::jlimit (-18.0f, 18.0f, db) / 18.0f * p.getHeight() * 0.5f;
}

static double hzFromText (const juce::String& text)
{
    const double v = text.getDoubleValue();
    return text.containsIgnoreCase ("k") ? v * 1000.0 : v;
}

juce::Point<float> CharEqScreen::dotFor (int band) const
{
    const int t = types.getSelected();
    const auto bands = bandsFor (t);
    const auto& b = bands[(size_t) band];
    double hz = b.fixedHz > 0.0 ? b.fixedHz : 1000.0;
    if (auto* f = param (b.freq))   // V74: continuous frequencies straight from the value (their text can say "Off")
        hz = dynamic_cast<juce::AudioParameterChoice*> (f) != nullptr ? hzFromText (f->getCurrentValueAsText()) : (double) f->convertFrom0to1 (f->getValue());
    const auto s = proc.readSettings();
    const auto d = VocalEQ::design (s.charEq[(size_t) t], (EqType) t, sr);
    float db = VocalEQ::magnitudeDb (d, hz, sr);
    if (juce::String (b.freq) == "HpfFreq" || juce::String (b.freq) == "LpfFreq") db = -3.0f;
    if (t == (int) EqType::K9000)   // V74: LE-K9 filters at their 'Off' end sit at the edge, level
    {
        if (juce::String (b.freq) == "HpfFreq" && hz <= 16.5) { hz = 20.0; db = 0.0f; }
        if (juce::String (b.freq) == "LpfFreq" && hz >= 21900.0) { hz = 20000.0; db = 0.0f; }
    }
    return { xForHz (hz), yForDb (db) };
}

float CharEqScreen::dotValue (int band) const
{
    const auto b = bandsFor (types.getSelected())[(size_t) band];
    float v = 0.0f;
    if (auto* p = param (b.gain))  v += p->convertFrom0to1 (p->getValue());
    if (auto* p = param (b.gain2)) v -= p->convertFrom0to1 (p->getValue());
    return v;
}

void CharEqScreen::begin (juce::RangedAudioParameter* p) { if (p != nullptr) p->beginChangeGesture(); }
void CharEqScreen::end (juce::RangedAudioParameter* p)   { if (p != nullptr) p->endChangeGesture(); }

void CharEqScreen::setDotValue (int band, float v)
{
    const auto b = bandsFor (types.getSelected())[(size_t) band];
    auto* g1 = param (b.gain);
    auto* g2 = param (b.gain2);
    auto set = [] (juce::RangedAudioParameter* p, float value) { if (p != nullptr) p->setValueNotifyingHost (p->convertTo0to1 (value)); };
    if (g1 != nullptr && g2 != nullptr)   // Pultec low: up = boost, down = cut
    {
        set (g1, juce::jmax (0.0f, v));
        set (g2, juce::jmax (0.0f, -v));
    }
    else if (g1 != nullptr) set (g1, v);
    else if (g2 != nullptr) set (g2, -v);
}

void CharEqScreen::setFreqFromX (int band, float x)
{
    const auto b = bandsFor (types.getSelected())[(size_t) band];
    auto* f = param (b.freq);
    if (f == nullptr) return;
    const double hz = hzForX (x);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (f))   // stepped frequencies: snap to the nearest
    {
        int best = 0;
        double bestDist = 1e9;
        for (int i = 0; i < choice->choices.size(); ++i)
        {
            const double d = std::abs (std::log (hzFromText (choice->choices[i]) / hz));
            if (d < bestDist) { bestDist = d; best = i; }
        }
        f->setValueNotifyingHost (f->convertTo0to1 ((float) best));
    }
    else
        f->setValueNotifyingHost (f->convertTo0to1 ((float) hz));
}

int CharEqScreen::hitDot (juce::Point<float> p) const
{
    const auto bands = bandsFor (types.getSelected());
    for (int i = (int) bands.size() - 1; i >= 0; --i)
        if (dotFor (i).getDistanceFrom (p) < 12.0f) return i;
    return -1;
}

int CharEqScreen::hitField (juce::Point<float> p) const
{
    for (int i = 0; i < (int) fieldRects.size(); ++i)
        if (fieldRects[(size_t) i].contains (p)) return i;
    return -1;
}

void CharEqScreen::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitDot (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 || hitField (e.position) >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void CharEqScreen::mouseDown (const juce::MouseEvent& e)
{
    dragDot = dragField = -1;
    const int t = types.getSelected();
    if (const int d = hitDot (e.position); d >= 0)
    {
        selected = dragDot = d;
        const auto b = bandsFor (t)[(size_t) d];
        begin (param (b.freq)); begin (param (b.gain)); begin (param (b.gain2));
        dragStartValue = dotValue (d);
        dragStartY = e.position.y;
    }
    else if (const int f = hitField (e.position); f >= 0)
    {
        dragField = f;
        auto* p = param (fieldsFor (t, selected)[(size_t) f].id);
        begin (p);
        dragStartValue = p != nullptr ? p->getValue() : 0.0f;
        dragStartY = e.position.y;
    }
    repaint();
}

void CharEqScreen::mouseDrag (const juce::MouseEvent& e)
{
    const int t = types.getSelected();
    if (dragDot >= 0)
    {
        const auto b = bandsFor (t)[(size_t) dragDot];
        setFreqFromX (dragDot, e.position.x);
        if (b.gain != nullptr || b.gain2 != nullptr)
        {
            auto* g = param (b.gain != nullptr ? b.gain : b.gain2);
            const auto range = g->getNormalisableRange();
            const float span = range.end - range.start;
            const float lo = (b.gain2 != nullptr && b.gain != nullptr) ? -span : (b.gain != nullptr ? range.start : -range.end);
            const float hi = b.gain != nullptr ? range.end : -range.start;
            setDotValue (dragDot, juce::jlimit (lo, hi, dragStartValue + (dragStartY - e.position.y) * span / 220.0f));
        }
    }
    else if (dragField >= 0)
    {
        if (auto* p = param (fieldsFor (t, selected)[(size_t) dragField].id))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStartValue + (dragStartY - e.position.y) / 160.0f));
    }
}

void CharEqScreen::mouseUp (const juce::MouseEvent&)
{
    const int t = types.getSelected();
    if (dragDot >= 0)
    {
        const auto b = bandsFor (t)[(size_t) dragDot];
        end (param (b.freq)); end (param (b.gain)); end (param (b.gain2));
    }
    else if (dragField >= 0)
        end (param (fieldsFor (t, selected)[(size_t) dragField].id));
    dragDot = dragField = -1;
}

void CharEqScreen::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int t = types.getSelected();
    juce::RangedAudioParameter* p = nullptr;
    if (const int d = hitDot (e.position); d >= 0) { selected = d; p = param (bandsFor (t)[(size_t) d].q); }   // scroll a dot: Q / width
    else if (const int f = hitField (e.position); f >= 0) p = param (fieldsFor (t, selected)[(size_t) f].id);
    if (p == nullptr) return;
    begin (p);
    p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + w.deltaY * 0.08f));
    end (p);
}

void CharEqScreen::mouseDoubleClick (const juce::MouseEvent& e)
{
    // double-click a dot or a readout: back to its default
    const int t = types.getSelected();
    std::vector<juce::RangedAudioParameter*> ps;
    if (const int d = hitDot (e.position); d >= 0)
    {
        const auto b = bandsFor (t)[(size_t) d];
        ps = { param (b.gain), param (b.gain2) };
    }
    else if (const int f = hitField (e.position); f >= 0)
        ps = { param (fieldsFor (t, selected)[(size_t) f].id) };
    for (auto* p : ps)
        if (p != nullptr) { begin (p); p->setValueNotifyingHost (p->getDefaultValue()); end (p); }
}

void CharEqScreen::timerCallback()
{
   #if MISHKAL_V43
    if (isVisible())
    {
        proc.getChain().charTap.pull (specWin);
        std::fill (specData.begin(), specData.end(), 0.0f);
        std::copy (specWin.begin(), specWin.end(), specData.begin());
        double sum = 0.0;
        for (auto v : specWin) sum += (double) v * v;
        const float rmsDb = (float) (10.0 * std::log10 (sum / (double) specSize + 1.0e-12));
        // 0 when quiet, 1 around -12 dBFS RMS: drives the glow
        const float target = juce::jlimit (0.0f, 1.0f, (rmsDb + 54.0f) / 42.0f);
        specLevel += (target > specLevel ? 0.45f : 0.08f) * (target - specLevel);
        specWindow.multiplyWithWindowingTable (specData.data(), (size_t) specSize);
        specFft.performFrequencyOnlyForwardTransform (specData.data());
        const double rate = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 48000.0;
        for (size_t i = 1; i < specSmooth.size(); ++i)
        {
            const double hz = (double) i * rate / specSize;
            const float db = juce::Decibels::gainToDecibels (specData[i] * 4.0f / (float) specSize, -120.0f)
                           + 3.0f * (float) std::log2 (juce::jmax (1.0, hz) / 1000.0);
            specSmooth[i] = juce::jmax (db, specSmooth[i] - 1.4f);
        }
        pulse = std::fmod (pulse + 0.035f, 1.0f);
    }
   #endif
    repaint();
}

juce::Colour CharEqScreen::bandColour (int t, int band) const
{
    if (t == (int) EqType::TubeP1)
    {
        static const juce::uint32 p1[3] { 0xffffa63d, 0xff4fe3ff, 0xffc58bff };
        return juce::Colour (p1[juce::jlimit (0, 2, band)]);
    }
    if (t >= (int) EqType::Api550)   // V74: low to high, warm to cool
    {
        static const juce::uint32 v[6] { 0xffffa63d, 0xffff7a4f, 0xffff5e7a, 0xffc58bff, 0xff4fe3ff, 0xff6c8cff };
        const int n = (int) bandsFor (t).size();
        if (t == (int) EqType::K9000) { static const juce::uint32 k[6] { 0xff6c8cff, 0xffffa63d, 0xffff5e7a, 0xffc58bff, 0xff4fe3ff, 0xff6c8cff }; return juce::Colour (k[juce::jlimit (0, 5, band)]); }
        return juce::Colour (v[juce::jlimit (0, 5, n <= 4 ? band + (band >= 2 ? 1 : 0) : band)]);
    }
    //                               high-pass   body        mud         presence    air
    static const juce::uint32 c[5] { 0xff6c8cff, 0xffffa63d, 0xffff5e7a, 0xffc58bff, 0xff4fe3ff };
    return juce::Colour (c[juce::jlimit (0, 4, band)]);
}

void CharEqScreen::paintV43 (juce::Graphics& g)
{
    const int t = types.getSelected();
    const auto bands = bandsFor (t);
    selected = juce::jlimit (0, (int) bands.size() - 1, selected);
    const auto s = proc.readSettings();
    const bool on = s.eqOn && s.charEq[(size_t) t].on;
    const float life = on ? 1.0f : 0.45f;            // everything dims while this EQ is switched out
    const float glow = on ? 0.55f + 0.45f * specLevel : 0.3f;
    const auto selCol = bandColour (t, selected);

    // screen: deep blue-black with a soft light from the top
    auto all = getLocalBounds().toFloat();
    {
        juce::ColourGradient bg (juce::Colour (0xff12141c), all.getCentreX(), all.getY(),
                                 juce::Colour (0xff050507), all.getCentreX(), all.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (all, 6.0f);
        juce::ColourGradient halo (selCol.withAlpha (0.10f * life), all.getCentreX(), all.getY() + all.getHeight() * 0.35f,
                                   juce::Colours::transparentBlack, all.getCentreX() + all.getWidth() * 0.75f, all.getY() + all.getHeight() * 0.35f, true);
        g.setGradientFill (halo);
        g.fillRoundedRectangle (all, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (all.reduced (0.5f), 6.0f, 1.0f);
    }
    const auto plot = plotArea();
    const float zero = yForDb (0.0f);

    // grid: fine dotted lines, brighter at the decades
    g.setFont (screenFont (10.5f, 0));
    for (double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = xForHz (hz);
        const bool major = hz == 100.0 || hz == 1000.0 || hz == 10000.0;
        g.setColour (juce::Colours::white.withAlpha (major ? 0.09f : 0.045f));
        for (float y = plot.getY() + 4.0f; y < plot.getBottom() - 16.0f; y += 4.0f) g.fillRect (x, y, 1.0f, 2.0f);
        g.setColour (juce::Colour (0xff7a7d88));
        g.drawText (hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (x - 16.0f, plot.getBottom() - 14.0f, 32.0f, 12.0f), juce::Justification::centred);
    }
    for (float db : { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f })
    {
        const float y = yForDb (db);
        g.setColour (juce::Colours::white.withAlpha (db == 0.0f ? 0.16f : 0.05f));
        g.drawHorizontalLine ((int) y, plot.getX() + 4.0f, plot.getRight() - 4.0f);
        g.setColour (juce::Colour (0xff7a7d88));
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getRight() - 26.0f, y - 12.0f, 22.0f, 11.0f),
                    juce::Justification::centredRight);
    }

    // live spectrum (after the Character EQs), rising from the floor
    {
        const double rate = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 48000.0;
        const float floorY = plot.getBottom() - 16.0f;
        auto yForSpec = [&] (float db) { return juce::jmap (juce::jlimit (-84.0f, 0.0f, db), -84.0f, 0.0f, floorY, plot.getY() + plot.getHeight() * 0.32f); };
        juce::Path sp;
        sp.startNewSubPath (plot.getX(), floorY);
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const double b1 = hzForX (x) * specSize / rate, b2 = hzForX (x + 2.0f) * specSize / rate;
            const int i1 = juce::jlimit (1, (int) specSmooth.size() - 1, (int) b1);
            const int i2 = juce::jlimit (i1, (int) specSmooth.size() - 1, (int) b2);
            float db = -120.0f;
            for (int i = i1; i <= i2; ++i) db = juce::jmax (db, specSmooth[(size_t) i]);
            sp.lineTo (x, yForSpec (db));
        }
        sp.lineTo (plot.getRight(), floorY);
        sp.closeSubPath();
        juce::ColourGradient sg (juce::Colour (0xff7f8cff).withAlpha (0.16f * life), 0.0f, plot.getY() + plot.getHeight() * 0.35f,
                                 juce::Colour (0xff7f8cff).withAlpha (0.0f), 0.0f, floorY, false);
        g.setGradientFill (sg);
        g.fillPath (sp);
        g.setColour (juce::Colour (0xffaab4ff).withAlpha (0.16f * life));
        g.strokePath (sp, juce::PathStrokeType (1.0f));
    }

    // curves
    auto curve = [&] (int unit)
    {
        const auto d = VocalEQ::design (s.charEq[(size_t) unit], (EqType) unit, sr);
        juce::Path p;
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const float y = yForDb (VocalEQ::magnitudeDb (d, hzForX (x), sr));
            if (x == plot.getX()) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        return p;
    };
    for (int u = 0; u < eqTypeNames.size(); ++u)
        if (u != t && s.eqOn && s.charEq[(size_t) u].on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.strokePath (curve (u), juce::PathStrokeType (1.0f));
        }

    // a horizontal gradient through every band's colour, so the curve changes colour as it passes each band
    std::vector<juce::Point<float>> dots;
    for (int i = 0; i < (int) bands.size(); ++i) dots.push_back (dotFor (i));
    juce::ColourGradient rainbow (bandColour (t, 0), plot.getX(), 0.0f, bandColour (t, (int) bands.size() - 1), plot.getRight(), 0.0f, false);
    for (int i = 0; i < (int) bands.size(); ++i)
    {
        const double pos = juce::jlimit (0.01, 0.99, (double) ((dots[(size_t) i].x - plot.getX()) / plot.getWidth()));
        rainbow.addColour (pos, bandColour (t, i));
    }
    auto withAlpha = [] (juce::ColourGradient cg, float a)
    {
        for (int i = 0; i < cg.getNumColours(); ++i) cg.setColour (i, cg.getColour (i).withMultipliedAlpha (a));
        return cg;
    };

    // light beams: a column of colour from the 0 dB line to each band's dot
    for (int i = 0; i < (int) bands.size(); ++i)
    {
        if (juce::String (bands[(size_t) i].freq) == "HpfFreq") continue;
        const auto pt = dots[(size_t) i];
        if (std::abs (pt.y - zero) < 2.0f) continue;
        const auto col = bandColour (t, i);
        const float w = i == selected ? 26.0f : 18.0f;
        auto beam = juce::Rectangle<float> (pt.x - w * 0.5f, juce::jmin (pt.y, zero), w, std::abs (pt.y - zero));
        juce::ColourGradient bg (col.withAlpha (0.38f * life * glow), pt.x, pt.y, col.withAlpha (0.0f), pt.x, zero, false);
        g.setGradientFill (bg);
        g.fillRect (beam);
    }

    // this EQ: fill under the curve, then a neon stroke (wide soft glow, colour line, white-hot core)
    {
        auto p = curve (t);
        auto fill = p;
        fill.lineTo (plot.getRight(), zero); fill.lineTo (plot.getX(), zero); fill.closeSubPath();
        g.setGradientFill (withAlpha (rainbow, 0.16f * life));
        g.fillPath (fill);
        const juce::PathStrokeType::JointStyle js = juce::PathStrokeType::curved;
        for (auto [width, alpha] : { std::pair<float, float> { 14.0f, 0.06f }, { 8.0f, 0.12f }, { 4.5f, 0.28f } })
        {
            g.setGradientFill (withAlpha (rainbow, alpha * glow * life));
            g.strokePath (p, juce::PathStrokeType (width, js, juce::PathStrokeType::rounded));
        }
        g.setGradientFill (withAlpha (rainbow, life));
        g.strokePath (p, juce::PathStrokeType (2.2f, js, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.45f * glow * life));
        g.strokePath (p, juce::PathStrokeType (0.8f, js, juce::PathStrokeType::rounded));
    }

    // dots: a halo, a coloured ring and a bright centre; the selected one breathes
    std::vector<juce::Rectangle<float>> tagRects;   // V74: names that would overlap move under their dot
    juce::ignoreUnused (tagRects);
    for (int i = 0; i < (int) bands.size(); ++i)
    {
        const auto pt = dots[(size_t) i];
        const auto col = bandColour (t, i).withMultipliedAlpha (life);
        const bool sel = i == selected, hov = i == hover;
        const float r = sel ? 7.5f : (hov ? 7.0f : 6.0f);
        const float haloR = r * (sel ? 3.2f + 0.6f * std::sin (pulse * juce::MathConstants<float>::twoPi) : 2.4f);
        juce::ColourGradient hg (col.withAlpha ((sel ? 0.55f : 0.32f) * glow * life), pt.x, pt.y,
                                 col.withAlpha (0.0f), pt.x + haloR, pt.y, true);
        g.setGradientFill (hg);
        g.fillEllipse (juce::Rectangle<float> (haloR * 2.0f, haloR * 2.0f).withCentre (pt));
        g.setColour (juce::Colour (0xff0a0b0f));
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt));
        g.setColour (col);
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt), sel ? 2.4f : 1.8f);
        g.setColour (sel ? juce::Colours::white.withAlpha (life) : col);
        g.fillEllipse (juce::Rectangle<float> (r * 0.9f, r * 0.9f).withCentre (pt));

        g.setColour (sel ? col : col.withMultipliedAlpha (0.75f));
        g.setFont (screenFont (10.0f, 1).withExtraKerningFactor (0.1f));
        const juce::String tagText (bands[(size_t) i].name);
        auto tag = juce::Rectangle<float> (MISHKAL_V74 ? 7.0f * (float) tagText.length() + 8.0f : 90.0f, 12.0f).withCentre ({ pt.x, pt.y - r - 11.0f });
        if (tag.getY() < plot.getY() + 16.0f) tag.setY (pt.y + r + 5.0f);    // flip under the dot near the top
        tag = tag.withX (juce::jlimit (plot.getX() + 2.0f, plot.getRight() - tag.getWidth() - 2.0f, tag.getX()));
       #if MISHKAL_V74
        for (auto& prev : tagRects)   // V74: bands close together (LE-M4's SUB and 40): put this name under its dot
            if (prev.intersects (tag)) { tag.setY (pt.y + r + 5.0f); break; }
        tagRects.push_back (tag);
       #endif
        g.drawText (tagText, tag, juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff5d6070));
    g.setFont (screenFont (10.5f, 0));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("drag a dot  \xc2\xb7  scroll for width / Q  \xc2\xb7  double-click to reset")), plot.reduced (8.0f, 6.0f), juce::Justification::topLeft);

    // readouts for the selected band, in its colour
    auto strip = getLocalBounds().toFloat().removeFromBottom (78.0f).reduced (8.0f, 8.0f);
    {
        juce::ColourGradient rg (selCol.withAlpha (0.16f * life), strip.getX(), strip.getY(),
                                 juce::Colour (0xff050608), strip.getX() + strip.getWidth() * 0.6f, strip.getY(), false);
        g.setGradientFill (rg);
        g.fillRoundedRectangle (strip, 5.0f);
        g.setColour (selCol.withAlpha (0.35f * life));
        g.drawRoundedRectangle (strip.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (selCol.withMultipliedAlpha (life));
        g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, strip.getHeight() - 16.0f).withPosition (strip.getX() + 5.0f, strip.getY() + 8.0f), 1.5f);
    }
    auto inner = strip.reduced (14.0f, 6.0f);
    g.setColour (selCol.withMultipliedAlpha (life));
    g.setFont (font (10.0f, true).withExtraKerningFactor (0.14f));
    g.drawText (bands[(size_t) selected].name, inner.removeFromTop (12.0f), juce::Justification::centredLeft);
    const auto fields = fieldsFor (t, selected);
    fieldRects.clear();
    const float fw = inner.getWidth() / (float) fields.size();
    for (int i = 0; i < (int) fields.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (inner.getX() + fw * (float) i, inner.getY(), fw, inner.getHeight());
        fieldRects.push_back (cell);
        auto* p = param (fields[(size_t) i].id);
        g.setColour (juce::Colour (0xff8a8d99));
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.12f));
        g.drawText (fields[(size_t) i].label, cell.removeFromTop (12.0f), juce::Justification::centredLeft);
        juce::String v = p != nullptr ? compactValue (p->getCurrentValueAsText()) : juce::String ("-");
        if (juce::String (fields[(size_t) i].label) == "FREQ" && v != "Off")
            v = v.endsWithChar ('k') ? v.dropLastCharacters (1) + " kHz" : v + " Hz";
        g.setFont (font (18.0f, true));
        g.setColour (selCol.withAlpha (0.25f * glow * life));      // soft glow behind the digits
        g.drawText (v, cell.translated (0.0f, 1.0f).expanded (1.0f, 0.0f), juce::Justification::centredLeft);
        g.setColour (i == dragField ? juce::Colours::white : selCol.brighter (0.35f).withMultipliedAlpha (life));
        g.drawText (v, cell, juce::Justification::centredLeft);
    }
}

void CharEqScreen::paint (juce::Graphics& g)
{
    sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
   #if MISHKAL_V43
    paintV43 (g);
    return;
   #endif
    const int t = types.getSelected();
    const auto bands = bandsFor (t);
    selected = juce::jlimit (0, (int) bands.size() - 1, selected);
    const auto s = proc.readSettings();
    const bool on = s.eqOn && s.charEq[(size_t) t].on;
    const auto amber = goliathAmber();

    // screen
    auto all = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (all, 5.0f);
    g.setColour (juce::Colour (0xff1f1f22));
    g.drawRoundedRectangle (all.reduced (0.5f), 5.0f, 1.0f);
    const auto plot = plotArea();

    // grid
    g.setFont (screenFont (10.5f, 0));
    for (double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = xForHz (hz);
        g.setColour (juce::Colour (hz == 100.0 || hz == 1000.0 || hz == 10000.0 ? 0xff2a2a2d : 0xff1a1a1c));
        g.drawVerticalLine ((int) x, plot.getY() + 4.0f, plot.getBottom() - 14.0f);
        g.setColour (juce::Colour (0xff6a6a70));
        g.drawText (hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (x - 16.0f, plot.getBottom() - 14.0f, 32.0f, 12.0f), juce::Justification::centred);
    }
    for (float db : { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f })
    {
        const float y = yForDb (db);
        g.setColour (juce::Colour (db == 0.0f ? 0xff36363a : 0xff1a1a1c));
        g.drawHorizontalLine ((int) y, plot.getX() + 4.0f, plot.getRight() - 4.0f);
        g.setColour (juce::Colour (0xff6a6a70));
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getRight() - 26.0f, y - 12.0f, 22.0f, 11.0f),
                    juce::Justification::centredRight);
    }

    // the other active character EQs, faint
    auto curve = [&] (int unit)
    {
        const auto d = VocalEQ::design (s.charEq[(size_t) unit], (EqType) unit, sr);
        juce::Path p;
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const float y = yForDb (VocalEQ::magnitudeDb (d, hzForX (x), sr));
            if (x == plot.getX()) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        return p;
    };
    for (int u = 0; u < eqTypeNames.size(); ++u)
        if (u != t && s.eqOn && s.charEq[(size_t) u].on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.strokePath (curve (u), juce::PathStrokeType (1.0f));
        }

    // this EQ: amber curve with a soft fill
    {
        auto p = curve (t);
        auto fill = p;
        const float zero = yForDb (0.0f);
        fill.lineTo (plot.getRight(), zero); fill.lineTo (plot.getX(), zero); fill.closeSubPath();
        g.setColour (amber.withAlpha (on ? 0.12f : 0.05f));
        g.fillPath (fill);
        g.setColour (on ? amber : amber.withAlpha (0.55f));   // dimmer while this EQ (or the section) is switched out
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // dots
    for (int i = 0; i < (int) bands.size(); ++i)
    {
        const auto pt = dotFor (i);
        const bool sel = i == selected;
        const float r = sel ? 7.0f : (i == hover ? 6.5f : 5.5f);
        if (sel)
        {
            g.setColour (amber.withAlpha (0.3f));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f + 10.0f, r * 2.0f + 10.0f).withCentre (pt));
        }
        g.setColour (sel ? amber : juce::Colour (0xffe6e6e6));
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt));
        g.setColour (juce::Colour (0xff8e8e93));
        g.setFont (screenFont (10.0f, 1).withExtraKerningFactor (0.08f));
        auto tag = juce::Rectangle<float> (90.0f, 12.0f).withCentre ({ pt.x, pt.y - r - 10.0f });
        tag = tag.withX (juce::jlimit (plot.getX() + 2.0f, plot.getRight() - tag.getWidth() - 2.0f, tag.getX()));   // keep names on screen
        g.drawText (bands[(size_t) i].name, tag, juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff5a5a60));
    g.setFont (screenFont (10.5f, 0));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("drag a dot  \xc2\xb7  scroll for width / Q  \xc2\xb7  double-click to reset")), plot.reduced (8.0f, 6.0f), juce::Justification::topLeft);

    // readouts for the selected band
    auto strip = getLocalBounds().toFloat().removeFromBottom (78.0f).reduced (8.0f, 8.0f);
    g.setColour (juce::Colour (0xff050608));
    g.fillRoundedRectangle (strip, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (strip.reduced (0.5f), 4.0f, 1.0f);
    auto inner = strip.reduced (10.0f, 6.0f);
    g.setColour (amber.withAlpha (0.6f));
    g.setFont (font (10.0f, true).withExtraKerningFactor (0.12f));
    g.drawText (bands[(size_t) selected].name, inner.removeFromTop (12.0f), juce::Justification::centredLeft);
    const auto fields = fieldsFor (t, selected);
    fieldRects.clear();
    const float fw = inner.getWidth() / (float) fields.size();
    for (int i = 0; i < (int) fields.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (inner.getX() + fw * (float) i, inner.getY(), fw, inner.getHeight());
        fieldRects.push_back (cell.translated (0.0f, 0.0f));
        auto* p = param (fields[(size_t) i].id);
        g.setColour (amber.withAlpha (0.55f));
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.12f));
        g.drawText (fields[(size_t) i].label, cell.removeFromTop (12.0f), juce::Justification::centredLeft);
        g.setColour (i == dragField ? amber.brighter (0.4f) : amber);
        g.setFont (font (17.0f, true));
        juce::String v = p != nullptr ? compactValue (p->getCurrentValueAsText()) : juce::String ("-");
        if (juce::String (fields[(size_t) i].label) == "FREQ")
            v = v.endsWithChar ('k') ? v.dropLastCharacters (1) + " kHz" : v + " Hz";
        g.drawText (v, cell, juce::Justification::centredLeft);
    }
}

//==============================================================================
void ViewTab::paint (juce::Graphics& g)
{
    const bool sel = isSelected && isSelected();
    auto r = getLocalBounds().toFloat();
    g.setColour (sel ? goliathLegend() : (isMouseOver() ? goliathLegend().withAlpha (0.7f) : goliathLabel()));
    g.setFont (font (11.0f, true).withExtraKerningFactor (0.08f));
    g.drawText (text, r.withTrimmedBottom (3.0f), juce::Justification::centred);
    if (sel)
    {
        g.setColour (goliathAmber());
        const float tw = juce::GlyphArrangement::getStringWidth (font (11.0f, true).withExtraKerningFactor (0.08f), text);
        g.fillRoundedRectangle (juce::Rectangle<float> (tw + 6.0f, 2.0f).withCentre ({ r.getCentreX(), r.getBottom() - 1.5f }), 1.0f);
    }
}
