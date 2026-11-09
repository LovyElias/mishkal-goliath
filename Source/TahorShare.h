#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <map>

// V19: the instance list (like Pro-Q 4) - every Tahor in the session publishes its output spectrum here, with its track's
// name and colour, so any Tahor can draw the others behind its own and show where they clash.
// One hub per loaded plug-in (all the instances a host runs in one process share it).
namespace tahor
{
class ShareHub
{
public:
    static constexpr int bins = 4097;   // an 8192-point FFT, the same as the editor's analyzer

    struct Snap
    {
        int id = 0;
        juce::String name;
        juce::Colour colour;   // the track's colour (transparent if the host doesn't say)
        double rate = 48000.0;
        std::vector<float> db;
        bool live = false;     // it has published something in the last second
    };

    static ShareHub& get() { static ShareHub h; return h; }

    int join()
    {
        const juce::ScopedLock sl (lock);
        const int id = next++;
        auto& s = slots[id];
        s.id = id; s.name = "Tahor " + juce::String (id);
        return id;
    }
    void leave (int id) { const juce::ScopedLock sl (lock); slots.erase (id); }
    int count() const { const juce::ScopedLock sl (lock); return (int) slots.size(); }

    void setInfo (int id, const juce::String& name, juce::Colour c)
    {
        const juce::ScopedLock sl (lock);
        auto it = slots.find (id);
        if (it == slots.end()) return;
        if (name.isNotEmpty()) it->second.name = name;
        it->second.colour = c;
    }

    void publish (int id, const std::vector<float>& db, double rate)
    {
        const juce::ScopedLock sl (lock);
        auto it = slots.find (id);
        if (it == slots.end()) return;
        it->second.db = db; it->second.rate = rate;
        it->second.stamp = juce::Time::getMillisecondCounter();
    }

    // everyone but `self` (newest data copied out)
    std::vector<Snap> others (int self) const
    {
        const juce::ScopedLock sl (lock);
        std::vector<Snap> out;
        const auto now = juce::Time::getMillisecondCounter();
        for (auto& [id, s] : slots)
        {
            if (id == self) continue;
            Snap n; n.id = id; n.name = s.name; n.colour = s.colour; n.rate = s.rate; n.db = s.db;
            n.live = ! s.db.empty() && now - s.stamp < 1000;
            out.push_back (std::move (n));
        }
        return out;
    }

    // someone is looking at the others (so the instances bother to publish)
    void want (int id) { const juce::ScopedLock sl (lock); wantUntil = juce::Time::getMillisecondCounter() + 2000; juce::ignoreUnused (id); }
    bool wanted() const { const juce::ScopedLock sl (lock); return juce::Time::getMillisecondCounter() < wantUntil; }

private:
    struct Slot { int id = 0; juce::String name; juce::Colour colour; double rate = 48000.0; std::vector<float> db; juce::uint32 stamp = 0; };
    juce::CriticalSection lock;
    std::map<int, Slot> slots;
    int next = 1;
    juce::uint32 wantUntil = 0;
};

// each instance's own analyzer for the hub: pulls its output, runs the same FFT and smoothing as the editor's
class ShareAnalyzer
{
public:
    ShareAnalyzer() : fft (13)
    {
        hann.resize (8192);
        for (int i = 0; i < 8192; ++i) hann[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 8191.0f);
        work.assign (16384, 0.0f); smooth.assign ((size_t) ShareHub::bins, -120.0f);
    }
    // message thread: `win` is the latest 8192 samples
    const std::vector<float>& update (const std::vector<float>& win)
    {
        const float norm = 2.0f / (8192.0f * 0.5f);
        for (int i = 0; i < 8192; ++i) work[(size_t) i] = win[(size_t) i] * hann[(size_t) i];
        std::fill (work.begin() + 8192, work.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (work.data(), true);
        for (int k = 0; k < ShareHub::bins; ++k)
        {
            const float d = juce::Decibels::gainToDecibels (work[(size_t) k] * norm, -140.0f);
            auto& s = smooth[(size_t) k];
            s = d > s ? 0.5f * s + 0.5f * d : 0.86f * s + 0.14f * d;
        }
        return smooth;
    }
private:
    juce::dsp::FFT fft;
    std::vector<float> hann, work, smooth;
};
} // namespace tahor
