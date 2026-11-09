#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "WallDsp.h"
#include <map>

#ifndef WALL_VERSION
 #define WALL_VERSION 1
#endif

namespace wall
{
    // the six stages you can drag into any order (the limiter, ceiling and dither stay at the end)
    enum Stage { stPunch = 0, stCharacter, stBands, stLeveler, stGold, stClipper, numMovable };
    inline const char* stageName (int s) { static const char* n[] { "PUNCH", "CHARACTER", "BANDS", "LEVELER", "GOLD", "CLIPPER" }; return n[juce::jlimit (0, 5, s)]; }
    inline const char* stageOnId (int s) { static const char* n[] { "punchOn", "chOn", "bandsOn", "levOn", "goldOn", "clipOn" }; return n[juce::jlimit (0, 5, s)]; }
    // "order" is a number 0..719: the permutation of the six (0 = the order above)
    std::array<int, 6> decodeOrder (int code);
    int encodeOrder (const std::array<int, 6>&);
    inline juce::String bandId (int b, const char* what) { return "b" + juce::String (b) + what; }

    // one column of the scrolling wave (100 a second)
    struct Frame { float inPk = 0, outPk = 0, grDb = 0, levDb = 0; };

    struct Preset { const char* name; const char* tag; std::vector<std::pair<const char*, float>> values; };
    const std::vector<Preset>& factoryPresets();
}

class WallProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    WallProcessor();
    ~WallProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Mishkal Wall"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return (int) wall::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override { loadFactoryPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    float param (const juce::String& pid) const { auto* v = apvts.getRawParameterValue (pid); return v != nullptr ? v->load() : 0.0f; }
    void setParam (const juce::String& pid, float value);
    void beginGesture (const juce::String& pid);
    void dragParam (const juce::String& pid, float value);
    void endGesture (const juce::String& pid);
    juce::RangedAudioParameter* par (const juce::String& pid) const { return apvts.getParameter (pid); }

    // presets, A / B, undo
    void loadFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);
    static juce::File userPresetFolder();
    juce::Array<juce::File> userPresets() const;
    juce::String presetName = juce::String::fromUTF8 ("Init \xc2\xb7 Default");
    bool presetDirty = false;
    void switchAB();
    int abSlot = 0;
    bool canUndo() const { return historyPos > 0; }
    bool canRedo() const { return historyPos + 1 < (int) history.size(); }
    void undo(); void redo(); void jumpHistory (int pos);
    juce::StringArray historyNames() const { return historyLabel; }
    int historyIndex() const { return historyPos; }

    std::array<int, 6> order() const { return wall::decodeOrder ((int) param ("order")); }
    void setOrder (const std::array<int, 6>& o) { setParam ("order", (float) wall::encodeOrder (o)); }
    void setStyle (int style);          // the limiter style - also sets its attack, release and stages to the style's own
    void bandsSwitched (bool on);       // switching the bands on starts each band from the general upward setting

    // LEARN: listens, then sets the input so the output sits at the target
    // V2: MIDI Learn - right-click a control, "MIDI Learn", move a knob on your controller
    void midiLearn (const juce::String& pid);
    void midiForget (const juce::String& pid);
    juce::String midiCcFor (const juce::String& pid) const;   // "CC 21" or empty
    std::atomic<int> learnIndex { -1 };
    std::array<std::atomic<int>, 128> ccMap;   // CC number -> parameter index (-1 none)
    juce::AbstractFifo ccFifo { 512 }; std::array<std::pair<int, float>, 512> ccQueue {};
    bool sidechainConnected() const;
    void startLearn();
    std::atomic<bool> learning { false };
    void startAutoTrim();
    bool autoTrim = false;
    void copyAB();
    int uiSize = 2, uiView = 0;   // (the editor's size and view, saved with the session)
    // V2: the display's own settings (saved with the session): wave speed, wave range, meter scale, peak labels, loudness scale
    std::map<juce::String, juce::var> uiProps { { "dispSpeed", 0 }, { "dispRange", 24 }, { "meterScale", 0 }, { "peakLabels", true }, { "loudRange", 18 }, { "loudRel", false } };
    int ui (const juce::String& k) const { auto it = uiProps.find (k); return it == uiProps.end() ? 0 : (int) it->second; }
    void setUi (const juce::String& k, juce::var v) { uiProps[k] = v; }
    bool keyActive = false;   // (the sidechain key is arriving)
    void applyLoudnessStandard (int index);
    void resetLoudness();

    // ----- for the screen
    juce::AbstractFifo frameFifo { 2048 }; std::array<wall::Frame, 2048> frames;
    std::vector<float> lufsHistory;     // short-term loudness every 0.1 s (message thread), for the LOUDNESS view
    std::atomic<float> inPk { 0 }, inRms { 0 }, outPk { 0 }, outRms { 0 };   // linear
    std::atomic<float> inCh[2] { {0}, {0} }, outCh[2] { {0}, {0} };          // per side (peak, linear)
    std::atomic<float> grDb { 0 }, levDb { 0 }, clipDb { 0 }, goldHot { 0 }, limGrDb { 0 };
    std::atomic<float> lufsM { -70 }, lufsS { -70 }, lufsI { -70 }, lra { 0 }, tpMaxDb { -70 }, tpShortDb { -70 }, corr { 1 };
    std::atomic<int> ispCount { 0 };
    std::atomic<float> previewDb { 0 };   // how much quieter the streaming preview is playing (dB, <= 0)
    wall::Bands& bandsDsp() { return bands; }
    wall::Clipper& clipperDsp() { return clipper; }
    double getRate() const { return preparedRate; }
    int currentLatency() const { return liveLatency.load(); }   // (what the audio is really delayed by right now)
    std::atomic<int> liveLatency { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    int latencyFor (int osIndex, float lookMs) const;
    void updateLatency();

    double preparedRate = 48000.0; int maxBlock = 512, currentProgram = 0, reportedLatency = -1;
    bool loadingPreset = false;

    // DSP
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 5> os, osKey;   // 2x .. 32x (V2), and the same for the sidechain key
    int osWas = -1;
    wall::DcFilter dc; wall::MonoBass mono;
    wall::Punch punch; wall::Character character; wall::Bands bands; wall::Leveler leveler; wall::Gold gold; wall::Clipper clipper;
    wall::Limiter limiter; wall::Ceiling ceiling; wall::Dither dither;
    std::array<wall::Delay, 2> dryMix, dryBypass, dryDelta;
    juce::AudioBuffer<float> work;
    std::vector<float> grTrace, mixDry[2], dBypass[2], dDelta[2], tmpR, keyBuf[2];
    void dryBypassIn (int i, float a, float b);
    void dryDeltaIn (int i, float a, float b);
    float trimS = 1.0f, inS = 1.0f, mixS = 1.0f, bypS = 0.0f, unityDb = 0.0f, prevDb = 0.0f;
    float eIn = 0.0f, eOut = 0.0f;
    wall::KWeight kwIn[2], kwOut[2];
    wall::TruePeak tpOut;
    double blockE = 0.0; int blockN = 0, blockLen = 4800;
    juce::AbstractFifo lufsFifo { 4096 }; std::array<float, 4096> lufsBlocks {};
    std::vector<float> blockHist, gated, shortHist;   // (message thread)
    float tpShort = 0.0f, tpMax = 0.0f, cLR = 0, cLL = 0, cRR = 0; bool overWas = false;
    wall::Frame acc; int accN = 0, hop = 480;
    std::atomic<bool> resetMeters { false };
    // learn
    double learnStart = 0, autoStart = 0; int learnSteps = 0;

    // undo / A-B
    std::vector<float> snapshot() const;
    void applySnapshot (const std::vector<float>&);
    void recordHistory();
    std::vector<std::vector<float>> history; juce::StringArray historyLabel; std::vector<float> lastSeen;
    int historyPos = -1, stableTicks = 0; bool pendingChange = false; juce::String pendingLabel;
    juce::ValueTree slots[2];

    struct DirtyListener : juce::AudioProcessorValueTreeState::Listener
    {
        WallProcessor& p; explicit DirtyListener (WallProcessor& o) : p (o) {}
        void parameterChanged (const juce::String&, float) override { if (! p.loadingPreset) p.presetDirty = true; }
    } dirtyListener { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WallProcessor)
};
