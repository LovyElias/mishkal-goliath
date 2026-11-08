#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp/ResoPro.h"
#include "Dsp/VocalChain.h"
#include "TahorModel.h"

#ifndef TAHOR_VERSION
 #define TAHOR_VERSION 11
#endif
#define TAHOR_V1 (TAHOR_VERSION >= 1)   // the first Tahor: RESO > TAHOR > CHARACTER, exactly the Claude Design canvas
#define TAHOR_V2 (TAHOR_VERSION >= 2)   // input meter left / output right, Pro-Q-style analyzer, flat vintage units, nodes on
                                        // the Character curves, smaller see-through hardware, full screen, a livelier background
#define TAHOR_V6 (TAHOR_VERSION >= 6)   // Reso's controls slide from the left, ADV from the right, dropdowns in the band panels,
                                        // colour shadows on Character, Reso's shades, centred arches, grain, smoother analyzer
#define TAHOR_V11 (TAHOR_VERSION >= 11)  // range on screen, bubbles, Character starts empty, original layouts, presets, shortcuts
#define TAHOR_V10 (TAHOR_VERSION >= 10)  // no Spectral, Pro-Q-style dynamic curves, 2U gear, meter readouts, quiet analyzer
#define TAHOR_V9 (TAHOR_VERSION >= 9)   // L / R splits everywhere, dynamic / spectral views, solo column, five new units
#define TAHOR_V8 (TAHOR_VERSION >= 8)   // shorter header, preset arrows, Reso into the arch, worn panels, Solo buttons,
                                        // the gear gallery, separate L / R curves
#define TAHOR_V7 (TAHOR_VERSION >= 7)   // arches back, smaller header controls, Mishkal logo, matching side panels centred,
                                        // THD slider on Character, + Add EQ, arched band panels with a coloured glow
#define TAHOR_V5 (TAHOR_VERSION >= 5)   // small band panels along the bottom, a thin solid Reso strip, Depth at 9 o'clock,
                                        // the hardware always shown, colour modes on top, the arched screen, sharper drawing
#define TAHOR_V4 (TAHOR_VERSION >= 4)   // solid band panels that follow the band, small hidden hardware, chips down the left,
                                        // a slim bottom bar, a rounded screen that fades into the header and bottom bar
#define TAHOR_V3 (TAHOR_VERSION >= 3)   // full screen fits the screen, smaller band chips, controls fade in only when a dot is touched

namespace tahor
{
    inline constexpr int numBands = vox::numCleanBands;   // 8
    inline juce::String bid (int band, const char* what) { return "t" + juce::String (band) + what; }
    inline juce::String nid (int node, const char* what) { return "rn" + juce::String (node) + what; }
    inline juce::String uid (int unit, int v) { return "u" + juce::String (unit) + "v" + juce::String (v); }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; const char* tag = "Utility"; };
    const std::vector<Preset>& factoryPresets();

    // the colours of the five default band colours (cycled for bands 6..8)
    inline juce::Colour bandColour (int b)
    {
        static const juce::uint32 c[8] { 0xff46b7c9, 0xffef6c6c, 0xfff3a93c, 0xff7ad08a, 0xffa78bfa, 0xffff6fae, 0xff4f8cff, 0xffe6c46a };
        return juce::Colour (c[b & 7]);
    }

    // lock-free audio -> UI tap for the analyzer
    using AnalyzerTap = vox::AnalyzerTap;
}

class TahorProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    TahorProcessor();
    ~TahorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Mishkal Tahor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // presets, A / B, undo
    void loadFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);
    static juce::File userPresetFolder();
    juce::Array<juce::File> userPresets() const;
    juce::String presetName = juce::String::fromUTF8 ("Init \xc2\xb7 Flat");
    bool presetDirty = false;
    void switchAB (int slot);
    int abSlot = 0;
    bool canUndo() const { return historyPos > 0; }
    bool canRedo() const { return historyPos + 1 < (int) history.size(); }
    void undo();
    void redo();

    float param (const juce::String& pid) const { return apvts.getRawParameterValue (pid)->load(); }
    void setParam (const juce::String& pid, float value);      // a one-off gesture from the UI (plain value)
    void beginGesture (const juce::String& pid);
    void dragParam (const juce::String& pid, float value);     // between begin / end
    void endGesture (const juce::String& pid);

    // the chain of vintage units (the chips): unit indices, in order
    std::vector<int> chain() const;
    void setChain (const std::vector<int>&);

    // settings snapshots (also used by the UI to draw the curves)
    vox::CleanBands tahorBands() const;
    tahor::ResoProSettings resoSettings (int side = -1) const;   // V9: side 0 / 1 = only that side's bands (split)
    void getUnitValues (int unit, float* v, int side = 0) const;   // V9: side 1 = a split unit's right channel
    bool unitSplit (int unit) const { return param ("u" + juce::String (unit) + "split") > 0.5f; }

    juce::AudioProcessorValueTreeState apvts;
    double getRate() const { return preparedRate; }

    // for the screen
    tahor::AnalyzerTap preTap, postTap;
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, inMeterL { 0.0f }, inMeterR { 0.0f }, inRms { 0.0f }, outRms { 0.0f };   // V10: + RMS
    std::array<std::atomic<float>, vox::numCleanBands> dynDb {};
    tahor::ResoPro& activeReso() { const int li = param ("rsLowLat") > 0.5f ? 1 : 0; return resoIsSplit.load() ? resoSplit[(size_t) li][0] : (li ? resoLow : reso); }
    std::atomic<bool> resoIsSplit { false };
    std::atomic<float> autoGainDb { 0.0f };   // V9: the auto-gain's correction
    // V9: the undo history, with a name for each step
    juce::StringArray historyNames() const { return historyLabel; }
    int historyIndex() const { return historyPos; }
    void jumpHistory (int pos);
    static juce::String factoryTag (int index);
    juce::String presetTagOf (const juce::File&) const;
    bool saveUserPreset (const juce::String& name, const juce::String& tag);
    std::atomic<bool> autoLearning { false };
    std::atomic<float> charSoloHz { 0.0f }, charSoloQ { 1.0f };   // V8: a Character (V9: or Reso) node on solo (0 = none)
    bool hasSidechain() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    int latencyNow() const;
    void prepareOs (int maxBlock);

    double preparedRate = 48000.0;
    int maxBlock = 512, currentProgram = 0, reportedLatency = -1;
    bool loadingPreset = false;

    // DSP
    tahor::ResoPro reso, resoLow;
    std::array<std::array<tahor::ResoPro, 2>, 2> resoSplit;   // V9: [low latency][side] - left and right on their own
    vox::TahorStage tahorStage;
    vox::CleanupEQ lrEq;                                   // bands set to L or R (always Natural)
    std::array<vox::VocalEQ, tahor::maxChain> charEq, charEqR;   // V9: charEqR = a split unit's right channel
    std::array<int, tahor::maxChain> charWas { -1, -1, -1, -1 };
    std::array<vox::Saturator, tahor::maxChain> charSat;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 5> os;   // 2x .. 32x
    int osWas = -1;
    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, 2> soloF, soloC;   // solo band-pass (two stages); V8: Character's
    juce::AudioBuffer<float> dry, keyBuf;
    std::vector<float> dryDelay[2]; int dryPos = 0;
    float matchIn = 0.0f, matchOut = 0.0f, matchGain = 1.0f;
    double learnSum = 0.0; int learnCount = 0;
    float inSmooth = 1.0f, outSmooth = 1.0f;

    // undo / A-B
    std::vector<float> snapshot() const;
    void applySnapshot (const std::vector<float>&);
    void recordHistory();
    std::vector<std::vector<float>> history;
    juce::StringArray historyLabel;
    std::vector<float> lastSeen;
    int historyPos = -1, stableTicks = 0;
    juce::ValueTree slots[2];
    juce::String slotNames[2];

    struct DirtyListener : juce::AudioProcessorValueTreeState::Listener
    {
        TahorProcessor& p;
        explicit DirtyListener (TahorProcessor& o) : p (o) {}
        void parameterChanged (const juce::String&, float) override { if (! p.loadingPreset) p.presetDirty = true; }
    } dirtyListener { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TahorProcessor)
};
