#include "PluginEditor.h"

using namespace vox;
using namespace vox::ui;

//==============================================================================
// Modern takes on each unit: dark panels tinted with the unit's signature colour,
// sleek knobs whose glowing arcs carry that colour.
static HwLook modernLook (juce::uint32 top, juce::uint32 bottom, juce::uint32 accent)
{
    HwLook l;
    l.active = true;
    l.modern = true;
    l.faceTop = juce::Colour (top);
    l.faceBottom = juce::Colour (bottom);
    l.accent = juce::Colour (accent);
    l.text = juce::Colour (0xffeef0f3);
    l.label = l.accent;
    l.groove = juce::Colours::black;
    l.knob = KnobStyle::ModernArc;
    l.knobCap = l.accent;
    return l;
}

HwLook vox::ui::hwLookForEq (EqType t)
{
    switch (t)
    {
        case EqType::TubeP1:   return modernLook (0xff1d2b35, 0xff111a21, 0xff8fd3f5);   // ice blue
        case EqType::EConsole: return modernLook (0xff25272b, 0xff16171a, 0xffe6e6e6);   // neutral; bands carry colour
        case EqType::ClassA81:
        {
            auto l = modernLook (0xff1c2737, 0xff101823, 0xffff5f57);   // navy, red gain knobs
            l.label = juce::Colour (0xffc9d4de);                          // silver labels so red stays special
            return l;
        }
        case EqType::Airy:
        default:               return modernLook (0xff13292e, 0xff0b171a, 0xff4fdcf2);   // deep teal, cyan
    }
}

HwLook vox::ui::hwLookForComp (CompType t)
{
    switch (t)
    {
        case CompType::FetPurple:  return modernLook (0xff2b1e40, 0xff170f24, 0xffb68eff);   // violet
        case CompType::RComp: return modernLook (0xff2a2622, 0xff171411, 0xffff9d42);   // graphite, orange
        case CompType::VariMu660:
        default:                   return modernLook (0xff2a261f, 0xff181510, 0xffe9c47c);   // warm graphite, gold
    }
}

KnobStyle vox::ui::hwLookForCompKnob (CompType t)
{
    switch (t)
    {
        case CompType::FetPurple:  return KnobStyle::BlackChrome;   // FET limiter: black knob, chrome ring
        case CompType::RComp: return KnobStyle::ChickenHead;   // vintage British studio chicken-head
        case CompType::StaLevel:
        case CompType::VariMu660:
        default:                   return KnobStyle::Bakelite;      // vari-mu: big black bakelite
    }
}

juce::Colour vox::ui::eqModeColour (EqType t)
{
    switch (t)
    {
        case EqType::TubeP1:   return juce::Colour (0xff1e88e5);   // deep blue
        case EqType::EConsole: return juce::Colour (0xff0fa64f);   // deep console green
        case EqType::ClassA81: return juce::Colour (0xffd91e18);   // deep red
        case EqType::Api550:   return juce::Colour (0xff2f6bff);   // V74: API blue
        case EqType::K9000:    return juce::Colour (0xffb8b8c0);   // V74: console silver-grey
        case EqType::Helios69: return juce::Colour (0xffe07a1f);   // V74: Helios amber
        case EqType::Maag4:    return juce::Colour (0xff8f5bff);   // V74: Maag purple
        case EqType::Airy:
        default:               return juce::Colour (0xff00a9c7);   // deep cyan
    }
}

juce::Colour vox::ui::compModeColour (CompType t)
{
    switch (t)
    {
        case CompType::FetPurple:  return juce::Colour (0xff7c3aed);   // deep violet
        case CompType::RComp: return juce::Colour (0xffef6c00);   // deep orange
        case CompType::StaLevel:   return juce::Colour (0xff4f8fbf);   // broadcast blue
        case CompType::Api2500:    return juce::Colour (0xff2f6bff);   // V75: API blue
        case CompType::SslG:       return juce::Colour (0xffd23a3a);   // V75: console red
        case CompType::WeissDs1:   return juce::Colour (0xffcfd3d8);   // V75: mastering silver
        case CompType::SplIron:    return juce::Colour (0xff8a6a4a);   // V75: iron bronze
        case CompType::Neve33609:  return juce::Colour (0xff5f7fa0);   // V75: Neve blue-grey
        case CompType::VariMu660:
        default:                   return juce::Colour (0xffd4a017);   // deep gold
    }
}

juce::Colour vox::ui::hwCapColour (EqType t, int role, bool isGain)
{
    if (t == EqType::EConsole)
    {
        // console colour code, brightened: filter gray, LF amber, LMF blue, HMF green, HF red
        static const juce::uint32 caps[5] { 0xffb9bec5, 0xffe59a5c, 0xff4f9dff, 0xff4fd38c, 0xffff5a50 };
        return juce::Colour (caps[juce::jlimit (0, 4, role)]);
    }
    if (t == EqType::ClassA81)
        return isGain ? juce::Colour (0xffff5f57) : juce::Colour (0xffc9d4de);   // red gain, silver frequency
    return hwLookForEq (t).accent;
}

//==============================================================================
static const juce::Image& hammerTileHD()   // V57: hammertone at 2x for Retina
{
    static juce::Image tile = []
    {
        constexpr int n = 256;
        juce::Image img (juce::Image::ARGB, n, n, true);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        juce::Random rng (11);
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                const float fx = (float) x * 0.5f, fy = (float) y * 0.5f;   // same cell size as 1x, twice the detail
                const float v = std::sin (fx * 0.37f + fy * 0.11f) * std::sin (fy * 0.29f - fx * 0.07f)
                              + (rng.nextFloat() - 0.5f) * 0.8f;
                bd.setPixelColour (x, y, v > 0.0f ? juce::Colours::white.withAlpha (juce::jmin (0.09f, v * 0.06f))
                                                   : juce::Colours::black.withAlpha (juce::jmin (0.14f, -v * 0.09f)));
            }
        return img;
    }();
    return tile;
}

static const juce::Image& hammerTile()
{
    static juce::Image tile = []
    {
        juce::Image img (juce::Image::ARGB, 128, 128, true);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        juce::Random rng (11);
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
            {
                // soft cellular speckle, like crackle/hammertone paint
                const float v = std::sin ((float) x * 0.37f + (float) y * 0.11f) * std::sin ((float) y * 0.29f - (float) x * 0.07f)
                              + (rng.nextFloat() - 0.5f) * 0.9f;
                bd.setPixelColour (x, y, v > 0.0f ? juce::Colours::white.withAlpha (juce::jmin (0.09f, v * 0.06f))
                                                   : juce::Colours::black.withAlpha (juce::jmin (0.14f, -v * 0.09f)));
            }
        return img;
    }();
    return tile;
}

void vox::ui::paintFaceplate (juce::Graphics& g, juce::Rectangle<float> r, const HwLook& l, float corner)
{
    if (l.modern)
    {
        g.setGradientFill (juce::ColourGradient (l.faceTop, 0.0f, r.getY(), l.faceBottom, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        // soft glow of the signature colour behind the header
        g.setGradientFill (juce::ColourGradient (l.accent.withAlpha (0.13f), r.getX() + r.getWidth() * 0.18f, r.getY(),
                                                 juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.18f + r.getWidth() * 0.45f,
                                                 r.getY(), true));
        g.fillRoundedRectangle (r, corner);
        // hairline rim: accent at the top fading out
        g.setGradientFill (juce::ColourGradient (l.accent.withAlpha (0.45f), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.06f), r.getX(), r.getY() + r.getHeight() * 0.6f, false));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
        // accent bar under the title area
        g.setColour (l.accent);
        g.fillRoundedRectangle (juce::Rectangle<float> (r.getX() + 14.0f, r.getY(), 40.0f, 2.5f), 1.25f);
        return;
    }

    g.setGradientFill (juce::ColourGradient (l.faceTop, 0.0f, r.getY(), l.faceBottom, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);

    if (l.hammertone)
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 57
        {
            juce::FillType ft (hammerTileHD(), juce::AffineTransform::scale (0.5f));
            ft.setOpacity (0.55f);
            g.setFillType (ft);
        }
       #else
        g.setTiledImageFill (hammerTile(), 0, 0, 0.55f);
       #endif
    else
        setBrushFill (g, l.lightPlate ? 0.35f : 0.55f);
    g.fillRoundedRectangle (r, corner);

    // soft top light and edge bevel
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (l.lightPlate ? 0.18f : 0.07f), 0.0f, r.getY(),
                                             juce::Colours::transparentWhite, 0.0f, r.getY() + r.getHeight() * 0.5f, false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (l.lightPlate ? 0.45f : 0.12f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
}

//==============================================================================
bool vox::ui::drawHardwareKnob (juce::Graphics& g, juce::Slider& s, juce::Rectangle<float> area, float pos,
                                float a0, float a1, KnobStyle style, juce::Colour cap, juce::Colour text)
{
    if (style == KnobStyle::Skin) return false;

    using PST = juce::PathStrokeType;
    const bool enabled = s.isEnabled();
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float radius = d * 0.5f;
    const float angle = a0 + pos * (a1 - a0);
    auto circle = [&c] (float r) { return juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };
    const auto xf = juce::AffineTransform::rotation (angle).translated (c);

    if (style == KnobStyle::ModernArc)
    {
        // Sleek modern knob: thin track, glowing value arc, matte body, lit pointer.
        const bool bipolar = s.getProperties()["bipolar"];
        const auto col = enabled ? cap : cap.withSaturation (0.0f).withAlpha (0.4f);
        const bool showEnds = radius >= 34.0f;
        const float arcR = radius - (showEnds ? 9.0f : 4.0f);

        // quiet min / max numbers at the arc ends
        if (showEnds)
        {
            g.setFont (font (9.0f, true));
            g.setColour (text.withAlpha (enabled ? 0.45f : 0.25f));
            for (float prop : { 0.0f, 1.0f })
            {
                const auto txt = compactValue (s.getTextFromValue (s.proportionOfLengthToValue (prop)));
                const auto pt = c.getPointOnCircumference (arcR + 3.0f, a0 + (a1 - a0) * prop).translated (0.0f, 9.0f);
                g.drawText (txt, juce::Rectangle<float> (34.0f, 11.0f).withCentre (pt), juce::Justification::centred);
            }
        }

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.strokePath (track, PST (3.0f, PST::curved, PST::rounded));

        const float from = bipolar ? (a0 + a1) * 0.5f : a0;
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        if (enabled) drawGlowStroke (g, val, col, 3.0f);
        else { g.setColour (col); g.strokePath (val, PST (3.0f, PST::curved, PST::rounded)); }

        // matte body with soft shadow and a fine top rim
        const float br = arcR - 7.0f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (circle (br).translated (0.0f, 3.0f).expanded (2.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff33363d), c.x, c.y - br, juce::Colour (0xff15171b), c.x, c.y + br, false));
        g.fillEllipse (circle (br));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), c.x, c.y - br,
                                                 juce::Colours::transparentWhite, c.x, c.y + br * 0.2f, false));
        g.drawEllipse (circle (br - 0.5f), 1.0f);

        // pointer: short lit line near the edge
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.5f, -br * 0.88f, 3.0f, br * 0.42f, 1.5f);
        if (enabled)
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillPath (ptr, xf);
            juce::Path glow;
            glow.addRoundedRectangle (-3.0f, -br * 0.9f, 6.0f, br * 0.46f, 3.0f);
            g.setColour (col.withAlpha (0.18f));
            g.fillPath (glow, xf);
        }
        g.setColour (enabled ? col.brighter (0.25f) : col);
        g.fillPath (ptr, xf);
        return true;
    }

    // ---- printed scale -----------------------------------------------------
    const int labels = radius >= 48.0f ? 5 : radius >= 36.0f ? 3 : 0;
    const float scaleR = radius - (labels > 0 ? 4.0f : 0.0f);
    g.setFont (font (labels == 5 ? 9.5f : 9.0f, true));
    for (int i = 0; i < labels; ++i)
    {
        const float prop = (float) i / (float) (labels - 1);
        const auto txt = compactValue (s.getTextFromValue (s.proportionOfLengthToValue (prop)));
        g.setColour (text.withAlpha (enabled ? 0.9f : 0.4f));
        g.drawText (txt, juce::Rectangle<float> (34.0f, 11.0f).withCentre (c.getPointOnCircumference (scaleR, a0 + (a1 - a0) * prop)),
                    juce::Justification::centred);
    }
    const float tickR = labels > 0 ? scaleR - (labels == 5 ? 8.0f : 6.5f) : radius - 3.0f;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        g.setColour (text.withAlpha (enabled ? 0.75f : 0.3f));
        g.drawLine ({ c.getPointOnCircumference (tickR - 2.5f, a), c.getPointOnCircumference (tickR + 1.5f, a) }, 1.3f);
    }
    const float kr = tickR - 3.0f;

    // shared drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (circle (kr).translated (0.0f, 3.0f).expanded (1.5f));

    auto ridgedPath = [&c] (float r, int ridges, float depth)
    {
        juce::Path p;
        for (int i = 0; i <= ridges * 8; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) (ridges * 8);
            const float rr = r * (1.0f - depth * (0.5f + 0.5f * std::cos ((float) ridges * th)));
            const auto pt = c.getPointOnCircumference (rr, th);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    };
    auto pointerLine = [&] (float from, float to, float w, juce::Colour col)
    {
        juce::Path p;
        p.addRoundedRectangle (-w * 0.5f, -to, w, to - from, w * 0.5f);
        g.setColour (enabled ? col : col.withAlpha (0.4f));
        g.fillPath (p, xf);
    };
    auto specular = [&] (float r, float amount)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (amount), c.x - r * 0.35f, c.y - r * 0.65f,
                                                 juce::Colours::transparentWhite, c.x, c.y + r * 0.15f, true));
        g.fillEllipse (circle (r).reduced (r * 0.08f));
    };

    switch (style)
    {
        case KnobStyle::Pointer:
        {
            // pointer tip protruding from the knob, then a fluted black body
            juce::Path tip;
            tip.addTriangle (-kr * 0.30f, -kr * 0.62f, kr * 0.30f, -kr * 0.62f, 0.0f, -kr * 1.12f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillPath (tip, xf.translated (0.0f, 2.0f));
            g.setColour (juce::Colour (0xff151515));
            g.fillPath (tip, xf);
            auto body = ridgedPath (kr * 0.86f, 18, 0.035f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x, c.y - kr, juce::Colour (0xff050505), c.x, c.y + kr, false));
            g.fillPath (body);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.strokePath (body, PST (1.0f));
            specular (kr * 0.7f, 0.18f);
            pointerLine (kr * 0.15f, kr * 1.05f, 2.6f, juce::Colours::white);
            break;
        }
        case KnobStyle::CapColour:
        {
            auto body = ridgedPath (kr, 24, 0.045f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d71), c.x, c.y - kr, juce::Colour (0xff26282b), c.x, c.y + kr, false));
            g.fillPath (body);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.strokePath (body, PST (1.0f));
            const float capR = kr * 0.62f;
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (circle (capR).translated (0.0f, 1.2f));
            g.setGradientFill (juce::ColourGradient (cap.brighter (0.35f), c.x, c.y - capR, cap.darker (0.35f), c.x, c.y + capR, false));
            g.fillEllipse (circle (capR));
            specular (capR, 0.25f);
            pointerLine (kr * 0.12f, kr * 0.98f, 2.4f, juce::Colours::white);
            break;
        }
        case KnobStyle::RedCap:
        {
            auto skirt = ridgedPath (kr, 30, 0.03f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd2d5d8), c.x, c.y - kr, juce::Colour (0xff7e8387), c.x, c.y + kr, false));
            g.fillPath (skirt);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.strokePath (skirt, PST (1.0f));
            pointerLine (kr * 0.66f, kr * 0.98f, 2.4f, juce::Colour (0xff1a1a1a));   // index line on the skirt
            const float capR = kr * 0.6f;
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillEllipse (circle (capR).translated (0.0f, 1.5f));
            g.setGradientFill (juce::ColourGradient (cap.brighter (0.3f), c.x, c.y - capR, cap.darker (0.45f), c.x, c.y + capR, false));
            g.fillEllipse (circle (capR));
            specular (capR, 0.3f);
            const bool redCap = cap.getRed() > cap.getBlue() + 40;
            pointerLine (capR * 0.15f, capR * 0.95f, 2.2f, redCap ? juce::Colours::white : juce::Colour (0xff111111));
            break;
        }
        case KnobStyle::Aluminium:
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f4f5), c.x - kr * 0.4f, c.y - kr * 0.8f,
                                                     juce::Colour (0xff8a9197), c.x + kr * 0.4f, c.y + kr, false));
            g.fillEllipse (circle (kr));
            for (float rr = kr * 0.15f; rr < kr * 0.95f; rr += 1.6f)   // turned-metal rings
            {
                g.setColour (juce::Colours::black.withAlpha (0.035f));
                g.drawEllipse (circle (rr), 0.6f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawEllipse (circle (kr), 1.2f);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawEllipse (circle (kr - 1.4f), 0.8f);
            pointerLine (kr * 0.25f, kr * 0.9f, 2.6f, juce::Colour (0xff1d2328));
            g.setColour (juce::Colour (0xff4fc7e0));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (c.getPointOnCircumference (kr * 0.92f + 5.0f, angle)));
            break;
        }
        case KnobStyle::BlackChrome:
        {
            // chrome ring
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f0f0), c.x - kr, c.y - kr,
                                                     juce::Colour (0xff6b6e72), c.x + kr, c.y + kr, false));
            g.fillEllipse (circle (kr));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa9adb1), c.x + kr, c.y - kr,
                                                     juce::Colours::transparentWhite, c.x, c.y, false));
            g.fillEllipse (circle (kr));
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawEllipse (circle (kr), 1.0f);
            // glossy black knob
            const float in = kr * 0.8f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff353535), c.x, c.y - in, juce::Colour (0xff040404), c.x, c.y + in, false));
            g.fillEllipse (circle (in));
            specular (in, 0.22f);
            pointerLine (kr * 0.1f, kr * 0.95f, 2.6f, juce::Colours::white);
            break;
        }
        case KnobStyle::FlatGray:
        {
            g.setColour (cap.brighter (0.25f));
            g.fillEllipse (circle (kr));
            g.setColour (cap);
            g.fillEllipse (circle (kr - 2.0f));
            g.setColour (cap.darker (0.25f));
            g.fillEllipse (circle (kr * 0.55f));
            pointerLine (kr * 0.3f, kr * 0.92f, 3.2f, juce::Colour (0xffff8a2a));
            break;
        }
        case KnobStyle::ModernArc:
        {
            // handled above (needs its own scale treatment)
            break;
        }
        case KnobStyle::ChickenHead:
        {
            // round black base with a soft rim
            const float baseR = kr * 0.78f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2f2f32), c.x, c.y - baseR,
                                                     juce::Colour (0xff070707), c.x, c.y + baseR, false));
            g.fillEllipse (circle (baseR));
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawEllipse (circle (baseR - 0.6f), 1.0f);

            // ivory chicken-head: wide rounded heel, tapering to a rounded pointer tip
            const float tipLen = kr * 1.05f, heelLen = kr * 0.42f;
            const float tipW = kr * 0.11f, midW = kr * 0.25f, heelW = kr * 0.35f;
            juce::Path head;
            head.startNewSubPath (-tipW, -tipLen + tipW);
            head.quadraticTo (0.0f, -tipLen - tipW * 0.6f, tipW, -tipLen + tipW);   // rounded tip
            head.lineTo (midW, -kr * 0.05f);
            head.quadraticTo (heelW, heelLen * 0.5f, heelW * 0.6f, heelLen);        // heel curve
            head.quadraticTo (0.0f, heelLen + heelW * 0.45f, -heelW * 0.6f, heelLen);
            head.quadraticTo (-heelW, heelLen * 0.5f, -midW, -kr * 0.05f);
            head.closeSubPath();

            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillPath (head, xf.translated (1.0f, 3.0f));
            const auto ivory = enabled ? juce::Colour (0xfff1e7cf) : juce::Colour (0xff8f897c);
            // shade across the pointer so it reads as a moulded shape
            g.setGradientFill (juce::ColourGradient (ivory.brighter (0.15f), c.x - kr * 0.4f, c.y - kr * 0.5f,
                                                     ivory.darker (0.35f), c.x + kr * 0.5f, c.y + kr * 0.6f, false));
            g.fillPath (head, xf);
            g.setColour (juce::Colour (0xff6b604c).withAlpha (0.7f));
            g.strokePath (head, PST (1.0f), xf);

            // engraved index line along the pointer
            juce::Path groove;
            groove.addRoundedRectangle (-1.1f, -tipLen + tipW * 1.4f, 2.2f, tipLen * 0.62f, 1.1f);
            g.setColour (juce::Colour (0xff1a1712));
            g.fillPath (groove, xf);

            // small centre boss
            g.setGradientFill (juce::ColourGradient (ivory.brighter (0.2f), c.x, c.y - 5.0f, ivory.darker (0.3f), c.x, c.y + 5.0f, false));
            g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (c));
            g.setColour (juce::Colour (0xff6b604c).withAlpha (0.6f));
            g.drawEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (c), 0.8f);
            break;
        }
        case KnobStyle::KnurledBlack:   // V76: matte black knurled skirt, a flat top a little lighter, a white index line
        {
            auto body = ridgedPath (kr, 40, 0.025f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillPath (body, juce::AffineTransform::translation (0.0f, 1.6f));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2c2f), c.x, c.y - kr, juce::Colour (0xff0a0a0b), c.x, c.y + kr, false));
            g.fillPath (body);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.strokePath (body, PST (0.8f));
            const float topR = kr * 0.74f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3b3f), c.x - topR * 0.5f, c.y - topR, juce::Colour (0xff141416), c.x + topR * 0.4f, c.y + topR, false));
            g.fillEllipse (circle (topR));
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawEllipse (circle (topR - 0.5f), 1.0f);
            specular (topR, 0.10f);
            pointerLine (kr * 0.18f, kr * 0.97f, 2.6f, juce::Colours::white);
            break;
        }
        case KnobStyle::Bakelite:
        default:
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e31), c.x, c.y - kr, juce::Colour (0xff060606), c.x, c.y + kr, false));
            g.fillEllipse (circle (kr));
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.drawEllipse (circle (kr - 0.6f), 1.2f);
            pointerLine (kr * 0.66f, kr * 0.98f, 3.0f, juce::Colour (0xfff3ead2));
            const float capR = kr * 0.66f;
            g.setColour (juce::Colour (0xff121213));
            g.fillEllipse (circle (capR));
            for (int i = 0; i < 44; ++i)
            {
                const float th = juce::MathConstants<float>::twoPi * (float) i / 44.0f;
                g.setColour (juce::Colours::white.withAlpha (0.05f + 0.2f * (0.5f + 0.5f * std::cos (th + 2.2f))));
                g.drawLine ({ c.getPointOnCircumference (capR * 0.86f, th), c.getPointOnCircumference (capR - 0.4f, th) }, 1.0f);
            }
            const float topR = capR * 0.84f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2f), c.x, c.y - topR, juce::Colour (0xff0b0b0c), c.x, c.y + topR, false));
            g.fillEllipse (circle (topR));
            specular (topR, 0.14f);
            pointerLine (topR * 0.4f, topR * 0.98f, 2.6f, juce::Colour (0xfff3ead2));
            break;
        }
    }
    return true;
}
