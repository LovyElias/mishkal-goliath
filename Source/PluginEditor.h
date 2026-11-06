#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include "Skin.h"

#ifndef MISHKAL_VERSION
 #define MISHKAL_VERSION 1   // 1 = the main version; 2, 3, 4... = the numbered try-outs (set by CMake)
#endif
#define MISHKAL_V2 (MISHKAL_VERSION >= 2)   // screen-style Character EQ
#define MISHKAL_V3 (MISHKAL_VERSION >= 3)   // minimal Reso and Tahor band controls
#define MISHKAL_V4 (MISHKAL_VERSION >= 4)   // red bar gain-reduction meters (De-ess, Zerem)
#define MISHKAL_V5 (MISHKAL_VERSION >= 5)   // red bar gain reduction in the Dynamics and Limiter meter blocks
#define MISHKAL_V6 (MISHKAL_VERSION >= 6)   // In/Out level as solid green-yellow-red bars in the Dynamics and Limiter meter blocks
#define MISHKAL_V7 (MISHKAL_VERSION >= 7)   // the GR ring round the Dynamics Threshold knob as one solid green-yellow-red arc
#define MISHKAL_V8 (MISHKAL_VERSION >= 8)   // the Gate column in Dynamics as a solid green-yellow-red bar
#define MISHKAL_V9 (MISHKAL_VERSION >= 9)   // Input / Output knobs in the minimal Reso style, in teal
#define MISHKAL_V10 (MISHKAL_VERSION >= 10) // neon glow on the teal Input / Output rings
#define MISHKAL_V11 (MISHKAL_VERSION >= 11) // the neon glow follows the signal: Input with the input level, Output with the output level
#define MISHKAL_V12 (MISHKAL_VERSION >= 12) // De-ess Freq / Threshold / Range in the minimal Reso style
#define MISHKAL_V13 (MISHKAL_VERSION >= 13) // modern switches: segmented pills, slide toggles, neon power buttons
#define MISHKAL_V14 (MISHKAL_VERSION >= 14) // gold scheme: gold controls (was coral), warm-white neon (was teal)
#define MISHKAL_V15 (MISHKAL_VERSION >= 15) // De-ess threshold glows: softly with the signal, brightly while it is de-essing
#define MISHKAL_V16 (MISHKAL_VERSION >= 16) // Limiter knobs in the minimal Reso style with a velvet finish
#define MISHKAL_V17 (MISHKAL_VERSION >= 17) // velvet Limiter knobs in violet
#define MISHKAL_V18 (MISHKAL_VERSION >= 18) // Limiter controls sit on a flat rounded panel like the Tahor / Reso one
#define MISHKAL_V19 (MISHKAL_VERSION >= 19) // the Limiter panel covers the whole module below the switches, meters included
#define MISHKAL_V20 (MISHKAL_VERSION >= 20) // Limiter oversampling and soft clip as dropdown menus
#define MISHKAL_V21 (MISHKAL_VERSION >= 21) // Limiter knobs flat like Reso (no velvet), still violet
#define MISHKAL_V22 (MISHKAL_VERSION >= 22) // Dynamics: each compressor's controls sit on a faceplate and knobs in the style of the hardware it is modelled on
#define MISHKAL_V23 (MISHKAL_VERSION >= 23) // Gate / Breath / Rider in the minimal Reso style on their own flat panel
#define MISHKAL_V24 (MISHKAL_VERSION >= 24) // 1964 PK gets its own In / Out knobs and a Link switch, on its own panel
#define MISHKAL_V25 (MISHKAL_VERSION >= 25) // 1964 PK: its own power button, panel title, violet in / out
#define MISHKAL_V26 (MISHKAL_VERSION >= 26) // Dynamics Mix knob in the minimal Reso style
#define MISHKAL_V27 (MISHKAL_VERSION >= 27) // LE-76 revision dropdown: Purple / Blue Stripe / Black, each with its own sound and faceplate
#define MISHKAL_V28 (MISHKAL_VERSION >= 28) // LE-76 revision as an LCD left / right selector (like the LE-RC ones)
#define MISHKAL_V29 (MISHKAL_VERSION >= 29) // LE-76 laid out like the classic FET unit: big Output, small Attack / Release, ratio push-buttons
#define MISHKAL_V30 (MISHKAL_VERSION >= 30) // LE-RC model selector: RComp or RVox (the one-knob vocal leveler with automatic make-up)
#define MISHKAL_V31 (MISHKAL_VERSION >= 31) // RVox: Link switch between Threshold and Output (on = automatic make-up, off = independent)
#define MISHKAL_V32 (MISHKAL_VERSION >= 32) // RComp / RVox shown as LE-RC / LE-RV, matching the LE- names
#define MISHKAL_V33 (MISHKAL_VERSION >= 33) // one OS button in the top bar: the whole plugin oversampled (the per-module OS controls go)
#define MISHKAL_V34 (MISHKAL_VERSION >= 34) // Zerem and Tape in the Reso look: flat titled panels, minimal knobs
#define MISHKAL_V35 (MISHKAL_VERSION >= 35) // Zerem knobs glow electric cyan, Tape knobs glow copper, with what each is doing
#define MISHKAL_V36 (MISHKAL_VERSION >= 36) // the Zerem / Tape drawer opens below the plugin (the window grows) and is shorter
#define MISHKAL_V37 (MISHKAL_VERSION >= 37) // Zerem MODEL selector: Zerem or Perach, the adaptive tone shaper
#define MISHKAL_V38 (MISHKAL_VERSION >= 38) // Perach: a small graph with four regions you drag up / down instead of tone knobs
#define MISHKAL_V39 (MISHKAL_VERSION >= 39) // Tape MODEL selector: Tape or Esh, the character tape saturator
#define MISHKAL_V40 (MISHKAL_VERSION >= 40) // the tape machine model is named Slil (Hebrew: reel)
#define MISHKAL_V41 (MISHKAL_VERSION >= 41) // the Zerem / Perach unit is titled TONE (drawer: TONE · TAPE)
#define MISHKAL_V42 (MISHKAL_VERSION >= 42) // signal flow: step numbers on module titles, live signal-path strip along the bottom bar
#define MISHKAL_V43 (MISHKAL_VERSION >= 43) // Character EQ screen: live spectrum, a glowing colour per band, light beams, neon curve
#define MISHKAL_V58 (MISHKAL_VERSION >= 58) // match the Figma frame: minimal gold big knob, minimal LE-STA Output, glowing meter-bridge bars
#define MISHKAL_V68 (MISHKAL_VERSION >= 68) // drag the modules in the signal-flow bar to change their order
#define MISHKAL_V69 (MISHKAL_VERSION >= 69) // limiter Max engine: styles, character, transients
#define MISHKAL_V70 (MISHKAL_VERSION >= 70) // limiter upward compression
#define MISHKAL_V72 (MISHKAL_VERSION >= 72) // Goliath XL Safe style
#define MISHKAL_V84 (MISHKAL_VERSION >= 84) // the big Threshold knob shows the signal going in and coming out (live arcs on its scale) and glows with it
#define MISHKAL_V83 (MISHKAL_VERSION >= 83) // no step numbers on the module titles (the order can change); no numbers in the Tahor screen's EQ key
#define MISHKAL_V82 (MISHKAL_VERSION >= 82) // fix: the Zerem screen lights up as soon as Tone is switched on (it waited for a preset change)
#define MISHKAL_V81 (MISHKAL_VERSION >= 81) // chain lanes: the Character EQs and compressors that are in, numbered in the order they run; numbers in the dropdown and the display key; the display draws all 8 EQs
#define MISHKAL_V80 (MISHKAL_VERSION >= 80) // module names: LACHATZ (dynamics), LACHASH (de-esser), CHOMAH (limiter), each with what it is under it
#define MISHKAL_V79 (MISHKAL_VERSION >= 79) // the preset library reorganised: 11 folders, chain-order presets, more FX
#define MISHKAL_V78 (MISHKAL_VERSION >= 78) // renamed Mishkal Goliath: wordmark "Goliath" with "mixing · mastering" under it (no more "channel strip")
#define MISHKAL_V77 (MISHKAL_VERSION >= 77) // LE-2500 back to the menu switches, no needle meter on LE-G (faceplates kept)
#define MISHKAL_V76 (MISHKAL_VERSION >= 76) // LE-2500 and LE-G faceplates redrawn after their hardware: push-buttons, badge, printed frame, console caps
#define MISHKAL_V75 (MISHKAL_VERSION >= 75) // five bus / mastering compressors: LE-2500, LE-G, LE-DS1, LE-IRON, LE-336
#define MISHKAL_V74 (MISHKAL_VERSION >= 74) // four more character EQs: LE-A550, LE-K9, LE-H69, LE-M4
#define MISHKAL_V71 (MISHKAL_VERSION >= 71) // Goliath Classic / Multi / XL names and colours, character EQ M/S
#define MISHKAL_V67 (MISHKAL_VERSION >= 67) // presets: favourites (star), search, step inside the current folder
#define MISHKAL_V66 (MISHKAL_VERSION >= 66) // Auto Input: listens a few seconds and sets the input level
#define MISHKAL_V65 (MISHKAL_VERSION >= 65) // delta (listen to what changes) on the compressors and the limiter
#define MISHKAL_V64 (MISHKAL_VERSION >= 64) // external sidechain (key) for the compressors and the de-esser
#define MISHKAL_V63 (MISHKAL_VERSION >= 63) // stereo width knob (bass stays mono) and a phase / correlation meter in the limiter module
#define MISHKAL_V62 (MISHKAL_VERSION >= 62) // limiter: true peak, a clipper before it, and a loudness target on the screen (click LUFS)
#define MISHKAL_V61 (MISHKAL_VERSION >= 61) // stereo link: Reso (L/R, like soothe's Link) and the limiter (like Pro-L's channel linking)
#define MISHKAL_V60 (MISHKAL_VERSION >= 60) // mid/side: Tahor bands on Stereo/Mid/Side, Reso M/S modes + side depth, tape M/S drive; mono tracks show one meter and no M/S
#define MISHKAL_V59 (MISHKAL_VERSION >= 59) // the window opens at 75% by default (new instances; saved sessions keep their size)
#define MISHKAL_V57 (MISHKAL_VERSION >= 57) // HD: brushed metal and hammertone textures at Retina (2x) resolution
#define MISHKAL_V56 (MISHKAL_VERSION >= 56) // Goliath only: the old skins and the Skin menu are removed
#define MISHKAL_V55 (MISHKAL_VERSION >= 55) // defaults + every preset: Reso depth 15%, Perach amount 2 / mix 80%
#define MISHKAL_V54 (MISHKAL_VERSION >= 54) // the 1964 PK light flashes red when it catches peaks
#define MISHKAL_V53 (MISHKAL_VERSION >= 53) // wordmark: capital G - "Goliath channel strip"
#define MISHKAL_V52 (MISHKAL_VERSION >= 52) // LE-STA is its own (4th) compressor in the Dynamics list; no module screws (more room)
#define MISHKAL_V51 (MISHKAL_VERSION >= 51) // 16 more LE Goliath Signature presets (presets only)
#define MISHKAL_V50 (MISHKAL_VERSION >= 50) // glowing L/R level bars (Dynamics + Limiter), tidy In/Gate/GR names, a new preset library
#define MISHKAL_V49 (MISHKAL_VERSION >= 49) // roomier top bar (MATCH reading fits); LE-STA tube option in the 660 slot; "LE Goliath Signature" presets
#define MISHKAL_V48 (MISHKAL_VERSION >= 48) // animated links: ghost marker + moving dots on the linked Out knob, flowing arrows under the link switch (1964 PK, LE-RV)
#define MISHKAL_V47 (MISHKAL_VERSION >= 47) // wordmark reads "goliath channel strip"; Goliath Signature presets (Perach, Esh, 1964 PK, LE-76 revs, LE-RV)
#define MISHKAL_V46 (MISHKAL_VERSION >= 46) // Tahor spectrum in the Limiter wave's colours: cyan -> violet -> pink by level, glowing edge
#define MISHKAL_V45 (MISHKAL_VERSION >= 45) // vibrant Limiter screen: glowing cyan-violet-pink wave, hot limiting edge, neon ceiling
#define MISHKAL_V44 (MISHKAL_VERSION >= 44) // Tahor EQ screen comes alive (same look): glowing spectrum + curve, coloured band beams, breathing selected dot
#define MISHKAL_ACCENT ((juce::uint32) (MISHKAL_V14 ? 0xffffb83du : 0xffff7a59u))   // rings, toggles, pill underlines
#define MISHKAL_NEON   ((juce::uint32) (MISHKAL_V14 ? 0xffffe7a8u : 0xff2ec4b6u))   // neon power buttons, Input / Output glow
#include "HardwareLook.h"

namespace vox::ui
{
    juce::Font font (float size, bool bold = false);          // Goliath: Fira Sans (the hardware legends)
    juce::Font screenFont (float size, int weight = 1);       // Barlow 0 regular / 1 medium / 2 semibold (Goliath display text)
    // Goliath two-way bat-handle toggle with its labels above and below
    void drawBatToggle (juce::Graphics&, juce::Rectangle<float> area, bool up, const juce::String& topLabel, const juce::String& bottomLabel);

    //==========================================================================
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void applyColours();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        int getSliderThumbRadius (juce::Slider&) override { return 9; }
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    };

    //==========================================================================
    class Knob : public juce::Component
    {
    public:
        using ColourFn = std::function<juce::Colour()>;
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId,
              const juce::String& title, const juce::Colour& accent, bool bipolar = false);
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId,
              const juce::String& title, ColourFn accent, bool bipolar = false);
        void resized() override;
        void lookAndFeelChanged() override;
        // re-point at another parameter (optionally with a new title / bipolar arc)
        void bind (juce::AudioProcessorValueTreeState&, const juce::String& paramId,
                   const juce::String& newTitle = {}, int bipolar = -1);

        // Show as a vertical console fader instead of a rotary knob
        void setFader (bool fader);

        // Classic-hardware knob style (KnobStyle::Skin = follow the skin)
        void setLook (KnobStyle style, juce::Colour cap, juce::Colour text, juce::Colour titleColour);

        juce::Slider slider;
        juce::Label title;
        void mouseEnter (const juce::MouseEvent&) override;
        void setTitleText (const juce::String& t)
        {
            if (t == rawTitle) return;
            rawTitle = t;
            if (! hovering && ! slider.isMouseButtonDown()) title.setText (isGoliath() ? t : t.toUpperCase(), juce::dontSendNotification);
        }
        void mouseExit (const juce::MouseEvent&) override;
    private:
        void showValueInTitle (bool show);
        juce::String rawTitle;
        bool hovering = false;
        ColourFn accent;
        KnobStyle lookStyle = KnobStyle::Skin;
        juce::Colour lookText, lookTitle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    //==========================================================================
    // Segmented mode switch bound to a choice parameter
    class ModeSelector : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        ModeSelector (juce::RangedAudioParameter&, const juce::StringArray& names, const juce::Colour& accent);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        int getIndex() const { return index; }
        void setNames (const juce::StringArray& n) { names = n; repaint(); }
        juce::StringArray batLabels;   // Goliath: two-way selectors draw as a bat toggle with these labels
        bool screenTabs = false;       // Goliath: drawn as text tabs on the black display
        bool vertical = false;         // Goliath: a column of LEDs with labels
        juce::String dropdownLabel;    // Goliath: when set, drawn as a small dropdown with this caption
        juce::String stepperLabel;     // Goliath: when set, drawn as a Zerem-style LCD with left / right arrows
        bool pills = false;            // V60 Goliath: a compact row of pills (e.g. stereo | mid | side)
        juce::String pillCaption;      //   ... with an optional caption on the left
        std::function<juce::Colour (int)> colourFor;   // optional: per-mode light colour
        int hwButtons = 0;             // V76 Goliath: a row of hardware push-buttons, 1 = light caps / green lamp (LE-2500), 2 = dark console caps / red lamp (LE-G)
        juce::String hwCaption;        //   ... with this caption printed above them
        juce::Colour hwLegend { 0xff111318 };
        float pillCaptionWidth() const { return pillCaption.isEmpty() ? 0.0f : 8.0f + 6.6f * (float) pillCaption.length(); }
    private:
        juce::StringArray names;
        const juce::Colour* accent;
        int index = 0;
        juce::ParameterAttachment attachment;
    };

    //==========================================================================
    class GrMeter : public juce::Component, private juce::Timer
    {
    public:
        GrMeter (std::atomic<float>& source, const juce::Colour& accent, juce::String label = "GR");
        void paint (juce::Graphics&) override;
        std::function<juce::Colour()> colourFor;   // optional: overrides the LED colour
    private:
        void timerCallback() override;
        std::atomic<float>& source;
        const juce::Colour* accent;
        juce::String label;
        float shown = 0.0f;
    };

    // Small light that shows when the gate is closing
    class GateLed : public juce::Component, private juce::Timer
    {
    public:
        explicit GateLed (std::atomic<float>& gateGr);
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override;
        std::atomic<float>& source;
        float shown = 0.0f;
    };

    // LED VU ladder (tape-deck style) with real VU ballistics (0 VU = -18 dBFS RMS)
    class TapeVu : public juce::Component, private juce::Timer
    {
    public:
        explicit TapeVu (std::atomic<float>& levelDb) : source (levelDb) { startTimerHz (30); }
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override;
        std::atomic<float>& source;
        float needle = -22.0f, peakHold = -22.0f;
        int holdFrames = 0;
    };

    class LevelMeter : public juce::Component, private juce::Timer
    {
    public:
        explicit LevelMeter (std::atomic<float>& sourceDb);
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override;
        std::atomic<float>& source;
        float shown = -100.0f;
    };

    //==========================================================================
    // Tall stereo meter strip on the edge of the window, with the Input or Output knob on top of it.
    // Click the meter to reset the peak readout and clip lights.
    class IoStrip : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit IoStrip (std::atomic<float> (&peaks)[2]);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void setKnob (juce::Component& k) { knob = &k; addAndMakeVisible (k); resized(); }
        void setButton (juce::Component& b) { button = &b; addAndMakeVisible (b); resized(); }   // sits under the knob
        void setLoudness (vox::LoudnessMeter& m) { loudness = &m; resized(); }                    // LUFS readout (Output)
        void setCompact (bool c) { compact = c; resized(); repaint(); }   // Goliath: short strip inside the display screen
        void setActive (bool a) { if (a) startTimerHz (30); else stopTimer(); }   // Goliath reads the peaks in its meter bridge instead
        void detach() { knob = nullptr; button = nullptr; }                      // Goliath moves the knob and button into the modules
        bool compact = false;
    private:
        void timerCallback() override;
        std::atomic<float> (&peaks)[2];
        juce::Component* knob = nullptr;
        juce::Component* button = nullptr;
        vox::LoudnessMeter* loudness = nullptr;
        juce::Rectangle<int> lufsArea;
        juce::Rectangle<int> meterArea;
        std::array<float, 2> shown { -100.0f, -100.0f }, hold { -100.0f, -100.0f };
        std::array<int, 2> holdFrames {};
        std::array<bool, 2> clip {};
        float maxDb = -100.0f;
    };

    //==========================================================================
    // Clean-up EQ graph: spectrum analyzer, curves and draggable band nodes
    juce::Colour bandColour (int band);

    class CleanupGraph : public juce::Component, private juce::Timer
    {
    public:
        explicit CleanupGraph (VoxStripProcessor&);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseMove (const juce::MouseEvent&) override;

        std::function<void (int)> onSelect;   // band index, or -1
        int getSelected() const { return selected; }
        void select (int band);
        bool resoView = false;                 // Goliath: the screen shows the resonance suppressor instead of the EQ

    private:
        void timerCallback() override;
        void updateSpectrum (AnalyzerTap&, std::vector<float>& window, std::vector<float>& smoothed);
        juce::Path spectrumPath (const std::vector<float>& smoothed, bool closed) const;

        float xForFreq (double hz) const;
        double freqForX (float x) const;
        float yForDb (float db) const;
        float dbForY (float y) const;
        juce::Point<float> nodePosition (const CleanBand&) const;
        int hitTest (juce::Point<float>) const;

        juce::RangedAudioParameter& param (int band, const char* field) const;
        void setParam (int band, const char* field, float value);
        void showBandMenu (int band);

        void paintResoView (juce::Graphics&, juce::Rectangle<float>);
        int resoEdgeAt (float x) const;        // 0 = Low edge, 1 = High edge, -1 = neither
        int resoDrag = -1;
        VoxStripProcessor& proc;
        int selected = -1, dragging = -1, hover = -1;
        bool tempSolo = false;      // Option-click on a node solos it while the mouse is held
        int soloBefore = -1;
        static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
        std::vector<float> preWindow, postWindow, preSmooth, postSmooth, fftData;
        float liveLevel = 0.0f, livePulse = 0.0f;   // V44: drives the glow / the selected dot's breathing
        static constexpr float rangeDb = 18.0f;
    };

    // Controls for the selected clean-up band
    // V60: the colour of a stereo placement (0 Stereo, 1 Mid, 2 Side)
    inline juce::Colour placeColour (int p) { return p == 1 ? juce::Colour (0xffffb83d) : p == 2 ? juce::Colour (0xff5fd7ff) : juce::Colour (0xffd8d8dc); }

    class BandPanel : public juce::Component
    {
    public:
        explicit BandPanel (juce::AudioProcessorValueTreeState&);
        void showBand (int band);
        void setSoloTarget (std::atomic<int>* t) { soloTarget = t; }
        void syncSolo();   // keep the SOLO button in step with the engine (the graph can solo too)
        int getBand() const { return band; }
        void paint (juce::Graphics&) override;
        void resized() override;
    private:
        juce::AudioProcessorValueTreeState& state;
        int band = -1;
        juce::ComboBox type, slope;
        juce::ToggleButton on { "ON" }, dyn { "DYNAMIC" }, solo { "SOLO" };
        std::atomic<int>* soloTarget = nullptr;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt, slopeAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt, dynAtt;
        std::unique_ptr<Knob> freq, gain, q, thresh, range;
        std::unique_ptr<ModeSelector> place;   // V60: Stereo / Mid / Side (stereo tracks only)
        bool placeShown() const;
    };

    //==========================================================================
    // Resonance suppressor controls (RESO tab)
    class ResoPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit ResoPanel (juce::AudioProcessorValueTreeState&);
        void paint (juce::Graphics&) override;
        void resized() override;
        void lookAndFeelChanged() override;
    private:
        void timerCallback() override;   // V60: follows the M/S mode and the track layout
        juce::AudioProcessorValueTreeState& state;
        juce::ToggleButton on { "ON" }, delta { "DELTA" };
        juce::AudioProcessorValueTreeState::ButtonAttachment onAtt, deltaAtt;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::unique_ptr<ModeSelector> msMode;   // V60: Stereo / Mid / Side / Mid + Side
        std::unique_ptr<Knob> sideDepth;        // V60: the side's own depth (Mid + Side)
        std::unique_ptr<Knob> link;             // V61: stereo link (Stereo mode, stereo tracks)
        bool msShown = false; int msWas = -1;
    };

    // Two-way tab switch (not a parameter, just which panel is showing)
    class TabSwitch : public juce::Component
    {
    public:
        TabSwitch (juce::StringArray names, std::vector<const juce::Colour*> colours);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void setIndex (int i);
        int getIndex() const { return index; }
        void setNames (const juce::StringArray& n) { names = n; repaint(); }
        std::function<void (int)> onChange;
    private:
        juce::StringArray names;
        std::vector<const juce::Colour*> colours;
        int index = 0;
    };

    //==========================================================================
    // Top bar controls
    class IconButton : public juce::Button
    {
    public:
        enum class Icon { Undo, Redo, Left, Right, Menu };
        IconButton (Icon i, bool isBoxed) : juce::Button ({}), icon (i), boxed (isBoxed) {}
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    private:
        Icon icon;
        bool boxed;
    };

    class PresetBox : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        void setText (const juce::String& t) { if (t != text) { text = t; repaint(); } }
        void setStar (bool show, bool lit) { if (show != starShown || lit != starLit) { starShown = show; starLit = lit; repaint(); } }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (starShown && e.x < 30) { if (onStar) onStar(); return; }   // V67: the star toggles the favourite
            if (onClick) onClick();
        }
        std::function<void()> onClick, onStar;
    private:
        juce::String text;
        bool starShown = false, starLit = false;
    };

    class ABSwitch : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        void setActive (int slot) { if (slot != active) { active = slot; repaint(); } }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        std::function<void (int)> onSelect;
        std::function<void()> onCopy;
    private:
        int active = 0;
    };

    void drawLogo (juce::Graphics&, juce::Rectangle<float> area);

    // Vox Flow style browser: an LCD with < STYLE > arrows, a description,
    // and live gain-reduction bars for the five bands
    class FlowDisplay : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        FlowDisplay (VoxStripProcessor&);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
    private:
        void timerCallback() override;
        void setStyle (int i);
        VoxStripProcessor& proc;
        juce::ParameterAttachment attachment;
        int style = 0;
        float shown[5] {};
        bool lastOn = false;   // V82: repaint when Tone is switched in or out
    };

    // Character EQ selector: each tab has an activation LED (click it to switch
    // that EQ in or out) and a name (click it to show that EQ's knobs)
    // Tab bar where each tab has an activation light (click it to switch that
    // unit in or out, any combination) and a name (click it to edit that unit).
    // Used by the Character EQ and the Compressor.
    class TypeBar : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        TypeBar (juce::AudioProcessorValueTreeState&, juce::StringArray names,
                 std::function<juce::String (int)> onParamId, std::function<juce::Colour (int)> colourFor,
                 juce::Identifier tabProperty, int defaultTab, juce::String tooltip,
                 juce::String orderParamId = {});   // tabs can be dragged to change processing order
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        int getSelected() const { return selected; }
        void setSelected (int t);
        std::function<void (int)> onSelect;
        bool vertical = false;   // Goliath: a column of lights with names, one row per unit
        // for the Goliath dropdown
        bool unitActive (int t) const { return isActive (t); }
        std::vector<int> getOrder() const { return order(); }
        void moveUnit (int t, int delta);                  // earlier (-1) or later (+1) in the chain
        void setUnitOrder (const std::vector<int>& o);     // V81: a whole new order (one undo step)
        void toggleUnit (int t);                           // switch a unit in or out
        const juce::StringArray& unitNames() const { return names; }
    private:
        void timerCallback() override { repaint(); }
        bool isActive (int t) const;
        float ledX (int slot) const;
        std::vector<int> order() const;            // order[slot] = unit
        void setOrder (const std::vector<int>&);
        int slotAt (float x) const;
        juce::String orderParamId;
        int downSlot = -1, dragUnit = -1;
        bool dragging = false;
        juce::AudioProcessorValueTreeState& state;
        juce::StringArray names;
        std::function<juce::String (int)> onParamId;
        std::function<juce::Colour (int)> colourFor;
        juce::Identifier tabProperty;
        int selected = 0;
    };

    //==========================================================================
    // Goliath display: In / Out / gain-reduction bars with peak and average readouts (click to reset the readouts)
    class MeterBridge : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit MeterBridge (VoxStripProcessor&);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void setActive (bool a) { if (a) startTimerHz (30); else stopTimer(); }
        // live values (dB) for the LED block in the Limiter module
        std::array<float, 2> inPk { -100.0f, -100.0f }, outPk { -100.0f, -100.0f };
        float limGr = 0.0f, compGr = 0.0f;
        bool isMono() const;   // V60: on a mono track the meters show one channel
    private:
        void timerCallback() override;
        VoxStripProcessor& proc;
        std::array<float, 2> inAvg { -100.0f, -100.0f }, outAvg { -100.0f, -100.0f };
        std::array<float, 2> inMs {}, outMs {};
        std::array<float, 2> inHold { -100.0f, -100.0f }, outHold { -100.0f, -100.0f };
        std::array<int, 2> inHoldT {}, outHoldT {};
        std::array<float, 2> inMax { -100.0f, -100.0f }, outMax { -100.0f, -100.0f };
        float grMax = 0.0f;
    };

    //==========================================================================
    // Goliath: dropdown that picks which Character EQ you edit; its light switches that EQ in or out
    class EqDropdown : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        EqDropdown (TypeBar& b, std::function<juce::String (int)> describeFn, const juce::String& what) : bar (b), describe (std::move (describeFn))
        {
            startTimerHz (10);
            setTooltip ("Pick the " + what + " to edit. The light switches it in or out. The menu also changes the order they run in.");
        }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        std::function<juce::String (int)> nameFor;   // V49: optional live name (e.g. LE-STA in the tube slot); empty = the bar's own
        juce::String nameOf (int t) const { auto n = nameFor ? nameFor (t) : juce::String(); return n.isNotEmpty() ? n : bar.unitNames()[t]; }
    private:
        void timerCallback() override { repaint(); }
        TypeBar& bar;
        std::function<juce::String (int)> describe;
    };

    // V81: the units that are switched in, as numbered chips in the order they run. Click a chip to edit that unit;
    // drag it left or right to change the order.
    class ChainLane : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        ChainLane (TypeBar& b, std::function<juce::Colour (int)> colourFn, const juce::String& what) : bar (b), colourFor (std::move (colourFn)), noun (what)
        {
            startTimerHz (10);
            setTooltip ("The " + what + "s that are in, numbered in the order they run. Click one to edit it; drag it left or right to change the order.");
        }
        std::function<juce::String (int)> nameFor;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
    private:
        struct Chip { int unit; juce::Rectangle<float> r; };
        std::vector<Chip> layoutChips (juce::Font& f, bool& shortNames) const;
        juce::String nameOf (int t, bool shortName) const;
        int dropIndex (float x) const;
        void timerCallback() override { if (! dragging) repaint(); }
        TypeBar& bar;
        std::function<juce::Colour (int)> colourFor;
        juce::String noun;
        int dragUnit = -1;
        bool dragging = false;
        float dragX = 0.0f;
    };

    //==========================================================================
    // Goliath Dynamics: ring of lights around the Threshold knob that fills with the compressor's gain reduction
    class GrRing : public juce::Component, private juce::Timer
    {
    public:
        GrRing (const MeterBridge& b) : bridge (b) { setInterceptsMouseClicks (false, false); startTimerHz (30); }
        void paint (juce::Graphics&) override;
        float knobRadius = 40.0f;   // set by the editor to match the knob it surrounds
    private:
        void timerCallback() override { repaint(); }
        const MeterBridge& bridge;
    };

    // Goliath Dynamics: LED ladders for In L/R, the gate's reduction and the compressor's gain reduction
    class DynMeterBlock : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        DynMeterBlock (const MeterBridge& b, std::atomic<float>& gate) : bridge (b), gateGr (gate)
        {
            startTimerHz (30);
            setTooltip ("In: input level.  Gate: how far the gate is turning the signal down.  GR: the compressor's gain reduction.");
        }
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override
        {
            const float v = gateGr.load();
            gateShown = v > gateShown ? v : gateShown * 0.8f + v * 0.2f;
            repaint();
        }
        const MeterBridge& bridge;
        std::atomic<float>& gateGr;
        float gateShown = 0.0f;
    };

    //==========================================================================
    // V2: the Character EQ as a screen. Drag a band's dot to shape the curve (left / right = frequency,
    // up / down = boost or cut, scroll = Q or bandwidth); the readouts underneath give exact values.
    class CharEqScreen : public juce::Component, private juce::Timer
    {
    public:
        CharEqScreen (VoxStripProcessor&, TypeBar&);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
    private:
        struct Field { const char* label; const char* id; };
        struct Band  { const char* name; const char* freq; const char* gain; const char* gain2; const char* q;   // gain2: Pultec cut on the same dot
                       double fixedHz = 0.0;                    // V74: a band with no frequency control sits here
                       std::vector<Field> more {}; };           // V74: extra readouts (shape, filter, E/G...)
        void timerCallback() override;
        void paintV43 (juce::Graphics&);
        juce::Colour bandColour (int t, int band) const;
        std::vector<Band> bandsFor (int t) const;
        std::vector<Field> fieldsFor (int t, int band) const;
        juce::RangedAudioParameter* param (const char* field) const;
        juce::Rectangle<float> plotArea() const;
        float xForHz (double hz) const;
        double hzForX (float x) const;
        float yForDb (float db) const;
        juce::Point<float> dotFor (int band) const;
        float dotValue (int band) const;                 // Pultec low: boost - cut
        void setDotValue (int band, float v);
        void setFreqFromX (int band, float x);
        int hitDot (juce::Point<float>) const;
        int hitField (juce::Point<float>) const;
        void begin (juce::RangedAudioParameter*);
        void end (juce::RangedAudioParameter*);

        VoxStripProcessor& proc;
        TypeBar& types;
        int selected = 1, hover = -1;
        int dragDot = -1, dragField = -1;
        float dragStartValue = 0.0f, dragStartY = 0.0f;
        std::vector<juce::Rectangle<float>> fieldRects;
        double sr = 48000.0;
        // V43: live spectrum after the Character EQs
        static constexpr int specOrder = 12, specSize = 1 << specOrder;
        juce::dsp::FFT specFft { specOrder };
        juce::dsp::WindowingFunction<float> specWindow { (size_t) specSize, juce::dsp::WindowingFunction<float>::hann };
        std::vector<float> specWin = std::vector<float> ((size_t) specSize, 0.0f);
        std::vector<float> specData = std::vector<float> ((size_t) specSize * 2, 0.0f);
        std::vector<float> specSmooth = std::vector<float> ((size_t) specSize / 2 + 1, -120.0f);
        float specLevel = 0.0f, pulse = 0.0f;
    };

    //==========================================================================
    // Goliath: a clickable name above a module's ON button: shows that stage's controls (lit = showing)
    class ViewTab : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit ViewTab (juce::String t) : text (std::move (t)) { startTimerHz (10); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        std::function<void()> onClick;
        std::function<bool()> isSelected;
    private:
        void timerCallback() override { repaint(); }
        juce::String text;
    };

    //==========================================================================
    // Goliath: the bar along the bottom that opens the Zerem + Tape drawer (like a FabFilter side-chain panel)
    class DrawerBar : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit DrawerBar (VoxStripProcessor& p) : proc (p) { startTimerHz (10); setTooltip ("Open or close the Zerem and Tape drawer"); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;   // V68: drag a module to a new place
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override { repaint(); }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        std::function<void()> onToggle;
        bool open = false;
    private:
        // V42: action 0 none, 1 toggle drawer, 2 flip pre/post, 3 (V68) back to the standard order; mid = 0..4 for movable modules
        struct Chip { juce::Rectangle<float> r; int action; int mid = -1; };
        void doAction (int action);
        std::array<int, 5> currentOrder() const;
        int dragMid = -1, dropSlot = -1, pressedAction = 0;
        bool dragging = false;
        std::vector<Chip> chips;
        void timerCallback() override { repaint(); }
        VoxStripProcessor& proc;
    };

    //==========================================================================
    // Goliath Limiter module: SPL-style round LED ladders for In L/R, gain reduction and Out L/R
    class LedMeterBlock : public juce::Component, private juce::Timer
    {
    public:
        explicit LedMeterBlock (const MeterBridge& b) : bridge (b) { startTimerHz (30); }
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override { repaint(); }
        const MeterBridge& bridge;
    };

    //==========================================================================
    // Goliath display: scrolling output envelope with limiting shown in red (bx-style waveform view)
    class LimiterScope : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit LimiterScope (VoxStripProcessor&);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;   // click the LUFS readout to reset the integrated reading
    private:
        void timerCallback() override;
        void stepTarget();     // V62: nudges the limiter threshold toward the loudness target
        VoxStripProcessor& proc;
        float targetLufs = 0.0f;   // V62: 0 = off
        int targetTicks = 0, targetGood = 0;
        juce::String targetMsg;
        static constexpr int columns = 220;
        std::array<float, columns> level {}, reduction {};
        int head = 0;
    };

    //==========================================================================
    // V66: Auto Input - click, play the loudest part for a few seconds, and it sets Input so the strip is hit
    // at about -18 dBFS average with the peaks no higher than -6 dBFS (where the presets are voiced)
    class AutoInputButton : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit AutoInputButton (VoxStripProcessor& p);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { toggle(); }
        void toggle();   // start listening, or stop
    private:
        void timerCallback() override;
        void finish();
        VoxStripProcessor& proc;
        bool listening = false;
        int ticks = 0, idleTicks = 0;
        std::vector<float> rmsDb;
        float peak = 0.0f;
        juce::String text { "auto" };
    };

    //==========================================================================
    // V63: phase correlation of the output: +1 = mono, 0 = very wide, below 0 = out of phase (trouble in mono)
    class PhaseMeter : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit PhaseMeter (std::atomic<float>& c) : corr (c)
        {
            startTimerHz (30);
            setTooltip ("Phase: how the left and right agree. +1 = mono, around +0.3 to +0.8 = a healthy stereo mix, "
                        "near 0 = very wide, below 0 (red) = parts cancel when played in mono - narrow the width or check the phase.");
        }
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override { const float v = corr.load(); shown += (v - shown) * 0.25f; low = juce::jmin (low * 0.995f + v * 0.005f, v) ; repaint(); }
        std::atomic<float>& corr;
        float shown = 1.0f, low = 1.0f;
    };

    // V69: limiter engine - three LEDs, Classic / Multi / Max (drives limMode + limMax)
    class LimEngineBox : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
    public:
        explicit LimEngineBox (juce::AudioProcessorValueTreeState& s) : apvts (s)
        {
            startTimerHz (10);
            setTooltip ("Limiter engine. Classic: one clean brickwall. Multi: splits the lows from the rest so the bass doesn't pull "
                        "the whole mix down (L3-style). Max: a loudness maximizer with its own styles (pick one under MAX STYLE), "
                        "a Character knob for how fast it lets go, and a Transients knob to keep the hits punchy.");
        }
        int current() const
        {
            if (apvts.getRawParameterValue ("limMax")->load() > 0.5f) return 2;
            return apvts.getRawParameterValue ("limMode")->load() > 0.5f ? 1 : 0;
        }
        void paint (juce::Graphics&) override;
        // V71: each engine has its own colour (the knobs follow it)
        static juce::Colour colourFor (int i) { return i == 1 ? juce::Colour (0xff22c3b8) : i == 2 ? juce::Colour (0xffff7a2f) : juce::Colour (0xff9b6bff); }
        void mouseDown (const juce::MouseEvent& e) override
        {
            const int i = juce::jlimit (0, 2, (int) ((float) e.y / (float) getHeight() * 3.0f));
            auto set = [this] (const char* id, float v)
            {
                if (auto* p = apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture(); }
            };
            set ("limMax", i == 2 ? 1.0f : 0.0f);
            if (i < 2) set ("limMode", (float) i);
            shown = i; repaint();
            if (onChange) onChange();
        }
        std::function<void()> onChange;
    private:
        void timerCallback() override { const int c = current(); if (c != shown) { shown = c; repaint(); if (onChange) onChange(); } }
        juce::AudioProcessorValueTreeState& apvts;
        int shown = -1;
    };

    //==========================================================================
    class Section : public juce::Component
    {
    public:
        Section (juce::AudioProcessorValueTreeState&, const juce::String& title,
                 const juce::String& powerParamId, const juce::Colour& accent);
        void paint (juce::Graphics&) override;
        void resized() override;
        void lookAndFeelChanged() override;
        juce::Rectangle<int> getContent() const
        {
            if (isGoliath() && compact) return getLocalBounds().reduced (12, 0).withTrimmedTop (42).withTrimmedBottom (10);   // V36 drawer: slim header, no screws
            if (isGoliath() && MISHKAL_V52) return getLocalBounds().reduced (12, 0).withTrimmedTop (54).withTrimmedBottom (14);   // V52: no screws - the title tab moves up
            if (isGoliath()) return getLocalBounds().reduced (12, 0).withTrimmedTop (90).withTrimmedBottom (46);   // below the title tab, above the screws
            return getLocalBounds().reduced (12).withTrimmedTop (30);
        }

        // Rack skin layout hints (set by the editor)
        std::vector<int> dividers;            // x positions of slanted grooves
        bool screwsLeft = true, screwsRight = true, grooveLeft = false, isScreenUnit = false;
        bool compact = false;                 // V36: slim Goliath header (the drawer units)
        juce::Rectangle<int> getHeaderRight() const { return getLocalBounds().reduced (12, 0).removeFromTop (38).reduced (0, 8).withTrimmedLeft (160); }

        const juce::Colour* accent;
        HwLook look;                          // classic-hardware faceplate (inactive = skin)
        void setLook (const HwLook& l) { look = l; repaint(); }
        void setPowerCaption (const juce::String& c) { power.getProperties().set ("caption", c); power.repaint(); }   // Goliath: name above the ON button
    private:
        juce::String title;
        juce::ToggleButton power;
        juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
    };
}

//==============================================================================
class VoxStripEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    bool sizeReady = false;   // set once the saved size is applied (so setup steps can't overwrite it)
    static constexpr int ioStripWidth = 84;                     // input / output meter strips on the edges
    int baseWidth = 1140 + 2 * (ioStripWidth + 12), baseHeight = 970;   // layout size at 100% (Goliath: 1440 x 940)
    void applyBaseSize();          // pick the layout size for the current skin and resize the window to match
    void reparentForSkin();        // Goliath moves the EQ graph into the top display
    void layoutGoliath();
    void layoutCompGoliath();
    void layoutCharEqGoliath();
    juce::Rectangle<int> goliathDisplay, goliathDeck, goliathGraphTitle;
    static inline const std::array<float, 5> uiScales { 0.75f, 0.9f, 1.0f, 1.15f, 1.25f };
    void setUiScale (float scale);
    void setSkin (int index);
    explicit VoxStripEditor (VoxStripProcessor&);
    ~VoxStripEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // starts a new undo step

private:
    void timerCallback() override;
    void layoutCompressor();
    void rebuildTexture();
    void showPresetMenu();
    void showMainMenu();
    void savePreset (bool askName);
    void searchPresets();   // V67
    void refreshToolbar();
    vox::ui::Knob& add (juce::Component& parent, const juce::String& paramId, const char* title,
                        const juce::Colour& accent, bool bipolar = false);

    VoxStripProcessor& proc;
    vox::ui::LookAndFeel lnf;
    std::vector<std::unique_ptr<vox::ui::Knob>> knobs;

    vox::ui::Section cleanSection, eqSection, compSection, dsSection, flowSection, tapeSection, limSection;
    vox::ui::FlowDisplay flowDisplay;
    vox::ui::Knob *fAmount{}, *fMix{};
    std::array<vox::ui::Knob*, 7> perKnobs {};          // V37: Perach - amount, low, body, presence, air, speed, mix
    vox::ui::ModeSelector flowModelSel;
    int shownFlowModel = -1;
    // V38: Perach's graph - the target (four regions, drag each handle up / down; double-click resets) and,
    // shaded underneath, what Perach is boosting / cutting right now
    struct PerachGraph : juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
        VoxStripProcessor& proc;
        static constexpr const char* names[4] { "LOW", "BODY", "PRESENCE", "AIR" };
        const char* ids[4] { vox::id::perLow, vox::id::perBody, vox::id::perPresence, vox::id::perAir };
        int dragging = -1, hovered = -1;
        float dragStartDb = 0.0f, dragStartY = 0.0f;
        std::vector<float> live;
        explicit PerachGraph (VoxStripProcessor& p) : proc (p)
        {
            setTooltip ("Drag a region up for more of it, down for less (Perach only adds or takes what the sound needs). "
                        "Double-click a handle to reset it. The shaded curve is what Perach is doing right now.");
            startTimerHz (30);
        }
        static constexpr float loHz = 30.0f, hiHz = 18000.0f, rangeDb = 8.0f;
        float xFor (float hz) const { return (float) getWidth() * std::log (hz / loHz) / std::log (hiHz / loHz); }
        float hzFor (float x) const { return loHz * std::pow (hiHz / loHz, x / (float) juce::jmax (1, getWidth())); }
        juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().withTrimmedBottom (16.0f); }
        float yFor (float db) const { auto p = plot(); return p.getCentreY() - db / rangeDb * (p.getHeight() * 0.5f - 6.0f); }
        float toneDb (int i) const { return proc.apvts.getRawParameterValue (ids[i])->load(); }
        float targetAt (float hz) const
        {
            float t = 0.0f;
            for (int i = 0; i < 4; ++i)
            {
                const float d = std::log2 (hz / vox::ToneShaper::toneHz[i]) / 0.75f;
                t += toneDb (i) * std::exp (-0.5f * d * d);
            }
            return t;
        }
        juce::Point<float> handle (int i) const { return { xFor (vox::ToneShaper::toneHz[i]), yFor (toneDb (i)) }; }
        int hit (juce::Point<float> p) const
        {
            int best = -1; float bd = 1.0e9f;
            for (int i = 0; i < 4; ++i)   // grab anywhere in the region's column
            {
                const float dx = std::abs (p.x - handle (i).x);
                if (dx < bd && dx < (float) getWidth() * 0.12f) { bd = dx; best = i; }
            }
            return best;
        }
        void paint (juce::Graphics& g) override
        {
            const auto rose = juce::Colour (0xffff5fa8);
            auto r = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff0b0b0d));
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (juce::Colour (0xff26272c));
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
            auto p = plot();
            g.setColour (juce::Colour (0xff1c1d21));
            for (float hz : { 100.0f, 1000.0f, 10000.0f }) g.drawVerticalLine ((int) xFor (hz), p.getY() + 4.0f, p.getBottom());
            g.setColour (juce::Colour (0xff2a2b30));
            g.drawHorizontalLine ((int) yFor (0.0f), 4.0f, r.getRight() - 4.0f);

            // live correction (shaded, from the zero line)
            auto& ts = proc.getChain().perach;
            const int nb = ts.numBins.load();
            const double sr = ts.getSampleRate();
            if (nb > 2 && sr > 0.0)
            {
                juce::Path area;
                area.startNewSubPath (0.0f, yFor (0.0f));
                for (int x = 0; x <= getWidth(); x += 2)
                {
                    const float hz = hzFor ((float) x);
                    const int k = juce::jlimit (1, nb - 1, (int) std::round (hz / (sr / ((nb - 1) * 2))));
                    const float db = juce::jlimit (-rangeDb, rangeDb, ts.shownDb[(size_t) k].load (std::memory_order_relaxed));
                    area.lineTo ((float) x, yFor (db));
                }
                area.lineTo ((float) getWidth(), yFor (0.0f));
                area.closeSubPath();
                g.setColour (rose.withAlpha (0.22f));
                g.fillPath (area);
            }

            // target curve and the four region handles
            juce::Path curve;
            for (int x = 0; x <= getWidth(); x += 2)
            {
                const float y = yFor (targetAt (hzFor ((float) x)));
                if (x == 0) curve.startNewSubPath (0.0f, y); else curve.lineTo ((float) x, y);
            }
            g.setColour (rose.withAlpha (0.25f));
            g.strokePath (curve, juce::PathStrokeType (5.0f));
            g.setColour (rose);
            g.strokePath (curve, juce::PathStrokeType (1.8f));
            for (int i = 0; i < 4; ++i)
            {
                const auto h = handle (i);
                const bool hot = i == dragging || i == hovered;
                g.setColour (rose.withAlpha (hot ? 0.35f : 0.18f));
                g.fillEllipse (juce::Rectangle<float> (hot ? 22.0f : 18.0f, hot ? 22.0f : 18.0f).withCentre (h));
                g.setColour (juce::Colour (0xffffe3f0));
                g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (h));
                g.setFont (vox::ui::font (10.0f, true).withExtraKerningFactor (0.1f));
                g.setColour (hot ? juce::Colour (0xfff2f0eb) : juce::Colour (0xff8a8c93));
                const auto lab = juce::Rectangle<float> (80.0f, 14.0f).withCentre ({ h.x, r.getBottom() - 9.0f });
                g.drawText (hot ? (juce::String (toneDb (i) > 0.04f ? "+" : "") + juce::String (toneDb (i), 1) + " dB") : juce::String (names[i]),
                            lab, juce::Justification::centred);
            }
        }
        void mouseMove (const juce::MouseEvent& e) override { const int h = hit (e.position); if (h != hovered) { hovered = h; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }
        void mouseDown (const juce::MouseEvent& e) override
        {
            dragging = hit (e.position);
            if (dragging < 0) return;
            dragStartDb = toneDb (dragging);
            dragStartY = e.position.y;
            proc.apvts.getParameter (ids[dragging])->beginChangeGesture();
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragging < 0) return;
            const float perPx = rangeDb / (plot().getHeight() * 0.5f - 6.0f) * (e.mods.isShiftDown() ? 0.25f : 1.0f);
            const float db = juce::jlimit (-6.0f, 6.0f, dragStartDb - (e.position.y - dragStartY) * perPx);
            auto* prm = proc.apvts.getParameter (ids[dragging]);
            prm->setValueNotifyingHost (prm->convertTo0to1 (db));
        }
        void mouseUp (const juce::MouseEvent&) override
        {
            if (dragging >= 0) proc.apvts.getParameter (ids[dragging])->endChangeGesture();
            dragging = -1;
        }
        void mouseDoubleClick (const juce::MouseEvent& e) override
        {
            const int i = hit (e.position);
            if (i < 0) return;
            auto* prm = proc.apvts.getParameter (ids[i]);
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->getDefaultValue()); prm->endChangeGesture();
        }
        void timerCallback() override { if (isShowing()) repaint(); }
    };
    std::unique_ptr<PerachGraph> perachGraph;
    vox::ui::Knob *tDrive{}, *tMix{};
    std::array<vox::ui::Knob*, 3> eshKnobs {};          // V39: Esh - input, process, output
    vox::ui::Knob* tMsDrive {};
    vox::ui::Knob* lLink {};
    vox::ui::Knob *lCharacter {}, *lTransients {}, *lUpward {};                // V69: Max engine
    std::unique_ptr<vox::ui::LimEngineBox> limEngine;          // V69
    std::unique_ptr<vox::ui::ModeSelector> limStyleSel;        // V69
    std::array<std::unique_ptr<vox::ui::ModeSelector>, 8> charPlaceSel;   // V71: character EQ stereo / mid / side
    vox::ui::Knob* lWidth {};                            // V63: stereo width
    std::unique_ptr<vox::ui::PhaseMeter> phaseMeter;     // V63
    std::unique_ptr<vox::ui::AutoInputButton> autoInput; // V66                             // V61: limiter channel link (stereo tracks)                          // V60: tape M/S drive (both tape models, stereo tracks)
    bool shownMono = false;                              // V60: the track layout the editor was laid out for
    vox::ui::ModeSelector tapeModelSel, eshCharSel, eshToneSel;
    int shownTapeModel = -1;
    vox::ui::TypeBar eqTypes, compTypes;
    vox::ui::ModeSelector limMode, dsMode, eqPosition, tapeStyleSel, tapeWidthSel, scHpfSel, limSoftSel;
    vox::ui::ModeSelector tapeOsSel, limOsSel;          // oversampling 1x / 2x / 4x
    juce::Label tapeOsLabel { {}, "OS" }, limOsLabel { {}, "OS" };
    juce::Label limSoftLabel { {}, "SOFT dB" };   // Goliath: caption for the soft-saturation LEDs
    vox::ui::ModeSelector tahorPhaseSel, tahorOsSel;    // Tahor EQ: Natural / Linear / Linear HQ, and oversampling
    vox::ui::ModeSelector rcBehavior, rcCharacter;      // LE-RC: Glide / Snap, Color / Pure
    juce::ToggleButton rcArc { "AUTO REL" };            // LE-RC auto release
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rcArcAttachment;
    juce::Label tahorOsLabel { {}, "OS" };
    void showComp (int unit);
    void showCharEq (int type);
    void layoutCharEq();
    void applyHardwareLooks();
    std::vector<std::vector<vox::ui::Knob*>> eqGroups;   // knob groups for the EQ being edited
    vox::ui::CleanupGraph cleanGraph;
    vox::ui::BandPanel bandPanel;
    vox::ui::ResoPanel resoPanel;
    vox::ui::TabSwitch rightTabs { { "BAND", "RESO" }, { &vox::ui::pal::clean, &vox::ui::pal::reso } };
    vox::ui::TabSwitch skinTabs  { vox::ui::skinNames, { &vox::ui::pal::io, &vox::ui::pal::io, &vox::ui::pal::io } };
    juce::Image texture;

    // Everything lives in this container, laid out at the base size and scaled
    // with a transform. Scaling a child (not the editor) works in every plugin
    // format, including AAX, whose wrapper ignores editor transforms.
    struct Content : juce::Component
    {
        std::function<void (juce::Graphics&)> painter;
        void paint (juce::Graphics& g) override { if (painter) painter (g); }
    } content, goliathNotch,   // goliathNotch: the curved black tab where the display meets the modules
      goliathDrawer;             // Goliath: slide-up panel holding Zerem and Tape
    vox::ui::DrawerBar drawerBar { proc };
    bool drawerOpen = false;
    void setDrawerOpen (bool open, bool animate);
    juce::Rectangle<int> drawerBounds (bool open) const;
    float uiScale = 1.0f;
    void paintContent (juce::Graphics&);

    vox::ui::IconButton undoBtn { vox::ui::IconButton::Icon::Undo, false }, redoBtn { vox::ui::IconButton::Icon::Redo, false };
    vox::ui::IconButton prevBtn { vox::ui::IconButton::Icon::Left, true }, nextBtn { vox::ui::IconButton::Icon::Right, true };
    vox::ui::IconButton menuBtn { vox::ui::IconButton::Icon::Menu, true };
    vox::ui::PresetBox presetBox;
    juce::TextButton saveBtn { "SAVE" }, saveAsBtn { "SAVE AS" };
    juce::TextButton bypassBtn { "BYPASS" };   // master bypass: the whole plugin in or out
    juce::TextButton matchBtn { "MATCH" };
    juce::TextButton trackBtn { "TRACK" };     // low-latency tracking mode
    juce::TextButton osBtn { "OS OFF" };      // V33: whole-plugin oversampling, click to step Off / 2x / 4x
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trackAttachment;
    juce::TextButton peakBtn { "1964 PK" };       // Spectra-style input peak catcher, on the Input strip
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> peakAttachment;
    vox::ui::Knob *pkInKnob{}, *pkOutKnob{};                     // V24: the peak catcher's own input / output
    juce::ToggleButton peakLinkBtn { "link" };
    juce::Label pkLinkLabel { {}, "link" };                         // V25: name over the link switch (which sits between in and out)
    juce::ToggleButton pkPowerBtn;                                 // V25: the 1964 PK's own on / off (same parameter as the 1964 PK button)
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pkPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> peakLinkAttachment;
    float peakFlash = 0.0f;     // level match: the bypassed sound at the processed loudness
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> matchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    vox::ui::ABSwitch abSwitch;
    void showRightTab (int index);
    vox::ui::GrMeter compMeter, dsMeter, flowMeter, limMeter;
    vox::ui::IoStrip inStrip, outStrip;
    vox::ui::TapeVu tapeVu;
    vox::ui::LimiterScope limScope;
    vox::ui::MeterBridge meterBridge;
    vox::ui::LedMeterBlock ledBlock { meterBridge };
    vox::ui::GrRing grRing { meterBridge };
    // V18: the flat rounded panel behind the Limiter controls (same look as the Tahor / Reso panel)
    struct FlatPanel : juce::Component
    {
        juce::String title;
        juce::Colour dot { 0xff9b6bff };
        float flash = 0.0f;   // V54: 0..1, the light flashes red (1964 PK catching peaks)
        FlatPanel() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override
        {
            if (! vox::ui::isGoliath()) return;
            auto r = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff15161a));
            g.fillRoundedRectangle (r, 8.0f);
            g.setColour (juce::Colour (0xff24262b));
            g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
            if (title.isNotEmpty())
            {
                const auto lamp = juce::Rectangle<float> (14.0f, 11.0f, 8.0f, 8.0f);
                if (flash > 0.01f)   // V54: catching a peak - the light flashes red with a glow (brighter for bigger catches)
                {
                    const juce::Colour red (0xffff3b3b);
                    g.setColour (red.withAlpha (0.22f * flash)); g.fillEllipse (lamp.expanded (7.0f));
                    g.setColour (red.withAlpha (0.45f * flash)); g.fillEllipse (lamp.expanded (3.5f));
                    g.setColour (dot.interpolatedWith (red, juce::jmin (1.0f, flash * 1.4f)));
                    g.fillEllipse (lamp);
                    g.setColour (juce::Colours::white.withAlpha (0.6f * flash));
                    g.fillEllipse (lamp.reduced (2.5f));
                }
                else
                {
                g.setColour (dot);
                g.fillEllipse (lamp);
                }
                g.setColour (juce::Colour (0xffe9e7e2));
                g.setFont (vox::ui::screenFont (16.0f, 0).withExtraKerningFactor (0.06f));
                g.drawText (title, 30, 4, getWidth() - 40, 22, juce::Justification::centredLeft);
            }
        }
    } limCtrlPanel, toolsPanel, pkPanel, zeremPanel, tapePanel;
    // V48: arrows flowing from the control you push to the one the link moves (down = coral, up = cyan)
    struct LinkFlow : juce::Component
    {
        LinkFlow() { setInterceptsMouseClicks (false, false); }
        float amount = 0.0f;   // the automatic change in dB (negative = turned down)
        float level = 0.0f;    // 0..1 signal level: brightens the flow
        float phase = 0.0f;
        bool linked = true;
        bool showValue = false;   // LE-RV: the real output under the arrows (its knob has no room inside)
        float result = 0.0f;
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            if (showValue)
            {
                auto txt = r.removeFromBottom (16.0f);
                if (linked && std::abs (amount) > 0.05f)
                {
                    const auto c2 = amount < 0.0f ? juce::Colour (0xffff5e7a) : juce::Colour (0xff4fe3ff);
                    g.setColour (c2);
                    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
                    g.drawText ((result > 0.0f ? "+" : "") + juce::String (result, 1) + " dB", txt, juce::Justification::centred);
                }
                r = r.withSizeKeepingCentre (r.getWidth(), 12.0f);
            }
            const float cy = r.getCentreY();
            const bool moving = linked && std::abs (amount) > 0.05f;
            const auto col = amount < 0.0f ? juce::Colour (0xffff5e7a) : juce::Colour (0xff4fe3ff);
            if (! linked)   // a broken link: two dim dashes with a gap
            {
                g.setColour (juce::Colour (0xff4a4c54));
                g.fillRoundedRectangle (r.getX() + 2.0f, cy - 1.0f, r.getWidth() * 0.38f, 2.0f, 1.0f);
                g.fillRoundedRectangle (r.getRight() - 2.0f - r.getWidth() * 0.38f, cy - 1.0f, r.getWidth() * 0.38f, 2.0f, 1.0f);
                return;
            }
            g.setColour ((moving ? col : juce::Colour (0xff6a6d78)).withAlpha (moving ? 0.25f : 0.35f));
            g.fillRoundedRectangle (r.getX() + 2.0f, cy - 1.0f, r.getWidth() - 4.0f, 2.0f, 1.0f);
            if (! moving) return;
            const float strength = juce::jlimit (0.0f, 1.0f, std::abs (amount) / 6.0f);
            const float a = (0.45f + 0.55f * strength) * (0.6f + 0.4f * level);
            const int n = 3;
            for (int i = 0; i < n; ++i)
            {
                const float t = std::fmod (phase + (float) i / (float) n, 1.0f);
                const float x = r.getX() + 3.0f + t * (r.getWidth() - 10.0f);
                const float fade = std::sin (t * juce::MathConstants<float>::pi);   // fade in and out at the ends
                juce::Path chev;
                chev.startNewSubPath (x, cy - 4.0f); chev.lineTo (x + 4.0f, cy); chev.lineTo (x, cy + 4.0f);
                g.setColour (col.withAlpha (0.25f * a * fade));
                g.strokePath (chev, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (col.withAlpha (a * fade));
                g.strokePath (chev, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    } pkFlow, rvFlow;
    float linkPhase = 0.0f;
    // V22: the faceplate behind the compressor's own controls, in the style of the unit it is modelled on
    struct HwPlate : juce::Component
    {
        vox::ui::HwLook look;
        juce::String legend;
        juce::Colour stripe { juce::Colours::transparentBlack };   // V27: a coloured band behind the legend (Blue Stripe)
        juce::String badge;                                         // V76: a small coloured badge before the legend
        juce::Colour badgeColour { 0xff1f4fa8 };
        juce::Colour frameColour { juce::Colours::transparentBlack };   // V76: a printed line framing the controls (console style)
        HwPlate() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override
        {
            if (! vox::ui::isGoliath() || ! look.active) return;
            auto r = getLocalBounds().toFloat();
            vox::ui::paintFaceplate (g, r, look, 6.0f);
            for (auto p : { juce::Point<float> (9.0f, 9.0f), juce::Point<float> (r.getRight() - 9.0f, 9.0f),
                            juce::Point<float> (9.0f, r.getBottom() - 9.0f), juce::Point<float> (r.getRight() - 9.0f, r.getBottom() - 9.0f) })
            {
                g.setColour (juce::Colours::black.withAlpha (0.5f));
                g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (p.translated (0.0f, 0.6f)));
                g.setColour (look.lightPlate ? juce::Colour (0xffd8d6d0) : juce::Colour (0xff8c8f93));
                g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (p));
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.drawLine (p.x - 2.2f, p.y, p.x + 2.2f, p.y, 1.0f);
            }
            if (! stripe.isTransparent())
            {
                auto band = juce::Rectangle<float> (r.getX() + 1.0f, r.getY() + 3.0f, r.getWidth() - 2.0f, 16.0f);
                g.setGradientFill (juce::ColourGradient (stripe.brighter (0.25f), 0.0f, band.getY(), stripe.darker (0.3f), 0.0f, band.getBottom(), false));
                g.fillRect (band);
            }
            const auto legendFont = vox::ui::font (10.5f, true).withExtraKerningFactor (0.14f);
            auto legendRow = r.reduced (20.0f, 4.0f).removeFromTop (16.0f);
            const float tw = juce::GlyphArrangement::getStringWidth (legendFont, legend);
            const float bw = badge.isEmpty() ? 0.0f : 8.0f + 7.5f * (float) badge.length();
            const float x0 = legendRow.getCentreX() - (tw + (bw > 0.0f ? bw + 7.0f : 0.0f)) * 0.5f;
            if (! frameColour.isTransparent())   // printed frame around the controls, broken where the legend sits
            {
                const float y = legendRow.getCentreY(), L = r.getX() + 13.0f, R = r.getRight() - 13.0f, B = r.getBottom() - 15.0f;
                const float gapL = x0 - 8.0f, gapR = x0 + tw + (bw > 0.0f ? bw + 7.0f : 0.0f) + 8.0f;
                juce::Path f;
                f.startNewSubPath (gapR, y); f.lineTo (R - 4.0f, y); f.quadraticTo (R, y, R, y + 4.0f); f.lineTo (R, B - 4.0f);
                f.quadraticTo (R, B, R - 4.0f, B); f.lineTo (L + 4.0f, B); f.quadraticTo (L, B, L, B - 4.0f); f.lineTo (L, y + 4.0f);
                f.quadraticTo (L, y, L + 4.0f, y); f.lineTo (gapL, y);
                g.setColour (frameColour);
                g.strokePath (f, juce::PathStrokeType (1.0f));
            }
            if (bw > 0.0f)
            {
                auto b = juce::Rectangle<float> (x0, legendRow.getCentreY() - 7.0f, bw, 14.0f);
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRoundedRectangle (b.translated (0.0f, 0.8f), 3.0f);
                g.setGradientFill (juce::ColourGradient (badgeColour.brighter (0.25f), 0.0f, b.getY(), badgeColour.darker (0.25f), 0.0f, b.getBottom(), false));
                g.fillRoundedRectangle (b, 3.0f);
                g.setColour (juce::Colours::white);
                g.setFont (vox::ui::font (10.5f, true).withExtraKerningFactor (0.06f));
                g.drawText (badge, b, juce::Justification::centred);
                legendRow.setLeft (b.getRight() + 7.0f);
                g.setColour (stripe.isTransparent() ? look.label : juce::Colours::white);
                g.setFont (legendFont);
                g.drawText (legend, legendRow, juce::Justification::centredLeft);
            }
            else
            {
                g.setColour (stripe.isTransparent() ? look.label : juce::Colours::white);
                g.setFont (legendFont);
                g.drawText (legend, r.reduced (20.0f, 4.0f).removeFromTop (16.0f), juce::Justification::centred);
            }
        }
    } compPlate;
    // V76: a backlit needle gain-reduction meter (console bus compressor): rests at 0 on the right, swings left as it compresses
    struct NeedleGr : juce::Component, private juce::Timer
    {
        std::atomic<float>* src = nullptr;
        float shown = 0.0f;
        NeedleGr() { setInterceptsMouseClicks (false, false); }
        void visibilityChanged() override { if (isVisible()) startTimerHz (30); else stopTimer(); }
        void timerCallback() override
        {
            const float v = src != nullptr ? juce::jlimit (0.0f, 24.0f, src->load()) : 0.0f;
            const float k = v > shown ? 0.35f : 0.12f;   // a little faster down than back
            if (std::abs (v - shown) > 0.01f) { shown += (v - shown) * k; repaint(); }
        }
        static float angleFor (float db)   // 0 dB at the right, 20 dB at the left
        {
            const float f = std::sqrt (juce::jlimit (0.0f, 20.0f, db) / 20.0f);
            return juce::jmap (f, 0.85f, -0.85f);
        }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (r.translated (0.0f, 1.0f), 4.0f);
            g.setColour (juce::Colour (0xff111214));
            g.fillRoundedRectangle (r, 4.0f);
            auto face = r.reduced (4.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6e7b8), face.getCentreX(), face.getBottom(),
                                                     juce::Colour (0xffc9b37a), face.getCentreX(), face.getY() - face.getHeight() * 0.4f, true));
            g.fillRoundedRectangle (face, 2.5f);
            const float H = face.getHeight();
            const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + H * 0.25f);
            const float ra = H * 1.25f - 12.0f;   // the scale arc
            const float R = ra / 0.80f;
            g.setColour (juce::Colour (0xff1b1712));
            juce::Path arc;
            arc.addCentredArc (pivot.x, pivot.y, ra, ra, 0.0f, angleFor (20.0f), angleFor (0.0f), true);
            g.strokePath (arc, juce::PathStrokeType (1.0f));
            g.setFont (vox::ui::font (8.5f, true));
            for (float db : { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 14.0f, 20.0f })
            {
                const float a = angleFor (db);
                const bool big = db == 0.0f || db == 4.0f || db == 10.0f || db == 20.0f;
                g.setColour (db >= 10.0f ? juce::Colour (0xffb3261e) : juce::Colour (0xff1b1712));
                g.drawLine ({ pivot.getPointOnCircumference (ra, a), pivot.getPointOnCircumference (ra + (big ? 6.0f : 4.0f), a) }, big ? 1.4f : 1.0f);
                if (big)
                    g.drawText (juce::String ((int) db), juce::Rectangle<float> (16.0f, 10.0f).withCentre (pivot.getPointOnCircumference (ra - 8.0f, a)),
                                juce::Justification::centred);
            }
            g.setColour (juce::Colour (0xff1b1712).withAlpha (0.8f));
            g.setFont (vox::ui::font (8.5f, true).withExtraKerningFactor (0.12f));
            g.drawText ("GR  dB", face.withTrimmedTop (H - 15.0f), juce::Justification::centred);
            g.saveState();
            g.reduceClipRegion (face.toNearestInt());
            const float a = angleFor (shown);
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.drawLine ({ pivot.translated (1.5f, 1.0f), pivot.getPointOnCircumference (R * 0.93f, a).translated (1.5f, 1.0f) }, 1.6f);
            g.setColour (juce::Colour (0xff141210));
            g.drawLine ({ pivot, pivot.getPointOnCircumference (R * 0.93f, a) }, 1.4f);
            g.restoreState();
            g.setColour (juce::Colours::white.withAlpha (0.10f));   // glass
            g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.35f), 2.5f);
        }
    } gNeedle;
    vox::ui::ModeSelector fetRevSel;   // V27: LE-76 revision
    vox::ui::ModeSelector rcModelSel;  // V30: LE-RC model (RComp / RVox)
    vox::ui::ModeSelector tubeModelSel, staModeSel, staRelSel;   // V49: tube slot model (LE-660 / LE-STA), LE-STA mode and recovery
    std::vector<std::pair<int, std::unique_ptr<vox::ui::ModeSelector>>> mcompSw;   // V75: the new compressors' switches (unit, switch)
    int shownTubeModel = -1;
    bool threshOnStaIn = false;
    int shownRcModel = -1;
    juce::ToggleButton voxLinkBtn;
    juce::Label voxLinkLabel { {}, "link" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> voxLinkAttachment;
    // V29: the classic FET unit's ratio push-buttons (20 / 12 / 8 / 4), driving the Ratio knob's parameter
    struct RatioButtons : juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
        juce::Slider* target = nullptr;
        juce::Colour legend { 0xfff2f2f2 };
        bool lightPlate = false;
        static constexpr float values[4] { 20.0f, 12.0f, 8.0f, 4.0f };
        int shown = -1;
        RatioButtons() { startTimerHz (15); }
        int selected() const
        {
            if (target == nullptr) return -1;
            const double v = target->getValue();
            int best = 0;
            for (int i = 1; i < 4; ++i) if (std::abs (std::log (v / values[i])) < std::abs (std::log (v / values[best]))) best = i;
            return best;
        }
        juce::Rectangle<float> buttonRect (int i) const
        {
            auto r = getLocalBounds().toFloat();
            r.removeFromTop (18.0f);
            const float h = juce::jmin (26.0f, (r.getHeight() - 9.0f) / 4.0f);
            return juce::Rectangle<float> (juce::jmin (r.getWidth() - 4.0f, 46.0f), h - 4.0f)
                       .withCentre ({ r.getCentreX(), r.getY() + h * ((float) i + 0.5f) });
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (legend);
            g.setFont (vox::ui::font (10.5f, true).withExtraKerningFactor (0.12f));
            g.drawText ("RATIO", getLocalBounds().removeFromTop (16), juce::Justification::centred);
            const int sel = selected();
            for (int i = 0; i < 4; ++i)
            {
                auto b = buttonRect (i);
                const bool on = i == sel;
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.fillRoundedRectangle (b.translated (0.0f, on ? 0.5f : 1.5f), 2.5f);
                auto cap = on ? b.translated (0.0f, 1.0f) : b;
                g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffd9d9d4) : juce::Colour (0xfff4f3ee), 0.0f, cap.getY(),
                                                         on ? juce::Colour (0xffa9a9a4) : juce::Colour (0xffc9c8c2), 0.0f, cap.getBottom(), false));
                g.fillRoundedRectangle (cap, 2.5f);
                g.setColour (juce::Colours::black.withAlpha (0.5f));
                g.drawRoundedRectangle (cap, 2.5f, 1.0f);
                if (on)   // pressed: sits lower, with a lit pip
                {
                    g.setColour (juce::Colour (0xffff4a3d));
                    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ cap.getX() + 7.0f, cap.getCentreY() }));
                }
                g.setColour (juce::Colour (0xff1a1a1a));
                g.setFont (vox::ui::font (12.0f, true));
                g.drawText (juce::String ((int) values[i]), cap, juce::Justification::centred);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            for (int i = 0; i < 4; ++i)
                if (target != nullptr && buttonRect (i).expanded (2.0f).contains (e.position))
                {
                    target->setValue (values[i], juce::sendNotificationSync);
                    repaint();
                }
        }
        void timerCallback() override { const int s = selected(); if (s != shown) { shown = s; repaint(); } }
    } ratioButtons;
    int shownFetRev = -1;
    vox::ui::DynMeterBlock dynBlock { meterBridge, proc.meters().gateGr };
    vox::ui::CharEqScreen charScreen { proc, eqTypes };   // V2 only
    juce::ToggleButton resoPowerBtn;                       // Goliath: Reso's own ON button beside Tahor's
    vox::ui::ViewTab tahorViewTab { "TAHOR" }, resoViewTab { "RESO" };   // click a name to see its controls
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> resoPowerAttachment;
    vox::ui::Knob* limSoftKnob = nullptr;   // Goliath: soft saturation as a stepped knob
    vox::ui::ModeSelector p1LowFreqSel;     // Goliath: LE-EP1 low frequency as a column of lights (20 / 30 / 60 / 100)
    juce::ToggleButton charMoreBtn { "MORE" };
    vox::ui::EqDropdown eqDropdown { eqTypes, [] (int t) { static const char* d[] { "Tube program EQ", "British console EQ", "Class-A EQ", "Air + silk EQ",
                                                                                   "Punchy American EQ", "Clean console EQ", "Vintage British EQ", "Fixed-band air EQ" };
                                                           return juce::String (d[juce::jlimit (0, 7, t)]); }, "EQ" };
    vox::ui::EqDropdown compDropdown { compTypes, [] (int u) { static const char* d[] { "FET compressor", "Clean all-rounder", MISHKAL_V52 ? "Tube vari-mu" : MISHKAL_V49 ? "Tube compressor" : "Tube vari-mu", "Tube program leveler",
                                                                                      "Punchy bus compressor", "Console bus compressor", "Mastering dynamics", "Tube mastering compressor", "Diode-bridge comp / limiter" };
                                                               return juce::String (d[juce::jlimit (0, 8, u)]); }, "compressor" };   // Goliath: flips the Character EQ knobs to their second page
    bool charEqMore = false;
    vox::ui::ChainLane eqLane { eqTypes, [] (int t) { return vox::ui::eqModeColour ((vox::EqType) t); }, "EQ" };
    vox::ui::ChainLane compLane { compTypes, [] (int u) { return vox::ui::compModeColour ((vox::CompType) u); }, "compressor" };
    vox::ui::GateLed gateLed, breathLed;
    juce::Label compInfo;

    juce::ToggleButton tapeNoiseBtn { "NOISE" };   // tape hiss on/off
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tapeNoiseAttachment;
    juce::ToggleButton listen { "LISTEN" };
    juce::ToggleButton dsKeyBtn { "EXT KEY" };                                          // V64
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dsKeyAtt;    // V64
    std::unique_ptr<vox::ui::ModeSelector> compKeySel;                                  // V64: int / ext
    juce::ToggleButton compDeltaBtn { "DELTA" }, limDeltaBtn { "DELTA" };              // V65
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compDeltaAtt, limDeltaAtt;
    juce::AudioProcessorValueTreeState::ButtonAttachment listenAttachment;
    juce::ToggleButton dsAuto { "AUTO" };
    juce::AudioProcessorValueTreeState::ButtonAttachment dsAutoAttachment;
    juce::ToggleButton arc { "AUTO REL" };   // limiter auto release
    juce::AudioProcessorValueTreeState::ButtonAttachment arcAttachment;
    juce::ToggleButton limTpBtn { "TRUE PEAK" };                                         // V62
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limTpAtt;    // V62
    std::unique_ptr<vox::ui::ModeSelector> limClipSel;                                  // V62

    vox::ui::Knob *inputKnob{}, *outputKnob{};
    vox::ui::Knob *hpf{}, *bodyF{}, *bodyG{}, *mudF{}, *mudG{}, *mudQ{}, *presF{}, *presG{}, *presQ{}, *airF{}, *airG{}, *silk{};
    vox::ui::Knob *cGate{}, *cThresh{}, *cRatio{}, *cAttack{}, *cRelease{}, *cMakeup{}, *cTc{}, *cMix{};
    vox::ui::Knob *cBreath{}, *cRider{};   // before the compressors: breath control and the vocal rider
    vox::ui::Knob *dFreq{}, *dThresh{}, *dRange{};
    vox::ui::Knob *lThresh{}, *lCeiling{}, *lRelease{}, *lMix{};
    juce::OwnedArray<juce::Label> eqColumnLabels;

    int shownCompType = -1;
    float gateMeterDb = -120.0f, gatePeakDb = -120.0f;
    // de-esser Freq slider: a small spectrum meter of the signal going into the de-esser
    static constexpr int dsFftOrder = 11, dsFftSize = 1 << dsFftOrder, dsCells = 44;
    juce::dsp::FFT dsFft { dsFftOrder };
    juce::dsp::WindowingFunction<float> dsWindow { (size_t) dsFftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> dsWin = std::vector<float> ((size_t) dsFftSize, 0.0f);
    std::vector<float> dsFftData = std::vector<float> ((size_t) dsFftSize * 2, 0.0f);
    std::array<float, dsCells> dsCellDb {};
    void updateDeEssMeter();
    int gatePeakHold = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxStripEditor)
};
