// Top bar: logo, undo/redo, preset browser, save, A/B compare, menu
#include "PluginEditor.h"

using namespace vox;
using namespace vox::ui;

// toggle lamp colours: 0 red (default, e.g. BYPASS), 1 yellow (MATCH, PEAK), 2 green (TRACK)
static juce::Colour lampColourFor (const juce::Button& b)
{
    const int c = (int) b.getProperties()["lampColour"];
    return c == 1 ? pal::eq : c == 2 ? juce::Colour (0xff3fcf6e) : pal::alert;
}

static void drawBarButtonBase (juce::Graphics& g, juce::Rectangle<float> r, bool highlighted, bool down)
{
    if (isGoliath())   // plain text on the black display; a faint underline on hover
    {
        if (highlighted || down)
        {
            g.setColour (juce::Colour (down ? 0xff5a5a5e : 0xff3a3a3d));
            g.fillRect (r.getX() + 6.0f, r.getBottom() - 4.0f, r.getWidth() - 12.0f, 1.0f);
        }
        return;
    }
    if (isMetal())
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f), 4.0f);
        if (isLightPanel())
            g.setGradientFill (juce::ColourGradient (down ? pal::panel.darker (0.15f) : pal::panelHi.brighter (highlighted ? 0.05f : 0.0f), 0.0f, r.getY(),
                                                     down ? pal::panel.darker (0.05f) : pal::panel.darker (0.12f), 0.0f, r.getBottom(), false));
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff26262a : 0xff434347), 0.0f, r.getY(),
                                                     juce::Colour (down ? 0xff303034 : 0xff2a2a2e), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.16f : 0.09f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        return;
    }
    g.setColour (down ? pal::panel : pal::panelHi.brighter (highlighted ? 0.08f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (isRoyal() ? pal::io.withAlpha (highlighted ? 0.6f : 0.35f) : (highlighted ? pal::dim : pal::line));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    drawBarButtonBase (g, r, highlighted, down || (b.getClickingTogglesState() && b.getToggleState()));
    if (b.getClickingTogglesState() && b.getToggleState() && ! isGoliath())   // a lit toggle: lamp-coloured glow (BYPASS red, MATCH yellow)
    {
        const auto lampCol = lampColourFor (b);
        g.setColour (lampCol.withAlpha (0.22f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (lampCol.withAlpha (0.9f));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);
    }
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool)
{
    auto area = b.getLocalBounds();
    if (isGoliath())
    {
        const bool toggle = b.getClickingTogglesState(), on = toggle && b.getToggleState();
        auto font_ = (b.getParentComponent() != nullptr && b.getParentComponent()->getParentComponent() != nullptr
                      && dynamic_cast<VoxStripEditor*> (b.getParentComponent()->getParentComponent()) != nullptr)
                         ? screenFont (12.5f, 2).withExtraKerningFactor (0.14f) : font (12.0f, true);
        if (toggle)   // lamp to the left of the label
        {
            auto lampCol = lampColourFor (b);
            if (on && b.getProperties().contains ("activity"))
                lampCol = lampCol.interpolatedWith (juce::Colour (0xffff3d3d), juce::jlimit (0.0f, 1.0f, (float) b.getProperties()["activity"]));
            const float tw = juce::GlyphArrangement::getStringWidth (font_, b.getButtonText());
            const float x0 = (float) area.getCentreX() - (tw + 14.0f) * 0.5f;
            drawGoliathLed (g, { x0, (float) area.getCentreY() - 4.5f, 9.0f, 9.0f }, lampCol, on);
            area = area.withTrimmedLeft (juce::roundToInt (x0 - (float) area.getX() + 14.0f)).withWidth (juce::roundToInt (tw + 6.0f));
        }
        g.setColour (! b.isEnabled() ? juce::Colour (0xff4a4a4e) : (on || highlighted) ? juce::Colour (0xffe6e6e6) : juce::Colour (0xff8e8e93));
        g.setFont (font_);
        g.drawText (b.getButtonText(), area, toggle ? juce::Justification::centredLeft : juce::Justification::centred);
        return;
    }
    if (b.getClickingTogglesState())   // toggle buttons carry a small lamp to the left of the label
    {
        const bool on = b.getToggleState();
        auto lampCol = lampColourFor (b);
        if (on && b.getProperties().contains ("activity"))   // e.g. PEAK flashing red as it catches
            lampCol = lampCol.interpolatedWith (pal::alert, juce::jlimit (0.0f, 1.0f, (float) b.getProperties()["activity"]));
        auto lamp = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ (float) area.getX() + 14.0f, (float) area.getCentreY() });
        if (on)
        {
            g.setColour (lampCol.withAlpha (0.45f));
            g.fillEllipse (lamp.expanded (2.5f));
        }
        g.setColour (on ? lampCol : juce::Colour (0xff2a1414));
        g.fillEllipse (lamp);
        area.removeFromLeft (16);
    }
    g.setColour (b.isEnabled() ? ((highlighted || b.getToggleState()) ? pal::text : pal::text.withAlpha (0.88f)) : pal::dim.withAlpha (0.5f));
    // shrink the label to fit narrow buttons (e.g. 1964 PEAK on the Input strip)
    float size = 12.5f;
    while (size > 9.0f && juce::GlyphArrangement::getStringWidth (font (size, true), b.getButtonText()) > (float) area.getWidth() - 6.0f)
        size -= 0.5f;
    g.setFont (font (size, true));
    g.drawText (b.getButtonText(), area, juce::Justification::centred);
}

//==============================================================================
void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    if (boxed) drawBarButtonBase (g, r, highlighted, down);

    const auto col = ! isEnabled() ? pal::dim.withAlpha (0.35f) : (highlighted ? pal::text : pal::text.withAlpha (0.8f));
    const auto c = r.getCentre();
    juce::Path p;
    using PST = juce::PathStrokeType;

    switch (icon)
    {
        case Icon::Left:
        case Icon::Right:
        {
            const float s = icon == Icon::Left ? -1.0f : 1.0f;
            p.addTriangle (c.x + 5.0f * s, c.y, c.x - 4.0f * s, c.y - 6.0f, c.x - 4.0f * s, c.y + 6.0f);
            g.setColour (col);
            g.fillPath (p);
            break;
        }
        case Icon::Menu:
            g.setColour (col);
            for (int i = -1; i <= 1; ++i)
                g.fillEllipse (juce::Rectangle<float> (3.6f, 3.6f).withCentre ({ c.x, c.y + 6.0f * (float) i }));
            break;
        case Icon::Undo:
        case Icon::Redo:
        {
            const float s = icon == Icon::Undo ? 1.0f : -1.0f;
            p.startNewSubPath (c.x - 7.0f * s, c.y - 1.0f);
            p.quadraticTo (c.x + 1.0f * s, c.y - 9.0f, c.x + 8.0f * s, c.y - 1.0f);
            p.quadraticTo (c.x + 9.0f * s, c.y + 4.0f, c.x + 5.0f * s, c.y + 7.0f);
            g.setColour (col);
            g.strokePath (p, PST (2.0f, PST::curved, PST::rounded));
            juce::Path head;
            head.addTriangle (c.x - 10.0f * s, c.y - 1.5f, c.x - 3.5f * s, c.y - 5.5f, c.x - 3.5f * s, c.y + 3.0f);
            g.fillPath (head);
            break;
        }
    }
}

//==============================================================================
void PresetBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    if (isGoliath())
    {
        g.setColour (juce::Colour (0xff121214));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (juce::Colour (0xff333337));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        auto arrowArea = r.removeFromRight (28.0f);
        juce::Path tri;
        const auto c = arrowArea.getCentre();
        tri.addTriangle (c.x - 4.5f, c.y - 2.5f, c.x + 4.5f, c.y - 2.5f, c.x, c.y + 3.0f);
        g.setColour (goliathLabel());
        g.fillPath (tri);
        if (starShown)   // V67: favourite star on the left
        {
            juce::Path star;
            const auto sc = juce::Point<float> (r.getX() + 16.0f, r.getCentreY());
            star.addStar (sc, 5, 3.0f, 7.0f, -juce::MathConstants<float>::halfPi * 0.0f);
            g.setColour (starLit ? juce::Colour (0xffffb83d) : juce::Colour (0xff4a4b50));
            if (starLit) g.fillPath (star); else g.strokePath (star, juce::PathStrokeType (1.2f));
        }
        g.setColour (juce::Colour (0xffe6e6e6));
        g.setFont (screenFont (14.0f, 1));
        g.drawText (text, r.withTrimmedLeft (28.0f), juce::Justification::centred, true);
        return;
    }
    g.setColour (isMetal() ? juce::Colour (0xff0d0d0f) : pal::bg.darker (0.3f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.5f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 7.0f, false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (isRoyal() ? pal::io.withAlpha (0.35f) : (isMetal() ? juce::Colours::black : pal::line));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    auto arrowArea = r.removeFromRight (26.0f);
    juce::Path tri;
    const auto c = arrowArea.getCentre();
    tri.addTriangle (c.x - 5.0f, c.y - 3.0f, c.x + 5.0f, c.y - 3.0f, c.x, c.y + 3.5f);
    g.setColour (pal::dim);
    g.fillPath (tri);

    g.setColour (isMetal() ? screenText() : pal::text);
    g.setFont (font (14.0f));
    g.drawText (text, r.withTrimmedLeft (26.0f), juce::Justification::centred, true);
}

//==============================================================================
void ABSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    drawBarButtonBase (g, r, false, false);
    const float w = r.getWidth() / 3.0f;
    const juce::String labels[3] { "A", ">", "B" };
    for (int i = 0; i < 3; ++i)
    {
        const auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
        const bool lit = (i == 0 && active == 0) || (i == 2 && active == 1);
        g.setColour (lit ? (isGoliath() ? juce::Colour (0xffe6e6e6) : pal::eq) : (i == 1 ? pal::text.withAlpha (0.85f) : pal::dim));
        g.setFont (isGoliath() ? screenFont (14.0f, 2) : font (14.0f, true));
        g.drawText (labels[i], cell, juce::Justification::centred);
    }
}

void ABSwitch::mouseDown (const juce::MouseEvent& e)
{
    const int cell = juce::jlimit (0, 2, (int) ((float) e.x / (float) getWidth() * 3.0f));
    if (cell == 1) { if (onCopy) onCopy(); }
    else if (onSelect) onSelect (cell == 0 ? 0 : 1);
}

//==============================================================================
// Mishkal mark: an engraved balance scale in a ring (mishkal = weight / balance).
// Gold beam, pans and base; silver post, chains and ring. Drawn on a 120 x 120 grid with
// line weights made heavier than the large logo so it stays crisp at top-bar size.
void vox::ui::drawLogo (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto box = area.withSizeKeepingCentre (d, d);
    const float u = d / 120.0f;
    auto pt = [&] (float x, float y) { return juce::Point<float> (box.getX() + x * u, box.getY() + y * u); };
    auto at = [&] (float x, float y, float w, float h) { return juce::Rectangle<float> (box.getX() + x * u, box.getY() + y * u, w * u, h * u); };

    const juce::Colour goldHi (0xfff7dc7a), gold (0xffe2b23a), goldLo (0xffa87a1c);
    const juce::Colour silverHi (0xfff4f5f7), silverLo (0xff8c8f95);
    auto goldV = [&] (float y0, float y1) { return juce::ColourGradient (goldHi, 0.0f, box.getY() + y0 * u, goldLo, 0.0f, box.getY() + y1 * u, false); };
    auto silverH = [&] (float x0, float x1)
    {
        juce::ColourGradient cg (silverLo, box.getX() + x0 * u, 0.0f, silverLo, box.getX() + x1 * u, 0.0f, false);
        cg.addColour (0.45, silverHi);
        return cg;
    };

    // ring
    {
        juce::ColourGradient cg (silverHi, box.getX(), box.getY(), juce::Colour (0xffe6e7ea), box.getRight(), box.getBottom(), false);
        cg.addColour (0.5, juce::Colour (0xff9ea1a7));
        g.setGradientFill (cg);
        g.drawEllipse (at (7.0f, 7.0f, 106.0f, 106.0f), 6.0f * u);
    }

    // chains
    g.setColour (juce::Colour (0xffc9ccd1));
    for (float cx : { 23.0f, 97.0f })
        for (float dx : { -10.0f, 10.0f })
            g.drawLine ({ pt (cx, 42.0f), pt (cx + dx, 67.0f) }, 2.6f * u);

    // pans
    for (float cx : { 23.0f, 97.0f })
    {
        juce::Path pan;
        pan.startNewSubPath (pt (cx - 12.5f, 66.0f));
        pan.lineTo (pt (cx + 12.5f, 66.0f));
        pan.quadraticTo (pt (cx + 11.5f, 77.0f), pt (cx, 79.0f));
        pan.quadraticTo (pt (cx - 11.5f, 77.0f), pt (cx - 12.5f, 66.0f));
        pan.closeSubPath();
        g.setGradientFill (goldV (66.0f, 79.0f));
        g.fillPath (pan);
    }

    // post (tapered, silver) and base
    {
        juce::Path post;
        post.startNewSubPath (pt (57.0f, 36.0f));
        post.lineTo (pt (63.0f, 36.0f));
        post.lineTo (pt (64.0f, 84.0f));
        post.lineTo (pt (56.0f, 84.0f));
        post.closeSubPath();
        g.setGradientFill (silverH (56.0f, 64.0f));
        g.fillPath (post);
        g.setGradientFill (silverH (50.0f, 70.0f));
        g.fillRoundedRectangle (at (50.0f, 83.0f, 20.0f, 4.5f), 1.0f * u);
        juce::Path base;
        base.startNewSubPath (pt (42.0f, 93.0f));
        base.quadraticTo (pt (42.0f, 87.0f), pt (48.0f, 87.0f));
        base.lineTo (pt (72.0f, 87.0f));
        base.quadraticTo (pt (78.0f, 87.0f), pt (78.0f, 93.0f));
        base.closeSubPath();
        g.setGradientFill (goldV (87.0f, 93.0f));
        g.fillPath (base);
    }

    // beam: gentle arch with ball ends
    {
        juce::Path beamPath;
        beamPath.startNewSubPath (pt (23.0f, 39.0f));
        beamPath.quadraticTo (pt (60.0f, 35.0f), pt (97.0f, 39.0f));
        beamPath.lineTo (pt (97.0f, 44.0f));
        beamPath.quadraticTo (pt (60.0f, 40.0f), pt (23.0f, 44.0f));
        beamPath.closeSubPath();
        juce::ColourGradient cg (juce::Colour (0xffb8892a), box.getX() + 23.0f * u, 0.0f, juce::Colour (0xffb8892a), box.getX() + 97.0f * u, 0.0f, false);
        cg.addColour (0.5, goldHi);
        g.setGradientFill (cg);
        g.fillPath (beamPath);
        g.setGradientFill (goldV (37.0f, 45.0f));
        g.fillEllipse (at (19.0f, 38.0f, 7.0f, 7.0f));
        g.fillEllipse (at (94.0f, 38.0f, 7.0f, 7.0f));
        g.fillEllipse (at (55.5f, 34.0f, 9.0f, 9.0f));   // pivot
    }

    // finial
    g.setGradientFill (juce::ColourGradient (silverHi, 0.0f, box.getY() + 24.0f * u, juce::Colour (0xff9a9da3), 0.0f, box.getY() + 33.0f * u, false));
    g.fillEllipse (at (56.0f, 24.5f, 8.0f, 8.0f));
    juce::Path tip;
    tip.startNewSubPath (pt (60.0f, 15.5f));
    tip.lineTo (pt (62.5f, 21.0f));
    tip.lineTo (pt (60.0f, 23.0f));
    tip.lineTo (pt (57.5f, 21.0f));
    tip.closeSubPath();
    g.setColour (gold);
    g.fillPath (tip);
}

//==============================================================================
void VoxStripEditor::mouseDown (const juce::MouseEvent&)
{
    proc.undoManager.beginNewTransaction();
}

void VoxStripEditor::refreshToolbar()
{
    presetBox.setText (proc.presets.getDisplayName());
   #if MISHKAL_V67
    presetBox.setStar (isGoliath(), proc.presets.isFavourite (proc.presets.getCurrentName()));
   #endif
    undoBtn.setEnabled (proc.undoManager.canUndo());
    redoBtn.setEnabled (proc.undoManager.canRedo());
    abSwitch.setActive (proc.presets.getActiveSlot());
}

void VoxStripEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const auto factory = proc.presets.getFactoryNames();
    const auto user = proc.presets.getUserNames();
    const auto current = proc.presets.getDisplayName().upToLastOccurrenceOf (" *", false, false);
   #if MISHKAL_V67
    // V67: search, favourites, and how the arrows step
    m.addItem (5000, "Search...");
    {
        const auto favs = proc.presets.getFavourites();
        juce::PopupMenu fm;
        for (int i = 0; i < favs.size(); ++i)
        {
            const int fi = factory.indexOf (favs[i]), ui = user.indexOf (favs[i]);
            if (fi >= 0)      fm.addItem (1 + fi, favs[i], true, favs[i] == current);
            else if (ui >= 0) fm.addItem (1000 + ui, favs[i], true, favs[i] == current);
        }
        if (fm.getNumItems() == 0) fm.addItem (-2, "(click the star next to a preset's name)", false);
        m.addSubMenu (juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85 Favourites")), fm);
    }
    m.addItem (5001, "Arrows step inside this folder", true, (bool) proc.apvts.state.getProperty ("stepFolder", false));
    m.addSeparator();
   #endif

    // Factory presets, grouped by category (uncategorised ones such as Default first)
    m.addSectionHeader ("Factory");
    juce::StringArray cats;
    for (int i = 0; i < factory.size(); ++i)
    {
        const auto c = proc.presets.getFactoryCategory (i);
        if (c.isEmpty()) m.addItem (1 + i, factory[i], true, factory[i] == current);
        else cats.addIfNotAlreadyThere (c);
    }
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        bool holdsCurrent = false;
        for (int i = 0; i < factory.size(); ++i)
            if (proc.presets.getFactoryCategory (i) == c)
            {
                sub.addItem (1 + i, factory[i], true, factory[i] == current);
                holdsCurrent = holdsCurrent || factory[i] == current;
            }
        m.addSubMenu (c, sub, true, nullptr, holdsCurrent);
    }

    // User presets: loose ones, then each category folder as a submenu
    m.addSectionHeader ("User");
    if (user.isEmpty())
        m.addItem (-1, "(none saved yet)", false);
    juce::StringArray folders;
    for (int i = 0; i < user.size(); ++i)
    {
        if (user[i].containsChar ('/')) folders.addIfNotAlreadyThere (user[i].upToFirstOccurrenceOf ("/", false, false));
        else m.addItem (1000 + i, user[i], true, user[i] == current);
    }
    for (auto& f : folders)
    {
        juce::PopupMenu sub;
        bool holdsCurrent = false;
        for (int i = 0; i < user.size(); ++i)
            if (user[i].upToFirstOccurrenceOf ("/", false, false) == f && user[i].containsChar ('/'))
            {
                sub.addItem (1000 + i, user[i].fromFirstOccurrenceOf ("/", false, false), true, user[i] == current);
                holdsCurrent = holdsCurrent || user[i] == current;
            }
        m.addSubMenu (f, sub, true, nullptr, holdsCurrent);
    }

    juce::Component::SafePointer<VoxStripEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBox),
        [safe, factory, user] (int r)
        {
            if (safe == nullptr || r <= 0) return;
           #if MISHKAL_V67
            if (r == 5000) { safe->searchPresets(); return; }
            if (r == 5001) { auto& st = safe->proc.apvts.state; st.setProperty ("stepFolder", ! (bool) st.getProperty ("stepFolder", false), nullptr); return; }
           #endif
            safe->proc.undoManager.beginNewTransaction();
            if (r < 1000) safe->proc.presets.loadFactory (r - 1);
            else          safe->proc.presets.loadUser (user[r - 1000]);
            safe->refreshToolbar();
        });
}

// V67: type a few letters, pick from everything that matches (name or folder)
void VoxStripEditor::searchPresets()
{
    auto* w = new juce::AlertWindow ("Find a preset", "Type part of a name or a folder (e.g. \"rap\", \"master\", \"esh\"):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("q", {}, {});
    w->addButton ("Find", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<VoxStripEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        const auto q = w->getTextEditorContents ("q").trim();
        if (safe == nullptr || r == 0 || q.isEmpty()) return;
        const auto factory = safe->proc.presets.getFactoryNames();
        const auto user = safe->proc.presets.getUserNames();
        juce::PopupMenu m;
        m.addSectionHeader ("Matches for \"" + q + "\"");
        for (int i = 0; i < factory.size(); ++i)
            if (factory[i].containsIgnoreCase (q) || safe->proc.presets.getFactoryCategory (i).containsIgnoreCase (q))
                m.addItem (1 + i, factory[i] + "   (" + safe->proc.presets.getFactoryCategory (i) + ")");
        for (int i = 0; i < user.size(); ++i)
            if (user[i].containsIgnoreCase (q)) m.addItem (1000 + i, user[i] + "   (user)");
        if (m.getNumItems() <= 1) m.addItem (-1, "(nothing found)", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&safe->presetBox), [safe, user] (int pick)
        {
            if (safe == nullptr || pick <= 0) return;
            safe->proc.undoManager.beginNewTransaction();
            if (pick < 1000) safe->proc.presets.loadFactory (pick - 1);
            else             safe->proc.presets.loadUser (user[pick - 1000]);
            safe->refreshToolbar();
        });
    }), true);
}

void VoxStripEditor::savePreset (bool askName)
{
    if (! askName && proc.presets.isUserPreset())
    {
        proc.presets.saveUser (proc.presets.getDisplayName().upToLastOccurrenceOf (" *", false, false));
        refreshToolbar();
        return;
    }

    auto* w = new juce::AlertWindow ("Save Preset", "Name your preset (and optionally a category to file it under):", juce::MessageBoxIconType::NoIcon, this);
    const auto currentFull = proc.presets.getDisplayName().upToLastOccurrenceOf (" *", false, false);
    const bool user = proc.presets.isUserPreset();
    w->addTextEditor ("name", user ? (currentFull.containsChar ('/') ? currentFull.fromLastOccurrenceOf ("/", false, false) : currentFull)
                                   : juce::String ("My Vocal"), "Name");
    w->addTextEditor ("category", user && currentFull.containsChar ('/') ? currentFull.upToLastOccurrenceOf ("/", false, false) : juce::String(),
                      "Category (optional)");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<VoxStripEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe != nullptr && result == 1)
        {
            const auto cat = w->getTextEditorContents ("category").trim();
            safe->proc.presets.saveUser ((cat.isNotEmpty() ? cat + "/" : juce::String()) + w->getTextEditorContents ("name"));
            safe->refreshToolbar();
        }
    }), true);
}

void VoxStripEditor::showMainMenu()
{
    juce::PopupMenu skins;
    for (int i = 0; i < skinNames.size(); ++i)
        skins.addItem (100 + i, skinNames[i], true, (int) pal::style == i);

    juce::PopupMenu sizes;
    for (int i = 0; i < (int) uiScales.size(); ++i)
        sizes.addItem (200 + i, juce::String (juce::roundToInt (uiScales[(size_t) i] * 100.0f)) + "%",
                       true, std::abs (uiScales[(size_t) i] - uiScale) < 0.01f);

    juce::PopupMenu m;
    m.addSubMenu ("Size", sizes);
    if (! MISHKAL_V56)   // V56: Goliath only - no skin menu
        m.addSubMenu ("Skin", skins);
    m.addSeparator();
    m.addItem (3, "Keep Input/Output when changing presets", true, proc.presets.isIoLocked());
    m.addItem (1, "Reset to Default");
    m.addItem (2, "Open Presets Folder");

    juce::Component::SafePointer<VoxStripEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuBtn),
        [safe] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r >= 200) safe->setUiScale (uiScales[(size_t) juce::jlimit (0, (int) uiScales.size() - 1, r - 200)]);
            else if (r >= 100) safe->setSkin (r - 100);
            else if (r == 1) { safe->proc.undoManager.beginNewTransaction(); safe->proc.presets.loadFactory (0); }
            else if (r == 3) safe->proc.presets.setIoLocked (! safe->proc.presets.isIoLocked());
            else if (r == 2)
            {
                auto dir = PresetManager::getUserDirectory();
                dir.createDirectory();
                dir.revealToUser();
            }
            safe->refreshToolbar();
        });
}
