#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "TahorProcessor.h"
#include <set>

namespace tahor
{
// The whole screen is one canvas drawn at the design's own size (1200 x 800) and scaled to the window.
// Everything is placed with the numbers of the approved Claude Design canvas (Tabs.dc.html / Units.dc.html).
namespace ui
{
    using R = juce::Rectangle<float>;
    using P = juce::Point<float>;
    inline constexpr float W = 1252.0f, H = 800.0f;   // V2: an input meter on the left (the design's 1200 + 52)
    // V3: the screen runs nearly edge to edge (Pro-Q style): thin meters at the window's edges, the graph between them
    inline constexpr float PW = 1216.0f;                            // the graph's width (was 1100)
    inline const R display { 18.0f, 70.0f, PW, 670.0f };
    inline constexpr float dx = 0.0f;
    // frequency <-> x inside the display (20 Hz .. 20 kHz over 1100 px, as the design's curves)
    // V12: 20 Hz and 20 kHz both sit inside the screen (room for their labels)
    inline constexpr float FX0 = 22.0f, FXW = PW - 22.0f - 46.0f;
    inline float fx (float hz) { return FX0 + FXW * std::log10 (juce::jmax (1.0f, hz) / 20.0f) / 3.0f; }
    inline float xf (float x)  { return 20.0f * std::pow (10.0f, 3.0f * (x - FX0) / FXW); }
    juce::Colour col (const char* hex, float alpha = 1.0f);
    juce::Colour col (juce::uint32 rgb, float alpha = 1.0f);

    enum class Face { Light, Regular, Medium, SemiBold, CondMedium, CondSemiBold };
    juce::Font font (Face f, float px, float letterSpacing = 0.0f);
    float textWidth (const juce::Font& f, const juce::String& s);
    void text (juce::Graphics& g, const juce::String& s, const juce::Font& f, juce::Colour c, R box,
               juce::Justification j = juce::Justification::centredLeft);
    inline float textScale = 1.0f;   // V17: the band panels' writing 20% bigger (set while a panel draws)
    struct BiggerText { BiggerText (float k) { textScale = k; } ~BiggerText() { textScale = 1.0f; } };
}

enum class Hit
{
    None, Undo, Redo, Preset, Save, SaveAs, AB, Match, Bypass, Tab, TabPower, Phase, Os, OsRender, Analyzer, Range,
    InBox, Auto, OutBox, ScExt, Resize,
    ResoKnob, ResoMode, LowLat, ResoMs, Delta, ResoSc, Adv, Strip, AdvClose, TiltKnob, LinkKnob, ResoPhase,
    FocusEdge, ResoNode, ResoShape, ResoNodeKnob, ResoBg,
    TahorNode, TahorType, TahorPlace, TahorKnob, TahorChip, TahorBand, AddBand, TahorBg,
    UnitPrev, UnitNext, UnitAdd, UnitKnob, UnitSwitch, PageDot, ChainChip, ChainRemove, CharMode, PeakDot, Glass, FullScreen, CharNode, Hardware, TabCycle, Size, CharKnob, Thd, AddEq, PresetStep, BandSolo, AddPick, AddClose,
    History, Browse, BrowseTag, BrowsePick, BrowseClose, AutoGain, CharSide, ResoPlace, TahorMode,
    BandPower, DynPower, DynRange, SpecPeak, EqMatch, Piano, BubbleEdit,
    KnobValue, Chain   // V16: the number under a knob (click: type it) · a block of the chain strip
};

class Canvas : public juce::Component, private juce::Timer
{
public:
    explicit Canvas (TahorProcessor&);
    ~Canvas() override;
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;   // V11: shortcuts

    std::function<void (float scale)> onResize;   // the corner grip
    std::function<void()> onFullScreen;           // V2: the full-screen button
    std::function<void (float)> onSize;           // V3: the size menu (Mini .. Extra Large)
    float currentScale = 1.0f;

    // UI state (kept in the processor's state tree so it comes back with the session)
    int tab = 0;              // 0 RESO, 1 TAHOR, 2 CHARACTER
    int selBand = 2, selNode = 1, preview = 0, hoverUnit = -1, selCharNode = -1;
    bool fullScreen = false;
    bool stripOpen = true, advOpen = false;

private:
    TahorProcessor& proc;
    void timerCallback() override;

    // painting (TahorPaint.cpp)
    void paintHeader (juce::Graphics&);
    void paintDisplay (juce::Graphics&);
    void paintSpectrum (juce::Graphics&);
    void paintSpectrumInner (juce::Graphics&, int mode);
    void paintResoLayer (juce::Graphics&);
    void paintCurves (juce::Graphics&);
    void paintLegend (juce::Graphics&);
    void paintResoTab (juce::Graphics&);
    void paintResoTabInner (juce::Graphics&);
    void paintResoStrip (juce::Graphics&);
    void paintAdvanced (juce::Graphics&);
    void paintTahorTab (juce::Graphics&);
    void paintCharTab (juce::Graphics&);
    void paintFaceplate (juce::Graphics&, ui::R plate, int unit);
    void paintMeters (juce::Graphics&);
    void paintBottomBar (juce::Graphics&);
    void neonKnob (juce::Graphics&, ui::P topLeft, float pos, float size = 52.0f);   // the silver ring knob (52 px in the design)
    float archTop[2] { 400, 800 }, archBot[2] { 400, 800 };   // V5: where the screen arches up into the header / down into the bar
    juce::Path screenPath() const;
    float archHalf() const;
    static constexpr float screenTop = 44.0f;   // V8
    bool addOpen = false;                        // V8: the + Add EQ gallery
    int charSolo = -1, charSoloUnit = -1;        // V8: the soloed Character node
    void paintAddGallery (juce::Graphics&);
    // V9
    bool browseOpen = false; juce::String browseTag = "All";
    int resoSolo = -1;                 // the soloed Reso node
    int charSide = 0;                  // a split Character unit: 0 = left, 1 = right on show
    float soloHzUi = 0.0f, soloQUi = 1.0f;   // set while painting, handed to the processor
    void paintBrowser (juce::Graphics&);
    // V10
    float anaAlpha = 0.0f, hwArrowA = 0.0f;
    float pkHold[2] { -90.0f, -90.0f }, avgDb[2] { -90.0f, -90.0f }; double pkTime[2] { 0, 0 };
    // V12
    std::array<bool, numBands> dynView {};                 // a band panel showing its dynamic controls (Dynamic stays on either way)
    std::set<int> selSet;                           // several Tahor bands selected together
    bool lasso = false; ui::P lassoA, lassoB;
    std::vector<std::pair<float, float>> groupStart; // each selected band's frequency / gain when a group drag began
    struct Clip { int kind = -1; std::vector<std::pair<juce::String, float>> v; } clip;   // cut / copy / paste
    std::vector<ui::P> specPeaks;                   // Spectrum Grab: peaks in the analyzer (display coords)
    int eqMatch = 0; int eqCount = 0;
    bool pianoOn = false;

    std::vector<float> keyWin, keyDb, accPre, accKey;
    void showNodeMenu (Hit kind, int idx, int side, ui::R at);
    void applyEqMatch();
    void copyBand (Hit kind, int idx);
    void pasteBand (Hit kind);
    void paintSoloColumn (juce::Graphics&);
    juce::String cuid (int v) const { return charSide == 1 && proc.unitSplit (preview) ? "u" + juce::String (preview) + "r" + juce::String (v) : uid (preview, v); }
    void splitTahor (int band, int place);
    void splitReso (int node, int place);
    void setCharSide (int s);
    void showHistoryMenu (ui::R at);
    ui::R presetRect { 216, 14, 176, 32 };
    void bandKnob (juce::Graphics&, ui::R box, float pos, juce::Colour c);           // the Tahor pop-up knob
    void vintageKnob (juce::Graphics&, ui::P topLeft, int unit, int knob, float pos, float pos2);
    const juce::Image& metalImage (int unit, int w, int h);
    float activeResoReduction();
    float resizeFrom = 1.0f;

    // hit zones, rebuilt on every paint
    struct Zone { ui::R r; Hit kind; int a = 0, b = 0; Hit k0 = Hit::None; };   // k0: a KnobValue's knob
    std::vector<Zone> rangeZones;
    // V14: type a value straight into a bubble (frequency, gain, Q)
    std::unique_ptr<juce::TextEditor> typeIn;
    juce::String typeInPid; int typeInKind = 0; int typeInCharNode = -1;   // V17: a Character band's bubble (Hz / dB -> its knob)
    void openTypeIn (const Zone& z);
    void closeTypeIn (bool commit);
    // V16: Pro-Q 4 style on every knob - type any value, and a right-click menu
    void openParamTypeIn (const juce::String& pid, ui::R near);
    void showParamMenu (const juce::String& pid, ui::R near);
    juce::String pidOf (const Zone& z) const { if (z.kind == Hit::KnobValue) { Zone k = z; k.kind = z.k0; return knobPid (k, false); } return knobPid (z, false); }
    void paintCursorReadout (juce::Graphics&);   // V14: the dynamic range's grab spots (added after the background)
    std::vector<Zone> zones;
    static bool isKnobHit (Hit k) { return k == Hit::ResoKnob || k == Hit::TiltKnob || k == Hit::LinkKnob || k == Hit::ResoNodeKnob || k == Hit::TahorKnob || k == Hit::CharKnob; }
    void zone (ui::R r, Hit k, int a = 0, int b = 0)
    {
        zones.push_back ({ r, k, a, b });
        // V16: a knob's number sits just under it - click it to type a value (like Pro-Q 4)
        if (isKnobHit (k) && k != Hit::ResoNodeKnob && std::abs (r.getWidth() - r.getHeight()) < 6.0f)
        {
            const bool beside = k == Hit::ResoKnob || k == Hit::TiltKnob || k == Hit::LinkKnob;   // Reso's side panels: name and number to the right
            zones.push_back ({ beside ? ui::R (r.getRight(), r.getY(), 96, r.getHeight()) : ui::R (r.getX() - 12, r.getBottom(), r.getWidth() + 24, 22), Hit::KnobValue, a, b, k });
        }
    }
    const Zone* zoneAt (ui::P p) const;
    void resetAt (const Zone&);   // V12: what alt-click (and double-click, outside Character) does
    std::vector<ui::R> glassRects;   // translucent panels: solid while the mouse is over them
    float glass (ui::R r) const { return (r.contains (mouse) || (dragging && r.contains (downAt))) ? 1.0f : 0.6f; }

    // interaction
    ui::P mouse { -1, -1 }, downAt;
    bool dragging = false;
    Zone down {};
    juce::String dragPid, dragPid2;
    float dragStart = 0.0f, dragStart2 = 0.0f;
    bool dragOuter = false;
    int chainDragFrom = -1;
    void click (const Zone&, const juce::MouseEvent&);
    juce::String knobPid (const Zone&, bool outer) const;
    void startKnobDrag (const juce::String& pid);
    void showPresetMenu();
    void saveAs();
    void addTahorBand (ui::P at);
    void addResoNode (ui::P at);

    // the analyzer
    std::vector<float> preWin, postWin, fftData, preDb, postDb;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> hann;
    void analyse (std::vector<float>& win, std::vector<float>& smoothDb);
    std::vector<float> resoShown;   // per display column (smoothed)

    float pulse = 0.0f;             // the Tron outline's breathing (0..1)
    float meterDb[4] { -90.0f, -90.0f, -90.0f, -90.0f };   // in L / R, out L / R
    float drift = 0.0f;
    // V16: frame pacing and the drawing benchmark (only in the stand-alone app with Desktop/Mishkal Projects/Tahor-bench.on)
    double lastPaintMs = 0.0; int skipTick = 0;
    float freeze = 0.0f; int playTicks = 0;   // V17: 1 = the analyzer held still (the mouse is over the graph)
    int benchStep = -1; double benchAt = 0.0, benchSum = 0.0, benchMax = 0.0; int benchFrames = 0, benchSlow = 0; bool benchOn = false;
    void benchTick();
    // V16: the chain strip (RESO / TAHOR / CHARACTER order)
    void paintChainStrip (juce::Graphics&);
    void paintTopCentre (juce::Graphics&);
    void flipCharUnit (int from, int to);
    void chainMouseUp (const juce::MouseEvent&);
    int tchainDrag = -1; float tchainDragX = 0.0f; float chainSlot[3] {};
    double lastWant = 0.0;
    float reveal = 0.0f;                 // V3: the tab's controls fade in when the mouse touches a dot or their home
    std::vector<ui::R> revealRects;                                     // V2: the background's slow movement
    void paintBackground (juce::Graphics&);
    void paintBackgroundInner (juce::Graphics&);
    juce::Image bgCache; float bgScale = 0.0f; int bgKey = -1, bgDrift = -1;   // V14: the background, drawn once and reused
    std::vector<float> preCol, postCol;                     // V2: the smoothed analyzer, per display column
    std::map<int, juce::Image> metal;   // V10: keyed by unit * 1000 + height (1U / 2U)
    std::unique_ptr<juce::FileChooser> chooser;
};
}

class TahorEditor : public juce::AudioProcessorEditor
{
    float scaleBeforeFull = 1.0f;
    static float globalScale (float set);
public:
    explicit TahorEditor (TahorProcessor&);
    ~TahorEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0b0b0d)); }

private:
    TahorProcessor& proc;
    tahor::Canvas canvas;
    juce::ComponentBoundsConstrainer limits;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TahorEditor)
};
