// Mishkal Goliath factory library: vocals, drums, bass, guitars, keys,
// orchestral, buses, mastering and creative effects.
//
// Every preset states each section it relies on (power switches included), because
// anything a preset leaves out snaps back to the defaults - and the defaults are
// set up for vocals (de-esser, Zerem and the console EQ's 80 Hz high-pass are on).

#include "FactoryPresets.h"
#include <map>

namespace vox
{
namespace
{
    using V = PresetManager::Values;

    struct B
    {
        V v;
        B& set (const juce::String& pid, float x) { v.push_back ({ pid, x }); return *this; }

        // Tahor EQ bands (0-7). Band 1 is the natural low cut, band 8 the natural high cut.
        B& band (int i, BandType t, float hz, float gain = 0.0f, float q = 1.0f, int slope = 1)
        {
            return set (bandId (i, "On"), 1).set (bandId (i, "Type"), (float) t).set (bandId (i, "Freq"), hz)
                  .set (bandId (i, "Gain"), gain).set (bandId (i, "Q"), q).set (bandId (i, "Slope"), (float) slope);
        }
        B& bell (int i, float hz, float gain, float q = 1.0f)  { return band (i, BandType::Bell, hz, gain, q); }
        B& dyn (int i, float threshDb, float rangeDb)          { return set (bandId (i, "Dyn"), 1).set (bandId (i, "DynThresh"), threshDb).set (bandId (i, "DynRange"), rangeDb); }
        B& lowCut (float hz, int slope = 1)                    { return band (0, BandType::LowCut, hz, 0.0f, 0.71f, slope); }
        B& highCut (float hz, int slope = 1)                   { return band (7, BandType::HighCut, hz, 0.0f, 0.71f, slope); }
        B& lowShelf (int i, float hz, float gain, float q = 0.7f)  { return band (i, BandType::LowShelf, hz, gain, q); }
        B& highShelf (int i, float hz, float gain, float q = 0.7f) { return band (i, BandType::HighShelf, hz, gain, q); }
        B& notch (int i, float hz, float q = 8.0f)             { return band (i, BandType::Notch, hz, 0.0f, q); }
        B& noTahor()                                           { return set (id::cleanOn, 0); }

        // Resonance suppressor
        B& reso (float depth, float lo = 300.0f, float hi = 12000.0f, float sharp = 0.4f, float select = 0.5f,
                 float attack = 5.0f, float release = 80.0f)
        {
           #if MISHKAL_VERSION >= 55
            depth = MISHKAL_DEF_RESO_DEPTH;   // V55: every preset starts Reso at 15% - gentle, dial in more if needed
           #endif
            return set (id::resoOn, 1).set (id::resoDepth, depth).set (id::resoLow, lo).set (id::resoHigh, hi)
                  .set (id::resoSharp, sharp).set (id::resoSelect, select).set (id::resoAttack, attack).set (id::resoRelease, release);
        }
        B& noReso() { return set (id::resoOn, 0); }

        // Character EQs: exactly one on (t = 0 LE-EP1, 1 LE-E4K, 2 LE-N81, 3 LE-SP1)
        B& charEq (int t, std::initializer_list<std::pair<const char*, float>> fields)
        {
            set (id::eqOn, 1);
            for (int i = 0; i < 4; ++i) set (charEqId (i, "On"), i == t ? 1.0f : 0.0f);
            for (auto& [f, x] : fields) set (charEqId (t, f), x);
            return *this;
        }
        // a second character EQ stacked on top of the first
        B& alsoEq (int t, std::initializer_list<std::pair<const char*, float>> fields)
        {
            set (charEqId (t, "On"), 1);
            for (auto& [f, x] : fields) set (charEqId (t, f), x);
            return *this;
        }
        B& noCharEq()    { set (id::eqOn, 0); for (int i = 0; i < 4; ++i) set (charEqId (i, "On"), 0); return *this; }
        B& eqAfterComp() { return set (id::eqAfterComp, 1); }

        // Compressors (0 = LE-76 FET, 1 = LE-RC Renaissance-style, 2 = LE-660 vari-mu)
        B& comps (std::initializer_list<int> active, std::initializer_list<std::tuple<int, const char*, float>> fields)
        {
            set (id::compOn, 1);
            for (int u = 0; u < 4; ++u)
                set (compId (u, "On"), std::find (active.begin(), active.end(), u) != active.end() ? 1.0f : 0.0f);
            // LE-RC: a musical starting point (3:1, 10 ms / 150 ms, AUTO release, Glide, Color) and a Gain that roughly
            // restores the level it takes away; the preset's own fields below override any of these.
            if (std::find (active.begin(), active.end(), 1) != active.end())
            {
                float thresh = -18.0f;
                for (auto& [u, f, x] : fields) if (u == 1 && juce::String (f) == "Thresh") thresh = x;
                set (compId (1, "Ratio"), 3.0f).set (compId (1, "Attack"), 10.0f).set (compId (1, "Release"), 150.0f)
                    .set (compId (1, "Arc"), 1).set (compId (1, "Behavior"), 0).set (compId (1, "Character"), 0)
                    .set (compId (1, "Makeup"), (float) juce::roundToInt (juce::jlimit (0.0f, 8.0f, -thresh * 0.3f)));
            }
            for (auto& [u, f, x] : fields) set (compId (u, f), x);
            return *this;
        }
        B& noComp()                 { set (id::compOn, 0); for (int u = 0; u < 4; ++u) set (compId (u, "On"), 0); return *this; }
        B& gate (float threshDb)    { return set (id::gateThresh, threshDb); }
        B& breath (float db)        { return set (id::breathAmount, db); }
        B& rider (float rangeDb)    { return set (id::riderRange, rangeDb); }
        B& scHpf (int choice)       { return set (id::compScHpf, (float) choice); }   // 1 = 80, 2 = 120, 3 = 160 Hz
        B& peakCatch()              { return set (id::peakCatch, 1); }

        // De-esser
        B& deEss (float hz, float threshDb, float rangeDb = 8.0f, int mode = 0, bool autoMode = false)
        {
            return set (id::dsOn, 1).set (id::dsFreq, hz).set (id::dsThresh, threshDb).set (id::dsRange, rangeDb)
                  .set (id::dsMode, (float) mode).set (id::dsAuto, autoMode ? 1.0f : 0.0f);
        }
        B& noDeEss() { return set (id::dsOn, 0); }

        // Zerem (styles: 0 Pop Vocal, 1 Rap Vocal, 2 Mid Forward, 3 R&B Smooth, 4 Airy Ballad, 5 Podcast,
        //        6 Dense, 7 Drums, 8 Bass, 9 Guitar, 10 Keys, 11 Mix Bus)
        B& zerem (int style, float amount = 0.5f, float mix = 1.0f)
        {
            return set (id::flowOn, 1).set (id::flowStyle, (float) style).set (id::flowAmount, amount).set (id::flowMix, mix);
        }
        B& noZerem() { return set (id::flowOn, 0); }

        // Tape (speed 0 = 30, 1 = 15, 2 = 7.5 ips; width 0 = 1/4", 1 = 1/2", 2 = 1", 3 = 2")
        B& tape (int speed, int width, float drive = 0.5f, float mix = 1.0f, bool noise = false)
        {
            return set (id::tapeOn, 1).set (id::tapeStyle, (float) speed).set (id::tapeWidth, (float) width)
                  .set (id::tapeDrive, drive).set (id::tapeMix, mix).set (id::tapeNoise, noise ? 1.0f : 0.0f);
        }

        // Limiter (mode 0 = Classic, 1 = Multi; soft 0 = off, 1 = 2 dB, 2 = 3 dB, 3 = 4 dB, 4 = 6 dB)
        B& lim (float threshDb, float ceilingDb = -1.0f, int mode = 0, int soft = 0, float mix = 1.0f, bool arc = true, float releaseMs = 60.0f)
        {
            return set (id::limOn, 1).set (id::limThresh, threshDb).set (id::limCeiling, ceilingDb).set (id::limMode, (float) mode)
                  .set (id::limSoft, (float) soft).set (id::limMix, mix).set (id::limArc, arc ? 1.0f : 0.0f).set (id::limRelease, releaseMs);
        }
        B& safety()  { return lim (0.0f, -0.3f); }   // just a safety net, no loudness change
        B& noLim()   { return set (id::limOn, 0); }

        // --- the newer modules ---------------------------------------------------------------
        // Perach (takes Zerem's place): amount 0-10, tone wants -6..+6 dB (low, body, presence, air), speed 0-10
        B& perach (float amount, float low, float body, float pres, float air, float speed = 5.0f, float mix = 1.0f)
        {
           #if MISHKAL_VERSION >= 55
            amount = MISHKAL_DEF_PER_AMOUNT; mix = MISHKAL_DEF_PER_MIX;   // V55: every preset starts Perach at 2 / 80% mix
           #endif
            return set (id::flowOn, 1).set (id::flowModel, 1).set (id::perAmount, amount).set (id::perLow, low).set (id::perBody, body)
                  .set (id::perPresence, pres).set (id::perAir, air).set (id::perSpeed, speed).set (id::perMix, mix);
        }
        // Esh (takes Slil's place): character 0 Clear, 1 Warm, 2 Round, 3 Glow, 4 Ember; tone 0 Warm, 1 Flat, 2 Bright
        B& esh (int character, float process, int tone = 1, float inDb = 0.0f, float outDb = 0.0f)
        {
            return set (id::tapeOn, 1).set (id::tapeModel, 1).set (id::eshChar, (float) character).set (id::eshProcess, process)
                  .set (id::eshTone, (float) tone).set (id::eshIn, inDb).set (id::eshOut, outDb);
        }
        // 1964 PK peak catcher with its own input / output (link: the output follows the input)
        B& pk (float inDb, float outDb = 0.0f, bool link = true)
        {
            return set (id::peakCatch, 1).set (id::peakIn, inDb).set (id::peakOut, outDb).set (id::peakLink, link ? 1.0f : 0.0f);
        }
        B& fetRev (int rev)              { return set (compId (0, "Rev"), (float) rev); }   // 0 Purple, 1 Blue Stripe, 2 Black
        // LE-RV: the one-knob vocal leveler in the LE-RC slot (link: the output follows the threshold)
        B& rv (float threshDb, bool link = true, float outDb = 0.0f)
        {
            return set (compId (1, "Model"), 1).set (compId (1, "Thresh"), threshDb).set (compId (1, "VoxLink"), link ? 1.0f : 0.0f)
                  .set (compId (1, "Makeup"), outDb);
        }

        // LE-STA in the tube slot (unit 2): input drive, mode 0 Single / 1 Double / 2 Triple, recovery 0 Fast .. 3 Very Slow
        B& sta (float inDb, int mode, int rel, float outDb, float mix = 1.0f)
        {
           #if MISHKAL_VERSION >= 52
            // V52: LE-STA is its own unit (3) and takes the place the tube slot had (the 660 switches out)
            return set (compId (2, "On"), 0).set (compId (3, "On"), 1).set (compId (3, "StaIn"), inDb).set (compId (3, "StaMode"), (float) mode)
                  .set (compId (3, "StaRel"), (float) rel).set (compId (3, "Makeup"), outDb).set (compId (3, "Mix"), mix);
           #endif
            return set (compId (2, "Model"), 1).set (compId (2, "StaIn"), inDb).set (compId (2, "StaMode"), (float) mode)
                  .set (compId (2, "StaRel"), (float) rel).set (compId (2, "Makeup"), outDb).set (compId (2, "Mix"), mix);
        }

        // V60 mid/side: a Tahor band on 0 Stereo / 1 Mid / 2 Side, Reso's M/S mode (+ side depth), the tape's M/S drive
        B& place (int i, int where)                  { return set (bandId (i, "Place"), (float) where); }
        B& resoMs (int mode, float sideDepth = 0.15f) { return set (id::resoMs, (float) mode).set (id::resoSideDepth, sideDepth); }
        B& msDrive (float amount)                    { return set (id::tapeMsDrive, amount); }

        B& output (float db) { return set (id::outputGain, db); }
        B& input (float db)  { return set (id::inputGain, db); }

        // --- V79: chain order and the V74 / V75 units -------------------------------------------
        // The middle of the chain in this order (0 Char EQ, 1 Dynamics, 2 De-Ess, 3 Tone, 4 Tape)
        B& chain (std::array<int, 5> order)  { return set (id::midOrder, (float) (indexFromPermutation<5> (order) + 1)); }
        // A character EQ alongside whatever else is on (t 4 LE-A550, 5 LE-K9, 6 LE-H69, 7 LE-M4, or 0-3)
        B& eq (int t, std::initializer_list<std::pair<const char*, float>> fields)
        {
            set (id::eqOn, 1).set (charEqId (t, "On"), 1);
            for (auto& [f, x] : fields) set (charEqId (t, f), x);
            return *this;
        }
        B& eqPlace (int t, int where)        { return set (charEqId (t, "Place"), (float) where); }   // 0 Stereo, 1 Mid, 2 Side
        // A compressor alongside whatever else is on (u 4 LE-2500, 5 LE-G, 6 LE-DS1, 7 LE-IRON, 8 LE-336, or 0-3)
        B& comp (int u, std::initializer_list<std::pair<const char*, float>> fields)
        {
            set (id::compOn, 1).set (compId (u, "On"), 1);
            for (auto& [f, x] : fields) set (compId (u, f), x);
            return *this;
        }
        B& noOldComps()  { set (id::compOn, 1); for (int u = 0; u < 4; ++u) set (compId (u, "On"), 0); return *this; }
        // Goliath XL with True Peak (styles 0 Smart, 1 Transient, 2 Pumping, 3 Balanced, 4 Crisp, 5 Clip, 6 Spectral, 7 Bands, 8 Safe)
        B& xlLim (float threshDb, int style, float character = 5.0f, float transients = 0.0f, float upward = 0.0f, float ceilingDb = -1.0f)
        {
            return lim (threshDb, ceilingDb).set (id::limTp, 1).set (id::limMax, 1).set (id::limMaxStyle, (float) style)
                    .set (id::limCharacter, character).set (id::limTransients, transients).set (id::limUpward, upward);
        }
        B& width (float w) { return set (id::outWidth, w); }
    };

    using F = PresetManager::Factory;

    // The library was voiced on top of the original vocal-strip starting point (sections on, console
    // high-pass at 80 Hz). Each preset starts from that base, then its own values override it, so the
    // presets sound the same even though a fresh insert is now a clean, flat strip.
    V vocalBase()
    {
        V v { { id::resoOn, 1 }, { id::dsOn, 1 }, { id::flowOn, 1 }, { id::compOn, 1 }, { id::eqOn, 1 }, { id::limOn, 1 },
              { id::flowStyle, 0 } };
        for (int t = 1; t < 4; ++t) v.push_back ({ charEqId (t, "HpfFreq"), 80.0f });
        return v;
    }
    F p (const char* name, const char* category, const B& b)
    {
        auto v = vocalBase();
        v.insert (v.end(), b.v.begin(), b.v.end());   // the preset's own values win (applied last)
       #if MISHKAL_VERSION >= 55
        // V55: with the gentler Reso (15%) and Perach (2 / 80%) these come out a little louder - trim the output so
        // every preset stays level-matched (measured on the test vocal)
        static const std::map<juce::String, float> levelTrim {
            { "Pop Lead Modern", -3.5f },
            { "Pop Lead Airy", -2.0f },
            { "R&B Lead Silk", -3.0f },
            { "Soul Lead Warm", -3.0f },
            { "Country Lead Clear", -4.0f },
            { "Indie Lead Intimate", -1.0f },
            { "Ballad Lead Smooth", -3.0f },
            { "Rap Lead Punch", -2.0f },
            { "Rap Lead Dark", -1.0f },
            { "Trap Lead Air", -1.0f },
            { "Rap Doubles Tucked", -2.5f },
            { "Gospel Lead Power", -2.0f },
            { "Gospel Choir Glue", -1.5f },
            { "Worship Lead Open", -2.0f },
            { "BV Stacks Tucked", -2.0f },
            { "BV Airy Doubles", -1.5f },
            { "BV Ad-libs Thin", -1.5f },
            { "Podcast Host Warm", -2.0f },
            { "Podcast Guest Cleanup", -1.5f },
            { "Voiceover Broadcast", -1.5f },
            { "Audiobook Even", -2.0f },
            { "Acoustic Strum Shine", -1.5f },
            { "Piano Clear Modern", -1.0f },
            { "Synth Pad Wide", -1.0f },
            { "Vocal Bus Leveler", -0.5f },
            { "Perach Pop Lead", -2.0f },
            { "Blue Stripe Rap", -2.0f },
            { "Black Smooth Ballad", -1.5f },
            { "LE-RV Podcast Leveler", -2.0f },
            { "Esh Glow Air", -2.0f },
            { "R&B Warm Esh", -1.5f },
            { "Trap Peak Catch", -1.5f },
            { "Gospel Perach Lift", -2.0f },
            { "Worship Pad Vocal", -1.5f },
            { "Afro Glow", -1.5f },
            { "LE-STA Velvet Vocal", -1.5f },
            { "Blue x STA Vocal Stack", -1.5f },
            { "Black 76 Silk Air", -2.0f },
            { "LE-STA Gospel Lift", -2.0f },
            { "Perach Dark Rap", -1.0f },
            { "LE-RV Radio Ready", -1.0f },
            { "Spoken Word Intimate", -0.5f },
            { "Dancehall Lead Bright", -1.5f },
            { "Choir Wash STA", -2.0f },
            { "Keys Glow Perach", -2.0f },
            { "Telephone Voice", -2.0f },
            { "AM Radio", -1.5f },
            { "Megaphone", -2.5f },
        };
        if (auto it = levelTrim.find (name); it != levelTrim.end())
        {
            float out = 0.0f;
            for (auto& [pid, x] : v) if (pid == id::outputGain) out = x;   // the last one wins
            v.push_back ({ id::outputGain, out + it->second });
        }
       #endif
       #if MISHKAL_VERSION >= 73
        // V73: tested on a real song, the mastering presets went up to +0.9 dBTP between the samples (they were made
        // before True Peak existed) - they now all hold the ceiling with True Peak on
        if (juce::String (category) == "Mastering" || juce::String (name).startsWith ("Master ")) v.push_back ({ id::limTp, 1 });
       #endif
        return { name, v, category };
    }

    // V73: presets tuned on real recordings. Built on a clean, flat strip (no vocal starting point, no level trims):
    // every section a preset uses is switched on here, everything else stays off.
    F tested (const char* name, const B& b)
    {
        return { name, b.v, "Tested on Real Audio" };
    }
}

std::vector<PresetManager::Factory> makeFactoryPresets()
{
    std::vector<F> list;
    auto add = [&] (F f) { list.push_back (std::move (f)); };

    // Default: a clean, flat, unity-gain strip: every section ready and switched off, except the resonance suppressor (on)
    add ({ "Default", {}, {} });

   #if MISHKAL_VERSION >= 50
    //================================================================ V50: THE NEW LIBRARY
    // Built on everything the strip has now: Tahor + Reso, the four Character EQs, LE-76 (Purple / Blue Stripe / Black),
    // LE-RC and LE-RV, LE-660 and LE-STA, the vocal tools, the 1964 PK's own in / out, Perach, Esh and Slil, the Limiter.
    const char* lv = "Lead Vocals";
    add (p ("Pop Lead Modern", lv, B().lowCut (90).reso (0.4f).bell (2, 320, -1.5f, 1.3f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 2 }, { "AirGain", 3 }, { "Silk", 0.35f }, { "MudGain", -1.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.8f }, { 0, "Release", 120 }, { 0, "Makeup", 2 },
                            { 2, "Makeup", 1 } }).sta (4, 2, 1, 1)
        .rider (4).scHpf (1).pk (3).deEss (6500, -32).perach (6, 0, -1, 1, 2, 5).lim (-3).output (4)));
    add (p ("Pop Lead Airy", lv, B().lowCut (110, 2).reso (0.45f).bell (5, 3200, -1.5f, 3).dyn (5, -28, -5)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 1.5f }, { "AirGain", 4 }, { "AirFreq", 12000 }, { "Silk", 0.5f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -22 }, { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } }).rv (-22)
        .breath (-8).deEss (7200, -33, 9, 1).perach (5, -0.5f, -1, 0.5f, 3, 5).esh (3, 30, 2).lim (-2.5f).output (-4)));
    add (p ("R&B Lead Silk", lv, B().lowCut (85).reso (0.35f).bell (2, 250, -1.5f, 1.2f)
        .charEq (3, { { "HpfFreq", 75 }, { "BodyGain", 1 }, { "PresGain", 1 }, { "AirGain", 3.5f }, { "Silk", 0.5f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -20 } }).sta (5, 2, 2, 1)
        .breath (-8).rider (5).deEss (7000, -31, 8, 1).perach (5, 0.5f, 0.5f, 0, 2, 4).esh (1, 35, 0).lim (-2.5f)));
    add (p ("Soul Lead Warm", lv, B().lowCut (80).reso (0.3f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 2.5f }, { "HighBw", 6 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 200 }, { 0, "Makeup", 2 } }).fetRev (2).sta (4, 1, 2, 1)
        .rider (4).deEss (6500, -31).perach (4, 0.5f, 0.5f, 0, 1, 3).esh (1, 40, 0).lim (-2)));
    add (p ("Rock Lead Grit", lv, B().lowCut (110, 2).reso (0.4f).bell (4, 1000, 1.5f, 0.9f)
        .charEq (2, { { "HpfFreq", 100 }, { "MudGain", -2 }, { "PresGain", 3 }, { "PresFreq", 2500 }, { "AirGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.4f }, { 0, "Release", 70 }, { 0, "Makeup", 5 } }).fetRev (1)
        .pk (4).deEss (5800, -30).zerem (2, 0.5f).esh (4, 55, 1, 0, -2).lim (-3, -1, 0, 1).output (-2.5f)));
    add (p ("Country Lead Clear", lv, B().lowCut (90).reso (0.35f)
        .charEq (1, { { "HpfFreq", 80 }, { "MudGain", -2 }, { "PresGain", 2 }, { "PresFreq", 3000 }, { "AirGain", 2 } })
        .comps ({ 1, 0 }, { { 1, "Thresh", -20 }, { 0, "Thresh", -12 }, { 0, "Ratio", 3 }, { 0, "Attack", 2 } }).fetRev (2)
        .rider (4).deEss (6200, -31).perach (5, 0, -1, 1, 1.5f, 5).esh (0, 30, 1).lim (-2)));
    add (p ("Indie Lead Intimate", lv, B().lowCut (75).reso (0.3f)
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 2 }, { "BodyFreq", 180 }, { "AirGain", 1.5f } })
        .comps ({ 2 }, {}).sta (3, 0, 2, 1)
        .breath (-4).deEss (6500, -32, 6).perach (3, 0.5f, 0.5f, 0, 1, 2).esh (2, 35, 0).lim (-1)));
    add (p ("Ballad Lead Smooth", lv, B().lowCut (80).reso (0.35f).bell (5, 2800, -1.5f, 2.5f).dyn (5, -26, -4)
        .charEq (3, { { "HpfFreq", 70 }, { "BodyGain", 1 }, { "AirGain", 3 }, { "Silk", 0.45f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 250 }, { 0, "Makeup", 2 },
                            { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 1.5f } }).fetRev (2)
        .breath (-6).rider (5).deEss (6800, -31, 7).perach (4, 0, 0, 0.5f, 2, 3).esh (3, 25, 1).lim (-2).output (1.5f)));

    const char* rap = "Rap & Hip-Hop";
    add (p ("Rap Lead Punch", rap, B().lowCut (100, 2).reso (0.45f)
        .charEq (1, { { "HpfFreq", 90 }, { "PresGain", 3 }, { "PresFreq", 3200 }, { "MudGain", -3 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.3f }, { 0, "Release", 80 }, { 0, "Makeup", 5 } }).fetRev (1)
        .pk (5).scHpf (2).deEss (6500, -34).perach (6, 0, -1.5f, 2, 1, 7).lim (-4)));
    add (p ("Rap Lead Dark", rap, B().lowCut (95, 2).reso (0.4f).bell (6, 7000, -1.5f, 1)
        .charEq (2, { { "HpfFreq", 85 }, { "BodyGain", 1.5f }, { "BodyFreq", 160 }, { "MudGain", -2 }, { "PresGain", 1.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 0, "Makeup", 3 } }).fetRev (2).sta (4, 1, 1, 0)
        .pk (3).deEss (6000, -31).perach (4, 1, 0.5f, 0, -1, 4).esh (4, 45, 0, 0, -1).lim (-3)));
    add (p ("Trap Lead Air", rap, B().lowCut (110, 2).reso (0.5f).bell (3, 400, -2.5f, 1.4f)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 3 }, { "PresFreq", 4000 }, { "AirGain", 4 }, { "Silk", 0.45f } })
        .comps ({ 0, 1 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.2f }, { 0, "Release", 60 }, { 0, "Makeup", 4 } }).fetRev (1).rv (-18)
        .pk (6).scHpf (2).deEss (7500, -34, 10).perach (6, 0, -1.5f, 1.5f, 2.5f, 8).esh (3, 35, 2).lim (-5, -1, 0, 1).output (-10)));
    add (p ("Drill Lead Hard", rap, B().lowCut (120, 2).reso (0.5f).bell (3, 450, -3, 1.2f)
        .charEq (1, { { "HpfFreq", 110 }, { "PresGain", 4 }, { "PresFreq", 2800 }, { "MudGain", -3 } })
        .comps ({ 0 }, { { 0, "Thresh", -26 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.2f }, { 0, "Release", 50 }, { 0, "Makeup", 7 } }).fetRev (1)
        .pk (8).scHpf (3).deEss (6500, -34, 10).zerem (1, 0.6f).esh (4, 60, 1, 0, -3).lim (-5, -1, 0, 2).output (-5.5f)));
    add (p ("Rap Doubles Tucked", rap, B().lowCut (180, 2).highCut (12000).reso (0.5f).bell (3, 350, -3, 1.2f)
        .charEq (1, { { "HpfFreq", 150 }, { "PresGain", 1 }, { "AirGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.5f }, { 0, "Makeup", 3 } })
        .deEss (6500, -36, 12).perach (5, -2, -1, 0, 1, 6).lim (-3).output (3)));

    const char* gw = "Gospel & Worship";
    add (p ("Gospel Lead Power", gw, B().lowCut (90).reso (0.45f).bell (5, 2800, -1.5f, 2.5f).dyn (5, -26, -4)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "PresGain", 2 }, { "AirGain", 2.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 0, "Makeup", 2 } }).sta (5, 1, 1, 1)
        .pk (4).rider (6).deEss (6500, -31).perach (6, 0, 0.5f, 1, 2.5f, 5).esh (2, 35, 1).lim (-3)));
    add (p ("Gospel Choir Glue", gw, B().lowCut (120).reso (0.45f).bell (2, 400, -2, 1)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 1 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
        .comps ({ 2 }, {}).sta (4, 0, 2, 1)
        .deEss (7000, -33, 8, 1).perach (4, -0.5f, -0.5f, 0, 2, 3).esh (3, 25, 1).lim (-2)));
    add (p ("Worship Lead Open", gw, B().lowCut (80).reso (0.35f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "HighFreq", 5 }, { "HighBoost", 2.5f }, { "HighBw", 7 } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -18 }, { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 1.5f } })
        .breath (-6).rider (5).deEss (6500, -31, 6).perach (4, 0, -0.5f, 0.5f, 2, 3).esh (3, 30, 1).lim (-1.5f)));
    add (p ("Praise Break Shout", gw, B().lowCut (110, 2).reso (0.5f).bell (4, 1100, 2, 1)
        .charEq (1, { { "HpfFreq", 100 }, { "PresGain", 3 }, { "PresFreq", 2500 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.3f }, { 0, "Release", 60 }, { 0, "Makeup", 6 } }).fetRev (1)
        .pk (6).deEss (6000, -32, 10).zerem (2, 0.55f).esh (4, 50, 1, 0, -2).lim (-4, -1, 0, 1).output (-3.5f)));

    const char* bv = "Background Vocals";
    add (p ("BV Stacks Tucked", bv, B().lowCut (180, 2).reso (0.5f).bell (3, 350, -3, 1.2f)
        .charEq (3, { { "HpfFreq", 150 }, { "PresGain", -1 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 1 }, { 0, "Makeup", 3 } }).fetRev (2)
        .deEss (7000, -36, 12).perach (5, -2, -1, -1, 1.5f, 5).esh (3, 25, 1).lim (-2).output (1.5f)));
    add (p ("BV Airy Doubles", bv, B().lowCut (200, 2).reso (0.5f)
        .charEq (3, { { "HpfFreq", 180 }, { "AirGain", 4 }, { "AirFreq", 14000 }, { "Silk", 0.55f } })
        .comps ({ 1 }, {}).rv (-20)
        .deEss (7500, -36, 12, 1).perach (4, -2, -1.5f, 0, 2.5f, 5).lim (-2).output (-4)));
    add (p ("BV Ad-libs Thin", bv, B().lowCut (300, 3).highCut (9000).reso (0.4f).bell (4, 1500, 3, 1)
        .charEq (1, { { "HpfFreq", 250 }, { "PresGain", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.3f }, { 0, "Makeup", 5 } }).fetRev (1)
        .deEss (6000, -32).zerem (6, 0.5f).esh (4, 50, 1, 0, -2).lim (-3).output (-1.5f)));
    add (p ("BV Gang Shouts", bv, B().lowCut (150, 2).reso (0.4f).bell (4, 1200, 2, 0.8f)
        .charEq (2, { { "HpfFreq", 130 }, { "MudGain", -2 }, { "PresGain", 2.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.5f }, { 0, "Makeup", 4 } }).fetRev (1).sta (4, 1, 1, 0)
        .deEss (6000, -32).esh (2, 45, 1).lim (-3).output (-5.5f)));

    const char* talk = "Podcast & Voice";
    add (p ("Podcast Host Warm", talk, B().lowCut (80, 2).reso (0.35f).bell (2, 300, -1.5f, 1.2f)
        .charEq (2, { { "HpfFreq", 70 }, { "BodyGain", 1.5f }, { "BodyFreq", 140 }, { "PresGain", 1.5f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -26 } }).rv (-26).sta (3, 1, 2, 0)
        .gate (-55).breath (-10).rider (6).deEss (6000, -30).perach (5, 0, 0, 1, 0, 4).lim (-3).output (-3)));
    add (p ("Podcast Guest Cleanup", talk, B().lowCut (100, 3).highCut (12000).reso (0.55f, 200, 8000).notch (1, 60).notch (2, 120)
        .charEq (1, { { "HpfFreq", 90 }, { "MudGain", -3 }, { "PresGain", 2 }, { "PresFreq", 3000 } })
        .comps ({ 1 }, { { 1, "Thresh", -28 } }).rv (-28)
        .gate (-50).breath (-12).rider (8).deEss (6000, -30).perach (6, -1, -0.5f, 1.5f, 0, 5).lim (-3).output (-9.5f)));
    add (p ("Voiceover Broadcast", talk, B().lowCut (90, 2).reso (0.4f).lowShelf (1, 120, 1.5f).bell (4, 2500, 1.5f, 1)
        .charEq (1, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "PresGain", 2 }, { "AirGain", 1.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 0, "Makeup", 3 } }).fetRev (2).sta (5, 2, 1, 0)
        .gate (-55).pk (3).deEss (6500, -31).perach (5, 0.5f, 0, 1, 1, 5).lim (-4).output (1)));
    add (p ("Audiobook Even", talk, B().lowCut (80, 2).reso (0.35f)
        .charEq (3, { { "HpfFreq", 70 }, { "MudGain", -1.5f }, { "AirGain", 1 } })
        .comps ({ 1 }, { { 1, "Thresh", -22 } }).rv (-22)
        .gate (-55).breath (-10).rider (6).deEss (6200, -31).perach (4, 0, 0, 0.5f, 0, 3).lim (-2).output (-8.5f)));

    const char* drums = "Drums";
    add (p ("Kick Modern Punch", drums, B().lowCut (30, 2).noReso().bell (2, 350, -4, 1.5f).bell (5, 3500, 3, 1.2f).bell (1, 60, 2, 1.2f)
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1.5f }, { "BodyFreq", 70 }, { "PresGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 80 }, { 0, "Makeup", 2 } }).fetRev (1)
        .pk (4).noDeEss().noZerem().esh (2, 40, 1).lim (-2).output (-2.5f)));
    add (p ("Snare Crack Tube", drums, B().lowCut (90, 2).reso (0.3f, 400, 6000).bell (2, 200, 2, 1.2f).bell (4, 900, -2, 2).bell (6, 5000, 3, 1)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "BodyFreq", 200 }, { "PresGain", 2 } })
        .comps ({ 2 }, {}).sta (6, 1, 0, 0)
        .pk (3).noDeEss().noZerem().esh (4, 45, 1, 0, -1).lim (-2).output (-3)));
    add (p ("Toms Round", drums, B().lowCut (60).noReso().bell (3, 400, -4, 1.5f).bell (5, 4000, 2.5f, 1.2f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "HighFreq", 3 }, { "HighBoost", 1.5f }, { "HighBw", 5 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 8 }, { 0, "Release", 100 }, { 0, "Makeup", 2 } })
        .gate (-40).noDeEss().noZerem().esh (2, 35, 1).lim (-2).output (-2)));
    add (p ("Overheads Silk", drums, B().lowCut (120, 2).reso (0.4f, 2000, 14000).bell (3, 450, -2, 1.2f)
        .charEq (3, { { "HpfFreq", 100 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .noDeEss().perach (3, 0, -0.5f, -0.5f, 1.5f, 3).esh (3, 25, 1).lim (-1)));
    add (p ("Drum Bus Tube Glue", drums, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1 }, { "BodyFreq", 80 }, { "AirGain", 1.5f } })
        .comps ({ 2 }, {}).sta (4, 0, 1, 0, 0.7f)
        .scHpf (2).pk (3).noDeEss().noZerem().esh (2, 40, 1).lim (-3, -1, 0, 1).output (-4)));
    add (p ("Drum Room Crush", drums, B().lowCut (60).noReso().bell (3, 500, -3, 1).highCut (12000)
        .charEq (2, { { "HpfFreq", 50 }, { "BodyGain", 2 }, { "AirGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -30 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.2f }, { 0, "Release", 60 }, { 0, "Makeup", 9 }, { 0, "Mix", 0.6f } }).fetRev (1)
        .noDeEss().noZerem().esh (4, 60, 1, 0, -3).lim (-3, -1, 0, 2).output (-3)));

    const char* bass = "Bass";
    add (p ("Bass DI Round", bass, B().lowCut (30, 2).noReso().bell (2, 250, -2, 1.2f).bell (5, 800, 1.5f, 1)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "LowAtten", 1 } })
        .comps ({ 2 }, {}).sta (5, 0, 1, 0)
        .noDeEss().noZerem().esh (1, 45, 0).lim (-2)));
    add (p ("Bass Amp Grit", bass, B().lowCut (35, 2).noReso().bell (3, 500, -2, 1.3f).bell (5, 1200, 2.5f, 1)
        .charEq (2, { { "HpfFreq", 30 }, { "BodyGain", 1.5f }, { "BodyFreq", 100 }, { "PresGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 8 }, { 0, "Attack", 1 }, { 0, "Release", 100 }, { 0, "Makeup", 4 } }).fetRev (1)
        .noDeEss().noZerem().esh (4, 55, 1, 0, -2).lim (-2)));
    add (p ("808 Clean Weight", bass, B().lowCut (28, 3).noReso().bell (3, 250, -3, 1.2f).highCut (6000)
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1 }, { "BodyFreq", 60 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 }, { 1, "Ratio", 4 }, { 1, "Attack", 20 }, { 1, "Release", 200 }, { 1, "Character", 1 } })
        .pk (3).noDeEss().noZerem().esh (1, 30, 0).lim (-3, -1, 0, 1).output (-6)));
    add (p ("Synth Bass Tight", bass, B().lowCut (30, 2).noReso().bell (2, 200, -2, 1)
        .charEq (1, { { "HpfFreq", 25 }, { "PresGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 80 }, { 0, "Makeup", 2 } }).fetRev (2)
        .noDeEss().noZerem().esh (0, 35, 1).lim (-2)));

    const char* gtr = "Guitars";
    add (p ("Acoustic Strum Shine", gtr, B().lowCut (90, 2).reso (0.4f, 300, 10000).bell (2, 220, -2.5f, 1.2f)
        .charEq (3, { { "HpfFreq", 80 }, { "AirGain", 2.5f }, { "Silk", 0.35f } })
        .comps ({ 1 }, { { 1, "Thresh", -18 }, { 1, "Ratio", 3 } })
        .noDeEss().perach (4, -0.5f, -1, 0, 1.5f, 4).esh (3, 25, 1).lim (-1)));
    add (p ("Electric Crunch Tube", gtr, B().lowCut (100, 2).reso (0.35f, 1500, 8000).bell (3, 400, -2, 1.3f).highCut (11000)
        .charEq (2, { { "HpfFreq", 90 }, { "MudGain", -1.5f }, { "PresGain", 1.5f } })
        .comps ({ 2 }, {}).sta (3, 1, 1, 0)
        .noDeEss().noZerem().esh (2, 40, 1).lim (-2)));
    add (p ("Clean Funk Snap", gtr, B().lowCut (140, 2).reso (0.3f).bell (5, 3000, 2.5f, 1)
        .charEq (1, { { "HpfFreq", 120 }, { "PresGain", 2 }, { "AirGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 60 }, { 0, "Makeup", 3 } }).fetRev (2)
        .noDeEss().noZerem().esh (0, 30, 2).lim (-2)));
    add (p ("Guitar Bus Esh", gtr, B().noReso().noTahor()
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 0.5f }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 2 }, { 2, "Makeup", 1 } })
        .scHpf (1).noDeEss().noZerem().esh (2, 35, 1).lim (-1)));

    const char* keys = "Keys";
    add (p ("Piano Clear Modern", keys, B().lowCut (50).reso (0.3f, 300, 8000).bell (3, 350, -2, 1)
        .charEq (3, { { "HpfFreq", 40 }, { "AirGain", 2 }, { "Silk", 0.3f } })
        .comps ({ 1 }, { { 1, "Thresh", -16 }, { 1, "Ratio", 2.5f }, { 1, "Character", 1 } })
        .noDeEss().perach (3, 0, -0.5f, 0, 1, 3).lim (-1)));
    add (p ("Piano Warm Tube", keys, B().lowCut (40).reso (0.2f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "HighFreq", 4 }, { "HighBoost", 1.5f }, { "HighBw", 6 } })
        .comps ({ 2 }, {}).sta (2, 0, 2, 1)
        .noDeEss().noZerem().esh (1, 35, 0).lim (-1)));
    add (p ("Rhodes Velvet", keys, B().lowCut (60).reso (0.25f, 800, 6000)
        .charEq (2, { { "HpfFreq", 50 }, { "BodyGain", 1.5f }, { "BodyFreq", 200 } })
        .comps ({ 2 }, {}).sta (3, 1, 2, 1)
        .noDeEss().noZerem().esh (1, 40, 0).lim (-1)));
    add (p ("Synth Pad Wide", keys, B().lowCut (120, 2).reso (0.45f, 1000, 12000).bell (3, 400, -2, 0.8f)
        .charEq (3, { { "HpfFreq", 100 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 3 }, { 2, "Makeup", 1 } })
        .noDeEss().perach (3, -1, -0.5f, 0, 1.5f, 2).esh (3, 25, 1).lim (-1).output (MISHKAL_VERSION >= 52 ? 2.0f : 4.0f)));

    const char* buses = "Buses";
    add (p ("Mix Bus Tube Glue", buses, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 20 }, { "AirGain", 1 } })
        .comps ({ 2 }, {}).sta (2, 0, 2, 0, 0.6f)
        .scHpf (2).noDeEss().noZerem().esh (0, 25, 1).lim (-1)));
    add (p ("Mix Bus Esh Polish", buses, B().noReso().noTahor()
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().perach (2.5f, 0, -0.5f, 0, 1, 2, 0.6f).esh (3, 25, 1).lim (-1)));
    add (p ("Vocal Bus Leveler", buses, B().lowCut (80).reso (0.25f)
        .charEq (3, { { "HpfFreq", 70 }, { "AirGain", 2 }, { "Silk", 0.35f } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } }).rv (-18)
        .deEss (7000, -32, 5, 1, true).perach (3, 0, -0.5f, 0.5f, 1, 3).lim (-1).output (-7.5f)));
    add (p ("Music Bus Warm Tape", buses, B().noReso().noTahor()
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 1.5f }, { "HighFreq", 5 }, { "HighBoost", 1.5f }, { "HighBw", 7 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 2 } })
        .scHpf (1).noDeEss().noZerem().tape (1, 3, 0.45f).lim (-1).output (1.5f)));

   #if MISHKAL_VERSION >= 60
    // V60: mid/side bus chains (stereo tracks)
    add (p ("Mix Bus M/S Tight Low End", buses, B().noReso().lowCut (20, 1)
        .band (1, BandType::LowCut, 150, 0, 0.71f, 2).place (1, 2)
        .charEq (1, { { "HpfFreq", 20 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().noZerem().tape (0, 2, 0.45f).msDrive (-0.3f).lim (-1)));
    add (p ("Drum Bus M/S Width", buses, B().noReso().lowCut (25, 2)
        .band (1, BandType::LowCut, 120, 0, 0.71f, 1).place (1, 2)
        .highShelf (6, 8000, 2).place (6, 2)
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1 }, { "BodyFreq", 80 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 100 }, { 0, "Makeup", 2 }, { 0, "Mix", 0.5f } })
        .fetRev (1).scHpf (2).noDeEss().noZerem().esh (2, 30, 1).msDrive (0.4f).lim (-2, -1, 0, 1).output (-4)));
    add (p ("Vocal Stack M/S Wide", buses, B().lowCut (90).reso (0.15f, 2000, 10000).resoMs (2)
        .bell (4, 2500, -2, 1).place (4, 1)                              // make room for the lead in the centre
        .highShelf (6, 9000, 2).place (6, 2)                             // and let the stack open up on the sides
        .charEq (3, { { "HpfFreq", 80 }, { "AirGain", 1.5f }, { "Silk", 0.3f } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 2 }, { 2, "Makeup", 1 } })
        .deEss (7000, -32, 6, 1, true).noZerem().lim (-1).output (1.5f)));
   #endif
    const char* master = "Mastering";
    add (p ("Master Clean Loud", master, B().reso (0.12f, 2000, 9000, 0.3f, 0.7f).lowCut (28, 2).bell (3, 300, -1, 0.7f)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1.5f }, { "AirFreq", 14000 }, { "Silk", 0.25f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().perach (2, 0, -0.5f, 0, 0.5f, 2, 0.5f).lim (-7, -1, 1, 2)));
    add (p ("Master Warm Tube", master, B().noReso().lowCut (25, 2)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 1.5f }, { "HighBw", 7 } })
        .comps ({ 2 }, {}).sta (2, 0, 3, 1, 0.6f)
        .scHpf (2).noDeEss().noZerem().esh (1, 25, 1).lim (-4, -1, 0, 1)));
    add (p ("Master Hip-Hop Weight", master, B().noReso().lowCut (25, 3).lowShelf (1, 70, 1.5f)
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 60 }, { "PresGain", 1 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 0 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().noZerem().esh (2, 30, 1).lim (-7, -1, 1, 2).output (-2.5f)));
    add (p ("Master Streaming Safe", master, B().noReso().lowCut (25, 2)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Mix", 0.6f } })
        .scHpf (2).noDeEss().perach (2, 0, 0, 0, 0.5f, 2, 0.5f).lim (-2, -1, 0, 1)));
    add (p ("Master Gospel Live Loud", master, B().reso (0.12f, 1500, 8000).lowCut (30, 2)
        .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 0.5f }, { "AirGain", 1.5f } })
        .comps ({ 2 }, {}).sta (2, 1, 2, 1, 0.6f)
        .scHpf (2).noDeEss().perach (2, 0, 0, 0.5f, 1, 2, 0.5f).esh (3, 20, 1).lim (-5, -1, 1, 1)));
    add (p ("Master Vintage Glow", master, B().noReso().lowCut (35, 2).highCut (16000, 1)
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 1.5f }, { "HighFreq", 3 }, { "HighBoost", 1 }, { "AttenFreq", 1 }, { "HighAtten", 1.5f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 3 } })
        .noDeEss().noZerem().tape (1, 2, 0.4f).lim (MISHKAL_VERSION >= 73 ? -4.0f : -3.0f, -1, 0, 1)
        .output (MISHKAL_VERSION >= 73 ? 0.0f : MISHKAL_VERSION >= 52 ? 1.0f : 2.5f)));   // V73: the +1 dB after the limiter pushed it past the ceiling
   #if MISHKAL_VERSION >= 60
    // V60: mid/side chains (they do their M/S work on stereo tracks; on mono the Side parts simply rest)
    add (p ("Master M/S Side Low Cut + Air", master, B().noReso().lowCut (25, 2)
        .band (1, BandType::LowCut, 120, 0, 0.71f, 1).place (1, 2)       // the sides lose their lows: a tight, centred bass
        .bell (3, 280, -1, 0.8f).place (3, 1)                            // a touch of mud out of the centre only
        .highShelf (6, 10000, 2).place (6, 2)                            // air on the sides: width without a harsher vocal
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().noZerem().lim (-5, -1, 1, 1)));
    add (p ("Master M/S Wide Tape", master, B().noReso().lowCut (25, 2)
        .band (1, BandType::LowCut, 100, 0, 0.71f, 1).place (1, 2)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1 }, { "HighFreq", 5 }, { "HighBoost", 1 }, { "HighBw", 7 } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 2 }, { 2, "Mix", 0.6f } })
        .scHpf (2).noDeEss().noZerem().esh (0, 30, 1).msDrive (0.35f).lim (-4, -1, 0, 1)));
    add (p ("Master M/S De-Harsh", master, B().reso (0.15f, 2000, 10000, 0.4f, 0.6f).resoMs (3, 0.3f).lowCut (25, 2)
        .charEq (3, { { "HpfFreq", 20 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Mix", 0.6f } })
        .scHpf (2).noDeEss().noZerem().lim (-3, -1, 0, 1)));
   #endif
   #endif
   #if MISHKAL_VERSION < 50

    //================================================================ LEAD VOCALS
    const char* lead = "Lead Vocals";
    add (p ("Vocal Starter", lead, B()));   // the original vocal-strip starting point
    add (p ("Pop Vocal", lead, B().lowCut (90).reso (0.4f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 2 }, { "AirGain", 3 }, { "Silk", 0.35f }, { "MudGain", -2 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.8f }, { 0, "Release", 120 }, { 0, "Makeup", 2 },
                            { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } })
        .deEss (6500, -32).zerem (0).lim (-3)));
    add (p ("Modern Pop Rider", lead, B().lowCut (90).reso (0.4f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 2 }, { "AirGain", 2.5f }, { "Silk", 0.3f }, { "MudGain", -2 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 2, "Thresh", -14 } })
        .breath (-10).rider (6).scHpf (1).tape (0, 1, 0.5f).deEss (6500, -30).zerem (0, 0.45f).lim (-3)));
    add (p ("Rap Upfront", lead, B().lowCut (100, 2).reso (0.45f)
        .charEq (1, { { "HpfFreq", 90 }, { "PresGain", 3 }, { "PresFreq", 3200 }, { "MudGain", -3 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.3f }, { 0, "Release", 80 }, { 0, "Makeup", 5 } })
        .deEss (6500, -34).zerem (1).lim (-4)));
    add (p ("Rap Tape Grit", lead, B().lowCut (100, 2).reso (0.45f)
        .charEq (1, { { "HpfFreq", 90 }, { "PresGain", 3 }, { "PresFreq", 3200 }, { "MudGain", -3 } })
        .comps ({ 0 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.4f }, { 0, "Release", 90 } })
        .rider (4).scHpf (2).peakCatch().tape (1, 3, 0.6f, 0.7f).deEss (6000, -28).zerem (1).lim (-4)));
    add (p ("Gentle Polish", lead, B().lowCut (80).reso (0.25f)
        .charEq (3, { { "HpfFreq", 70 }, { "AirGain", 2 }, { "Silk", 0.4f } })
        .comps ({ 0 }, { { 0, "Thresh", -14 }, { 0, "Ratio", 2 }, { 0, "Attack", 3 } })
        .deEss (6500, -30).zerem (2, 0.3f).lim (-1)));
    add (p ("R&B Silk", lead, B().lowCut (85).reso (0.35f).bell (2, 250, -1.5f, 1.2f)
        .charEq (3, { { "HpfFreq", 75 }, { "BodyGain", 1 }, { "PresGain", 1 }, { "AirGain", 3.5f }, { "Silk", 0.5f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -22 }, { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 1.5f } })
        .breath (-8).rider (5).deEss (7000, -31, 8, 1).zerem (3, 0.5f).tape (0, 1, 0.45f).lim (-2.5f)));
    add (p ("Trap Lead Bright", lead, B().lowCut (110, 2).reso (0.5f).bell (3, 400, -2.5f, 1.4f)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 3 }, { "PresFreq", 4000 }, { "AirGain", 4 }, { "Silk", 0.45f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.2f }, { 0, "Release", 60 }, { 0, "Makeup", 4 },
                            { 2, "Thresh", -14 }, { 2, "Tc", 0 } })
        .scHpf (2).deEss (7500, -34, 10).zerem (1, 0.6f).lim (-5, -1, 0, 1)));
    add (p ("Rock Vocal Grit", lead, B().lowCut (110, 2).reso (0.4f).bell (4, 1000, 1.5f, 0.9f)
        .charEq (2, { { "HpfFreq", 100 }, { "MudGain", -2 }, { "PresGain", 3 }, { "PresFreq", 2500 }, { "AirGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.4f }, { 0, "Release", 70 }, { 0, "Makeup", 5 } })
        .deEss (5800, -30).zerem (2, 0.55f).tape (1, 0, 0.65f).lim (-3, -1, 0, 1)));
    add (p ("Country Clear", lead, B().lowCut (90).reso (0.35f)
        .charEq (1, { { "HpfFreq", 80 }, { "MudGain", -2 }, { "PresGain", 2 }, { "PresFreq", 3000 }, { "AirGain", 2 } })
        .comps ({ 1, 0 }, { { 1, "Thresh", -20 }, { 0, "Thresh", -12 }, { 0, "Ratio", 3 }, { 0, "Attack", 2 } })
        .rider (4).deEss (6200, -31).zerem (0, 0.4f).lim (-2)));
    add (p ("Indie Intimate", lead, B().lowCut (75).reso (0.3f)
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 2 }, { "BodyFreq", 180 }, { "AirGain", 1.5f } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .breath (-4).deEss (6500, -32, 6).zerem (4, 0.35f).tape (0, 2, 0.4f).lim (-1)));
    add (p ("Gospel Power", lead, B().lowCut (90).reso (0.45f).bell (5, 2800, -1.5f, 2.5f).dyn (5, -26, -4)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "PresGain", 2 }, { "AirGain", 2.5f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 2 } })
        .rider (8).deEss (6800, -30).zerem (6, 0.4f).lim (-3, -1, 0, 1)));
    add (p ("Afrobeats Lead", lead, B().lowCut (100, 2).reso (0.4f)
        .charEq (3, { { "HpfFreq", 90 }, { "MudGain", -2 }, { "PresGain", 2.5f }, { "AirGain", 3 }, { "Silk", 0.4f } })
        .comps ({ 0, 1 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.6f }, { 1, "Thresh", -16 } })
        .rider (5).scHpf (1).deEss (7000, -31).zerem (0, 0.5f).tape (0, 1, 0.45f).lim (-3)));
    add (p ("Female Pop Air", lead, B().lowCut (120, 2).reso (0.45f).bell (5, 3200, -1.5f, 3).dyn (5, -28, -5)
        .charEq (3, { { "HpfFreq", 100 }, { "BodyGain", -1 }, { "AirGain", 3.5f }, { "AirFreq", 14000 }, { "Silk", 0.45f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 2, "Thresh", -14 } })
        .breath (-8).rider (5).deEss (7500, -32).zerem (4, 0.45f).lim (-3)));
    add (p ("Male Pop Body", lead, B().lowCut (80).reso (0.4f).bell (2, 300, -2, 1.3f)
        .charEq (2, { { "HpfFreq", 70 }, { "BodyGain", 1.5f }, { "BodyFreq", 140 }, { "PresGain", 2 }, { "AirGain", 2 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.8f }, { 2, "Thresh", -15 } })
        .breath (-6).rider (5).deEss (5800, -31).zerem (0, 0.45f).lim (-3)));

    //================================================================ BACKING VOCALS
    const char* bv = "Backing Vocals";
    add (p ("Stacked Harmonies", bv, B().lowCut (180, 2).reso (0.5f).bell (3, 350, -3, 1.2f)
        .charEq (3, { { "HpfFreq", 150 }, { "PresGain", -1 }, { "AirGain", 4 }, { "Silk", 0.5f } })
        .comps ({ 2 }, { { 2, "Thresh", -20 }, { 2, "Tc", 1 }, { 2, "Makeup", 2 } })
        .breath (-12).deEss (7000, -36, 12).zerem (4, 0.5f).lim (-2)));
    add (p ("Airy Doubles", bv, B().lowCut (200, 2).reso (0.5f)
        .charEq (3, { { "HpfFreq", 180 }, { "MudGain", -3 }, { "AirGain", 5 }, { "Silk", 0.6f } })
        .comps ({ 1 }, { { 1, "Thresh", -22 } })
        .breath (-14).deEss (7500, -38, 14).zerem (4, 0.4f).lim (-1)));
    add (p ("Gang Shouts", bv, B().lowCut (150, 2).reso (0.4f).bell (4, 1200, 2, 0.8f)
        .charEq (1, { { "HpfFreq", 140 }, { "MudGain", -3 }, { "PresGain", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 10 }, { 0, "Attack", 0.3f }, { 0, "Release", 60 }, { 0, "Makeup", 5 } })
        .deEss (6000, -30).zerem (6, 0.5f).tape (1, 0, 0.7f).lim (-3)));
    add (p ("Ad-libs Thin", bv, B().lowCut (300, 3).highCut (9000).reso (0.4f).bell (4, 1500, 3, 1)
        .charEq (1, { { "HpfFreq", 250 }, { "PresGain", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.3f }, { 0, "Makeup", 4 } })
        .deEss (6500, -32).zerem (1, 0.5f).lim (-2)));
    add (p ("Choir Wash", bv, B().lowCut (120).reso (0.45f).bell (2, 400, -2, 1)
        .charEq (0, { { "HighFreq", 4 }, { "HighBoost", 3 }, { "HighBw", 6 } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 3 }, { 2, "Makeup", 1.5f } })
        .deEss (6800, -34, 10, 1).zerem (4, 0.4f).lim (-1)));

    //================================================================ BALLADS & WARMTH
    const char* warm = "Ballads & Warmth";
    add (p ("Worship Ballad", warm, B().lowCut (80).reso (0.3f)
        .charEq (2, { { "HpfFreq", 70 }, { "BodyGain", 1.5f }, { "AirGain", 3 }, { "PresGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -20 }, { 2, "Tc", 1 }, { 2, "Makeup", 3 } })
        .gate (-55).deEss (6500, -30).zerem (4).lim (-2)));
    add (p ("Tube Warmth", warm, B().lowCut (70).reso (0.3f)
        .charEq (0, { { "LowFreq", 3 }, { "LowBoost", 3 }, { "LowAtten", 2.5f }, { "HighFreq", 4 }, { "HighBw", 6 }, { "HighBoost", 4 },
                      { "AttenFreq", 2 }, { "HighAtten", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -18 }, { 2, "Tc", 1 }, { 2, "Makeup", 2.5f } })
        .deEss (6500, -30).zerem (3).lim (-2)));
    add (p ("Soul Warm", warm, B().lowCut (80).reso (0.3f)
        .charEq (2, { { "HpfFreq", 70 }, { "BodyGain", 2 }, { "BodyFreq", 200 }, { "PresGain", 1.5f }, { "AirGain", 1 } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -20 }, { 2, "Thresh", -14 }, { 2, "Tc", 2 } })
        .rider (4).deEss (6000, -30).zerem (3, 0.45f).tape (1, 2, 0.55f).lim (-2)));
    add (p ("Jazz Croon", warm, B().lowCut (70).reso (0.2f)
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 2 }, { "HighFreq", 3 }, { "HighBoost", 2 }, { "HighBw", 7 } })
        .comps ({ 1 }, { { 1, "Thresh", -16 } })
        .deEss (6000, -30, 6).noZerem().tape (0, 2, 0.4f).lim (-0.5f)));
    add (p ("Acoustic Singer-Songwriter", warm, B().lowCut (85).reso (0.35f).bell (3, 450, -1.5f, 1.2f)
        .charEq (2, { { "HpfFreq", 75 }, { "BodyGain", 1 }, { "PresGain", 1.5f }, { "AirGain", 2.5f } })
        .comps ({ 1 }, { { 1, "Thresh", -20 } })
        .breath (-5).rider (4).deEss (6500, -31).zerem (4, 0.35f).tape (0, 1, 0.4f).lim (-1.5f)));

    //================================================================ SPOKEN WORD
    const char* talk = "Spoken Word";
    add (p ("Podcast Voice", talk, B().lowCut (80, 2).reso (0.35f)
        .charEq (1, { { "HpfFreq", 80 }, { "MudGain", -2.5f }, { "PresGain", 1.5f } })
        .comps ({ 1 }, { { 1, "Thresh", -22 } })
        .gate (-45).deEss (5500, -30).zerem (5).lim (0, -1)));
    add (p ("Radio Host", talk, B().lowCut (70, 2).reso (0.4f).lowShelf (1, 120, 2)
        .charEq (1, { { "HpfFreq", 60 }, { "BodyGain", 2 }, { "BodyFreq", 120 }, { "MudGain", -3 }, { "PresGain", 3 }, { "AirGain", 2 } })
        .comps ({ 0, 1 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.5f }, { 0, "Makeup", 4 }, { 1, "Thresh", -18 } })
        .gate (-48).rider (6).deEss (5500, -30).zerem (6, 0.45f).lim (-4, -1, 1)));
    add (p ("Audiobook Narration", talk, B().lowCut (80, 2).reso (0.35f)
        .charEq (2, { { "HpfFreq", 70 }, { "MudGain", -1.5f }, { "PresGain", 1 } })
        .comps ({ 1 }, { { 1, "Thresh", -20 } })
        .gate (-55).breath (-12).rider (6).deEss (5800, -32).zerem (5, 0.35f).lim (-2, -3)));
    add (p ("Voiceover Punchy", talk, B().lowCut (90, 2).reso (0.4f).bell (4, 2500, 2, 1)
        .charEq (1, { { "HpfFreq", 80 }, { "MudGain", -3 }, { "PresGain", 2.5f }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.3f }, { 0, "Release", 60 }, { 0, "Makeup", 6 } })
        .gate (-45).breath (-14).deEss (6000, -30).zerem (6, 0.5f).lim (-5, -1, 0, 1)));
    add (p ("Interview Cleanup", talk, B().lowCut (100, 3).highCut (12000).reso (0.55f, 200, 8000).notch (1, 60).notch (2, 120)
        .charEq (1, { { "HpfFreq", 90 }, { "MudGain", -3 }, { "PresGain", 2 } })
        .comps ({ 1 }, { { 1, "Thresh", -24 } })
        .gate (-40).rider (8).deEss (5500, -30).zerem (5, 0.5f).lim (-3)));

    //================================================================ DRUMS
    const char* drums = "Drums";
    add (p ("Kick Punch", drums, B().lowCut (30, 2).noReso().bell (2, 350, -4, 1.5f).bell (5, 3500, 3, 1.2f).bell (1, 60, 2, 1.2f)
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 2 }, { "BodyFreq", 70 }, { "PresGain", 2 }, { "PresFreq", 4000 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 12 }, { 0, "Release", 80 }, { 0, "Makeup", 3 } })
        .gate (-40).noDeEss().zerem (7, 0.4f).safety()));
    add (p ("Kick Sub Boom", drums, B().lowCut (25, 3).noReso().lowShelf (1, 60, 3).bell (3, 300, -5, 1.4f).highCut (8000)
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 5 }, { "LowAtten", 3 } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 0 }, { 2, "Makeup", 2 } })
        .noDeEss().noZerem().tape (1, 3, 0.55f).safety()));
    add (p ("Snare Crack", drums, B().lowCut (90, 2).reso (0.3f, 400, 6000).bell (2, 200, 2, 1.2f).bell (4, 900, -2, 2).bell (6, 5000, 3, 1)
        .charEq (1, { { "HpfFreq", 80 }, { "PresGain", 3 }, { "PresFreq", 5000 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 5 }, { 0, "Attack", 6 }, { 0, "Release", 90 }, { 0, "Makeup", 3 } })
        .gate (-38).peakCatch().noDeEss().zerem (7, 0.5f).safety()));
    add (p ("Snare Fat", drums, B().lowCut (70, 1).reso (0.25f, 500, 5000).bell (2, 180, 3, 1.1f).bell (4, 1000, -2, 2)
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 3 }, { "BodyFreq", 200 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 2 } })
        .gate (-40).noDeEss().zerem (7, 0.4f).tape (1, 3, 0.6f).safety()));
    add (p ("Toms Tight", drums, B().lowCut (60, 1).noReso().bell (3, 400, -4, 1.5f).bell (5, 4000, 2.5f, 1.2f)
        .charEq (1, { { "HpfFreq", 50 }, { "BodyGain", 2 }, { "BodyFreq", 100 }, { "PresGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 100 }, { 0, "Makeup", 2 } })
        .gate (-36).noDeEss().zerem (7, 0.35f).safety()));
    add (p ("Hi-Hat Smooth", drums, B().lowCut (300, 3).reso (0.5f, 3000, 16000, 0.5f, 0.6f).bell (5, 6000, -2, 2).dyn (5, -30, -5)
        .noCharEq()
        .comps ({ 1 }, { { 1, "Thresh", -18 }, { 1, "Ratio", 2.5f }, { 1, "Attack", 5 }, { 1, "Release", 60 }, { 1, "Behavior", 1 }, { 1, "Character", 1 }, { 1, "Makeup", 2 } })
        .deEss (9000, -30, 6, 1).noZerem().safety()));
    add (p ("Overheads Air", drums, B().lowCut (120, 2).reso (0.4f, 2000, 14000).bell (3, 450, -2, 1.2f)
        .charEq (3, { { "HpfFreq", 100 }, { "AirGain", 3 }, { "AirFreq", 12000 }, { "Silk", 0.3f } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } })
        .noDeEss().zerem (7, 0.3f).safety()));
    add (p ("Room Smash", drums, B().lowCut (60).noReso().bell (3, 500, -3, 1).highCut (12000)
        .charEq (1, { { "HpfFreq", 50 }, { "PresGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -30 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.1f }, { 0, "Release", 60 }, { 0, "Makeup", 10 } })
        .noDeEss().zerem (6, 0.6f).tape (2, 0, 0.7f).lim (-4)));
    add (p ("Drum Bus Glue", drums, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 80 }, { "PresGain", 1 }, { "AirGain", 1.5f } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } })
        .scHpf (1).noDeEss().zerem (7, 0.35f).tape (1, 3, 0.5f, 0.6f).lim (-1.5f)));
    add (p ("Drum Bus Parallel Crush", drums, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 20 }, { "PresGain", 2 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -28 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.5f }, { 0, "Release", 80 }, { 0, "Makeup", 9 }, { 0, "Mix", 0.35f } })
        .noDeEss().zerem (7, 0.4f).tape (1, 3, 0.6f).lim (-2)));
    add (p ("Percussion Shine", drums, B().lowCut (150, 2).reso (0.35f, 1500, 14000).bell (6, 8000, 2, 1)
        .charEq (3, { { "HpfFreq", 120 }, { "AirGain", 2.5f }, { "Silk", 0.35f } })
        .comps ({ 0 }, { { 0, "Thresh", -14 }, { 0, "Ratio", 3 }, { 0, "Attack", 3 }, { 0, "Release", 80 } })
        .peakCatch().noDeEss().zerem (7, 0.3f).safety()));
    add (p ("Breakbeat Vintage", drums, B().lowCut (40).noReso().highCut (11000)
        .charEq (2, { { "HpfFreq", 30 }, { "BodyGain", 2 }, { "BodyFreq", 100 }, { "PresGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 2 }, { 0, "Release", 120 }, { 0, "Makeup", 5 } })
        .noDeEss().zerem (7, 0.5f).tape (2, 0, 0.65f, 1, true).lim (-3)));

    //================================================================ BASS
    const char* bass = "Bass";
    add (p ("Bass DI Solid", bass, B().lowCut (30, 2).noReso().bell (2, 250, -2, 1.2f).bell (5, 800, 1.5f, 1)
        .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 2 }, { "BodyFreq", 80 }, { "PresGain", 1.5f }, { "PresFreq", 2000 } })
        .comps ({ 1, 0 }, { { 1, "Thresh", -18 }, { 0, "Thresh", -10 }, { 0, "Ratio", 4 }, { 0, "Attack", 5 }, { 0, "Release", 150 } })
        .noDeEss().zerem (8, 0.5f).tape (1, 3, 0.5f).safety()));
    add (p ("Bass Amp Growl", bass, B().lowCut (35, 2).noReso().bell (3, 500, -2, 1.3f).bell (5, 1200, 2.5f, 1)
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1.5f }, { "BodyFreq", 100 }, { "PresGain", 3 }, { "PresFreq", 1500 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 6 }, { 0, "Attack", 3 }, { 0, "Release", 120 }, { 0, "Makeup", 3 } })
        .noDeEss().zerem (8, 0.55f).tape (2, 3, 0.6f).safety()));
    add (p ("808 Control", bass, B().lowCut (28, 3).noReso().bell (3, 250, -3, 1.2f).highCut (6000)
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 3 }, { "LowAtten", 1 } })
        .comps ({ 1 }, { { 1, "Thresh", -16 } })
        .noDeEss().zerem (8, 0.6f).lim (-2, -1, 0, 2)));
    add (p ("Synth Bass Tight", bass, B().lowCut (30, 2).noReso().bell (2, 200, -2, 1)
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 60 }, { "PresGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -14 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 0, "Release", 100 } })
        .noDeEss().zerem (8, 0.5f).safety()));
    add (p ("Upright Bass Woody", bass, B().lowCut (40).reso (0.2f, 150, 3000).bell (3, 300, -1.5f, 1.2f)
        .charEq (2, { { "HpfFreq", 30 }, { "BodyGain", 1.5f }, { "BodyFreq", 100 }, { "PresGain", 1 }, { "PresFreq", 1800 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .noDeEss().noZerem().tape (0, 2, 0.4f).safety()));

    //================================================================ GUITARS
    const char* gtr = "Guitars";
    add (p ("Acoustic Sparkle", gtr, B().lowCut (90, 2).reso (0.4f, 300, 10000).bell (2, 220, -2.5f, 1.2f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 1 }, { "AirGain", 3.5f }, { "Silk", 0.4f } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .peakCatch().noDeEss().zerem (9, 0.4f).safety()));
    add (p ("Acoustic Strum Bed", gtr, B().lowCut (120, 2).reso (0.45f, 250, 9000).bell (3, 350, -3, 1)
        .charEq (3, { { "HpfFreq", 110 }, { "AirGain", 3 }, { "Silk", 0.35f } })
        .comps ({ 2 }, { { 2, "Thresh", -18 }, { 2, "Tc", 1 }, { 2, "Makeup", 2 } })
        .noDeEss().zerem (9, 0.45f).tape (0, 1, 0.45f).safety()));
    add (p ("Fingerpicked Warmth", gtr, B().lowCut (70).reso (0.3f).bell (5, 3000, -1, 2)
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 1.5f }, { "BodyFreq", 180 }, { "AirGain", 2 } })
        .comps ({ 1 }, { { 1, "Thresh", -16 } })
        .peakCatch().noDeEss().noZerem().tape (0, 2, 0.4f).safety()));
    add (p ("Electric Rhythm Crunch", gtr, B().lowCut (100, 2).reso (0.35f, 1500, 8000).bell (3, 400, -2, 1.3f).highCut (11000)
        .charEq (1, { { "HpfFreq", 90 }, { "PresGain", 2 }, { "PresFreq", 2500 } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .noDeEss().zerem (9, 0.5f).tape (1, 3, 0.55f).safety()));
    add (p ("Electric Lead Smooth", gtr, B().lowCut (110, 2).reso (0.45f, 2000, 9000, 0.5f).bell (5, 3500, -2, 2.5f).dyn (5, -24, -4).highCut (10000)
        .charEq (2, { { "HpfFreq", 100 }, { "BodyGain", 1 }, { "PresGain", 1 }, { "PresFreq", 1800 } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 150 }, { 0, "Makeup", 2 } })
        .noDeEss().zerem (9, 0.45f).tape (1, 2, 0.5f).safety()));
    add (p ("Clean Funk", gtr, B().lowCut (140, 2).reso (0.3f).bell (5, 3000, 2.5f, 1)
        .charEq (1, { { "HpfFreq", 120 }, { "PresGain", 2 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.5f }, { 0, "Release", 80 }, { 0, "Makeup", 4 } })
        .peakCatch().noDeEss().zerem (9, 0.4f).safety()));
    add (p ("Heavy Rhythm Wall", gtr, B().lowCut (90, 3).reso (0.5f, 1500, 7000, 0.5f, 0.6f).bell (3, 500, -2.5f, 1.4f).highCut (9500, 2)
        .charEq (1, { { "HpfFreq", 80 }, { "BodyGain", 1 }, { "BodyFreq", 120 }, { "PresGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 0 }, { 2, "Makeup", 1 } })
        .noDeEss().zerem (9, 0.5f).safety()));
    add (p ("Guitar Bus Tape", gtr, B().noReso().noTahor()
        .charEq (2, { { "HpfFreq", 60 }, { "BodyGain", 1 }, { "PresGain", 1 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .noDeEss().zerem (9, 0.3f).tape (2, 2, 0.5f).lim (-1)));

    //================================================================ KEYS & SYNTHS
    const char* keys = "Keys & Synths";
    add (p ("Piano Clear", keys, B().lowCut (50).reso (0.3f, 300, 8000).bell (3, 350, -2, 1)
        .charEq (3, { { "HpfFreq", 40 }, { "PresGain", 1 }, { "AirGain", 2 }, { "Silk", 0.3f } })
        .comps ({ 1 }, { { 1, "Thresh", -16 } })
        .noDeEss().zerem (10, 0.4f).safety()));
    add (p ("Piano Pop Bright", keys, B().lowCut (90, 2).reso (0.35f).bell (3, 300, -3, 1)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 2 }, { "AirGain", 3.5f }, { "Silk", 0.4f } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 120 }, { 0, "Makeup", 2 } })
        .noDeEss().zerem (10, 0.5f).lim (-1)));
    add (p ("Piano Vintage Warm", keys, B().lowCut (40).reso (0.2f)
        .charEq (0, { { "LowFreq", 3 }, { "LowBoost", 2 }, { "HighFreq", 3 }, { "HighBoost", 1.5f }, { "AttenFreq", 1 }, { "HighAtten", 2 } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 2 }, { 2, "Makeup", 1 } })
        .noDeEss().noZerem().tape (1, 2, 0.5f, 1, true).safety()));
    add (p ("Rhodes Warmth", keys, B().lowCut (60).reso (0.25f, 800, 6000)
        .charEq (2, { { "HpfFreq", 50 }, { "BodyGain", 2 }, { "BodyFreq", 220 }, { "PresGain", -1 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .noDeEss().zerem (10, 0.4f).tape (1, 2, 0.6f).safety()));
    add (p ("Organ Body", keys, B().lowCut (50).reso (0.3f, 1000, 6000).bell (5, 2500, -2, 1.5f)
        .charEq (2, { { "HpfFreq", 40 }, { "BodyGain", 1.5f }, { "BodyFreq", 150 } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 2 } })
        .noDeEss().noZerem().tape (1, 3, 0.55f).safety()));
    add (p ("Synth Pad Smooth", keys, B().lowCut (120, 2).reso (0.45f, 1000, 12000).bell (3, 400, -2, 0.8f)
        .charEq (3, { { "HpfFreq", 100 }, { "AirGain", 2 }, { "Silk", 0.3f } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 3 } })
        .noDeEss().zerem (10, 0.35f).safety()));
    add (p ("Synth Lead Bite", keys, B().lowCut (150, 2).reso (0.35f).bell (5, 2500, 2.5f, 1.2f)
        .charEq (1, { { "HpfFreq", 120 }, { "PresGain", 3 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.5f }, { 0, "Release", 80 }, { 0, "Makeup", 3 } })
        .noDeEss().zerem (2, 0.5f).tape (1, 0, 0.6f).lim (-1)));
    add (p ("Pluck Sparkle", keys, B().lowCut (200, 2).reso (0.3f).bell (6, 7000, 2.5f, 1)
        .charEq (3, { { "HpfFreq", 160 }, { "AirGain", 3 }, { "Silk", 0.4f } })
        .comps ({ 0 }, { { 0, "Thresh", -14 }, { 0, "Ratio", 3 }, { 0, "Attack", 2 } })
        .peakCatch().noDeEss().zerem (10, 0.4f).safety()));

    //================================================================ STRINGS, BRASS & WINDS
    const char* orch = "Strings, Brass & Winds";
    add (p ("Strings Silky", orch, B().lowCut (60).reso (0.45f, 1500, 9000, 0.5f).bell (5, 3000, -1.5f, 1.5f)
        .charEq (3, { { "HpfFreq", 50 }, { "AirGain", 2.5f }, { "Silk", 0.5f } })
        .comps ({ 1 }, { { 1, "Thresh", -16 } })
        .noDeEss().zerem (10, 0.3f).safety()));
    add (p ("Solo Violin", orch, B().lowCut (180, 2).reso (0.5f, 1500, 8000, 0.5f, 0.6f).bell (5, 2800, -2, 2).dyn (5, -26, -4)
        .charEq (2, { { "HpfFreq", 150 }, { "BodyGain", 1 }, { "BodyFreq", 300 }, { "AirGain", 2 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .noDeEss().noZerem().tape (0, 2, 0.4f).safety()));
    add (p ("Cello Rich", orch, B().lowCut (50).reso (0.3f, 400, 5000).bell (3, 350, -1.5f, 1.2f)
        .charEq (2, { { "HpfFreq", 40 }, { "BodyGain", 1.5f }, { "BodyFreq", 120 }, { "PresGain", 1 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .noDeEss().noZerem().safety()));
    add (p ("Brass Bite", orch, B().lowCut (100, 2).reso (0.4f, 1000, 6000).bell (5, 2500, 2, 1.2f)
        .charEq (1, { { "HpfFreq", 90 }, { "MudGain", -2 }, { "PresGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 0, "Release", 100 }, { 0, "Makeup", 2 } })
        .noDeEss().zerem (2, 0.45f).tape (1, 2, 0.5f).safety()));
    add (p ("Sax Smooth", orch, B().lowCut (90, 2).reso (0.45f, 800, 6000, 0.5f).bell (5, 3000, -2, 2).dyn (5, -24, -4)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1 }, { "BodyFreq", 250 }, { "AirGain", 1 } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .deEss (5500, -30, 5).zerem (3, 0.4f).tape (0, 2, 0.45f).safety()));
    add (p ("Flute Air", orch, B().lowCut (250, 2).reso (0.35f, 1000, 10000).bell (3, 700, -1, 1)
        .charEq (3, { { "HpfFreq", 200 }, { "AirGain", 2.5f }, { "Silk", 0.35f } })
        .comps ({ 1 }, { { 1, "Thresh", -18 } })
        .breath (-6).deEss (7000, -32, 6).noZerem().safety()));

    //================================================================ BUSES
    const char* buses = "Buses";
    add (p ("Mix Bus Glue", buses, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 20 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().zerem (11, 0.3f).lim (-1)));
    add (p ("Mix Bus Tape Glue", buses, B().noReso().noTahor()
        .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 0.5f }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 2 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().zerem (11, 0.25f).tape (0, 1, 0.45f).lim (-1)));
    add (p ("Vocal Bus Sheen", buses, B().lowCut (80).reso (0.25f)
        .charEq (3, { { "HpfFreq", 70 }, { "AirGain", 2 }, { "Silk", 0.35f } })
        .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .deEss (7000, -32, 5, 1, true).zerem (0, 0.3f).lim (-1)));
    add (p ("Music Bus Warm", buses, B().noReso().noTahor()
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 1.5f }, { "HighFreq", 5 }, { "HighBoost", 1.5f }, { "HighBw", 7 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 2 } })
        .scHpf (1).noDeEss().noZerem().tape (1, 3, 0.45f).lim (-1)));
    add (p ("Parallel Vocal Crush", buses, B().lowCut (120, 2).reso (0.3f)
        .charEq (1, { { "HpfFreq", 100 }, { "PresGain", 3 }, { "AirGain", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -30 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.2f }, { 0, "Release", 60 }, { 0, "Makeup", 10 }, { 0, "Mix", 0.3f } })
        .deEss (6500, -34, 10).zerem (6, 0.5f).lim (-2)));

    //================================================================ MASTERING
    // Tip: for mastering, also switch the Tahor EQ to Linear and Limiter OS to 2x (session settings, not saved in presets).
    const char* master = "Mastering";
    add (p ("Master Transparent", master, B().noReso().lowCut (25, 2)
        .noCharEq()
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Mix", 0.6f } })
        .scHpf (2).noDeEss().noZerem().lim (-3, -1, 0, 1)));
    add (p ("Master Loud Pop", master, B().reso (0.12f, 2000, 9000, 0.3f, 0.7f).lowCut (28, 2).bell (3, 300, -1, 0.7f)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1.5f }, { "AirFreq", 14000 }, { "Silk", 0.25f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().zerem (11, 0.35f).lim (-7, -1, 1, 2)));
    add (p ("Master Hip-Hop Punch", master, B().noReso().lowCut (25, 3).lowShelf (1, 70, 1.5f)
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 60 }, { "PresGain", 1 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 0 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().zerem (11, 0.4f).tape (1, 3, 0.4f, 0.5f).lim (-7, -1, 1, 2)));
    add (p ("Master Warm Tape", master, B().noReso().lowCut (25, 2)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 1.5f }, { "HighBw", 7 } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 2 }, { 2, "Mix", 0.7f } })
        .scHpf (2).noDeEss().noZerem().tape (0, 1, 0.45f).lim (-4, -1, 0, 1)));
    add (p ("Master Streaming -14 LUFS", master, B().noReso().lowCut (25, 2)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Mix", 0.6f } })
        .scHpf (2).noDeEss().zerem (11, 0.25f).lim (-2, -1, 0, 1)));
    add (p ("Master Rock Glue", master, B().noReso().lowCut (28, 2).bell (3, 400, -1, 0.8f)
        .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 0.5f }, { "PresGain", 1 }, { "AirGain", 1 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (1).noDeEss().zerem (11, 0.35f).tape (1, 3, 0.4f, 0.5f).lim (-5, -1, 1, 1)));
    add (p ("Master Acoustic Gentle", master, B().noReso().lowCut (25, 1)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.25f } })
        .comps ({ 1 }, { { 1, "Thresh", -12 }, { 1, "Ratio", 2 }, { 1, "Attack", 30 }, { 1, "Release", 300 }, { 1, "Character", 1 }, { 1, "Makeup", 1.5f }, { 1, "Mix", 0.5f } })
        .noDeEss().noZerem().lim (-2, -1)));
    add (p ("Master EDM Loud", master, B().reso (0.1f, 2500, 10000, 0.3f, 0.7f).lowCut (25, 3).lowShelf (1, 50, 1)
        .charEq (1, { { "HpfFreq", 20 }, { "PresGain", 1 }, { "AirGain", 2 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 0 }, { 2, "Makeup", 1.5f } })
        .scHpf (2).noDeEss().zerem (11, 0.45f).lim (-9, -1, 1, 3)));
    add (p ("Master Vintage Vinyl", master, B().noReso().lowCut (35, 2).highCut (15000, 1)
        .charEq (0, { { "LowFreq", 2 }, { "LowBoost", 2 }, { "HighFreq", 3 }, { "HighBoost", 1 }, { "AttenFreq", 1 }, { "HighAtten", 2 } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 3 } })
        .noDeEss().noZerem().tape (2, 0, 0.45f, 1, true).lim (-3, -1, 0, 1)));
    add (p ("Master Gospel Live", master, B().reso (0.12f, 1500, 8000).lowCut (30, 2)
        .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 0.5f }, { "AirGain", 1.5f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().zerem (11, 0.3f).lim (-5, -1, 1, 1)));
    add (p ("Master De-Harsh", master, B().reso (0.25f, 2000, 10000, 0.4f, 0.6f).lowCut (25, 2).bell (5, 3000, -1.5f, 1.5f).dyn (5, -24, -3)
        .charEq (3, { { "HpfFreq", 20 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Mix", 0.6f } })
        .scHpf (2).deEss (7500, -26, 3, 1, true).noZerem().lim (-3, -1, 0, 1)));

   #endif   // MISHKAL_VERSION < 50: the original library

   #if defined (MISHKAL_VERSION) && MISHKAL_VERSION >= 47
    //================================================================ GOLIATH SIGNATURE (V47): the newer modules
    // Perach (tone), Esh (tape), the 1964 PK's own in / out, LE-76 Blue Stripe / Black and LE-RV.
   #if MISHKAL_VERSION >= 49
    const char* sig = "LE Goliath Signature";
   #else
    const char* sig = "Goliath Signature";
   #endif
    add (p ("Perach Pop Lead", sig, B().lowCut (90).reso (0.4f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 1.5f }, { "AirGain", 2 }, { "Silk", 0.3f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.8f }, { 0, "Release", 120 }, { 0, "Makeup", 2 },
                            { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } })
        .deEss (6500, -32).perach (6, 0, -1, 1, 2, 5).lim (-3)));
    add (p ("Blue Stripe Rap", sig, B().lowCut (100, 2).reso (0.45f)
        .charEq (1, { { "HpfFreq", 90 }, { "PresGain", 3 }, { "PresFreq", 3200 }, { "MudGain", -3 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.3f }, { 0, "Release", 80 }, { 0, "Makeup", 5 } })
        .fetRev (1).pk (4).deEss (6500, -34).perach (5, 0, -1.5f, 2, 1, 7).esh (0, 35, 1).lim (-4)));
    add (p ("Black Smooth Ballad", sig, B().lowCut (80).reso (0.3f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 2.5f }, { "HighBw", 7 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 250 }, { 0, "Makeup", 2 },
                            { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 1.5f } })
        .fetRev (2).breath (-6).deEss (6500, -31, 6).perach (4, 0.5f, 0, 0.5f, 1.5f, 3).esh (1, 30, 0).lim (-1.5f)));
    add (p ("LE-RV One-Knob Vocal", sig, B().lowCut (90).reso (0.35f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 1.5f }, { "AirGain", 2 }, { "Silk", 0.3f } })
        .comps ({ 1 }, { { 1, "Thresh", -24 } }).rv (-24)
        .deEss (6500, -31).zerem (0, 0.4f).lim (-2).output (-8)));
    add (p ("LE-RV Podcast Leveler", sig, B().lowCut (80, 2).reso (0.35f)
        .charEq (1, { { "HpfFreq", 70 }, { "MudGain", -2 }, { "PresGain", 2 }, { "PresFreq", 3000 } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -28 }, { 2, "Thresh", -14 }, { 2, "Tc", 1 } }).rv (-28)
        .breath (-8).rider (6).deEss (6000, -30).perach (5, -1, 0, 1.5f, 0, 4).lim (-3).output (-4)));
    add (p ("Esh Ember Grit", sig, B().lowCut (100, 2).reso (0.4f)
        .charEq (2, { { "HpfFreq", 90 }, { "MudGain", -2 }, { "PresGain", 2.5f }, { "PresFreq", 2500 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 6 }, { 0, "Attack", 0.4f }, { 0, "Release", 70 }, { 0, "Makeup", 4 } })
        .fetRev (1).pk (3).deEss (6000, -30).zerem (2, 0.4f).esh (4, 70, 1, 2, -2).lim (-3, -1, 0, 1).output (-5)));
    add (p ("Esh Glow Air", sig, B().lowCut (110, 2).reso (0.45f)
        .charEq (3, { { "HpfFreq", 100 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -20 }, { 2, "Thresh", -14 }, { 2, "Tc", 1 } })
        .deEss (7000, -32, 8, 1).perach (5, 0, -1, 0.5f, 2, 5).esh (3, 45, 2).lim (-2.5f)));
    add (p ("R&B Warm Esh", sig, B().lowCut (85).reso (0.35f).bell (2, 250, -1.5f, 1.2f)
        .charEq (3, { { "HpfFreq", 75 }, { "BodyGain", 1 }, { "AirGain", 3 }, { "Silk", 0.5f } })
        .comps ({ 1, 2 }, { { 1, "Thresh", -22 }, { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 1.5f } })
        .breath (-8).rider (5).deEss (7000, -31, 8, 1).perach (5, 0.5f, 0.5f, 0, 2, 4).esh (1, 40, 0).lim (-2.5f).output (-2.5f)));
    add (p ("Trap Peak Catch", sig, B().lowCut (110, 2).reso (0.5f).bell (3, 400, -2.5f, 1.4f)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 3 }, { "PresFreq", 4000 }, { "AirGain", 3.5f }, { "Silk", 0.45f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.2f }, { 0, "Release", 60 }, { 0, "Makeup", 4 },
                            { 2, "Thresh", -14 }, { 2, "Tc", 0 } })
        .fetRev (1).pk (6).scHpf (2).deEss (7500, -34, 10).perach (6, 0, -1.5f, 1.5f, 2, 8).lim (-5, -1, 0, 1)));
    add (p ("Gospel Perach Lift", sig, B().lowCut (90).reso (0.45f).bell (5, 2800, -1.5f, 2.5f).dyn (5, -26, -4)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "PresGain", 1.5f }, { "AirGain", 2 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 2, "Thresh", -16 }, { 2, "Tc", 2 }, { 2, "Makeup", 2 } })
        .pk (3).rider (5).deEss (6500, -31).perach (6, 0, 0.5f, 1, 2.5f, 5).esh (2, 35, 1).lim (-3)));
    add (p ("Worship Pad Vocal", sig, B().lowCut (80).reso (0.3f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "HighFreq", 5 }, { "HighBoost", 2 }, { "HighBw", 7 } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 3 }, { 2, "Makeup", 1.5f } })
        .breath (-6).deEss (6500, -31, 6).perach (4, 0, -0.5f, 0, 2, 2).esh (3, 30, 1).lim (-1.5f)));
    add (p ("Afro Glow", sig, B().lowCut (100, 2).reso (0.4f)
        .charEq (3, { { "HpfFreq", 90 }, { "PresGain", 2 }, { "AirGain", 3 }, { "Silk", 0.35f } })
        .comps ({ 0, 1 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 1 }, { 1, "Thresh", -20 } })
        .fetRev (1).deEss (7000, -32).perach (5, 0, -1, 1, 2.5f, 6).esh (3, 40, 2).lim (-3).output (-3)));
    add (p ("Drum Bus Esh Punch", sig, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1 }, { "BodyFreq", 80 }, { "AirGain", 1.5f } })
        .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 100 }, { 0, "Makeup", 2 }, { 0, "Mix", 0.5f } })
        .fetRev (1).pk (4).scHpf (2).noDeEss().noZerem().esh (2, 55, 1).lim (-3, -1, 0, 1).output (-6)));
    add (p ("Bass Esh Weight", sig, B().lowCut (30, 2).noReso().bell (3, 300, -2, 1.2f)
        .charEq (2, { { "HpfFreq", 25 }, { "BodyGain", 1.5f }, { "BodyFreq", 90 } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 1 }, { 2, "Makeup", 1.5f } })
        .noDeEss().noZerem().esh (1, 55, 0).lim (-2).output (-2)));
    add (p ("Mix Bus Perach Polish", sig, B().noReso().noTahor()
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "Silk", 0.2f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().perach (3, 0, -0.5f, 0, 1, 2, 0.7f).esh (0, 25, 1).lim (-1)));
    add (p ("Master Esh Loud", sig, B().reso (0.12f, 2000, 9000, 0.3f, 0.7f).lowCut (28, 2)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1.5f }, { "AirFreq", 14000 }, { "Silk", 0.25f } })
        .comps ({ 2 }, { { 2, "Thresh", -12 }, { 2, "Tc", 1 }, { 2, "Makeup", 1 } })
        .scHpf (2).noDeEss().perach (2.5f, 0, -0.5f, 0, 1, 2, 0.6f).esh (3, 30, 1).lim (-7, -1, 1, 2).output (-2)));
   #if MISHKAL_VERSION >= 49
    add (p ("LE-STA Velvet Vocal", sig, B().lowCut (90).reso (0.35f)
        .charEq (3, { { "HpfFreq", 80 }, { "PresGain", 1.5f }, { "AirGain", 2.5f }, { "Silk", 0.35f } })
        .comps ({ 2 }, {}).sta (6, 2, 1, 3)
        .breath (-6).deEss (6500, -31).perach (4, 0, -0.5f, 0.5f, 1.5f, 4).lim (-2)));
    add (p ("LE-STA Bass Thump", sig, B().lowCut (30, 2).noReso().bell (3, 300, -1.5f, 1.2f)
        .charEq (2, { { "HpfFreq", 25 }, { "BodyGain", 1 }, { "BodyFreq", 90 } })
        .comps ({ 2 }, {}).sta (4, 0, 1, 2)
        .noDeEss().noZerem().lim (-2)));
   #endif
   #if MISHKAL_VERSION >= 51
    // V51: more signature chains - the new units stacked the way they're meant to be used
    add (p ("Blue x STA Vocal Stack", sig, B().lowCut (95).reso (0.4f).bell (2, 300, -1.5f, 1.3f)
        .charEq (3, { { "HpfFreq", 85 }, { "PresGain", 2 }, { "AirGain", 3 }, { "Silk", 0.4f } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.5f }, { 0, "Release", 100 }, { 0, "Makeup", 3 } })
        .fetRev (1).sta (4, 2, 1, 0).pk (3).rider (4).deEss (6800, -32).perach (5, 0, -1, 1, 2, 5).esh (3, 30, 1).lim (-3)));
    add (p ("Black 76 Silk Air", sig, B().lowCut (100, 2).reso (0.45f).bell (5, 3000, -1.5f, 3).dyn (5, -28, -4)
        .charEq (3, { { "HpfFreq", 90 }, { "PresGain", 1 }, { "AirGain", 4 }, { "AirFreq", 14000 }, { "Silk", 0.55f } })
        .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 2 }, { 0, "Release", 180 }, { 0, "Makeup", 3 } }).fetRev (2)
        .breath (-6).deEss (7200, -33, 9, 1).perach (4, -0.5f, -1, 0, 2.5f, 4).esh (3, 35, 2).lim (-2.5f)));
    add (p ("LE-STA Gospel Lift", sig, B().lowCut (90).reso (0.45f).bell (5, 2800, -1.5f, 2.5f).dyn (5, -26, -4)
        .charEq (2, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "PresGain", 1.5f }, { "AirGain", 2.5f } })
        .comps ({ 2 }, {}).sta (7, 2, 1, 0).pk (3).rider (5).deEss (6500, -31).perach (6, 0, 0.5f, 1, 2.5f, 5).esh (2, 35, 1).lim (-3)));
    add (p ("Velvet Ballad STA", sig, B().lowCut (80).reso (0.3f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 2 }, { "HighBw", 7 } })
        .comps ({ 2 }, {}).sta (5, 0, 3, 0).breath (-6).deEss (6500, -31, 6).perach (3, 0.5f, 0.5f, 0, 1.5f, 2).esh (1, 35, 0).lim (-1.5f)));
    add (p ("Perach Dark Rap", sig, B().lowCut (100, 2).reso (0.45f).bell (6, 7500, -2, 1)
        .charEq (2, { { "HpfFreq", 90 }, { "BodyGain", 2 }, { "BodyFreq", 150 }, { "MudGain", -2 }, { "PresGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.4f }, { 0, "Release", 90 }, { 0, "Makeup", 4 } }).fetRev (1)
        .pk (4).scHpf (2).deEss (6000, -32).perach (6, 1.5f, 1, -1, -1.5f, 6).esh (4, 40, 0, 0, -1).lim (-4).output (-1.5f)));
    add (p ("Ember Rock Scream", sig, B().lowCut (120, 2).reso (0.5f).bell (4, 1200, 2, 0.9f)
        .charEq (1, { { "HpfFreq", 110 }, { "MudGain", -2.5f }, { "PresGain", 3 }, { "PresFreq", 2500 } })
        .comps ({ 0, 2 }, { { 0, "Thresh", -26 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.3f }, { 0, "Release", 60 }, { 0, "Makeup", 6 } })
        .fetRev (1).sta (3, 2, 0, 0).pk (6).deEss (5800, -30, 10).esh (4, 75, 1, 2, -3).lim (-4, -1, 0, 2).output (-6.5f)));
    add (p ("LE-RV Radio Ready", sig, B().lowCut (90, 2).reso (0.4f).lowShelf (1, 120, 1.5f)
        .charEq (1, { { "HpfFreq", 80 }, { "BodyGain", 1 }, { "PresGain", 2 }, { "PresFreq", 3200 }, { "AirGain", 1.5f } })
        .comps ({ 1 }, { { 1, "Thresh", -24 } }).rv (-24).pk (3).gate (-55).breath (-8).deEss (6500, -31)
        .perach (5, 0.5f, 0, 1, 1, 5).esh (0, 35, 1).lim (-3).output (-11)));
    add (p ("Spoken Word Intimate", sig, B().lowCut (75, 2).reso (0.3f)
        .charEq (2, { { "HpfFreq", 65 }, { "BodyGain", 2 }, { "BodyFreq", 160 }, { "AirGain", 1 } })
        .comps ({ 2 }, {}).sta (4, 1, 2, 0).gate (-55).breath (-6).deEss (6200, -32, 6).perach (3, 0.5f, 0.5f, 0, 0.5f, 3).esh (1, 30, 0).lim (-1.5f)));
    add (p ("Dancehall Lead Bright", sig, B().lowCut (110, 2).reso (0.45f)
        .charEq (3, { { "HpfFreq", 100 }, { "PresGain", 2.5f }, { "PresFreq", 3500 }, { "AirGain", 3 }, { "Silk", 0.35f } })
        .comps ({ 0, 1 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 4 }, { 0, "Attack", 0.5f }, { 0, "Makeup", 3 } }).fetRev (1).rv (-16)
        .pk (4).deEss (7000, -33).perach (5, 0, -1, 1.5f, 2, 7).esh (3, 40, 2).lim (-4).output (-10)));
    add (p ("Choir Wash STA", sig, B().lowCut (130).reso (0.45f).bell (2, 400, -2, 1)
        .charEq (3, { { "HpfFreq", 110 }, { "AirGain", 3 }, { "Silk", 0.45f } })
        .comps ({ 2 }, {}).sta (4, 0, 3, 0).deEss (7000, -34, 8, 1).perach (4, -1, -0.5f, 0, 2.5f, 2).esh (3, 30, 1).lim (-2).output (3)));
    add (p ("808 PK Slam", sig, B().lowCut (28, 3).noReso().bell (3, 250, -3, 1.2f).highCut (7000)
        .charEq (1, { { "HpfFreq", 25 }, { "BodyGain", 1.5f }, { "BodyFreq", 60 } })
        .comps ({ 1 }, { { 1, "Thresh", -16 }, { 1, "Ratio", 4 }, { 1, "Attack", 15 }, { 1, "Release", 150 }, { 1, "Character", 0 } })
        .pk (8).noDeEss().noZerem().esh (2, 50, 1).lim (-4, -1, 0, 2).output (-9)));
    add (p ("Kick STA Thump", sig, B().lowCut (30, 2).noReso().bell (2, 350, -3, 1.5f).bell (5, 3500, 2.5f, 1.2f)
        .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2.5f }, { "LowAtten", 1 } })
        .comps ({ 2 }, {}).sta (6, 0, 0, 0).noDeEss().noZerem().esh (2, 35, 1).lim (-2).output (-1.5f)));
    add (p ("Snare Ember Crack", sig, B().lowCut (90, 2).reso (0.3f, 400, 6000).bell (2, 200, 2, 1.2f).bell (6, 5500, 3, 1)
        .charEq (1, { { "HpfFreq", 80 }, { "BodyGain", 1.5f }, { "BodyFreq", 200 }, { "PresGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 8 }, { 0, "Attack", 3 }, { 0, "Release", 80 }, { 0, "Makeup", 4 } }).fetRev (1)
        .pk (4).noDeEss().noZerem().esh (4, 60, 1, 0, -2).lim (-2).output (-3.5f)));
    add (p ("Keys Glow Perach", sig, B().lowCut (60).reso (0.3f, 400, 8000).bell (3, 350, -1.5f, 1)
        .charEq (3, { { "HpfFreq", 50 }, { "AirGain", 2 }, { "Silk", 0.35f } })
        .comps ({ 2 }, {}).sta (2, 0, 2, 0).noDeEss().perach (4, 0, -0.5f, 0, 1.5f, 3).esh (3, 30, 1).lim (-1).output (4)));
    add (p ("Mix Bus Black Parallel", sig, B().noReso().noTahor()
        .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 0.5f }, { "AirGain", 1 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 150 }, { 0, "Makeup", 4 }, { 0, "Mix", 0.3f } }).fetRev (2)
        .scHpf (2).noDeEss().noZerem().esh (0, 25, 1).lim (-1)));
    add (p ("Master LE-STA Glue", sig, B().noReso().lowCut (25, 2)
        .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1.5f }, { "Silk", 0.25f } })
        .comps ({ 2 }, {}).sta (2, 1, 3, 0, 0.5f).scHpf (2).noDeEss().perach (2, 0, -0.5f, 0, 0.5f, 2, 0.5f).esh (3, 20, 1).lim (-6, -1, 1, 2)));
   #endif
   #endif

    //================================================================ CREATIVE FX
    const char* fx = "Creative FX";
    add (p ("Telephone Voice", fx, B().lowCut (400, 3).highCut (3400, 3).bell (4, 1500, 4, 1).reso (0.3f)
        .charEq (1, { { "HpfFreq", 300 }, { "PresGain", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 10 }, { 0, "Attack", 0.3f }, { 0, "Makeup", 8 } })
        .noDeEss().zerem (2, 0.5f).tape (2, 0, 0.8f).lim (-3)));
    add (p ("AM Radio", fx, B().lowCut (300, 3).highCut (5000, 3).bell (4, 1800, 3, 0.8f)
        .charEq (0, { { "HighFreq", 1 }, { "HighBoost", 3 } })
        .comps ({ 0 }, { { 0, "Thresh", -26 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.2f }, { 0, "Makeup", 8 } })
        .noDeEss().zerem (6, 0.6f).tape (2, 0, 0.9f, 1, true).lim (-4, -1, 0, 3)));
    add (p ("Lo-Fi Cassette", fx, B().lowCut (60).highCut (9000, 2).noReso()
        .charEq (2, { { "HpfFreq", 50 }, { "BodyGain", 2 }, { "AirGain", -3 } })
        .comps ({ 2 }, { { 2, "Thresh", -16 }, { 2, "Tc", 4 }, { 2, "Makeup", 2 } })
        .noDeEss().noZerem().tape (2, 0, 0.75f, 1, true).lim (-2, -1, 0, 2)));
    add (p ("Megaphone", fx, B().lowCut (600, 3).highCut (4000, 3).bell (4, 2000, 6, 1.5f)
        .charEq (1, { { "HpfFreq", 400 }, { "PresGain", 4 } })
        .comps ({ 0 }, { { 0, "Thresh", -30 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.1f }, { 0, "Makeup", 10 } })
        .noDeEss().zerem (6, 0.7f).tape (2, 0, 1.0f).lim (-6, -1, 0, 4)));
    add (p ("Dark Underwater", fx, B().highCut (1200, 3).lowCut (40).noReso()
        .noCharEq()
        .comps ({ 2 }, { { 2, "Thresh", -18 }, { 2, "Tc", 5 }, { 2, "Makeup", 2 } })
        .noDeEss().noZerem().tape (2, 2, 0.5f).lim (-2)));
    add (p ("Parallel Saturation Sauce", fx, B().lowCut (60).reso (0.2f)
        .charEq (1, { { "HpfFreq", 50 }, { "PresGain", 2 }, { "AirGain", 2 } })
        .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.4f }, { 0, "Makeup", 8 }, { 0, "Mix", 0.25f } })
        .noDeEss().zerem (6, 0.5f, 0.6f).tape (2, 3, 0.85f, 0.5f).lim (-2, -1, 0, 2)));

   #if MISHKAL_VERSION >= 73
    //================================================================ V73: TESTED ON REAL AUDIO
    // Each one was run through the strip on a real recording (a dry male lead, a stereo vocal stack, hip-hop and live
    // drum loops, an 808, an electric bass, acoustic and electric guitars, a full beat and a finished song) and tuned
    // until the measurements hit their targets: level, loudness range, true peak, mud, presence and sibilance.
    // True Peak is on wherever the limiter is, because bright, hard-limited material went up to +1.5 dBTP without it.
    {
        auto tpLim = [] (B b, float th, float ceil = -1.0f) { return b.lim (th, ceil).set (id::limTp, 1); };
        auto xl = [] (B b, int style, float character = 5.0f, float transients = 0.0f, float upward = 0.0f)
        {
            return b.set (id::limMax, 1).set (id::limMaxStyle, (float) style).set (id::limCharacter, character)
                    .set (id::limTransients, transients).set (id::limUpward, upward);
        };
        // Lead vocals (dry male lead: level swing 9.5 LU -> 2.4-3.6 LU, mud down, presence up, esses no harsher)
        add (tested ("Tested Lead Vocal Modern", tpLim (B().lowCut (90, 2).reso (0.15f, 1500, 9000).bell (2, 350, -3.5f, 0.8f)
            .charEq (3, { { "HpfFreq", 20 }, { "PresGain", 2.5f }, { "PresFreq", 3500 }, { "AirGain", 3 }, { "Silk", 0.3f } })
            .comps ({ 0, 1 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 120 }, { 0, "Makeup", 3 },
                                { 1, "Thresh", -28 } })
            .rider (8).scHpf (1).deEss (6500, -36), -3)));
        add (tested ("Tested Lead Vocal Warm R&B", tpLim (B().lowCut (80, 2).reso (0.15f, 1500, 9000).bell (2, 350, -2.5f, 0.8f)
            .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 2 }, { "LowAtten", 1 }, { "HighFreq", 5 }, { "HighBoost", 2.5f }, { "HighBw", 6 } })
            .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 200 }, { 0, "Makeup", 3 } }).fetRev (2)
            .sta (5, 2, 2, 1).rider (8).deEss (6800, -34, 8, 1).esh (1, 35, 0), -2).output (1)));
        add (tested ("Tested Lead Vocal Rap Upfront", tpLim (B().lowCut (100, 2).reso (0.15f, 1500, 9000).bell (2, 350, -3, 0.8f).bell (4, 1000, 1.5f, 0.9f)
            .charEq (2, { { "HpfFreq", 20 }, { "MudGain", -1 }, { "PresGain", 3 }, { "PresFreq", 3000 }, { "AirGain", 2 } })
            .comps ({ 0, 1 }, { { 0, "Thresh", -26 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.8f }, { 0, "Release", 80 }, { 0, "Makeup", 5 },
                                { 1, "Thresh", -26 } }).fetRev (1)
            .rider (6).scHpf (2).deEss (6000, -33).pk (3), -3).output (-1.5f)));
        // Stereo vocal stack: glued (2.2 LU), lows cleared, air on the sides only, a little wider
        add (tested ("Tested Vocal Stack Glue", tpLim (B().lowCut (150, 2).reso (0.15f, 1500, 9000).bell (2, 350, -3, 0.8f)
            .bell (6, 10000, 2.5f, 0.7f).place (6, 2)
            .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 2.5f }, { "Silk", 0.4f } })
            .comps ({ 1 }, { { 1, "Thresh", -24 } }).sta (4, 2, 2, 1, 0.7f).deEss (7000, -36, 9, 1).set (id::outWidth, 1.25f), -2).output (3.5f)));
        // Drums
        add (tested ("Tested Hip-Hop Drum Bus", tpLim (B().noReso().lowCut (28, 2).bell (2, 350, -2, 1).bell (5, 5000, 1.5f, 0.8f)
            .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 80 }, { "PresGain", 1.5f }, { "AirGain", 1 } })
            .comps ({ 0 }, { { 0, "Thresh", -16 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 100 }, { 0, "Makeup", 2 }, { 0, "Mix", 0.5f } })
            .fetRev (2).scHpf (1).esh (4, 25, 1), -3)));
        add (tested ("Tested Hip-Hop Drum Knock", xl (tpLim (B().noReso().lowCut (28, 2).bell (2, 350, -2.5f, 1)
            .charEq (2, { { "HpfFreq", 20 }, { "BodyGain", 1.5f }, { "BodyFreq", 70 }, { "PresGain", 1 } })
            .comps ({ 0 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 8 }, { 0, "Attack", 15 }, { 0, "Release", 80 }, { 0, "Makeup", 3 }, { 0, "Mix", 0.45f } })
            .fetRev (1).scHpf (1), -4), 1, 4, 0.3f)));
        add (tested ("Tested Live Drums Soul", tpLim (B().noReso().lowCut (35, 2).bell (2, 320, -3.5f, 1.2f).dyn (2, -30, -4)
            .bell (3, 480, -2, 1.5f).bell (6, 6400, -1.5f, 3)
            .charEq (0, { { "LowFreq", 1 }, { "LowBoost", 1.5f }, { "LowAtten", 0.5f }, { "HighFreq", 5 }, { "HighBoost", 1.5f }, { "HighBw", 6 } })
            .comps ({ 2 }, { { 2, "Thresh", -14 }, { 2, "Tc", 1 }, { 2, "Makeup", 4 } }).scHpf (1).tape (1, 2, 0.4f), -4)));
        add (tested ("Tested Live Drums Four-on-Floor", tpLim (B().noReso().lowCut (30, 2).bell (2, 250, -1.5f, 1).highShelf (6, 9000, 2.5f)
            .charEq (1, { { "HpfFreq", 20 }, { "MudGain", -1 }, { "PresGain", 1.5f }, { "AirGain", 1.5f } })
            .comps ({ 0 }, { { 0, "Thresh", -14 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 60 }, { 0, "Makeup", 1.5f }, { 0, "Mix", 0.5f } })
            .scHpf (1), -2)));
        // Bass
        add (tested ("Tested 808 Harmonics", tpLim (B().noReso().lowCut (25, 3).bell (3, 250, -2, 1.2f).highCut (8000)
            .tape (2, 3, 0.7f, 1).output (-4), -3)));
        add (tested ("Tested Electric Bass Growl", tpLim (B().noReso().lowCut (35, 2).bell (2, 240, -3, 1.2f).bell (4, 700, 1.5f, 1.2f)
            .charEq (1, { { "HpfFreq", 20 }, { "BodyGain", 1 }, { "BodyFreq", 90 }, { "PresGain", 1.5f }, { "PresFreq", 1500 } })
            .comps ({ 0, 1 }, { { 0, "Thresh", -18 }, { 0, "Ratio", 4 }, { 0, "Attack", 10 }, { 0, "Release", 120 }, { 0, "Makeup", 2 },
                                { 1, "Thresh", -18 } }).fetRev (2).esh (2, 30, 1), -2)));
        // Guitars
        add (tested ("Tested Acoustic Guitar Shine", tpLim (B().lowCut (90, 2).reso (0.15f, 600, 9000).bell (2, 280, -3, 1).bell (4, 1050, -2, 2)
            .charEq (3, { { "HpfFreq", 20 }, { "PresGain", 1 }, { "AirGain", 2 }, { "Silk", 0.2f } })
            .comps ({ 1 }, { { 1, "Thresh", -22 } }).sta (3, 1, 1, 1, 0.6f), -1)));
        add (tested ("Tested Electric Guitar Arp", tpLim (B().lowCut (110, 2).reso (0.15f, 500, 6000).bell (2, 250, -2, 0.9f).bell (5, 2500, 1.5f, 1)
            .charEq (0, { { "HighFreq", 4 }, { "HighBoost", 2 }, { "HighBw", 6 } })
            .comps ({ 1 }, { { 1, "Thresh", -20 } }).tape (1, 2, 0.4f, 0.7f).set (id::outWidth, 1.15f), -1)));
        // A finished beat: gain down first, so the limiter only guards it
        add (tested ("Tested Beat Bus Glue", tpLim (B().noReso().input (-4).lowCut (25, 2).bell (2, 300, -1.5f, 0.8f)
            .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1.5f }, { "Silk", 0.15f } }), 0)));
        // Mastering (a finished song at -13.4 LUFS / 0.0 dBTP): -9.5, -11 and -12 LUFS, all at -1 dBTP, punch kept
        auto masterBase = []
        {
            return B().noReso().lowCut (25, 2).bell (2, 300, -0.8f, 0.7f)
                .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "AirFreq", 14000 }, { "Silk", 0.15f } })
                .comps ({ 2 }, { { 2, "Thresh", -10 }, { 2, "Tc", 1 }, { 2, "Makeup", 0.5f }, { 2, "Mix", 0.5f } }).scHpf (2);
        };
        add (tested ("Tested Master Loud", xl (tpLim (masterBase(), -6), 3, 5, 0.2f, 2)));
        add (tested ("Tested Master Balanced", xl (tpLim (masterBase(), -4), 0)));
        add (tested ("Tested Master Streaming", xl (tpLim (masterBase(), -3), 8)));
       #if MISHKAL_VERSION >= 75
        // V75: one master per mastering compressor, same song. Each one takes 1-2.5 dB of glue off the mix (measured
        // with the limiter off) and Goliath XL with True Peak finishes it: -9 to -12 LUFS at -1 dBTP.
        auto mbus = [] (std::initializer_list<std::pair<const char*, float>> comp)
        {
            auto b = B().noReso().lowCut (25, 2).bell (2, 300, -0.8f, 0.7f)
                .charEq (3, { { "HpfFreq", 20 }, { "AirGain", 1 }, { "AirFreq", 14000 }, { "Silk", 0.15f } })
                .comps ({}, {}).scHpf (2);
            for (auto& [pid, x] : comp) b.set (pid, x);
            return b;
        };
        add (tested ("Tested Master LE-2500 Punch", xl (tpLim (mbus ({ { "a25On", 1 }, { "a25Thresh", -14 }, { "a25Ratio", 2 }, { "a25Attack", 4 },
            { "a25Release", 300 }, { "a25Knee", 2 }, { "a25Thrust", 2 }, { "a25Makeup", 1 } }), -6), 3, 5, 0.15f)));
        add (tested ("Tested Master LE-G Glue", xl (tpLim (mbus ({ { "sslgOn", 1 }, { "sslgThresh", -14 }, { "sslgRatio", 0 }, { "sslgAttack", 5 },
            { "sslgRelease", 4 }, { "sslgMakeup", 1 } }), -6), 0)));
        add (tested ("Tested Master LE-DS1 Clean", xl (tpLim (mbus ({ { "ds1On", 1 }, { "ds1Thresh", -16 }, { "ds1Ratio", 1.5f }, { "ds1Attack", 10 },
            { "ds1Release", 300 }, { "ds1Knee", 1 }, { "ds1Detect", 1 }, { "ds1RelMode", 1 }, { "ds1Makeup", 1 } }), -3), 8)));
        add (tested ("Tested Master LE-IRON Warm", xl (tpLim (mbus ({ { "ironOn", 1 }, { "ironThresh", -14 }, { "ironAttack", 3 }, { "ironRelease", 5 },
            { "ironBias", 0 }, { "ironRect", 1 }, { "ironAirbass", 1 }, { "ironMakeup", 1 } }), -5), 3)));
        add (tested ("Tested Master LE-336 Glue", xl (tpLim (mbus ({ { "n33On", 1 }, { "n33Thresh", -14 }, { "n33Ratio", 0 }, { "n33Release", 4 },
            { "n33Makeup", 1 } }), -5), 3)));
       #endif
    }
   #endif

   #if MISHKAL_VERSION >= 79
    //================================================================ V79: THE LIBRARY, REORGANISED
    // Eleven folders by what you're working on. The best of the earlier presets stay (the ones measured on real
    // audio lose their "Tested" prefix), the rest are retired, and the new ones use what Goliath can do now: the
    // modules in your own order, the four classic EQs (LE-A550, LE-K9, LE-H69, LE-M4), the five mastering
    // compressors and Goliath XL. Every new preset was run on a real recording and checked for level, true peak and tone.
    {
        static const char* const folders[] { "Vocals", "Stacks & BVs", "Rap & Hip-Hop", "Gospel & Worship", "Podcast & Voice",
                                             "Drums", "Bass & 808", "Guitars & Keys", "Buses", "Mastering", "FX" };
        struct Keep { const char* from; const char* folder; const char* to; };
        static const Keep keep[] {
            { "Tested Lead Vocal Modern", "Vocals", "Lead Vocal Modern" },        { "Tested Lead Vocal Warm R&B", "Vocals", "Lead Vocal Warm R&B" },
            { "Pop Lead Airy", "Vocals", nullptr },          { "R&B Lead Silk", "Vocals", nullptr },          { "Soul Lead Warm", "Vocals", nullptr },
            { "Rock Lead Grit", "Vocals", nullptr },         { "Country Lead Clear", "Vocals", nullptr },     { "Indie Lead Intimate", "Vocals", nullptr },
            { "Ballad Lead Smooth", "Vocals", nullptr },     { "Perach Pop Lead", "Vocals", nullptr },        { "Black Smooth Ballad", "Vocals", nullptr },
            { "LE-STA Velvet Vocal", "Vocals", nullptr },    { "LE-RV One-Knob Vocal", "Vocals", nullptr },   { "R&B Warm Esh", "Vocals", nullptr },
            { "Afro Glow", "Vocals", nullptr },              { "Dancehall Lead Bright", "Vocals", nullptr },
            { "Tested Vocal Stack Glue", "Stacks & BVs", "Vocal Stack Glue" },
            { "BV Stacks Tucked", "Stacks & BVs", nullptr }, { "BV Airy Doubles", "Stacks & BVs", nullptr },  { "BV Ad-libs Thin", "Stacks & BVs", nullptr },
            { "BV Gang Shouts", "Stacks & BVs", nullptr },   { "Blue x STA Vocal Stack", "Stacks & BVs", nullptr }, { "Vocal Stack M/S Wide", "Stacks & BVs", nullptr },
            { "Tested Lead Vocal Rap Upfront", "Rap & Hip-Hop", "Rap Lead Upfront" },
            { "Rap Lead Punch", "Rap & Hip-Hop", nullptr },  { "Rap Lead Dark", "Rap & Hip-Hop", nullptr },   { "Trap Lead Air", "Rap & Hip-Hop", nullptr },
            { "Drill Lead Hard", "Rap & Hip-Hop", nullptr }, { "Rap Doubles Tucked", "Rap & Hip-Hop", nullptr }, { "Blue Stripe Rap", "Rap & Hip-Hop", nullptr },
            { "Trap Peak Catch", "Rap & Hip-Hop", nullptr }, { "Perach Dark Rap", "Rap & Hip-Hop", nullptr },
            { "Gospel Lead Power", "Gospel & Worship", nullptr }, { "Gospel Choir Glue", "Gospel & Worship", nullptr }, { "Worship Lead Open", "Gospel & Worship", nullptr },
            { "Praise Break Shout", "Gospel & Worship", nullptr }, { "Gospel Perach Lift", "Gospel & Worship", nullptr }, { "Worship Pad Vocal", "Gospel & Worship", nullptr },
            { "LE-STA Gospel Lift", "Gospel & Worship", nullptr }, { "Choir Wash STA", "Gospel & Worship", nullptr },
            { "Podcast Host Warm", "Podcast & Voice", nullptr }, { "Podcast Guest Cleanup", "Podcast & Voice", nullptr }, { "Voiceover Broadcast", "Podcast & Voice", nullptr },
            { "Audiobook Even", "Podcast & Voice", nullptr }, { "LE-RV Podcast Leveler", "Podcast & Voice", nullptr }, { "LE-RV Radio Ready", "Podcast & Voice", nullptr },
            { "Spoken Word Intimate", "Podcast & Voice", nullptr },
            { "Tested Hip-Hop Drum Bus", "Drums", "Hip-Hop Drum Bus" },           { "Tested Hip-Hop Drum Knock", "Drums", "Hip-Hop Drum Knock" },
            { "Tested Live Drums Soul", "Drums", "Live Drums Soul" },             { "Tested Live Drums Four-on-Floor", "Drums", "Live Drums Four-on-Floor" },
            { "Kick Modern Punch", "Drums", nullptr },       { "Kick STA Thump", "Drums", nullptr },          { "Snare Crack Tube", "Drums", nullptr },
            { "Snare Ember Crack", "Drums", nullptr },       { "Toms Round", "Drums", nullptr },              { "Overheads Silk", "Drums", nullptr },
            { "Drum Room Crush", "Drums", nullptr },
            { "Tested 808 Harmonics", "Bass & 808", "808 Harmonics" },            { "Tested Electric Bass Growl", "Bass & 808", "Electric Bass Growl" },
            { "Bass DI Round", "Bass & 808", nullptr },      { "Bass Amp Grit", "Bass & 808", nullptr },      { "808 Clean Weight", "Bass & 808", nullptr },
            { "808 PK Slam", "Bass & 808", nullptr },        { "Synth Bass Tight", "Bass & 808", nullptr },   { "LE-STA Bass Thump", "Bass & 808", nullptr },
            { "Bass Esh Weight", "Bass & 808", nullptr },
            { "Tested Acoustic Guitar Shine", "Guitars & Keys", "Acoustic Guitar Shine" }, { "Tested Electric Guitar Arp", "Guitars & Keys", "Electric Guitar Arp" },
            { "Electric Crunch Tube", "Guitars & Keys", nullptr }, { "Clean Funk Snap", "Guitars & Keys", nullptr }, { "Guitar Bus Esh", "Guitars & Keys", nullptr },
            { "Piano Clear Modern", "Guitars & Keys", nullptr }, { "Piano Warm Tube", "Guitars & Keys", nullptr }, { "Rhodes Velvet", "Guitars & Keys", nullptr },
            { "Synth Pad Wide", "Guitars & Keys", nullptr }, { "Keys Glow Perach", "Guitars & Keys", nullptr },
            { "Tested Beat Bus Glue", "Buses", "Beat Bus Glue" },
            { "Mix Bus Tube Glue", "Buses", nullptr },       { "Mix Bus Esh Polish", "Buses", nullptr },      { "Mix Bus Perach Polish", "Buses", nullptr },
            { "Mix Bus Black Parallel", "Buses", nullptr },  { "Mix Bus M/S Tight Low End", "Buses", nullptr }, { "Music Bus Warm Tape", "Buses", nullptr },
            { "Vocal Bus Leveler", "Buses", nullptr },       { "Drum Bus Tube Glue", "Buses", nullptr },      { "Drum Bus Esh Punch", "Buses", nullptr },
            { "Drum Bus M/S Width", "Buses", nullptr },
            { "Tested Master Loud", "Mastering", "Master Loud" },                 { "Tested Master Balanced", "Mastering", "Master Balanced" },
            { "Tested Master Streaming", "Mastering", "Master Streaming" },
            { "Tested Master LE-2500 Punch", "Mastering", "Master LE-2500 Punch" }, { "Tested Master LE-G Glue", "Mastering", "Master LE-G Glue" },
            { "Tested Master LE-DS1 Clean", "Mastering", "Master LE-DS1 Clean" },   { "Tested Master LE-IRON Warm", "Mastering", "Master LE-IRON Warm" },
            { "Tested Master LE-336 Glue", "Mastering", "Master LE-336 Glue" },
            { "Master Hip-Hop Weight", "Mastering", nullptr }, { "Master Gospel Live Loud", "Mastering", nullptr }, { "Master Vintage Glow", "Mastering", nullptr },
            { "Master Esh Loud", "Mastering", nullptr },     { "Master LE-STA Glue", "Mastering", nullptr },
            { "Master M/S Side Low Cut + Air", "Mastering", nullptr }, { "Master M/S Wide Tape", "Mastering", nullptr }, { "Master M/S De-Harsh", "Mastering", nullptr },
            { "Telephone Voice", "FX", nullptr },            { "AM Radio", "FX", nullptr },                   { "Lo-Fi Cassette", "FX", nullptr },
            { "Megaphone", "FX", nullptr },                  { "Dark Underwater", "FX", nullptr },            { "Parallel Saturation Sauce", "FX", nullptr },
            { "Ember Rock Scream", "FX", nullptr },
        };

        std::vector<F> lib;
        for (auto& f : list) if (f.name == "Default") lib.push_back (f);
        for (auto& k : keep)
            for (auto& f : list)
                if (f.name == k.from)
                {
                    auto g = f;
                    if (k.to != nullptr) g.name = k.to;
                    g.category = k.folder;
                    lib.push_back (std::move (g));
                }
        auto nu = [&] (const char* name, const char* folder, const B& b) { lib.push_back ({ name, b.v, folder }); };
        // the middle of the chain: 0 Char EQ, 1 Dynamics, 2 De-Ess, 3 Tone, 4 Tape
        constexpr int EQ = 0, DYN = 1, DS = 2, TONE = 3, TAPE = 4;

        //--- Vocals
        // tracked to tape: the machine first, so the compressor hears (and rounds) the tape's soft peaks
        nu ("Lead Vocal Tape First", "Vocals", B().noReso().lowCut (90, 2).bell (2, 320, -2.5f, 0.8f)
            .tape (1, 1, 0.55f, 1.0f).chain ({ TAPE, EQ, DYN, DS, TONE })
            .charEq (2, { { "HpfFreq", 20 }, { "PresGain", 2 }, { "PresFreq", 3500 }, { "AirGain", 2 } })
            .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 120 }, { 0, "Makeup", 3 } })
            .deEss (6500, -34).scHpf (1).xlLim (-5, 0).output (0));
        // compress first, then the air: the boost lands after the squeeze, so it stays open and isn't pumped
        nu ("Lead Vocal Air After Comp", "Vocals", B().noReso().lowCut (90, 2).bell (2, 350, -3, 0.8f)
            .comps ({ 1 }, { { 1, "Thresh", -26 } }).chain ({ DYN, DS, EQ, TONE, TAPE })
            .eq (7, { { "G160", -1 }, { "G2k5", 1 }, { "AirFreq", 2 }, { "AirGain", 5 } })
            .deEss (6800, -34).scHpf (1).xlLim (0, 0).output (0));
        nu ("Lead Vocal API Forward", "Vocals", B().noReso().lowCut (100, 2)
            .eq (4, { { "LowFreq", 4 }, { "LowGain", -2 }, { "LmFreq", 4 }, { "LmGain", -2 }, { "HmFreq", 2 }, { "HmGain", 3 }, { "HighFreq", 3 }, { "HighGain", 2 } })
            .comps ({ 0 }, { { 0, "Thresh", -24 }, { 0, "Ratio", 4 }, { 0, "Attack", 3 }, { 0, "Release", 100 }, { 0, "Makeup", 3 } }).fetRev (1)
            .deEss (6500, -33).scHpf (1).xlLim (-5, 0).output (0));
        nu ("Lead Vocal SSL Console", "Vocals", B().noReso().lowCut (80, 1)
            .eq (5, { { "HpfFreq", 90 }, { "LmfFreq", 350 }, { "LmfGain", -3 }, { "LmfQ", 1.2f }, { "HmfFreq", 3200 }, { "HmfGain", 2.5f }, { "HfFreq", 11000 }, { "HfGain", 2.5f }, { "Mode", 1 } })
            .noOldComps().comp (5, { { "Thresh", -22 }, { "Ratio", 1 }, { "Attack", 3 }, { "Release", 1 }, { "Makeup", 3 } })
            .deEss (6500, -34).scHpf (1).xlLim (-4.5, 0).output (0));
        // vocal "mastering": finish a lead (or a vocal-only bounce) - clean glue, then de-ess last so it catches what the EQ and tape brightened
        nu ("Lead Vocal Finish", "Vocals", B().noReso().lowCut (80, 2).bell (2, 300, -1.5f, 0.8f)
            .eq (7, { { "G650", -0.5f }, { "G2k5", 0.8f }, { "AirFreq", 2 }, { "AirGain", 3 } })
            .noOldComps().comp (6, { { "Thresh", -22 }, { "Ratio", 2 }, { "Attack", 10 }, { "Release", 200 }, { "Makeup", 2 }, { "Knee", 1 }, { "Detect", 1 }, { "RelMode", 1 } })
            .esh (3, 0.25f, 1).chain ({ EQ, DYN, TONE, TAPE, DS }).deEss (6800, -36, 8, 1)
            .xlLim (-5.5, 8).output (0));
        nu ("Lead Vocal Neve Silk", "Vocals", B().noReso().lowCut (90, 2)
            .charEq (2, { { "HpfFreq", 20 }, { "MudGain", -1.5f }, { "PresGain", 1.5f }, { "AirGain", 2.5f } })
            .noOldComps().comp (8, { { "Thresh", -22 }, { "Ratio", 2 }, { "Release", 4 }, { "Makeup", 3 } })
            .deEss (6500, -35).scHpf (1).xlLim (-2, 0).output (0));

        //--- Rap & Hip-Hop
        nu ("Rap Lead Helios Bite", "Rap & Hip-Hop", B().noReso().lowCut (100, 2).bell (2, 320, -2.5f, 0.9f)
            .eq (6, { { "BassFreq", 2 }, { "BassGain", -1.5f }, { "MidFreq", 3 }, { "MidGain", 3 }, { "TrebleGain", 2 } })
            .comps ({ 0 }, { { 0, "Thresh", -26 }, { 0, "Ratio", 8 }, { 0, "Attack", 0.8f }, { 0, "Release", 80 }, { 0, "Makeup", 5 } }).fetRev (1)
            .chain ({ EQ, DYN, TONE, TAPE, DS }).deEss (6200, -33).scHpf (2).pk (2).xlLim (-6, 1, 5, 0.2f).output (0));
        nu ("Rap Lead Two-Stage", "Rap & Hip-Hop", B().noReso().lowCut (100, 2).bell (2, 300, -3, 0.8f)
            .charEq (1, { { "HpfFreq", 20 }, { "PresGain", 2.5f }, { "PresFreq", 3000 }, { "AirGain", 1.5f } })
            .comps ({ 0, 1 }, { { 0, "Thresh", -20 }, { 0, "Ratio", 12 }, { 0, "Attack", 0.4f }, { 0, "Release", 60 }, { 0, "Makeup", 3 }, { 1, "Thresh", -24 } })
            .deEss (6200, -33).scHpf (2).xlLim (-2, 0).output (0));

        //--- Gospel & Worship
        nu ("Gospel Lead Neve Lift", "Gospel & Worship", B().noReso().lowCut (90, 2).bell (2, 340, -2.5f, 0.8f)
            .charEq (2, { { "HpfFreq", 20 }, { "PresGain", 2 }, { "PresFreq", 3500 }, { "AirGain", 3 } })
            .noOldComps().comp (8, { { "Thresh", -22 }, { "Ratio", 2 }, { "Release", 5 }, { "LimThresh", -8 }, { "Makeup", 3 } })
            .rider (6).deEss (6800, -34).scHpf (1).xlLim (-3.5, 0).output (0));
        nu ("Worship Lead Tape Air", "Gospel & Worship", B().noReso().lowCut (90, 2).bell (2, 320, -2, 0.8f)
            .comps ({ 3 }, {}).sta (4, 2, 2, 1).chain ({ DYN, TAPE, EQ, DS, TONE }).tape (1, 2, 0.4f, 1.0f)
            .eq (7, { { "G2k5", 1 }, { "AirFreq", 2 }, { "AirGain", 5 } })
            .deEss (6800, -35).xlLim (-3.5, 0).output (0));

        //--- Stacks & BVs
        nu ("BV Stack Side Air", "Stacks & BVs", B().noReso().lowCut (160, 2).bell (2, 350, -3, 0.8f)
            .eq (7, { { "G160", -1.5f }, { "AirFreq", 2 }, { "AirGain", 6 } }).eqPlace (7, 2)
            .noOldComps().comp (5, { { "Thresh", -20 }, { "Ratio", 1 }, { "Attack", 4 }, { "Release", 4 }, { "Makeup", 2 } })
            .deEss (7000, -36, 9, 1).width (1.3f).xlLim (-4, 0).output (0));

        //--- Drums
        nu ("Drum Bus Helios Smash", "Drums", B().noReso().lowCut (30, 2)
            .eq (6, { { "BassFreq", 1 }, { "BassGain", 3 }, { "MidFreq", 4 }, { "MidGain", 2 }, { "TrebleGain", 3 } })
            .noOldComps().comp (5, { { "Thresh", -24 }, { "Ratio", 2 }, { "Attack", 3 }, { "Release", 0 }, { "Makeup", 6 }, { "Mix", 0.45f } })
            .scHpf (1).xlLim (-3, 1, 5, 0.3f).output (0));
        // tape first: the transients hit the tape and come out rounder, then the 2500 glues what's left
        nu ("Drum Bus Tape Into 2500", "Drums", B().noReso().lowCut (30, 2)
            .tape (1, 2, 0.6f).chain ({ TAPE, EQ, DYN, DS, TONE })
            .eq (4, { { "LowFreq", 2 }, { "LowGain", 2 }, { "HmFreq", 3 }, { "HmGain", 2 }, { "HighFreq", 3 }, { "HighGain", 1.5f } })
            .noOldComps().comp (4, { { "Thresh", -16 }, { "Ratio", 3 }, { "Attack", 3 }, { "Release", 150 }, { "Makeup", 2 }, { "Knee", 1 }, { "Thrust", 2 } })
            .xlLim (-3, 1).output (0));
        nu ("Live Drums SSL Room", "Drums", B().noReso().lowCut (35, 2).bell (2, 400, -2, 1.2f)
            .eq (5, { { "LfFreq", 80 }, { "LfGain", 2 }, { "LmfFreq", 450 }, { "LmfGain", -2 }, { "HmfFreq", 4000 }, { "HmfGain", 2 }, { "HfFreq", 10000 }, { "HfGain", 2 }, { "Mode", 1 } })
            .noOldComps().comp (5, { { "Thresh", -22 }, { "Ratio", 1 }, { "Attack", 3 }, { "Release", 4 }, { "Makeup", 4 }, { "Mix", 0.6f } })
            .xlLim (-5, 1).output (0));

        //--- Bass & 808
        nu ("Bass Iron Weight", "Bass & 808", B().noReso().lowCut (30, 2).bell (3, 250, -2, 1.2f)
            .eq (7, { { "SubGain", 1.5f }, { "G40", 1 }, { "G650", 1 } })
            .noOldComps().comp (7, { { "Thresh", -18 }, { "Attack", 2 }, { "Release", 5 }, { "Bias", 2 }, { "Rect", 1 }, { "Makeup", 3 } })
            .xlLim (-3, 0).output (0));
        // tape first for harmonics, then compress, then clip the peaks: loud on small speakers
        nu ("808 Tape Then Clip", "Bass & 808", B().noReso().lowCut (25, 3).bell (3, 250, -2, 1.2f)
            .tape (1, 2, 0.7f).chain ({ TAPE, EQ, DYN, DS, TONE })
            .noOldComps().comp (4, { { "Thresh", -14 }, { "Ratio", 3 }, { "Attack", 3 }, { "Release", 120 }, { "Makeup", 2 } })
            .xlLim (-4, 5).output (0));

        //--- Guitars & Keys
        nu ("Piano Maag Air", "Guitars & Keys", B().noReso().lowCut (40, 2).bell (2, 300, -1.5f, 0.8f)
            .eq (7, { { "G160", -0.5f }, { "G2k5", 0.8f }, { "AirFreq", 2 }, { "AirGain", 4 } })
            .noOldComps().comp (6, { { "Thresh", -20 }, { "Ratio", 2 }, { "Attack", 15 }, { "Release", 250 }, { "Makeup", 2 }, { "Knee", 1 }, { "Detect", 1 }, { "RelMode", 1 } })
            .xlLim (-1, 0).output (0));
        nu ("Electric Guitar K9 Bite", "Guitars & Keys", B().noReso().lowCut (100, 2)
            .eq (5, { { "HpfFreq", 90 }, { "LmfFreq", 300 }, { "LmfGain", -2.5f }, { "HmfFreq", 2500 }, { "HmfGain", 3 }, { "HmfQ", 1.4f }, { "LpfFreq", 12000 }, { "Mode", 1 } })
            .noOldComps().comp (5, { { "Thresh", -20 }, { "Ratio", 1 }, { "Attack", 4 }, { "Release", 2 }, { "Makeup", 2 } })
            .xlLim (-5, 0).output (0));
        nu ("Acoustic Helios Sparkle", "Guitars & Keys", B().noReso().lowCut (90, 2).bell (2, 250, -2.5f, 1)
            .eq (6, { { "BassFreq", 2 }, { "BassGain", -1 }, { "MidFreq", 2 }, { "MidGain", -1 }, { "TrebleGain", 3 } })
            .noOldComps().comp (6, { { "Thresh", -20 }, { "Ratio", 2 }, { "Attack", 10 }, { "Release", 150 }, { "Makeup", 2 }, { "Knee", 1 }, { "Detect", 1 } })
            .xlLim (-1, 0).output (0));

        //--- Buses
        nu ("Mix Bus SSL Glue", "Buses", B().noReso().lowCut (25, 2)
            .eq (5, { { "LfFreq", 60 }, { "LfGain", 1 }, { "HfFreq", 12000 }, { "HfGain", 1 } })
            .noOldComps().comp (5, { { "Thresh", -16 }, { "Ratio", 1 }, { "Attack", 5 }, { "Release", 4 }, { "Makeup", 1.5f } })
            .scHpf (1).xlLim (-1, 0).output (0));
        nu ("Mix Bus API Punch", "Buses", B().noReso().lowCut (25, 2)
            .eq (4, { { "LowFreq", 3 }, { "LowGain", 1.5f }, { "HmFreq", 2 }, { "HmGain", 1 }, { "HighFreq", 3 }, { "HighGain", 1 } })
            .noOldComps().comp (4, { { "Thresh", -16 }, { "Ratio", 2 }, { "Attack", 4 }, { "Release", 300 }, { "Makeup", 1.5f }, { "Knee", 2 }, { "Thrust", 2 } })
            .xlLim (-1, 1, 5, 0.1f).output (0));
        nu ("Vocal Bus Iron Glue", "Buses", B().noReso().lowCut (100, 2).bell (2, 320, -1.5f, 0.8f)
            .noOldComps().comp (7, { { "Thresh", -18 }, { "Attack", 2 }, { "Release", 5 }, { "Bias", 1 }, { "Rect", 1 }, { "Makeup", 2 } })
            .eq (7, { { "AirFreq", 2 }, { "AirGain", 3 } }).chain ({ DYN, EQ, DS, TONE, TAPE }).deEss (7000, -36, 8, 1)
            .xlLim (-5, 0).output (0));

        //--- Mastering (a finished song, -13.4 LUFS / 0 dBTP; every one holds -1 dBTP)
        // the Neve chain: console EQ, then the 33609, then a touch of tape, then Goliath XL
        nu ("Master Neve Chain", "Mastering", B().noReso().lowCut (25, 2)
            .charEq (2, { { "HpfFreq", 20 }, { "MudGain", -0.5f }, { "AirGain", 1.5f } })
            .noOldComps().comp (8, { { "Thresh", -14 }, { "Ratio", 0 }, { "Release", 4 }, { "Makeup", 1 } })
            .tape (0, 3, 0.3f, 0.6f).scHpf (2).xlLim (-5, 3).output (0));
        nu ("Master API Chain", "Mastering", B().noReso().lowCut (25, 2)
            .eq (4, { { "LowFreq", 2 }, { "LowGain", 1 }, { "HmFreq", 3 }, { "HmGain", 0.5f }, { "HighFreq", 4 }, { "HighGain", 1.5f } })
            .noOldComps().comp (4, { { "Thresh", -14 }, { "Ratio", 2 }, { "Attack", 4 }, { "Release", 300 }, { "Makeup", 1 }, { "Knee", 2 }, { "Thrust", 2 } })
            .xlLim (-6, 1, 5, 0.15f).output (0));
        nu ("Master Weiss Streaming", "Mastering", B().noReso().lowCut (25, 2)
            .eq (7, { { "AirFreq", 2 }, { "AirGain", 2 } })
            .noOldComps().comp (6, { { "Thresh", -16 }, { "Ratio", 1.5f }, { "Attack", 10 }, { "Release", 300 }, { "Makeup", 1 }, { "Knee", 1 }, { "Detect", 1 }, { "RelMode", 1 } })
            .xlLim (-1, 8).output (0));
        // compress first, then EQ: the tone you set isn't changed by the compressor
        nu ("Master EQ After Glue", "Mastering", B().noReso().lowCut (25, 2)
            .noOldComps().comp (5, { { "Thresh", -14 }, { "Ratio", 0 }, { "Attack", 5 }, { "Release", 4 }, { "Makeup", 1 } }).scHpf (2)
            .chain ({ DYN, EQ, DS, TONE, TAPE }).eq (7, { { "SubGain", 0.5f }, { "G2k5", 0.5f }, { "AirFreq", 2 }, { "AirGain", 2 } })
            .xlLim (-5, 3).output (0));
        // two compressors in series: the 336 smooths, the 2500 adds punch on top
        nu ("Master Two-Stage Glue", "Mastering", B().noReso().lowCut (25, 2)
            .noOldComps().comp (8, { { "Thresh", -12 }, { "Ratio", 0 }, { "Release", 4 }, { "Makeup", 0.5f } })
            .comp (4, { { "Thresh", -12 }, { "Ratio", 1.5f }, { "Attack", 4 }, { "Release", 300 }, { "Makeup", 0.5f }, { "Knee", 2 }, { "Thrust", 2 } })
            .set (id::compOrder9, (float) indexFromPermutationFast<9> (std::array<int, 9> { 0, 1, 2, 3, 8, 4, 5, 6, 7 }))
            .scHpf (2).xlLim (-5, 3).output (0));
        // a song built around the voice: a little presence on the mid only, the sides left alone
        nu ("Master Vocal Forward", "Mastering", B().noReso().lowCut (25, 2)
            .eq (5, { { "HmfFreq", 2800 }, { "HmfGain", 1.2f }, { "HmfQ", 0.7f }, { "LmfFreq", 300 }, { "LmfGain", -0.8f }, { "LmfQ", 0.8f } }).eqPlace (5, 1)
            .noOldComps().comp (7, { { "Thresh", -14 }, { "Attack", 3 }, { "Release", 5 }, { "Bias", 0 }, { "Rect", 1 }, { "Airbass", 1 }, { "Makeup", 1 } })
            .xlLim (-5, 3).output (0));

        //--- FX
        nu ("Tape Pump Wash", "FX", B().noReso().lowCut (60, 2).highCut (12000)
            .tape (2, 3, 0.85f).chain ({ TAPE, DYN, EQ, DS, TONE })
            .noOldComps().comp (5, { { "Thresh", -30 }, { "Ratio", 2 }, { "Attack", 2 }, { "Release", 0 }, { "Makeup", 8 } })
            .xlLim (-13.5, 2).output (0));
        nu ("Crushed Parallel Vocal", "FX", B().noReso().lowCut (120, 2)
            .eq (6, { { "MidFreq", 3 }, { "MidGain", 5 }, { "TrebleGain", 4 } })
            .comps ({ 0 }, { { 0, "Thresh", -34 }, { 0, "Ratio", 20 }, { 0, "Attack", 0.1f }, { 0, "Release", 50 }, { 0, "Makeup", 14 }, { 0, "Mix", 0.35f } }).fetRev (1)
            .deEss (6000, -32).xlLim (-2, 5).output (0));
        nu ("Distorted Edge Vocal", "FX", B().noReso().lowCut (150, 2)
            .esh (4, 0.85f, 2, 6).chain ({ TAPE, EQ, DYN, DS, TONE })
            .eq (5, { { "HpfFreq", 200 }, { "LpfFreq", 9000 }, { "HmfFreq", 2000 }, { "HmfGain", 4 }, { "Mode", 1 } })
            .comps ({ 0 }, { { 0, "Thresh", -22 }, { 0, "Ratio", 8 }, { 0, "Attack", 1 }, { 0, "Release", 80 }, { 0, "Makeup", 3 } })
            .xlLim (-3.5, 5).output (0));
        nu ("Small Speaker Mono", "FX", B().noReso().lowCut (180, 3).highCut (6500, 2).bell (3, 1500, 3, 1)
            .width (0.0f).noOldComps().comp (5, { { "Thresh", -24 }, { "Ratio", 1 }, { "Attack", 1 }, { "Release", 1 }, { "Makeup", 4 } })
            .xlLim (-5, 5).output (0));
        nu ("Wide Dream Vocal", "FX", B().noReso().lowCut (120, 2).bell (2, 350, -3, 0.8f)
            .eq (7, { { "AirFreq", 2 }, { "AirGain", 8 } }).eqPlace (7, 2)
            .comps ({ 3 }, {}).sta (5, 2, 3, 2).tape (1, 3, 0.5f).msDrive (0.5f).width (1.6f)
            .deEss (7000, -36, 9, 1).xlLim (-4.5, 0).output (0));
        nu ("Breakbeat Squash", "FX", B().noReso().lowCut (40, 2)
            .noOldComps().comp (4, { { "Thresh", -28 }, { "Ratio", 10 }, { "Attack", 1 }, { "Release", 80 }, { "Makeup", 10 }, { "Type", 1 } })
            .eq (6, { { "BassFreq", 2 }, { "BassGain", 4 }, { "TrebleGain", 5 } }).chain ({ DYN, EQ, TAPE, DS, TONE }).tape (2, 2, 0.6f)
            .xlLim (-3, 5).output (0));
        nu ("Air Blast Whisper", "FX", B().noReso().lowCut (200, 2)
            .eq (7, { { "G650", -3 }, { "G2k5", 2 }, { "AirFreq", 1 }, { "AirGain", 10 } })
            .noOldComps().comp (6, { { "Thresh", -34 }, { "Ratio", 8 }, { "Attack", 1 }, { "Release", 120 }, { "Makeup", 12 } }).chain ({ EQ, DYN, DS, TONE, TAPE })
            .deEss (8000, -30, 10).xlLim (-4.5, 0).output (0));

        // Level-matched inside each folder (measured on the folder's test recording: a dry male lead, a vocal stack,
        // an acoustic guitar), so switching presets compares the sound, not the volume. True Peak is on wherever
        // the limiter is: four keys presets and two FX went over 0 dBTP without it.
        static const std::map<juce::String, float> levelMatch {
            { "Lead Vocal Modern", -0.5f },
            { "Lead Vocal Warm R&B", 0.5f },
            { "Pop Lead Airy", 2.5f },
            { "R&B Lead Silk", 1.5f },
            { "Soul Lead Warm", 2.0f },
            { "Rock Lead Grit", -1.0f },
            { "Country Lead Clear", 1.0f },
            { "Indie Lead Intimate", 2.0f },
            { "Ballad Lead Smooth", 2.0f },
            { "Perach Pop Lead", 3.0f },
            { "Black Smooth Ballad", 2.0f },
            { "LE-RV One-Knob Vocal", -1.0f },
            { "R&B Warm Esh", 2.5f },
            { "Afro Glow", -0.5f },
            { "Lead Vocal Air After Comp", -0.5f },
            { "Lead Vocal SSL Console", -0.5f },
            { "Lead Vocal Finish", -0.5f },
            { "Vocal Stack Glue", -1.5f },
            { "BV Stacks Tucked", 1.0f },
            { "BV Airy Doubles", 4.0f },
            { "BV Ad-libs Thin", 1.5f },
            { "BV Gang Shouts", 3.5f },
            { "Vocal Stack M/S Wide", -1.0f },
            { "BV Stack Side Air", -0.5f },
            { "Rap Lead Punch", 2.0f },
            { "Trap Lead Air", -1.0f },
            { "Drill Lead Hard", 1.0f },
            { "Rap Doubles Tucked", 4.0f },
            { "Perach Dark Rap", 1.5f },
            { "Rap Lead Two-Stage", -1.5f },
            { "Gospel Lead Power", -1.0f },
            { "Gospel Choir Glue", 2.0f },
            { "Worship Lead Open", 1.0f },
            { "Praise Break Shout", -1.5f },
            { "Gospel Perach Lift", 1.0f },
            { "Worship Pad Vocal", 2.0f },
            { "Choir Wash STA", 0.5f },
            { "Gospel Lead Neve Lift", -1.0f },
            { "Worship Lead Tape Air", -1.0f },
            { "Voiceover Broadcast", -1.0f },
            { "LE-RV Radio Ready", -1.5f },
            { "Electric Guitar Arp", -2.0f },
            { "Clean Funk Snap", -1.5f },
            { "Guitar Bus Esh", -1.5f },
            { "Piano Clear Modern", -2.5f },
            { "Piano Warm Tube", 0.5f },
            { "Synth Pad Wide", -1.5f },
            { "Keys Glow Perach", -2.0f },
            { "Telephone Voice", -2.5f },
            { "AM Radio", -5.5f },
            { "Lo-Fi Cassette", -5.0f },
            { "Megaphone", -5.0f },
            { "Dark Underwater", 2.0f },
            { "Parallel Saturation Sauce", -4.5f },
            { "Ember Rock Scream", 2.0f },
            { "Distorted Edge Vocal", 0.5f },
            { "Small Speaker Mono", 1.0f },
            { "Wide Dream Vocal", 1.0f }
        };
        for (auto& f : lib)
        {
            bool limiterOn = false;
            float out = 0.0f;
            for (auto& [pid, x] : f.values)
            {
                if (pid == id::limOn) limiterOn = x > 0.5f;
                if (pid == id::outputGain) out = x;
            }
            if (limiterOn) f.values.push_back ({ id::limTp, 1 });
            if (auto it = levelMatch.find (f.name); it != levelMatch.end()) f.values.push_back ({ id::outputGain, out + it->second });
        }

        // folders in the order above, presets in the order they were listed within each
        std::stable_sort (lib.begin(), lib.end(), [] (const F& a, const F& b)
        {
            auto rank = [] (const juce::String& c) { int i = 0; for (auto* f : folders) { if (c == f) return i; ++i; } return c.isEmpty() ? -1 : 99; };
            return rank (a.category) < rank (b.category);
        });
        list = std::move (lib);
    }
   #endif
    return list;
}

} // namespace vox
