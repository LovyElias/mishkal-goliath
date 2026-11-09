#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "WallProcessor.h"

// Mishkal Wall's screen: one canvas at the approved mock's own size (1252 x 800), scaled to the window.
namespace wallui
{
    using R = juce::Rectangle<float>;
    inline constexpr float W = 1252.0f, H = 800.0f, C = W * 0.5f;
    enum class Face { Light, Regular, Medium, SemiBold, CondMedium, CondSemiBold };

    enum class Hit
    {
        None, Knob, Toggle, Choice, CycleTab, Learn, Undo, Redo, History, Preset, PresetPrev, PresetNext, SavePreset,
        AB, Match, Power, StylePrev, StyleNext, StyleDot, Preview, Unity, View, ChainBlock, ChainDot, Size, Delta, Auto,
        GoldStep, BandBody, BandXover, BandAdd, BandRemove, BandSolo, BandByp, LoudReset, PanelClose, Ceiling
    };
}

class WallCanvas : public juce::Component, private juce::Timer
{
public:
    explicit WallCanvas (WallProcessor&);
    ~WallCanvas() override;
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { mouse = { -1, -1 }; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void (float)> onSize;
    std::function<void()> onFullScreen;
    float currentScale = 1.0f;
    bool fullScreen = false;

    // which screen: 0 the wave, 1 the bands, 2 loudness; and the stage open over it (-1 none, 0..5 a stage, 6 the limiter)
    int view = 0, stage = -1;
    int selBand = 1;
    float animT = 0.0f;   // seconds (for the moving bits)

private:
    WallProcessor& proc;
    void timerCallback() override;
    float pv (const char* id) const { return proc.param (id); }
    float pv (const juce::String& id) const { return proc.param (id); }
    float norm (const juce::String& id) const;

    // ---- painting (WallPaint.cpp)
    void paintScreen (juce::Graphics&);
    void paintWave (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintSides (juce::Graphics&);
    void paintMeters (juce::Graphics&);
    void paintGrMeter (juce::Graphics&);
    void paintCrestMeter (juce::Graphics&);
    void paintLoudStrip (juce::Graphics&);
    void paintBottom (juce::Graphics&);
    void paintChain (juce::Graphics&);
    void paintMainLights (juce::Graphics&);
    void paintStage (juce::Graphics&);
    void paintPunchChar (juce::Graphics&, bool punch);
    void paintLeveler (juce::Graphics&);
    void paintGold (juce::Graphics&);
    void paintClipper (juce::Graphics&);
    void paintLimiter (juce::Graphics&);
    void paintBands (juce::Graphics&);
    void paintLoudView (juce::Graphics&);
    void paintPreviewBanner (juce::Graphics&);
    juce::Path screenPath() const;

    // the controls (they also leave their hit zones)
    void knob (juce::Graphics&, float cx, float cy, float d, float pos, juce::Colour c, bool bip);
    void paramKnob (juce::Graphics&, const juce::String& pid, float cx, float y, float d, const juce::String& label, const juce::String& value,
                    juce::Colour c, bool bip, int style, const char* bipL = nullptr, const char* bipR = nullptr, float valSize = 0.0f);
    void cycleTab (juce::Graphics&, float cx, float y, const juce::String& label, float w, float sc, const juce::String& pid);
    void goldSlider (juce::Graphics&, float cx, float cy, float w);
    void goldLight (juce::Graphics&, float cx, float ty, float tw2, float th, float d, bool bold);

    // ---- the zones under the mouse, rebuilt with every paint
    struct Zone { wallui::R r; wallui::Hit kind; juce::String pid; int a = 0; };
    std::vector<Zone> zones;
    juce::AffineTransform zoneXf;   // (a panel drawn smaller: its zones are mapped back to the screen)
    void zone (wallui::R r, wallui::Hit k, const juce::String& pid = {}, int a = 0) { zones.push_back ({ r.transformedBy (zoneXf), k, pid, a }); }
    const Zone* zoneAt (juce::Point<float>) const;

    // ---- interaction
    juce::Point<float> mouse { -1, -1 }, downAt;
    Zone down {}; bool dragging = false, moved = false;
    float dragStart = 0.0f;
    int chainDrag = -1; float chainDragX = 0.0f;
    void click (const Zone&, const juce::MouseEvent&);
    void showParamMenu (const juce::String& pid, wallui::R near);
    void openTypeIn (const juce::String& pid, wallui::R near);
    void closeTypeIn (bool commit);
    std::unique_ptr<juce::TextEditor> typeIn; juce::String typeInPid;
    void showPresetMenu (wallui::R at);
    void savePresetAs();
    void showHistoryMenu (wallui::R at);
    void showCeilingMenu (wallui::R at);
    void showSizeMenu (wallui::R at);
    void showPreviewMenu (wallui::R at);
    void stepPreset (int dir);

    // ---- what the screen shows
    static constexpr int cols = 620;
    std::array<wall::Frame, cols> wave {}; int wavePos = 0;
    float shown[8] {};   // smoothed meters: in L/R, out L/R (dB)
    float pkHold[2] { -90, -90 }; double pkTime[2] {};
    float grShown = 0, grAvg = 0, grPeak = 0; double grPeakT = 0;
    float crest = 0, crestAvg = 0, levShown = 0, clipShown = 0, goldShown = 0, limShown = 0;
    float corrShown = 1, corrLo = 1, corrHi = 1;
    juce::Image bgCache; float bgScale = 0.0f;
    double lastPaintMs = 0.0; int skip = 0;
    juce::Image logo;
    std::unique_ptr<juce::FileChooser> chooser;
};

class WallEditor : public juce::AudioProcessorEditor
{
public:
    explicit WallEditor (WallProcessor&);
    ~WallEditor() override = default;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0d0d10)); }
    WallCanvas& getCanvas() { return canvas; }
private:
    WallProcessor& proc;
    WallCanvas canvas;
    juce::ComponentBoundsConstrainer limits;
    float scaleBeforeFull = 1.0f;
    static float globalScale (float set);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WallEditor)
};
