#include "TahorProcessor.h"
#include "TahorEditor.h"

using namespace tahor;

//==============================================================================
// Factory presets: each lists only what it changes from the defaults (the defaults are "Vocal · Air & Body")
const std::vector<Preset>& tahor::factoryPresets()
{
    // V11: the factory library - 69 presets in ten folders (the browser's tags)
    // Pressure V1: CLEAN starting points (each lists only what it changes from the defaults)
    static const std::vector<Preset> list {
        { "Init \xc2\xb7 Default", {  }, "Utility" },
        { "Vocal \xc2\xb7 Smooth Leveler", { { "cEngine", 0.0f }, { "cThresh", -22.0f }, { "cRatio", 3.0f }, { "cKnee", 8.0f }, { "cAttack", 6.0f }, { "cRelease", 140.0f }, { "cAutoRel", 1.0f }, { "cMakeup", 4.0f } }, "Vocals" },
        { "Vocal \xc2\xb7 One-Knob Vox", { { "cEngine", 1.0f }, { "cVoxComp", -18.0f }, { "cVoxGate", -50.0f } }, "Vocals" },
        { "Vocal \xc2\xb7 Upfront Pop", { { "cEngine", 4.0f }, { "cProStyle", 3.0f }, { "cThresh", -26.0f }, { "cRatio", 4.0f }, { "cAttack", 3.0f }, { "cRelease", 90.0f }, { "cMakeup", 5.0f } }, "Vocals" },
        { "Mix Bus \xc2\xb7 Glue", { { "cEngine", 4.0f }, { "cProStyle", 5.0f }, { "cThresh", -18.0f }, { "cRatio", 2.0f }, { "cAttack", 30.0f }, { "cRelease", 300.0f }, { "cKnee", 6.0f }, { "cMakeup", 2.0f } }, "Mix Bus" },
        { "Mastering \xc2\xb7 Transparent DS1", { { "cEngine", 2.0f }, { "cThresh", -14.0f }, { "cRatio", 2.0f }, { "cDsKnee", 0.6f }, { "cAttack", 3.0f }, { "cMakeup", 1.5f } }, "Mastering" },
        { "Mastering \xc2\xb7 Gentle MLA", { { "cEngine", 3.0f }, { "cMlaIn", 6.0f }, { "cMlaRatio", 2.0f }, { "cMakeup", 1.5f } }, "Mastering" },
        { "Drums \xc2\xb7 Punch", { { "cEngine", 4.0f }, { "cProStyle", 6.0f }, { "cThresh", -20.0f }, { "cRatio", 4.0f }, { "cAttack", 25.0f }, { "cRelease", 80.0f }, { "cMakeup", 4.0f } }, "Drums" },
    };
    return list;
}

//==============================================================================
TahorProcessor::TahorProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "TAHOR", createLayout())
{
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p)) apvts.addParameterListener (wp->paramID, &dirtyListener);
    startTimerHz (20);
}

TahorProcessor::~TahorProcessor()
{
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p)) apvts.removeParameterListener (wp->paramID, &dirtyListener);
}

juce::AudioProcessorValueTreeState::ParameterLayout TahorProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;
    using R = juce::NormalisableRange<float>;
    auto F = [&L] (const juce::String& id, const juce::String& name, R r, float def, const juce::String& unit = {})
    { L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel (unit))); };
    auto B = [&L] (const juce::String& id, const juce::String& name, bool def)
    { L.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    auto C = [&L] (const juce::String& id, const juce::String& name, juce::StringArray items, int def)
    { L.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def)); };
    auto hz = [] (float lo, float hi) { R r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    // global
    B ("bypass", "Bypass", false);
    B ("match", "Match", false);
    C ("phase", "Processing", { "Natural Phase", "Linear Phase", "Linear Phase HQ" }, 0);
    C ("os", "Oversampling", { "Off", "2x", "4x", "8x", "16x", "32x" }, 1);
    C ("osRender", "Render Oversampling", { "Same", "2x", "4x", "8x", "16x", "32x" }, 5);
    F ("inGain", "Input", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    F ("outGain", "Output", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    B ("scExt", "Sidechain External", true);
    C ("analyzer", "Analyzer", { "Pre + Post", "Post", "Pre", "Off" }, 0);
    C ("range", "Range", { juce::String::fromUTF8 ("\xc2\xb1" "3 dB"), juce::String::fromUTF8 ("\xc2\xb1" "6 dB"),
                           juce::String::fromUTF8 ("\xc2\xb1" "12 dB"), juce::String::fromUTF8 ("\xc2\xb1" "30 dB") }, 2);
    B ("resoOn", "Helpers On", true);
    B ("tahorOn", "Clean On", true);
    B ("charOn", "Vintage On", true);

    // ===== PRESSURE =====
    // (the three tabs' power buttons keep their old ids: resoOn = HELPERS, tahorOn = CLEAN, charOn = VINTAGE)
    // HELPERS (LE-4)
    B ("hLiftOn", "Lift On", true);  B ("hRideOn", "Rider On", true);  B ("hBreathOn", "Breath On", true);  B ("hPeakOn", "1964 On", true);
    C ("hLiftMode", "Lift Mode", { "Upward", "Dyn EQ" }, 0);
    F ("hLowHz", "Lift Low Freq", hz (40.0f, 500.0f), 120.0f, "Hz");
    F ("hLowThr", "Lift Low Thresh", R (-60.0f, 0.0f, 0.1f), -30.0f, "dB");
    F ("hLowLift", "Lift Low Amount", R (0.0f, 12.0f, 0.1f), 4.0f, "dB");
    F ("hHighHz", "Lift High Freq", hz (2000.0f, 16000.0f), 6000.0f, "Hz");
    F ("hHighThr", "Lift High Thresh", R (-60.0f, 0.0f, 0.1f), -34.0f, "dB");
    F ("hHighLift", "Lift High Amount", R (0.0f, 12.0f, 0.1f), 3.0f, "dB");
    F ("hRideRange", "Rider Range", R (0.0f, 12.0f, 0.1f), 6.0f, "dB");
    F ("hRideTarget", "Rider Target", R (-30.0f, -6.0f, 0.1f), -16.0f, "LUFS");
    C ("hRideSpeed", "Rider Speed", { "Slow", "Med", "Fast" }, 1);
    F ("hBreath", "Breath Amount", R (-20.0f, 0.0f, 0.1f), -8.0f, "dB");
    F ("hBreathSense", "Breath Sense", R (0.0f, 100.0f, 1.0f), 55.0f, "%");
    { R r (5.0f, 200.0f, 1.0f); r.setSkewForCentre (40.0f); F ("hBreathFade", "Breath Fade", r, 40.0f, "ms"); }
    F ("hPkIn", "1964 Input", R (-40.0f, 10.0f, 0.5f), -40.0f, "dBu");
    F ("hPkOut", "1964 Output", R (-10.0f, 10.0f, 0.1f), 4.0f, "dBu");
    { R r (1.1f, 100.0f, 0.01f); r.setSkewForCentre (4.0f); F ("hPkSlope", "1964 Slope", r, 2.0f, ":1"); }
    F ("hPkRel", "1964 Release", R (0.0f, 10.0f, 0.1f), 3.0f);
    C ("hPkMeter", "1964 Meter", { "GR", "+4", "+8" }, 0);
    C ("hPkPad", "1964 Pad", { "Off", "10", "20" }, 0);
    // CLEAN
    C ("cEngine", "Clean Engine", { "LE-11/25E", "LE-755C", "LE-DS1", "LE-MLA", "LE-580E" }, 0);
    F ("cThresh", "Threshold", R (-60.0f, 0.0f, 0.1f), -24.0f, "dB");
    { R r (1.0f, 20.0f, 0.01f); r.setSkewForCentre (4.0f); F ("cRatio", "Ratio", r, 4.0f, ":1"); }
    F ("cKnee", "Knee", R (0.0f, 24.0f, 0.1f), 6.0f, "dB");
    F ("cDsKnee", "DS1 Soft Knee", R (0.0f, 1.0f, 0.01f), 0.6f);
    { R r (0.05f, 250.0f, 0.01f); r.setSkewForCentre (8.0f); F ("cAttack", "Attack", r, 8.0f, "ms"); }
    { R r (5.0f, 3000.0f, 1.0f); r.setSkewForCentre (150.0f); F ("cRelease", "Release", r, 120.0f, "ms"); }
    B ("cAutoRel", "Auto Release", false);
    B ("cAutoAtk", "Auto Attack", false);
    C ("cGlide", "Glide / Snap", { "Glide", "Snap" }, 0);
    C ("cColor", "Color / Pure", { "Color", "Pure" }, 0);
    C ("cPeak", "Peak / RMS", { "Peak", "RMS" }, 0);
    B ("cDelta", "Delta", false);
    { R r (10.0f, 1000.0f, 1.0f); r.setSkewForCentre (200.0f); F ("cDsRelF", "DS1 Release Fast", r, 200.0f, "ms"); }
    { R r (100.0f, 5000.0f, 1.0f); r.setSkewForCentre (1200.0f); F ("cDsRelS", "DS1 Release Slow", r, 1200.0f, "ms"); }
    { R r (1.0f, 300.0f, 0.1f); r.setSkewForCentre (30.0f); F ("cDsAvg", "DS1 Average", r, 30.0f, "ms"); }
    C ("cDsBand", "DS1 Full / Band", { "Full Range", "Band" }, 0);
    F ("cMakeup", "Make-up", R (-12.0f, 24.0f, 0.1f), 4.0f, "dB");
    F ("cMix", "Mix", R (0.0f, 100.0f, 1.0f), 100.0f, "%");
    B ("cAutoGain", "Clean Auto Gain", false);
    F ("cRange", "Range", R (0.0f, 60.0f, 0.5f), 24.0f, "dB");
    F ("cHold", "Hold", R (0.0f, 500.0f, 1.0f), 0.0f, "ms");
    F ("cVoxComp", "755C Compress", R (-40.0f, 0.0f, 0.1f), 0.0f, "dB");
    F ("cVoxGate", "755C Gate", R (-80.0f, -20.0f, 0.5f), -80.0f, "dB");
    F ("cVoxGain", "755C Gain", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    B ("cVoxMatch", "755C Level Match", false);
    F ("cMlaIn", "MLA Input Gain", R (0.0f, 20.0f, 1.0f), 8.0f, "dB");
    C ("cMlaRatio", "MLA Ratio", { "1.2", "1.4", "1.7", "2", "2.5", "3", "5", "8", "INF" }, 3);
    C ("cMlaAtk", "MLA Attack", { "0.005", "0.01", "0.015", "0.02", "0.03", "0.05", "0.1", "0.2", "0.5" }, 3);
    C ("cMlaRel", "MLA Release", { "0.02", "0.03", "0.05", "0.07", "0.10", "0.15", "0.2", "0.3", "0.5" }, 4);
    B ("cMlaLink", "MLA ImageLink", true);
    C ("cMlaMeter", "MLA Meter", { "GR", "VU" }, 0);
    { juce::StringArray st; for (auto* n : pressure::proStyles) st.add (n); C ("cProStyle", "580E Style", st, 0); }
    C ("cProChar", "580E Character", { "Tube", "Diode", "Bright" }, 0);
    B ("cProCharOn", "580E Drive On", true);
    F ("cProDrive", "580E Drive", R (0.0f, 100.0f, 1.0f), 30.0f, "%");
    C ("cProPre", "580E Pre / Post", { "Pre", "Post" }, 0);
    B ("cProAutoThr", "580E Auto Threshold", false);
    F ("cProDry", "580E Dry", R (-60.0f, 0.0f, 0.1f), -60.0f, "dB");
    F ("cProWet", "580E Wet", R (-24.0f, 12.0f, 0.1f), 0.0f, "dB");
    // the sidechain
    C ("cScSrc", "Sidechain Source", { "Internal", "External" }, 0);
    B ("cScListen", "Sidechain Listen", false);
    F ("cScGain", "Sidechain Gain", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    F ("cScLink", "Stereo Link", R (0.0f, 100.0f, 1.0f), 100.0f, "%");
    C ("cScMs", "Sidechain M / S", { "Stereo", "Mid", "Side" }, 0);
    F ("cLook", "Look-ahead", R (0.0f, 10.0f, 0.1f), 2.0f, "ms");
    F ("cScHp", "Sidechain High Pass", hz (20.0f, 500.0f), 80.0f, "Hz");
    F ("cScBellHz", "Sidechain Bell Freq", hz (200.0f, 12000.0f), 3000.0f, "Hz");
    F ("cScBellDb", "Sidechain Bell Gain", R (-12.0f, 12.0f, 0.1f), 3.0f, "dB");
    F ("cScLp", "Sidechain Low Pass", hz (2000.0f, 20000.0f), 14000.0f, "Hz");
    B ("autoGain", "Auto Gain", false);   // V9
    return L;
}

//==============================================================================
vox::CleanBands TahorProcessor::tahorBands() const
{
    vox::CleanBands bands;
    for (int b = 0; b < numBands; ++b)
    {
        auto& c = bands[(size_t) b];
        c.on = param (bid (b, "On")) > 0.5f && param (bid (b, "Byp")) < 0.5f;   // V12: a disabled band keeps its settings
        c.type = (vox::BandType) (int) param (bid (b, "Type"));
        c.freq = param (bid (b, "Hz")); c.gain = param (bid (b, "Gain")); c.q = param (bid (b, "Q"));
        c.slope = (int) param (bid (b, "Slope"));
        c.place = (int) param (bid (b, "Place"));
        c.dyn = param (bid (b, "Dyn")) > 0.5f || param (bid (b, "Spec")) > 0.5f;   // V10: an old Spectral band plays as Dynamic
        c.dynThresh = param (bid (b, "Thr")); c.dynRange = param (bid (b, "Rng"));
        c.spectral = false;   // V10: Spectral is gone from Tahor (Reso does that job)
        c.sc = param (bid (b, "Sc")) > 0.5f;
        c.dynAttMs = param (bid (b, "Att")); c.dynRelMs = param (bid (b, "Rel")); c.specSel = param (bid (b, "Sel"));   // V9
    }
    return bands;
}

ResoProSettings TahorProcessor::resoSettings (int side) const
{
    ResoProSettings s;
    s.on = param ("resoOn") > 0.5f;
    s.depth = param ("rsDepth"); s.detail = param ("rsDetail") / 100.0f;
    s.attackMs = param ("rsAttack"); s.releaseMs = param ("rsRelease");
    s.maxCutDb = param ("rsMaxCut"); s.mix = param ("rsMix") / 100.0f; s.trimDb = param ("rsTrim");
    s.hard = param ("rsMode") > 0.5f; s.msMode = (int) param ("rsMs");
    s.delta = param ("rsDelta") > 0.5f; s.sidechain = param ("rsKey") > 0.5f;
    s.link = param ("rsLink") / 100.0f;
    s.detailTilt = param ("rsDTilt") / 100.0f; s.attackTilt = param ("rsATilt") / 100.0f; s.releaseTilt = param ("rsRTilt") / 100.0f;
    s.minPhase = param ("rsPhase") < 0.5f;
    s.lowHz = param ("rsLow"); s.highHz = param ("rsHigh");
    for (int i = 0; i < maxResoNodes; ++i)
    {
        auto& n = s.nodes[(size_t) i];
        n.on = param (nid (i, "On")) > 0.5f && param (nid (i, "Byp")) < 0.5f; n.shape = (int) param (nid (i, "Shape"));   // V12: + disabled
        n.hz = param (nid (i, "Hz")); n.amount = param (nid (i, "Amt")); n.q = param (nid (i, "Q"));
        const int pl = (int) param (nid (i, "Place"));   // V9: a split node only works on its own side
        if (side >= 0 && pl != 0 && pl != side + 1) n.on = false;
    }
    return s;
}

void TahorProcessor::getUnitValues (int unit, float* v, int side, bool applyOff) const
{
    for (int j = 0; j < unitValues; ++j) v[j] = param (side == 1 ? "u" + juce::String (unit) + "r" + juce::String (j) : uid (unit, j));
    // V12: a disabled node sounds as if its knob were at rest (its own setting is kept)
    const int mask = applyOff ? charOff[(size_t) juce::jlimit (0, numUnits - 1, unit)].load() : 0;
    if (mask != 0)
    {
        const auto& nodes = unitNodes (unit);
        for (int i = 0; i < (int) nodes.size(); ++i)
            if (mask & (1 << i))
            {
                const auto& nd = nodes[(size_t) i];
                if (nd.gv >= 0) v[nd.gv] = (nd.gmode == 0 || nd.gmode == 3) ? 0.5f : 0.0f;
                else if (nd.fv >= 0) v[nd.fv] = 0.0f;
            }
    }
}

std::vector<int> TahorProcessor::chain() const
{
    std::vector<int> c;
    for (int s = 0; s < maxChain; ++s) { const int u = (int) param ("chain" + juce::String (s)) - 1; if (u >= 0) c.push_back (u); }
    return c;
}

void TahorProcessor::setChain (const std::vector<int>& c)
{
    for (int s = 0; s < maxChain; ++s) setParam ("chain" + juce::String (s), s < (int) c.size() ? (float) (c[(size_t) s] + 1) : 0.0f);
}

bool TahorProcessor::hasSidechain() const
{
    if (auto* bus = getBus (true, 1)) return bus->isEnabled() && bus->getNumberOfChannels() > 0;
    return false;
}

//==============================================================================
void TahorProcessor::prepareOs (int mb)
{
    for (int k = 0; k < 5; ++k)
    {
        os[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) (k + 1),
                            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        os[(size_t) k]->initProcessing ((size_t) juce::jmax (32, mb));
    }
}

void TahorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedRate = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);
    reso.prepare (sampleRate, false);
    resoLow.prepare (sampleRate, true);
    for (int li = 0; li < 2; ++li) for (auto& rr : resoSplit[(size_t) li]) rr.prepare (sampleRate, li == 1);
    tahorStage.prepare (sampleRate, maxBlock);
    tahorStage.msEnabled = true;
    lrEq.prepare (sampleRate);
    for (auto& e : charEq) { e.prepare (sampleRate, maxBlock); e.externalColour = true; }
    for (auto& e : charEqR) { e.prepare (sampleRate, maxBlock); e.externalColour = true; }
    for (auto& s : charSat) s.prepare (sampleRate);
    charWas.fill (-1);
    prepareOs (maxBlock);
    clean.prepare (sampleRate, maxBlock);
    helpers.prepare (sampleRate);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    for (auto& st : soloC) for (auto& f : st) { f.coefficients = new juce::dsp::IIR::Coefficients<float> (1, 0, 0, 1, 0, 0); f.prepare (spec); }
    for (auto& st : soloF) for (auto& f : st) { f.coefficients = new juce::dsp::IIR::Coefficients<float> (1, 0, 0, 1, 0, 0); f.prepare (spec); }
    dry.setSize (2, maxBlock);
    keyBuf.setSize (2, maxBlock);
    for (auto& d : dryDelay) d.assign (1 << 17, 0.0f);
    dryPos = 0;
    reportedLatency = -1;
    setLatencySamples (latencyNow());
}

bool TahorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (layouts.getMainInputChannelSet() != out) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false;
    }
    return true;
}

int TahorProcessor::latencyNow() const
{
    // Pressure V1: only CLEAN's look-ahead delays the sound
    return param ("tahorOn") > 0.5f ? pressure::CleanCompressor::lookSamples (cleanSettings(), preparedRate) : 0;
}

pressure::HelperSettings TahorProcessor::helperSettings() const
{
    pressure::HelperSettings s;
    s.liftOn = param ("hLiftOn") > 0.5f; s.rideOn = param ("hRideOn") > 0.5f; s.breathOn = param ("hBreathOn") > 0.5f; s.peakOn = param ("hPeakOn") > 0.5f;
    s.liftMode = (int) param ("hLiftMode");
    s.lowHz = param ("hLowHz"); s.lowThr = param ("hLowThr"); s.lowLift = param ("hLowLift");
    s.highHz = param ("hHighHz"); s.highThr = param ("hHighThr"); s.highLift = param ("hHighLift");
    s.rideRange = param ("hRideRange"); s.rideTarget = param ("hRideTarget"); s.rideSpeed = (int) param ("hRideSpeed");
    s.breathDb = param ("hBreath"); s.breathSense = param ("hBreathSense"); s.breathFadeMs = param ("hBreathFade");
    s.inDbu = param ("hPkIn"); s.outDbu = param ("hPkOut"); s.slope = param ("hPkSlope"); s.release = param ("hPkRel"); s.pad = (int) param ("hPkPad");
    return s;
}

pressure::CleanSettings TahorProcessor::cleanSettings() const
{
    pressure::CleanSettings s;
    s.engine = (int) param ("cEngine");
    s.threshDb = param ("cThresh"); s.ratio = param ("cRatio"); s.kneeDb = param ("cKnee"); s.ds1Knee = param ("cDsKnee");
    s.attackMs = param ("cAttack"); s.releaseMs = param ("cRelease");
    s.autoRelease = param ("cAutoRel") > 0.5f; s.autoAttack = param ("cAutoAtk") > 0.5f; s.rms = (int) param ("cPeak") == 1;
    s.behavior = (int) param ("cGlide"); s.colour = (int) param ("cColor");
    s.dsRelFastMs = param ("cDsRelF"); s.dsRelSlowMs = param ("cDsRelS"); s.dsAvgMs = param ("cDsAvg"); s.dsBand = (int) param ("cDsBand");
    s.makeupDb = param ("cMakeup"); s.mix = param ("cMix") * 0.01f; s.autoGain = param ("cAutoGain") > 0.5f; s.delta = param ("cDelta") > 0.5f;
    s.rangeDb = param ("cRange"); s.holdMs = param ("cHold");
    s.voxComp = param ("cVoxComp"); s.voxGateDb = param ("cVoxGate"); s.voxGainDb = param ("cVoxGain"); s.voxMatch = param ("cVoxMatch") > 0.5f;
    s.mlaIn = (int) param ("cMlaIn"); s.mlaRatio = (int) param ("cMlaRatio"); s.mlaAtk = (int) param ("cMlaAtk"); s.mlaRel = (int) param ("cMlaRel");
    s.mlaLink = param ("cMlaLink") > 0.5f;
    s.proStyle = (int) param ("cProStyle"); s.proChar = (int) param ("cProChar"); s.proCharOn = param ("cProCharOn") > 0.5f; s.proDrive = param ("cProDrive");
    s.proPost = (int) param ("cProPre") == 1; s.proAutoThr = param ("cProAutoThr") > 0.5f; s.proDryDb = param ("cProDry"); s.proWetDb = param ("cProWet");
    s.external = (int) param ("cScSrc") == 1; s.listen = param ("cScListen") > 0.5f; s.scGainDb = param ("cScGain"); s.link = param ("cScLink") * 0.01f;
    s.ms = (int) param ("cScMs"); s.lookMs = param ("cLook");
    s.hpHz = param ("cScHp"); s.bellHz = param ("cScBellHz"); s.bellDb = param ("cScBellDb"); s.lpHz = param ("cScLp");
    if (s.engine == pressure::EMla || s.engine == pressure::E755) s.rangeDb = 60.0f;   // (their own timing / range)
    return s;
}

void TahorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto main = getBusBuffer (buffer, true, 0);
    const int n = main.getNumSamples();
    const int nc = juce::jmin (2, main.getNumChannels());
    if (n == 0 || nc == 0) return;
    if (n > maxBlock) { prepareToPlay (preparedRate, n); }

    // the sidechain (key) input
    const bool scOn = (int) param ("cScSrc") == 1 && hasSidechain();
    if (scOn)
    {
        auto key = getBusBuffer (buffer, true, 1);
        keyBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < 2; ++c) keyBuf.copyFrom (c, 0, key, juce::jmin (c, key.getNumChannels() - 1), 0, n);
        keyTap.push (keyBuf);   // V12: EQ Match listens to the reference here
    }

    // the untouched signal, delayed by the latency, for BYPASS
    const int lat = latencyNow();
    const int dlen = (int) dryDelay[0].size();
    for (int c = 0; c < 2; ++c)
    {
        dry.setSize (2, n, false, false, true);
        auto* x = main.getReadPointer (juce::jmin (c, nc - 1));
        auto* d = dry.getWritePointer (c);
        auto& line = dryDelay[c];
        for (int i = 0; i < n; ++i)
        {
            const int w = (dryPos + i) & (dlen - 1);
            line[(size_t) w] = x[i];
            d[i] = line[(size_t) ((w - juce::jmin (lat, dlen - 1)) & (dlen - 1))];
        }
    }
    dryPos = (dryPos + n) & (dlen - 1);

    // input level (and AUTO learning) + MATCH's reference
    double msIn = 0.0;
    for (int c = 0; c < nc; ++c) { auto* x = main.getReadPointer (c); for (int i = 0; i < n; ++i) msIn += (double) x[i] * x[i]; }
    msIn /= (double) (n * nc);
    if (autoLearning.load()) { learnSum += msIn * n; learnCount += n; }
    const float inG = juce::Decibels::decibelsToGain (param ("inGain"));
    main.applyGainRamp (0, n, inSmooth, inG); inSmooth = inG;
    preTap.push (main);
    inMeterL.store (main.getMagnitude (0, 0, n));
    inMeterR.store (main.getMagnitude (nc - 1, 0, n));
    { float r2 = 0; for (int c = 0; c < nc; ++c) r2 = juce::jmax (r2, main.getRMSLevel (c, 0, n)); inRms.store (r2); }   // V10: for the readouts

    // ===== PRESSURE: HELPERS > CLEAN > VINTAGE =====
    // HELPERS (LE-4): Lift > Rider > Breath > 1964
    if (param ("resoOn") > 0.5f) helpers.process (main, nc, n, helperSettings());
    else { helpers.peakGr.store (0.0f); helpers.rideDb.store (0.0f); helpers.breathRed.store (0.0f); helpers.liftLow.store (0.0f); helpers.liftHigh.store (0.0f); }
    // CLEAN
    {
        const auto cs = cleanSettings();
        float gr = 0.0f;
        if (param ("tahorOn") > 0.5f)
        {
            const bool ext = cs.external && hasSidechain();
            gr = clean.process (main, nc, n, ext ? &keyBuf : nullptr, cs);
        }
        grDb.store (gr);
        cleanAvgGr.store (0.98f * cleanAvgGr.load() + 0.02f * gr);
    }
    // VINTAGE (V1: passes the sound through - wired up in a later version)

    // output: MATCH (equal loudness with the input), OUT, BYPASS
    double msOut = 0.0;
    for (int c = 0; c < nc; ++c) { auto* x = main.getReadPointer (c); for (int i = 0; i < n; ++i) msOut += (double) x[i] * x[i]; }
    msOut /= (double) (n * nc);
    const float k = std::exp (-(float) n / (1.5f * (float) preparedRate));
    matchIn = k * matchIn + (1.0f - k) * (float) msIn;
    matchOut = k * matchOut + (1.0f - k) * (float) msOut;
    float target = 1.0f;
    if (param ("match") > 0.5f && matchOut > 1.0e-9f && matchIn > 1.0e-9f) target = juce::jlimit (0.125f, 8.0f, std::sqrt (matchIn / matchOut));
    matchGain += (target - matchGain) * (1.0f - k);
    const float outG = juce::Decibels::decibelsToGain (param ("outGain") + (param ("autoGain") > 0.5f ? autoGainDb.load() : 0.0f)) * matchGain;   // V9: + auto gain
    main.applyGainRamp (0, n, outSmooth, outG); outSmooth = outG;

    if (param ("bypass") > 0.5f)
        for (int c = 0; c < nc; ++c) main.copyFrom (c, 0, dry, c, 0, n);

    postTap.push (main);
    meterL.store (main.getMagnitude (0, 0, n));
    meterR.store (main.getMagnitude (nc - 1, 0, n));
    { float r2 = 0; for (int c = 0; c < nc; ++c) r2 = juce::jmax (r2, main.getRMSLevel (c, 0, n)); outRms.store (r2); }   // V10
}

juce::AudioProcessorEditor* TahorProcessor::createEditor() { return new TahorEditor (*this); }

//==============================================================================
void TahorProcessor::setParam (const juce::String& pid, float value)
{
    if (auto* p = apvts.getParameter (pid))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}
void TahorProcessor::beginGesture (const juce::String& pid) { if (auto* p = apvts.getParameter (pid)) p->beginChangeGesture(); }
void TahorProcessor::dragParam (const juce::String& pid, float value) { if (auto* p = apvts.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value)); }
void TahorProcessor::endGesture (const juce::String& pid) { if (auto* p = apvts.getParameter (pid)) p->endChangeGesture(); }

int TahorProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String TahorProcessor::getProgramName (int index)
{
    const auto& l = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) l.size()) ? juce::String::fromUTF8 (l[(size_t) index].name) : juce::String();
}
void TahorProcessor::setCurrentProgram (int index) { loadFactoryPreset (index); }

void TahorProcessor::loadFactoryPreset (int index)
{
    const auto& l = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) l.size())) return;
    loadingPreset = true;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    bool usesReso = false;
    for (auto& [pid, v] : l[(size_t) index].values)
    {
        const juce::String id (pid);
        if (id == "resoOn" || id.startsWith ("rs") || id.startsWith ("rn")) usesReso = true;
        if (auto* p = apvts.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    }
    if (! usesReso) if (auto* p = apvts.getParameter ("resoOn")) p->setValueNotifyingHost (0.0f);   // V14: a preset that doesn't use Reso leaves it bypassed
    loadingPreset = false;
    currentProgram = index;
    presetName = juce::String::fromUTF8 (l[(size_t) index].name);
    presetDirty = false;
}

juce::String TahorProcessor::factoryTag (int index)
{
    return factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, index)].tag;   // V11: each preset carries its folder
}

juce::String TahorProcessor::presetTagOf (const juce::File& f) const
{
    if (auto xml = juce::XmlDocument::parse (f)) return xml->getStringAttribute ("presetTag", "Mine");
    return "Mine";
}

bool TahorProcessor::saveUserPreset (const juce::String& name, const juce::String& tag)
{
    apvts.state.setProperty ("presetTag", tag, nullptr);
    return saveUserPreset (name);
}

juce::File TahorProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Presets/Mishkal Audio/Pressure");
}

juce::Array<juce::File> TahorProcessor::userPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml");
    files.sort();
    return files;
}

bool TahorProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto folder = userPresetFolder();
    folder.createDirectory();
    auto state = apvts.copyState();
    state.setProperty ("presetName", name.trim(), nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (folder.getChildFile (clean + ".xml"))) { presetName = name.trim(); presetDirty = false; return true; }
    return false;
}

bool TahorProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loadingPreset = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadingPreset = false;
            presetName = xml->getStringAttribute ("presetName", f.getFileNameWithoutExtension());
            presetDirty = false;
            return true;
        }
    return false;
}

void TahorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetDirty", presetDirty, nullptr);
    { juce::StringArray m; for (auto& o : charOff) m.add (juce::String (o.load())); state.setProperty ("charOff", m.joinIntoString (","), nullptr); }   // V12
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TahorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loadingPreset = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadingPreset = false;
            presetName = xml->getStringAttribute ("presetName", presetName);
            presetDirty = xml->getBoolAttribute ("presetDirty", false);
            { const auto m = juce::StringArray::fromTokens (xml->getStringAttribute ("charOff"), ",", "");   // V12
              for (int u = 0; u < numUnits; ++u) charOff[(size_t) u].store (u < m.size() ? m[u].getIntValue() : 0); }
        }
}

//==============================================================================
std::vector<float> TahorProcessor::snapshot() const
{
    std::vector<float> v;
    for (auto* p : getParameters()) v.push_back (p->getValue());
    return v;
}

void TahorProcessor::applySnapshot (const std::vector<float>& v)
{
    const auto& params = getParameters();
    for (int i = 0; i < juce::jmin ((int) v.size(), params.size()); ++i)
        if (std::abs (params[i]->getValue() - v[(size_t) i]) > 1.0e-6f)
        {
            params[i]->beginChangeGesture(); params[i]->setValueNotifyingHost (v[(size_t) i]); params[i]->endChangeGesture();
        }
    lastSeen = v; stableTicks = 0;
}

void TahorProcessor::recordHistory()
{
    auto now = snapshot();
    if (now != lastSeen) { lastSeen = now; stableTicks = 0; return; }
    if (++stableTicks != 7) return;
    if (historyPos >= 0 && history[(size_t) historyPos] == now) return;
    // V9: name the step - what changed since the last one
    juce::String label = "Start";
    if (historyPos >= 0)
    {
        const auto& prev = history[(size_t) historyPos];
        const auto& params = getParameters();
        int changed = 0, first = -1;
        for (int i = 0; i < juce::jmin ((int) prev.size(), (int) now.size()); ++i)
            if (std::abs (prev[(size_t) i] - now[(size_t) i]) > 1.0e-6f) { if (first < 0) first = i; ++changed; }
        if (changed > 8) label = "Preset / many changes";
        else if (first >= 0)
        {
            auto* p = params[first];
            label = p->getName (40) + "  " + p->getCurrentValueAsText() + (p->getLabel().isNotEmpty() ? " " + p->getLabel() : juce::String());
            if (changed > 1) label << "  (+" << (changed - 1) << ")";
        }
    }
    history.resize ((size_t) (historyPos + 1));
    historyLabel.removeRange (historyPos + 1, historyLabel.size());
    history.push_back (now);
    historyLabel.add (label);
    if (history.size() > 200) { history.erase (history.begin()); historyLabel.remove (0); }
    historyPos = (int) history.size() - 1;
}

void TahorProcessor::jumpHistory (int pos)
{
    if (pos < 0 || pos >= (int) history.size() || pos == historyPos) return;
    historyPos = pos;
    applySnapshot (history[(size_t) pos]);
    presetDirty = true;
}

void TahorProcessor::undo() { if (canUndo()) { applySnapshot (history[(size_t) --historyPos]); presetDirty = true; } }
void TahorProcessor::redo() { if (canRedo()) { applySnapshot (history[(size_t) ++historyPos]); presetDirty = true; } }

void TahorProcessor::switchAB (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abSlot) return;
    slots[abSlot] = apvts.copyState();
    slotNames[abSlot] = presetName + (presetDirty ? " *" : "");
    abSlot = slot;
    if (slots[slot].isValid())
    {
        loadingPreset = true;
        apvts.replaceState (slots[slot].createCopy());
        loadingPreset = false;
        presetName = slotNames[slot].trimCharactersAtEnd (" *");
        presetDirty = slotNames[slot].endsWith (" *");
    }
}

void TahorProcessor::timerCallback()
{
    if (! loadingPreset) recordHistory();
    // AUTO GAIN (header): gives back about half of what CLEAN takes on average
    autoGainDb.store (juce::jlimit (0.0f, 24.0f, cleanAvgGr.load() * 0.5f));
    const int lat = latencyNow();
    if (lat != reportedLatency) { reportedLatency = lat; setLatencySamples (lat); }
    // AUTO: listen for ~3 seconds, then set IN so the level sits around -18 dBFS RMS
    if (autoLearning.load() && learnCount > 0 && learnCount > (int) (3.0 * preparedRate))
    {
        const double ms = learnSum / learnCount;
        autoLearning.store (false);
        if (ms > 1.0e-9)
            setParam ("inGain", juce::jlimit (-24.0f, 24.0f, -18.0f - (float) (10.0 * std::log10 (ms))));
        learnSum = 0.0; learnCount = 0;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TahorProcessor(); }
