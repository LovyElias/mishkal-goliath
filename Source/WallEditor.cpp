// Mishkal Wall - the editor: the timer that feeds the screen, the mouse (Pro-Q 4 style on every control), the menus, the window
#include "WallDraw.h"

#if JUCE_MAC
float wallMacFullScale (void* nsView, float W, float H, float curW, float curH);
void wallMacPinTop (void* nsView);
#endif

using namespace wallui;

WallCanvas::WallCanvas (WallProcessor& p) : proc (p)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    view = juce::jlimit (0, 2, proc.uiView);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    startTimerHz (40);
}
WallCanvas::~WallCanvas() { stopTimer(); }

float WallCanvas::norm (const juce::String& id) const
{
    if (auto* p = proc.par (id)) return p->getValue();
    return 0.0f;
}

const WallCanvas::Zone* WallCanvas::zoneAt (juce::Point<float> p) const
{
    for (auto it = zones.rbegin(); it != zones.rend(); ++it)   // (the last drawn is on top)
        if (it->r.contains (p)) return &*it;
    return nullptr;
}

// ============================================================================================================
void WallCanvas::timerCallback()
{
    const float dt = 1.0f / 40.0f;
    animT += dt;
    // the wave's new columns
    {
        int s1, z1, s2, z2;
        auto& f = proc.frameFifo;
        const int ready = f.getNumReady();
        f.prepareToRead (ready, s1, z1, s2, z2);
        for (int j = 0; j < z1; ++j) { wave[(size_t) wavePos] = proc.frames[(size_t) (s1 + j)]; wavePos = (wavePos + 1) % cols; }
        for (int j = 0; j < z2; ++j) { wave[(size_t) wavePos] = proc.frames[(size_t) (s2 + j)]; wavePos = (wavePos + 1) % cols; }
        f.finishedRead (z1 + z2);
    }
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    // edge meters: quick up, falling 24 dB a second; peak numbers hold 2 s
    const float lv[4] { proc.inCh[0].load(), proc.inCh[1].load(), proc.outCh[0].load(), proc.outCh[1].load() };
    for (int i = 0; i < 4; ++i) { const float d = wall::gainToDb (lv[i]); shown[i] = d > shown[i] ? d : std::max (d, shown[i] - 24.0f * dt); }
    for (int i = 0; i < 2; ++i)
    {
        const float d = std::max (shown[i * 2], shown[i * 2 + 1]);
        if (d >= pkHold[i] || now - pkTime[i] > 2.0) { pkHold[i] = d; pkTime[i] = now; }
    }
    const float gr = proc.grDb.load();
    grShown = gr > grShown ? gr : grShown + (gr - grShown) * 0.12f;
    grAvg += (gr - grAvg) * (dt / 3.0f);
    if (grShown >= grPeak || now - grPeakT > 2.0) { grPeak = grShown; grPeakT = now; }
    limShown = proc.limGrDb.load() > limShown ? proc.limGrDb.load() : limShown + (proc.limGrDb.load() - limShown) * 0.12f;
    levShown += (proc.levDb.load() - levShown) * 0.2f;
    const float cl = proc.clipDb.load(); clipShown = cl > clipShown ? cl : clipShown + (cl - clipShown) * 0.1f;
    const float gh = juce::jlimit (0.0f, 1.0f, proc.goldHot.load() * 4.0f + (pv ("goldOn") > 0.5f ? (float) (int) pv ("goldDrive") * 0.06f : 0.0f));
    goldShown = gh > goldShown ? goldShown + (gh - goldShown) * 0.5f : goldShown + (gh - goldShown) * 0.08f;
    if (proc.lufsS.load() > -60.0f)
    {
        const float c2 = juce::jlimit (0.0f, 20.0f, proc.tpShortDb.load() - proc.lufsS.load());
        crest += (c2 - crest) * 0.15f; crestAvg += (c2 - crestAvg) * (dt / 4.0f);
    }
    const float co = proc.corr.load();
    corrShown += (co - corrShown) * 0.2f;
    corrLo = std::min (corrShown, corrLo + (corrShown - corrLo) * dt * 0.4f);
    corrHi = std::max (corrShown, corrHi + (corrShown - corrHi) * dt * 0.4f);
    // (if painting runs slow, skip every other frame rather than fall behind)
    if (lastPaintMs > 22.0 && (++skip & 1)) return;
    repaint();
}

// ============================================================================================================
// the mouse
void WallCanvas::mouseMove (const juce::MouseEvent& e) { mouse = e.position; }

void WallCanvas::mouseDown (const juce::MouseEvent& e)
{
    if (typeIn != nullptr) closeTypeIn (true);
    mouse = downAt = e.position; moved = false; dragging = false; chainDrag = -1;
    const auto* z = zoneAt (e.position);
    down = z != nullptr ? *z : Zone {};
    if (z == nullptr) return;
    if (e.mods.isPopupMenu())
    {
        switch (z->kind)
        {
            case Hit::Knob: if (z->pid == "ceiling") showCeilingMenu (z->r); else showParamMenu (z->pid, z->r); break;
            case Hit::Learn: showParamMenu ("target", z->r); break;
            case Hit::Preview: showPreviewMenu (z->r); break;
            case Hit::CycleTab: case Hit::Choice: case Hit::Toggle: case Hit::GoldStep: showParamMenu (z->pid, z->r); break;
            case Hit::ChainBlock: case Hit::ChainDot:
            {
                juce::PopupMenu m;
                m.addItem (1, "Back to the first order (Punch > Character > Bands > Leveler > Gold > Clipper)");
                m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z->r).getSmallestIntegerContainer()),
                                 [this] (int r) { if (r == 1) proc.setOrder ({ 0, 1, 2, 3, 4, 5 }); repaint(); });
                break;
            }
            default: break;
        }
        down = {};
        return;
    }
    if (z->kind == Hit::Knob && e.mods.isAltDown())   // Option-click: back to its default
    {
        if (auto* p = proc.par (z->pid)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
        down = {};
        return;
    }
    if (z->kind == Hit::Knob || z->kind == Hit::Learn || z->kind == Hit::BandXover)
    {
        const juce::String pid = z->kind == Hit::Learn ? juce::String ("target") : z->pid;
        dragStart = norm (pid);
        proc.beginGesture (pid);
        dragging = true;
    }
}

void WallCanvas::mouseDrag (const juce::MouseEvent& e)
{
    mouse = e.position;
    if (e.getDistanceFromDragStart() > 3) moved = true;
    switch (down.kind)
    {
        case Hit::Knob: case Hit::Learn:
        {
            if (! moved) return;
            const juce::String pid = down.kind == Hit::Learn ? juce::String ("target") : down.pid;
            const float speed = e.mods.isCommandDown() || e.mods.isShiftDown() ? 1.0f / 900.0f : 1.0f / 220.0f;   // Command: fine (Pro-Q 4)
            const float v = juce::jlimit (0.0f, 1.0f, dragStart + (downAt.y - e.position.y + (e.position.x - downAt.x) * 0.25f) * speed);
            if (auto* p = proc.par (pid)) p->setValueNotifyingHost (v);
            break;
        }
        case Hit::BandXover:
        {
            // back into the bands' own drawing coordinates (drawn at 80% around the bottom centre)
            const float ax = C, sc = 0.8f, lx = (e.position.x - ax) / sc + ax;
            const float BX0 = C - 372 + 10, BX1 = C + 372 - 10;
            float hz = 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (lx - BX0) / (BX1 - BX0)));
            if (auto* p = proc.par (down.pid)) p->setValueNotifyingHost (p->convertTo0to1 (p->getNormalisableRange().snapToLegalValue (hz)));
            break;
        }
        case Hit::ChainBlock:
            if (down.a < 6 && down.pid.isEmpty() && moved) { chainDrag = down.a; chainDragX = e.position.x; repaint(); }
            break;
        default: break;
    }
}

void WallCanvas::mouseUp (const juce::MouseEvent& e)
{
    if (dragging)
    {
        const juce::String pid = down.kind == Hit::Learn ? juce::String ("target") : down.pid;
        proc.endGesture (pid);
        dragging = false;
    }
    if (chainDrag >= 0)
    {
        // drop: where it lands in the strip
        const float bw = 76, aw = 7, lw = tw ("CHAIN", Face::CondMedium, 9, 2) + 10, total = 7 * bw + 6 * aw, x0 = C - total / 2 + lw / 2 + 34;
        const int to = juce::jlimit (0, 5, (int) std::floor ((e.position.x - x0) / (bw + aw)));
        auto o = proc.order();
        const int st = o[(size_t) chainDrag];
        std::vector<int> v (o.begin(), o.end()); v.erase (v.begin() + chainDrag); v.insert (v.begin() + to, st);
        std::array<int, 6> n {}; for (int i = 0; i < 6; ++i) n[(size_t) i] = v[(size_t) i];
        proc.setOrder (n);
        chainDrag = -1; down = {};
        repaint();
        return;
    }
    if (! moved && down.kind != Hit::None) click (down, e);
    down = {};
    repaint();
}

void WallCanvas::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto* z = zoneAt (e.position);
    if (z == nullptr) return;
    if (z->kind == Hit::Knob) openTypeIn (z->pid, z->r);
    if (z->kind == Hit::Learn) openTypeIn ("target", z->r);
}

void WallCanvas::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto* z = zoneAt (e.position);
    if (z == nullptr) return;
    juce::String pid;
    if (z->kind == Hit::Knob || z->kind == Hit::BandXover) pid = z->pid;
    else if (z->kind == Hit::Learn) pid = "target";
    else return;
    if (auto* p = proc.par (pid))
    {
        const float step = e.mods.isCommandDown() ? 0.01f : 0.05f;
        p->beginChangeGesture(); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY + w.deltaX) * step)); p->endChangeGesture();
    }
}

bool WallCanvas::keyPressed (const juce::KeyPress& k)
{
    const bool cmd = k.getModifiers().isCommandDown(), shift = k.getModifiers().isShiftDown();
    if (cmd && k.getKeyCode() == 'Z') { if (shift) proc.redo(); else proc.undo(); return true; }
    if (k == juce::KeyPress::escapeKey) { stage = -1; repaint(); return true; }
    return false;
}

// ============================================================================================================
void WallCanvas::click (const Zone& z, const juce::MouseEvent& e)
{
    auto flip = [this] (const juce::String& pid) { proc.setParam (pid, pv (pid) > 0.5f ? 0.0f : 1.0f); };
    switch (z.kind)
    {
        case Hit::Knob: if (z.a == 1) openTypeIn (z.pid, z.r); break;
        case Hit::Toggle:
            if (z.pid == "bandsOn") proc.bandsSwitched (pv ("bandsOn") < 0.5f); else flip (z.pid);
            break;
        case Hit::Choice:
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.par (z.pid)))
            {
                juce::PopupMenu m;
                m.addSectionHeader (c->getName (40));
                for (int i = 0; i < c->choices.size(); ++i) m.addItem (1 + i, c->choices[i], true, i == c->getIndex());
                const auto pid = z.pid;
                m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (z.r).getSmallestIntegerContainer()),
                                 [this, pid] (int r) { if (r > 0) proc.setParam (pid, (float) (r - 1)); repaint(); });
            }
            break;
        case Hit::CycleTab:
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.par (z.pid)))
            {
                const int n = c->choices.size(), dir = (e.position.x < z.r.getX() + z.r.getWidth() * 0.25f) ? -1 : 1;
                const int to = (c->getIndex() + dir + n) % n;
                if (z.pid == "limStyle") proc.setStyle (to); else proc.setParam (z.pid, (float) to);
            }
            break;
        case Hit::Learn: proc.startLearn(); break;
        case Hit::Undo: proc.undo(); break;
        case Hit::Redo: proc.redo(); break;
        case Hit::History: showHistoryMenu (z.r); break;
        case Hit::Preset: showPresetMenu (z.r); break;
        case Hit::PresetPrev: stepPreset (-1); break;
        case Hit::PresetNext: stepPreset (1); break;
        case Hit::SavePreset: savePresetAs(); break;
        case Hit::AB: proc.switchAB(); break;
        case Hit::Match: proc.copyAB(); break;
        case Hit::Power: flip ("power"); break;
        case Hit::StylePrev: proc.setStyle (((int) pv ("limStyle") + 8) % 9); break;
        case Hit::StyleNext: proc.setStyle (((int) pv ("limStyle") + 1) % 9); break;
        case Hit::StyleDot: proc.setStyle (z.a); break;
        case Hit::Preview: proc.setParam ("preview", pv ("preview") > 0.5f ? 0.0f : pv ("previewWhere") + 1.0f); break;
        case Hit::Unity: flip ("unity"); break;
        case Hit::Delta: flip ("delta"); break;
        case Hit::Auto: proc.startAutoTrim(); break;
        case Hit::View: view = z.a; stage = -1; proc.uiView = view; break;
        case Hit::ChainBlock:
        {
            const int st = z.pid == "lamp" ? z.a : (z.a < 6 ? proc.order()[(size_t) z.a] : 6);
            if (st == wall::stBands) { view = view == 1 ? 0 : 1; stage = -1; }
            else { if (view != 0) { view = 0; stage = st; } else stage = stage == st ? -1 : st; }
            proc.uiView = view;
            break;
        }
        case Hit::ChainDot:
        {
            const int st = z.a < 6 ? proc.order()[(size_t) z.a] : 6;
            if (st == 6) flip ("limOn");
            else if (st == wall::stBands) proc.bandsSwitched (pv ("bandsOn") < 0.5f);
            else flip (wall::stageOnId (st));
            break;
        }
        case Hit::Size: showSizeMenu (z.r); break;
        case Hit::GoldStep: proc.setParam ("goldDrive", (float) z.a); if (z.a > 0) proc.setParam ("goldOn", 1.0f); break;
        case Hit::BandBody: selBand = z.a; break;
        case Hit::BandAdd: proc.setParam ("bCount", std::min (4.0f, pv ("bCount") + 1)); selBand = (int) pv ("bCount") - 1; break;
        case Hit::BandRemove: proc.setParam ("bCount", std::max (2.0f, pv ("bCount") - 1)); break;
        case Hit::BandSolo: flip (wall::bandId (z.a, "Solo")); break;
        case Hit::BandByp: flip (wall::bandId (z.a, "Byp")); break;
        case Hit::LoudReset: proc.resetLoudness(); break;
        default: break;
    }
    repaint();
}

// ============================================================================================================
// typing a value (double-click a knob, or click its number)
void WallCanvas::openTypeIn (const juce::String& pid, R near)
{
    auto* p = proc.par (pid);
    if (p == nullptr || dynamic_cast<juce::AudioParameterBool*> (p) != nullptr) return;
    if (typeIn != nullptr) closeTypeIn (true);
    typeInPid = pid;
    typeIn = std::make_unique<juce::TextEditor>();
    auto& t = *typeIn;
    t.setFont (font (Face::Regular, 12.0f));
    t.setJustification (juce::Justification::centred);
    t.setColour (juce::TextEditor::backgroundColourId, hex (0x1c1c22));
    t.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    t.setColour (juce::TextEditor::outlineColourId, amber);
    t.setColour (juce::TextEditor::focusedOutlineColourId, amber);
    t.setColour (juce::TextEditor::highlightColourId, amber.withAlpha (0.4f));
    t.setIndents (4, 3);
    juce::String now;
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p)) now = c->getCurrentChoiceName();
    else
    {
        const float v = proc.param (pid);
        const auto range = p->getNormalisableRange();
        now = juce::String (v, range.interval >= 1.0f ? 0 : (range.interval >= 0.1f ? 1 : 2));
        if (now.containsChar ('.')) now = now.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    }
    t.setText (now, false);
    auto r = near.withSizeKeepingCentre (juce::jlimit (64.0f, 110.0f, near.getWidth()), 22.0f).constrainedWithin (getLocalBounds().toFloat().reduced (4));
    t.setBounds (r.getSmallestIntegerContainer());
    addAndMakeVisible (t);
    t.selectAll(); t.grabKeyboardFocus();
    const juce::Component::SafePointer<WallCanvas> sp (this);
    t.onReturnKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
    t.onEscapeKey = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (false); }); };
    t.onFocusLost = [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->closeTypeIn (true); }); };
}

void WallCanvas::closeTypeIn (bool commit)
{
    if (typeIn == nullptr) return;
    if (commit)
        if (auto* p = proc.par (typeInPid))
        {
            const auto txt = typeIn->getText().trim();
            float v = 0.0f; bool ok = false;
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p))
            {
                for (int i = 0; i < c->choices.size() && ! ok; ++i) if (c->choices[i].equalsIgnoreCase (txt) || c->choices[i].startsWithIgnoreCase (txt)) { v = (float) i; ok = true; }
                if (! ok && txt.containsAnyOf ("0123456789"))
                {
                    const float want = txt.retainCharacters ("-.0123456789").getFloatValue(); float best = 1e9f;
                    for (int i = 0; i < c->choices.size(); ++i) { const float f = c->choices[i].retainCharacters ("-.0123456789").getFloatValue(); if (std::abs (f - want) < best) { best = std::abs (f - want); v = (float) i; ok = true; } }
                }
            }
            else
            {
                auto s = txt.toLowerCase().replace (",", ".").replace (juce::String::fromUTF8 ("\xe2\x88\x92"), "-");
                const float m = s.containsChar ('k') ? 1000.0f : 1.0f;
                auto n = s.retainCharacters ("-+0123456789.");
                if (n.containsAnyOf ("0123456789")) { v = n.getFloatValue() * m; ok = true; }
            }
            if (ok) proc.setParam (typeInPid, p->getNormalisableRange().snapToLegalValue (juce::jlimit (p->getNormalisableRange().start, p->getNormalisableRange().end, v)));
        }
    auto old = std::move (typeIn);
    removeChildComponent (old.get());
    juce::MessageManager::callAsync ([o = std::shared_ptr<juce::TextEditor> (old.release())] {});
    grabKeyboardFocus();
    repaint();
}

// right-click any control: type a value, back to its default, or pick a setting
void WallCanvas::showParamMenu (const juce::String& pid, R near)
{
    auto* p = proc.par (pid);
    if (p == nullptr) return;
    juce::PopupMenu m;
    m.addSectionHeader (p->getName (40));
    const bool isBool = dynamic_cast<juce::AudioParameterBool*> (p) != nullptr;
    if (! isBool) m.addItem (1, "Type a value...");
    m.addItem (2, "Back to default  (" + (p->getText (p->getDefaultValue(), 20) + " " + p->getLabel()).trim() + ")");
    auto* c = dynamic_cast<juce::AudioParameterChoice*> (p);
    if (c != nullptr) { m.addSeparator(); for (int i = 0; i < c->choices.size(); ++i) m.addItem (100 + i, c->choices[i], true, i == c->getIndex()); }
    if (pid == "limLinkT" || pid == "limLinkR")
    {
        m.addSeparator(); m.addSectionHeader ("Release link");
        for (int v : { 0, 30, 60, 100 }) m.addItem (200 + v, juce::String (v) + "%", true, juce::roundToInt (pv ("limLinkR")) == v);
    }
    m.addSeparator();
    m.addItem (3, "Fine: hold Command while dragging", false, false);
    m.addItem (4, "Reset: Option-click   Type: double-click", false, false);
    const juce::Component::SafePointer<WallCanvas> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (near).getSmallestIntegerContainer()),
        [sp, pid, near] (int r)
        {
            if (sp == nullptr || r <= 0) return;
            auto* q = sp->proc.par (pid);
            if (r == 1) sp->openTypeIn (pid, near);
            else if (r == 2 && q != nullptr) { q->beginChangeGesture(); q->setValueNotifyingHost (q->getDefaultValue()); q->endChangeGesture(); }
            else if (r >= 200) sp->proc.setParam ("limLinkR", (float) (r - 200));
            else if (r >= 100) { if (pid == "limStyle") sp->proc.setStyle (r - 100); else sp->proc.setParam (pid, (float) (r - 100)); }
            sp->repaint();
        });
}

// right-click the ceiling: the platforms
void WallCanvas::showCeilingMenu (R at)
{
    juce::PopupMenu m;
    m.addSectionHeader ("CEILING FOR");
    const std::tuple<const char*, float, bool> items[] { { "Streaming (Spotify, YouTube)", -1.0f, true }, { "Apple Music", -1.0f, true }, { "SoundCloud / MP3", -1.5f, true },
                                                         { "CD / Master", -0.3f, true }, { "Club / DJ", -0.1f, false } };
    int i = 1;
    for (auto& [n, v, tp] : items) { m.addItem (i, juce::String (n) + "    " + dbs (v) + (tp ? " dBTP" : " dB"), true, std::abs (pv ("ceiling") - v) < 0.05f && (pv ("tp") > 0.5f) == tp); ++i; }
    m.addSeparator();
    m.addItem (20, "Type a value...");
    m.addItem (21, juce::String ("Default (") + minus + "1.0)");
    const juce::Component::SafePointer<WallCanvas> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at).getSmallestIntegerContainer()), [sp, at] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        static const float v[] { -1.0f, -1.0f, -1.5f, -0.3f, -0.1f };
        static const bool tp[] { true, true, true, true, false };
        if (r >= 1 && r <= 5) { sp->proc.setParam ("ceiling", v[r - 1]); sp->proc.setParam ("tp", tp[r - 1] ? 1.0f : 0.0f); }
        if (r == 20) sp->openTypeIn ("ceiling", at);
        if (r == 21) sp->proc.setParam ("ceiling", -1.0f);
        sp->repaint();
    });
}

void WallCanvas::showPreviewMenu (R at)
{
    juce::PopupMenu m;
    m.addSectionHeader ("PLAY IT AS LOUD AS ... WILL");
    static const char* n[] { "Spotify  (-14 LUFS)", "Apple Music  (-16 LUFS)", "YouTube  (-14 LUFS)", "Tidal  (-14 LUFS)", "Amazon  (-14 LUFS)" };
    for (int i = 0; i < 5; ++i) m.addItem (1 + i, n[i], true, (int) pv ("previewWhere") == i);
    m.addSeparator(); m.addItem (10, "Preview off", true, pv ("preview") < 0.5f);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at).getSmallestIntegerContainer()), [this] (int r)
    {
        if (r >= 1 && r <= 5) { proc.setParam ("previewWhere", (float) (r - 1)); proc.setParam ("preview", (float) r); }
        if (r == 10) proc.setParam ("preview", 0.0f);
        repaint();
    });
}

void WallCanvas::showSizeMenu (R at)
{
    juce::PopupMenu m;
    static const float sc[5] { 0.7f, 0.85f, 1.0f, 1.2f, 1.4f };
    static const char* nm[5] { "Mini  (876 x 560)", "Small  (1064 x 680)", "Medium  (1252 x 800)", "Large  (1502 x 960)", "Extra Large  (1753 x 1120)" };
    for (int i = 0; i < 5; ++i) m.addItem (1 + i, nm[i], true, ! fullScreen && std::abs (currentScale - sc[i]) < 0.01f);
    m.addSeparator(); m.addItem (10, "Full screen", true, fullScreen);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at).getSmallestIntegerContainer()), [this] (int r)
    {
        if (r >= 1 && r <= 5) { fullScreen = false; if (onSize) onSize (sc[r - 1]); }
        if (r == 10 && onFullScreen) onFullScreen();
        repaint();
    });
}

void WallCanvas::showHistoryMenu (R at)
{
    juce::PopupMenu m;
    m.addSectionHeader ("HISTORY");
    const auto names = proc.historyNames();
    for (int i = names.size() - 1; i >= 0 && i >= names.size() - 30; --i) m.addItem (1 + i, names[i], true, i == proc.historyIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at).getSmallestIntegerContainer()), [this] (int r) { if (r > 0) proc.jumpHistory (r - 1); repaint(); });
}

void WallCanvas::stepPreset (int dir)
{
    const int n = proc.getNumPrograms();
    proc.setCurrentProgram ((proc.getCurrentProgram() + dir + n) % n);
    repaint();
}

void WallCanvas::showPresetMenu (R at)
{
    juce::PopupMenu m;
    m.addSectionHeader ("WALL PRESETS");
    for (int i = 0; i < proc.getNumPrograms(); ++i) m.addItem (1 + i, proc.getProgramName (i), true, proc.presetName == proc.getProgramName (i));
    const auto user = proc.userPresets();
    if (! user.isEmpty())
    {
        m.addSeparator(); m.addSectionHeader ("MINE");
        for (int i = 0; i < user.size(); ++i) m.addItem (500 + i, user[i].getFileNameWithoutExtension(), true, proc.presetName == user[i].getFileNameWithoutExtension());
    }
    m.addSeparator();
    m.addItem (900, "Save as...");
    m.addItem (901, "Show my presets folder");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (at).getSmallestIntegerContainer()), [this, user] (int r)
    {
        if (r >= 1 && r < 500) proc.setCurrentProgram (r - 1);
        else if (r >= 500 && r < 900 && r - 500 < user.size()) proc.loadUserPreset (user[r - 500]);
        else if (r == 900) savePresetAs();
        else if (r == 901) { auto f = WallProcessor::userPresetFolder(); f.createDirectory(); f.revealToUser(); }
        repaint();
    });
}

void WallCanvas::savePresetAs()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name this setting:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.presetName.upToFirstOccurrenceOf (juce::String (" ") + middot, false, false));
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    const juce::Component::SafePointer<WallCanvas> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty()) sp->proc.saveUserPreset (n); sp->repaint(); }
    }), true);
}

// ============================================================================================================
WallEditor::WallEditor (WallProcessor& p) : AudioProcessorEditor (p), proc (p), canvas (p)
{
    addAndMakeVisible (canvas);
    canvas.onFullScreen = [this]
    {
        if (! canvas.fullScreen)
        {
            scaleBeforeFull = (float) getWidth() / W;
            float s = 0.0f;
           #if JUCE_MAC
            if (auto* peer = getPeer()) s = wallMacFullScale (peer->getNativeHandle(), W, H, (float) getWidth(), (float) getHeight());
           #endif
            if (s <= 0.0f)
            {
                auto area = getScreenBounds();
                if (auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (area)) area = d->userArea;
                s = juce::jmin ((float) area.getWidth() / W, (float) (area.getHeight() - 80) / H);
            }
            canvas.fullScreen = true;
            setSize (juce::roundToInt (W * s), juce::roundToInt (H * s));
        }
        else
        {
            canvas.fullScreen = false;
            setSize (juce::roundToInt (W * scaleBeforeFull), juce::roundToInt (H * scaleBeforeFull));
        }
       #if JUCE_MAC
        juce::Component::SafePointer<WallEditor> safe (this);
        for (int ms : { 60, 250 })
            juce::Timer::callAfterDelay (ms, [safe] { if (safe != nullptr) if (auto* peer = safe->getPeer()) wallMacPinTop (peer->getNativeHandle()); });
       #endif
    };
    canvas.onSize = [this] (float s) { setSize (juce::roundToInt (W * s), juce::roundToInt (H * s)); globalScale (s); };
    limits.setFixedAspectRatio (W / H);
    limits.setSizeLimits (751, 480, 3840, 2454);
    setConstrainer (&limits);
    setResizable (true, false);
    const float s = juce::jlimit (0.6f, 1.6f, (float) proc.apvts.state.getProperty ("uiScale", globalScale (-1.0f)));
    setSize (juce::roundToInt (W * s), juce::roundToInt (H * s));
}

// the size you picked last is remembered for every new Wall (like Pro-L 2); pass < 0 to read it
float WallEditor::globalScale (float set)
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Wall"; o.filenameSuffix = "settings"; o.folderName = "Mishkal Audio";
    o.osxLibrarySubFolder = "Application Support";
    juce::PropertiesFile f (o);
    if (set > 0.0f) { f.setValue ("scale", set); f.saveIfNeeded(); return set; }
    return (float) f.getDoubleValue ("scale", 0.85);
}

void WallEditor::resized()
{
    const float s = (float) getWidth() / W;
    canvas.currentScale = s;
    canvas.setBounds (0, 0, (int) W, (int) H);
    canvas.setTransform (juce::AffineTransform::scale (s));
    proc.apvts.state.setProperty ("uiScale", s, nullptr);
}
