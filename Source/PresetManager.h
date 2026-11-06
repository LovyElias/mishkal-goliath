#pragma once

// Presets (factory + user), A/B compare. Presets set parameter values only,
// so loading one never changes the skin and is undoable.

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <map>
#include <vector>

namespace vox
{

class PresetManager : private juce::AudioProcessorParameter::Listener
{
public:
    using Values = std::vector<std::pair<juce::String, float>>;   // param id -> real value
    struct Factory { juce::String name; Values values; juce::String category = {}; };

    // Never changed by presets or A/B: listening / monitoring switches, not part of a sound
    static bool isSessionOnly (const juce::String& id)
    {
        return id == "masterBypass" || id == "levelMatch" || id == "trackMode"
            || id == "tapeOs" || id == "limOs" || id == "globalOs" || id == "cleanOs" || id == "cleanPhase";   // quality settings that change latency
    }
    // "Keep Input/Output when changing presets" (main menu); remembered with the session
    bool isIoLocked() const { return (bool) state.state.getProperty ("lockIO", false); }
    void setIoLocked (bool b) { state.state.setProperty ("lockIO", b, nullptr); }
    bool isKept (const juce::String& id) const
    {
        return isSessionOnly (id) || (isIoLocked() && (id == "inputGain" || id == "outputGain"));
    }

    explicit PresetManager (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        for (auto* p : state.processor.getParameters())
            p->addListener (this);
        currentName = "Default";
    }

    ~PresetManager() override
    {
        for (auto* p : state.processor.getParameters())
            p->removeListener (this);
    }

    void setFactoryPresets (std::vector<Factory> f) { factory = std::move (f); }
    void setFactoryCategory (const juce::String& name, const juce::String& category)
    {
        for (auto& f : factory) if (f.name == name) f.category = category;
    }

    juce::StringArray getFactoryNames() const
    {
        juce::StringArray n;
        for (auto& f : factory) n.add (f.name);
        return n;
    }
    const Values& getFactoryValues (int i) const
    {
        static const Values none;
        return juce::isPositiveAndBelow (i, (int) factory.size()) ? factory[(size_t) i].values : none;
    }
    juce::String getFactoryCategory (int i) const
    {
        return juce::isPositiveAndBelow (i, (int) factory.size()) ? factory[(size_t) i].category : juce::String();
    }

    static juce::File getUserDirectory()
    {
       #if JUCE_MAC
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support");
       #else
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #endif
        // Renamed from "Mishkal Vocal Strip": carry existing user presets over once
        auto dir = base.getChildFile ("Mishkal Goliath Strip");
        auto old = base.getChildFile ("Mishkal Vocal Strip");
        if (! dir.exists() && old.isDirectory())
            old.moveFileTo (dir);
        return dir.getChildFile ("Presets");
    }

    // User presets; ones saved into a category folder come back as "Category/Name"
    juce::StringArray getUserNames() const
    {
        juce::StringArray n;
        const auto dir = getUserDirectory();
        for (auto& f : dir.findChildFiles (juce::File::findFiles, true, "*.mishkalpreset"))
            n.add (f.withFileExtension ({}).getRelativePathFrom (dir).replaceCharacter ('\\', '/'));
        n.sort (true);
        return n;
    }

    // All presets in browse order: factory first, then user
    juce::StringArray getAllNames() const
    {
        auto all = getFactoryNames();
        all.addArray (getUserNames());
        return all;
    }

    void loadFactory (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) factory.size())) return;
        resetToDefaults();
        applyValues (factory[(size_t) index].values);
        finishLoad (factory[(size_t) index].name, false);
    }

    bool loadUser (const juce::String& name)
    {
        auto xml = juce::XmlDocument::parse (getUserDirectory().getChildFile (name + ".mishkalpreset"));
        if (xml == nullptr) return false;

        resetToDefaults();
        Values v;
        for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
            v.push_back ({ e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("value") });
        applyValues (v);
        finishLoad (name, true);
        return true;
    }

    // rawName may be "Category/Name" to file it in a category folder
    bool saveUser (const juce::String& rawName)
    {
        juce::StringArray parts;
        parts.addTokens (rawName, "/", "");
        parts.trim(); parts.removeEmptyStrings();
        for (auto& p : parts) p = juce::File::createLegalFileName (p);
        parts.removeEmptyStrings();
        const auto name = parts.joinIntoString ("/");
        if (name.isEmpty()) return false;

        juce::XmlElement xml ("VOXPRESET");
        for (auto* p : state.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                if (isSessionOnly (rp->getParameterID())) continue;
                auto* e = xml.createNewChildElement ("PARAM");
                e->setAttribute ("id", rp->getParameterID());
                e->setAttribute ("value", rp->convertFrom0to1 (rp->getValue()));
            }

        auto file = getUserDirectory().getChildFile (name + ".mishkalpreset");
        file.getParentDirectory().createDirectory();
        if (! xml.writeTo (file)) return false;

        currentName = name;
        currentIsUser = true;
        dirty = false;
        state.state.setProperty ("presetName", currentName, nullptr);
        return true;
    }

    void load (const juce::String& name)
    {
        const int f = getFactoryNames().indexOf (name);
        if (f >= 0) loadFactory (f); else loadUser (name);
    }

    void step (int delta)
    {
        const auto all = getAllNames();
        if (all.isEmpty()) return;
        int i = all.indexOf (currentName);
        i = (i < 0 ? 0 : (i + delta + all.size()) % all.size());
        load (all[i]);
    }

    // V67: favourites (shared by every session) and stepping inside the current folder ----------------------
    static juce::File getFavouritesFile() { return getUserDirectory().getParentDirectory().getChildFile ("favourites.txt"); }
    juce::StringArray getFavourites() const
    {
        juce::StringArray f;
        f.addLines (getFavouritesFile().loadFileAsString());
        f.removeEmptyStrings();
        return f;
    }
    bool isFavourite (const juce::String& name) const { return name.isNotEmpty() && getFavourites().contains (name); }
    void toggleFavourite (const juce::String& name)
    {
        if (name.isEmpty()) return;
        auto f = getFavourites();
        if (f.contains (name)) f.removeString (name); else f.add (name);
        getFavouritesFile().getParentDirectory().createDirectory();
        getFavouritesFile().replaceWithText (f.joinIntoString ("\n"));
    }
    juce::String getCurrentName() const { return currentName; }

    // the folder a preset lives in: its factory category, or a user preset's sub-folder ("" = loose / Default)
    juce::String folderOf (const juce::String& name) const
    {
        const int f = getFactoryNames().indexOf (name);
        if (f >= 0) return "F:" + getFactoryCategory (f);
        return "U:" + (name.containsChar ('/') ? name.upToFirstOccurrenceOf ("/", false, false) : juce::String());
    }
    // step = all presets, or only the ones in the current preset's folder
    void stepInFolder (int delta)
    {
        const auto all = getAllNames();
        const auto folder = folderOf (currentName);
        juce::StringArray same;
        for (auto& n : all) if (folderOf (n) == folder) same.add (n);
        if (same.isEmpty()) { step (delta); return; }
        int i = same.indexOf (currentName);
        i = (i < 0 ? 0 : (i + delta + same.size()) % same.size());
        load (same[i]);
    }

    juce::String getDisplayName() const { return currentName + (dirty.load() ? " *" : ""); }
    bool isUserPreset() const { return currentIsUser; }

    void restoreName()   // after the host restores a session
    {
        currentName = state.state.getProperty ("presetName", "Default").toString();
        currentIsUser = getFactoryNames().indexOf (currentName) < 0;
    }

    //==========================================================================
    // A/B compare: two snapshots of every parameter
    int getActiveSlot() const { return activeSlot; }

    void selectSlot (int slot)
    {
        if (slot == activeSlot) return;
        slots[activeSlot] = snapshot();
        activeSlot = slot;
        if (! slots[activeSlot].empty())
            applySnapshot (slots[activeSlot]);
        else
            slots[activeSlot] = snapshot();   // first visit: B starts as a copy of A
    }

    void copyActiveToOther()
    {
        slots[1 - activeSlot] = snapshot();
    }

private:
    void parameterValueChanged (int, float) override { if (! loading) dirty = true; }
    void parameterGestureChanged (int, bool) override {}

    void resetToDefaults()
    {
        for (auto* p : state.processor.getParameters())
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (isKept (rp->getParameterID())) continue;
            p->setValueNotifyingHost (p->getDefaultValue());
        }
    }

    void applyValues (const Values& v)
    {
        loading = true;
        for (auto& [pid, value] : v)
            if (! isKept (pid))
                if (auto* p = state.getParameter (pid))
                    p->setValueNotifyingHost (p->convertTo0to1 (value));
        loading = false;
    }

    void finishLoad (const juce::String& name, bool user)
    {
        currentName = name;
        currentIsUser = user;
        dirty = false;
        state.state.setProperty ("presetName", currentName, nullptr);
    }

    std::map<juce::String, float> snapshot() const
    {
        std::map<juce::String, float> m;
        for (auto* p : state.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                m[rp->getParameterID()] = rp->getValue();
        return m;
    }

    void applySnapshot (const std::map<juce::String, float>& m)
    {
        for (auto& [pid, norm] : m)
            if (! isSessionOnly (pid))   // A/B never flips TRACK, MATCH or BYPASS
                if (auto* p = state.getParameter (pid))
                    p->setValueNotifyingHost (norm);
    }

    juce::AudioProcessorValueTreeState& state;
    std::vector<Factory> factory;
    juce::String currentName;
    bool currentIsUser = false;
    std::atomic<bool> dirty { false };
    bool loading = false;
    int activeSlot = 0;
    std::map<juce::String, float> slots[2];
};

} // namespace vox
