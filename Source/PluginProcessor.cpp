#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "PluginEditor.h"

using namespace vox;
using APF   = juce::AudioParameterFloat;
using Range = juce::NormalisableRange<float>;

namespace
{
    juce::String hzText (float v, int)
    {
        return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz"
                           : juce::String (v / 1000.0f, v < 10000.0f ? 2 : 1) + " kHz";
    }
    juce::String dbText (float v, int)    { return (v > 0.04f ? "+" : "") + juce::String (v, 1) + " dB"; }
    juce::String msText (float v, int)    { return v < 1.0f ? juce::String (v * 1000.0f, 0) + " us"
                                                            : v < 100.0f ? juce::String (v, 1) + " ms"
                                                                         : juce::String (juce::roundToInt (v)) + " ms"; }
    juce::String gateText (float v, int)  { return v <= VoxGate::offBelowDb ? juce::String ("Off") : dbText (v, 0); }
    juce::String pctText (float v, int)   { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
    juce::String numText (float v, int)   { return juce::String (v, 1); }
    juce::String riderText (float v, int) { return v < 0.25f ? juce::String ("Off") : juce::String (juce::CharPointer_UTF8 ("\xc2\xb1")) + juce::String (v, 1) + " dB"; }
    juce::String breathText (float v, int){ return v > -0.25f ? juce::String ("Off") : juce::String (v, 1) + " dB"; }
    juce::String ratioText (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + ":1"; }
    juce::String qText (float v, int)     { return "Q " + juce::String (v, 2); }

    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    std::unique_ptr<APF> param (const char* pid, const char* name, Range range, float def,
                                juce::String (*text) (float, int), const char* unit)
    {
        return std::make_unique<APF> (juce::ParameterID { pid, 1 }, name, range, def,
                                      juce::AudioParameterFloatAttributes()
                                          .withLabel (unit)
                                          .withStringFromValueFunction (text));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxStripProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto onOff = [&] (const char* pid, const char* name, bool def)
    {
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { pid, 1 }, name, def));
    };

    l.add (param (id::inputGain,  "Input",  Range (-24.0f, 24.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::outputGain, "Output", Range (-24.0f, 12.0f, 0.1f), 0.0f, dbText, "dB"));

    // A fresh insert is a clean, flat, unity-gain strip: every section is ready but switched off,
    // and the EQs start flat (console high-pass at 20 Hz). The old vocal start lives on as "Vocal Starter".

    // Resonance suppressor (soothe-style, before the clean-up EQ)
    onOff (id::resoOn, "Reso On", true);    // on when you insert the plugin
    l.add (param (id::resoDepth,   "Reso Depth",       Range (0.0f, 1.0f, 0.01f),             MISHKAL_DEF_RESO_DEPTH, pctText, ""));
    l.add (param (id::resoSharp,   "Reso Sharpness",   Range (0.0f, 1.0f, 0.01f),             0.4f,    pctText, ""));
    l.add (param (id::resoSelect,  "Reso Selectivity", Range (0.0f, 1.0f, 0.01f),             0.5f,    pctText, ""));
    l.add (param (id::resoAttack,  "Reso Attack",      skewed (0.5f, 100.0f, 8.0f),           5.0f,    msText,  "ms"));
    l.add (param (id::resoRelease, "Reso Release",     skewed (5.0f, 500.0f, 80.0f),          80.0f,   msText,  "ms"));
    l.add (param (id::resoLow,     "Reso Low",         skewed (20.0f, 20000.0f, 640.0f),      300.0f,  hzText,  "Hz"));
    l.add (param (id::resoHigh,    "Reso High",        skewed (20.0f, 20000.0f, 640.0f),      12000.0f, hzText, "Hz"));
    l.add (param (id::resoMix,     "Reso Mix",         Range (0.0f, 1.0f, 0.01f),             1.0f,    pctText, ""));
    onOff (id::resoDelta, "Reso Delta", false);
    // V60 mid/side (stereo tracks): which half Reso works on, and the side's own depth in Mid + Side
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::resoMs, 1 }, "Reso M/S Mode",
                                                         juce::StringArray { "Stereo", "Mid", "Side", "Mid + Side" }, 0));
    l.add (param (id::resoSideDepth, "Reso Side Depth", Range (0.0f, 1.0f, 0.01f), MISHKAL_DEF_RESO_DEPTH, pctText, ""));
    // V61: stereo link (like soothe's Link): 100% = left and right share one detector, 0% = each side on its own
    l.add (param (id::resoLink, "Reso Stereo Link", Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, ""));

    // Clean-up EQ (8 bands, all off until you add them on the graph)
    onOff (id::peakCatch, "Input Peak Catch", false);
    // the peak catcher's own input drive and output trim; Link turns the output down by whatever the input pushes
    l.add (param (id::peakIn,  "Peak Catch Input",  Range (-12.0f, 18.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::peakOut, "Peak Catch Output", Range (-18.0f, 12.0f, 0.1f), 0.0f, dbText, "dB"));
    onOff (id::peakLink, "Peak Catch Link", true);
    onOff (id::trackMode, "Tracking Mode (Low Latency)", false);
    onOff (id::cleanOn, "Tahor EQ On", true);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::cleanPhase, 1 }, "Tahor EQ Phase",
                                                         juce::StringArray { "Natural", "Linear", "Linear HQ" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::cleanOs, 1 }, "Tahor EQ Oversampling",
                                                         juce::StringArray { "Off", "2x", "4x" }, 0));
    {
        const float freqs[numCleanBands] { 60.0f, 150.0f, 300.0f, 600.0f, 1200.0f, 2500.0f, 5000.0f, 12000.0f };
        for (int b = 0; b < numCleanBands; ++b)
        {
            const auto n = "Band " + juce::String (b + 1) + " ";
            const int defType = b == 0 ? (int) BandType::LowCut : b == numCleanBands - 1 ? (int) BandType::HighCut : (int) BandType::Bell;
            auto pid = [b] (const char* f) { return juce::ParameterID { bandId (b, f), 1 }; };
            auto fparam = [&] (const char* f, const juce::String& name, Range r, float def, juce::String (*text) (float, int), const char* unit)
            {
                l.add (std::make_unique<APF> (pid (f), name, r, def,
                    juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (text)));
            };

            l.add (std::make_unique<juce::AudioParameterBool>   (pid ("On"),   n + "On", false));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Type"), n + "Type", bandTypeNames, defType));
            fparam ("Freq", n + "Freq", skewed (20.0f, 20000.0f, 640.0f), freqs[b], hzText, "Hz");
            fparam ("Gain", n + "Gain", Range (-30.0f, 30.0f, 0.01f), 0.0f, dbText, "dB");
            fparam ("Q",    n + "Q",    skewed (0.1f, 18.0f, 1.0f), defType == (int) BandType::Bell ? 1.0f : 0.71f, qText, "");
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Slope"), n + "Slope", slopeNames, 1));
            l.add (std::make_unique<juce::AudioParameterBool>   (pid ("Dyn"),   n + "Dynamic", false));
            fparam ("DynThresh", n + "Dyn Threshold", Range (-80.0f, 0.0f, 0.1f), -30.0f, dbText, "dB");
            fparam ("DynRange",  n + "Dyn Range",     Range (-24.0f, 24.0f, 0.1f), -6.0f, dbText, "dB");
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Place"), n + "Stereo Placement",   // V60
                                                                 juce::StringArray { "Stereo", "Mid", "Side" }, 0));
        }
    }

    // Character EQs: four independent EQs, each with its own settings and on/off
    onOff (id::eqOn, "Character EQ On", false);
    // Tube P1: Pultec-style program EQ with stepped frequencies and 0-10 dials
    {
        auto pid = [] (const char* f) { return juce::ParameterID { charEqId (0, f), 1 }; };
        auto dial = [&] (const char* f, const char* name)
        {
            l.add (std::make_unique<APF> (pid (f), juce::String ("LE-EP1 ") + name, Range (0.0f, 10.0f, 0.1f), 0.0f,
                   juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })));
        };
        auto choice = [&] (const char* f, const char* name, const juce::StringArray& opts, int def)
        {
            l.add (std::make_unique<juce::AudioParameterChoice> (pid (f), juce::String ("LE-EP1 ") + name, opts, def));
        };
        l.add (std::make_unique<juce::AudioParameterBool> (pid ("On"), "LE-EP1 On", false));
        choice ("LowFreq",   "Low Frequency",   p1LowFreqs, 2);
        dial   ("LowBoost",  "Low Boost");
        dial   ("LowAtten",  "Low Atten");
        choice ("HighFreq",  "High Frequency",  p1HighFreqs, 4);
        l.add (std::make_unique<APF> (pid ("HighBw"), "LE-EP1 Bandwidth", Range (0.0f, 10.0f, 0.1f), 5.0f,
               juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                   { return v < 2.5f ? juce::String ("Sharp ") + juce::String (v, 1) : v > 7.5f ? juce::String ("Broad ") + juce::String (v, 1) : juce::String (v, 1); })));
        dial   ("HighBoost", "High Boost");
        choice ("AttenFreq", "Atten Frequency", p1AttenFreqs, 1);
        dial   ("HighAtten", "High Atten");
    }
   #if MISHKAL_VERSION >= 74
    // V74: four more character EQs, each with the controls of the hardware it is modelled on
    {
        auto onOffEq = [&] (int t, const char* f, const juce::String& name, bool def)
        { l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { charEqId (t, f), 1 }, eqTypeNames[t] + " " + name, def)); };
        auto choiceEq = [&] (int t, const char* f, const juce::String& name, const juce::StringArray& c, int def)
        { l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { charEqId (t, f), 1 }, eqTypeNames[t] + " " + name, c, def)); };
        auto floatEq = [&] (int t, const char* f, const juce::String& name, Range r, float def, juce::String (*text) (float, int), const char* unit)
        {
            l.add (std::make_unique<APF> (juce::ParameterID { charEqId (t, f), 1 }, eqTypeNames[t] + " " + name, r, def,
                   juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (text)));
        };
        const juce::StringArray shelfPeak { "Shelf", "Peak" };
        // LE-A550 (API 550B-style)
        onOffEq (4, "On", "On", false);  onOffEq (4, "Filter", "Band-pass Filter", false);
        choiceEq (4, "LowFreq", "Low Freq", apiLowFreqs, 3);        floatEq (4, "LowGain", "Low Gain", Range (-12.0f, 12.0f, 0.5f), 0.0f, dbText, "dB");
        choiceEq (4, "LowShape", "Low Shape", shelfPeak, 0);
        choiceEq (4, "LmFreq", "Low-Mid Freq", apiLmFreqs, 4);       floatEq (4, "LmGain", "Low-Mid Gain", Range (-12.0f, 12.0f, 0.5f), 0.0f, dbText, "dB");
        choiceEq (4, "HmFreq", "High-Mid Freq", apiHmFreqs, 2);      floatEq (4, "HmGain", "High-Mid Gain", Range (-12.0f, 12.0f, 0.5f), 0.0f, dbText, "dB");
        choiceEq (4, "HighFreq", "High Freq", apiHighFreqs, 3);      floatEq (4, "HighGain", "High Gain", Range (-12.0f, 12.0f, 0.5f), 0.0f, dbText, "dB");
        choiceEq (4, "HighShape", "High Shape", shelfPeak, 0);
        // LE-K9 (SSL 9000K-style)
        auto offLow  = [] (float v, int) { return v <= 16.5f ? juce::String ("Off") : hzText (v, 0); };
        auto offHigh = [] (float v, int) { return v >= 21900.0f ? juce::String ("Off") : hzText (v, 0); };
        onOffEq (5, "On", "On", false);
        floatEq (5, "HpfFreq", "High-Pass", skewed (16.0f, 350.0f, 80.0f), 16.0f, offLow, "Hz");
        floatEq (5, "LpfFreq", "Low-Pass", skewed (3000.0f, 22000.0f, 10000.0f), 22000.0f, offHigh, "Hz");
        floatEq (5, "LfFreq", "LF Freq", skewed (30.0f, 450.0f, 100.0f), 100.0f, hzText, "Hz");   floatEq (5, "LfGain", "LF Gain", Range (-15.0f, 15.0f, 0.1f), 0.0f, dbText, "dB");
        onOffEq (5, "LfBell", "LF Bell", false);
        floatEq (5, "LmfFreq", "LMF Freq", skewed (200.0f, 2500.0f, 600.0f), 400.0f, hzText, "Hz"); floatEq (5, "LmfGain", "LMF Gain", Range (-15.0f, 15.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (5, "LmfQ", "LMF Q", skewed (0.4f, 3.0f, 1.0f), 1.0f, qText, "");
        floatEq (5, "HmfFreq", "HMF Freq", skewed (600.0f, 7000.0f, 2000.0f), 3000.0f, hzText, "Hz"); floatEq (5, "HmfGain", "HMF Gain", Range (-15.0f, 15.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (5, "HmfQ", "HMF Q", skewed (0.4f, 3.0f, 1.0f), 1.0f, qText, "");
        floatEq (5, "HfFreq", "HF Freq", skewed (1500.0f, 16000.0f, 6000.0f), 10000.0f, hzText, "Hz"); floatEq (5, "HfGain", "HF Gain", Range (-15.0f, 15.0f, 0.1f), 0.0f, dbText, "dB");
        onOffEq (5, "HfBell", "HF Bell", false);
        choiceEq (5, "Mode", "Curves", juce::StringArray { "E", "G" }, 0);
        // LE-H69 (Helios 69-style)
        onOffEq (6, "On", "On", false);
        choiceEq (6, "BassFreq", "Bass Freq", heliosBassFreqs, 2);   floatEq (6, "BassGain", "Bass", Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        choiceEq (6, "MidFreq", "Mid Freq", heliosMidFreqs, 3);      floatEq (6, "MidGain", "Mid", Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (6, "TrebleGain", "Treble", Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        // LE-M4 (Maag EQ4-style)
        onOffEq (7, "On", "On", false);
        floatEq (7, "SubGain", "Sub", Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (7, "G40",  "40 Hz",   Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (7, "G160", "160 Hz",  Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (7, "G650", "650 Hz",  Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        floatEq (7, "G2k5", "2.5 kHz", Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB");
        choiceEq (7, "AirFreq", "Air Freq", maagAirFreqs, 2);         floatEq (7, "AirGain", "Air", Range (0.0f, 15.0f, 0.1f), 0.0f, dbText, "dB");
        l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id::eqOrder8, 1 }, "Character EQ Order (8)", 0, 40319, 0));
    }
   #endif
    for (int t = 0; t < eqTypeNames.size(); ++t)   // V71: each character EQ can work on the mid or the side only
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { charEqId (t, "Place"), 1 }, eqTypeNames[t] + " Placement",
                                                             juce::StringArray { "Stereo", "Mid", "Side" }, 0));

    for (int t = 1; t < 4; ++t)
    {
        const auto n = eqTypeNames[t] + " ";
        auto pid = [t] (const char* f) { return juce::ParameterID { charEqId (t, f), 1 }; };
        auto add = [&] (const char* f, const juce::String& name, Range r, float def, juce::String (*text) (float, int), const char* unit)
        {
            l.add (std::make_unique<APF> (pid (f), n + name, r, def,
                   juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (text)));
        };
        l.add (std::make_unique<juce::AudioParameterBool> (pid ("On"), n + "On", t == 1));   // E-Console on by default
        add ("HpfFreq",  "High-Pass",     skewed (20.0f, 400.0f, 80.0f),        20.0f,    hzText, "Hz");
        add ("BodyFreq", "Body Freq",     skewed (50.0f, 500.0f, 150.0f),       180.0f,   hzText, "Hz");
        add ("BodyGain", "Body Gain",     Range (-15.0f, 15.0f, 0.1f),          0.0f,     dbText, "dB");
        add ("MudFreq",  "Mud Freq",      skewed (100.0f, 2000.0f, 400.0f),     350.0f,   hzText, "Hz");
        add ("MudGain",  "Mud Gain",      Range (-15.0f, 15.0f, 0.1f),          0.0f,     dbText, "dB");
        add ("MudQ",     "Mud Q",         skewed (0.3f, 4.0f, 1.0f),            1.2f,     qText,  "");
        add ("PresFreq", "Presence Freq", skewed (1000.0f, 10000.0f, 3000.0f),  3500.0f,  hzText, "Hz");
        add ("PresGain", "Presence Gain", Range (-15.0f, 15.0f, 0.1f),          0.0f,     dbText, "dB");
        add ("PresQ",    "Presence Q",    skewed (0.3f, 4.0f, 1.0f),            0.9f,     qText,  "");
        add ("AirFreq",  "Air Freq",      skewed (5000.0f, 20000.0f, 10000.0f), 12000.0f, hzText, "Hz");
        add ("AirGain",  "Air Gain",      Range (-15.0f, 15.0f, 0.1f),          0.0f,     dbText, "dB");
        if (t == (int) EqType::Airy)
            add ("Silk", "Silk", Range (0.0f, 1.0f, 0.01f), 0.25f, pctText, "");
    }

    // Gate
    l.add (param (id::gateThresh, "Gate", Range (-80.0f, 0.0f, 0.1f), -80.0f, gateText, "dB"));
    // breaths turned down, then the vocal rider, then the compressors
    l.add (param (id::breathAmount, "Breath Control", Range (-24.0f, 0.0f, 0.5f), 0.0f, breathText, "dB"));
    l.add (param (id::riderRange,   "Vocal Rider Range", Range (0.0f, 12.0f, 0.5f), 0.0f, riderText, "dB"));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::compScHpf, 1 }, "Compressor Sidechain HPF",
                                                         juce::StringArray { "Off", "80 Hz", "120 Hz", "160 Hz" }, 0));

    // Compressors: three independent units, each with its own on/off and settings
    onOff (id::compOn, "Comp On", false);

    // Processing order (drag the tabs) and where the character EQs sit
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::eqOrder, 1 },   "Character EQ Order", orderNames<4> (eqTypeNames), 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::compOrder, 1 }, "Compressor Order",   orderNames<3> (compTypeNames3), 0));
    // V52: four compressors (LE-STA got its own unit) - the order of all four
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::compOrder4, 1 }, "Compressor Order (4)", orderNames<4> (compTypeNames), 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::eqAfterComp, 1 }, "Character EQ Position",
                                                         juce::StringArray { "Before Comp", "After Comp" }, 0));
    for (int u = 0; u < 4; ++u)
    {
        const auto n = compTypeNames[u] + " ";
        auto pid = [u] (const char* fld) { return juce::ParameterID { compId (u, fld), 1 }; };
        auto add = [&] (const char* fld, const juce::String& name, Range r, float def, juce::String (*text) (float, int), const char* unit)
        {
            l.add (std::make_unique<APF> (pid (fld), n + name, r, def,
                   juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (text)));
        };
        l.add (std::make_unique<juce::AudioParameterBool> (pid ("On"), n + "On", u == 0));   // FET Purple on by default
        add ("Thresh", "Threshold", Range (-60.0f, 0.0f, 0.1f), -18.0f, dbText, "dB");
        if (u == (int) CompType::FetPurple)
        {
            add ("Ratio",   "Ratio",   skewed (1.0f, 20.0f, 4.0f),      4.0f,   ratioText, "");
            add ("Attack",  "Attack",  skewed (0.02f, 100.0f, 3.0f),    0.8f,   msText,    "ms");
            add ("Release", "Release", skewed (20.0f, 2000.0f, 200.0f), 120.0f, msText,    "ms");
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Rev"), n + "Revision",
                                                                 juce::StringArray { "Purple", "Blue Stripe", "Black" }, 0));
        }
        if (u == (int) CompType::RComp)
        {
            add ("Ratio",   "Ratio",   skewed (1.0f, 50.0f, 4.0f),      3.0f,   ratioText, "");
            add ("Attack",  "Attack",  skewed (0.5f, 500.0f, 10.0f),    5.0f,   msText,    "ms");
            add ("Release", "Release", skewed (5.0f, 3000.0f, 200.0f),  150.0f, msText,    "ms");
            l.add (std::make_unique<juce::AudioParameterBool>   (pid ("Arc"), n + "Auto Release", true));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Behavior"), n + "Release Shape", juce::StringArray { "Glide", "Snap" }, 0));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Character"), n + "Tone", juce::StringArray { "Color", "Pure" }, 0));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Model"), n + "Model", juce::StringArray { "LE-RC", "LE-RV" }, 0));
            l.add (std::make_unique<juce::AudioParameterBool> (pid ("VoxLink"), n + "LE-RV Link", true));   // threshold <-> output (automatic make-up)
        }
        if (u == (int) CompType::StaLevel)   // V52: LE-STA as its own unit - Input, Mode, Recovery (+ Output / Mix below)
        {
            add ("StaIn", "Input", Range (-12.0f, 24.0f, 0.1f), 0.0f, dbText, "dB");
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("StaMode"), n + "Mode", juce::StringArray { "Single", "Double", "Triple" }, 1));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("StaRel"), n + "Recovery", juce::StringArray { "Fast", "Medium", "Slow", "Very Slow" }, 1));
        }
        if (u == (int) CompType::VariMu660)
        {
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Tc"), n + "Time Constant",
                                                                 juce::StringArray { "TC 1", "TC 2", "TC 3", "TC 4", "TC 5", "TC 6" }, 1));
            // the tube slot's second model: LE-STA (Sta-Level-style program level amplifier)
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("Model"), n + "Model", juce::StringArray { "LE-660", "LE-STA" }, 0));
            add ("StaIn", "LE-STA Input", Range (-12.0f, 24.0f, 0.1f), 0.0f, dbText, "dB");
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("StaMode"), n + "LE-STA Mode", juce::StringArray { "Single", "Double", "Triple" }, 1));
            l.add (std::make_unique<juce::AudioParameterChoice> (pid ("StaRel"), n + "LE-STA Recovery", juce::StringArray { "Fast", "Medium", "Slow", "Very Slow" }, 1));
        }
        add ("Makeup", "Make-up", Range (-12.0f, 24.0f, 0.1f), 0.0f, dbText, "dB");
        add ("Mix",    "Mix",     Range (0.0f, 1.0f, 0.01f),   1.0f, pctText, "");
    }

   #if MISHKAL_VERSION >= 75
    // V75: five bus / mastering compressors, each with the controls of the hardware it is modelled on
    {
        auto pid = [] (int u, const char* fld) { return juce::ParameterID { compId (u, fld), 1 }; };
        auto f = [&] (int u, const char* fld, const juce::String& name, Range r, float def, juce::String (*text) (float, int), const char* unit)
        {
            l.add (std::make_unique<APF> (pid (u, fld), compTypeNames[u] + " " + name, r, def,
                   juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (text)));
        };
        auto ch = [&] (int u, const char* fld, const juce::String& name, const juce::StringArray& c, int def)
        { l.add (std::make_unique<juce::AudioParameterChoice> (pid (u, fld), compTypeNames[u] + " " + name, c, def)); };
        auto on = [&] (int u) { l.add (std::make_unique<juce::AudioParameterBool> (pid (u, "On"), compTypeNames[u] + " On", false)); };
        auto common = [&] (int u, float threshDef)
        {
            on (u);
            f (u, "Thresh", "Threshold", Range (-40.0f, 0.0f, 0.1f), threshDef, dbText, "dB");
        };
        auto tail = [&] (int u)
        {
            f (u, "Makeup", "Make-up", Range (-6.0f, 20.0f, 0.1f), 0.0f, dbText, "dB");
            f (u, "Mix", "Mix", Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, "");
        };
        // LE-2500 (API 2500-style)
        common (4, -16.0f);
        f (4, "Ratio", "Ratio", skewed (1.5f, 10.0f, 3.0f), 4.0f, ratioText, "");
        ch (4, "Attack", "Attack", juce::StringArray { "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 2);
        f (4, "Release", "Release", skewed (50.0f, 2000.0f, 300.0f), 300.0f, msText, "ms");
        tail (4);
        ch (4, "Knee", "Knee", juce::StringArray { "Hard", "Med", "Soft" }, 1);
        ch (4, "Thrust", "Thrust", juce::StringArray { "Norm", "Med", "Loud" }, 0);
        ch (4, "Type", "Type", juce::StringArray { "New", "Old" }, 0);
        // LE-G (SSL G bus-style)
        common (5, -16.0f);
        ch (5, "Ratio", "Ratio", juce::StringArray { "2:1", "4:1", "10:1" }, 1);
        ch (5, "Attack", "Attack", juce::StringArray { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 5);
        ch (5, "Release", "Release", juce::StringArray { "0.1 s", "0.3 s", "0.6 s", "1.2 s", "Auto" }, 4);
        tail (5);
        // LE-DS1 (Weiss DS1-style)
        common (6, -14.0f);
        f (6, "Ratio", "Ratio", skewed (1.0f, 20.0f, 2.5f), 2.0f, ratioText, "");
        f (6, "Attack", "Attack", skewed (0.1f, 100.0f, 5.0f), 3.0f, msText, "ms");
        f (6, "Release", "Release", skewed (20.0f, 3000.0f, 200.0f), 200.0f, msText, "ms");
        tail (6);
        ch (6, "Knee", "Knee", juce::StringArray { "Hard", "Soft" }, 1);
        ch (6, "Detect", "Detection", juce::StringArray { "Peak", "RMS" }, 1);
        ch (6, "RelMode", "Release Mode", juce::StringArray { "Single", "Average" }, 1);
        // LE-IRON (SPL Iron-style)
        common (7, -14.0f);
        ch (7, "Attack", "Attack", juce::StringArray { "1", "2", "3", "4", "5", "6" }, 2);
        ch (7, "Release", "Release", juce::StringArray { "1", "2", "3", "4", "5", "Auto" }, 5);
        tail (7);
        ch (7, "Bias", "Tube Bias", juce::StringArray { "Low", "Mid", "High", "Max" }, 1);
        ch (7, "Rect", "Rectifier", juce::StringArray { "Silicon", "Germanium", "LED" }, 1);
        ch (7, "Airbass", "Airbass", juce::StringArray { "Off", "On" }, 0);
        // LE-336 (Neve 33609-style)
        common (8, -16.0f);
        ch (8, "Ratio", "Ratio", juce::StringArray { "1.5:1", "2:1", "3:1", "4:1", "6:1" }, 1);
        ch (8, "Release", "Recovery", juce::StringArray { "100 ms", "400 ms", "800 ms", "1.5 s", "A1", "A2" }, 4);
        f (8, "LimThresh", "Limiter", Range (-20.0f, 0.0f, 0.1f), 0.0f, [] (float v, int) { return v > -0.05f ? juce::String ("Off") : dbText (v, 0); }, "dB");
        ch (8, "LimRel", "Limiter Recovery", juce::StringArray { "50 ms", "100 ms", "200 ms", "800 ms", "A1", "A2" }, 1);
        ch (8, "LimAtt", "Limiter Attack", juce::StringArray { "Fast", "Slow" }, 0);
        tail (8);
        l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id::compOrder9, 1 }, "Compressor Order (9)", 0, 362879, 0));
    }
   #endif
    // De-esser
    onOff (id::dsOn, "De-esser On", false);
    l.add (param (id::dsFreq,   "De-ess Freq",      skewed (2000.0f, 12000.0f, 6000.0f), 6500.0f, hzText, "Hz"));
    l.add (param (id::dsThresh, "De-ess Threshold", Range (-60.0f, 0.0f, 0.1f),          -30.0f,  dbText, "dB"));
    l.add (param (id::dsRange,  "De-ess Range",     Range (0.0f, 24.0f, 0.1f),           8.0f,    dbText, "dB"));
    onOff (id::dsListen, "De-ess Listen", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::dsMode, 1 }, "De-ess Style", dsModeNames, 0));
    onOff (id::dsAuto, "De-ess Auto", false);

    // Vox Flow (styled 5-band vocal stage before the limiter)
    onOff (id::flowOn, "Zerem On", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::flowStyle, 1 }, "Zerem Style", flowStyleNames(), 11));   // Mix Bus: works on anything
    l.add (param (id::flowAmount, "Zerem Amount", Range (0.0f, 1.0f, 0.01f), 0.5f, pctText, ""));
    l.add (param (id::flowMix,    "Zerem Mix",    Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, ""));
    // Perach: the adaptive tone shaper that can take Zerem's place
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::flowModel, 1 }, "Zerem Model", juce::StringArray { "Zerem", "Perach" }, 0));
    l.add (param (id::perAmount,   "Perach Amount",   Range (0.0f, 10.0f, 0.1f), MISHKAL_DEF_PER_AMOUNT, numText, ""));
    l.add (param (id::perLow,      "Perach Low",      Range (-6.0f, 6.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::perBody,     "Perach Body",     Range (-6.0f, 6.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::perPresence, "Perach Presence", Range (-6.0f, 6.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::perAir,      "Perach Air",      Range (-6.0f, 6.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::perSpeed,    "Perach Speed",    Range (0.0f, 10.0f, 0.1f), 5.0f, numText, ""));
    l.add (param (id::perMix,      "Perach Mix",      Range (0.0f, 1.0f, 0.01f), MISHKAL_DEF_PER_MIX, pctText, ""));

    // Tape (just before the limiter)
    onOff (id::tapeOn, "Tape On", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::tapeStyle, 1 }, "Tape Speed", tapeStyleNames, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::tapeWidth, 1 }, "Tape Width", tapeWidthNames, 2));
    onOff (id::tapeNoise, "Tape Noise", false);
    const juce::StringArray osNames { "Off", "2x", "4x" };
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::tapeOs, 1 }, "Tape Oversampling", osNames, 1));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::limOs, 1 }, "Limiter Oversampling", osNames, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::globalOs, 1 }, "Oversampling (whole plugin)", osNames, 0));
    l.add (param (id::tapeDrive, "Tape Drive", Range (0.0f, 1.0f, 0.01f), 0.5f, pctText, ""));
    l.add (param (id::tapeMix,   "Tape Mix",   Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, ""));
    // Esh: the character tape saturator that can take the tape machine's place
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::tapeModel, 1 }, "Tape Model", juce::StringArray { "Slil", "Esh" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::eshChar, 1 }, "Esh Character",
                                                         juce::StringArray { "Clear", "Warm", "Round", "Glow", "Ember" }, 0));
    l.add (param (id::eshProcess, "Esh Process", Range (0.0f, 100.0f, 1.0f), 40.0f, numText, ""));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::eshTone, 1 }, "Esh Tone", juce::StringArray { "Warm", "Flat", "Bright" }, 1));
    l.add (param (id::eshIn,  "Esh Input",  Range (-10.0f, 10.0f, 0.1f), 0.0f, dbText, "dB"));
    l.add (param (id::eshOut, "Esh Output", Range (-6.0f, 6.0f, 0.1f),   0.0f, dbText, "dB"));
    // V60: tilt the tape toward the mid (-) or the sides (+); 0 = plain stereo tape
    l.add (std::make_unique<APF> (juce::ParameterID { id::tapeMsDrive, 1 }, "Tape M/S Drive", Range (-1.0f, 1.0f, 0.01f), 0.0f,
           juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
           {
               const int pc = juce::roundToInt (std::abs (v) * 100.0f);
               return pc == 0 ? juce::String ("Stereo") : (v < 0.0f ? "Mid " : "Side ") + juce::String (pc) + "%";
           })));

    // Master bypass (also what the DAW's own bypass button controls)
    onOff (id::masterBypass, "Bypass", false);
    onOff (id::levelMatch, "Level Match", false);

    // Limiter
    onOff (id::limOn, "Limiter On", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::limMode, 1 }, "Limiter Mode", limModeNames, 0));
    l.add (param (id::limThresh,  "Limiter Threshold", Range (-24.0f, 0.0f, 0.1f), 0.0f,  dbText, "dB"));
    l.add (param (id::limCeiling, "Limiter Ceiling", Range (-12.0f, 0.0f, 0.1f),    -1.0f, dbText, "dB"));
    l.add (param (id::limRelease, "Limiter Release", skewed (1.0f, 500.0f, 60.0f),  60.0f, msText, "ms"));
    l.add (param (id::limMix, "Limiter Mix", Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, ""));
    onOff (id::limArc, "Limiter Auto Release", true);
    // V61: channel link (like Pro-L's): 100% = both sides always get the same gain
    l.add (param (id::limLink, "Limiter Channel Link", Range (0.0f, 1.0f, 0.01f), 1.0f, pctText, ""));
    // V62: true-peak detection and a clipper in front of the limiter
    onOff (id::limTp, "Limiter True Peak", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::limClip, 1 }, "Limiter Clipper",
                                                         juce::StringArray { "Off", "1 dB", "2 dB", "3 dB", "4 dB", "6 dB" }, 0));
    // V63: stereo width on the way out (widening keeps the bass in the middle)
    l.add (param (id::outWidth, "Stereo Width", Range (0.0f, 2.0f, 0.01f), 1.0f, pctText, ""));
    // V64: the compressors / the de-esser can listen to the DAW's sidechain input instead of the track itself
    onOff (id::compScExt, "Compressor External Sidechain", false);
    onOff (id::dsScExt, "De-ess External Sidechain", false);
    // V65: listen to what the compressors / the limiter change
    onOff (id::compDelta, "Compressor Delta", false);
    onOff (id::limDelta, "Limiter Delta", false);
    // V68: the order of Char EQ / Dynamics / De-Ess / Tone / Tape (0 = the standard order; 1..120 = a custom one)
    l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id::midOrder, 1 }, "Module Order", 0, 120, 0));
    // V69: the Max limiter engine (styles in the spirit of the Ozone Maximizer's IRC modes)
    onOff (id::limMax, MISHKAL_VERSION >= 71 ? "Limiter Goliath XL" : "Limiter Max Engine", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::limMaxStyle, 1 }, MISHKAL_VERSION >= 71 ? "Limiter XL Style" : "Limiter Max Style", limMaxStyleNames, 3));
    l.add (param (id::limCharacter, "Limiter Character", Range (0.0f, 10.0f, 0.1f), 5.0f, numText, ""));
    l.add (param (id::limTransients, "Limiter Transient Emphasis", Range (0.0f, 1.0f, 0.01f), 0.0f, pctText, ""));
    l.add (param (id::limUpward, "Limiter Upward Compression", Range (0.0f, 12.0f, 0.1f), 0.0f, dbText, ""));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id::limSoft, 1 }, "Limiter Soft Saturation",
                                                         juce::StringArray { "Off", "2 dB", "3 dB", "4 dB", "6 dB" }, 0));

    return l;
}

VoxStripProcessor::VoxStripProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                         #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 64
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)   // V64: external key, off until routed
                         #endif
                          ),
      apvts (*this, &undoManager, "VoxStrip", createLayout())
{
    // Factory presets (FactoryPresets.cpp): anything a preset leaves out stays at its default
    presets.setFactoryPresets (makeFactoryPresets());

    auto f = [this] (const char* pid, float ChainSettings::* dst) { floatBindings.push_back ({ apvts.getRawParameterValue (pid), dst }); };
    auto b = [this] (const char* pid, bool  ChainSettings::* dst) { boolBindings.push_back  ({ apvts.getRawParameterValue (pid), dst }); };

    f (id::inputGain, &ChainSettings::inputDb);   f (id::outputGain, &ChainSettings::outputDb);
    b (id::cleanOn, &ChainSettings::cleanOn);
    b (id::peakCatch, &ChainSettings::peakCatch);
    f (id::peakIn, &ChainSettings::peakInDb);     f (id::peakOut, &ChainSettings::peakOutDb);
    b (id::peakLink, &ChainSettings::peakLink);
    b (id::tapeNoise, &ChainSettings::tapeNoise);
    b (id::trackMode, &ChainSettings::trackMode);
    b (id::eqOn, &ChainSettings::eqOn);
    f (id::limMix, &ChainSettings::limMix);
    f (id::gateThresh, &ChainSettings::gateThreshDb);
    f (id::breathAmount, &ChainSettings::breathDb);
    f (id::riderRange, &ChainSettings::riderRangeDb);
    b (id::compOn, &ChainSettings::compOn);
    b (id::dsOn, &ChainSettings::dsOn);           b (id::dsAuto, &ChainSettings::dsAuto);
    b (id::flowOn, &ChainSettings::flowOn);
    b (id::tapeOn, &ChainSettings::tapeOn);
    f (id::tapeDrive, &ChainSettings::tapeDrive); f (id::tapeMix, &ChainSettings::tapeMix);
    f (id::flowAmount, &ChainSettings::flowAmount); f (id::flowMix, &ChainSettings::flowMix);           b (id::dsListen, &ChainSettings::dsListen);
    f (id::dsFreq, &ChainSettings::dsHz);         f (id::dsThresh, &ChainSettings::dsThreshDb); f (id::dsRange, &ChainSettings::dsRangeDb);
    b (id::limOn, &ChainSettings::limOn);
    b (id::limArc, &ChainSettings::limArc);
    f (id::limThresh, &ChainSettings::limThreshDb); f (id::limCeiling, &ChainSettings::limCeilingDb); f (id::limRelease, &ChainSettings::limReleaseMs);

    {
        auto raw = [&] (const char* field) { return apvts.getRawParameterValue (charEqId (0, field)); };
        p1Params = { raw ("On"), raw ("LowFreq"), raw ("LowBoost"), raw ("LowAtten"), raw ("HighFreq"),
                     raw ("HighBw"), raw ("HighBoost"), raw ("AttenFreq"), raw ("HighAtten") };
    }
    for (int t = 0; t < eqTypeNames.size(); ++t) charEqPlaceParams[(size_t) t] = apvts.getRawParameterValue (charEqId (t, "Place"));   // V71
   #if MISHKAL_VERSION >= 74
    for (int m = 0; m < 4; ++m)
        for (auto* f : v74EqFields[m]) if (f != nullptr)
            v74EqParams[(size_t) m].push_back (apvts.getRawParameterValue (charEqId (4 + m, f)));
    eqOrder8Param = apvts.getRawParameterValue (id::eqOrder8);
   #endif
    for (int t = 1; t < 4; ++t)
    {
        auto raw = [&] (const char* field) { return apvts.getRawParameterValue (charEqId (t, field)); };
        charEqParams[(size_t) t] = { raw ("On"), raw ("HpfFreq"), raw ("BodyFreq"), raw ("BodyGain"), raw ("MudFreq"), raw ("MudGain"),
                                     raw ("MudQ"), raw ("PresFreq"), raw ("PresGain"), raw ("PresQ"), raw ("AirFreq"), raw ("AirGain"),
                                     t == (int) EqType::Airy ? raw ("Silk") : nullptr };
    }
    for (int u = 0; u < 4; ++u)
    {
        auto raw = [&] (const char* fld) { return apvts.getParameter (compId (u, fld)) != nullptr ? apvts.getRawParameterValue (compId (u, fld)) : nullptr; };
        compParams[(size_t) u] = { raw ("On"), raw ("Thresh"), raw ("Ratio"), raw ("Attack"), raw ("Release"), raw ("Makeup"), raw ("Mix"), raw ("Tc"),
                                   raw ("Arc"), raw ("Behavior"), raw ("Character"), raw ("Rev"), raw ("Model"), raw ("VoxLink") };
        if (u == (int) CompType::VariMu660) staParams = { raw ("StaIn"), raw ("StaMode"), raw ("StaRel") };
        if (u == (int) CompType::StaLevel)  staParams4 = { raw ("StaIn"), raw ("StaMode"), raw ("StaRel") };
    }
   #if MISHKAL_VERSION >= 75
    for (int m = 0; m < 5; ++m)
        for (auto* fld : v75CompFields[m]) if (fld != nullptr)
            v75CompParams[(size_t) m].push_back (apvts.getRawParameterValue (compId (4 + m, fld)));
    compOrder9Param = apvts.getRawParameterValue (id::compOrder9);
   #endif
    limModeParam  = apvts.getRawParameterValue (id::limMode);
    dsModeParam   = apvts.getRawParameterValue (id::dsMode);
    flowStyleParam = apvts.getRawParameterValue (id::flowStyle);
    tapeStyleParam = apvts.getRawParameterValue (id::tapeStyle);
    tapeWidthParam = apvts.getRawParameterValue (id::tapeWidth);
    compScHpfParam = apvts.getRawParameterValue (id::compScHpf);
    limSoftParam   = apvts.getRawParameterValue (id::limSoft);
    tapeOsParam    = apvts.getRawParameterValue (id::tapeOs);
    limOsParam     = apvts.getRawParameterValue (id::limOs);
    cleanPhaseParam = apvts.getRawParameterValue (id::cleanPhase);
    cleanOsParam    = apvts.getRawParameterValue (id::cleanOs);
    eqOrderParam     = apvts.getRawParameterValue (id::eqOrder);
    bypassParam      = apvts.getRawParameterValue (id::masterBypass);
    trackModeParam   = apvts.getRawParameterValue (id::trackMode);
    globalOsParam    = apvts.getRawParameterValue (id::globalOs);
    flowModelParam   = apvts.getRawParameterValue (id::flowModel);
    {
        const char* ids[] { id::tapeModel, id::eshChar, id::eshProcess, id::eshTone, id::eshIn, id::eshOut };
        for (size_t i = 0; i < 6; ++i) eshParams[i] = apvts.getRawParameterValue (ids[i]);
    }
    {
        const char* ids[] { id::perAmount, id::perLow, id::perBody, id::perPresence, id::perAir, id::perSpeed, id::perMix };
        for (size_t i = 0; i < 7; ++i) perParams[i] = apvts.getRawParameterValue (ids[i]);
    }
    levelMatchParam  = apvts.getRawParameterValue (id::levelMatch);
    compOrderParam   = apvts.getRawParameterValue (id::compOrder);
    compOrder4Param  = apvts.getRawParameterValue (id::compOrder4);
    eqAfterCompParam = apvts.getRawParameterValue (id::eqAfterComp);

    {
        auto raw = [&] (const char* pid) { return apvts.getRawParameterValue (pid); };
        resoParams = { raw (id::resoOn), raw (id::resoDepth), raw (id::resoSharp), raw (id::resoSelect), raw (id::resoAttack),
                       raw (id::resoRelease), raw (id::resoLow), raw (id::resoHigh), raw (id::resoMix), raw (id::resoDelta),
                       raw (id::resoMs), raw (id::resoSideDepth) };
        tapeMsDriveParam = raw (id::tapeMsDrive);
        resoLinkParam = raw (id::resoLink);
        limLinkParam  = raw (id::limLink);
        limTpParam    = raw (id::limTp);
        limClipParam  = raw (id::limClip);
        widthParam    = raw (id::outWidth);
        compScExtParam = raw (id::compScExt);
        dsScExtParam   = raw (id::dsScExt);
        compDeltaParam = raw (id::compDelta);
        limDeltaParam  = raw (id::limDelta);
        midOrderParam  = raw (id::midOrder);
        limMaxParams[0] = raw (id::limMax); limMaxParams[1] = raw (id::limMaxStyle);
        limMaxParams[2] = raw (id::limCharacter); limMaxParams[3] = raw (id::limTransients);
        limUpwardParam = raw (id::limUpward);
    }

    for (int bi = 0; bi < numCleanBands; ++bi)
    {
        auto raw = [&] (const char* field) { return apvts.getRawParameterValue (bandId (bi, field)); };
        bandParams[(size_t) bi] = { raw ("On"), raw ("Type"), raw ("Freq"), raw ("Gain"), raw ("Q"),
                                    raw ("Slope"), raw ("Dyn"), raw ("DynThresh"), raw ("DynRange"), raw ("Place") };
    }
}

ChainSettings VoxStripProcessor::readSettings() const
{
    ChainSettings s;
    for (auto& fb : floatBindings) s.*(fb.dst) = fb.src->load();
    for (auto& bb : boolBindings)  s.*(bb.dst) = bb.src->load() > 0.5f;
    {
        const auto& p = p1Params;
        auto& e = s.charEq[0];
        e.on          = p.on->load() > 0.5f;
        e.p1LowHz     = p1Hz (p1LowFreqs,   juce::roundToInt (p.lowFreq->load()));
        e.p1LowBoost  = p.lowBoost->load();
        e.p1LowAtten  = p.lowAtten->load();
        e.p1HighHz    = p1Hz (p1HighFreqs,  juce::roundToInt (p.highFreq->load()));
        e.p1HighBw    = p.highBw->load();
        e.p1HighBoost = p.highBoost->load();
        e.p1AttenHz   = p1Hz (p1AttenFreqs, juce::roundToInt (p.attenFreq->load()));
        e.p1HighAtten = p.highAtten->load();
    }
    for (int t = 1; t < 4; ++t)
    {
        const auto& p = charEqParams[(size_t) t];
        auto& e = s.charEq[(size_t) t];
        e = CharEqSettings { p.on->load() > 0.5f, p.hpf->load(), p.bodyF->load(), p.bodyG->load(), p.mudF->load(), p.mudG->load(), p.mudQ->load(),
              p.presF->load(), p.presG->load(), p.presQ->load(), p.airF->load(), p.airG->load(), p.silk != nullptr ? p.silk->load() : 0.0f };
    }
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 71
   #if MISHKAL_VERSION >= 74
    {
        auto v = [&] (int m, int i) { return v74EqParams[(size_t) m][(size_t) i]->load(); };
        auto c = [&] (const juce::StringArray& list, int m, int i) { return p1Hz (list, juce::roundToInt (v (m, i))); };
        auto& a = s.charEq[4];   // LE-A550: On Filter LowFreq LowGain LowShape LmFreq LmGain HmFreq HmGain HighFreq HighGain HighShape
        a.on = v (0, 0) > 0.5f; a.xHpfHz = v (0, 1) > 0.5f ? 50.0f : 0.0f;
        a.xHz = { c (apiLowFreqs, 0, 2), c (apiLmFreqs, 0, 5), c (apiHmFreqs, 0, 7), c (apiHighFreqs, 0, 9), 0, 0 };
        a.xDb = { v (0, 3), v (0, 6), v (0, 8), v (0, 10), 0, 0 };
        a.xShape = { juce::roundToInt (v (0, 4)), 0, 0, juce::roundToInt (v (0, 11)), 0, 0 };
        auto& k = s.charEq[5];   // LE-K9: On Hpf Lpf LfF LfG LfBell LmfF LmfG LmfQ HmfF HmfG HmfQ HfF HfG HfBell Mode
        k.on = v (1, 0) > 0.5f; k.xHpfHz = v (1, 1) > 16.5f ? v (1, 1) : 0.0f; k.xLpfHz = v (1, 2) < 21900.0f ? v (1, 2) : 0.0f;
        k.xHz = { v (1, 3), v (1, 6), v (1, 9), v (1, 12), 0, 0 };
        k.xDb = { v (1, 4), v (1, 7), v (1, 10), v (1, 13), 0, 0 };
        k.xQ  = { 1, v (1, 8), v (1, 11), 1, 1, 1 };
        k.xShape = { v (1, 5) > 0.5f ? 1 : 0, 0, 0, v (1, 14) > 0.5f ? 1 : 0, 0, 0 };
        k.xMode = juce::roundToInt (v (1, 15));
        auto& h = s.charEq[6];   // LE-H69: On BassFreq BassGain MidFreq MidGain TrebleGain
        h.on = v (2, 0) > 0.5f;
        h.xHz = { c (heliosBassFreqs, 2, 1), c (heliosMidFreqs, 2, 3), 10000.0f, 0, 0, 0 };
        h.xDb = { v (2, 2), v (2, 4), v (2, 5), 0, 0, 0 };
        auto& q = s.charEq[7];   // LE-M4: On Sub G40 G160 G650 G2k5 AirFreq AirGain
        q.on = v (3, 0) > 0.5f;
        q.xDb = { v (3, 1), v (3, 2), v (3, 3), v (3, 4), v (3, 5), v (3, 7) };
        q.xHz = { 20.0f, 40.0f, 160.0f, 650.0f, 2500.0f, c (maagAirFreqs, 3, 6) };
    }
   #endif
    for (int t = 0; t < eqTypeNames.size(); ++t) s.charEq[(size_t) t].place = juce::jlimit (0, 2, juce::roundToInt (charEqPlaceParams[(size_t) t]->load()));   // V71
   #endif
    for (size_t u = 0; u < 4; ++u)
    {
        const auto& p = compParams[u];
        auto& c = s.comps[u];
        auto get = [] (std::atomic<float>* a, float fallback) { return a != nullptr ? a->load() : fallback; };
        c.on        = p.on->load() > 0.5f;
        c.type      = (CompType) u;
        c.threshDb  = p.thresh->load();
        c.ratio     = get (p.ratio, 4.0f);
        c.attackMs  = get (p.attack, 0.8f);
        c.releaseMs = get (p.release, 120.0f);
        c.makeupDb  = p.makeup->load();
        c.mix       = p.mix->load();
        c.timeConstant = juce::jlimit (0, 5, juce::roundToInt (get (p.tc, 1.0f)));
        c.arc       = get (p.arc, 1.0f) > 0.5f;
        c.behavior  = juce::jlimit (0, 1, juce::roundToInt (get (p.behavior, 0.0f)));
        c.character = juce::jlimit (0, 1, juce::roundToInt (get (p.character, 0.0f)));
        c.fetRev    = juce::jlimit (0, 2, juce::roundToInt (get (p.rev, 0.0f)));
        if (u == (size_t) CompType::StaLevel)
        {
            c.staInDb = get (staParams4[0], 0.0f);
            c.staMode = juce::jlimit (0, 2, juce::roundToInt (get (staParams4[1], 1.0f)));
            c.staRel  = juce::jlimit (0, 3, juce::roundToInt (get (staParams4[2], 1.0f)));
           #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 52
           #else
            c.on = false;   // before V52 there are only three compressors
           #endif
        }
        else if (u == (size_t) CompType::VariMu660)
        {
           #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 52
            c.tubeModel = 0;   // V52: the 660 is only the 660 (LE-STA is its own unit)
           #else
            c.tubeModel = juce::jlimit (0, 1, juce::roundToInt (get (p.model, 0.0f)));
           #endif
            c.staInDb   = get (staParams[0], 0.0f);
            c.staMode   = juce::jlimit (0, 2, juce::roundToInt (get (staParams[1], 1.0f)));
            c.staRel    = juce::jlimit (0, 3, juce::roundToInt (get (staParams[2], 1.0f)));
        }
        else
        c.rcModel   = juce::jlimit (0, 1, juce::roundToInt (get (p.model, 0.0f)));
        c.voxLink   = get (p.voxLink, 1.0f) > 0.5f;
    }
    s.limMode  = (LimMode)  juce::jlimit (0, 1, juce::roundToInt (limModeParam->load()));
    s.flowStyle = juce::jlimit (0, numFlowStyles - 1, juce::roundToInt (flowStyleParam->load()));
    s.flowModel = juce::jlimit (0, 1, juce::roundToInt (flowModelParam->load()));
    s.tapeModel     = juce::jlimit (0, 1, juce::roundToInt (eshParams[0]->load()));
    s.esh.character = juce::jlimit (0, 4, juce::roundToInt (eshParams[1]->load()));
    s.esh.process   = eshParams[2]->load();
    s.esh.tone      = juce::jlimit (0, 2, juce::roundToInt (eshParams[3]->load()));
    s.esh.inDb      = eshParams[4]->load();
    s.esh.outDb     = eshParams[5]->load();
    s.perach.amount = perParams[0]->load();
    for (size_t t = 0; t < 4; ++t) s.perach.tone[t] = perParams[1 + t]->load();
    s.perach.speed = perParams[5]->load();
    s.perach.mix   = perParams[6]->load();
    s.tapeStyle = juce::jlimit (0, numTapeStyles - 1, juce::roundToInt (tapeStyleParam->load()));
    s.tapeWidth = juce::jlimit (0, numTapeWidths - 1, juce::roundToInt (tapeWidthParam->load()));
    s.compScHpf = juce::jlimit (0, 3, juce::roundToInt (compScHpfParam->load()));
    s.limSoft   = juce::jlimit (0, 4, juce::roundToInt (limSoftParam->load()));
    s.tapeOs    = juce::jlimit (0, 2, juce::roundToInt (tapeOsParam->load()));
    s.limOs     = juce::jlimit (0, 2, juce::roundToInt (limOsParam->load()));
    s.cleanPhase = juce::jlimit (0, 2, juce::roundToInt (cleanPhaseParam->load()));
    s.cleanOs    = juce::jlimit (0, 2, juce::roundToInt (cleanOsParam->load()));
    if (globalOsParam != nullptr && globalOsParam->load() > 0.5f)   // whole-plugin oversampling replaces the per-module kind
        s.tapeOs = s.limOs = s.cleanOs = 0;
    {
        const auto o4 = permutationFromIndex<4> (juce::jlimit (0, 23, juce::roundToInt (eqOrderParam->load())));
        for (int i = 0; i < 4; ++i) s.eqOrder[(size_t) i] = o4[(size_t) i];
        for (int i = 4; i < numCharEqs; ++i) s.eqOrder[(size_t) i] = i;
       #if MISHKAL_VERSION >= 74
        if (const int o8 = juce::roundToInt (eqOrder8Param->load()); o8 > 0)   // V74: the order of all eight (0 = the four's order, then the new ones)
            s.eqOrder = permutationFromIndexFast<8> (juce::jlimit (0, 40319, o8));
       #endif
    }
   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 52
    {
        const auto o4 = permutationFromIndex<4> (juce::jlimit (0, 23, juce::roundToInt (compOrder4Param->load())));
        for (int i = 0; i < 4; ++i) s.compOrder[(size_t) i] = o4[(size_t) i];
        for (int i = 4; i < numComps; ++i) s.compOrder[(size_t) i] = i;
       #if MISHKAL_VERSION >= 75
        if (const int o9 = juce::roundToInt (compOrder9Param->load()); o9 > 0)   // V75: the order of all nine (0 = the four's order, then the new ones)
            s.compOrder = permutationFromIndexFast<9> (juce::jlimit (0, 362879, o9));
        {
            auto v = [&] (int m, int i) { return v75CompParams[(size_t) m][(size_t) i]->load(); };
            auto ix = [&] (int m, int i) { return juce::roundToInt (v (m, i)); };
            auto& a = s.comps[4];   // LE-2500: On Thresh Ratio Attack Release Makeup Mix Knee Thrust Type
            a.on = v (0, 0) > 0.5f; a.threshDb = v (0, 1); a.ratio = v (0, 2); a.releaseMs = v (0, 4); a.makeupDb = v (0, 5); a.mix = v (0, 6);
            a.sw = { ix (0, 7), ix (0, 8), ix (0, 9), ix (0, 3) };
            auto& g = s.comps[5];   // LE-G: On Thresh Ratio Attack Release Makeup Mix
            g.on = v (1, 0) > 0.5f; g.threshDb = v (1, 1); g.ratio = MasterCompressor::sslRatio (ix (1, 2));
            g.attackMs = MasterCompressor::sslAttackMs (ix (1, 3)); g.releaseMs = MasterCompressor::sslReleaseMs (ix (1, 4));
            g.makeupDb = v (1, 5); g.mix = v (1, 6);
            auto& w = s.comps[6];   // LE-DS1: On Thresh Ratio Attack Release Makeup Mix Knee Detect RelMode
            w.on = v (2, 0) > 0.5f; w.threshDb = v (2, 1); w.ratio = v (2, 2); w.attackMs = v (2, 3); w.releaseMs = v (2, 4);
            w.makeupDb = v (2, 5); w.mix = v (2, 6); w.sw = { ix (2, 7), ix (2, 8), ix (2, 9), 0 };
            auto& r = s.comps[7];   // LE-IRON: On Thresh Attack Release Makeup Mix Bias Rect Airbass
            static constexpr float biasRatio[4] { 1.5f, 2.0f, 3.0f, 4.0f };
            r.on = v (3, 0) > 0.5f; r.threshDb = v (3, 1); r.attackMs = MasterCompressor::ironAttackMs (ix (3, 2));
            r.releaseMs = MasterCompressor::ironReleaseMs (ix (3, 3)); r.makeupDb = v (3, 4); r.mix = v (3, 5);
            r.sw = { ix (3, 6), ix (3, 7), ix (3, 8), 0 }; r.ratio = biasRatio[juce::jlimit (0, 3, ix (3, 6))];
            auto& nv = s.comps[8];  // LE-336: On Thresh Ratio Release LimThresh LimRel LimAtt Makeup Mix
            nv.on = v (4, 0) > 0.5f; nv.threshDb = v (4, 1); nv.ratio = MasterCompressor::neveRatio (ix (4, 2));
            nv.releaseMs = MasterCompressor::neveCompRecMs (ix (4, 3)); nv.limThreshDb = v (4, 4);
            nv.sw = { ix (4, 5), ix (4, 6), 0, 0 }; nv.makeupDb = v (4, 7); nv.mix = v (4, 8);
        }
       #endif
    }
   #else
    {
        const auto o3 = permutationFromIndex<3> (juce::jlimit (0, 5, juce::roundToInt (compOrderParam->load())));
        s.compOrder = { o3[0], o3[1], o3[2], 3, 4, 5, 6, 7, 8 };   // the fourth unit (LE-STA) doesn't exist yet: always last, always off
    }
   #endif
    s.eqAfterComp = eqAfterCompParam->load() > 0.5f;
    s.dsMode   = (DsMode)   juce::jlimit (0, 1, juce::roundToInt (dsModeParam->load()));

    {
        const auto& r = resoParams;
        s.reso = { r.on->load() > 0.5f, r.depth->load(), r.sharp->load(), r.select->load(), r.attack->load(),
                   r.release->load(), r.low->load(), r.high->load(), r.mix->load(), r.delta->load() > 0.5f };
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 60
        s.msEnabled = true;   // V60: mid/side options (they only act on stereo tracks)
        s.reso.msEnabled = true;
        s.reso.msMode    = juce::jlimit (0, 3, juce::roundToInt (r.ms->load()));
        s.reso.sideDepth = r.sideDepth->load();
        s.tapeMsDrive    = tapeMsDriveParam->load();
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 61
        s.reso.link = resoLinkParam->load();
        s.limLink   = limLinkParam->load();
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 62
        s.limTp   = limTpParam->load() > 0.5f;
        s.limClip = juce::jlimit (0, 5, juce::roundToInt (limClipParam->load()));
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 63
        s.width = widthParam->load();
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 64
        s.compScExt = compScExtParam->load() > 0.5f;
        s.dsScExt   = dsScExtParam->load() > 0.5f;
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 65
        s.compDelta = compDeltaParam->load() > 0.5f;
        s.limDelta  = limDeltaParam->load() > 0.5f;
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 68
        if (const int mo = juce::jlimit (0, 120, juce::roundToInt (midOrderParam->load())); mo > 0)
        {
            s.midOrderCustom = true;
            s.midOrder = permutationFromIndex<5> (mo - 1);
        }
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 69
        s.limMax        = limMaxParams[0]->load() > 0.5f;
        s.limMaxStyle   = juce::jlimit (0, limMaxStyleNames.size() - 1, juce::roundToInt (limMaxParams[1]->load()));
        s.limCharacter  = limMaxParams[2]->load();
        s.limTransients = limMaxParams[3]->load();
       #endif
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 70
        s.limUpward = limUpwardParam->load();
       #endif
    }

    for (int bi = 0; bi < numCleanBands; ++bi)
    {
        const auto& p = bandParams[(size_t) bi];
        auto& b = s.clean[(size_t) bi];
        b.on    = p.on->load() > 0.5f;
        b.type  = (BandType) juce::jlimit (0, 5, juce::roundToInt (p.type->load()));
        b.freq  = p.freq->load();
        b.gain  = p.gain->load();
        b.q     = p.q->load();
        b.slope = juce::jlimit (0, 3, juce::roundToInt (p.slope->load()));
        b.dyn   = p.dyn->load() > 0.5f;
        b.dynThresh = p.thresh->load();
        b.dynRange  = p.range->load();
       #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 60
        b.place     = juce::jlimit (0, 2, juce::roundToInt (p.place->load()));
       #endif
    }
    return s;
}

int VoxStripProcessor::wantedOs (const vox::ChainSettings& s) const
{
    if (s.trackMode || globalOsParam == nullptr) return 0;   // tracking needs zero latency
    return juce::jlimit (0, 2, juce::roundToInt (globalOsParam->load()));
}

int VoxStripProcessor::totalLatency (const vox::ChainSettings& s) const
{
    const int os = activeOs.load();
    const int chainLat = chain.latencyFor (s);
    if (os == 0) return chainLat;
    const auto& o = os == 1 ? *os2 : *os4;
    return juce::roundToInt (o.getLatencyInSamples()) + (chainLat + (1 << os) - 1) / (1 << os);
}

void VoxStripProcessor::prepareChain (int os)
{
    os = juce::jlimit (0, 2, os);
    const int factor = 1 << os;
    if (os == 0) chain.prepare (baseRate, baseBlock);
    else         chain.prepare (baseRate * factor, juce::jmin (baseBlock, osChunk) * factor);
    chainRate = baseRate * factor;
    if (os2 != nullptr) os2->reset();
    if (os4 != nullptr) os4->reset();
    activeOs = os;

    const int lat = totalLatency (readSettings());
    reportedLatency = lat;
    setLatencySamples (lat);

    // bypass delay line: long enough for the largest latency at any setting; read at whichever latency is active
    dryDelay.setSize (2, chain.maxLatency() + 4096 + 1);
    dryDelay.clear();
    dryDelayPos = 0;
}

void VoxStripProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseRate = sampleRate;
    baseBlock = juce::jmax (1, samplesPerBlock);
    using OS = juce::dsp::Oversampling<float>;
    os2 = std::make_unique<OS> (2, 1, OS::filterHalfBandFIREquiripple, true, true);
    os4 = std::make_unique<OS> (2, 2, OS::filterHalfBandFIREquiripple, true, true);
    os2->initProcessing ((size_t) juce::jmin (baseBlock, osChunk));
    os4->initProcessing ((size_t) juce::jmin (baseBlock, osChunk));
    pendingOs = -1;
    prepareChain (wantedOs (readSettings()));
    dryBlock.setSize (2, juce::jmax (1, samplesPerBlock));
    keyUp.setSize (2, juce::jmin (juce::jmax (1, samplesPerBlock), osChunk) * 4 + 8);   // V64: the oversampled key
    bypassMix.reset (sampleRate, 0.03);

    // K-weighting (ITU-R BS.1770): a +4 dB high shelf around 1.7 kHz and a 38 Hz high-pass
    const vox::KWeighting kw (sampleRate);
    for (int i = 0; i < 2; ++i)
    {
        kShelf[(size_t) i].coefficients = kw.shelf;       kShelf[(size_t) i].reset();
        kHighPass[(size_t) i].coefficients = kw.highPass; kHighPass[(size_t) i].reset();
    }
    msDry = msWet = 0.0;
    loudness.prepare (sampleRate);
    matchGain.reset (sampleRate, 0.05);
    matchGain.setCurrentAndTargetValue (1.0f);
    bypassMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 1.0f : 0.0f);
}

void VoxStripProcessor::handleAsyncUpdate()
{
    const int want = pendingOs.exchange (-1);
    if (want >= 0 && want != activeOs.load())
    {
        suspendProcessing (true);    // waits for the audio thread to leave processBlock
        prepareChain (want);
        suspendProcessing (false);
        return;                      // prepareChain told the DAW the new latency
    }
    setLatencySamples (reportedLatency.load());
}

bool VoxStripProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.inputBuses.size() > 1)   // V64: the sidechain may be off, mono or stereo
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return layouts.getMainInputChannelSet() == out;
}

void VoxStripProcessor::processBlock (juce::AudioBuffer<float>& fullBuffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // V64: the host buffer can also carry the sidechain channels; the strip itself only ever sees the main bus
    if (fullBuffer.getNumChannels() == 0 || fullBuffer.getNumSamples() == 0) return;
    const int mainCh = juce::jlimit (0, fullBuffer.getNumChannels(),
                                     juce::jmax (getMainBusNumInputChannels(), getMainBusNumOutputChannels()));
    juce::AudioBuffer<float> buffer (fullBuffer.getArrayOfWritePointers(), juce::jmax (1, juce::jmin (mainCh, fullBuffer.getNumChannels())),
                                     fullBuffer.getNumSamples());
    const juce::AudioBuffer<float>* key = nullptr;
    juce::AudioBuffer<float> keyBuf;
    if (getBusCount (true) > 1 && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled())
    {
        const int first = getChannelIndexInProcessBlockBuffer (true, 1, 0);
        const int count = getBus (true, 1)->getNumberOfChannels();
        if (count > 0 && first >= 0 && first + count <= fullBuffer.getNumChannels())   // only when the host really sent it
        {
            keyBuf.setDataToReferTo (fullBuffer.getArrayOfWritePointers() + first, count, fullBuffer.getNumSamples());
            key = &keyBuf;
        }
    }

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    const int nc = juce::jmin (2, buffer.getNumChannels());
    if (n == 0 || nc == 0) return;
    if (dryBlock.getNumSamples() < n)
        dryBlock.setSize (2, n, false, false, true);   // only if the host sends a bigger block than promised

    const auto settings = readSettings();

    // Whole-plugin oversampling changed: re-prepare on the message thread (keep running at the old rate till then)
    if (const int want = wantedOs (settings); want != activeOs.load() && pendingOs.load() != want)
    {
        pendingOs = want;
        triggerAsyncUpdate();
    }

    // Tracking mode or oversampling changed: tell the DAW the new latency (from the message thread)
    const int wantLatency = totalLatency (settings);
    if (wantLatency != reportedLatency.load())
    {
        reportedLatency = wantLatency;
        triggerAsyncUpdate();
    }

    // Always run the dry signal through the latency delay, so it is ready the moment you hit Bypass
    const int len = dryDelay.getNumSamples();
    const int lat = juce::jlimit (0, len - 1, wantLatency);
    for (int c = 0; c < 2; ++c)
    {
        const auto* in = buffer.getReadPointer (juce::jmin (c, nc - 1));
        auto* line = dryDelay.getWritePointer (c);
        auto* out = dryBlock.getWritePointer (c);
        int pos = dryDelayPos;
        for (int i = 0; i < n; ++i)
        {
            line[pos] = in[i];
            int rd = pos - lat; if (rd < 0) rd += len;
            out[i] = line[rd];
            if (++pos >= len) pos = 0;
        }
    }
    dryDelayPos = (dryDelayPos + n) % len;

    // The chain always runs, so un-bypassing is seamless (no stale sound inside its buffers)
    if (const int os = activeOs.load(); os > 0)
    {
        // whole-plugin oversampling: up, run the entire chain at 2x / 4x, back down (in chunks the oversampler was sized for)
        auto& o = os == 1 ? *os2 : *os4;
        const int step = juce::jmin (osChunk, baseBlock);   // what the oversampler and the chain were prepared for
        for (int start = 0; start < n; start += step)
        {
            const int len = juce::jmin (step, n - start);
            juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nc, (size_t) start, (size_t) len);
            auto up = o.processSamplesUp (block);
            float* chans[2] { up.getChannelPointer (0), up.getChannelPointer ((up.getNumChannels() > 1 ? (size_t) 1 : (size_t) 0)) };
            juce::AudioBuffer<float> hi (chans, nc, (int) up.getNumSamples());
            if (key != nullptr)   // V64: the key at the oversampled rate (held samples are plenty for a detector)
            {
                const int f = 1 << os, upN = (int) up.getNumSamples();
                const int kc = juce::jmin (2, key->getNumChannels());
                for (int c = 0; c < kc; ++c)
                {
                    auto* d = keyUp.getWritePointer (c);
                    const auto* k = key->getReadPointer (c, start);
                    for (int i = 0; i < upN; ++i) d[i] = k[juce::jmin (len - 1, i / f)];
                }
                keyUpView.setDataToReferTo (keyUp.getArrayOfWritePointers(), kc, upN);
                chain.setSidechain (&keyUpView);
            }
            else chain.setSidechain (nullptr);
            chain.process (hi, settings);
            o.processSamplesDown (block);
        }
    }
    else
    {
        chain.setSidechain (key);
        chain.process (buffer, settings);
    }
    chain.setSidechain (nullptr);

    // Level match: K-weighted loudness of the raw (delayed, so it lines up) and processed signal
    {
        double eDry = 0.0, eWet = 0.0;
        for (int i = 0; i < n; ++i)
        {
            float d = 0.0f, w = 0.0f;
            for (int c = 0; c < nc; ++c) { d += dryBlock.getSample (c, i); w += buffer.getSample (c, i); }
            d = kHighPass[0].processSample (kShelf[0].processSample (d / (float) nc));
            w = kHighPass[1].processSample (kShelf[1].processSample (w / (float) nc));
            eDry += (double) d * d;
            eWet += (double) w * w;
        }
        eDry /= n; eWet /= n;
        if (eDry > 1.0e-7)   // only while the vocal is actually playing (above about -70 dB)
        {
            const double a = std::exp (-(double) n / (3.0 * getSampleRate()));   // ~3 s average
            msDry = a * msDry + (1.0 - a) * eDry;
            msWet = a * msWet + (1.0 - a) * eWet;
        }
        const float diffDb = (msDry > 1.0e-12 && msWet > 1.0e-12)
                               ? (float) (10.0 * std::log10 (msWet / msDry)) : 0.0f;
        levelMatchDb.store (diffDb);
        const bool matchOn = levelMatchParam->load() > 0.5f;
        matchGain.setTargetValue (matchOn ? juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 24.0f, diffDb)) : 1.0f);
    }

    bypassMix.setTargetValue (bypassParam->load() > 0.5f ? 1.0f : 0.0f);
    if (! bypassMix.isSmoothing() && bypassMix.getCurrentValue() <= 0.0f)
    {
        matchGain.skip (n);
        loudness.process (buffer);   // LUFS of what actually leaves the plugin
        return;   // fully on: nothing to blend
    }

    for (int i = 0; i < n; ++i)
    {
        const float m = bypassMix.getNextValue();
        const float g = matchGain.getNextValue();   // only ever applied to the bypassed signal
        for (int c = 0; c < nc; ++c)
        {
            auto* w = buffer.getWritePointer (c);
            w[i] = w[i] * (1.0f - m) + dryBlock.getSample (c, i) * g * m;
        }
    }

    // the output meter shows what really leaves the plugin
    auto& mt = chain.meters;
    float peak = 0.0f;
    for (int c = 0; c < 2; ++c)
    {
        const float pk = buffer.getMagnitude (juce::jmin (c, nc - 1), 0, n);
        mt.outPeak[c].store (pk);
        peak = juce::jmax (peak, pk);
    }
    mt.outDb.store (juce::Decibels::gainToDecibels (peak, -100.0f));
    loudness.process (buffer);   // LUFS of what actually leaves the plugin
}

void VoxStripProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoxStripProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            presets.restoreName();
        }
}

juce::AudioProcessorEditor* VoxStripProcessor::createEditor() { return new VoxStripEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxStripProcessor(); }
