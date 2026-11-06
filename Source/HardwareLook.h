#pragma once

// Classic-hardware looks: when a character EQ or compressor mode is selected,
// its section takes on a faceplate colour and knob style in tribute to the
// hardware that mode is modelled on. Colours and shapes only: no logos or names.

#include <juce_gui_basics/juce_gui_basics.h>
#include "VocalChain.h"

namespace vox::ui
{
    enum class KnobStyle
    {
        Skin = 0,       // whatever the current skin draws
        Pointer,        // big black fluted knob with a pointer tip (program EQ)
        CapColour,      // gray knob with a coloured cap (British console)
        RedCap,         // gray skirt, red or gray cap (class-A module)
        Aluminium,      // brushed silver (modern boutique)
        BlackChrome,    // black knob with a chrome ring (FET limiter)
        Bakelite,       // big vintage black skirted knob (vari-mu)
        FlatGray,       // flat modern plugin knob with an orange pointer
        ModernArc,      // sleek matte knob, glowing value arc in the mode's colour
        ChickenHead,    // vintage British studio: ivory chicken-head pointer on a black base
        KnurledBlack    // V76: matte black knurled knob, flat top, white index (American bus compressor)
    };

    struct HwLook
    {
        bool active = false;
        juce::Colour faceTop, faceBottom;   // faceplate gradient
        juce::Colour text;                  // values, scale numbers, title
        juce::Colour label;                 // knob titles / group labels
        juce::Colour groove;                // slanted dividers
        KnobStyle knob = KnobStyle::Skin;
        juce::Colour knobCap;               // default cap / body colour for the style
        bool hammertone = false;            // speckled crackle finish
        bool lightPlate = false;            // dark text on a light plate
        bool modern = false;                // clean tinted panel with an accent glow (no textures)
        juce::Colour accent;                // the mode's signature colour
    };

    HwLook hwLookForEq (EqType);
    HwLook hwLookForComp (CompType);
    KnobStyle hwLookForCompKnob (CompType);   // outboard knob style for a compressor mode

    // Colour for a character EQ knob's cap, by EQ type and band role (0 HPF, 1 low, 2 low-mid, 3 high-mid, 4 high, 5 gain-vs-freq flag)
    juce::Colour hwCapColour (EqType, int bandRole, bool isGain);

    // Signature colour of each mode, used for its activation light
    juce::Colour eqModeColour (EqType);
    juce::Colour compModeColour (CompType);

    void paintFaceplate (juce::Graphics&, juce::Rectangle<float>, const HwLook&, float corner);

    // Draws the scale (numbers + ticks) and a knob in the given style. Returns false if style is Skin.
    bool drawHardwareKnob (juce::Graphics&, juce::Slider&, juce::Rectangle<float> area, float pos,
                           float a0, float a1, KnobStyle, juce::Colour cap, juce::Colour text);
}
