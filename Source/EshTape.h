#pragma once
// Esh (Hebrew: "fire") - a character tape saturator: pick a character, set how hard it works (Process),
// tilt the result Warm / Flat / Bright, and trim the level in and out.
//
// Signal path per channel:
//   input trim -> character EQ before the tape (head bump, low-mid weight, pre-emphasis)
//   -> gentle program "glue" (a soft, slow level control on the hottest moments)
//   -> tape-style saturation (soft, slightly asymmetric: mostly low-order harmonics)
//   -> de-emphasis (so hot highs are rounded off: louder, smoother, less sibilant)
//   -> tone (Warm / Flat / Bright) -> output trim.
// Pushing Process drives the tape harder; the level is roughly made up so you hear density, not just volume.

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace vox
{

struct EshSettings
{
    bool  on = false;
    int   character = 0;        // 0 Clear, 1 Warm, 2 Round, 3 Glow, 4 Ember
    float process = 40.0f;      // 0..100
    int   tone = 1;             // 0 Warm, 1 Flat, 2 Bright
    float inDb = 0.0f;          // -10..+10
    float outDb = 0.0f;         // -6..+6
};

struct EshCharacter
{
    const char* name;
    float maxDriveDb;      // extra drive at Process 100
    float bias;            // asymmetry (even harmonics)
    float preEmphDb;       // highs pushed into the tape and taken back out after (rounds hot highs)
    float headBumpDb;      // low-end bump around 90 Hz at Process 100
    float lowMidDb;        // low-mid weight around 260 Hz at Process 100
    float glue;            // 0..1 program level control
    float sheenDb;         // extra top after the tape (+ brighter, louder; - darker, smoother)
};

inline const EshCharacter& eshCharacter (int c)
{
    static const EshCharacter table[5] {
        { "Clear", 12.0f, 0.02f, 2.0f, 0.6f, 0.0f, 0.0f, 0.0f },   // natural, well-aligned tape: fuller, still open
        { "Warm",  13.0f, 0.05f, 2.0f, 1.8f, 1.0f, 0.0f, 0.0f },   // more low end, thicker low-mids
        { "Round", 14.0f, 0.06f, 3.0f, 1.8f, 1.0f, 0.5f, 0.0f },   // warm plus mid-range glue
        { "Glow",  15.0f, 0.04f, 4.0f, 1.0f, 0.3f, 0.3f, 2.5f },    // brighter and louder
        { "Ember", 18.0f, 0.08f, 10.0f, 1.2f, 0.6f, 0.6f, -3.5f } }; // loudest: hot highs and sibilance rounded smoothly
    return table[juce::jlimit (0, 4, c)];
}

class EshTape
{
public:
    static constexpr int numCharacters = 5;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto* c : { &bump, &lowMid, &pre, &de, &tilt, &sheen })
        {
            *c = new juce::dsp::IIR::Coefficients<float> (juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sr, 1000.0, 1.0, 1.0f));
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            fBump[ch].coefficients = bump;   fLowMid[ch].coefficients = lowMid;
            fPre[ch].coefficients = pre;     fDe[ch].coefficients = de;
            fTilt[ch].coefficients = tilt;   fSheen[ch].coefficients = sheen;
        }
        lastKey = -1.0f;
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            for (auto* f : { &fBump[ch], &fLowMid[ch], &fPre[ch], &fDe[ch], &fTilt[ch], &fSheen[ch] }) f->reset();
            dcX[ch] = dcY[ch] = 0.0f;
        }
        env = 0.0f;
        level = 0.0f;
    }

    // returns the output level (RMS dBFS) for the VU meter
    float process (juce::AudioBuffer<float>& buffer, const EshSettings& s)
    {
        const int n = buffer.getNumSamples();
        const int nc = juce::jmin (2, buffer.getNumChannels());
        if (! s.on || n == 0) return -100.0f;

        const auto& ch = eshCharacter (s.character);
        const float amt = juce::jlimit (0.0f, 100.0f, s.process) / 100.0f;
        updateFilters (s, ch, amt);

        const float inGain = juce::Decibels::decibelsToGain (juce::jlimit (-10.0f, 10.0f, s.inDb));
        const float drive = juce::Decibels::decibelsToGain (ch.maxDriveDb * amt);
        const float bias = ch.bias * (0.4f + 0.6f * amt);
        const float tb = std::tanh (bias);
        const float makeup = 1.0f / std::sqrt (drive);   // keeps most of the level steady; some added loudness remains
        const float outGain = juce::Decibels::decibelsToGain (juce::jlimit (-6.0f, 6.0f, s.outDb));
        const float glue = ch.glue * amt;
        const float envA = std::exp (-1.0f / (0.005f * (float) sr)), envR = std::exp (-1.0f / (0.25f * (float) sr));
        const float dcR = (float) (1.0 - juce::MathConstants<double>::twoPi * 5.0 / sr);
        double sum = 0.0;

        for (int i = 0; i < n; ++i)
        {
            float pk = 0.0f;
            for (int c = 0; c < nc; ++c) pk = juce::jmax (pk, std::abs (buffer.getSample (c, i) * inGain));
            env = pk > env ? envA * env + (1.0f - envA) * pk : envR * env + (1.0f - envR) * pk;
            // glue: above about -20 dBFS, ease the level down (2:1-ish, at most 6 dB)
            const float overDb = juce::Decibels::gainToDecibels (env, -120.0f) + 20.0f;
            const float glueGain = glue > 0.0f && overDb > 0.0f ? juce::Decibels::decibelsToGain (-juce::jmin (6.0f, overDb * 0.5f * glue)) : 1.0f;

            for (int c = 0; c < nc; ++c)
            {
                float x = buffer.getSample (c, i) * inGain;
                x = fBump[c].processSample (x);
                x = fLowMid[c].processSample (x);
                x = fPre[c].processSample (x) * glueGain;
                // tape-like saturation: soft, slightly asymmetric
                const float d = x * drive;
                float y = (std::tanh (d + bias) - tb) / (1.0f - tb * tb);
                y *= makeup;
                // remove any DC the asymmetry made
                const float dcOut = y - dcX[c] + dcR * dcY[c];
                dcX[c] = y; dcY[c] = dcOut;
                y = fDe[c].processSample (dcOut);
                y = fSheen[c].processSample (y);
                y = fTilt[c].processSample (y) * outGain;
                buffer.setSample (c, i, y);
                sum += (double) y * y;
            }
        }
        const float rmsDb = (float) (10.0 * std::log10 (sum / (double) (n * juce::jmax (1, nc)) + 1.0e-20));
        level = rmsDb;
        return rmsDb;
    }

private:
    void updateFilters (const EshSettings& s, const EshCharacter& ch, float amt)
    {
        const float key = (float) s.character * 1000.0f + std::round (amt * 200.0f) + (float) s.tone * 100000.0f;
        if (key == lastKey) return;
        lastKey = key;
        using AC = juce::dsp::IIR::ArrayCoefficients<float>;
        auto db = [] (float d) { return juce::Decibels::decibelsToGain (d); };
        const float pe = ch.preEmphDb * (0.3f + 0.7f * amt);
        *bump   = AC::makePeakFilter (sr, 90.0, 0.9, db (ch.headBumpDb * amt));
        *lowMid = AC::makePeakFilter (sr, 260.0, 0.8, db (ch.lowMidDb * amt));
        *pre    = AC::makeHighShelf (sr, 3500.0, 0.6, db (pe));
        *de     = AC::makeHighShelf (sr, 3500.0, 0.6, db (-pe));
        *sheen  = AC::makeHighShelf (sr, ch.sheenDb < 0.0f ? 6500.0 : 9000.0, 0.7, db (ch.sheenDb * amt));
        const float tiltDb = s.tone == 0 ? -2.0f : s.tone == 2 ? 2.0f : 0.0f;
        *tilt   = AC::makeHighShelf (sr, 6000.0, 0.6, db (tiltDb));
    }

    double sr = 48000.0;
    float lastKey = -1.0f, env = 0.0f, level = 0.0f;
    juce::dsp::IIR::Coefficients<float>::Ptr bump, lowMid, pre, de, tilt, sheen;
    std::array<juce::dsp::IIR::Filter<float>, 2> fBump, fLowMid, fPre, fDe, fTilt, fSheen;
    std::array<float, 2> dcX {}, dcY {};
};

} // namespace vox
