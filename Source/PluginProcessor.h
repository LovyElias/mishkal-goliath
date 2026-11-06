#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "VocalChain.h"
#include "PresetManager.h"

namespace vox
{
    // Display names for the modes. Rename them here if you like.
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 74
    // V74: + API 550B-, SSL 9000K-, Helios 69- and Maag EQ4-style
    inline const juce::StringArray eqTypeNames   { "LE-EP1", "LE-E4K", "LE-N81", "LE-SP1", "LE-A550", "LE-K9", "LE-H69", "LE-M4" };
   #else
    inline const juce::StringArray eqTypeNames   { "LE-EP1", "LE-E4K", "LE-N81", "LE-SP1" };      // Pultec-, E-console-, Class A 81- and AirEQ-style
   #endif
    // V74 stepped frequencies (the hardware's switch positions)
    inline const juce::StringArray apiLowFreqs   { "30 Hz", "40 Hz", "50 Hz", "100 Hz", "200 Hz", "300 Hz", "400 Hz" };
    inline const juce::StringArray apiLmFreqs    { "75 Hz", "150 Hz", "180 Hz", "240 Hz", "500 Hz", "700 Hz", "1 kHz" };
    inline const juce::StringArray apiHmFreqs    { "800 Hz", "1.5 kHz", "3 kHz", "5 kHz", "8 kHz", "10 kHz", "12.5 kHz" };
    inline const juce::StringArray apiHighFreqs  { "2.5 kHz", "5 kHz", "7 kHz", "10 kHz", "12.5 kHz", "15 kHz", "20 kHz" };
    inline const juce::StringArray heliosBassFreqs { "50 Hz", "60 Hz", "100 Hz", "200 Hz" };
    inline const juce::StringArray heliosMidFreqs  { "700 Hz", "1 kHz", "1.5 kHz", "2.2 kHz", "3.5 kHz", "4.7 kHz", "6 kHz" };
    inline const juce::StringArray maagAirFreqs    { "2.5 kHz", "5 kHz", "10 kHz", "20 kHz", "40 kHz" };
    // V55: gentler starting points - Reso depth 15%, Perach amount 2 with 80% mix (defaults and every preset)
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 55
    #define MISHKAL_DEF_RESO_DEPTH 0.15f
    #define MISHKAL_DEF_PER_AMOUNT 2.0f
    #define MISHKAL_DEF_PER_MIX    0.8f
   #else
    #define MISHKAL_DEF_RESO_DEPTH 0.4f
    #define MISHKAL_DEF_PER_AMOUNT 5.0f
    #define MISHKAL_DEF_PER_MIX    1.0f
   #endif
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 75
    // V75: + API 2500-, SSL G bus-, Weiss DS1-, SPL Iron- and Neve 33609-style
    inline const juce::StringArray compTypeNames { "LE-76", "LE-RC", "LE-660", "LE-STA", "LE-2500", "LE-G", "LE-DS1", "LE-IRON", "LE-336" };
   #else
    inline const juce::StringArray compTypeNames { "LE-76", "LE-RC", "LE-660", "LE-STA" };   // LE-STA: its own unit from V52
   #endif
    inline const juce::StringArray compTypeNames3 { "LE-76", "LE-RC", "LE-660" };                  // FET (1176-style), Renaissance-style compressor, vari-mu (Fairchild-style)
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 71
    inline const juce::StringArray limModeNames  { "Goliath Classic", "Goliath Multi" };   // V71 names
   #else
    inline const juce::StringArray limModeNames  { "Classic", "Multi" };
   #endif
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 72
    inline const juce::StringArray limMaxStyleNames { "Smart", "Transient", "Loud - Pumping", "Loud - Balanced", "Loud - Crisp", "Loud - Clip", "Spectral", "Bands", "Safe" };   // V72: + Safe
   #else
    inline const juce::StringArray limMaxStyleNames { "Smart", "Transient", "Loud - Pumping", "Loud - Balanced", "Loud - Crisp", "Loud - Clip", "Spectral", "Bands" };
   #endif
    inline const juce::StringArray dsModeNames   { "Split", "Wide" };
    inline const juce::StringArray tapeStyleNames { "30 ips", "15 ips", "7.5 ips" };   // tape speed
    inline const juce::StringArray tapeWidthNames { "1/4\"", "1/2\"", "1\"", "2\"" };   // tape width
    inline juce::StringArray flowStyleNames()
    {
        juce::StringArray n;
        for (int i = 0; i < numFlowStyles; ++i) n.add (flowStyleInfo (i).name);
        return n;
    }
    inline const juce::StringArray bandTypeNames { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch" };
    inline const juce::StringArray slopeNames    { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };

    // Clean-up EQ band parameter IDs, e.g. bandId (0, "Freq") -> "b1Freq"
    inline juce::String bandId (int band, const char* field) { return "b" + juce::String (band + 1) + field; }

    // Names for every processing order, e.g. "Tube P1 > E-Console > Class A 81 > Airy"
    template <size_t N>
    inline juce::StringArray orderNames (const juce::StringArray& unitNames)
    {
        juce::StringArray out;
        int count = 1;
        for (size_t i = 2; i <= N; ++i) count *= (int) i;
        for (int idx = 0; idx < count; ++idx)
        {
            juce::StringArray parts;
            for (int u : permutationFromIndex<N> (idx)) parts.add (unitNames[u]);
            out.add (parts.joinIntoString (" > "));
        }
        return out;
    }

    // Compressor unit parameter IDs, e.g. compId (2, "Thresh") -> "vmuThresh"
    // FET Purple: On Thresh Ratio Attack Release Makeup Mix
    // LE-RC: On Thresh Ratio Attack Release Makeup Mix Arc Behavior Character    Vari-Mu 660: On Thresh Tc Makeup Mix
    inline juce::String compId (int unit, const char* field)
    {
        static const char* prefix[9] { "fet", "vox", "vmu", "sta", "a25", "sslg", "ds1", "iron", "n33" };   // sta = LE-STA (V52); V75: + five
        return juce::String (prefix[juce::jlimit (0, 8, unit)]) + field;
    }

    // Character EQ parameter IDs, one set per EQ type, e.g. charEqId (3, "PresGain") -> "airyPresGain"
    // Fields: On HpfFreq BodyFreq BodyGain MudFreq MudGain MudQ PresFreq PresGain PresQ AirFreq AirGain (+ Silk on Airy)
    // Tube P1 (type 0) instead has: On LowFreq LowBoost LowAtten HighFreq HighBw HighBoost AttenFreq HighAtten
    inline const juce::StringArray p1LowFreqs   { "20 Hz", "30 Hz", "60 Hz", "100 Hz" };
    inline const juce::StringArray p1HighFreqs  { "3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz" };
    inline const juce::StringArray p1AttenFreqs { "5 kHz", "10 kHz", "20 kHz" };
    inline float p1Hz (const juce::StringArray& list, int index)
    {
        const auto t = list[juce::jlimit (0, list.size() - 1, index)];
        return t.getFloatValue() * (t.containsIgnoreCase ("khz") ? 1000.0f : 1.0f);
    }
    inline juce::String charEqId (int type, const char* field)
    {
        static const char* prefix[8] { "p1a", "ecn", "a81", "airy", "api", "k9", "h69", "m4" };
        return juce::String (prefix[juce::jlimit (0, 7, type)]) + field;
    }

    namespace id
    {
        #define VOX_ID(name) inline constexpr const char* name = #name;
        VOX_ID (masterBypass) VOX_ID (levelMatch) VOX_ID (peakCatch) VOX_ID (trackMode) VOX_ID (peakIn) VOX_ID (peakOut) VOX_ID (peakLink)
        VOX_ID (riderRange) VOX_ID (breathAmount) VOX_ID (compScHpf) VOX_ID (limSoft) VOX_ID (tapeOs) VOX_ID (limOs) VOX_ID (globalOs) VOX_ID (cleanPhase) VOX_ID (cleanOs) VOX_ID (inputGain)  VOX_ID (outputGain)  VOX_ID (cleanOn)
        VOX_ID (resoOn) VOX_ID (resoDepth) VOX_ID (resoSharp) VOX_ID (resoSelect) VOX_ID (resoAttack)
        VOX_ID (resoRelease) VOX_ID (resoLow) VOX_ID (resoHigh) VOX_ID (resoMix) VOX_ID (resoDelta) VOX_ID (resoMs) VOX_ID (resoSideDepth) VOX_ID (tapeMsDrive) VOX_ID (resoLink) VOX_ID (limLink) VOX_ID (limTp) VOX_ID (limClip) VOX_ID (outWidth) VOX_ID (compScExt) VOX_ID (dsScExt) VOX_ID (compDelta) VOX_ID (limDelta) VOX_ID (midOrder) VOX_ID (limMax) VOX_ID (limMaxStyle) VOX_ID (limCharacter) VOX_ID (limTransients) VOX_ID (limUpward)
        VOX_ID (eqOn) VOX_ID (limMix)
        VOX_ID (gateThresh)
        VOX_ID (compOn) VOX_ID (eqOrder) VOX_ID (eqOrder8) VOX_ID (compOrder) VOX_ID (compOrder4) VOX_ID (compOrder9) VOX_ID (eqAfterComp)
        VOX_ID (dsOn) VOX_ID (dsFreq) VOX_ID (dsThresh) VOX_ID (dsRange) VOX_ID (dsListen) VOX_ID (dsMode) VOX_ID (dsAuto)
        VOX_ID (flowOn) VOX_ID (flowStyle) VOX_ID (flowAmount) VOX_ID (flowMix) VOX_ID (flowModel) VOX_ID (perAmount) VOX_ID (perLow) VOX_ID (perBody) VOX_ID (perPresence) VOX_ID (perAir) VOX_ID (perSpeed) VOX_ID (perMix)
        VOX_ID (tapeOn) VOX_ID (tapeStyle) VOX_ID (tapeWidth) VOX_ID (tapeDrive) VOX_ID (tapeMix) VOX_ID (tapeNoise) VOX_ID (tapeModel) VOX_ID (eshChar) VOX_ID (eshProcess) VOX_ID (eshTone) VOX_ID (eshIn) VOX_ID (eshOut)
        VOX_ID (limOn) VOX_ID (limMode) VOX_ID (limThresh) VOX_ID (limCeiling) VOX_ID (limRelease) VOX_ID (limArc)
        #undef VOX_ID
    }
}

class VoxStripProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    VoxStripProcessor();
    ~VoxStripProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override { chain.reset(); dryDelay.clear(); }
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (vox::id::masterBypass); }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Reads the current parameter values (safe from any thread)
    vox::ChainSettings readSettings() const;

    juce::UndoManager undoManager;            // must be declared before apvts
    juce::AudioProcessorValueTreeState apvts;
    vox::PresetManager presets { apvts };
    vox::Meters& meters() { return chain.meters; }
    vox::LoudnessMeter loudness;               // LUFS of the final output (short-term + integrated)
    vox::VocalChain& getChain() { return chain; }
    // V60: a mono track (one channel in and out) has no mid/side, so the editor hides those controls
    bool isMonoTrack() const { return getMainBusNumInputChannels() == 1 || getMainBusNumOutputChannels() == 1; }
    // the rate the chain (and its analyzer taps) runs at: the host rate x the whole-plugin oversampling factor
    double getCurrentSampleRate() const { const double r = chainRate.load(); return r > 0.0 ? r : (getSampleRate() > 0 ? getSampleRate() : 48000.0); }
    int getActiveOversampling() const { return activeOs.load(); }   // 0 off, 1 = 2x, 2 = 4x (whole plugin)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct FloatBinding { std::atomic<float>* src; float vox::ChainSettings::* dst; };
    struct BoolBinding  { std::atomic<float>* src; bool  vox::ChainSettings::* dst; };
    std::vector<FloatBinding> floatBindings;
    std::vector<BoolBinding>  boolBindings;
    struct CharEqParams { std::atomic<float> *on, *hpf, *bodyF, *bodyG, *mudF, *mudG, *mudQ, *presF, *presG, *presQ, *airF, *airG, *silk; };
    std::array<std::atomic<float>*, 8> charEqPlaceParams {};   // V71 (V74: eight)
    std::array<std::vector<std::atomic<float>*>, 4> v74EqParams;   // V74: LE-A550, LE-K9, LE-H69, LE-M4, in v74EqFields order
    std::atomic<float>* eqOrder8Param = nullptr;
    std::array<std::vector<std::atomic<float>*>, 5> v75CompParams;   // V75: the five new compressors, in v75CompFields order
    std::atomic<float>* compOrder9Param = nullptr;
    static constexpr const char* const v75CompFields[5][12] {
        { "On", "Thresh", "Ratio", "Attack", "Release", "Makeup", "Mix", "Knee", "Thrust", "Type" },
        { "On", "Thresh", "Ratio", "Attack", "Release", "Makeup", "Mix" },
        { "On", "Thresh", "Ratio", "Attack", "Release", "Makeup", "Mix", "Knee", "Detect", "RelMode" },
        { "On", "Thresh", "Attack", "Release", "Makeup", "Mix", "Bias", "Rect", "Airbass" },
        { "On", "Thresh", "Ratio", "Release", "LimThresh", "LimRel", "LimAtt", "Makeup", "Mix" } };
    static constexpr const char* const v74EqFields[4][16] {
        { "On", "Filter", "LowFreq", "LowGain", "LowShape", "LmFreq", "LmGain", "HmFreq", "HmGain", "HighFreq", "HighGain", "HighShape" },
        { "On", "HpfFreq", "LpfFreq", "LfFreq", "LfGain", "LfBell", "LmfFreq", "LmfGain", "LmfQ", "HmfFreq", "HmfGain", "HmfQ", "HfFreq", "HfGain", "HfBell", "Mode" },
        { "On", "BassFreq", "BassGain", "MidFreq", "MidGain", "TrebleGain" },
        { "On", "SubGain", "G40", "G160", "G650", "G2k5", "AirFreq", "AirGain" } };
    std::array<CharEqParams, 4> charEqParams {};   // [0] unused: Tube P1 has its own controls
    struct TubeP1Params { std::atomic<float> *on, *lowFreq, *lowBoost, *lowAtten, *highFreq, *highBw, *highBoost, *attenFreq, *highAtten; } p1Params {};
    std::atomic<float> *eqOrderParam = nullptr, *compOrderParam = nullptr, *compOrder4Param = nullptr, *eqAfterCompParam = nullptr;
    struct CompParams { std::atomic<float> *on, *thresh, *ratio, *attack, *release, *makeup, *mix, *tc, *arc, *behavior, *character, *rev, *model, *voxLink; };
    std::array<std::atomic<float>*, 3> staParams {};    // LE-STA in the tube slot (V49-V51): Input, Mode, Recovery
    std::array<std::atomic<float>*, 3> staParams4 {};   // LE-STA as its own unit (V52)
    std::array<CompParams, 4> compParams {};
    std::atomic<float>* limModeParam = nullptr;
    std::atomic<float>* dsModeParam = nullptr;
    std::atomic<float>* flowStyleParam = nullptr;
    std::atomic<float>* tapeStyleParam = nullptr;
    std::atomic<float>* tapeWidthParam = nullptr;
    std::atomic<float>* compScHpfParam = nullptr;
    std::atomic<float>* limSoftParam = nullptr;
    std::atomic<float>* tapeOsParam = nullptr;
    std::atomic<float>* limOsParam = nullptr;
    std::atomic<float>* cleanPhaseParam = nullptr;
    std::atomic<float>* cleanOsParam = nullptr;

    struct ResoParams { std::atomic<float> *on, *depth, *sharp, *select, *attack, *release, *low, *high, *mix, *delta, *ms, *sideDepth; } resoParams {};
    std::atomic<float>* tapeMsDriveParam = nullptr;
    std::atomic<float>* resoLinkParam = nullptr;
    std::atomic<float>* limLinkParam = nullptr;
    std::atomic<float>* limTpParam = nullptr;
    std::atomic<float>* limClipParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* compScExtParam = nullptr;
    std::atomic<float>* dsScExtParam = nullptr;
    std::atomic<float>* compDeltaParam = nullptr;
    std::atomic<float>* limDeltaParam = nullptr;
    std::atomic<float>* midOrderParam = nullptr;
    std::atomic<float>* limUpwardParam {};    // V70
    std::atomic<float>* limMaxParams[4] {};   // V69: on, style, character, transients
    juce::AudioBuffer<float> keyUp, keyUpView;   // V64: the sidechain at the oversampled rate
public:
    bool hasSidechain() const { return getBusCount (true) > 1 && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled(); }
private:
    struct BandParams { std::atomic<float> *on, *type, *freq, *gain, *q, *slope, *dyn, *thresh, *range, *place; };
    std::array<BandParams, vox::numCleanBands> bandParams {};

    vox::VocalChain chain;

    // Master bypass: the dry input, delayed by the plugin's latency so bypassing never shifts timing,
    // crossfaded over 30 ms so switching never clicks
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* trackModeParam = nullptr;
    std::atomic<int> reportedLatency { -1 };   // the latency the DAW was last told about (samples)
    void handleAsyncUpdate() override;            // tells the DAW the new latency (message thread)

    // Whole-plugin oversampling: the entire chain runs at 2x or 4x. Switching re-prepares the chain on the message
    // thread (processing is briefly suspended), so the audio thread never allocates.
    std::atomic<float>* globalOsParam = nullptr;
    std::atomic<float>* flowModelParam = nullptr;
    std::array<std::atomic<float>*, 6> eshParams {};   // Esh: model, character, process, tone, in, out
    std::array<std::atomic<float>*, 7> perParams {};   // Perach: amount, low, body, presence, air, speed, mix
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
    std::atomic<int> activeOs { 0 }, pendingOs { -1 };
    std::atomic<double> chainRate { 0.0 };
    double baseRate = 48000.0;
    int baseBlock = 512;
    static constexpr int osChunk = 2048;            // the oversampled path runs the host block in chunks of this size
    int wantedOs (const vox::ChainSettings& s) const;
    void prepareChain (int os);                     // message thread / prepareToPlay only
    int totalLatency (const vox::ChainSettings& s) const;
    juce::AudioBuffer<float> dryDelay;        // circular delay line, one per channel
    int dryDelayPos = 0;
    juce::SmoothedValue<float> bypassMix;     // 0 = plugin, 1 = bypassed
    juce::AudioBuffer<float> dryBlock;        // this block's delayed dry signal

    // Level match: loudness of the processed vs the raw vocal (K-weighted, ~3 s average);
    // with MATCH on, the bypassed signal is brought to the processed loudness for a fair comparison
    std::atomic<float>* levelMatchParam = nullptr;
    std::array<juce::dsp::IIR::Filter<float>, 2> kShelf, kHighPass;   // [0] dry, [1] processed
    double msDry = 0.0, msWet = 0.0;
    juce::SmoothedValue<float> matchGain { 1.0f };
public:
    std::atomic<float> levelMatchDb { 0.0f };  // processed minus raw loudness, for the MATCH button
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxStripProcessor)
};
