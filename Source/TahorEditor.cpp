#include "TahorEditor.h"
#include "BinaryData.h"

#if JUCE_MAC
float tahorMacFullScale (void* nsView, float W, float H, float curW, float curH);
void tahorMacPinTop (void* nsView);
#endif

using namespace tahor;
using namespace tahor::ui;

//==============================================================================
juce::Colour ui::col (const char* hex, float alpha)
{
    auto s = juce::String (hex).trimCharactersAtStart ("#");
    return juce::Colour (0xff000000u | (juce::uint32) s.getHexValue32()).withAlpha (alpha);
}
juce::Colour ui::col (juce::uint32 rgb, float alpha) { return juce::Colour (0xff000000u | rgb).withAlpha (alpha); }

static juce::Typeface::Ptr typefaceFor (Face f)
{
    static juce::Typeface::Ptr t[6] {
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansLight_ttf, (size_t) BinaryData::FiraSansLight_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansRegular_ttf, (size_t) BinaryData::FiraSansRegular_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansMedium_ttf, (size_t) BinaryData::FiraSansMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::FiraSansSemiBold_ttf, (size_t) BinaryData::FiraSansSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedMedium_ttf, (size_t) BinaryData::BarlowCondensedMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, (size_t) BinaryData::BarlowCondensedSemiBold_ttfSize) };
    return t[(int) f];
}

juce::Font ui::font (Face f, float px, float letterSpacing)
{
    juce::Font fo (juce::FontOptions (typefaceFor (f)).withPointHeight (px));
    if (letterSpacing != 0.0f) fo.setExtraKerningFactor (letterSpacing / px);
    return fo;
}

float ui::textWidth (const juce::Font& f, const juce::String& s)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, s, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

void ui::text (juce::Graphics& g, const juce::String& s, const juce::Font& f, juce::Colour c, R box, juce::Justification j)
{
    g.setColour (c);
    g.setFont (f);
    g.drawText (s, box, j, false);
}

//==============================================================================
Canvas::Canvas (TahorProcessor& p) : proc (p)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);   // V11: shortcuts
    auto& st = proc.apvts.state;
    tab = (int) st.getProperty ("uiTab", 0);
    pianoOn = (bool) st.getProperty ("uiPiano", false);   // V13
    selBand = (int) st.getProperty ("uiBand", 2);
    selNode = (int) st.getProperty ("uiNode", 1);
    preview = (int) st.getProperty ("uiPreview", 0);
    stripOpen = (bool) st.getProperty ("uiStrip", true);
    advOpen = (bool) st.getProperty ("uiAdv", false);
    fft = std::make_unique<juce::dsp::FFT> (13);   // V5: 8192 points, finer in the lows
    preWin.assign (8192, 0.0f); postWin.assign (8192, 0.0f);
    fftData.assign (16384, 0.0f);
    preDb.assign (4097, -120.0f); postDb.assign (4097, -120.0f);
    hann.resize (8192);
    for (int i = 0; i < 8192; ++i) hann[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 8192.0f);
    resoShown.assign ((size_t) PW + 1, 0.0f);
    if (juce::JUCEApplicationBase::isStandaloneApp()   // V16: (the stand-alone app only - never inside a host)
        && juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("Mishkal Projects/Tahor-bench.on").existsAsFile()) benchStep = 0;
    startTimerHz (60);   // V6: smoother movement; V14: 60 a second (V16: frames are paced if drawing ever runs long)
}

Canvas::~Canvas()
{
    auto& st = proc.apvts.state;
    st.setProperty ("uiTab", tab, nullptr); st.setProperty ("uiBand", selBand, nullptr); st.setProperty ("uiNode", selNode, nullptr);
    st.setProperty ("uiPreview", preview, nullptr); st.setProperty ("uiStrip", stripOpen, nullptr); st.setProperty ("uiAdv", advOpen, nullptr);
}

void Canvas::analyse (std::vector<float>& win, std::vector<float>& smoothDb)
{
    const float norm = 2.0f / (8192.0f * 0.5f);
    for (int i = 0; i < 8192; ++i) fftData[(size_t) i] = win[(size_t) i] * hann[(size_t) i];
    std::fill (fftData.begin() + 8192, fftData.end(), 0.0f);
    fft->performFrequencyOnlyForwardTransform (fftData.data(), true);
    for (int k = 0; k < 4097; ++k)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[(size_t) k] * norm, -140.0f);
        auto& s = smoothDb[(size_t) k];
        s = db > s ? 0.5f * s + 0.5f * db : 0.86f * s + 0.14f * db;
    }
}

void Canvas::timerCallback()
{
    proc.preTap.pull (preWin);
    proc.postTap.pull (postWin);
    if (eqMatch == 1)   // V12: EQ MATCH learning - your sound (pre) and the reference (sidechain)
    {
        if (keyWin.size() != 8192) { keyWin.assign (8192, 0.0f); keyDb.assign (4097, -120.0f); }
        proc.keyTap.pull (keyWin);
        analyse (keyWin, keyDb);
        if (accPre.size() == 4097)
        {
            for (int k = 0; k < 4097; ++k) { accPre[(size_t) k] += preDb[(size_t) k]; accKey[(size_t) k] += keyDb[(size_t) k]; }
            ++eqCount;
        }
    }
    analyse (preWin, preDb);
    analyse (postWin, postDb);

    // what Reso removes, per display column (held a little so it reads)
    auto& r = proc.activeReso();
    const int nb = r.numBins.load();
    if (nb > 2)
    {
        const float binHz = (float) (r.getSampleRate() / (double) ((nb - 1) * 2));
        std::vector<float> col ((size_t) PW + 1, 0.0f);
        for (int k = 1; k < nb; ++k)
        {
            const int x = juce::roundToInt (fx ((float) k * binHz));
            if (x < 0 || x > (int) PW) continue;
            col[(size_t) x] = juce::jmin (col[(size_t) x], r.reductionDb[(size_t) k].load (std::memory_order_relaxed));
        }
        // fill the gaps at the bottom end (few bins per column there)
        float last = 0.0f; int lastX = 0;
        for (int x = 0; x <= (int) PW; ++x)
        {
            if (col[(size_t) x] < 0.0f || x == (int) PW)
            {
                for (int i = lastX + 1; i < x; ++i) col[(size_t) i] = juce::jmin (col[(size_t) i], last + (col[(size_t) x] - last) * (float) (i - lastX) / (float) (x - lastX));
                last = col[(size_t) x]; lastX = x;
            }
        }
        for (int x = 0; x <= (int) PW; ++x)
        {
            auto& s = resoShown[(size_t) x];
            s = col[(size_t) x] < s ? col[(size_t) x] : 0.8f * s + 0.2f * col[(size_t) x];
        }
    }
    for (int c = 0; c < 4; ++c)
    {
        const auto& m = c == 0 ? proc.inMeterL : c == 1 ? proc.inMeterR : c == 2 ? proc.meterL : proc.meterR;
        const float db = juce::Decibels::gainToDecibels (m.load(), -90.0f);
        meterDb[c] = db > meterDb[c] ? db : juce::jmax (db, meterDb[c] - 1.2f);
    }
    // V10: the readouts - peak (held 1.5 s) and average (RMS, ~300 ms) for input and output
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        for (int s2 = 0; s2 < 2; ++s2)
        {
            const float pk = juce::jmax (meterDb[s2 * 2], meterDb[s2 * 2 + 1]);
            if (pk >= pkHold[s2] || now - pkTime[s2] > 1.5) { pkHold[s2] = pk; pkTime[s2] = now; }
            const float rms = juce::Decibels::gainToDecibels ((s2 == 0 ? proc.inRms : proc.outRms).load(), -90.0f);
            avgDb[s2] += (rms - avgDb[s2]) * 0.15f;
        }
        // the analyzer only shows while something plays (it sinks away in silence, like Reso's reduction)
        const float lvl = juce::jmax (meterDb[0], meterDb[1], meterDb[2], meterDb[3]);
        anaAlpha += ((lvl > -70.0f ? 1.0f : 0.0f) - anaAlpha) * 0.08f;
    }
    drift = std::fmod (drift + 1.0f / (30.0f * 60.0f), 1.0f);   // the background's glows: once round a minute
    // V3: only the dots show; touching one (or the place a panel lives) fades the tab's controls in, leaving fades them out
    {
        bool want = (dragging && down.kind != Hit::None) || typeIn != nullptr;
        if (auto* z = zoneAt (mouse))
            if (z->kind == Hit::TahorNode || z->kind == Hit::ResoNode || z->kind == Hit::CharNode || z->kind == Hit::PeakDot || z->kind == Hit::FocusEdge)
                want = true;
        for (auto& r : revealRects) if (r.contains (mouse)) want = true;
        // V4: stays up while the mouse is on the band or its panel; half a second after it leaves both, it fades
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (want) lastWant = now;
        reveal = want ? juce::jmin (1.0f, reveal + 0.34f)
                      : (now - lastWant < 4000.0 ? reveal : juce::jmax (0.0f, reveal - 0.2f));
    }
    pulse = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * (juce::Time::getMillisecondCounterHiRes() * 0.001 / 1.6)));
    if (benchStep >= 0) benchTick();
    // V16: if a frame takes longer than the time between frames, skip a tick so the host always has room to breathe
    if (lastPaintMs > 15.0 && ! dragging && (++skipTick % 2) == 1) return;
    repaint();
}

// V16: the drawing benchmark - each tab at Medium, Large, Extra Large, Extra Large with the tabs off, and 1.6x, four
// seconds each; the times go to Tahor-bench.log next to the switch file, then the switch is renamed so it runs once
static juce::String benchVersion()
{
   #ifdef JucePlugin_VersionString
    return JucePlugin_VersionString;
   #else
    return "dev";
   #endif
}
void Canvas::benchTick()
{
    static const float sc[5] { 1.0f, 1.2f, 1.4f, 1.4f, 1.6f };
    const double now = juce::Time::getMillisecondCounterHiRes();
    const auto dir = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("Mishkal Projects");
    auto start = [&]
    {
        tab = benchStep % 3;
        const float onOff = benchStep / 3 == 3 ? 0.0f : 1.0f;
        proc.setParam ("resoOn", onOff); proc.setParam ("tahorOn", onOff); proc.setParam ("charOn", onOff);
        if (onSize) onSize (sc[benchStep / 3]);
        benchAt = now; benchSum = benchMax = 0.0; benchFrames = benchSlow = 0; benchOn = false;
    };
    if (benchAt == 0.0) { dir.getChildFile ("Tahor-bench.log").appendText ("\n=== Tahor " + benchVersion() + " ===\n"); start(); return; }
    if (now - benchAt > 1000.0) benchOn = true;
    if (now - benchAt > 5000.0)
    {
        const juce::String line = "tab " + juce::String (benchStep % 3) + (benchStep / 3 == 3 ? " (off)" : "") + "  size " + juce::String (sc[benchStep / 3], 1)
            + "  frames/s " + juce::String (benchFrames / 4.0, 1) + "  slow(>40ms) " + juce::String (benchSlow)
            + "  paint avg " + juce::String (benchFrames > 0 ? benchSum / benchFrames : 0.0, 1) + " ms  max " + juce::String (benchMax, 1);
        dir.getChildFile ("Tahor-bench.log").appendText (line + "\n");
        if (++benchStep >= 15) { benchStep = -1; benchOn = false; dir.getChildFile ("Tahor-bench.on").moveFileTo (dir.getChildFile ("Tahor-bench.done")); return; }
        start();
    }
}

const Canvas::Zone* Canvas::zoneAt (P p) const
{
    for (auto it = zones.rbegin(); it != zones.rend(); ++it)
        if (it->r.contains (p)) return &*it;
    return nullptr;
}

//==============================================================================
juce::String Canvas::knobPid (const Zone& z, bool outer) const
{
    static const char* rk[7] { "rsDepth", "rsDetail", "rsAttack", "rsRelease", "rsMaxCut", "rsMix", "rsTrim" };
    static const char* tk[3] { "rsDTilt", "rsATilt", "rsRTilt" };
    switch (z.kind)
    {
        case Hit::ResoKnob:     return rk[juce::jlimit (0, 6, z.a)];
        case Hit::TiltKnob:     return tk[juce::jlimit (0, 2, z.a)];
        case Hit::LinkKnob:     return "rsLink";
        case Hit::ResoNodeKnob: return nid (selNode, z.a == 0 ? "Amt" : "Q");
        case Hit::TahorKnob:
        {
            static const char* t[8] { "Hz", "Gain", "Q", "Thr", "Rng", "Att", "Rel", "Sel" };   // V9: + the dynamic / spectral view
            return bid (selBand, t[juce::jlimit (0, 7, z.a)]);
        }
        case Hit::InBox:        return "inGain";
        case Hit::OutBox:       return "outGain";
        case Hit::CharKnob: return cuid (z.a);
        case Hit::UnitKnob:
        {
            const auto& k = units()[(size_t) preview].knobs[(size_t) z.a];
            return cuid (outer && k.v2 >= 0 ? k.v2 : k.v);
        }
        default: break;
    }
    return {};
}

void Canvas::startKnobDrag (const juce::String& pid)
{
    dragPid = pid;
    if (auto* p = proc.apvts.getParameter (pid)) { dragStart = p->getValue(); proc.beginGesture (pid); }
}

void Canvas::mouseMove (const juce::MouseEvent& e)
{
    mouse = e.position;
    int hu = -1;
    // V4: touching a band selects it, so its panel opens under it
    if (! dragging)
        if (auto* z = zoneAt (mouse))
        {
            if (z->kind == Hit::TahorNode) selBand = z->a;
            if (z->kind == Hit::ResoNode)  selNode = z->a;
            if (z->kind == Hit::CharNode)  selCharNode = z->a;
        }
    if (auto* z = zoneAt (mouse)) if (z->kind == Hit::PeakDot || z->kind == Hit::ChainChip) hu = z->kind == Hit::PeakDot ? z->a : proc.chain()[(size_t) z->a];
    hoverUnit = hu;
    repaint();
}

void Canvas::mouseExit (const juce::MouseEvent&) { mouse = { -1, -1 }; hoverUnit = -1; repaint(); }

void Canvas::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();   // V11: so the shortcuts work
    if (e.mods.isAltDown())   // V11: alt-click resets
    {
        if (auto* z0 = zoneAt (e.position))
            if (z0->kind == Hit::ResoKnob || z0->kind == Hit::TiltKnob || z0->kind == Hit::LinkKnob || z0->kind == Hit::ResoNodeKnob || z0->kind == Hit::TahorKnob
                || z0->kind == Hit::InBox || z0->kind == Hit::OutBox || z0->kind == Hit::UnitKnob || z0->kind == Hit::CharKnob || z0->kind == Hit::CharNode)
            { resetAt (*z0); repaint(); return; }
            else if (z0->kind == Hit::KnobValue) { Zone k = *z0; k.kind = z0->k0; resetAt (k); repaint(); return; }
    }
    if (e.mods.isPopupMenu())   // V16: right-click a knob or its number - type, default, or pick (like Pro-Q 4)
        if (auto* z0 = zoneAt (e.position))
            if (isKnobHit (z0->kind) || z0->kind == Hit::KnobValue || z0->kind == Hit::UnitKnob || z0->kind == Hit::InBox || z0->kind == Hit::OutBox)
            { if (typeIn != nullptr) closeTypeIn (true); showParamMenu (pidOf (*z0), z0->r); return; }
    if (e.mods.isPopupMenu())   // V12: right-click (or two-finger click) on a dot - its menu
        if (auto* z0 = zoneAt (e.position))
            if (z0->kind == Hit::TahorNode || z0->kind == Hit::ResoNode || z0->kind == Hit::CharNode)
            {
                if (z0->kind == Hit::TahorNode) selBand = z0->a;
                if (z0->kind == Hit::ResoNode) selNode = z0->a;
                if (z0->kind == Hit::CharNode) { selCharNode = z0->a; if (proc.unitSplit (preview)) charSide = z0->b; }
                showNodeMenu (z0->kind, z0->a, z0->b, z0->r);
                return;
            }
    if (typeIn != nullptr) closeTypeIn (true);
    if (auto* zt = zoneAt (e.position))
        if (zt->kind == Hit::BubbleEdit) { openTypeIn (*zt); return; }
    if (auto* zc = zoneAt (e.position); zc != nullptr && zc->kind == Hit::Chain)   // V16: a block of the chain strip
    { down = *zc; downAt = e.position; mouse = e.position; dragging = true; tchainDrag = -1; dragPid = {}; dragPid2 = {}; return; }   // V14: click a value in the bubble to type it
    downAt = e.position; mouse = e.position;
    dragging = true;
    dragPid = {}; dragPid2 = {};
    chainDragFrom = -1;
    auto* z = zoneAt (e.position);
    down = z != nullptr ? *z : Zone {};
    switch (down.kind)
    {
        case Hit::ResoKnob: case Hit::TiltKnob: case Hit::LinkKnob: case Hit::ResoNodeKnob: case Hit::InBox: case Hit::OutBox: case Hit::CharKnob:
            startKnobDrag (knobPid (down, false)); break;
        case Hit::Thd:   // V7: jump to where you press, then drag across
            dragPid = "charThd"; proc.beginGesture (dragPid);
            proc.dragParam (dragPid, juce::jlimit (0.0f, 100.0f, (e.position.x - down.r.getX() - 6.0f) / (down.r.getWidth() - 12.0f) * 100.0f));
            break;
        case Hit::TahorKnob:
            startKnobDrag (down.a == 1 && e.mods.isShiftDown() ? bid (selBand, "Rng") : knobPid (down, false)); break;
        case Hit::UnitKnob:
        {
            const auto c = down.r.getCentre();
            dragOuter = down.b == 1 && e.position.getDistanceFrom (c) > down.r.getWidth() * 0.5f * 0.62f;
            startKnobDrag (knobPid (down, dragOuter));
            break;
        }
        case Hit::TahorChip:
            if (down.a == 0 || (down.a == 9 && proc.param (bid (selBand, "Dyn")) > 0.5f)) startKnobDrag (bid (selBand, "Thr"));   // drag up / down = threshold
            break;
        case Hit::ResoNode:
            selNode = down.a;
            dragPid = nid (selNode, "Hz"); dragPid2 = nid (selNode, "Amt");
            proc.beginGesture (dragPid); proc.beginGesture (dragPid2);
            break;
        case Hit::TahorNode:
            if (e.mods.isShiftDown())   // V12: shift-click adds / removes it from the selection
            {
                if (selSet.empty() && proc.param (bid (selBand, "On")) > 0.5f) selSet.insert (selBand);
                if (selSet.count (down.a)) selSet.erase (down.a); else selSet.insert (down.a);
                down.kind = Hit::None;
                break;
            }
            if (! selSet.count (down.a)) selSet.clear();
            selBand = down.a;
            if (selSet.size() > 1)   // V12: move the selected bands together
            {
                groupStart.clear();
                for (int b : selSet) { groupStart.push_back ({ proc.param (bid (b, "Hz")), proc.param (bid (b, "Gain")) }); proc.beginGesture (bid (b, "Hz")); proc.beginGesture (bid (b, "Gain")); }
                break;
            }
            dragPid = bid (selBand, "Hz"); dragPid2 = bid (selBand, "Gain");
            proc.beginGesture (dragPid); proc.beginGesture (dragPid2);
            break;
        case Hit::TahorBg:   // V12: drag across the background to select several bands
            lasso = true; lassoA = lassoB = e.position;
            break;
        case Hit::DynRange:   // V12: pull the dynamic range up or down on the screen; V14: from anywhere on its outline
            selBand = down.a;
            dragPid = bid (selBand, "Rng"); proc.beginGesture (dragPid);
            dragStart = proc.param (dragPid);
            break;
        case Hit::SpecPeak:   // V12: Spectrum Grab - the peak becomes a band, and you're holding it
            if (down.a < (int) specPeaks.size())
            {
                const auto pk = specPeaks[(size_t) down.a];
                const int before = selBand;
                addTahorBand ({ pk.x + display.getX(), pk.y + display.getY() });
                if (selBand != before || proc.param (bid (selBand, "On")) > 0.5f)
                {
                    proc.setParam (bid (selBand, "Q"), 4.0f);
                    down = Zone { down.r, Hit::TahorNode, selBand };
                    selSet.clear();
                    dragPid = bid (selBand, "Hz"); dragPid2 = bid (selBand, "Gain");
                    proc.beginGesture (dragPid); proc.beginGesture (dragPid2);
                }
            }
            break;
        case Hit::FocusEdge:
            dragPid = down.a == 0 ? "rsLow" : "rsHigh"; proc.beginGesture (dragPid);
            break;
        case Hit::ChainChip:
            chainDragFrom = down.a;
            break;
        case Hit::CharNode:
        {
            selCharNode = down.a;
            if (proc.unitSplit (preview)) charSide = down.b;   // V9: a split unit: the side you touched
            const auto& nd = unitNodes (preview)[(size_t) down.a];
            if (nd.fv >= 0) { dragPid = cuid (nd.fv); proc.beginGesture (dragPid); }
            if (nd.gv >= 0) { dragPid2 = cuid (nd.gv); proc.beginGesture (dragPid2); }
            break;
        }
        case Hit::Resize:
            resizeFrom = std::sqrt (std::abs (getTransform().getDeterminant()));
            break;
        default: break;
    }
    repaint();
}

void Canvas::mouseDrag (const juce::MouseEvent& e)
{
    mouse = e.position;
    const auto d = e.position - downAt;
    if (down.kind == Hit::Chain)
    {
        if (tchainDrag < 0 && std::abs (d.x) > 4.0f) tchainDrag = down.a;
        if (tchainDrag >= 0) tchainDragX = e.position.x;
        repaint();
        return;
    }
    switch (down.kind)
    {
        case Hit::ResoKnob: case Hit::TiltKnob: case Hit::LinkKnob: case Hit::ResoNodeKnob: case Hit::TahorKnob:
        case Hit::InBox: case Hit::OutBox: case Hit::UnitKnob: case Hit::TahorChip: case Hit::CharKnob:
            if (dragPid.isNotEmpty())
                if (auto* p = proc.apvts.getParameter (dragPid))
                {
                    const bool fine = e.mods.isCommandDown() || (e.mods.isShiftDown() && down.kind != Hit::TahorKnob);   // V16: Command = fine (Pro-Q 4)
                    const float sens = fine ? 1000.0f : 200.0f;
                    p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - d.y / sens));
                }
            break;
        case Hit::Thd:
            proc.dragParam (dragPid, juce::jlimit (0.0f, 100.0f, (e.position.x - down.r.getX() - 6.0f) / (down.r.getWidth() - 12.0f) * 100.0f));
            break;
        case Hit::ResoNode:
        {
            const float x = juce::jlimit (0.0f, PW, e.position.x - display.getX());
            const float y = e.position.y - display.getY();
            proc.dragParam (dragPid, juce::jlimit (20.0f, 20000.0f, xf (x)));
            proc.dragParam (dragPid2, juce::jlimit (-10.0f, 10.0f, (330.0f - y) / 40.0f));
            break;
        }
        case Hit::TahorBg:
            lassoB = e.position;
            break;
        case Hit::DynRange:
        {
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            const float k = juce::jmax (0.05f, (float) (down.b > 0 ? down.b : 1000) / 1000.0f);   // a spot near the edge moves less per dB
            const float dDb = -(e.position.y - downAt.y) / 335.0f * rng;
            proc.dragParam (dragPid, juce::jlimit (-24.0f, 24.0f, dragStart + dDb / k));
            break;
        }
        case Hit::TahorNode:
        if (selSet.size() > 1 && selSet.count (down.a) && ! groupStart.empty())   // V12: a group moves together
        {
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            const float ratio = xf (e.position.x - display.getX()) / xf (downAt.x - display.getX());
            const float dg = -(e.position.y - downAt.y) / 335.0f * rng;
            size_t i = 0;
            for (int b : selSet)
            {
                proc.dragParam (bid (b, "Hz"), juce::jlimit (10.0f, 30000.0f, groupStart[i].first * ratio));
                if ((int) proc.param (bid (b, "Type")) <= 2) proc.dragParam (bid (b, "Gain"), juce::jlimit (-30.0f, 30.0f, groupStart[i].second + dg));
                ++i;
            }
            break;
        }
        else
        {
            const float x = juce::jlimit (0.0f, PW, e.position.x - display.getX());
            const float y = e.position.y - display.getY();
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            proc.dragParam (dragPid, juce::jlimit (10.0f, 30000.0f, xf (x)));
            const int type = (int) proc.param (bid (selBand, "Type"));
            if (type <= 2) proc.dragParam (dragPid2, juce::jlimit (-30.0f, 30.0f, (335.0f - y) / 335.0f * rng));
            break;
        }
        case Hit::FocusEdge:
        {
            const float x = juce::jlimit (0.0f, PW, e.position.x - display.getX());
            proc.dragParam (dragPid, juce::jlimit (20.0f, 20000.0f, xf (x)));
            break;
        }
        case Hit::CharNode:
        {
            const auto& nd = unitNodes (preview)[(size_t) down.a];
            const float x = juce::jlimit (0.0f, PW, e.position.x - display.getX() - nodePairOffset (preview, down.a));   // V9: paired dots sit apart
            const float y = e.position.y - display.getY();
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            if (nd.fv >= 0) if (auto* p = proc.apvts.getParameter (dragPid)) p->setValueNotifyingHost (freqPos (nd, xf (x)));
            if (nd.gv >= 0) if (auto* p = proc.apvts.getParameter (dragPid2)) p->setValueNotifyingHost (gainPos (nd, (335.0f - y) / 335.0f * rng));
            break;
        }
        case Hit::ChainChip:
        {
            auto c = proc.chain();
            const int to = juce::jlimit (0, (int) c.size() - 1, chainDragFrom + juce::roundToInt (d.y / 30.0f));   // V4: the chips stack down the left
            if (to != chainDragFrom && chainDragFrom >= 0)
            {
                std::swap (c[(size_t) chainDragFrom], c[(size_t) to]);
                proc.setChain (c);
                chainDragFrom = to; downAt.y += (float) (to - down.a) * 30.0f; down.a = to;
            }
            break;
        }
        case Hit::Resize:
            if (onResize)
            {
                const float s = juce::jlimit (0.6f, 3.0f, resizeFrom * (1.0f + juce::jmax (d.x / W, d.y / H)));
                onResize (s);
            }
            break;
        default: break;
    }
    repaint();
}

void Canvas::chainMouseUp (const juce::MouseEvent&)
{
    dragging = false;
    if (tchainDrag >= 0)   // dropped: the parts in the order they now sit, left to right
    {
        const int (&o)[3] = tahor::chainOrders[juce::jlimit (0, 5, (int) proc.param ("order"))];
        std::vector<std::pair<float, int>> at;
        for (int i = 0; i < 3; ++i) at.push_back ({ o[i] == tchainDrag ? tchainDragX : chainSlot[i], o[i] });
        std::stable_sort (at.begin(), at.end(), [] (auto& a, auto& b) { return a.first < b.first; });
        proc.setParam ("order", (float) tahor::chainOrderIndex (at[0].second, at[1].second, at[2].second));
    }
    else tab = juce::jlimit (0, 2, down.a);   // a click: its tab
    tchainDrag = -1; down = {};
    proc.apvts.state.setProperty ("uiTab", tab, nullptr);
    repaint();
}

void Canvas::mouseUp (const juce::MouseEvent& e)
{
    if (down.kind == Hit::KnobValue && e.position.getDistanceFrom (downAt) <= 3.0f && e.getNumberOfClicks() == 1)   // V16: a click on a knob's number
    {
        dragging = false; const auto z = down; down = {};
        openParamTypeIn (pidOf (z), z.r);
        return;
    }
    if (down.kind == Hit::Chain) { chainMouseUp (e); return; }
    dragging = false;
    if (dragPid.isNotEmpty()) proc.endGesture (dragPid);
    if (dragPid2.isNotEmpty()) proc.endGesture (dragPid2);
    if (! groupStart.empty()) { for (int b : selSet) { proc.endGesture (bid (b, "Hz")); proc.endGesture (bid (b, "Gain")); } groupStart.clear(); }
    const bool moved = e.position.getDistanceFrom (downAt) > 3.0f;
    if (lasso)   // V12: the bands inside the box are selected
    {
        lasso = false;
        if (moved)
        {
            const R lr = R::leftTopRightBottom (juce::jmin (lassoA.x, lassoB.x), juce::jmin (lassoA.y, lassoB.y), juce::jmax (lassoA.x, lassoB.x), juce::jmax (lassoA.y, lassoB.y));
            selSet.clear();
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            for (int b = 0; b < numBands; ++b)
                if (proc.param (bid (b, "On")) > 0.5f)
                {
                    const float gdb = (int) proc.param (bid (b, "Type")) <= 2 ? proc.param (bid (b, "Gain")) : 0.0f;
                    const P pt (display.getX() + fx (proc.param (bid (b, "Hz"))), display.getY() + 335.0f - juce::jlimit (-rng * 0.95f, rng * 0.95f, gdb) / rng * 335.0f);
                    if (lr.contains (pt)) selSet.insert (b);
                }
            if (! selSet.empty()) selBand = *selSet.begin();
            dragPid = {}; dragPid2 = {};
            repaint();
            return;
        }
        selSet.clear();
    }
    if (! moved && down.kind != Hit::None) click (down, e);
    dragPid = {}; dragPid2 = {};
    auto& st = proc.apvts.state;
    st.setProperty ("uiTab", tab, nullptr); st.setProperty ("uiPreview", preview, nullptr);
    repaint();
}

void Canvas::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto* z = zoneAt (e.position);
    if (z == nullptr) return;
    auto reset = [this] (const juce::String& pid) { if (auto* p = proc.apvts.getParameter (pid)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); } };
    juce::ignoreUnused (reset);
    switch (z->kind)
    {
        case Hit::ResoKnob: case Hit::TiltKnob: case Hit::LinkKnob: case Hit::ResoNodeKnob: case Hit::TahorKnob:
        case Hit::InBox: case Hit::OutBox: case Hit::UnitKnob: case Hit::CharKnob: case Hit::KnobValue:
            if (typeIn == nullptr) openParamTypeIn (pidOf (*z), z->r);   // V16: double-click types a value (Option-click resets), like Pro-Q 4
            break;
        case Hit::Thd: proc.setParam ("charThd", 50.0f); break;
        case Hit::ResoNode: proc.setParam (nid (z->a, "On"), 0.0f); break;
        case Hit::TahorNode: proc.setParam (bid (z->a, "On"), 0.0f); break;
        case Hit::ResoBg: addResoNode (e.position); break;
        case Hit::CharNode: proc.charOff[(size_t) preview].fetch_xor (1 << z->a); break;   // V12: double-click switches a Character band off / on
        case Hit::TahorBg: addTahorBand (e.position); break;
        case Hit::Hardware:   // V14: double-click the gear at the bottom to add it to the chain (no need for the list)
        {
            auto c = proc.chain();
            if ((int) c.size() < maxChain && std::find (c.begin(), c.end(), preview) == c.end()) { c.push_back (preview); proc.setChain (c); }
            break;
        }
        default: break;
    }
    repaint();
}

// V14: type a value into the bubble - "1k", "2.5 kHz", "440", a note ("A4", "C#3"), "-3", "0.7"
static bool parseTyped (juce::String s, int kind, float& out)
{
    s = s.trim().toLowerCase().replace (",", ".");
    if (s.isEmpty()) return false;
    if (kind == 0)
    {
        static const char* nm[12] { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };
        for (int n = 11; n >= 0; --n)   // a note name first (sharps before naturals)
            if (s.startsWith (nm[n]) && s.length() > (int) strlen (nm[n]))
            {
                auto rest = s.substring ((int) strlen (nm[n])).trim();
                if (rest.containsOnly ("-0123456789"))
                {
                    const int midi = (rest.getIntValue() + 1) * 12 + n;
                    out = 440.0f * std::pow (2.0f, (float) (midi - 69) / 12.0f);
                    return true;
                }
            }
        float mult = 1.0f;
        if (s.containsChar ('k')) { mult = 1000.0f; }
        auto numPart = s.retainCharacters ("-0123456789.");
        if (numPart.isEmpty() || ! numPart.containsAnyOf ("0123456789")) return false;
        out = numPart.getFloatValue() * mult;
        return out > 0.0f;
    }
    auto numPart = s.retainCharacters ("-+0123456789.");
    if (! numPart.containsAnyOf ("0123456789")) return false;
    out = numPart.getFloatValue();
    return true;
}

void Canvas::openTypeIn (const Zone& z)
{
    const bool reso = z.b >= 100;
    const int idx = reso ? z.b - 100 : z.b;
    static const char* tn[3] { "Hz", "Gain", "Q" };
    static const char* rn[3] { "Hz", "Amt", "Q" };
    if (z.a == 3)   // the shape: a menu, like Pro-Q's
    {
        const juce::String pid = reso ? nid (idx, "Shape") : bid (idx, "Type");
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (pid)))
        {
            juce::PopupMenu m;
            for (int i = 0; i < c->choices.size(); ++i) m.addItem (1 + i, c->choices[i], true, i == c->getIndex());
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r).getSmallestIntegerContainer()),
                [this, pid] (int r) { if (r > 0) proc.setParam (pid, (float) (r - 1)); lastWant = juce::Time::getMillisecondCounterHiRes(); repaint(); });
        }
        return;
    }
    typeInKind = z.a;
    typeInPid = reso ? nid (idx, rn[z.a]) : bid (idx, tn[z.a]);
    typeIn = std::make_unique<juce::TextEditor>();
    auto& t = *typeIn;
    t.setFont (ui::font (ui::Face::Regular, 12.0f));
    t.setJustification (juce::Justification::centred);
    t.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1c1c22));
    t.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    t.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xffe9a23b));
    t.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xffe9a23b));
    t.setColour (juce::TextEditor::highlightColourId, juce::Colour (0x66e9a23b));
    t.setIndents (4, 3);
    const float v = proc.param (typeInPid);
    auto trimZ = [] (juce::String s) { if (s.containsChar ('.')) s = s.trimCharactersAtEnd ("0").trimCharactersAtEnd ("."); return s; };
    t.setText (z.a == 0 ? (v >= 1000.0f ? trimZ (juce::String (v / 1000.0f, 2)) + "k" : juce::String (juce::roundToInt (v)))
                        : juce::String (v, z.a == 2 ? 2 : 1), false);
    const auto r = z.r.withSizeKeepingCentre (juce::jmax (z.r.getWidth() + 16.0f, 58.0f), 20.0f);
    t.setBounds (r.getSmallestIntegerContainer());
    addAndMakeVisible (t);
    t.selectAll();
    t.grabKeyboardFocus();
    const juce::Component::SafePointer<Canvas> sp (this);
    t.onReturnKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
    t.onEscapeKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (false); }); };
    t.onFocusLost = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
}

// V16: type any knob's value (double-click it, click its number, or "Type a value..." on its right-click menu)
void Canvas::openParamTypeIn (const juce::String& pid, R near)
{
    auto* p = proc.apvts.getParameter (pid);
    if (p == nullptr || dynamic_cast<juce::AudioParameterBool*> (p) != nullptr) return;
    if (typeIn != nullptr) closeTypeIn (true);
    typeInKind = 9;
    typeInPid = pid;
    typeIn = std::make_unique<juce::TextEditor>();
    auto& t = *typeIn;
    t.setFont (ui::font (ui::Face::Regular, 12.0f));
    t.setJustification (juce::Justification::centred);
    t.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1c1c22));
    t.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    t.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xffe9a23b));
    t.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xffe9a23b));
    t.setColour (juce::TextEditor::highlightColourId, juce::Colour (0x66e9a23b));
    t.setIndents (4, 3);
    juce::String now;
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p)) now = c->getCurrentChoiceName();
    else
    {
        const float v = proc.param (pid);
        const auto range = proc.apvts.getParameterRange (pid);
        const int dec = range.interval >= 1.0f ? 0 : (std::abs (v) >= 100.0f ? 0 : (range.interval >= 0.1f || range.interval <= 0.0f ? 1 : 2));
        now = std::abs (v) >= 10000.0f ? juce::String (v / 1000.0f, 1) + "k" : juce::String (v, dec);
        if (now.containsChar ('.')) now = now.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    }
    t.setText (now, false);
    auto r = near.withSizeKeepingCentre (juce::jlimit (64.0f, 110.0f, near.getWidth()), 22.0f);
    r = r.constrainedWithin (getLocalBounds().toFloat().reduced (4));
    t.setBounds (r.getSmallestIntegerContainer());
    addAndMakeVisible (t);
    t.selectAll();
    t.grabKeyboardFocus();
    const juce::Component::SafePointer<Canvas> sp (this);
    t.onReturnKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
    t.onEscapeKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (false); }); };
    t.onFocusLost = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
    repaint();
}

// V16: right-click a knob - type a value, back to its default, or pick a setting
void Canvas::showParamMenu (const juce::String& pid, R near)
{
    auto* p = proc.apvts.getParameter (pid);
    if (p == nullptr) return;
    juce::PopupMenu m;
    m.addSectionHeader (p->getName (40));
    if (dynamic_cast<juce::AudioParameterBool*> (p) == nullptr) m.addItem (1, "Type a value...");
    m.addItem (2, "Back to default  (" + (p->getText (p->getDefaultValue(), 20) + " " + p->getLabel()).trim() + ")");
    auto* c = dynamic_cast<juce::AudioParameterChoice*> (p);
    if (c != nullptr && c->choices.size() <= 40)
    {
        m.addSeparator();
        for (int i = 0; i < c->choices.size(); ++i) m.addItem (100 + i, c->choices[i], true, i == c->getIndex());
    }
    m.addSeparator();
    m.addItem (3, "Fine: hold Command while dragging", false, false);
    m.addItem (4, "Reset: Option-click   Type: double-click", false, false);
    const juce::Component::SafePointer<Canvas> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (near).getSmallestIntegerContainer()),
        [sp, pid, near] (int r)
        {
            if (sp == nullptr || r <= 0) return;
            auto* q = sp->proc.apvts.getParameter (pid);
            if (q == nullptr) return;
            if (r == 1) sp->openParamTypeIn (pid, near);
            if (r == 2) { q->beginChangeGesture(); q->setValueNotifyingHost (q->getDefaultValue()); q->endChangeGesture(); }
            if (r >= 100) sp->proc.setParam (pid, (float) (r - 100));
            sp->repaint();
        });
}

void Canvas::closeTypeIn (bool commit)
{
    if (typeIn == nullptr) return;
    float v = 0.0f;
    bool ok = false;
    if (commit && typeInKind == 9)   // Pressure: any number; a choice by its name or the nearest of its values
    {
        const auto txt = typeIn->getText().trim();
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (typeInPid)))
        {
            auto numOf = [] (juce::String s, float& f)
            {
                s = s.toLowerCase().replace (",", ".");
                const float m = s.containsChar ('k') ? 1000.0f : 1.0f;
                const auto n = s.retainCharacters ("-0123456789.");
                if (! n.containsAnyOf ("0123456789")) return false;
                f = n.getFloatValue() * m; return true;
            };
            for (int i = 0; i < c->choices.size() && ! ok; ++i)
                if (c->choices[i].trim().equalsIgnoreCase (txt)) { v = (float) i; ok = true; }
            float want = 0.0f;
            if (! ok && numOf (txt, want))
            {
                float best = 1.0e9f;
                for (int i = 0; i < c->choices.size(); ++i)
                    if (float f = 0.0f; numOf (c->choices[i], f) && std::abs (f - want) < best) { best = std::abs (f - want); v = (float) i; ok = true; }
                if (! ok && want >= 1.0f && want <= (float) c->choices.size()) { v = want - 1.0f; ok = true; }   // or its place in the list
            }
        }
        else
        {
            auto s = txt.toLowerCase().replace (",", ".");
            const float m = s.containsChar ('k') ? 1000.0f : 1.0f;
            auto n = s.retainCharacters ("-+0123456789.");
            if (n.containsAnyOf ("0123456789")) { v = n.getFloatValue() * m; ok = true; }
        }
    }
    else if (commit) ok = parseTyped (typeIn->getText(), typeInKind, v);
    if (ok)
        if (auto* p = proc.apvts.getParameter (typeInPid))
        {
            const auto range = proc.apvts.getParameterRange (typeInPid);
            p->beginChangeGesture();
            p->setValueNotifyingHost (range.convertTo0to1 (range.snapToLegalValue (v)));
            p->endChangeGesture();
        }
    auto old = std::move (typeIn);   // removed after this call returns
    removeChildComponent (old.get());
    juce::MessageManager::callAsync ([o = std::shared_ptr<juce::TextEditor> (old.release())] {});
    grabKeyboardFocus();
    lastWant = juce::Time::getMillisecondCounterHiRes();
    repaint();
}

// V12: reset what's under the mouse (alt-click)
void Canvas::resetAt (const Zone& z)
{
    auto reset = [this] (const juce::String& pid) { if (auto* p = proc.apvts.getParameter (pid)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); } };
    switch (z.kind)
    {
        case Hit::ResoKnob: case Hit::TiltKnob: case Hit::LinkKnob: case Hit::ResoNodeKnob: case Hit::TahorKnob:
        case Hit::InBox: case Hit::OutBox: case Hit::UnitKnob: case Hit::CharKnob:
            reset (knobPid (z, false)); break;
        case Hit::CharNode:
        {
            const auto& nd = unitNodes (preview)[(size_t) z.a];
            if (nd.gv >= 0) proc.setParam (cuid (nd.gv), nd.gmode == 0 || nd.gmode == 3 ? 0.5f : 0.0f);
            else if (nd.fv >= 0) proc.setParam (cuid (nd.fv), 0.0f);
            break;
        }
        default: break;
    }
}

// V12: cut / copy / paste a band
void Canvas::copyBand (Hit kind, int idx)
{
    clip = {};
    if (kind == Hit::TahorNode)
    {
        clip.kind = 0;
        for (auto* w : { "Type", "Hz", "Gain", "Q", "Slope", "Place", "Dyn", "Thr", "Rng", "Att", "Rel", "Sc" }) clip.v.push_back ({ w, proc.param (bid (idx, w)) });
    }
    else if (kind == Hit::ResoNode)
    {
        clip.kind = 1;
        for (auto* w : { "Shape", "Hz", "Amt", "Q" }) clip.v.push_back ({ w, proc.param (nid (idx, w)) });
    }
}

void Canvas::pasteBand (Hit kind)
{
    if (kind == Hit::TahorNode && clip.kind == 0)
        for (int b = 0; b < numBands; ++b)
            if (proc.param (bid (b, "On")) < 0.5f)
            {
                for (auto& [w, v] : clip.v) proc.setParam (bid (b, w.toRawUTF8()), v);
                proc.setParam (bid (b, "Byp"), 0.0f); proc.setParam (bid (b, "Pair"), 0.0f);
                proc.setParam (bid (b, "On"), 1.0f); selBand = b; return;
            }
    if (kind == Hit::ResoNode && clip.kind == 1)
        for (int i = 0; i < maxResoNodes; ++i)
            if (proc.param (nid (i, "On")) < 0.5f)
            {
                for (auto& [w, v] : clip.v) proc.setParam (nid (i, w.toRawUTF8()), v);
                proc.setParam (nid (i, "Byp"), 0.0f); proc.setParam (nid (i, "Pair"), 0.0f);
                proc.setParam (nid (i, "On"), 1.0f); selNode = i; return;
            }
}

// V12: the right-click menu on a dot (Pro-Q style); only what the tab can do is listed
void Canvas::showNodeMenu (Hit kind, int idx, int side, R at)
{
    juce::PopupMenu m;
    if (kind == Hit::TahorNode)
    {
        const bool byp = proc.param (bid (idx, "Byp")) > 0.5f, dyn = proc.param (bid (idx, "Dyn")) > 0.5f;
        const int type = (int) proc.param (bid (idx, "Type")), place = (int) proc.param (bid (idx, "Place"));
        m.addItem (1, byp ? "Enable" : "Disable");
        if (type <= 2) m.addItem (2, "Invert gain");
        if (type <= 2) m.addItem (3, dyn ? "Make static" : "Make dynamic");
        m.addItem (4, "Solo", true, proc.param (bid (idx, "Solo")) > 0.5f);
        juce::PopupMenu shape; const char* tn[6] { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" };
        for (int i = 0; i < 6; ++i) shape.addItem (10 + i, tn[i], true, i == type);
        m.addSubMenu ("Shape", shape);
        if (type == 3 || type == 4)
        {
            juce::PopupMenu sl; const char* sn[6] { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
            for (int i = 0; i < 6; ++i) sl.addItem (20 + i, sn[i], true, i == (int) proc.param (bid (idx, "Slope")));
            m.addSubMenu ("Slope", sl);
        }
        juce::PopupMenu pl; const char* pn[5] { "Stereo", "Mid", "Side", "Left", "Right" };
        for (int i = 0; i < 5; ++i) pl.addItem (30 + i, pn[i], true, i == place);
        m.addSubMenu ("Stereo placement", pl);
        m.addSeparator();
        m.addItem (40, "Cut"); m.addItem (41, "Copy"); m.addItem (42, "Paste", clip.kind == 0);
        m.addItem (43, "Split (left / right)", place < 3);
        m.addItem (44, "Delete");
    }
    else if (kind == Hit::ResoNode)
    {
        const bool byp = proc.param (nid (idx, "Byp")) > 0.5f;
        const int shape = (int) proc.param (nid (idx, "Shape")), place = (int) proc.param (nid (idx, "Place"));
        m.addItem (1, byp ? "Enable" : "Disable");
        m.addItem (2, "Invert amount");
        m.addItem (4, "Solo", true, resoSolo == idx);
        juce::PopupMenu sh; const char* sn[8] { "Bell", "Bandpass", "Low shelf", "High shelf", "Tilt", "Notch", "Low cut", "High cut" };
        for (int i = 0; i < 8; ++i) sh.addItem (10 + i, sn[i], true, i == shape);
        m.addSubMenu ("Shape", sh);
        juce::PopupMenu pl; const char* pn[3] { "Stereo", "Left", "Right" };
        for (int i = 0; i < 3; ++i) pl.addItem (30 + i, pn[i], true, i == place);
        m.addSubMenu ("Stereo placement", pl);
        m.addSeparator();
        m.addItem (40, "Cut"); m.addItem (41, "Copy"); m.addItem (42, "Paste", clip.kind == 1);
        m.addItem (43, "Split (left / right)", place == 0);
        m.addItem (44, "Delete");
    }
    else
    {
        const auto& nd = unitNodes (preview)[(size_t) idx];
        const bool dis = (proc.charOff[(size_t) preview].load() >> idx) & 1;
        m.addItem (1, dis ? "Enable" : "Disable");
        if (nd.gv >= 0 && nd.gmode == 0) m.addItem (2, "Invert gain");
        m.addItem (4, "Solo", true, charSolo == idx && charSoloUnit == preview);
        juce::PopupMenu pl; const char* pn[3] { "Stereo", "Left", "Right" };
        const int cs = proc.unitSplit (preview) ? 1 + charSide : 0;
        for (int i = 0; i < 3; ++i) pl.addItem (30 + i, pn[i], true, i == cs);
        m.addSubMenu ("Stereo placement", pl);
        m.addSeparator();
        m.addItem (45, "Reset to the hardware's rest");
    }
    juce::ignoreUnused (side);
    juce::Component::SafePointer<Canvas> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at.toNearestInt())), [safe, kind, idx] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& c = *safe; auto& p = c.proc;
        auto flip = [&p] (const juce::String& pid) { p.setParam (pid, p.param (pid) > 0.5f ? 0.0f : 1.0f); };
        if (kind == Hit::TahorNode)
        {
            if (r == 1) flip (bid (idx, "Byp"));
            if (r == 2) p.setParam (bid (idx, "Gain"), -p.param (bid (idx, "Gain")));
            if (r == 3) { flip (bid (idx, "Dyn")); p.setParam (bid (idx, "Spec"), 0.0f); }
            if (r == 4) { const bool was = p.param (bid (idx, "Solo")) > 0.5f; for (int b = 0; b < numBands; ++b) p.setParam (bid (b, "Solo"), 0.0f); p.setParam (bid (idx, "Solo"), was ? 0.0f : 1.0f); }
            if (r >= 10 && r < 16) p.setParam (bid (idx, "Type"), (float) (r - 10));
            if (r >= 20 && r < 26) p.setParam (bid (idx, "Slope"), (float) (r - 20));
            if (r >= 30 && r < 35) c.splitTahor (idx, r - 30);
            if (r == 40) { c.copyBand (kind, idx); p.setParam (bid (idx, "On"), 0.0f); }
            if (r == 41) c.copyBand (kind, idx);
            if (r == 42) c.pasteBand (kind);
            if (r == 43) c.splitTahor (idx, 3);
            if (r == 44) p.setParam (bid (idx, "On"), 0.0f);
        }
        else if (kind == Hit::ResoNode)
        {
            if (r == 1) flip (nid (idx, "Byp"));
            if (r == 2) p.setParam (nid (idx, "Amt"), -p.param (nid (idx, "Amt")));
            if (r == 4) c.resoSolo = c.resoSolo == idx ? -1 : idx;
            if (r >= 10 && r < 18) p.setParam (nid (idx, "Shape"), (float) (r - 10));
            if (r >= 30 && r < 33) c.splitReso (idx, r - 30);
            if (r == 40) { c.copyBand (kind, idx); p.setParam (nid (idx, "On"), 0.0f); }
            if (r == 41) c.copyBand (kind, idx);
            if (r == 42) c.pasteBand (kind);
            if (r == 43) c.splitReso (idx, 1);
            if (r == 44) p.setParam (nid (idx, "On"), 0.0f);
        }
        else
        {
            const auto& nd = unitNodes (c.preview)[(size_t) idx];
            if (r == 1) p.charOff[(size_t) c.preview].fetch_xor (1 << idx);
            if (r == 2 && nd.gv >= 0) p.setParam (c.cuid (nd.gv), 1.0f - p.param (c.cuid (nd.gv)));
            if (r == 4) { if (c.charSolo == idx && c.charSoloUnit == c.preview) { c.charSolo = -1; c.charSoloUnit = -1; } else { c.charSolo = idx; c.charSoloUnit = c.preview; } }
            if (r >= 30 && r < 33) c.setCharSide (r - 30);
            if (r == 45) c.resetAt (Zone { {}, Hit::CharNode, idx });
        }
        c.repaint();
    });
}

// V12: EQ MATCH - the reference (sidechain) minus your sound, smoothed, fitted with up to six Tahor bells
void Canvas::applyEqMatch()
{
    if (eqCount < 10) return;
    const float sr = (float) juce::jmax (44100.0, proc.getRate()), binHz = sr / 8192.0f;
    std::vector<float> hz, d;
    for (int i = 0; i < 120; ++i)
    {
        const float f = 30.0f * std::pow (16000.0f / 30.0f, (float) i / 119.0f);
        const int k0 = juce::jlimit (1, 4095, (int) (f * 0.891f / binHz)), k1 = juce::jlimit (k0 + 1, 4096, (int) (f * 1.122f / binHz) + 1);
        double a = 0, b = 0; for (int k = k0; k <= k1; ++k) { a += accKey[(size_t) k]; b += accPre[(size_t) k]; }
        hz.push_back (f); d.push_back ((float) ((a - b) / (double) ((k1 - k0 + 1) * eqCount)));
    }
    double mean = 0, wn = 0;   // level doesn't count - only the tone
    for (size_t i = 0; i < hz.size(); ++i) if (hz[i] >= 100 && hz[i] <= 8000) { mean += d[i]; wn += 1; }
    mean /= juce::jmax (1.0, wn);
    for (auto& v : d) v = juce::jlimit (-18.0f, 18.0f, v - (float) mean);
    for (int fit = 0; fit < 6; ++fit)
    {
        int best = -1; float bv = 0;
        for (size_t i = 0; i < d.size(); ++i) if (std::abs (d[i]) > std::abs (bv)) { bv = d[i]; best = (int) i; }
        if (best < 0 || std::abs (bv) < 1.0f) break;
        int lo = best, hi = best;
        while (lo > 0 && std::abs (d[(size_t) lo]) > std::abs (bv) * 0.5f) --lo;
        while (hi < (int) d.size() - 1 && std::abs (d[(size_t) hi]) > std::abs (bv) * 0.5f) ++hi;
        const float bwOct = juce::jmax (0.2f, std::log2 (hz[(size_t) hi] / hz[(size_t) lo]));
        const float q = juce::jlimit (0.4f, 4.0f, 1.0f / bwOct * 1.4f);
        const float gain = juce::jlimit (-9.0f, 9.0f, bv * 0.8f);
        int slot = -1;
        for (int b = 0; b < numBands; ++b) if (proc.param (bid (b, "On")) < 0.5f) { slot = b; break; }
        if (slot < 0) break;
        proc.setParam (bid (slot, "Type"), 0.0f); proc.setParam (bid (slot, "Hz"), hz[(size_t) best]);
        proc.setParam (bid (slot, "Gain"), gain); proc.setParam (bid (slot, "Q"), q);
        proc.setParam (bid (slot, "Place"), 0.0f); proc.setParam (bid (slot, "Byp"), 0.0f); proc.setParam (bid (slot, "On"), 1.0f);
        vox::CleanBand cb; cb.on = true; cb.type = vox::BandType::Bell; cb.freq = hz[(size_t) best]; cb.gain = gain; cb.q = q;
        for (size_t i = 0; i < d.size(); ++i) d[i] -= (float) vox::tahorAnalogBandDb (cb, cb.freq, cb.gain, cb.q, hz[i]);
        selBand = slot;
    }
}

void Canvas::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    auto* z = zoneAt (e.position);
    if (z == nullptr) return;
    juce::String pid;
    switch (z->kind)
    {
        case Hit::ResoKnob: case Hit::TiltKnob: case Hit::LinkKnob: case Hit::ResoNodeKnob: case Hit::TahorKnob:
        case Hit::InBox: case Hit::OutBox: case Hit::UnitKnob: case Hit::CharKnob: pid = knobPid (*z, false); break;
        case Hit::TahorNode: pid = bid (z->a, "Q"); break;
        case Hit::ResoNode: pid = nid (z->a, "Q"); break;
        case Hit::CharNode: { const auto& nd = unitNodes (preview)[(size_t) z->a]; if (nd.qv < 0) return; pid = cuid (nd.qv); break; }
        default: return;
    }
    if (auto* p = proc.apvts.getParameter (pid))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + w.deltaY * 0.05f));
        p->endChangeGesture();
    }
}

//==============================================================================
// V9: split a band into left and right partners (each its own dot, its own settings), or join them back
void Canvas::splitTahor (int b, int place)
{
    const int pair = (int) proc.param (bid (b, "Pair")) - 1;
    const bool hasPair = pair >= 0 && pair != b && proc.param (bid (pair, "On")) > 0.5f;
    if (place >= 3)
    {
        proc.setParam (bid (b, "Place"), (float) place);
        const float other = place == 3 ? 4.0f : 3.0f;
        if (hasPair) { proc.setParam (bid (pair, "Place"), other); return; }
        for (int f = 0; f < numBands; ++f)
            if (f != b && proc.param (bid (f, "On")) < 0.5f)
            {
                for (auto* w : { "Type", "Hz", "Gain", "Q", "Slope", "Dyn", "Thr", "Rng", "Spec", "Sc", "Att", "Rel", "Sel" })
                    proc.setParam (bid (f, w), proc.param (bid (b, w)));
                proc.setParam (bid (f, "Place"), other);
                proc.setParam (bid (f, "Solo"), 0.0f);
                proc.setParam (bid (f, "Pair"), (float) (b + 1));
                proc.setParam (bid (b, "Pair"), (float) (f + 1));
                proc.setParam (bid (f, "On"), 1.0f);
                return;
            }
        return;   // no free band: it just works on one side
    }
    proc.setParam (bid (b, "Place"), (float) place);
    if (hasPair && (int) proc.param (bid (pair, "Pair")) - 1 == b)
    {
        proc.setParam (bid (pair, "On"), 0.0f);
        proc.setParam (bid (pair, "Pair"), 0.0f);
    }
    proc.setParam (bid (b, "Pair"), 0.0f);
}

void Canvas::splitReso (int i, int place)
{
    const int pair = (int) proc.param (nid (i, "Pair")) - 1;
    const bool hasPair = pair >= 0 && pair != i && proc.param (nid (pair, "On")) > 0.5f;
    if (place >= 1)
    {
        proc.setParam (nid (i, "Place"), (float) place);
        const float other = place == 1 ? 2.0f : 1.0f;
        if (hasPair) { proc.setParam (nid (pair, "Place"), other); return; }
        for (int f = 0; f < maxResoNodes; ++f)
            if (f != i && proc.param (nid (f, "On")) < 0.5f)
            {
                for (auto* w : { "Shape", "Hz", "Amt", "Q" }) proc.setParam (nid (f, w), proc.param (nid (i, w)));
                proc.setParam (nid (f, "Place"), other);
                proc.setParam (nid (f, "Pair"), (float) (i + 1));
                proc.setParam (nid (i, "Pair"), (float) (f + 1));
                proc.setParam (nid (f, "On"), 1.0f);
                return;
            }
        return;
    }
    proc.setParam (nid (i, "Place"), 0.0f);
    if (hasPair && (int) proc.param (nid (pair, "Pair")) - 1 == i)
    {
        proc.setParam (nid (pair, "On"), 0.0f);
        proc.setParam (nid (pair, "Pair"), 0.0f);
    }
    proc.setParam (nid (i, "Pair"), 0.0f);
}

void Canvas::setCharSide (int s)
{
    const auto sp = "u" + juce::String (preview) + "split";
    if (s == 0) { proc.setParam (sp, 0.0f); charSide = 0; return; }
    if (! proc.unitSplit (preview))   // the right side starts as a copy of the left
    {
        for (int j = 0; j < unitValues; ++j) proc.setParam ("u" + juce::String (preview) + "r" + juce::String (j), proc.param (uid (preview, j)));
        proc.setParam (sp, 1.0f);
    }
    charSide = s - 1;
}

void Canvas::showHistoryMenu (R at)
{
    const auto names = proc.historyNames();
    const int cur = proc.historyIndex();
    juce::PopupMenu m;
    m.addSectionHeader ("History (newest first)");
    for (int i = names.size() - 1; i >= juce::jmax (0, names.size() - 30); --i)
        m.addItem (1 + i, names[i], i != cur, i == cur);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at.toNearestInt())),
                     [this] (int r) { if (r > 0) proc.jumpHistory (r - 1); repaint(); });
}

// V11: keyboard shortcuts
//   1 / 2 / 3: RESO / TAHOR / CHARACTER      cmd-Z / shift-cmd-Z: undo / redo
//   delete / backspace: remove the selected band (Tahor, Reso)   S: solo the selected band
//   arrows: nudge the selected band (left / right = frequency, up / down = gain; shift = finer)
//   B: bypass   M: match   escape: close the gallery / browser
bool Canvas::keyPressed (const juce::KeyPress& k)
{
    const auto c = k.getTextCharacter();
    const bool cmd = k.getModifiers().isCommandDown(), shift = k.getModifiers().isShiftDown();
    auto toggle = [this] (const juce::String& pid) { proc.setParam (pid, proc.param (pid) > 0.5f ? 0.0f : 1.0f); };
    bool done = true;
    if (cmd && (c == 'z' || c == 'Z' || k.getKeyCode() == 'Z')) { if (shift) proc.redo(); else proc.undo(); }
    else if (cmd) done = false;
    else if (c == '1' || c == '2' || c == '3') tab = c - '1';
    else if (k == juce::KeyPress::escapeKey) { addOpen = false; browseOpen = false; }
    else if (k == juce::KeyPress::deleteKey || k == juce::KeyPress::backspaceKey)
    {
        if (tab == 1 && selSet.size() > 1) { for (int b : selSet) proc.setParam (bid (b, "On"), 0.0f); selSet.clear(); }
        else if (tab == 1 && proc.param (bid (selBand, "On")) > 0.5f) proc.setParam (bid (selBand, "On"), 0.0f);
        else if (tab == 0 && proc.param (nid (selNode, "On")) > 0.5f) proc.setParam (nid (selNode, "On"), 0.0f);
        else done = false;
    }
    else if (c == 's' || c == 'S')
    {
        if (tab == 1) { const bool was = proc.param (bid (selBand, "Solo")) > 0.5f;
                        for (int b = 0; b < numBands; ++b) proc.setParam (bid (b, "Solo"), 0.0f);
                        proc.setParam (bid (selBand, "Solo"), was ? 0.0f : 1.0f); }
        else if (tab == 0) resoSolo = resoSolo == selNode ? -1 : selNode;
        else done = false;
    }
    else if (c == 'b' || c == 'B') toggle ("bypass");
    else if (c == 'm' || c == 'M') toggle ("match");
    else if (k.isKeyCode (juce::KeyPress::leftKey) || k.isKeyCode (juce::KeyPress::rightKey) || k.isKeyCode (juce::KeyPress::upKey) || k.isKeyCode (juce::KeyPress::downKey))
    {
        const bool horiz = k.isKeyCode (juce::KeyPress::leftKey) || k.isKeyCode (juce::KeyPress::rightKey);
        const float dir = (k.isKeyCode (juce::KeyPress::rightKey) || k.isKeyCode (juce::KeyPress::upKey)) ? 1.0f : -1.0f;
        if (tab == 1 && proc.param (bid (selBand, "On")) > 0.5f)
        {
            if (horiz) proc.setParam (bid (selBand, "Hz"), juce::jlimit (10.0f, 30000.0f, proc.param (bid (selBand, "Hz")) * std::pow (2.0f, dir * (shift ? 1.0f / 48 : 1.0f / 12))));
            else proc.setParam (bid (selBand, "Gain"), juce::jlimit (-30.0f, 30.0f, proc.param (bid (selBand, "Gain")) + dir * (shift ? 0.1f : 0.5f)));
        }
        else if (tab == 0 && proc.param (nid (selNode, "On")) > 0.5f)
        {
            if (horiz) proc.setParam (nid (selNode, "Hz"), juce::jlimit (20.0f, 20000.0f, proc.param (nid (selNode, "Hz")) * std::pow (2.0f, dir * (shift ? 1.0f / 48 : 1.0f / 12))));
            else proc.setParam (nid (selNode, "Amt"), juce::jlimit (-6.0f, 6.0f, proc.param (nid (selNode, "Amt")) + dir * (shift ? 0.1f : 0.5f)));
        }
        else done = false;
    }
    else done = false;
    if (done) repaint();
    return done;
}

void Canvas::addTahorBand (P at)
{
    for (int b = 0; b < numBands; ++b)
        if (proc.param (bid (b, "On")) < 0.5f)
        {
            const float x = at.x - display.getX(), y = at.y - display.getY();
            static const float ranges[4] { 3.0f, 6.0f, 12.0f, 30.0f };
            const float rng = ranges[juce::jlimit (0, 3, (int) proc.param ("range"))] * 1.5f;
            proc.setParam (bid (b, "Type"), 0.0f);
            proc.setParam (bid (b, "Hz"), juce::jlimit (10.0f, 30000.0f, xf (x)));
            proc.setParam (bid (b, "Gain"), juce::jlimit (-30.0f, 30.0f, (335.0f - y) / 335.0f * rng));
            proc.setParam (bid (b, "Q"), 1.0f);
            proc.setParam (bid (b, "On"), 1.0f);
            selBand = b;
            return;
        }
}

void Canvas::addResoNode (P at)
{
    for (int i = 0; i < maxResoNodes; ++i)
        if (proc.param (nid (i, "On")) < 0.5f)
        {
            proc.setParam (nid (i, "Shape"), 0.0f);
            proc.setParam (nid (i, "Hz"), juce::jlimit (20.0f, 20000.0f, xf (at.x - display.getX())));
            proc.setParam (nid (i, "Amt"), juce::jlimit (-10.0f, 10.0f, (330.0f - (at.y - display.getY())) / 40.0f));
            proc.setParam (nid (i, "Q"), 1.0f);
            proc.setParam (nid (i, "On"), 1.0f);
            selNode = i;
            return;
        }
}

void Canvas::showPresetMenu()
{
    juce::PopupMenu m;
    // V3: Save and Save As live here
    bool isUser = false;
    for (auto& f : proc.userPresets()) if (f.getFileNameWithoutExtension() == proc.presetName) isUser = true;
    m.addItem (5001, isUser ? "Save" : "Save (as a new preset)");
    m.addItem (5002, "Save As...");
    m.addItem (5003, "Browse presets...");   // V9
    m.addSeparator();
    const auto& f = factoryPresets();
    const auto users = proc.userPresets();
    for (auto& tag : presetTagNames())   // V12: one submenu per folder
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) f.size(); ++i)
            if (TahorProcessor::factoryTag (i) == tag)
            { const auto nm = juce::String::fromUTF8 (f[(size_t) i].name); sub.addItem (1 + i, nm, true, nm == proc.presetName); }
        for (int i = 0; i < users.size(); ++i)
            if (proc.presetTagOf (users[i]) == tag)
            { const auto nm = users[i].getFileNameWithoutExtension(); sub.addItem (1000 + i, nm, true, nm == proc.presetName); }
        if (sub.getNumItems() > 0) m.addSubMenu (tag, sub);
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetRect.toNearestInt())),
        [this, users, isUser] (int r)
        {
            if (r == 5001) { if (isUser) proc.saveUserPreset (proc.presetName); else saveAs(); }
            else if (r == 5002) saveAs();
            else if (r == 5003) { browseOpen = true; addOpen = false; }
            else if (r >= 1000) proc.loadUserPreset (users[r - 1000]);
            else if (r > 0) proc.loadFactoryPreset (r - 1);
            repaint();
        });
}

void Canvas::saveAs()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name this preset", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.presetName);
    w->addComboBox ("tag", presetTagNames(), "Tag");   // V9: a tag, for the browser
    if (auto* cb = w->getComboBoxComponent ("tag")) cb->setSelectedItemIndex (presetTagNames().indexOf ("Mine"));
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        auto* cb = w->getComboBoxComponent ("tag");
        if (r == 1) proc.saveUserPreset (w->getTextEditorContents ("name"), cb != nullptr ? cb->getText() : juce::String ("Mine"));
        repaint();
    }), true);
}

void Canvas::click (const Zone& z, const juce::MouseEvent& e)
{
    auto toggle = [this] (const juce::String& pid) { proc.setParam (pid, proc.param (pid) > 0.5f ? 0.0f : 1.0f); };
    auto cycle = [this] (const juce::String& pid, int n) { proc.setParam (pid, (float) (((int) proc.param (pid) + 1) % n)); };
    auto& st = proc.apvts.state;
    switch (z.kind)
    {
        case Hit::Undo: proc.undo(); break;
        case Hit::Redo: proc.redo(); break;
        case Hit::Preset: showPresetMenu(); break;
        case Hit::Save:
        {
            bool isUser = false;
            for (auto& f : proc.userPresets()) if (f.getFileNameWithoutExtension() == proc.presetName) isUser = true;
            if (isUser) proc.saveUserPreset (proc.presetName); else saveAs();
            break;
        }
        case Hit::SaveAs: saveAs(); break;
        case Hit::AB: proc.switchAB (1 - proc.abSlot); break;
        case Hit::Match: toggle ("match"); break;
        case Hit::Bypass: toggle ("bypass"); break;
        case Hit::Tab: tab = z.a; break;
        case Hit::TabPower: toggle (z.a == 0 ? "resoOn" : z.a == 1 ? "tahorOn" : "charOn"); break;
        case Hit::Phase: cycle ("phase", 3); break;
        case Hit::Os: cycle ("os", 6); break;
        case Hit::OsRender: cycle ("osRender", 6); break;
        case Hit::Analyzer: cycle ("analyzer", 4); break;
        case Hit::Range: cycle ("range", 4); break;
        case Hit::Auto: proc.autoLearning.store (true); break;
        case Hit::ScExt: toggle ("scExt"); break;
        case Hit::ResoMode: proc.setParam ("rsMode", (float) z.a); break;
        case Hit::LowLat: toggle ("rsLowLat"); break;
        case Hit::ResoMs: proc.setParam ("rsMs", (float) z.a); break;
        case Hit::Delta: toggle ("rsDelta"); break;
        case Hit::ResoSc: toggle ("rsKey"); break;
        case Hit::Adv: case Hit::AdvClose: advOpen = ! advOpen; st.setProperty ("uiAdv", advOpen, nullptr); break;
        case Hit::Strip: stripOpen = ! stripOpen; st.setProperty ("uiStrip", stripOpen, nullptr); break;
        case Hit::ResoPhase: proc.setParam ("rsPhase", (float) z.a); break;
        case Hit::ResoNode: selNode = z.a; st.setProperty ("uiNode", selNode, nullptr); break;
        case Hit::ResoShape:
        {
            if (z.a >= 0) { proc.setParam (nid (selNode, "Shape"), (float) z.a); break; }
            // V6: the shape dropdown
            juce::PopupMenu m;
            const char* names[8] { "Bell", "Bandpass", "Low shelf", "High shelf", "Tilt", "Notch", "Low cut", "High cut" };
            const int cur = (int) proc.param (nid (selNode, "Shape"));
            for (int i = 0; i < 8; ++i) m.addItem (1 + i, names[i], true, i == cur);
            const int n = selNode;
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r.toNearestInt())),
                             [this, n] (int r) { if (r > 0) proc.setParam (nid (n, "Shape"), (float) (r - 1)); repaint(); });
            break;
        }
        case Hit::TahorNode: case Hit::TahorBand: selBand = z.a; st.setProperty ("uiBand", selBand, nullptr); break;
        case Hit::TahorType:
        {
            juce::PopupMenu m;
            const int type = (int) proc.param (bid (selBand, "Type"));
            const char* names[6] { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" };
            for (int i = 0; i < 6; ++i) m.addItem (1 + i, names[i], true, i == type);
            if (type == 3 || type == 4)
            {
                m.addSeparator();
                const char* sl[6] { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
                for (int i = 0; i < 6; ++i) m.addItem (100 + i, sl[i], true, i == (int) proc.param (bid (selBand, "Slope")));
            }
            m.addSeparator();
            m.addItem (200, "Remove band");
            const int b = selBand;
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r.toNearestInt())), [this, b] (int r)
            {
                if (r >= 1 && r <= 6) proc.setParam (bid (b, "Type"), (float) (r - 1));
                if (r >= 100 && r < 106) proc.setParam (bid (b, "Slope"), (float) (r - 100));
                if (r == 200) proc.setParam (bid (b, "On"), 0.0f);
                repaint();
            });
            break;
        }
        case Hit::TahorPlace: case Hit::ResoPlace: case Hit::CharSide:
        {
            if (z.a >= 0)
            {
                if (z.kind == Hit::TahorPlace) splitTahor (selBand, z.a);
                else if (z.kind == Hit::ResoPlace) splitReso (selNode, z.a);
                else setCharSide (z.a);
                break;
            }
            // V12: placement is a dropdown (Pro-Q style)
            juce::PopupMenu m;
            const Hit k = z.kind;
            if (k == Hit::TahorPlace)
            {
                const char* pn[5] { "Stereo", "Mid", "Side", "Left", "Right" };
                const int cur = (int) proc.param (bid (selBand, "Place"));
                for (int i = 0; i < 5; ++i) m.addItem (1 + i, pn[i], true, i == cur);
            }
            else
            {
                const char* pn[3] { "Stereo", "Left", "Right" };
                const int cur = k == Hit::ResoPlace ? (int) proc.param (nid (selNode, "Place")) : (proc.unitSplit (preview) ? 1 + charSide : 0);
                for (int i = 0; i < 3; ++i) m.addItem (1 + i, pn[i], true, i == cur);
            }
            juce::Component::SafePointer<Canvas> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r.toNearestInt())), [safe, k] (int r)
            {
                if (safe == nullptr || r <= 0) return;
                if (k == Hit::TahorPlace) safe->splitTahor (safe->selBand, r - 1);
                else if (k == Hit::ResoPlace) safe->splitReso (safe->selNode, r - 1);
                else safe->setCharSide (r - 1);
                safe->repaint();
            });
            break;
        }
        case Hit::AutoGain: toggle ("autoGain"); break;
        case Hit::History: showHistoryMenu (z.r); break;
        case Hit::Browse: browseOpen = ! browseOpen; addOpen = false; break;
        case Hit::BrowseClose: browseOpen = false; break;
        case Hit::BrowseTag: browseTag = z.a == 0 ? juce::String ("All") : juce::String (presetTagNames()[z.a - 1]); break;
        case Hit::BrowsePick:
            if (z.a >= 1000) { const auto users = proc.userPresets(); if (z.a - 1000 < users.size()) proc.loadUserPreset (users[z.a - 1000]); }
            else proc.loadFactoryPreset (z.a);
            browseOpen = false;
            break;
        case Hit::BandPower:   // V12: a band off / on, its settings kept
            if (z.a == 0) toggle (bid (z.b, "Byp"));
            else if (z.a == 2) toggle (nid (z.b, "Byp"));
            else proc.charOff[(size_t) preview].fetch_xor (1 << z.b);
            break;
        case Hit::DynPower: toggle (bid (z.a, "Dyn")); proc.setParam (bid (z.a, "Spec"), 0.0f); break;
        case Hit::Piano: pianoOn = ! pianoOn; st.setProperty ("uiPiano", pianoOn, nullptr); break;   // V13
        case Hit::EqMatch:
            if (eqMatch == 0) { eqMatch = 1; eqCount = 0; accPre.assign (4097, 0.0f); accKey.assign (4097, 0.0f); }
            else { applyEqMatch(); eqMatch = 0; }
            break;
        case Hit::TahorMode:   // V9: EQ / Dynamic / Spectral
            proc.setParam (bid (selBand, "Dyn"), z.a == 1 ? 1.0f : 0.0f);
            proc.setParam (bid (selBand, "Spec"), z.a == 2 ? 1.0f : 0.0f);
            break;
        case Hit::TahorChip:
        {
            static const char* w[4] { "Dyn", "Spec", "Sc", "Solo" };
            if (z.a != 9) { toggle (bid (selBand, w[juce::jlimit (0, 3, z.a)])); break; }
            // V12: the panel's view (EQ or Dynamic controls) is separate from Dynamic being on - it stays on in the EQ view
            juce::PopupMenu m;
            const bool dy = proc.param (bid (selBand, "Dyn")) > 0.5f || proc.param (bid (selBand, "Spec")) > 0.5f;
            m.addSectionHeader ("Show");
            m.addItem (1, "EQ controls", true, ! dynView[(size_t) selBand]);
            m.addItem (2, "Dynamic controls", true, dynView[(size_t) selBand]);
            m.addSeparator();
            m.addItem (3, "Dynamic on", true, dy);
            m.addItem (4, "Sidechain (the detector hears the key input)", true, proc.param (bid (selBand, "Sc")) > 0.5f);
            const int b = selBand;
            juce::Component::SafePointer<Canvas> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r.toNearestInt())), [safe, b] (int r)
            {
                if (safe == nullptr || r <= 0) return;
                auto& p = safe->proc;
                if (r == 1) safe->dynView[(size_t) b] = false;
                if (r == 2) { safe->dynView[(size_t) b] = true; if (p.param (bid (b, "Dyn")) < 0.5f) p.setParam (bid (b, "Dyn"), 1.0f); }
                if (r == 3) { p.setParam (bid (b, "Dyn"), p.param (bid (b, "Dyn")) > 0.5f || p.param (bid (b, "Spec")) > 0.5f ? 0.0f : 1.0f); p.setParam (bid (b, "Spec"), 0.0f); }
                if (r == 4) p.setParam (bid (b, "Sc"), p.param (bid (b, "Sc")) > 0.5f ? 0.0f : 1.0f);
                safe->repaint();
            });
            break;
        }
        case Hit::AddBand: addTahorBand ({ display.getX() + fx (1000.0f), display.getY() + 335.0f }); break;
        case Hit::UnitPrev: preview = (preview + numUnits - 1) % numUnits; selCharNode = -1; break;
        case Hit::UnitNext: preview = (preview + 1) % numUnits; selCharNode = -1; break;
        case Hit::UnitAdd:
        {
            auto c = proc.chain();
            if ((int) c.size() < maxChain && std::find (c.begin(), c.end(), preview) == c.end()) { c.push_back (preview); proc.setChain (c); }
            break;
        }
        case Hit::AddEq: addOpen = ! addOpen; break;   // V8: the gear gallery
        case Hit::AddClose: addOpen = false; break;
        case Hit::AddPick:
        {
            auto c = proc.chain();
            if ((int) c.size() < maxChain && std::find (c.begin(), c.end(), z.a) == c.end()) { c.push_back (z.a); proc.setChain (c); }
            preview = z.a; selCharNode = -1; addOpen = false;
            break;
        }
        case Hit::BandSolo:
            if (z.a == 0)
            {
                const bool was = proc.param (bid (selBand, "Solo")) > 0.5f;
                for (int b = 0; b < numBands; ++b) if (b != selBand && proc.param (bid (b, "Solo")) > 0.5f) proc.setParam (bid (b, "Solo"), 0.0f);
                proc.setParam (bid (selBand, "Solo"), was ? 0.0f : 1.0f);
            }
            else if (z.a == 2) resoSolo = resoSolo == selNode ? -1 : selNode;   // V9: Reso's solo
            else
            {
                if (charSolo == z.b && charSoloUnit == preview) { charSolo = -1; charSoloUnit = -1; proc.charSoloHz.store (0.0f); }
                else { charSolo = z.b; charSoloUnit = preview; }
            }
            break;
        case Hit::PresetStep:   // V8: step through the presets (factory, then yours)
        {
            const auto& f = factoryPresets();
            const auto users = proc.userPresets();
            const int total = (int) f.size() + users.size();
            int cur = -1;
            for (int i = 0; i < (int) f.size(); ++i) if (juce::String::fromUTF8 (f[(size_t) i].name) == proc.presetName) cur = i;
            for (int i = 0; i < users.size(); ++i) if (users[i].getFileNameWithoutExtension() == proc.presetName) cur = (int) f.size() + i;
            const int nx = cur < 0 ? (z.a > 0 ? 0 : total - 1) : (cur + z.a + total) % total;
            if (nx < (int) f.size()) proc.loadFactoryPreset (nx); else proc.loadUserPreset (users[nx - (int) f.size()]);
            break;
        }
        case Hit::PageDot: preview = z.a; break;
        case Hit::UnitSwitch: toggle (uid (5, 8 + z.a)); break;
        case Hit::ChainChip: { auto c = proc.chain(); if (z.a < (int) c.size()) { preview = c[(size_t) z.a]; selCharNode = -1; } break; }
        case Hit::ChainRemove: { auto c = proc.chain(); if (z.a < (int) c.size()) { c.erase (c.begin() + z.a); proc.setChain (c); } break; }
        case Hit::CharMode: proc.setParam ("charMode", (float) z.a); break;
        case Hit::PeakDot: preview = z.a; selCharNode = -1; break;
        case Hit::CharNode: selCharNode = z.a; break;
        case Hit::FullScreen: if (onFullScreen) onFullScreen(); break;
        case Hit::TabCycle: tab = (tab + z.a) % 3; break;
        case Hit::Size:
        {
            juce::PopupMenu m;
            static const float sc[5] { 0.7f, 0.85f, 1.0f, 1.2f, 1.4f };
            static const char* nm[5] { "Mini  (876 x 560)", "Small  (1064 x 680)", "Medium  (1252 x 800)", "Large  (1502 x 960)", "Extra Large  (1753 x 1120)" };
            for (int i = 0; i < 5; ++i) m.addItem (1 + i, nm[i], true, ! fullScreen && std::abs (currentScale - sc[i]) < 0.01f);
            m.addSeparator();
            m.addItem (10, "Full screen", true, fullScreen);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r.toNearestInt())), [this] (int r)
            {
                if (r >= 1 && r <= 5) { if (fullScreen) fullScreen = false; if (onSize) onSize (sc[r - 1]); }
                if (r == 10 && onFullScreen) onFullScreen();
                repaint();
            });
            break;
        }
        default: break;
    }
    juce::ignoreUnused (e);
}

//==============================================================================
TahorEditor::TahorEditor (TahorProcessor& p) : AudioProcessorEditor (p), proc (p), canvas (p)
{
    addAndMakeVisible (canvas);
    canvas.onResize = [this] (float s) { setSize (juce::roundToInt (ui::W * s), juce::roundToInt (ui::H * s)); globalScale (s); };
    // V2: full screen - as big as the screen the window is on allows (and back again)
    canvas.onFullScreen = [this]
    {
        if (! canvas.fullScreen)
        {
            scaleBeforeFull = (float) getWidth() / ui::W;
            float s = 0.0f;
           #if JUCE_MAC
            // V3: measure the host window's own bars and fit inside the screen's visible area, then pin the top
            if (auto* peer = getPeer()) s = tahorMacFullScale (peer->getNativeHandle(), ui::W, ui::H, (float) getWidth(), (float) getHeight());
           #endif
            if (s <= 0.0f)
            {
                auto area = getScreenBounds();
                if (auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (area)) area = d->userArea;
                s = juce::jmin ((float) area.getWidth() / ui::W, (float) (area.getHeight() - 80) / ui::H);
            }
            canvas.fullScreen = true;
            setSize (juce::roundToInt (ui::W * s), juce::roundToInt (ui::H * s));
        }
        else
        {
            canvas.fullScreen = false;
            setSize (juce::roundToInt (ui::W * scaleBeforeFull), juce::roundToInt (ui::H * scaleBeforeFull));
        }
       #if JUCE_MAC
        juce::Component::SafePointer<TahorEditor> safe (this);
        for (int ms : { 60, 250 })   // after the host has resized its window
            juce::Timer::callAfterDelay (ms, [safe] { if (safe != nullptr) if (auto* peer = safe->getPeer()) tahorMacPinTop (peer->getNativeHandle()); });
       #endif
    };
    canvas.onSize = [this] (float s)
    {
        setSize (juce::roundToInt (ui::W * s), juce::roundToInt (ui::H * s));
        globalScale (s);
    };
    limits.setFixedAspectRatio (ui::W / ui::H);
    limits.setSizeLimits (751, 480, 3840, 2454);
    setConstrainer (&limits);
    setResizable (true, false);
    const float s = juce::jlimit (0.6f, 1.6f, (float) proc.apvts.state.getProperty ("uiScale", globalScale (-1.0f)));
    setSize (juce::roundToInt (ui::W * s), juce::roundToInt (ui::H * s));
}

TahorEditor::~TahorEditor() {}

// V3: the size you picked last is remembered for every new Tahor (like Pro-Q); pass < 0 to read it
float TahorEditor::globalScale (float set)
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Tahor"; o.filenameSuffix = "settings"; o.folderName = "Mishkal Audio";
    o.osxLibrarySubFolder = "Application Support";
    juce::PropertiesFile f (o);
    if (set > 0.0f) { f.setValue ("scale", set); f.saveIfNeeded(); return set; }
    return (float) f.getDoubleValue ("scale", 0.85);   // V12: opens at Small
}

void TahorEditor::resized()
{
    const float s = (float) getWidth() / ui::W;
    canvas.currentScale = s;
    canvas.setBounds (0, 0, (int) ui::W, (int) ui::H);
    canvas.setTransform (juce::AffineTransform::scale (s));
    proc.apvts.state.setProperty ("uiScale", s, nullptr);
}
