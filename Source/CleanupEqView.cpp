// Clean-up EQ graph and band panel (Pro-Q style interaction)
#include "PluginEditor.h"

using namespace vox;
using namespace vox::ui;

juce::Colour vox::ui::bandColour (int band)
{
    return pal::bands[(size_t) juce::jlimit (0, numCleanBands - 1, band)];
}

static void drawSubPanel (juce::Graphics& g, juce::Rectangle<float> r)
{
    if (isGoliath()) return;   // the controls sit straight on the module faceplate
    if (isRack())   // dark glass inset, like a display window in the rack unit
    {
        g.setGradientFill (juce::ColourGradient (pal::screen2, 0.0f, r.getY(), pal::screen, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.6f), 0.0f, r.getY(),
                                                 juce::Colours::transparentBlack, 0.0f, r.getY() + 10.0f, false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawHorizontalLine ((int) r.getBottom(), r.getX() + 4.0f, r.getRight() - 4.0f);
        return;
    }
    g.setGradientFill (juce::ColourGradient (pal::panelHi.brighter (isHardware() ? 0.0f : 0.05f), 0.0f, r.getY(),
                                             pal::panelHi.darker (0.15f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, isHardware() ? 3.0f : 8.0f);
    g.setColour (isRoyal() ? pal::io.withAlpha (0.25f) : (isHardware() ? juce::Colours::black.withAlpha (0.4f) : pal::line.withAlpha (0.8f)));
    g.drawRoundedRectangle (r.reduced (0.5f), isHardware() ? 3.0f : 8.0f, 1.0f);
}

//==============================================================================
CleanupGraph::CleanupGraph (VoxStripProcessor& p) : proc (p)
{
    preWindow.assign ((size_t) fftSize, 0.0f);
    postWindow.assign ((size_t) fftSize, 0.0f);
    preSmooth.assign ((size_t) fftSize / 2, -120.0f);
    postSmooth.assign ((size_t) fftSize / 2, -120.0f);
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    startTimerHz (30);
}

float CleanupGraph::xForFreq (double hz) const
{
    return (float) (std::log (hz / 20.0) / std::log (1000.0)) * (float) getWidth();
}

double CleanupGraph::freqForX (float x) const
{
    return juce::jlimit (20.0, 20000.0, 20.0 * std::pow (1000.0, (double) x / juce::jmax (1, getWidth())));
}

float CleanupGraph::yForDb (float db) const  { return juce::jmap (db, rangeDb, -rangeDb, 0.0f, (float) getHeight()); }
float CleanupGraph::dbForY (float y) const   { return juce::jmap (y, 0.0f, (float) getHeight(), rangeDb, -rangeDb); }

juce::Point<float> CleanupGraph::nodePosition (const CleanBand& b) const
{
    return { xForFreq (b.freq), yForDb (b.hasGain() ? juce::jlimit (-rangeDb, rangeDb, b.gain) : 0.0f) };
}

int CleanupGraph::hitTest (juce::Point<float> pos) const
{
    const auto s = proc.readSettings();
    int best = -1;
    float bestDist = 11.0f;
    for (int i = 0; i < numCleanBands; ++i)
    {
        if (! s.clean[(size_t) i].on) continue;
        const float d = nodePosition (s.clean[(size_t) i]).getDistanceFrom (pos);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

juce::RangedAudioParameter& CleanupGraph::param (int band, const char* field) const
{
    return *proc.apvts.getParameter (bandId (band, field));
}

void CleanupGraph::setParam (int band, const char* field, float value)
{
    auto& p = param (band, field);
    p.setValueNotifyingHost (p.convertTo0to1 (value));
}

void CleanupGraph::select (int band)
{
    selected = band;
    if (onSelect) onSelect (band);
    repaint();
}

//==============================================================================
int CleanupGraph::resoEdgeAt (float x) const
{   // (V3 draws round handles on the same edges, so the same hit test works)
    const float xl = xForFreq (proc.apvts.getRawParameterValue (id::resoLow)->load());
    const float xh = xForFreq (proc.apvts.getRawParameterValue (id::resoHigh)->load());
    if (std::abs (x - xl) < 10.0f) return 0;
    if (std::abs (x - xh) < 10.0f) return 1;
    return -1;
}

void CleanupGraph::mouseDown (const juce::MouseEvent& e)
{
    if (resoView)   // drag the edges of the focus range
    {
        resoDrag = resoEdgeAt (e.position.x);
        if (resoDrag >= 0) proc.apvts.getParameter (resoDrag == 0 ? id::resoLow : id::resoHigh)->beginChangeGesture();
        return;
    }
    const int hit = hitTest (e.position);

    if (e.mods.isPopupMenu())
    {
        if (hit >= 0) { select (hit); showBandMenu (hit); }
        return;
    }

    if (hit < 0) return;

    select (hit);
    if (e.mods.isAltDown())   // Option-click: solo this band while held
    {
        auto& sb = proc.getChain().soloBand;
        soloBefore = sb.load();
        sb.store (hit);
        tempSolo = true;
    }
    dragging = hit;
    param (hit, "Freq").beginChangeGesture();
    param (hit, "Gain").beginChangeGesture();
}

void CleanupGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (resoView)
    {
        if (resoDrag < 0) return;
        const float lo = proc.apvts.getRawParameterValue (id::resoLow)->load();
        const float hi = proc.apvts.getRawParameterValue (id::resoHigh)->load();
        float f = (float) freqForX (e.position.x);
        f = resoDrag == 0 ? juce::jmin (f, hi / 1.5f) : juce::jmax (f, lo * 1.5f);   // keep at least half an octave between them
        auto* p = proc.apvts.getParameter (resoDrag == 0 ? id::resoLow : id::resoHigh);
        p->setValueNotifyingHost (p->convertTo0to1 (f));
        return;
    }
    if (dragging < 0) return;

    const auto b = proc.readSettings().clean[(size_t) dragging];
    setParam (dragging, "Freq", (float) freqForX (e.position.x));
    if (b.hasGain())
        setParam (dragging, "Gain", juce::jlimit (-rangeDb, rangeDb, dbForY (e.position.y)));
}

void CleanupGraph::mouseUp (const juce::MouseEvent&)
{
    if (resoDrag >= 0)
    {
        proc.apvts.getParameter (resoDrag == 0 ? id::resoLow : id::resoHigh)->endChangeGesture();
        resoDrag = -1;
        return;
    }
    if (tempSolo)
    {
        proc.getChain().soloBand.store (soloBefore);
        tempSolo = false;
    }
    if (dragging < 0) return;
    param (dragging, "Freq").endChangeGesture();
    param (dragging, "Gain").endChangeGesture();
    dragging = -1;
}

void CleanupGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (resoView)   // double-click an edge: back to its default
    {
        if (const int edge = resoEdgeAt (e.position.x); edge >= 0)
        {
            auto* p = proc.apvts.getParameter (edge == 0 ? id::resoLow : id::resoHigh);
            p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture();
        }
        return;
    }
    const int hit = hitTest (e.position);

    if (hit >= 0)   // double-click a node: remove the band
    {
        setParam (hit, "On", 0.0f);
        select (-1);
        return;
    }

    // Double-click empty space: add a band there (cuts near the edges, like Pro-Q)
    const auto s = proc.readSettings();
    int freeBand = -1;
    for (int i = 0; i < numCleanBands && freeBand < 0; ++i)
        if (! s.clean[(size_t) i].on) freeBand = i;
    if (freeBand < 0) return;

    const double f = freqForX (e.position.x);
    const BandType t = f <= 40.0 ? BandType::LowCut : f >= 14000.0 ? BandType::HighCut : BandType::Bell;

    setParam (freeBand, "Type", (float) (int) t);
    setParam (freeBand, "Freq", (float) f);
    setParam (freeBand, "Gain", t == BandType::Bell ? juce::jlimit (-rangeDb, rangeDb, dbForY (e.position.y)) : 0.0f);
    setParam (freeBand, "Q", t == BandType::Bell ? 1.0f : 0.71f);
    setParam (freeBand, "Dyn", 0.0f);
    setParam (freeBand, "On", 1.0f);
    select (freeBand);
}

void CleanupGraph::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (resoView)   // scroll anywhere on the screen: Depth
    {
        auto* p = proc.apvts.getParameter (id::resoDepth);
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + w.deltaY * 0.1f));
        p->endChangeGesture();
        return;
    }
    const int band = hover >= 0 ? hover : selected;
    if (band < 0) return;

    const float q = proc.readSettings().clean[(size_t) band].q;
    setParam (band, "Q", juce::jlimit (0.1f, 18.0f, q * std::exp (w.deltaY * 1.5f)));
}

void CleanupGraph::mouseMove (const juce::MouseEvent& e)
{
    if (resoView)
    {
        setMouseCursor (resoEdgeAt (e.position.x) >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        return;
    }
    const int h = hitTest (e.position);
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::CrosshairCursor);
        repaint();
    }
}

void CleanupGraph::showBandMenu (int band)
{
    const auto b = proc.readSettings().clean[(size_t) band];
    juce::PopupMenu m;
    m.addSectionHeader ("Band " + juce::String (band + 1));
    for (int i = 0; i < bandTypeNames.size(); ++i)
        m.addItem (1 + i, bandTypeNames[i], true, (int) b.type == i);

    if (b.isCut())
    {
        m.addSeparator();
        for (int i = 0; i < slopeNames.size(); ++i)
            m.addItem (11 + i, slopeNames[i], true, b.slope == i);
    }
    if (b.hasGain())
    {
        m.addSeparator();
        m.addItem (20, "Dynamic", true, b.dyn);
    }
    if (MISHKAL_V60 && isGoliath() && ! proc.isMonoTrack())
    {
        m.addSeparator();
        const char* pl[] { "Stereo", "Mid only", "Side only" };
        for (int i = 0; i < 3; ++i) m.addItem (40 + i, pl[i], true, b.place == i);
    }
    m.addSeparator();
    m.addItem (30, "Delete band");

    juce::Component::SafePointer<CleanupGraph> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
        [safe, band, b] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r >= 1 && r <= 6)       safe->setParam (band, "Type", (float) (r - 1));
            else if (r >= 11 && r <= 14) safe->setParam (band, "Slope", (float) (r - 11));
            else if (r == 20)            safe->setParam (band, "Dyn", b.dyn ? 0.0f : 1.0f);
            else if (r == 30)            { safe->setParam (band, "On", 0.0f); safe->select (-1); }
            else if (r >= 40 && r <= 42) safe->setParam (band, "Place", (float) (r - 40));
        });
}

//==============================================================================
void CleanupGraph::timerCallback()
{
    updateSpectrum (proc.getChain().preTap,  preWindow,  preSmooth);
    updateSpectrum (proc.getChain().postTap, postWindow, postSmooth);
    if (MISHKAL_V44)
    {
        double sum = 0.0;
        for (auto v : postWindow) sum += (double) v * v;
        const float rmsDb = (float) (10.0 * std::log10 (sum / (double) (postWindow.empty() ? 1 : postWindow.size()) + 1.0e-12));
        const float target = juce::jlimit (0.0f, 1.0f, (rmsDb + 54.0f) / 42.0f);
        liveLevel += (target > liveLevel ? 0.45f : 0.08f) * (target - liveLevel);
        livePulse = std::fmod (livePulse + 0.035f, 1.0f);
    }
    repaint();
}

void CleanupGraph::updateSpectrum (AnalyzerTap& tap, std::vector<float>& win, std::vector<float>& smoothed)
{
    tap.pull (win);
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (win.begin(), win.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double sr = proc.getCurrentSampleRate();
    for (size_t i = 1; i < smoothed.size(); ++i)
    {
        const double hz = (double) i * sr / fftSize;
        // 4 / N normalises a full-scale sine (Hann window) to 0 dB; +3 dB/oct tilt
        // makes a voice look roughly flat, like most pro analyzers
        float db = juce::Decibels::gainToDecibels (fftData[i] * 4.0f / (float) fftSize, -120.0f)
                 + 3.0f * (float) std::log2 (juce::jmax (1.0, hz) / 1000.0);
        smoothed[i] = juce::jmax (db, smoothed[i] - 1.6f);
    }
}

juce::Path CleanupGraph::spectrumPath (const std::vector<float>& sm, bool closed) const
{
    const double sr = proc.getCurrentSampleRate();
    const float h = (float) getHeight();
    auto yFor = [h] (float db) { return juce::jmap (juce::jlimit (-96.0f, 6.0f, db), -96.0f, 6.0f, h, 0.0f); };

    juce::Path p;
    const int w = getWidth();
    for (int x = 0; x <= w; x += 2)
    {
        const double b1 = freqForX ((float) x) * fftSize / sr;
        const double b2 = freqForX ((float) x + 2.0f) * fftSize / sr;
        float db;
        if (b2 - b1 < 1.0)
        {
            const int i = juce::jlimit (1, (int) sm.size() - 2, (int) b1);
            const float frac = (float) (b1 - i);
            db = sm[(size_t) i] + (sm[(size_t) i + 1] - sm[(size_t) i]) * frac;
        }
        else
        {
            db = -120.0f;
            for (int i = (int) b1; i <= (int) b2 && i < (int) sm.size(); ++i)
                db = juce::jmax (db, sm[(size_t) juce::jmax (1, i)]);
        }

        if (x == 0) p.startNewSubPath ((float) x, yFor (db));
        else        p.lineTo ((float) x, yFor (db));
    }

    if (closed)
    {
        p.lineTo ((float) w, h);
        p.lineTo (0.0f, h);
        p.closeSubPath();
    }
    return p;
}

void CleanupGraph::paint (juce::Graphics& g)
{
    const ScreenColours screenColours;
    const auto bounds = getLocalBounds().toFloat();
    const auto s = proc.readSettings();
    const double sr = proc.getCurrentSampleRate();

    drawScreenBackground (g, bounds);
    g.saveState();
    g.reduceClipRegion (getLocalBounds().reduced (1));

    // Analyzer: input as a faint line, output as a fill
    if (MISHKAL_V46 && isGoliath() && ! resoView)   // coloured by level like the Limiter wave: cyan low, violet, hot pink at the top
    {
        const juce::Colour cyan (0xff3fd6ff), violet (0xffa070ff), pink (0xffff4fc8);
        const float lift = 0.75f + 0.35f * liveLevel;
        const auto sp = spectrumPath (postSmooth, true);
        const float top = bounds.getY(), bot = bounds.getBottom();
        auto heat = [&] (float a)
        {
            juce::ColourGradient cg (pink.withAlpha (a), 0.0f, top + bounds.getHeight() * 0.15f, cyan.withAlpha (a * 0.6f), 0.0f, bot, false);
            cg.addColour (0.45, violet.withAlpha (a * 0.85f));
            return cg;
        };
        g.setGradientFill (heat (0.42f * lift));
        g.fillPath (sp);
        // fade the fill out toward the floor so the grid still reads
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.0f, top + bounds.getHeight() * 0.55f,
                                                 juce::Colour (0xff0b0b0c).withAlpha (0.55f), 0.0f, bot, false));
        g.fillPath (sp);
        using PST = juce::PathStrokeType;
        g.setGradientFill (heat (0.16f * lift)); g.strokePath (sp, PST (6.0f, PST::curved));
        g.setGradientFill (heat (0.32f * lift)); g.strokePath (sp, PST (3.0f, PST::curved));
        g.setGradientFill (heat (0.95f));        g.strokePath (sp, PST (1.1f, PST::curved));
    }
    else if (MISHKAL_V44 && isGoliath() && ! resoView)   // same grey, but lit from the top edge and breathing with the signal
    {
        const auto sp = spectrumPath (postSmooth, true);
        const auto spB = sp.getBounds();
        g.setGradientFill (juce::ColourGradient (pal::clean.withAlpha (0.26f + 0.12f * liveLevel), 0.0f, spB.getY(),
                                                 pal::clean.withAlpha (0.03f), 0.0f, bounds.getBottom(), false));
        g.fillPath (sp);
        g.setColour (juce::Colours::white.withAlpha (0.05f + 0.07f * liveLevel));
        g.strokePath (sp, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved));
        g.setColour (juce::Colours::white.withAlpha (0.16f + 0.16f * liveLevel));
        g.strokePath (sp, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }
    else
    {
    g.setColour (pal::clean.withAlpha (0.16f));
    g.fillPath (spectrumPath (postSmooth, true));
    }
    g.setColour (pal::dim.withAlpha (0.35f));
    g.strokePath (spectrumPath (preSmooth, false), juce::PathStrokeType (1.0f));

    // Grid
    g.setFont (font (9.5f));
    for (double hz : { 30.0, 50.0, 100.0, 200.0, 300.0, 500.0, 1000.0, 2000.0, 3000.0, 5000.0, 10000.0 })
    {
        const float x = xForFreq (hz);
        g.setColour (pal::line.withAlpha (0.55f));
        g.drawVerticalLine ((int) x, 0.0f, bounds.getBottom());
        g.setColour (pal::dim.withAlpha (0.75f));
        g.drawText (hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (x + 3.0f, bounds.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
    }
    for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        const float y = yForDb (db);
        g.setColour (db == 0.0f ? pal::line.brighter (0.35f) : pal::line.withAlpha (0.55f));
        g.drawHorizontalLine ((int) y, 0.0f, bounds.getRight());
        g.setColour (pal::dim.withAlpha (0.75f));
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (bounds.getRight() - 30.0f, y - 12.0f, 26.0f, 11.0f),
                    juce::Justification::right);
    }

    // Resonance suppressor: live reduction, hanging down from 0 dB
    {
        auto& reso = proc.getChain().reso;
        const int nb = reso.numBins.load();
        const double rsr = reso.getSampleRate();
        if (nb > 2)
        {
            juce::Path p;
            float deepest = 0.0f;
            const float zero = yForDb (0.0f);
            p.startNewSubPath (0.0f, zero);
            for (int x = 0; x <= getWidth(); x += 2)
            {
                const double bin = freqForX ((float) x) * (double) ((nb - 1) * 2) / rsr;
                const int k = juce::jlimit (1, nb - 2, (int) bin);
                const float frac = (float) (bin - k);
                const float a = reso.reductionDb[(size_t) k].load (std::memory_order_relaxed);
                const float b = reso.reductionDb[(size_t) k + 1].load (std::memory_order_relaxed);
                const float db = a + (b - a) * juce::jlimit (0.0f, 1.0f, frac);
                deepest = juce::jmin (deepest, db);
                p.lineTo ((float) x, yForDb (juce::jmax (-rangeDb, db)));
            }
            p.lineTo ((float) getWidth(), zero);
            p.closeSubPath();
            if (deepest < -0.2f)
            {
                const bool minimal = MISHKAL_V3 && resoView;
                const auto col = minimal ? juce::Colour (MISHKAL_ACCENT) : pal::reso;
                g.setColour (col.withAlpha (minimal ? 0.16f : 0.28f));
                g.fillPath (p);
                g.setColour (col.withAlpha (minimal ? 1.0f : 0.85f));
                g.strokePath (p, juce::PathStrokeType (minimal ? 1.6f : 1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }

    if (resoView)
    {
        paintResoView (g, bounds);
        g.restoreState();
        return;
    }

    // Character EQs: each active one drawn as its own curve in its tab-light colour,
    // with a small key in the corner so you can tell at a glance what you used
    const int w = getWidth();
    const float zeroLine = yForDb (0.0f);
    std::array<VocalEQ::Design, (size_t) numCharEqs> charDesigns {};
    const int shownEqs = MISHKAL_V81 ? numCharEqs : 4;   // V81: all eight (the V74 EQs were missing here)
    int activeCount = 0;
    for (int t = 0; t < shownEqs; ++t)
        if (s.eqOn && s.charEq[(size_t) t].on)
        {
            charDesigns[(size_t) t] = VocalEQ::design (s.charEq[(size_t) t], (EqType) t, sr);
            ++activeCount;
        }
    if (activeCount > 0)
    {
        for (int t = 0; t < shownEqs; ++t)
        {
            if (! (s.eqOn && s.charEq[(size_t) t].on)) continue;
            juce::Path p;
            for (int x = 0; x <= w; x += 2)
            {
                const float y = yForDb (juce::jlimit (-rangeDb * 2.0f, rangeDb * 2.0f,
                                                      VocalEQ::magnitudeDb (charDesigns[(size_t) t], freqForX ((float) x), sr)));
                if (x == 0) p.startNewSubPath ((float) x, y); else p.lineTo ((float) x, y);
            }
            const auto col = eqModeColour ((EqType) t);
            auto fill = p;
            fill.lineTo ((float) w, zeroLine); fill.lineTo (0.0f, zeroLine); fill.closeSubPath();
            g.setColour (col.withAlpha (0.10f));
            g.fillPath (fill);

            // neon glow: soft wide halo layers, then a bright core with a hot centre line
            using PST = juce::PathStrokeType;
            const auto glow = col.brighter (0.35f);
            g.setColour (glow.withAlpha (0.05f)); g.strokePath (p, PST (14.0f, PST::curved, PST::rounded));
            g.setColour (glow.withAlpha (0.09f)); g.strokePath (p, PST (8.0f,  PST::curved, PST::rounded));
            g.setColour (glow.withAlpha (0.20f)); g.strokePath (p, PST (4.5f,  PST::curved, PST::rounded));
            g.setColour (glow);                   g.strokePath (p, PST (2.0f,  PST::curved, PST::rounded));
            g.setColour (glow.brighter (0.8f).withAlpha (0.85f)); g.strokePath (p, PST (0.8f, PST::curved, PST::rounded));
        }

        if (activeCount > 1)   // the combined effect of the stacked EQs, dashed
        {
            juce::Path sum;
            for (int x = 0; x <= w; x += 2)
            {
                const double hz = freqForX ((float) x);
                float db = 0.0f;
                for (int t = 0; t < shownEqs; ++t)
                    if (s.charEq[(size_t) t].on) db += VocalEQ::magnitudeDb (charDesigns[(size_t) t], hz, sr);
                const float y = yForDb (juce::jlimit (-rangeDb * 2.0f, rangeDb * 2.0f, db));
                if (x == 0) sum.startNewSubPath ((float) x, y); else sum.lineTo ((float) x, y);
            }
            juce::Path dashed;
            const float dashes[] { 5.0f, 4.0f };
            juce::PathStrokeType (1.2f).createDashedStroke (dashed, sum, dashes, 2);
            g.setColour (pal::text.withAlpha (0.55f));
            g.fillPath (dashed);
        }

        // key: a coloured dot + name per active character EQ, top left
        g.setFont (font (10.5f, true));
        float kx = 10.0f;
        const float ky = 8.0f;
        auto pill = [&] (juce::Colour c, const juce::String& name, bool dashedKey)
        {
            const float tw = juce::GlyphArrangement::getStringWidth (font (10.5f, true), name);
            auto r = juce::Rectangle<float> (kx, ky, tw + 26.0f, 18.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (r, 9.0f);
            if (dashedKey)
            {
                g.setColour (pal::text.withAlpha (0.7f));
                for (int i = 0; i < 2; ++i) g.fillRect (r.getX() + 6.0f + (float) i * 6.0f, r.getCentreY() - 0.75f, 4.0f, 1.5f);
            }
            else
            {
                const auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ r.getX() + 11.0f, r.getCentreY() });
                g.setColour (c.brighter (0.35f).withAlpha (0.35f));
                g.fillEllipse (dot.expanded (3.0f));   // the key's dot glows like its line
                g.setColour (c.brighter (0.35f));
                g.fillEllipse (dot);
            }
            g.setColour (pal::text.withAlpha (0.9f));
            g.drawText (name, r.withTrimmedLeft (20.0f), juce::Justification::centredLeft);
            kx = r.getRight() + 6.0f;
        };
        int nth = 0;
        for (int t : s.eqOrder)   // in processing order, like the tabs (V81: numbered, like the chain lane)
            if (t < shownEqs && s.eqOn && s.charEq[(size_t) t].on)
                pill (eqModeColour ((EqType) t), MISHKAL_V81 && ! MISHKAL_V83 ? juce::String (++nth) + "  " + eqTypeNames[t] : eqTypeNames[t], false);
        if (activeCount > 1)
            pill ({}, "Combined", true);
    }

    // Tahor curves (include the live movement of dynamic bands), drawn exactly as the current mode runs them:
    // Natural = the digital filters at their (oversampled) rate, Linear = the analog shapes
    const bool linearMode = ! s.trackMode && s.cleanPhase > 0;
    const double designSr = sr * (double) (s.trackMode ? 1 : (1 << juce::jlimit (0, 2, s.cleanOs)));
    std::array<CleanupEQ::BandDesign, numCleanBands> designs {};
    std::array<float, numCleanBands> dynNow {};
    bool any = false;
    for (int i = 0; i < numCleanBands; ++i)
    {
        const auto& b = s.clean[(size_t) i];
        if (! b.on) continue;
        any = true;
        dynNow[(size_t) i] = (b.dyn && b.hasGain()) ? proc.getChain().cleanup.dynOffsetDb[(size_t) i].load() : 0.0f;
        designs[(size_t) i] = CleanupEQ::design (b, b.freq, b.gain + dynNow[(size_t) i], b.q, designSr);
    }
    auto bandGain = [&] (int i, double hz) -> double   // linear magnitude of one band
    {
        const auto& b = s.clean[(size_t) i];
        if (linearMode)
            return juce::Decibels::decibelsToGain (tahorAnalogBandDb (b, b.freq, b.gain + dynNow[(size_t) i], b.q, hz), -240.0);
        return CleanupEQ::bandMagnitude (designs[(size_t) i], hz, designSr);
    };

    const float zeroY = yForDb (0.0f);
    for (int i = 0; i < numCleanBands; ++i)
    {
        const bool focus = i == selected || i == hover;
        const bool tint = MISHKAL_V44 && isGoliath() && s.cleanOn
                          && (s.clean[(size_t) i].hasGain() || s.clean[(size_t) i].type == BandType::Notch);   // no wash under the cut filters
        if (! s.clean[(size_t) i].on || (! focus && ! tint)) continue;
        juce::Path p;
        for (int x = 0; x <= w; x += 2)
        {
            const double hz = freqForX ((float) x);
            const float db = (float) juce::Decibels::gainToDecibels (bandGain (i, hz), -120.0);
            const float y = yForDb (juce::jlimit (-rangeDb * 2.0f, rangeDb * 2.0f, db));
            if (x == 0) p.startNewSubPath ((float) x, y); else p.lineTo ((float) x, y);
        }
        auto fill = p;
        fill.lineTo ((float) w, zeroY); fill.lineTo (0.0f, zeroY); fill.closeSubPath();
        if (MISHKAL_V44 && isGoliath() && ! focus)   // V44: every band's own shape glows faintly in its colour
        {
            g.setGradientFill (juce::ColourGradient (bandColour (i).withAlpha (0.10f + 0.10f * liveLevel), 0.0f, zeroY,
                                                     bandColour (i).withAlpha (0.22f + 0.12f * liveLevel), 0.0f, zeroY + (zeroY > 0 ? 60.0f : 0.0f), false));
            g.fillPath (fill);
            g.setColour (bandColour (i).withAlpha (0.45f));
            g.strokePath (p, juce::PathStrokeType (1.0f));
            continue;
        }
        g.setColour (bandColour (i).withAlpha (0.18f));
        g.fillPath (fill);
        g.setColour (bandColour (i).withAlpha (0.7f));
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }

    // V60: bands on Side draw their own (dashed, cyan) curve; the main curve is what the mid / centre gets.
    // On a mono track Side bands do nothing, so they're left out.
    const bool monoTrack = proc.isMonoTrack();
    bool anySide = false, anyMid = false;
    for (const auto& b : s.clean) if (b.on) { anySide = anySide || b.place == 2; anyMid = anyMid || b.place == 1; }
    auto inCurve = [&] (int i, int which)   // which 0 = main (stereo + mid), 1 = side (stereo + side)
    {
        const int pl = s.clean[(size_t) i].place;
        return which == 0 ? pl != 2 : pl != 1;
    };
    if (any && s.cleanOn && MISHKAL_V60 && isGoliath() && anySide && ! monoTrack)
    {
        juce::Path side;
        for (int x = 0; x <= w; x += 2)
        {
            const double hz = freqForX ((float) x);
            double mag = 1.0;
            for (int i = 0; i < numCleanBands; ++i)
                if (s.clean[(size_t) i].on && inCurve (i, 1)) mag *= bandGain (i, hz);
            const float y = yForDb (juce::jlimit (-rangeDb * 2.0f, rangeDb * 2.0f, (float) juce::Decibels::gainToDecibels (mag, -120.0)));
            if (x == 0) side.startNewSubPath ((float) x, y); else side.lineTo ((float) x, y);
        }
        juce::Path dashed;
        const float dash[] { 6.0f, 4.0f };
        juce::PathStrokeType (1.6f).createDashedStroke (dashed, side, dash, 2);
        const auto sc = placeColour (2);
        g.setColour (sc.withAlpha (0.18f)); g.strokePath (side, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (sc.withAlpha (0.95f)); g.fillPath (dashed);
    }
    if (any && s.cleanOn && MISHKAL_V60 && isGoliath() && (anySide || anyMid) && ! monoTrack)   // a small legend
    {
        auto lg = juce::Rectangle<float> ((float) getWidth() - 132.0f, 28.0f, 120.0f, 14.0f);
        g.setFont (screenFont (10.5f, 1));
        g.setColour (juce::Colour (0xffd8d8d8));
        g.fillRect (lg.getX(), lg.getCentreY() - 1.0f, 14.0f, 2.0f);
        g.drawText (anyMid ? "mid" : "stereo", lg.withTrimmedLeft (18.0f).withWidth (40.0f), juce::Justification::centredLeft);
        if (anySide)
        {
            g.setColour (placeColour (2));
            for (float d = 0.0f; d < 14.0f; d += 5.0f) g.fillRect (lg.getX() + 62.0f + d, lg.getCentreY() - 1.0f, 3.0f, 2.0f);
            g.drawText ("side", lg.withTrimmedLeft (80.0f), juce::Justification::centredLeft);
        }
    }

    if (any && s.cleanOn)
    {
        juce::Path total;
        for (int x = 0; x <= w; x += 2)
        {
            const double hz = freqForX ((float) x);
            double mag = 1.0;
            for (int i = 0; i < numCleanBands; ++i)
                if (s.clean[(size_t) i].on && (! (MISHKAL_V60 && isGoliath()) || inCurve (i, 0)))
                    mag *= bandGain (i, hz);
            const float y = yForDb (juce::jlimit (-rangeDb * 2.0f, rangeDb * 2.0f,
                                                  (float) juce::Decibels::gainToDecibels (mag, -120.0)));
            if (x == 0) total.startNewSubPath ((float) x, y); else total.lineTo ((float) x, y);
        }
        if (isGoliath() && MISHKAL_V44)   // the same grey-white curve, now glowing with the signal, with a colour beam under each band
        {
            using PST = juce::PathStrokeType;
            const float glow = 0.7f + 0.6f * liveLevel;
            for (int i = 0; i < numCleanBands; ++i)
            {
                const auto& b = s.clean[(size_t) i];
                if (! b.on || ! b.hasGain()) continue;
                const auto pos = nodePosition (b);
                if (std::abs (pos.y - zeroY) < 2.0f) continue;
                const auto col = bandColour (i);
                const float bw = i == selected ? 24.0f : 16.0f;
                g.setGradientFill (juce::ColourGradient (col.withAlpha (0.34f * glow), pos.x, pos.y, col.withAlpha (0.0f), pos.x, zeroY, false));
                g.fillRect (juce::Rectangle<float> (pos.x - bw * 0.5f, juce::jmin (pos.y, zeroY), bw, std::abs (pos.y - zeroY)));
            }
            auto fill = total;
            fill.lineTo ((float) w, zeroY); fill.lineTo (0.0f, zeroY); fill.closeSubPath();
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), 0.0f, zeroY - 60.0f,
                                                     juce::Colours::white.withAlpha (0.04f), 0.0f, zeroY + 60.0f, false));
            g.fillPath (fill);
            g.setColour (juce::Colours::white.withAlpha (0.07f * glow)); g.strokePath (total, PST (14.0f, PST::curved, PST::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.14f * glow)); g.strokePath (total, PST (7.0f,  PST::curved, PST::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.28f * glow)); g.strokePath (total, PST (3.6f,  PST::curved, PST::rounded));
            g.setColour (juce::Colour (0xffd8d8d8));                     g.strokePath (total, PST (1.8f,  PST::curved, PST::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.6f * glow));  g.strokePath (total, PST (0.7f,  PST::curved, PST::rounded));
        }
        else if (isGoliath())   // grey-white curve with a faint fill, like the display design
        {
            auto fill = total;
            fill.lineTo ((float) w, zeroY); fill.lineTo (0.0f, zeroY); fill.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillPath (fill);
            g.setColour (juce::Colour (0xffd0d0d0));
            g.strokePath (total, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (isRack() || isModern())
            drawGlowStroke (g, total, pal::clean, 2.4f);
        else
        {
            g.setColour (pal::text);
            g.strokePath (total, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // Band solo: highlight the slice of the spectrum you're hearing
    {
        const int sb = proc.getChain().soloBand.load();
        if (sb >= 0 && sb < numCleanBands && s.clean[(size_t) sb].on)
        {
            const auto& b = s.clean[(size_t) sb];
            const bool narrow = b.type == BandType::Bell || b.type == BandType::Notch;
            const float q = narrow ? juce::jlimit (0.5f, 8.0f, b.q) : 0.7f;
            const float octaves = 2.0f * std::asinh (1.0f / (2.0f * q)) / std::log (2.0f);
            const float x1 = xForFreq (b.freq / std::pow (2.0f, octaves * 0.5f));
            const float x2 = xForFreq (b.freq * std::pow (2.0f, octaves * 0.5f));
            const float h = (float) getHeight();
            const auto band = juce::Rectangle<float> (x1, 0.0f, x2 - x1, h);
            // dim everything outside the solo slice
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, x1, h));
            g.fillRect (juce::Rectangle<float> (x2, 0.0f, (float) getWidth() - x2, h));
            juce::ColourGradient cg (pal::eq.withAlpha (0.16f), 0.0f, 0.0f, pal::eq.withAlpha (0.04f), 0.0f, h, false);
            g.setGradientFill (cg);
            g.fillRect (band);
            g.setColour (pal::eq.withAlpha (0.6f));
            g.drawVerticalLine ((int) x1, 0.0f, h);
            g.drawVerticalLine ((int) x2, 0.0f, h);
            auto tag = juce::Rectangle<float> (0.0f, 0.0f, 92.0f, 18.0f).withCentre ({ (x1 + x2) * 0.5f, h - 22.0f });
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillRoundedRectangle (tag, 9.0f);
            g.setColour (pal::eq);
            g.drawRoundedRectangle (tag, 9.0f, 1.0f);
            g.setFont (font (10.5f, true));
            g.drawText ("SOLO  BAND " + juce::String (sb + 1), tag, juce::Justification::centred);
        }
    }

    // Nodes
    for (int i = 0; i < numCleanBands; ++i)
    {
        const auto& b = s.clean[(size_t) i];
        if (! b.on) continue;
        const auto pos = nodePosition (b);
        const auto col = s.cleanOn ? bandColour (i) : pal::dim;

        if (b.dyn && b.hasGain())
        {
            const float dynDb = proc.getChain().cleanup.dynOffsetDb[(size_t) i].load();
            const float y2 = yForDb (juce::jlimit (-rangeDb, rangeDb, b.gain + dynDb));
            const float yr = yForDb (juce::jlimit (-rangeDb, rangeDb, b.gain + b.dynRange));
            g.setColour (col.withAlpha (0.35f));
            g.drawLine (pos.x, pos.y, pos.x, yr, 2.0f);
            g.setColour (col);
            g.drawEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ pos.x, y2 }), 1.5f);
        }

        if (isGoliath())
        {
            // white dots; the selected band is red with a soft ring
            const bool sel = i == selected;
            const float rad = sel ? 6.0f : (i == hover ? 5.5f : 4.5f);
            const auto dotCol = ! s.cleanOn ? pal::dim : sel ? juce::Colour (0xffe2343e) : juce::Colour (0xffe6e6e6);
            if (MISHKAL_V44 && s.cleanOn)   // a glow in the band's colour; the selected ring breathes
            {
                const float glow = 0.7f + 0.6f * liveLevel;
                const float haloR = sel ? 18.0f + 4.0f * std::sin (livePulse * juce::MathConstants<float>::twoPi) : 14.0f;
                const auto hc = sel ? dotCol : bandColour (i);
                g.setGradientFill (juce::ColourGradient (hc.withAlpha ((sel ? 0.5f : 0.35f) * glow), pos.x, pos.y,
                                                         hc.withAlpha (0.0f), pos.x + haloR, pos.y, true));
                g.fillEllipse (juce::Rectangle<float> (haloR * 2.0f, haloR * 2.0f).withCentre (pos));
            }
            if (sel)
            {
                const float ringD = MISHKAL_V44 ? 20.0f + 3.0f * std::sin (livePulse * juce::MathConstants<float>::twoPi) : 20.0f;
                g.setColour (dotCol.withAlpha (0.35f));
                g.drawEllipse (juce::Rectangle<float> (ringD, ringD).withCentre (pos), 2.0f);
            }
            g.setColour (dotCol);
            g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (pos));
            if (MISHKAL_V44 && s.cleanOn && ! sel)   // thin ring in the band's colour around the white dot
            {
                g.setColour (bandColour (i));
                g.drawEllipse (juce::Rectangle<float> (rad * 2.0f + 3.5f, rad * 2.0f + 3.5f).withCentre (pos), 1.8f);
            }
            g.setColour (MISHKAL_V44 && s.cleanOn ? bandColour (i).withAlpha (0.9f) : juce::Colour (0xff8e8e93));
            g.setFont (screenFont (10.5f, 1));
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (16.0f, 12.0f).withCentre ({ pos.x, pos.y - rad - 9.0f }), juce::Justification::centred);
            if (MISHKAL_V60 && b.place != 0 && ! monoTrack)   // V60: a small M / S tag beside the number
            {
                const float side = pos.x > (float) getWidth() - 22.0f ? -12.0f : 12.0f;   // flip inside at the right edge
                const auto tag = juce::Rectangle<float> (13.0f, 12.0f).withCentre ({ pos.x + side, pos.y - rad - 9.0f });
                g.setColour (placeColour (b.place).withAlpha (0.2f));
                g.fillRoundedRectangle (tag, 3.0f);
                g.setColour (placeColour (b.place));
                g.drawRoundedRectangle (tag, 3.0f, 1.0f);
                g.setFont (screenFont (9.0f, 1));
                g.drawText (b.place == 1 ? "M" : "S", tag, juce::Justification::centred);
            }
        }
        else if (isRack())
        {
            // Dark node with a coloured ring and a light number
            const float r = (i == selected || i == hover) ? 10.0f : 8.5f;
            const auto node = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pos);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillEllipse (node.translated (0.0f, 1.5f).expanded (1.0f));
            g.setColour (col.withAlpha (i == selected ? 0.35f : 0.0f));
            g.fillEllipse (node.expanded (4.0f));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3e), pos.x, node.getY(), juce::Colour (0xff141416), pos.x, node.getBottom(), false));
            g.fillEllipse (node);
            g.setColour (col);
            g.drawEllipse (node.reduced (1.2f), 2.4f);
            g.setColour (juce::Colours::white);
            g.setFont (font (11.5f, true));
            g.drawText (juce::String (i + 1), node, juce::Justification::centred);
        }
        else
        {
            const float r = (i == selected || i == hover) ? 8.0f : 6.5f;
            g.setColour (col);
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pos));
            if (i == selected)
            {
                g.setColour (pal::text);
                g.drawEllipse (juce::Rectangle<float> (r * 2.0f + 5.0f, r * 2.0f + 5.0f).withCentre (pos), 1.5f);
            }
            g.setColour (pal::bg);
            g.setFont (font (10.0f, true));
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pos), juce::Justification::centred);
        }
    }

    // Readout next to the node being hovered or dragged
    const int info = dragging >= 0 ? dragging : hover;
    if (info >= 0 && s.clean[(size_t) info].on)
    {
        const auto& b = s.clean[(size_t) info];
        const auto pos = nodePosition (b);
        juce::StringArray lines { bandTypeNames[(int) b.type], param (info, "Freq").getCurrentValueAsText() };
        if (b.hasGain()) lines.add ("Gain: " + param (info, "Gain").getCurrentValueAsText());
        else if (b.isCut()) lines.add (slopeNames[b.slope]);

        auto box = juce::Rectangle<float> (112.0f, 16.0f + 15.0f * (float) lines.size());
        box.setPosition (pos.x + 16.0f, pos.y - box.getHeight() - 10.0f);
        if (box.getRight() > (float) getWidth() - 6.0f) box.setX (pos.x - box.getWidth() - 16.0f);
        if (box.getY() < 6.0f) box.setY (pos.y + 14.0f);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (box.translated (0.0f, 2.0f), 4.0f);
        g.setColour ((isMetal() ? juce::Colour (0xff2b2b2f) : pal::panelHi).withAlpha (0.96f));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (bandColour (info));
        g.fillRoundedRectangle (box.withWidth (3.0f).reduced (0.0f, 4.0f), 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (box, 4.0f, 1.0f);

        auto t = box.reduced (10.0f, 8.0f);
        for (int l = 0; l < lines.size(); ++l)
        {
            g.setColour (l == 0 ? pal::text : pal::dim.brighter (0.3f));
            g.setFont (font (l == 0 ? 13.0f : 12.0f, l == 0));
            g.drawText (lines[l], t.removeFromTop (15.0f), juce::Justification::centred);
        }
    }

    if (! any)
    {
        g.setColour (pal::dim);
        g.setFont (font (12.0f));
        g.drawText ("Double-click to add a band  -  drag to move  -  scroll for Q  -  right-click for type, slope, dynamic",
                    getLocalBounds().reduced (12).removeFromTop (20), juce::Justification::centredLeft);
    }

    g.restoreState();
    g.setColour (pal::line);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

//==============================================================================
BandPanel::BandPanel (juce::AudioProcessorValueTreeState& s) : state (s)
{
    type.addItemList (bandTypeNames, 1);
    slope.addItemList (slopeNames, 1);
    for (auto* c : { &type, &slope })
    {
        c->setColour (juce::ComboBox::backgroundColourId, pal::bg);
        c->setColour (juce::ComboBox::outlineColourId, pal::line);
        c->setColour (juce::ComboBox::textColourId, pal::text);
        c->setColour (juce::ComboBox::arrowColourId, pal::dim);
        addChildComponent (c);
    }
    addChildComponent (on);
    addChildComponent (dyn);
    addChildComponent (solo);
    solo.setTooltip ("Solo: hear only the frequencies around this band, to find exactly where a problem sits. "
                     "Tip: Option-click and hold a band's dot on the graph to solo it while you drag.");
    solo.onClick = [this]
    {
        if (soloTarget != nullptr) soloTarget->store (solo.getToggleState() && band >= 0 ? band : -1);
    };

    type.onChange = [this] { resized(); };
    dyn.onStateChange = [this] { resized(); };
}

bool BandPanel::placeShown() const
{
    if (! (MISHKAL_V60 && isGoliath()) || place == nullptr) return false;
    auto* p = dynamic_cast<VoxStripProcessor*> (&state.processor);
    return p == nullptr || ! p->isMonoTrack();
}

void BandPanel::showBand (int b)
{
    place.reset();
    typeAtt.reset(); slopeAtt.reset(); onAtt.reset(); dynAtt.reset();
    freq.reset(); gain.reset(); q.reset(); thresh.reset(); range.reset();
    band = b;

    for (auto* c : std::initializer_list<juce::Component*> { &type, &slope, &on, &dyn, &solo })
        c->setVisible (b >= 0);
    // while a solo is running, it follows the band you pick
    if (soloTarget != nullptr && soloTarget->load() >= 0)
        soloTarget->store (b);
    syncSolo();

    if (b >= 0)
    {
        const auto col = bandColour (b);
        for (auto* c : { &type, &slope })
        {
            c->setColour (juce::ComboBox::backgroundColourId, isLightPanel() ? pal::panelHi : pal::bg);
            c->setColour (juce::ComboBox::outlineColourId, isRoyal() ? pal::io.withAlpha (0.4f) : pal::line);
            c->setColour (juce::ComboBox::textColourId, pal::text);
            c->setColour (juce::ComboBox::arrowColourId, pal::dim);
        }
        on.setColour (juce::ToggleButton::tickColourId, col);
        solo.setColour (juce::ToggleButton::tickColourId, pal::eq);
        dyn.setColour (juce::ToggleButton::tickColourId, col);
        typeAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, bandId (b, "Type"),  type);
        slopeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, bandId (b, "Slope"), slope);
        onAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>   (state, bandId (b, "On"),    on);
        dynAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>   (state, bandId (b, "Dyn"),   dyn);

        auto make = [&] (std::unique_ptr<Knob>& k, const char* field, const char* title, bool bipolar)
        {
            k = std::make_unique<Knob> (state, bandId (b, field), title, Knob::ColourFn ([b] { return bandColour (b); }), bipolar);
            addAndMakeVisible (*k);
        };
        make (freq,   "Freq",      "Freq",      false);
        make (gain,   "Gain",      "Gain",      true);
        make (q,      "Q",         "Q",         false);
        make (thresh, "DynThresh", "Threshold", false);
        make (range,  "DynRange",  "Range",     true);
        if (MISHKAL_V60 && isGoliath())
            if (auto* pp = state.getParameter (bandId (b, "Place")))
            {
                static const juce::Colour placeAccent (0xffd8d8dc);
                place = std::make_unique<ModeSelector> (*pp, juce::StringArray { "stereo", "mid", "side" }, placeAccent);
                place->pills = true;
                place->colourFor = [] (int i) { return placeColour (i); };
                place->setTooltip ("Where this band works: Stereo = both sides as usual, Mid = only the centre "
                                   "(vocal, kick, bass), Side = only the width (reverb, wide guitars, room). Stereo tracks only.");
                addChildComponent (*place);
            }
    }

    resized();
    repaint();
}

void BandPanel::syncSolo()
{
    const bool s = soloTarget != nullptr && band >= 0 && soloTarget->load() == band;
    if (solo.getToggleState() != s) solo.setToggleState (s, juce::dontSendNotification);
}

void BandPanel::paint (juce::Graphics& g)
{
    if (isGoliath() && MISHKAL_V3)   // minimal: the same flat panel as Reso, lowercase title with the band's colour dot
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff15161a));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (0xff24262b));
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        if (band < 0)
        {
            g.setColour (juce::Colour (0xff9da0a8));
            g.setFont (screenFont (13.0f, 0));
            g.drawFittedText ("select a band on the screen,\nor double-click it to add one", getLocalBounds().reduced (16), juce::Justification::centred, 3);
            return;
        }
        g.setColour (juce::Colour (MISHKAL_ACCENT));
        g.fillEllipse (14.0f, 11.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xffe9e7e2));
        g.setFont (screenFont (16.0f, 0).withExtraKerningFactor (0.06f));
        g.drawText ("band " + juce::String (band + 1), 30, 4, 120, 22, juce::Justification::centredLeft);
        return;
    }
    drawSubPanel (g, getLocalBounds().toFloat());

    if (band < 0)
    {
        g.setColour (pal::dim);
        g.setFont (font (12.0f));
        g.drawFittedText ("Select a band on the graph,\nor double-click it to add one.",
                          getLocalBounds().reduced (16), juce::Justification::centred, 3);
        return;
    }

    g.setColour (bandColour (band));
    g.fillEllipse (14.0f, 14.0f, 10.0f, 10.0f);
    g.setColour (pal::text);
    g.setFont (font (13.0f, true));
    g.drawText ("BAND " + juce::String (band + 1), 30, 8, 120, 22, juce::Justification::centredLeft);
}

void BandPanel::resized()
{
    if (band < 0 || freq == nullptr || range == nullptr) return;   // still being set up

    const auto t = (BandType) type.getSelectedItemIndex();
    const bool hasGain = t == BandType::Bell || t == BandType::LowShelf || t == BandType::HighShelf;
    const bool isCut = t == BandType::LowCut || t == BandType::HighCut;
    const bool dynOn = hasGain && dyn.getToggleState();

    auto r = getLocalBounds().reduced (10, 8);
    auto header = r.removeFromTop (22);
    on.setBounds (header.removeFromRight (54).reduced (0, 1));
    header.removeFromRight (6);
    solo.setBounds (header.removeFromRight (62).reduced (0, 1));

    r.removeFromTop (4);
    auto combos = r.removeFromTop (24);
    type.setBounds (combos.removeFromLeft (isCut ? combos.getWidth() / 2 - 3 : combos.getWidth()));
    combos.removeFromLeft (6);
    slope.setBounds (combos);
    slope.setVisible (isCut);
    if (place != nullptr)   // V60: stereo / mid / side under the type (hidden on mono tracks)
    {
        const bool show = placeShown();
        place->setVisible (show);
        if (show) { r.removeFromTop (6); place->setBounds (r.removeFromTop (22)); }
    }

    if (isGoliath() && MISHKAL_V3)
    {
        // minimal: big freq, gain / q, then dynamic with threshold / range; lowercase labels and value boxes
        for (auto* k : { freq.get(), gain.get(), q.get(), thresh.get(), range.get() })
            k->slider.getProperties().set ("minimal", true);
        freq->setTitleText ("freq");  gain->setTitleText ("gain");  q->setTitleText ("q");
        thresh->setTitleText ("threshold");  range->setTitleText ("range");
        for (auto* c : { &type, &slope })
        {
            c->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111215));
            c->setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff34363c));
            c->setColour (juce::ComboBox::textColourId, juce::Colour (0xffe9e7e2));
            c->setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff9da0a8));
        }
        gain->setVisible (hasGain);
        r.removeFromTop (6);
        const int freqH = juce::jmin (150, r.getHeight() * 2 / 5);
        freq->setBounds (r.removeFromTop (freqH).withSizeKeepingCentre (juce::jmin (r.getWidth(), 140), freqH));
        r.removeFromTop (4);
        const int rowH = juce::jmin (96, (r.getHeight() - 26) / 2);
        auto row = r.removeFromTop (rowH);
        const int w = row.getWidth() / 2;
        if (hasGain) { gain->setBounds (row.removeFromLeft (w)); q->setBounds (row); }
        else q->setBounds (row.withSizeKeepingCentre (w, row.getHeight()));
        r.removeFromTop (2);
        dyn.setVisible (hasGain);
        dyn.setBounds (r.removeFromTop (22).withSizeKeepingCentre (120, 22));
        thresh->setVisible (dynOn);
        range->setVisible (dynOn);
        r.removeFromTop (2);
        auto dynRow = r.removeFromTop (juce::jmin (rowH, r.getHeight()));
        thresh->setBounds (dynRow.removeFromLeft (w));
        range->setBounds (dynRow);
        for (auto* k : { freq.get(), gain.get(), q.get(), thresh.get(), range.get() }) k->resized();
        return;
    }
    if (isGoliath())
    {
        // tall module: big Freq knob, Gain + Q below, then the dynamic controls
        gain->setVisible (hasGain);
        r.removeFromTop (6);
        freq->setBounds (r.removeFromTop (124).withSizeKeepingCentre (juce::jmin (r.getWidth(), 170), 124));
        r.removeFromTop (2);
        auto row = r.removeFromTop (96);
        const int w = row.getWidth() / 2;
        if (hasGain) { gain->setBounds (row.removeFromLeft (w)); q->setBounds (row); }
        else q->setBounds (row.withSizeKeepingCentre (w, row.getHeight()));
        r.removeFromTop (4);
        dyn.setVisible (hasGain);
        dyn.setBounds (r.removeFromTop (22).withSizeKeepingCentre (120, 22));
        thresh->setVisible (dynOn);
        range->setVisible (dynOn);
        r.removeFromTop (4);
        auto dynRow = r.removeFromTop (juce::jmin (96, r.getHeight()));
        thresh->setBounds (dynRow.removeFromLeft (w));
        range->setBounds (dynRow);
        return;
    }

    r.removeFromTop (2);
    auto row = r.removeFromTop (74);
    std::vector<Knob*> top { freq.get() };
    if (hasGain) top.push_back (gain.get());
    top.push_back (q.get());
    gain->setVisible (hasGain);
    const int w = row.getWidth() / 3;
    for (auto* k : top) k->setBounds (row.removeFromLeft (w).reduced (2, 0));

    r.removeFromTop (2);
    dyn.setVisible (hasGain);
    dyn.setBounds (r.removeFromTop (22).withSizeKeepingCentre (110, 22));

    thresh->setVisible (dynOn);
    range->setVisible (dynOn);
    r.removeFromTop (2);
    auto dynRow = r.removeFromTop (72).withSizeKeepingCentre (w * 2, 72);
    thresh->setBounds (dynRow.removeFromLeft (w).reduced (2, 0));
    range->setBounds (dynRow.reduced (2, 0));
}

//==============================================================================
ResoPanel::ResoPanel (juce::AudioProcessorValueTreeState& st)
    : state (st), onAtt (st, id::resoOn, on), deltaAtt (st, id::resoDelta, delta)
{
    lookAndFeelChanged();
    delta.setTooltip ("Hear only what is being removed");
    addAndMakeVisible (on);
    addAndMakeVisible (delta);

    const std::pair<const char*, const char*> list[] {
        { id::resoDepth, "Depth" }, { id::resoSharp, "Sharpness" }, { id::resoSelect, "Selectivity" }, { id::resoMix, "Mix" },
        { id::resoAttack, "Attack" }, { id::resoRelease, "Release" }, { id::resoLow, "Low" }, { id::resoHigh, "High" } };
    for (auto& [pid, title] : list)
    {
        knobs.push_back (std::make_unique<Knob> (state, pid, title, pal::reso));
        addAndMakeVisible (*knobs.back());
    }
    if (MISHKAL_V60)
    {
        static const juce::Colour msAccent (0xffd8d8dc);
        msMode = std::make_unique<ModeSelector> (*state.getParameter (id::resoMs), juce::StringArray { "stereo", "mid", "side", "m + s" }, msAccent);
        msMode->pills = true;
        msMode->colourFor = [] (int i) { return i == 3 ? juce::Colour (0xffb98cff) : placeColour (i); };
        msMode->setTooltip ("Stereo: one detector for both sides (the image never moves). Mid: only the centre is treated. "
                            "Side: only the width. M + S: centre and width each get their own detector, so a loud vocal "
                            "doesn't dull the cymbals; side depth sets how hard the sides are treated.");
        addChildComponent (*msMode);
        sideDepth = std::make_unique<Knob> (state, id::resoSideDepth, "side depth", pal::reso);
        sideDepth->slider.getProperties().set ("minimal", true);
        sideDepth->setTitleText ("side depth");
        addChildComponent (*sideDepth);
       #if MISHKAL_V61
        link = std::make_unique<Knob> (state, id::resoLink, "link", pal::reso);
        link->slider.getProperties().set ("minimal", true);
        link->setTitleText ("link");
        link->slider.setTooltip ("Stereo link. 100%: left and right share one detector and always get the same treatment, so the image "
                                 "never moves. Lower: each side is also treated on its own, so a harsh guitar on one side is tamed "
                                 "without dulling the other. 0% = fully separate. Stereo mode only.");
        addChildComponent (*link);
       #endif
        startTimerHz (8);
    }
}

void ResoPanel::timerCallback()
{
    auto* p = dynamic_cast<VoxStripProcessor*> (&state.processor);
    const bool show = isGoliath() && (p == nullptr || ! p->isMonoTrack());
    const int mode = juce::roundToInt (state.getRawParameterValue (id::resoMs)->load());
    if (show != msShown || mode != msWas) { msShown = show; msWas = mode; resized(); }
}

void ResoPanel::lookAndFeelChanged()
{
    on.setColour (juce::ToggleButton::tickColourId, pal::reso);
    delta.setColour (juce::ToggleButton::tickColourId, pal::reso);
    repaint();
}

void ResoPanel::paint (juce::Graphics& g)
{
    if (isGoliath() && MISHKAL_V3)   // minimal: a flat inset panel, a lowercase title with a small dot
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff15161a));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (0xff24262b));
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        g.setColour (juce::Colour (MISHKAL_ACCENT));
        g.fillEllipse (14.0f, 11.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xffe9e7e2));
        g.setFont (screenFont (16.0f, 0).withExtraKerningFactor (0.06f));
        g.drawText ("resonance", 30, 4, 140, 22, juce::Justification::centredLeft);
        return;
    }
    drawSubPanel (g, getLocalBounds().toFloat());
    g.setColour (pal::reso);
    g.fillEllipse (14.0f, 14.0f, 10.0f, 10.0f);
    g.setColour (pal::text);
    g.setFont (font (13.0f, true));
    g.drawText ("RESONANCE", 30, 8, 120, 22, juce::Justification::centredLeft);
}

void ResoPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    auto header = r.removeFromTop (22);
    on.setBounds (header.removeFromRight (54).reduced (0, 1));
    header.removeFromRight (6);
    delta.setBounds (header.removeFromRight (64).reduced (0, 1));

    r.removeFromTop (8);
    if (isGoliath() && MISHKAL_V3)   // minimal: big depth, then a tidy 2-column grid; lowercase labels, value boxes
    {
        on.setVisible (false);
        knobs[6]->setVisible (false);
        knobs[7]->setVisible (false);
        const char* names[] { "depth", "sharpness", "selectivity", "mix", "attack", "release" };
        for (int i = 0; i < 6; ++i)
        {
            knobs[(size_t) i]->slider.getProperties().set ("minimal", true);
            knobs[(size_t) i]->setTitleText (names[i]);
            knobs[(size_t) i]->resized();
        }
        auto inner = getLocalBounds().reduced (6, 0).withTrimmedTop (34);
        delta.setBounds (getLocalBounds().reduced (10, 0).removeFromTop (30).removeFromRight (74));
        const bool ms = msMode != nullptr && msShown;
        const int depthH = juce::jmin (ms ? 150 : 170, inner.getHeight() / 2 - (ms ? 14 : 0));
        knobs[0]->setBounds (inner.removeFromTop (depthH).withSizeKeepingCentre (140, depthH));
        if (msMode != nullptr)   // V60: the M/S mode under depth, side depth in the free slot (stereo tracks only)
        {
            msMode->setVisible (ms);
            sideDepth->setVisible (ms && msWas == 3);
            if (ms) { inner.removeFromTop (4); msMode->setBounds (inner.removeFromTop (22).reduced (6, 0)); }
        }
        inner.removeFromTop (8);
        const int rowH = juce::jmin (98, inner.getHeight() / 2);
        auto row1 = inner.removeFromTop (rowH), row2 = inner.removeFromTop (rowH);
        const int w3 = row1.getWidth() / 3;
        if (link != nullptr)   // V61: sharpness / selectivity / link, attack / release / mix; in M + S the depths sit side by side
        {
            link->setVisible (ms);
            link->setEnabled (msWas == 0);   // the link ties left to right (Stereo mode)
            link->setAlpha (msWas == 0 ? 1.0f : 0.35f);
            knobs[1]->setBounds (row1.removeFromLeft (w3)); knobs[2]->setBounds (row1.removeFromLeft (w3));
            if (ms) { link->setBounds (row1); link->resized(); knobs[3]->setBounds (row2.removeFromRight (w3)); }
            else    knobs[3]->setBounds (row1);
            knobs[4]->setBounds (row2.removeFromLeft (w3)); knobs[5]->setBounds (row2.removeFromLeft (w3));
            if (sideDepth != nullptr && ms && msWas == 3)
            {
                auto top = knobs[0]->getBounds().withX (getLocalBounds().reduced (6, 0).getX()).withWidth (getLocalBounds().reduced (6, 0).getWidth());
                const int half = top.getWidth() / 2;
                knobs[0]->setBounds (top.removeFromLeft (half).reduced (4, 10));
                sideDepth->setBounds (top.reduced (4, 10));
                knobs[0]->setTitleText ("mid depth");
                sideDepth->resized();
            }
            knobs[0]->resized();
            return;
        }
        knobs[1]->setBounds (row1.removeFromLeft (w3)); knobs[2]->setBounds (row1.removeFromLeft (w3)); knobs[3]->setBounds (row1);
        knobs[4]->setBounds (row2.removeFromLeft (w3)); knobs[5]->setBounds (row2.removeFromLeft (w3));
        if (sideDepth != nullptr) { sideDepth->setBounds (row2); sideDepth->resized(); }
        return;
    }
    if (isGoliath())   // big Depth, then Sharpness / Selectivity, then Attack / Release / Mix; the range lives on the screen
    {
        on.setVisible (false);   // the module's RESO button does this
        knobs[6]->setVisible (false);
        knobs[7]->setVisible (false);
        knobs[0]->getProperties();
        knobs[0]->setBounds (r.removeFromTop (juce::jmin (150, r.getHeight() / 2 - 10)));
        r.removeFromTop (4);
        const int rowH = juce::jmin (100, r.getHeight() / 2);
        auto row1 = r.removeFromTop (rowH);
        knobs[1]->setBounds (row1.removeFromLeft (row1.getWidth() / 2));
        knobs[2]->setBounds (row1);
        auto row2 = r.removeFromTop (rowH);
        const int w3 = row2.getWidth() / 3;
        knobs[4]->setBounds (row2.removeFromLeft (w3));
        knobs[5]->setBounds (row2.removeFromLeft (w3));
        knobs[3]->setBounds (row2);
        return;
    }
    on.setVisible (true);
    knobs[6]->setVisible (true);
    knobs[7]->setVisible (true);
    const int rowH = juce::jmin (106, r.getHeight() / 2);
    const int w = r.getWidth() / 4;
    for (int row = 0; row < 2; ++row)
    {
        auto line = r.removeFromTop (rowH);
        for (int col = 0; col < 4; ++col)
            knobs[(size_t) (row * 4 + col)]->setBounds (line.removeFromLeft (w).reduced (1, 2));
    }
}

//==============================================================================
TabSwitch::TabSwitch (juce::StringArray n, std::vector<const juce::Colour*> c) : names (std::move (n)), colours (std::move (c)) {}

void TabSwitch::setIndex (int i)
{
    if (i == index) return;
    index = i;
    repaint();
    if (onChange) onChange (index);
}

void TabSwitch::mouseDown (const juce::MouseEvent& e)
{
    setIndex (juce::jlimit (0, names.size() - 1, (int) ((float) e.x / (float) getWidth() * (float) names.size())));
}

void TabSwitch::paint (juce::Graphics& g)
{
    drawSegmented (g, getLocalBounds().toFloat(), names, index, *colours[(size_t) juce::jlimit (0, (int) colours.size() - 1, index)]);
}

//==============================================================================
// Resonance view: the focus range between the Low and High edges is lit, the rest is dimmed.
// The live reduction (drawn above, hanging from 0 dB) shows exactly which resonances are being tamed.
void CleanupGraph::paintResoView (juce::Graphics& g, juce::Rectangle<float> b)
{
    if (MISHKAL_V3)
    {
        // minimal: soft dimming outside the range, round handles on the 0 dB line, and a floating info bar
        const auto& a = proc.apvts;
        const bool on = a.getRawParameterValue (id::resoOn)->load() > 0.5f;
        const float lo = a.getRawParameterValue (id::resoLow)->load(), hi = a.getRawParameterValue (id::resoHigh)->load();
        const float xl = xForFreq (lo), xh = xForFreq (hi), y0 = yForDb (0.0f);
        const auto coral = juce::Colour (MISHKAL_ACCENT), ink = juce::Colour (0xffe9e7e2);
        g.setColour (juce::Colour (0xff0b0b0c).withAlpha (0.55f));
        g.fillRect (b.withRight (xl));
        g.fillRect (b.withLeft (xh));
        g.setColour (ink.withAlpha (0.25f));
        g.drawLine (xl, b.getY(), xl, b.getBottom(), 1.0f);
        g.drawLine (xh, b.getY(), xh, b.getBottom(), 1.0f);
        auto fmt = [] (float hz) { return hz >= 1000.0f ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz" : juce::String ((int) hz) + " Hz"; };
        for (auto [x, hz] : { std::pair<float, float> { xl, lo }, { xh, hi } })
        {
            g.setColour (juce::Colour (0xff0b0b0c));
            g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ x, y0 }));
            g.setColour (on ? ink : ink.withAlpha (0.5f));
            g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ x, y0 }));
            g.setFont (screenFont (11.5f, 0));
            g.setColour (ink.withAlpha (0.7f));
            g.drawText (fmt (hz), juce::Rectangle<float> (80.0f, 14.0f).withCentre ({ x, y0 - 18.0f }), juce::Justification::centred);
        }
        // floating info bar
        const float depth = a.getRawParameterValue (id::resoDepth)->load();
        auto bar = juce::Rectangle<float> (300.0f, 40.0f).withCentre ({ b.getCentreX(), b.getBottom() - 34.0f });
        g.setColour (juce::Colour (0xff15161a).withAlpha (0.94f));
        g.fillRoundedRectangle (bar, 6.0f);
        g.setColour (juce::Colour (0xff2c2e33));
        g.drawRoundedRectangle (bar.reduced (0.5f), 6.0f, 1.0f);
        const juce::String labels[] { "depth", "low", "high" };
        const juce::String values[] { juce::String (juce::roundToInt (depth * 100.0f)) + " %", fmt (lo), fmt (hi) };
        auto cells = bar.reduced (10.0f, 4.0f);
        const float cw = cells.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            auto cell = cells.removeFromLeft (cw);
            g.setColour (juce::Colour (0xff9da0a8));
            g.setFont (screenFont (10.5f, 0));
            g.drawText (labels[i], cell.removeFromTop (14.0f), juce::Justification::centred);
            g.setColour (i == 0 ? (on ? coral : coral.withAlpha (0.5f)) : ink);
            g.setFont (screenFont (13.5f, 1));
            g.drawText (values[i] + (i == 0 && ! on ? "  off" : ""), cell, juce::Justification::centred);
        }
        return;
    }
    const auto& a = proc.apvts;
    const bool on = a.getRawParameterValue (id::resoOn)->load() > 0.5f;
    const float lo = a.getRawParameterValue (id::resoLow)->load(), hi = a.getRawParameterValue (id::resoHigh)->load();
    const float xl = xForFreq (lo), xh = xForFreq (hi);
    const auto red = pal::reso;

    // dim outside the focus range
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (b.withRight (xl));
    g.fillRect (b.withLeft (xh));
    g.setColour (red.withAlpha (on ? 0.06f : 0.03f));
    g.fillRect (juce::Rectangle<float> (xl, b.getY(), xh - xl, b.getHeight()));

    // edges with grab tabs
    auto edge = [&] (float x, const juce::String& label, float hz, bool left)
    {
        g.setColour (red.withAlpha (on ? 0.9f : 0.45f));
        g.fillRect (x - 1.0f, b.getY(), 2.0f, b.getHeight());
        auto tab = juce::Rectangle<float> (74.0f, 30.0f).withPosition (left ? x + 4.0f : x - 78.0f, b.getY() + 8.0f);
        g.setColour (juce::Colour (0xff0b0b0c).withAlpha (0.9f));
        g.fillRoundedRectangle (tab, 3.0f);
        g.setColour (red.withAlpha (0.8f));
        g.drawRoundedRectangle (tab, 3.0f, 1.0f);
        g.setFont (screenFont (9.5f, 1).withExtraKerningFactor (0.12f));
        g.drawText (label, tab.withHeight (14.0f).translated (0.0f, 2.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xffe6e6e6));
        g.setFont (screenFont (12.5f, 2));
        g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz" : juce::String ((int) hz) + " Hz",
                    tab.withTrimmedTop (13.0f), juce::Justification::centred);
    };
    edge (xl, "LOW", lo, true);
    edge (xh, "HIGH", hi, false);

    // depth readout + hint, bottom centre of the focus range
    const float depth = a.getRawParameterValue (id::resoDepth)->load();
    g.setColour (on ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff76767c));
    g.setFont (screenFont (13.0f, 2).withExtraKerningFactor (0.12f));
    g.drawText ("DEPTH " + juce::String (juce::roundToInt (depth * 100.0f)) + "%" + (on ? "" : "   (OFF)"),
                juce::Rectangle<float> (xl, b.getBottom() - 46.0f, xh - xl, 16.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff6a6a70));
    g.setFont (screenFont (11.0f, 0));
    g.drawText ("drag the red edges to set the range  -  scroll for depth", juce::Rectangle<float> (xl, b.getBottom() - 30.0f, xh - xl, 14.0f),
                juce::Justification::centred);
}
