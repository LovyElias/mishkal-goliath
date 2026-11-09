#include "WallProcessor.h"
#include "WallEditor.h"

using namespace wall;

// ============================================================================================================
// the chain's order as one number (a permutation of the six movable stages)
std::array<int, 6> wall::decodeOrder (int code)
{
    code = juce::jlimit (0, 719, code);
    std::vector<int> pool { 0, 1, 2, 3, 4, 5 };
    std::array<int, 6> o {};
    int f = 120;   // 5!
    for (int i = 0; i < 6; ++i)
    {
        const int k = code / f; code %= f;
        o[(size_t) i] = pool[(size_t) k]; pool.erase (pool.begin() + k);
        if (i < 5) f /= (5 - i);
    }
    return o;
}
int wall::encodeOrder (const std::array<int, 6>& o)
{
    std::vector<int> pool { 0, 1, 2, 3, 4, 5 };
    int code = 0, f = 120;
    for (int i = 0; i < 6; ++i)
    {
        const auto it = std::find (pool.begin(), pool.end(), o[(size_t) i]);
        const int k = it == pool.end() ? 0 : (int) (it - pool.begin());
        code += k * f;
        if (it != pool.end()) pool.erase (it);
        if (i < 5) f /= (5 - i);
    }
    return code;
}

// ============================================================================================================
// presets - our own names
const std::vector<Preset>& wall::factoryPresets()
{
    static const std::vector<Preset> p {
        { "Init", "Default", {} },
        { "Glass Master", "Streaming", { { "limStyle", 0 }, { "limAtk", 3 }, { "limRel", 120 }, { "limStages", 2 }, { "input", 4 }, { "ceiling", -1 }, { "clipOn", 0 }, { "goldOn", 0 }, { "chAmt", 2 } } },
        { "Hammer", "Drums & Beats", { { "limStyle", 1 }, { "limAtk", 0.6f }, { "limRel", 40 }, { "limStages", 2 }, { "input", 6 }, { "pTrans", 50 }, { "cAmt", 3 }, { "cCurve", 0 } } },
        { "Ruach", "Acoustic & Vocal", { { "limStyle", 2 }, { "limAtk", 2 }, { "limRel", 250 }, { "limStages", 2 }, { "input", 3 }, { "pTrans", 10 }, { "clipOn", 0 }, { "chFlav", 1 }, { "chAmt", 3 } } },
        { "Stone", "Rock", { { "limStyle", 3 }, { "limAtk", 1.5f }, { "limRel", 80 }, { "limStages", 2 }, { "input", 6 }, { "chFlav", 3 }, { "chAmt", 5 }, { "cAmt", 2.5f } } },
        { "Jericho", "Club Loud", { { "limStyle", 4 }, { "limAtk", 0.5f }, { "limRel", 25 }, { "limStages", 2 }, { "input", 9 }, { "ceiling", -0.1f }, { "goldOn", 1 }, { "goldDrive", 3 }, { "cAmt", 3.5f }, { "target", -8 } } },
        { "Tower", "Pop", { { "limStyle", 5 }, { "limAtk", 1.2f }, { "limRel", 60 }, { "limStages", 3 }, { "input", 6 }, { "target", -9 } } },
        { "Mortar", "Bus Glue", { { "limStyle", 6 }, { "limAtk", 4 }, { "limRel", 200 }, { "limStages", 2 }, { "input", 3 }, { "levOn", 1 }, { "clipOn", 0 } } },
        { "Refuge", "Classical & Film", { { "limStyle", 7 }, { "limAtk", 5 }, { "limRel", 400 }, { "limStages", 2 }, { "input", 2 }, { "punchOn", 0 }, { "chOn", 0 }, { "clipOn", 0 }, { "target", -16 } } },
        { "Goliath", "Hip-Hop", { { "limStyle", 8 }, { "limAtk", 1 }, { "limRel", 50 }, { "limStages", 3 }, { "input", 8 }, { "bandsOn", 1 }, { "b0Up", 2 }, { "b0Ratio", -0.3f }, { "mono", 1 }, { "goldOn", 1 }, { "goldDrive", 2 } } },
        { "Gold Rush", "Warm & Round", { { "limStyle", 5 }, { "goldOn", 1 }, { "goldDrive", 4 }, { "chFlav", 1 }, { "chAmt", 6 }, { "chEO", -60 }, { "input", 5 } } },
        { "Open Hands", "Gentle", { { "limStyle", 0 }, { "limAtk", 4 }, { "limRel", 300 }, { "input", 2 }, { "pTrans", 0 }, { "pUp", 1 }, { "clipOn", 0 } } },
    };
    return p;
}

// ============================================================================================================
juce::AudioProcessorValueTreeState::ParameterLayout WallProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, float step = 0.01f, float skewMid = -1.0f, const char* unit = "")
    {
        NormalisableRange<float> r (lo, hi, step);
        if (skewMid > lo && skewMid < hi) r.setSkewForCentre (skewMid);
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto bo = [&] (const char* id, const char* name, bool def) { v.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto ch = [&] (const char* id, const char* name, StringArray items, int def) { v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };
    auto in = [&] (const char* id, const char* name, int lo, int hi, int def) { v.push_back (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def)); };

    // global
    bo ("power", "Wall On", true);
    fl ("trim", "In Trim", -24.0f, 24.0f, 0.0f, 0.1f, -1, "dB");
    fl ("input", "Input", 0.0f, 24.0f, 0.0f, 0.1f, 6.0f, "dB");
    fl ("target", "Target", -24.0f, -5.0f, -9.0f, 0.5f, -1, "LUFS");
    fl ("ceiling", "Ceiling", -6.0f, 0.0f, -1.0f, 0.1f, -1, "dB");
    bo ("tp", "True Peak", true);
    fl ("out", "Out", -24.0f, 0.0f, 0.0f, 0.1f, -1, "dB");
    bo ("unity", "Unity Gain", false);
    bo ("delta", "Delta", false);
    fl ("mix", "Mix", 0.0f, 100.0f, 100.0f, 0.1f, -1, "%");
    ch ("preview", "Preview", { "Off", "Spotify", "Apple Music", "YouTube", "Tidal", "Amazon" }, 0);
    ch ("previewWhere", "Preview Platform", { "Spotify", "Apple Music", "YouTube", "Tidal", "Amazon" }, 0);
    ch ("os", "Oversampling", { "Off", "2x", "4x", "8x" }, 2);
    ch ("dither", "Dither", { "Off", "24", "16" }, 1);
    bo ("dc", "DC Filter", true);
    bo ("mono", "Mono Bass", true);
    fl ("monoHz", "Mono Below", 20.0f, 300.0f, 120.0f, 1.0f, 100.0f, "Hz");
    in ("order", "Chain Order", 0, 719, 0);

    bo ("punchOn", "Punch On", true);
    bo ("chOn", "Character On", true);
    bo ("bandsOn", "Bands On", false);
    bo ("levOn", "Leveler On", false);
    bo ("goldOn", "Gold On", true);
    bo ("clipOn", "Clipper On", true);
    bo ("limOn", "Limiter On", true);

    // punch
    fl ("pTrans", "Transients", -100.0f, 100.0f, 40.0f, 1.0f, -1, "%");
    fl ("pUp", "Upward", 0.0f, 12.0f, 2.0f, 0.1f, -1, "dB");
    fl ("pThr", "Upward Threshold", -60.0f, 0.0f, -30.0f, 0.5f, -1, "dB");
    ch ("pSpeed", "Punch Speed", { "Slow", "Medium", "Fast" }, 1);
    ch ("pMode", "Punch Stereo", { "Stereo", "Mid", "Side", "M/S Split" }, 0);
    bo ("pUpOn", "Upward On", true);
    // character
    fl ("chAmt", "Character", 0.0f, 10.0f, 4.2f, 0.1f);
    fl ("chDens", "Density", 0.0f, 100.0f, 35.0f, 1.0f, -1, "%");
    fl ("chEO", "Even / Odd", -100.0f, 100.0f, -30.0f, 1.0f);
    fl ("chEnh", "Enhance", 0.0f, 5.0f, 1.5f, 0.1f);
    ch ("chFlav", "Flavour", { "Clean", "Tape", "Tube", "Console" }, 2);
    // bands
    in ("bCount", "Bands", 2, 4, 3);
    fl ("x1", "Crossover 1", 40.0f, 1000.0f, 120.0f, 1.0f, 200.0f, "Hz");
    fl ("x2", "Crossover 2", 400.0f, 8000.0f, 3000.0f, 1.0f, 2000.0f, "Hz");
    fl ("x3", "Crossover 3", 2000.0f, 16000.0f, 9000.0f, 1.0f, 6000.0f, "Hz");
    for (int b = 0; b < 4; ++b)
    {
        const String n = b == 0 ? "Low" : b == 1 ? "Mid" : b == 2 ? "High" : "Air";
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "Ratio"), 1 }, n + " Ratio", NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "Up"), 1 }, n + " Upward", NormalisableRange<float> (0.0f, 12.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "Lim"), 1 }, n + " Limit", NormalisableRange<float> (0.0f, 12.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "Gain"), 1 }, n + " Gain", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
        v.push_back (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, "Solo"), 1 }, n + " Solo", false));
        v.push_back (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, "Byp"), 1 }, n + " Bypass", false));
    }
    fl ("bThr", "Bands Threshold", -48.0f, 0.0f, -18.0f, 0.5f, -1, "dB");
    ch ("bLink", "Bands Link", { "Off", "Low > Mid", "Low > Top", "Low > All (half)", "Low > All" }, 1);
    bo ("bSc", "S/C Boost", false);
    // leveler
    fl ("lTarget", "Leveler Target", -30.0f, -4.0f, -12.0f, 0.5f, -1, "dB");
    fl ("lRange", "Leveler Range", 0.0f, 12.0f, 6.0f, 0.5f, -1, "dB");
    fl ("lFreeze", "Leveler Freeze", -70.0f, -20.0f, -40.0f, 1.0f, -1, "dB");
    ch ("lSpeed", "Leveler Speed", { "Slow", "Medium", "Fast" }, 0);
    // gold
    ch ("goldDrive", "Gold Drive", { "Off", "2 dB", "3 dB", "4 dB", "6 dB" }, 0);
    // clipper
    fl ("cAmt", "Clip Amount", 0.0f, 6.0f, 2.0f, 0.1f, -1, "dB");
    fl ("cKnee", "Clip Knee", 0.0f, 100.0f, 40.0f, 1.0f, -1, "%");
    fl ("cCeil", "Clip Ceiling", -6.0f, 0.0f, -0.3f, 0.1f, -1, "dB");
    ch ("cCurve", "Clip Curve", { "Hard", "Soft", "Analog", "Tube" }, 2);
    // limiter
    StringArray sn; for (auto& s : styles()) sn.add (s.name);
    ch ("limStyle", "Style", sn, 5);
    fl ("limAtk", "Attack", 0.1f, 10.0f, 1.2f, 0.01f, 2.0f, "ms");
    fl ("limRel", "Release", 5.0f, 1000.0f, 60.0f, 0.1f, 100.0f, "ms");
    in ("limStages", "Stages", 1, 3, 3);
    bo ("limArc", "ARC", true);
    fl ("limLook", "Lookahead", 0.5f, 5.0f, 2.0f, 0.1f, -1, "ms");
    fl ("limLinkT", "Link Transients", 0.0f, 100.0f, 100.0f, 1.0f, -1, "%");
    fl ("limLinkR", "Link Release", 0.0f, 100.0f, 60.0f, 1.0f, -1, "%");
    return { v.begin(), v.end() };
}

// ============================================================================================================
WallProcessor::WallProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WALL", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->getParameterID(), &dirtyListener);
    for (int i = 0; i < 3; ++i)
        os[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>> (2, i + 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    lastSeen = snapshot();
    history.push_back (lastSeen); historyLabel.add ("Open"); historyPos = 0;
    startTimerHz (30);
}

WallProcessor::~WallProcessor()
{
    stopTimer();
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (rp->getParameterID(), &dirtyListener);
}

bool WallProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return l.getMainInputChannelSet() == out;
}

const juce::String WallProcessor::getProgramName (int index)
{
    const auto& p = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) p.size())) return {};
    return juce::String (p[(size_t) index].name) + juce::String::fromUTF8 (" \xc2\xb7 ") + p[(size_t) index].tag;
}

void WallProcessor::prepareToPlay (double sr, int block)
{
    preparedRate = sr; maxBlock = std::max (1, block);
    for (auto& o : os) { o->initProcessing ((size_t) maxBlock); o->reset(); }
    work.setSize (2, maxBlock * 8 + 8, false, true, true);
    grTrace.assign ((size_t) (maxBlock * 8 + 8), 1.0f);
    for (int c = 0; c < 2; ++c) { mixDry[c].assign ((size_t) maxBlock, 0.0f); dBypass[c].assign ((size_t) maxBlock, 0.0f); dDelta[c].assign ((size_t) maxBlock, 0.0f); }
    tmpR.assign ((size_t) maxBlock, 0.0f);
    const int maxL = (int) std::ceil (5.0 * 0.001 * sr) * 8 + 8;
    limiter.prepare (sr * 4.0, maxL);
    ceiling.prepare (sr);
    dc.prepare (sr); mono.prepare (sr);
    // the dry path waits as long as the whole thing (8x oversampling's latency + the longest lookahead + the ceiling)
    int maxOs = 0; for (auto& o : os) maxOs = std::max (maxOs, (int) std::ceil (o->getLatencyInSamples()) + 2);
    const int maxLat = maxOs + (int) std::ceil (5.0 * 0.001 * sr) + 2 + ceiling.latency() + 4;
    for (auto& d : dryMix) d.prepare (maxLat);
    for (auto& d : dryBypass) d.prepare (maxLat);
    for (auto& d : dryDelta) d.prepare (maxLat);
    for (auto& k : kwIn) k.prepare (sr);
    for (auto& k : kwOut) k.prepare (sr);
    tpOut.reset();
    blockLen = std::max (1, (int) std::round (sr * 0.1)); blockE = 0; blockN = 0;
    hop = std::max (1, (int) std::round (sr * 0.01)); accN = 0; acc = {};
    osWas = -1;
    trimS = juce::Decibels::decibelsToGain (param ("trim")); inS = dbToGain (param ("input"));
    mixS = param ("mix") * 0.01f; bypS = param ("power") > 0.5f ? 0.0f : 1.0f;
    updateLatency();
}

int WallProcessor::latencyFor (int osIndex, float lookMs) const
{
    const int osLat = osIndex <= 0 ? 0 : (int) std::round (os[(size_t) (osIndex - 1)]->getLatencyInSamples());
    const int Lb = std::max (1, (int) std::round (lookMs * 0.001 * preparedRate));
    return osLat + Lb + ceiling.latency();
}

void WallProcessor::updateLatency()
{
    const int lat = latencyFor ((int) param ("os"), param ("limLook"));
    if (lat != reportedLatency) { reportedLatency = lat; setLatencySamples (lat); }
}

void WallProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nch = buffer.getNumChannels();
    if (n <= 0 || nch <= 0) return;
    if (n > maxBlock)   // (a host that sends more than it said: do it in pieces)
    {
        juce::MidiBuffer m;
        for (int s = 0; s < n; s += maxBlock)
        {
            const int k = std::min (maxBlock, n - s);
            juce::AudioBuffer<float> part (buffer.getArrayOfWritePointers(), nch, s, k);
            processBlock (part, m);
        }
        return;
    }
    float* L = buffer.getWritePointer (0);
    float* R = nch > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    const bool monoIO = nch < 2;
    const double fs = preparedRate;

    // ---- settings for this block
    const int osIdx = (int) param ("os"), factor = 1 << osIdx;
    const double fsOs = fs * factor;
    if (osIdx != osWas)
    {
        punch.prepare (fsOs); character.prepare (fsOs); bands.prepare (fsOs); leveler.prepare (fsOs);
        limiter.setRate (fsOs);
        if (osIdx > 0) os[(size_t) (osIdx - 1)]->reset();
        osWas = osIdx;
    }
    const float lookMs = param ("limLook");
    const int Lb = std::max (1, (int) std::round (lookMs * 0.001 * fs));
    const int lat = latencyFor (osIdx, lookMs);
    liveLatency = lat;
    const int dryLat = lat - ceiling.latency();
    for (auto& d : dryMix) d.setDelay (dryLat);
    for (auto& d : dryBypass) d.setDelay (lat);
    for (auto& d : dryDelta) d.setDelay (lat);

    const float ceilDb = param ("ceiling");
    const bool tp = param ("tp") > 0.5f;
    const float trimT = dbToGain (param ("trim")), inT = dbToGain (param ("input"));
    const float mixT = param ("mix") * 0.01f, bypT = param ("power") > 0.5f ? 0.0f : 1.0f;
    const bool dcOn = param ("dc") > 0.5f, monoOn = param ("mono") > 0.5f;
    const float monoHz = param ("monoHz");
    const bool bandsOn = param ("bandsOn") > 0.5f;

    PunchSettings ps; ps.transients = param ("pTrans") * 0.01f; ps.upwardDb = param ("pUp"); ps.thresholdDb = param ("pThr");
    ps.speed = (int) param ("pSpeed"); ps.mode = (int) param ("pMode"); ps.upwardOn = param ("pUpOn") > 0.5f && ! bandsOn;   // with the bands on, each band has its own upward
    CharacterSettings cs; cs.amount = param ("chAmt"); cs.density = param ("chDens") * 0.01f; cs.evenOdd = param ("chEO") * 0.01f; cs.enhance = param ("chEnh"); cs.flavour = (int) param ("chFlav");
    BandsSettings bs; bs.count = (int) param ("bCount"); bs.xover = { param ("x1"), param ("x2"), param ("x3") };
    for (int k = 1; k < 3; ++k) bs.xover[(size_t) k] = std::max (bs.xover[(size_t) k], bs.xover[(size_t) (k - 1)] * 1.5f);
    for (int b = 0; b < 4; ++b)
    {
        auto& d = bs.band[(size_t) b];
        d.ratio = param (bandId (b, "Ratio")); d.upwardDb = param (bandId (b, "Up")); d.limitDb = param (bandId (b, "Lim")); d.gainDb = param (bandId (b, "Gain"));
        d.solo = param (bandId (b, "Solo")) > 0.5f; d.bypass = param (bandId (b, "Byp")) > 0.5f;
    }
    bs.masterThrDb = param ("bThr"); bs.linkMode = (int) param ("bLink"); bs.scBoost = param ("bSc") > 0.5f;
    LevelerSettings ls; ls.targetDb = param ("lTarget"); ls.rangeDb = param ("lRange"); ls.freezeDb = param ("lFreeze"); ls.speed = (int) param ("lSpeed");
    static constexpr float goldSteps[] { 0.0f, 2.0f, 3.0f, 4.0f, 6.0f };
    const float goldDb = goldSteps[juce::jlimit (0, 4, (int) param ("goldDrive"))];
    ClipperSettings cl; cl.amountDb = param ("cAmt"); cl.knee = param ("cKnee") * 0.01f; cl.ceilingDb = param ("cCeil"); cl.curve = (int) param ("cCurve");
    LimiterSettings lm; lm.style = (int) param ("limStyle"); lm.attackMs = param ("limAtk"); lm.releaseMs = param ("limRel");
    lm.lookaheadMs = lookMs; lm.linkT = param ("limLinkT") * 0.01f; lm.linkR = param ("limLinkR") * 0.01f;
    lm.stages = (int) param ("limStages"); lm.arc = param ("limArc") > 0.5f;
    const bool limOn = param ("limOn") > 0.5f;
    lm.ceilingDb = limOn ? ceilDb : 60.0f;   // (off: it only waits, so the latency stays the same)
    // the release link: how much the two sides let go together
    const int Ls = Lb * factor;
    const int A = juce::jlimit (1, Ls + 1, (int) std::round (lm.attackMs * 0.001 * fsOs));

    // ---- input: trim, DC, mono bass, then the drive
    const float kT = 1.0f - coefMs (20.0f, fs);
    float pkIn = 0.0f, sqIn = 0.0f, pkInC[2] {};
    for (int i = 0; i < n; ++i)
    {
        trimS += kT * (trimT - trimS); inS += kT * (inT - inS);
        float a = L[i] * trimS, b = (monoIO ? L[i] : R[i]) * trimS;
        if (dcOn) { a = dc.process (0, a); b = dc.process (1, b); }
        if (monoOn) mono.process (a, b, monoHz);
        pkInC[0] = std::max (pkInC[0], std::abs (a * inS)); pkInC[1] = std::max (pkInC[1], std::abs (b * inS));
        pkIn = std::max (pkIn, std::max (std::abs (a), std::abs (b))); sqIn += 0.5f * (a * a + b * b);
        { const float ka = kwIn[0].process (a), kb = kwIn[1].process (b); eIn += (1.0f - coefMs (3000.0f, fs) ) * (ka * ka + kb * kb - eIn); }
        dryBypassIn (i, a, b);
        a *= inS; b *= inS;
        dryDeltaIn (i, a, b);
        L[i] = a; if (! monoIO) R[i] = b; else tmpR[(size_t) i] = b;
    }
    float* Rw = monoIO ? tmpR.data() : R;
    inPk = pkIn; inCh[0] = pkInC[0]; inCh[1] = pkInC[1]; inRms = std::sqrt (sqIn / (float) n);

    // dry copy for MIX (with the drive), aligned to the wet path before the ceiling
    for (int i = 0; i < n; ++i) { mixDry[0][(size_t) i] = dryMix[0].process (L[i]); mixDry[1][(size_t) i] = dryMix[1].process (Rw[i]); }

    // ---- the chain, oversampled
    float* chans[2] { L, Rw };
    juce::dsp::AudioBlock<float> base (chans, 2, (size_t) n);
    juce::dsp::AudioBlock<float> up = base;
    if (osIdx > 0) up = os[(size_t) (osIdx - 1)]->processSamplesUp (base);
    const int m = (int) up.getNumSamples();
    float* ul = up.getChannelPointer (0); float* ur = up.getChannelPointer (1);
    const auto ord = order();
    for (int k = 0; k < 6; ++k)
    {
        const int st = ord[(size_t) k];
        if (param (stageOnId (st)) < 0.5f) continue;
        switch (st)
        {
            case stPunch:     punch.process (ul, ur, m, ps); break;
            case stCharacter: character.process (ul, ur, m, cs); break;
            case stBands:     bands.process (ul, ur, m, bs); break;
            case stLeveler:   leveler.process (ul, ur, m, ls); break;
            case stGold:      gold.process (ul, ur, m, goldDb); break;
            case stClipper:   clipper.process (ul, ur, m, cl, std::max (1, (int) (fsOs / 6000.0))); break;
            default: break;
        }
    }
    if (param ("levOn") < 0.5f) leveler.gainNow = 0.0f;
    if (param ("goldOn") < 0.5f) gold.hot = 0.0f;
    if (param ("clipOn") < 0.5f) clipper.shaveDb = 0.0f;
    limiter.process (ul, ur, m, lm, Ls, A, grTrace.data());
    if (osIdx > 0) os[(size_t) (osIdx - 1)]->processSamplesDown (base);

    // ---- mix, the ceiling, out, unity, preview, dither, bypass
    const float kM = 1.0f - coefMs (30.0f, fs);
    for (int i = 0; i < n; ++i)
    {
        mixS += kM * (mixT - mixS);
        L[i] = mixDry[0][(size_t) i] + mixS * (L[i] - mixDry[0][(size_t) i]);
        Rw[i] = mixDry[1][(size_t) i] + mixS * (Rw[i] - mixDry[1][(size_t) i]);
    }
    ceiling.process (L, Rw, n, ceilDb, tp, fs);

    const float outG = dbToGain (param ("out"));
    const bool unity = param ("unity") > 0.5f, delta = param ("delta") > 0.5f;
    // what the streaming preview is doing: play it as quietly as the platform will
    static constexpr float platform[] { -14.0f, -16.0f, -14.0f, -14.0f, -14.0f };
    const int pv = (int) param ("preview");
    float prevT = 0.0f;
    if (pv > 0)
    {
        const float lv = lufsI.load() > -69.0f ? lufsI.load() : lufsS.load();
        if (lv > -69.0f) prevT = std::min (0.0f, platform[pv - 1] - lv);
    }
    const float uT = unity ? juce::jlimit (-24.0f, 0.0f, 10.0f * std::log10 ((eIn + 1.0e-12f) / (eOut + 1.0e-12f))) : 0.0f;
    const float Th = dbToGain (ceilDb);
    const float kU = 1.0f - coefMs (200.0f, fs);
    const int bits = (int) param ("dither") == 1 ? 24 : (int) param ("dither") == 2 ? 16 : 0;
    float pkO = 0.0f, sqO = 0.0f, pkOC[2] {};
    for (int i = 0; i < n; ++i)
    {
        unityDb += kU * (uT - unityDb); prevDb += kU * (prevT - prevDb);
        const float g = outG * dbToGain (unityDb + prevDb);
        float a = L[i] * g, b = Rw[i] * g;
        if (delta)   // what Wall changed: the driven input minus the result
        {
            a = dryDelta[0].process (dDelta[0][(size_t) i]) - a; b = dryDelta[1].process (dDelta[1][(size_t) i]) - b;
        }
        else { dryDelta[0].process (dDelta[0][(size_t) i]); dryDelta[1].process (dDelta[1][(size_t) i]); }
        L[i] = a; Rw[i] = b;
    }
    dither.process (L, Rw, n, bits);
    const float kB = 1.0f - coefMs (15.0f, fs);
    for (int i = 0; i < n; ++i)
    {
        float a = L[i], b = Rw[i];
        if (! delta) { a = juce::jlimit (-Th, Th, a); b = juce::jlimit (-Th, Th, b); }   // nothing past the ceiling, ever
        bypS += kB * (bypT - bypS);
        const float da = dryBypass[0].process (dBypass[0][(size_t) i]), db = dryBypass[1].process (dBypass[1][(size_t) i]);
        if (bypS > 1.0e-4f) { a += bypS * (da - a); b += bypS * (db - b); }
        L[i] = a; Rw[i] = b;
        // meters: output
        pkOC[0] = std::max (pkOC[0], std::abs (a)); pkOC[1] = std::max (pkOC[1], std::abs (b));
        pkO = std::max (pkO, std::max (std::abs (a), std::abs (b))); sqO += 0.5f * (a * a + b * b);
        const float ka = kwOut[0].process (a), kb = kwOut[1].process (b);
        const float ke = ka * ka + kb * kb;
        eOut += (1.0f - coefMs (3000.0f, fs)) * (ke - eOut);
        blockE += ke; if (++blockN >= blockLen) { int s1, z1, s2, z2; lufsFifo.prepareToWrite (1, s1, z1, s2, z2); if (z1 > 0) lufsBlocks[(size_t) s1] = (float) (blockE / blockN); lufsFifo.finishedWrite (z1); blockE = 0; blockN = 0; }
        const float tpv = tp ? tpOut.push (a, b) : std::max (std::abs (a), std::abs (b));
        tpShort = std::max (tpv, tpShort * (1.0f - (float) (1.0 / (3.0 * fs))));
        tpMax = std::max (tpMax, tpv);
        const bool over = tpv > Th * 1.0059f;   // (0.05 dB over)
        if (over && ! overWas) ispCount = ispCount.load() + 1;
        overWas = over;
        const float kc = 1.0f - coefMs (300.0f, fs);
        cLR += kc * (a * b - cLR); cLL += kc * (a * a - cLL); cRR += kc * (b * b - cRR);
        // the wave: one column every 10 ms
        acc.inPk = std::max (acc.inPk, std::max (std::abs (dBypass[0][(size_t) i]), std::abs (dBypass[1][(size_t) i])) * inS);
        acc.outPk = std::max (acc.outPk, std::max (std::abs (a), std::abs (b)));
        float gmin = 1.0f; for (int j = i * factor; j < (i + 1) * factor && j < m; ++j) gmin = std::min (gmin, grTrace[(size_t) j]);
        acc.grDb = std::max (acc.grDb, -gainToDb (gmin));
        if (++accN >= hop)
        {
            acc.levDb = leveler.gainNow.load();
            int s1, z1, s2, z2; frameFifo.prepareToWrite (1, s1, z1, s2, z2); if (z1 > 0) frames[(size_t) s1] = acc; frameFifo.finishedWrite (z1);
            acc = {}; accN = 0;
        }
    }
    outCh[0] = pkOC[0]; outCh[1] = pkOC[1];
    outPk = pkO; outRms = std::sqrt (sqO / (float) n);
    limGrDb = limiter.grDb.load();
    grDb = limOn ? limiter.grDb.load() + ceiling.grDb.load() : ceiling.grDb.load();
    clipDb = clipper.shaveDb.load(); goldHot = gold.hot.load(); levDb = leveler.gainNow.load();
    tpShortDb = gainToDb (tpShort); tpMaxDb = gainToDb (tpMax);
    corr = (cLL > 1.0e-9f && cRR > 1.0e-9f) ? juce::jlimit (-1.0f, 1.0f, cLR / std::sqrt (cLL * cRR)) : 1.0f;
    previewDb = prevDb;
    if (resetMeters.exchange (false)) { tpMax = 0.0f; ispCount = 0; }
}

// the two dry taps: bypass (after trim, before the drive) and delta (after the drive)
void WallProcessor::dryBypassIn (int i, float a, float b) { dBypass[0][(size_t) i] = a; dBypass[1][(size_t) i] = b; }
void WallProcessor::dryDeltaIn (int i, float a, float b) { dDelta[0][(size_t) i] = a; dDelta[1][(size_t) i] = b; }

// ============================================================================================================
// message thread: loudness, learn, undo
void WallProcessor::timerCallback()
{
    updateLatency();
    // loudness from the 100 ms blocks
    int s1, z1, s2, z2;
    const int ready = lufsFifo.getNumReady();
    lufsFifo.prepareToRead (ready, s1, z1, s2, z2);
    auto take = [this] (float e)
    {
        blockHist.push_back (e);
        if (blockHist.size() > 36000) blockHist.erase (blockHist.begin(), blockHist.begin() + 6000);
        auto meanOf = [this] (int k) { const int nb = std::min (k, (int) blockHist.size()); double s = 0; for (int j = 0; j < nb; ++j) s += blockHist[blockHist.size() - 1 - (size_t) j]; return nb > 0 ? s / nb : 0.0; };
        auto toL = [] (double e2) { return e2 > 1.0e-12 ? (float) (-0.691 + 10.0 * std::log10 (e2)) : -70.0f; };
        const float M = toL (meanOf (4)), S = toL (meanOf (30));
        lufsM = std::max (-70.0f, M); lufsS = std::max (-70.0f, S);
        // integrated: 400 ms blocks (75% overlap), gated at -70 then 10 LU under
        if (blockHist.size() >= 4)
        {
            const double e4 = meanOf (4);
            if (toL (e4) > -70.0f) { gated.push_back ((float) e4); }
            double sum = 0; for (auto g : gated) sum += g;
            if (! gated.empty())
            {
                const float rel = toL (sum / (double) gated.size()) - 10.0f;
                double s3 = 0; int c3 = 0; for (auto g : gated) if (toL (g) > rel) { s3 += g; ++c3; }
                lufsI = c3 > 0 ? std::max (-70.0f, toL (s3 / c3)) : -70.0f;
            }
        }
        // range: the short-term values, gated 20 LU under, 10th to 95th percentile
        if (S > -70.0f) shortHist.push_back (S);
        if (shortHist.size() > 36000) shortHist.erase (shortHist.begin(), shortHist.begin() + 6000);
        if (shortHist.size() > 10 && (shortHist.size() % 5) == 0)
        {
            double se = 0; for (auto v : shortHist) se += std::pow (10.0, (v + 0.691) / 10.0);
            const float rel = (float) (-0.691 + 10.0 * std::log10 (se / (double) shortHist.size())) - 20.0f;
            std::vector<float> v; v.reserve (shortHist.size()); for (auto x : shortHist) if (x > rel) v.push_back (x);
            if (v.size() > 4) { std::sort (v.begin(), v.end()); lra = v[(size_t) ((v.size() - 1) * 95 / 100)] - v[(size_t) ((v.size() - 1) * 10 / 100)]; }
        }
        lufsHistory.push_back (S);
        if (lufsHistory.size() > 6000) lufsHistory.erase (lufsHistory.begin(), lufsHistory.begin() + 1000);
    };
    for (int j = 0; j < z1; ++j) take (lufsBlocks[(size_t) (s1 + j)]);
    for (int j = 0; j < z2; ++j) take (lufsBlocks[(size_t) (s2 + j)]);
    lufsFifo.finishedRead (z1 + z2);

    // LEARN: listen, nudge the input towards the target, a few times
    if (learning)
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        if (now - learnStart > 3.0)
        {
            const float S = lufsS.load();
            if (S > -60.0f)
            {
                const float err = param ("target") - S;
                setParam ("input", juce::jlimit (0.0f, 24.0f, param ("input") + err * 0.8f));
                if (std::abs (err) < 0.3f || ++learnSteps >= 5) learning = false;
            }
            learnStart = now;
        }
    }
    if (autoTrim)
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        if (now - autoStart > 3.0)
        {
            const float inLv = -0.691f + 10.0f * std::log10 (eIn + 1.0e-12f);
            if (inLv > -60.0f) setParam ("trim", juce::jlimit (-24.0f, 24.0f, param ("trim") + (-18.0f - inLv)));
            autoTrim = false;
        }
    }
    recordHistory();
}

void WallProcessor::startLearn() { learning = true; learnSteps = 0; learnStart = juce::Time::getMillisecondCounterHiRes() * 0.001; }
void WallProcessor::startAutoTrim() { autoTrim = true; autoStart = juce::Time::getMillisecondCounterHiRes() * 0.001; }
void WallProcessor::resetLoudness()
{
    gated.clear(); shortHist.clear(); blockHist.clear(); lufsHistory.clear();
    lufsI = -70.0f; lra = 0.0f; resetMeters = true;
}

// ============================================================================================================
void WallProcessor::setParam (const juce::String& pid, float value)
{
    if (auto* p = par (pid)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (value)); p->endChangeGesture(); }
}
void WallProcessor::beginGesture (const juce::String& pid) { if (auto* p = par (pid)) p->beginChangeGesture(); }
void WallProcessor::dragParam (const juce::String& pid, float value) { if (auto* p = par (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value)); }
void WallProcessor::endGesture (const juce::String& pid) { if (auto* p = par (pid)) p->endChangeGesture(); }

void WallProcessor::setStyle (int style)
{
    style = juce::jlimit (0, 8, style);
    const auto& s = styles()[(size_t) style];
    setParam ("limStyle", (float) style); setParam ("limAtk", s.attackMs); setParam ("limRel", s.releaseMs); setParam ("limStages", (float) s.stages);
}

void WallProcessor::bandsSwitched (bool on)
{
    if (on)
    {
        bool any = false; for (int b = 0; b < 4; ++b) any |= param (bandId (b, "Up")) > 0.01f;
        if (! any && param ("pUpOn") > 0.5f)
            for (int b = 0; b < 4; ++b) setParam (bandId (b, "Up"), param ("pUp"));
    }
    setParam ("bandsOn", on ? 1.0f : 0.0f);
}

// ---------------- presets
void WallProcessor::loadFactoryPreset (int index)
{
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    loadingPreset = true;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        { rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->getDefaultValue()); rp->endChangeGesture(); }
    for (auto& [id, v] : ps[(size_t) index].values) setParam (id, v);
    currentProgram = index;
    presetName = getProgramName (index);
    loadingPreset = false; presetDirty = false;
}

juce::File WallProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Mishkal Audio/Wall/Presets");
}
juce::Array<juce::File> WallProcessor::userPresets() const
{
    auto a = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.wallpreset");
    a.sort();
    return a;
}
bool WallProcessor::saveUserPreset (const juce::String& name)
{
    auto f = userPresetFolder(); f.createDirectory();
    auto st = apvts.copyState();
    std::unique_ptr<juce::XmlElement> x (st.createXml());
    if (x == nullptr) return false;
    if (! x->writeTo (f.getChildFile (juce::File::createLegalFileName (name) + ".wallpreset"))) return false;
    presetName = name; presetDirty = false;
    return true;
}
bool WallProcessor::loadUserPreset (const juce::File& f)
{
    std::unique_ptr<juce::XmlElement> x (juce::XmlDocument::parse (f));
    if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return false;
    loadingPreset = true;
    apvts.replaceState (juce::ValueTree::fromXml (*x));
    loadingPreset = false;
    presetName = f.getFileNameWithoutExtension(); presetDirty = false;
    return true;
}

// ---------------- A / B
void WallProcessor::switchAB()
{
    slots[abSlot] = apvts.copyState();
    abSlot = 1 - abSlot;
    if (slots[abSlot].isValid()) { loadingPreset = true; apvts.replaceState (slots[abSlot].createCopy()); loadingPreset = false; }
}
void WallProcessor::copyAB()
{
    slots[1 - abSlot] = apvts.copyState();
}

// ---------------- undo / redo (a step is a moment the settings stopped moving)
std::vector<float> WallProcessor::snapshot() const
{
    std::vector<float> s;
    for (auto* p : getParameters()) s.push_back (p->getValue());
    return s;
}
void WallProcessor::applySnapshot (const std::vector<float>& s)
{
    auto ps = getParameters();
    loadingPreset = true;
    for (int i = 0; i < ps.size() && i < (int) s.size(); ++i)
        if (std::abs (ps[i]->getValue() - s[(size_t) i]) > 1.0e-6f) { ps[i]->beginChangeGesture(); ps[i]->setValueNotifyingHost (s[(size_t) i]); ps[i]->endChangeGesture(); }
    loadingPreset = false;
    lastSeen = s;
}
void WallProcessor::recordHistory()
{
    auto now = snapshot();
    if (now == lastSeen) { if (pendingChange && ++stableTicks > 12) { pendingChange = false; history.resize ((size_t) historyPos + 1); historyLabel.removeRange (historyPos + 1, historyLabel.size());
                                                                       history.push_back (now); historyLabel.add (pendingLabel); historyPos = (int) history.size() - 1;
                                                                       if (history.size() > 64) { history.erase (history.begin()); historyLabel.remove (0); --historyPos; } }
                           return; }
    auto ps = getParameters();
    for (int i = 0; i < ps.size() && i < (int) now.size(); ++i)
        if (std::abs (now[(size_t) i] - lastSeen[(size_t) i]) > 1.0e-6f) { pendingLabel = ps[i]->getName (40) + " " + ps[i]->getCurrentValueAsText(); break; }
    lastSeen = now; stableTicks = 0; pendingChange = true;
}
void WallProcessor::undo() { if (historyPos > 0) { --historyPos; applySnapshot (history[(size_t) historyPos]); pendingChange = false; } }
void WallProcessor::redo() { if (historyPos + 1 < (int) history.size()) { ++historyPos; applySnapshot (history[(size_t) historyPos]); pendingChange = false; } }
void WallProcessor::jumpHistory (int pos) { if (juce::isPositiveAndBelow (pos, (int) history.size())) { historyPos = pos; applySnapshot (history[(size_t) pos]); pendingChange = false; } }

// ---------------- state
void WallProcessor::getStateInformation (juce::MemoryBlock& mb)
{
    auto st = apvts.copyState();
    st.setProperty ("presetName", presetName, nullptr);
    st.setProperty ("wallVersion", WALL_VERSION, nullptr);
    st.setProperty ("ab", abSlot, nullptr);
    st.setProperty ("uiSize", uiSize, nullptr);
    st.setProperty ("uiView", uiView, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, mb);
}
void WallProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> x (getXmlFromBinary (data, size));
    if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return;
    auto st = juce::ValueTree::fromXml (*x);
    loadingPreset = true;
    apvts.replaceState (st);
    loadingPreset = false;
    presetName = st.getProperty ("presetName", presetName).toString();
    abSlot = (int) st.getProperty ("ab", 0);
    uiSize = (int) st.getProperty ("uiSize", uiSize);
    uiView = (int) st.getProperty ("uiView", uiView);
    presetDirty = false;
}

juce::AudioProcessorEditor* WallProcessor::createEditor() { return new WallEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WallProcessor(); }
