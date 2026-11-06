#pragma once

// Skins: three complete looks for Mishkal, switchable at runtime.
// Every component reads its colours from vox::ui::pal at paint time, so
// applySkin() + a repaint restyles the whole plugin.

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace vox::ui
{
    enum class SkinStyle { Onyx = 0, Rack, Hardware, Modern, Royal,
                           BritishConsole, MasteringSilver, VintageCream, BroadcastGreen, StudioWhite, MidnightGlow, Goliath };
    inline const juce::StringArray skinNames { "Onyx", "Rack", "Hardware", "Modern", "Royal",
                                               "British Console", "Mastering Silver", "Vintage Cream",
                                               "Broadcast Green", "Studio White", "Midnight Glow", "Goliath" };
    inline constexpr int numSkins = 12;

    struct SkinDef
    {
        juce::uint32 bg, bg2, panel, panelHi, line, text, dim, screen, screen2;
        juce::uint32 clean, reso, eq, comp, deess, limiter, io;
        std::array<juce::uint32, 8> bands;
        juce::uint32 eqLow = 0, eqHigh = 0;   // 0 = same as eq
        juce::uint32 meter = 0xffef8a1f;      // LED meter colour (rack-style skins)
        juce::uint32 alert = 0xffff6a3d;      // lamps + gain-reduction meters (rack-style skins)
        // Hardware-style knobs: skirt and cap gradients (top, bottom) and pointer; 0 = the default bakelite
        juce::uint32 skirtTop = 0, skirtBot = 0, capTop = 0, capBot = 0, pointer = 0;
    };

    inline const SkinDef& skinDef (SkinStyle s)
    {
        static const SkinDef defs[numSkins] = {
            // Onyx: matte black and gunmetal, vintage black knobs, one signal-yellow accent
            { 0xff09090a, 0xff141416, 0xff1e1e20, 0xff28282b, 0xff3b3b3f, 0xffe9e9ea, 0xff8b8b90, 0xff060607, 0xff0f0f11,
              0xffd6d6d9, 0xffe0352b, 0xffe6c23c, 0xffbcbcc0, 0xffbcbcc0, 0xffbcbcc0, 0xffbcbcc0,
              { 0xffe6c23c, 0xffe0352b, 0xfff0dc7a, 0xffff6a4d, 0xffd6a43f, 0xffb8262a, 0xfff2f2f2, 0xffff9a3c },
              0, 0, 0xffe6c23c, 0xffe0352b },
            // Rack: black rack screen over a charcoal faceplate, colour-coded grip knobs
            { 0xff111113, 0xff1c1c1f, 0xff2b2b2e, 0xff333337, 0xff4a4a4f, 0xfff3f3f3, 0xff9d9da3, 0xff0b0c0e, 0xff131418,
              0xff24b9da, 0xffff6fae, 0xffe9b52f, 0xff3cb985, 0xff9b72e3, 0xffd4d4d6, 0xffd4d4d6,
              { 0xffd8443c, 0xffe9b52f, 0xff24b9da, 0xff3cb985, 0xffff6fae, 0xff9b72e3, 0xffff9a3c, 0xffb6e04a },
              0xffd5302c, 0xff2f87e3 },
            // Hardware: gunmetal faceplates, cream legends, warm vintage lamps
            { 0xff2a2928, 0xff363431, 0xff3d3b38, 0xff4c4945, 0xff5d5953, 0xfff1e7d0, 0xffb3a993, 0xff121512, 0xff1a201b,
              0xff9cc3d5, 0xffe39ab8, 0xffecad4f, 0xff9ccfae, 0xffc2abe6, 0xffe0684c, 0xfff1e7d0,
              { 0xff9cc3d5, 0xff9ccfae, 0xffecd27a, 0xffeca068, 0xffe3869a, 0xffc2abe6, 0xff8fd0d6, 0xffc7d98a } },
            // Modern: deep ink, glass panels, luminous accents
            { 0xff0a0c13, 0xff161b2c, 0xff131826, 0xff1b2133, 0xff2a3149, 0xffeef1ff, 0xff8890ad, 0xff080a12, 0xff121a2c,
              0xff45c8ff, 0xffff5ccf, 0xffffc84a, 0xff2ee6c5, 0xffa58bff, 0xffff6370, 0xffd5dbff,
              { 0xff45c8ff, 0xff4ff0a0, 0xffffd54a, 0xffff9a4a, 0xffff5c8a, 0xffb07bff, 0xff3ee6e6, 0xffc4f04f } },
            // Royal: deep purple, warm gold, candlelight glow
            { 0xff120a1e, 0xff28153f, 0xff1d1130, 0xff281842, 0xff3d2a5a, 0xfff7efdc, 0xffab98c6, 0xff0d0716, 0xff1d1031,
              0xff9db6ff, 0xffff8fc8, 0xffeac36b, 0xff7fd8c4, 0xffc3a0ff, 0xffff8b6a, 0xffeac36b,
              { 0xffeac36b, 0xff9db6ff, 0xffff8fc8, 0xff7fd8c4, 0xffc3a0ff, 0xffffb36b, 0xfff2e39b, 0xffff8b6a } },
            // British Console: grey-blue enamel, white legends, grey skirts with red caps
            { 0xff35404b, 0xff3f4b57, 0xff4f5d6b, 0xff5d6c7b, 0xff6f7f8f, 0xfff4f4ef, 0xffbfc8d0, 0xff10141a, 0xff171d25,
              0xff8fd0ff, 0xffff8a8a, 0xffffd166, 0xff9fe0b0, 0xffc9b3ff, 0xffff6b5b, 0xfff4f4ef,
              { 0xff8fd0ff, 0xff9fe0b0, 0xffffd166, 0xffffa66b, 0xffff8a8a, 0xffc9b3ff, 0xff7fe3e0, 0xffd6ef7f },
              0, 0, 0xffef8a1f, 0xffff4b3a,
              0xff9aa1a8, 0xff4e555c, 0xffd0322e, 0xff7a1414, 0xfff4f4ef },
            // Mastering Silver: brushed aluminium, black engraving, machined silver knobs, amber meters
            { 0xff9da2a8, 0xffb4b8bd, 0xffc4c8cc, 0xffdadde0, 0xff878c92, 0xff16181b, 0xff474c52, 0xff0d0f11, 0xff16191c,
              0xff1f7fbf, 0xffc2337a, 0xffb07a10, 0xff1e8a5a, 0xff6a4bc4, 0xffc23a2a, 0xff16181b,
              { 0xff1f7fbf, 0xff1e8a5a, 0xffb07a10, 0xffc4601a, 0xffc2337a, 0xff6a4bc4, 0xff12898f, 0xff6f8a12 },
              0, 0, 0xffffb02e, 0xffe0502a,
              0xfff1f2f3, 0xff8d9298, 0xffe3e5e7, 0xffa6abb0, 0xff111214 },
            // Vintage Cream: cream enamel on a black frame, black bakelite knobs, valve-glow lamps
            { 0xff221f1c, 0xff2d2925, 0xffe6dabd, 0xfff2e9d2, 0xffb9ab8b, 0xff1d1914, 0xff6b604e, 0xff14110e, 0xff1d1915,
              0xff2f6f8f, 0xffa8324a, 0xffa86a10, 0xff3c7a4a, 0xff6a4a8c, 0xffb3402a, 0xff1d1914,
              { 0xff2f6f8f, 0xff3c7a4a, 0xffa86a10, 0xffb85a20, 0xffa8324a, 0xff6a4a8c, 0xff2a7f7a, 0xff6a7a1a },
              0, 0, 0xffffa23a, 0xffff6a2a,
              0, 0, 0, 0, 0xfff2e9d2 },
            // Broadcast Green: army-green enamel, black knobs, cream legends
            { 0xff262d22, 0xff2f382b, 0xff4a5843, 0xff586851, 0xff6b7a62, 0xffefead8, 0xffb4b8a0, 0xff0e110c, 0xff151a12,
              0xffa8d8c0, 0xffffa0a0, 0xfff0cf6a, 0xffb8e08a, 0xffd0b8ff, 0xffff7a5a, 0xffefead8,
              { 0xffa8d8c0, 0xffb8e08a, 0xfff0cf6a, 0xffffaa6a, 0xffffa0a0, 0xffd0b8ff, 0xff8fe0d8, 0xffe0e88a },
              0, 0, 0xffb8e04a, 0xffff6a3d,
              0, 0, 0, 0, 0xfff4f0de },
            // Studio White: light, flat and clean, with dark screens and saturated accents
            { 0xffe4e7ec, 0xfff6f7f9, 0xfffcfcfd, 0xffffffff, 0xffc9cfd8, 0xff1a1f2b, 0xff666f82, 0xff151a26, 0xff1d2433,
              0xff1a8fe0, 0xffe0408f, 0xffd99a00, 0xff12a37a, 0xff7a55e0, 0xffe0453a, 0xff1a1f2b,
              { 0xff1a8fe0, 0xff12a37a, 0xffd99a00, 0xffe0701a, 0xffe0408f, 0xff7a55e0, 0xff0fa5b0, 0xff7aa012 } },
            // Midnight Glow: near-black with neon accents
            { 0xff030305, 0xff0b0a14, 0xff09090f, 0xff101019, 0xff1f1e2e, 0xfff4f2ff, 0xff7e7a99, 0xff020204, 0xff0a0912,
              0xff00e5ff, 0xffff2bd6, 0xffffe600, 0xff00ff9c, 0xffa64dff, 0xffff3d6e, 0xffe4e0ff,
              { 0xff00e5ff, 0xff00ff9c, 0xffffe600, 0xffff8a00, 0xffff2bd6, 0xffa64dff, 0xff2bfff2, 0xffc6ff00 } },
            // Goliath (default): black display screens over a brushed 500-series rack, amber LEDs, red for limiting
            { 0xff0b0b0c, 0xff141415, 0xff1c1c1e, 0xff2a2a2d, 0xff38383c, 0xffdedcd5, 0xff8e8e8a, 0xff0b0b0c, 0xff101012,
              0xffd4d4d4, 0xffe2343e, 0xffffb43c, 0xffffb43c, 0xffffb43c, 0xffe2343e, 0xffdedcd5,
              { 0xffe8e8e8, 0xffffb43c, 0xffe2343e, 0xffa8c3a0, 0xff8fb8d6, 0xffff8a3d, 0xffd9a3e0, 0xffc9d97a },
              0, 0, 0xff55e070, 0xffffb43c } };
        return defs[(int) s];
    }

    namespace pal
    {
        inline juce::Colour bg, bg2, panel, panelHi, line, text, dim, screen, screen2;
        inline juce::Colour clean, reso, eq, comp, deess, limiter, io;
        inline std::array<juce::Colour, 8> bands;
        inline juce::Colour eqLow, eqHigh;
        inline juce::Colour meter { 0xffef8a1f };   // LED meter colour
        inline juce::Colour alert { 0xffff6a3d };   // lamps + gain reduction
        inline SkinStyle style = SkinStyle::Onyx;
        inline juce::Colour skirtTop { 0xff3a3836 }, skirtBot { 0xff121110 }, capTop { 0xff2c2a28 }, capBot { 0xff0b0a0a };
        inline juce::Colour pointer;    // hardware knob pointer (defaults to the legend colour)
    }

    inline void applySkin (SkinStyle s)
    {
        const auto& d = skinDef (s);
        using C = juce::Colour;
        pal::style = s;
        pal::bg = C (d.bg); pal::bg2 = C (d.bg2); pal::panel = C (d.panel); pal::panelHi = C (d.panelHi);
        pal::line = C (d.line); pal::text = C (d.text); pal::dim = C (d.dim);
        pal::screen = C (d.screen); pal::screen2 = C (d.screen2);
        pal::clean = C (d.clean); pal::reso = C (d.reso); pal::eq = C (d.eq); pal::comp = C (d.comp);
        pal::deess = C (d.deess); pal::limiter = C (d.limiter); pal::io = C (d.io);
        for (size_t i = 0; i < 8; ++i) pal::bands[i] = C (d.bands[i]);
        pal::eqLow  = d.eqLow  != 0 ? C (d.eqLow)  : pal::eq;
        pal::eqHigh = d.eqHigh != 0 ? C (d.eqHigh) : pal::eq;
        pal::meter  = C (d.meter);
        pal::alert  = C (d.alert);
        pal::skirtTop = C (d.skirtTop != 0 ? d.skirtTop : 0xff3a3836u);
        pal::skirtBot = C (d.skirtBot != 0 ? d.skirtBot : 0xff121110u);
        pal::capTop   = C (d.capTop   != 0 ? d.capTop   : 0xff2c2a28u);
        pal::capBot   = C (d.capBot   != 0 ? d.capBot   : 0xff0b0a0au);
        pal::pointer  = d.pointer != 0 ? C (d.pointer) : pal::text;
    }

    inline bool isOnyx()     { return pal::style == SkinStyle::Onyx; }
    inline bool isGoliath()  { return pal::style == SkinStyle::Goliath; }
    // Goliath legend colours: sage section titles, light engraving, dim labels
    inline juce::Colour goliathSage()   { return juce::Colour (0xffa8c3a0); }
    inline juce::Colour goliathLegend() { return juce::Colour (0xffdedcd5); }
    inline juce::Colour goliathLabel()  { return juce::Colour (0xff9a9a95); }
    inline juce::Colour goliathAmber()  { return juce::Colour (0xffffb43c); }
    inline bool isRack()     { return pal::style == SkinStyle::Rack || isOnyx(); }   // Onyx shares the rack layout
    inline bool isHardware() { using S = SkinStyle; const auto s = pal::style;   // faceplate family
                               return s == S::Hardware || s == S::BritishConsole || s == S::MasteringSilver
                                   || s == S::VintageCream || s == S::BroadcastGreen; }
    inline bool isLightPanel() { return pal::panel.getPerceivedBrightness() > 0.6f; }
    // Text drawn on the black screens and readouts: stays light even on light-panel skins
    inline juce::Colour screenText() { return isLightPanel() ? juce::Colour (0xffe8eaee) : pal::text; }
    inline juce::Colour screenDim()  { return isLightPanel() ? juce::Colour (0xff9aa0aa) : pal::dim; }

    // While painting a black screen, text/dim switch to light colours on light-panel skins
    struct ScreenColours
    {
        ScreenColours() : text (pal::text), dim (pal::dim)
        {
            if (isLightPanel()) { pal::text = screenText(); pal::dim = screenDim(); }
        }
        ~ScreenColours() { pal::text = text; pal::dim = dim; }
        juce::Colour text, dim;
    };
    inline bool isMetal()    { return isRack() || isHardware(); }   // shared hardware-style details
    inline bool isModern()   { using S = SkinStyle; const auto s = pal::style;   // glass family
                               return s == S::Modern || s == S::StudioWhite || s == S::MidnightGlow; }
    inline bool isRoyal()    { return pal::style == SkinStyle::Royal; }

    // Shared drawing helpers (PluginEditor.cpp)
    void drawSegmented (juce::Graphics&, juce::Rectangle<float>, const juce::StringArray& names, int index, juce::Colour accent);
    void drawGlowStroke (juce::Graphics&, const juce::Path&, juce::Colour, float width);
    void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius, float angle);
    void drawScreenBackground (juce::Graphics&, juce::Rectangle<float>);
    void drawRackHole (juce::Graphics&, juce::Rectangle<float>);
    void drawGoliathLed (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, bool on);   // round LED in a black bezel
    void drawGoliathScrew (juce::Graphics&, juce::Point<float> centre, float radius, float angle);     // black cross-head screw
    juce::String compactValue (const juce::String& text);

    // Fine brushed-metal tile, generated once and tiled over faceplates
    inline const juce::Image& brushTile()
    {
        static juce::Image tile = []
        {
            juce::Image img (juce::Image::ARGB, 256, 256, true);
            juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
            juce::Random rng (7);
            for (int y = 0; y < 256; ++y)
            {
                float streak = rng.nextFloat() * 2.0f - 1.0f, run = 0.0f;
                for (int x = 0; x < 256; ++x)
                {
                    if (rng.nextInt (70) == 0) streak = rng.nextFloat() * 2.0f - 1.0f;
                    run = run * 0.85f + (rng.nextFloat() * 2.0f - 1.0f) * 0.15f;
                    const float v = streak * 0.5f + run * 2.0f;
                    bd.setPixelColour (x, y, v > 0.0f ? juce::Colours::white.withAlpha (juce::jmin (0.05f, v * 0.04f))
                                                       : juce::Colours::black.withAlpha (juce::jmin (0.08f, -v * 0.06f)));
                }
            }
            return img;
        }();
        return tile;
    }

    // V57 (HD): the same brushed metal generated at 2x - every streak lands on a real Retina pixel
    inline const juce::Image& brushTileHD()
    {
        static juce::Image tile = []
        {
            constexpr int n = 512;
            juce::Image img (juce::Image::ARGB, n, n, true);
            juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
            juce::Random rng (7);
            for (int y = 0; y < n; ++y)
            {
                float streak = rng.nextFloat() * 2.0f - 1.0f, run = 0.0f;
                for (int x = 0; x < n; ++x)
                {
                    if (rng.nextInt (140) == 0) streak = rng.nextFloat() * 2.0f - 1.0f;   // same streak length as 1x
                    run = run * 0.92f + (rng.nextFloat() * 2.0f - 1.0f) * 0.08f;
                    const float v = streak * 0.5f + run * 2.6f;
                    // finer grain: hairline streaks, a touch more contrast so they read at full resolution
                    bd.setPixelColour (x, y, v > 0.0f ? juce::Colours::white.withAlpha (juce::jmin (0.06f, v * 0.045f))
                                                       : juce::Colours::black.withAlpha (juce::jmin (0.09f, -v * 0.065f)));
                }
            }
            return img;
        }();
        return tile;
    }

    // Brushed-metal fill: HD (2x tile drawn at half size) from V57, the original tile before
    inline void setBrushFill (juce::Graphics& g, float opacity)
    {
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 57
        juce::FillType ft (brushTileHD(), juce::AffineTransform::scale (0.5f));
        ft.setOpacity (opacity);
        g.setFillType (ft);
       #else
        g.setTiledImageFill (brushTile(), 0, 0, opacity);
       #endif
    }
}
