// Offline checks for the Mishkal DSP. Build with -DVOX_BUILD_TESTS=ON.
#include "VocalChain.h"
#include <cstdio>

using namespace vox;

static constexpr double sr = 48000.0;
static constexpr int block = 512;
static int failures = 0;

static void check (bool ok, const char* what, double value)
{
    std::printf ("%s  %-58s %8.2f\n", ok ? "PASS" : "FAIL", what, value);
    if (! ok) ++failures;
}

// Makes only character EQ type t active, with settings e
static void use (ChainSettings& s, EqType t, CharEqSettings e)
{
    s.eqOn = true;
    for (auto& x : s.charEq) x.on = false;
    e.on = true;
    s.charEq[(size_t) t] = e;
}

static ChainSettings allOff()
{
    ChainSettings s;
    s.eqOn = s.compOn = s.dsOn = s.limOn = false;
    s.reso.on = false;
    s.flowOn = false;
    return s;
}

// Runs a sine through a fresh chain; returns output peak (dB) over the last half
static double runSine (const ChainSettings& s, double hz, float ampDb, double seconds = 1.0,
                       float* compGr = nullptr)
{
    VocalChain chain;
    chain.prepare (sr, block);
    const float amp = juce::Decibels::decibelsToGain (ampDb);
    const int total = (int) (seconds * sr);
    double phase = 0.0, peak = 0.0;
    juce::AudioBuffer<float> buf (2, block);

    for (int pos = 0; pos < total; pos += block)
    {
        for (int i = 0; i < block; ++i)
        {
            const float v = amp * (float) std::sin (phase);
            phase += juce::MathConstants<double>::twoPi * hz / sr;
            buf.setSample (0, i, v); buf.setSample (1, i, v);
        }
        chain.process (buf, s);
        if (pos > total / 2)
            peak = juce::jmax (peak, (double) buf.getMagnitude (0, 0, block));
        if (compGr) *compGr = chain.meters.compGr.load();
    }
    return juce::Decibels::gainToDecibels (peak, -200.0);
}

// Level (dB re full scale) of one frequency in a signal, via Goertzel
static double toneDb (const std::vector<float>& x, double hz)
{
    const double w = juce::MathConstants<double>::twoPi * hz / sr, cw = 2.0 * std::cos (w);
    double s1 = 0.0, s2 = 0.0;
    for (auto v : x) { const double s0 = v + cw * s1 - s2; s2 = s1; s1 = s0; }
    const double power = s1 * s1 + s2 * s2 - cw * s1 * s2;
    return juce::Decibels::gainToDecibels (2.0 * std::sqrt (power) / (double) x.size(), -200.0);
}

// Renders a sine through the chain and returns the second half of the output
static std::vector<float> renderSine (const ChainSettings& s, double hz, float ampDb)
{
    VocalChain chain; chain.prepare (sr, block);
    const float amp = juce::Decibels::decibelsToGain (ampDb);
    std::vector<float> out;
    juce::AudioBuffer<float> buf (2, block);
    double phase = 0.0;
    for (int pos = 0; pos < (int) sr; pos += block)
    {
        for (int i = 0; i < block; ++i)
        {
            const float v = amp * (float) std::sin (phase);
            phase += juce::MathConstants<double>::twoPi * hz / sr;
            buf.setSample (0, i, v); buf.setSample (1, i, v);
        }
        chain.process (buf, s);
        if (pos >= (int) sr / 2)
            for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
    }
    return out;
}

int main()
{
    // 1. Everything bypassed = input delayed by the reported latency, bit for bit
    {
        VocalChain chain; chain.prepare (sr, block);
        const int lat = chain.getLatencySamples();
        auto s = allOff();
        juce::Random rng (1);
        std::vector<float> in (sr), out (sr);
        for (auto& v : in) v = rng.nextFloat() * 2.0f - 1.0f;
        juce::AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos + block <= (int) in.size(); pos += block)
        {
            for (int i = 0; i < block; ++i) { buf.setSample (0, i, in[(size_t) (pos + i)]); buf.setSample (1, i, in[(size_t) (pos + i)]); }
            chain.process (buf, s);
            for (int i = 0; i < block; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
        }
        double err = 0.0;
        for (size_t i = (size_t) lat + 2048; i < 40000; ++i) err = juce::jmax (err, (double) std::abs (out[i] - in[i - (size_t) lat]));
        check (err < 1e-5, "Bypassed chain nulls against input (max error)", err);
        check (lat == 12 + 1024 + 4 + 72, "Latency at 48 kHz: peak catch + reso + tape + limiter", lat);
    }

    // 2. Limiter never passes the ceiling, even driven hard with transients,
    //    in both Classic and Multi modes, with and without ARC
    for (int mode = 0; mode < 4; ++mode)
    for (float ceilingDb : { -1.0f, -3.0f, -0.1f })
    {
        auto s = allOff(); s.limOn = true; s.limCeilingDb = ceilingDb; s.limThreshDb = -12.0f; s.limReleaseMs = 20.0f;
        s.limMode = (LimMode) (mode % 2);
        s.limArc = mode >= 2;
        VocalChain chain; chain.prepare (sr, block);
        juce::Random rng (7);
        juce::AudioBuffer<float> buf (2, block);
        float worst = 0.0f;
        for (int b = 0; b < 400; ++b)
        {
            for (int i = 0; i < block; ++i)
            {
                float v = (rng.nextFloat() * 2.0f - 1.0f) * (rng.nextInt (50) == 0 ? 1.0f : 0.2f);
                buf.setSample (0, i, v); buf.setSample (1, i, -v * 0.7f);
            }
            chain.process (buf, s);
            worst = juce::jmax (worst, buf.getMagnitude (0, block), buf.getMagnitude (1, 0, block));
        }
        char label[96]; std::snprintf (label, sizeof label, "Limiter %s%s, %.1f dB ceiling: peak (dBFS)",
                                       mode % 2 == 0 ? "Classic" : "Multi", mode >= 2 ? "+ARC" : "", ceilingDb);
        const double w = juce::Decibels::gainToDecibels ((double) worst);
        check (w <= ceilingDb + 0.001, label, w);
    }

    // 3. De-esser: turns down 8 kHz, leaves 400 Hz alone
    {
        auto s = allOff(); s.dsOn = true; s.dsHz = 5000.0f; s.dsThreshDb = -30.0f; s.dsRangeDb = 8.0f;
        double hi = runSine (s, 8000.0, -10.0f), lo = runSine (s, 400.0, -10.0f);
        check (hi < -15.0 && hi > -19.0, "De-esser: 8 kHz at -10 dBFS comes out at (dB)", hi);
        check (std::abs (lo + 10.0) < 0.3, "De-esser: 400 Hz at -10 dBFS comes out at (dB)", lo);
    }

    // 3b. De-esser styles: Split keeps the body, Wide dips the whole voice; Auto follows level
    {
        auto twoTone = [] (const ChainSettings& st, float bodyDb, float essDb)
        {
            VocalChain chain; chain.prepare (sr, block);
            std::vector<float> out;
            juce::AudioBuffer<float> buf (2, block);
            double p1 = 0.0, p2 = 0.0;
            const float a1 = juce::Decibels::decibelsToGain (bodyDb), a2 = juce::Decibels::decibelsToGain (essDb);
            for (int pos = 0; pos < (int) sr; pos += block)
            {
                for (int i = 0; i < block; ++i)
                {
                    const float v = a1 * (float) std::sin (p1) + a2 * (float) std::sin (p2);
                    p1 += juce::MathConstants<double>::twoPi * 400.0 / sr;
                    p2 += juce::MathConstants<double>::twoPi * 8000.0 / sr;
                    buf.setSample (0, i, v); buf.setSample (1, i, v);
                }
                chain.process (buf, st);
                if (pos >= (int) sr / 2) for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
            }
            return out;
        };

        auto s = allOff(); s.dsOn = true; s.dsHz = 5000.0f; s.dsThreshDb = -30.0f; s.dsRangeDb = 8.0f;
        s.dsMode = DsMode::Split; auto split = twoTone (s, -12.0f, -10.0f);
        s.dsMode = DsMode::Wide;  auto wide  = twoTone (s, -12.0f, -10.0f);
        const double splitBody = toneDb (split, 400.0) + 12.0, wideBody = toneDb (wide, 400.0) + 12.0;
        const double splitEss  = toneDb (split, 8000.0) + 10.0, wideEss = toneDb (wide, 8000.0) + 10.0;
        check (std::abs (splitBody) < 0.3, "De-ess Split: 400 Hz body change while de-essing (dB)", splitBody);
        check (splitEss < -5.0, "De-ess Split: 8 kHz ess change (dB)", splitEss);
        check (wideBody < -5.0 && std::abs (wideBody - wideEss) < 0.5, "De-ess Wide: body dips with the ess (dB)", wideBody);

        // Auto: same reduction whether the singer is loud or 20 dB softer
        s.dsMode = DsMode::Split; s.dsAuto = true;
        auto loud = twoTone (s, -6.0f, -8.0f), soft = twoTone (s, -26.0f, -28.0f);
        const double loudGr = toneDb (loud, 8000.0) + 8.0, softGr = toneDb (soft, 8000.0) + 28.0;
        check (loudGr < -4.0 && std::abs (loudGr - softGr) < 1.0, "De-ess Auto: ess reduction loud vs soft (dB diff)", loudGr - softGr);
        s.dsAuto = false;
        auto softManual = twoTone (s, -26.0f, -28.0f);
        const double softManualGr = toneDb (softManual, 8000.0) + 28.0;
        check (softManualGr > -1.0, "De-ess manual: soft ess below threshold untouched (dB)", softManualGr);
        // Auto ignores a vowel with little top end
        s.dsAuto = true;
        auto vowel = twoTone (s, -6.0f, -40.0f);
        const double vowelGr = toneDb (vowel, 8000.0) + 40.0;
        check (vowelGr > -1.0, "De-ess Auto: dull vowel left alone (dB)", vowelGr);
    }

        // 3c. Stacked compressors: each unit has its own settings; chaining compresses more
    {
        auto s = allOff(); s.compOn = true;
        for (auto& u : s.comps) { u.on = false; u.threshDb = -18.0f; u.ratio = 4.0f; }
        s.comps[0].on = true;                                  // FET alone
        double fetOnly = runSine (s, 1000.0, -6.0f);
        s.comps[2].on = true; s.comps[2].timeConstant = 0;      // + Vari-Mu
        double fetVari = runSine (s, 1000.0, -6.0f);
        check (fetVari < fetOnly - 1.0, "Comp stack: FET + Vari-Mu compress more than FET alone (dB)", fetVari - fetOnly);

        // a unit that is off has no effect, whatever its settings
        auto t = allOff(); t.compOn = true;
        for (auto& u : t.comps) u.on = false;
        t.comps[1].threshDb = -60.0f;                           // Vox Leveler set extreme, but off
        double none = runSine (t, 1000.0, -6.0f);
        check (std::abs (none + 6.0) < 0.05, "Comp stack: all units off = unchanged (dB)", none);

        // section power overrides the units
        s.compOn = false;
        double off = runSine (s, 1000.0, -6.0f);
        check (std::abs (off + 6.0) < 0.05, "Comp stack: section off = unchanged (dB)", off);

        ChainSettings d;
        check (d.comps[0].on && ! d.comps[1].on && ! d.comps[2].on, "Comp default: only FET Purple on (1 = yes)", 1.0);
    }

    // 3d. Processing order: EQ before/after the compressors, and unit order within a group
    {
        // A +12 dB bell into a compressor gets squashed; after the compressor it keeps its full boost
        auto s = allOff(); s.compOn = true; s.eqOn = true;
        for (auto& u : s.comps) u.on = false;
        s.comps[0].on = true; s.comps[0].threshDb = -30.0f; s.comps[0].ratio = 10.0f;
        CharEqSettings e; e.hpfHz = 20.0f; e.presHz = 1000.0f; e.presDb = 12.0f; e.presQ = 1.0f;
        for (auto& x : s.charEq) x.on = false;
        s.charEq[1] = e; s.charEq[1].on = true;
        s.eqAfterComp = false; double pre  = runSine (s, 1000.0, -30.0f);
        s.eqAfterComp = true;  double post = runSine (s, 1000.0, -30.0f);
        check (post > pre + 6.0, "Order: EQ after comp keeps more of a +12 dB boost (dB diff)", post - pre);

        // Compressor order matters: makeup into a heavy limiter-like stage vs the reverse
        auto c = allOff(); c.compOn = true;
        for (auto& u : c.comps) u.on = false;
        c.comps[0] = { true, CompType::FetPurple, -30.0f, 20.0f, 0.1f, 50.0f, 12.0f, 1.0f, 1 };   // FET: hard, +12 makeup
        c.comps[2] = { true, CompType::VariMu660, -20.0f, 4.0f, 1.0f, 100.0f, 0.0f, 1.0f, 0 };
        c.compOrder = { 0, 1, 2 }; double fetFirst  = runSine (c, 1000.0, -12.0f);
        c.compOrder = { 2, 1, 0 }; double variFirst = runSine (c, 1000.0, -12.0f);
        check (std::abs (fetFirst - variFirst) > 1.0, "Order: FET-first vs Vari-first sound different (dB diff)", fetFirst - variFirst);

        // permutation helpers round-trip, and defaults are the natural order
        bool ok = true;
        for (int i = 0; i < 24; ++i) ok = ok && indexFromPermutation (permutationFromIndex<4> (i)) == i;
        for (int i = 0; i < 6; ++i)  ok = ok && indexFromPermutation (permutationFromIndex<3> (i)) == i;
        ChainSettings d;
        ok = ok && d.eqOrder == std::array<int, 4> { 0, 1, 2, 3 } && d.compOrder == std::array<int, 3> { 0, 1, 2 } && ! d.eqAfterComp;
        check (ok, "Order: helpers round-trip, defaults natural (1 = yes)", ok ? 1.0 : 0.0);
    }

    // 4. Compressor modes
    {
        auto s = allOff(); s.compOn = true;
        // helper: use only compressor unit t, with the shared test settings
        auto only = [&s] (CompType t)
        {
            for (auto& u : s.comps) u.on = false;
            auto& u = s.comps[(size_t) t];
            u.on = true; u.threshDb = -18.0f; u.ratio = 4.0f;
        };
        float gr = 0.0f;

        only (CompType::FetPurple);
        double fet = runSine (s, 1000.0, -6.0f, 1.0, &gr);
        check (gr > 6.0f && gr < 10.5f, "FET Purple 4:1, 12 dB over: gain reduction (dB)", gr);
        check (fet < -12.0 && fet > -17.0, "FET Purple output level (dB)", fet);

        // LE-RC (Renaissance style): ratio, Glide vs Snap release, AUTO release, Color vs Pure
        only (CompType::RComp);
        {
            auto& rc = s.comps[1];
            rc.ratio = 4.0f; rc.attackMs = 5.0f; rc.releaseMs = 100.0f; rc.character = 1;
            runSine (s, 1000.0, -6.0f, 1.0, &gr);
            check (gr > 7.0f && gr < 10.0f, "LE-RC 4:1, 12 dB over: gain reduction (dB)", gr);
            rc.ratio = 10.0f;
            runSine (s, 1000.0, -6.0f, 1.0, &gr);
            check (gr > 9.5f && gr < 11.5f, "LE-RC 10:1, 12 dB over: gain reduction (dB)", gr);

            // release after a loud burst: time for the gain reduction to fall from 6 dB to 1 dB
            auto recover = [&] (int behavior, bool arc, double burstSeconds)
            {
                rc.ratio = 4.0f; rc.behavior = behavior; rc.arc = arc;
                VocalChain chain; chain.prepare (sr, block);
                juce::AudioBuffer<float> buf (2, block); double ph = 0.0; int t6 = -1, t1 = -1, pos = 0;
                const int burst = (int) (burstSeconds * sr), total = burst + (int) (3.0 * sr);
                for (; pos < total; pos += block)
                {
                    const float a = pos < burst ? 0.5f : 0.06f;   // -6 dBFS burst, then -24 dBFS (under threshold)
                    for (int i = 0; i < block; ++i) { const float v = a * (float) std::sin (ph); ph += juce::MathConstants<double>::twoPi * 1000.0 / sr; buf.setSample (0, i, v); buf.setSample (1, i, v); }
                    chain.process (buf, s);
                    if (pos >= burst)
                    {
                        const float g = chain.compUnitGr[1].load();
                        if (t6 < 0 && g < 6.0f) t6 = pos;
                        if (t6 >= 0 && t1 < 0 && g < 1.0f) { t1 = pos; break; }
                    }
                }
                return t1 < 0 || t6 < 0 ? 9999.0 : 1000.0 * (t1 - t6) / sr;
            };
            const double opto = recover (0, false, 0.5), electro = recover (1, false, 0.5);
            std::printf ("      LE-RC release from 6 to 1 dB GR (100 ms knob): Glide %.0f ms, Snap %.0f ms\n", opto, electro);
            check (opto > electro * 1.8, "LE-RC Glide releases slower near threshold than Snap (ms ratio)", opto / juce::jmax (1.0, electro));
            const double arcShort = recover (1, true, 0.2), arcLong = recover (1, true, 3.0);
            std::printf ("      LE-RC AUTO release after a 0.2 s burst %.0f ms, after a 3 s burst %.0f ms\n", arcShort, arcLong);
            check (arcLong > arcShort * 1.5, "LE-RC AUTO: the release stretches under sustained compression (ms ratio)", arcLong / juce::jmax (1.0, arcShort));

            rc.behavior = 0; rc.arc = true;
            auto h3 = [&] (int character) {
                rc.character = character; rc.ratio = 4.0f;
                const auto y = renderSine (s, 500.0, -6.0f);
                return toneDb (y, 1500.0) - toneDb (y, 500.0); };
            const double warm = h3 (0), smooth = h3 (1);
            std::printf ("      LE-RC 3rd harmonic: Color %.1f dB, Pure %.1f dB\n", warm, smooth);
            check (warm > smooth + 10.0, "LE-RC Color adds harmonics, Pure stays clean (dB)", warm - smooth);
            rc.character = 0;
        }

        only (CompType::VariMu660); s.comps[2].timeConstant = 0;
        runSine (s, 1000.0, -12.0f, 1.0, &gr); float grLow = gr;
        runSine (s, 1000.0,   0.0f, 1.0, &gr); float grHigh = gr;
        // Rising ratio: 12 dB more input should give clearly more than 12*(1-1/2) dB more GR... check it grows
        check (grHigh - grLow > 6.0f, "Vari-Mu: extra GR for +12 dB input (dB)", grHigh - grLow);
    }

    // 5. EQ characters
    {
        auto s = allOff();
        CharEqSettings base; base.presHz = 3000.0f; base.presDb = 6.0f; base.presQ = 1.0f; base.hpfHz = 20.0f;
        for (auto t : { EqType::EConsole, EqType::ClassA81 })
        {
            use (s, t, base);
            double g = runSine (s, 3000.0, -24.0f) + 24.0;
            const char* name = t == EqType::EConsole ? "E-Console" : "Class A 81";
            char label[96]; std::snprintf (label, sizeof label, "EQ %s: +6 dB bell at 3 kHz measures (dB)", name);
            check (std::abs (g - 6.0) < 0.5, label, g);
        }

        // HPF slope: E-Console (18 dB/oct) should cut harder an octave below than Class A 81 (12)
        auto h = allOff(); CharEqSettings he; he.hpfHz = 200.0f;
        use (h, EqType::ClassA81, he); double a81  = runSine (h, 100.0, -12.0f, 1.5) + 12.0;
        use (h, EqType::EConsole, he); double econ = runSine (h, 100.0, -12.0f, 1.5) + 12.0;
        check (std::abs (a81 + 12.0) < 1.5, "Class A 81 HPF one octave down (dB)", a81);
        check (std::abs (econ + 18.1) < 1.5, "E-Console HPF one octave down (dB)", econ);

        // Proportional Q: at small gain the Class A 81 bell is broader than E-Console's constant-Q bell
        CharEqSettings p; p.hpfHz = 20.0f; p.presHz = 3000.0f; p.presDb = 3.0f; p.presQ = 1.0f;
        auto dc = VocalEQ::design (p, EqType::EConsole, sr);
        auto dn = VocalEQ::design (p, EqType::ClassA81, sr);
        double atOctave = VocalEQ::magnitudeDb (dn, 6000.0, sr) - VocalEQ::magnitudeDb (dc, 6000.0, sr);
        check (atOctave > 0.3, "Class A 81 broader bell: extra lift an octave up (dB)", atOctave);
    }

    // 6. Gate: quiet noise ducked, voice-level signal untouched
    {
        auto s = allOff(); s.gateThreshDb = -40.0f;
        double quiet = runSine (s, 300.0, -60.0f), loud = runSine (s, 300.0, -20.0f);
        check (quiet < -85.0, "Gate at -40 dB: -60 dBFS hum comes out at (dB)", quiet);
        check (std::abs (loud + 20.0) < 0.1, "Gate at -40 dB: -20 dBFS voice comes out at (dB)", loud);
        s.gateThreshDb = -80.0f;
        double offQ = runSine (s, 300.0, -60.0f);
        check (std::abs (offQ + 60.0) < 0.1, "Gate off: -60 dBFS passes at (dB)", offQ);
    }

    // 7. Maximizer: threshold -6 raises a quiet signal by 6 dB when under the ceiling
    {
        auto s = allOff(); s.limOn = true; s.limMode = LimMode::Classic; s.limThreshDb = -6.0f; s.limCeilingDb = -1.0f;
        double out = runSine (s, 500.0, -20.0f);
        check (std::abs (out + 14.0) < 0.1, "Maximizer: -20 dBFS with threshold -6 comes out at (dB)", out);
        s.limMode = LimMode::Multi;
        double outM = runSine (s, 500.0, -20.0f);
        check (std::abs (outM + 14.0) < 0.2, "Maximizer Multi: same signal comes out at (dB)", outM);
    }

    // 7b. Vox Flow styles (stage between de-esser and limiter)
    {
        auto s = allOff(); s.flowOn = true; s.flowStyle = (int) FlowStyle::Pop; s.flowAmount = 0.5f; s.flowMix = 1.0f;
        double quietBody = runSine (s, 1200.0, -40.0f) + 40.0;
        double quietAir  = runSine (s, 10500.0, -40.0f) + 40.0;
        double loudBody  = runSine (s, 1200.0, -6.0f) + 6.0;
        double loudHarsh = runSine (s, 4000.0, -6.0f) + 6.0;
        check (std::abs (quietBody) < 0.3, "Vox Flow Pop: quiet 1.2 kHz body change (dB)", quietBody);
        check (quietAir > 0.8 && quietAir < 2.5, "Vox Flow Pop: quiet 10.5 kHz sheen (dB)", quietAir);
        check (loudBody > -2.5, "Vox Flow Pop: loud 1.2 kHz body change (dB)", loudBody);
        check (loudHarsh < loudBody - 3.0, "Vox Flow Pop: loud 4 kHz harshness change (dB)", loudHarsh);

        s.flowStyle = (int) FlowStyle::MidForward;
        double mid = runSine (s, 1400.0, -40.0f) + 40.0;
        check (mid > 2.3 && mid < 3.5, "Vox Flow Mid Forward: quiet 1.4 kHz boost (dB)", mid);

        s.flowStyle = (int) FlowStyle::AiryBallad;
        double airy = runSine (s, 10500.0, -40.0f) + 40.0;
        check (airy > quietAir + 0.5, "Vox Flow Airy Ballad: more air than Pop (dB @10.5k)", airy);

        // Rap holds loud lows harder than R&B (faster, higher ratio)
        s.flowStyle = (int) FlowStyle::Rap;  double rapLow = runSine (s, 120.0, -6.0f, 1.5) + 6.0;
        s.flowStyle = (int) FlowStyle::RnB;  double rnbLow = runSine (s, 120.0, -6.0f, 1.5) + 6.0;
        check (rapLow < rnbLow - 1.0, "Vox Flow Rap vs R&B: loud 120 Hz held harder (dB diff)", rapLow - rnbLow);

        // Amount: 0% = nearly transparent, 100% = more of the style than 50%
        s.flowStyle = (int) FlowStyle::MidForward;
        s.flowAmount = 0.0f; double a0 = runSine (s, 1400.0, -40.0f) + 40.0;
        s.flowAmount = 1.0f; double a1 = runSine (s, 1400.0, -40.0f) + 40.0;
        check (std::abs (a0) < 0.3, "Vox Flow Amount 0%: quiet 1.4 kHz change (dB)", a0);
        check (a1 > mid + 1.0, "Vox Flow Amount 100%: stronger than 50% (dB)", a1);

        // Mix 0%: flat at every frequency (dry path is phase-matched)
        s.flowStyle = (int) FlowStyle::Dense; s.flowAmount = 1.0f; s.flowMix = 0.0f;
        double worst = 0.0;
        for (double hz : { 120.0, 400.0, 1400.0, 4000.0, 10500.0 })
            worst = juce::jmax (worst, std::abs (runSine (s, hz, -10.0f) + 10.0));
        check (worst < 0.15, "Vox Flow Mix 0%: flat response (max dB off)", worst);
        s.flowMix = 0.5f; double m5 = runSine (s, 4000.0, -6.0f) + 6.0;
        s.flowMix = 1.0f; double m1 = runSine (s, 4000.0, -6.0f) + 6.0;
        check (m5 > m1 && m5 < 0.5, "Vox Flow Mix 50%: between dry and full (dB)", m5);

        // every style stays finite and sane on a hot signal
        bool ok = true;
        for (int st = 0; st < numFlowStyles; ++st)
        {
            s.flowStyle = st; s.flowAmount = 1.0f; s.flowMix = 1.0f;
            const double o = runSine (s, 700.0, -3.0f);
            ok = ok && std::isfinite (o) && o < 6.0 && o > -30.0;
        }
        check (ok, "Zerem: all 12 styles finite and sane at 100% (1 = yes)", ok ? 1.0 : 0.0);
    }

    // 7c. Airy EQ: bells hit their gain; air band is broader than a normal shelf
    {
        auto s = allOff(); CharEqSettings e; e.hpfHz = 20.0f; e.presHz = 3000.0f; e.presDb = 6.0f; e.presQ = 1.0f;
        use (s, EqType::Airy, e);
        double bell = runSine (s, 3000.0, -24.0f) + 24.0;
        check (std::abs (bell - 6.0) < 0.5, "Airy: +6 dB bell at 3 kHz measures (dB)", bell);
        CharEqSettings a; a.hpfHz = 20.0f; a.airHz = 12000.0f; a.airDb = 6.0f;
        auto dc = VocalEQ::design (a, EqType::EConsole, sr);
        auto da = VocalEQ::design (a, EqType::Airy, sr);
        double extra = VocalEQ::magnitudeDb (da, 5000.0, sr) - VocalEQ::magnitudeDb (dc, 5000.0, sr);
        check (extra > 0.5, "Airy: air band reaches lower than a plain shelf (dB @5k)", extra);
    }

    // 7d. Silk (Airy only): adds a 2nd harmonic to air content, leaves lows alone
    {
        auto s = allOff(); CharEqSettings e; e.hpfHz = 20.0f;
        auto silkAt = [&] (EqType t, float amount) { e.silk = amount; use (s, t, e); };
        silkAt (EqType::Airy, 0.0f);  auto dry  = renderSine (s, 8000.0, -24.0f);
        silkAt (EqType::Airy, 1.0f);  auto wet  = renderSine (s, 8000.0, -24.0f);
        silkAt (EqType::Airy, 0.25f); auto mid  = renderSine (s, 8000.0, -24.0f);
        const double fund = toneDb (wet, 8000.0);
        const double h2dry = toneDb (dry, 16000.0) - toneDb (dry, 8000.0);
        const double h2wet = toneDb (wet, 16000.0) - fund;
        const double h2mid = toneDb (mid, 16000.0) - toneDb (mid, 8000.0);
        check (h2dry < -80.0, "Silk 0%: 2nd harmonic of 8 kHz (dB re fundamental)", h2dry);
        check (h2mid > -60.0 && h2mid < h2wet, "Silk 25%: 2nd harmonic (dB re fundamental)", h2mid);
        check (h2wet > -45.0 && h2wet < -20.0, "Silk 100%: 2nd harmonic (dB re fundamental)", h2wet);
        check (std::abs (fund - toneDb (dry, 8000.0)) < 0.3, "Silk 100%: change in 8 kHz level (dB)", fund - toneDb (dry, 8000.0));

        silkAt (EqType::Airy, 0.0f); auto lowDry = renderSine (s, 300.0, -6.0f);
        silkAt (EqType::Airy, 1.0f); auto lowWet = renderSine (s, 300.0, -6.0f);
        const double lowH = toneDb (lowWet, 600.0) - toneDb (lowDry, 600.0);
        std::printf ("      (300 Hz 2nd harmonic: dry %.1f dB, wet %.1f dB re fundamental)\n",
                     toneDb (lowDry, 600.0) - toneDb (lowDry, 300.0), toneDb (lowWet, 600.0) - toneDb (lowWet, 300.0));
        check (lowH < 0.5, "Silk 100%: extra 2nd harmonic on a 300 Hz voice (dB)", lowH);
        check (std::abs (toneDb (lowWet, 300.0) - toneDb (lowDry, 300.0)) < 0.05, "Silk 100%: 300 Hz level change (dB)",
               toneDb (lowWet, 300.0) - toneDb (lowDry, 300.0));

        silkAt (EqType::EConsole, 1.0f);
        auto clean = renderSine (s, 8000.0, -24.0f);
        const double h2clean = toneDb (clean, 16000.0) - toneDb (clean, 8000.0);
        check (h2clean < -80.0, "Silk ignored outside Airy (2nd harmonic, dB)", h2clean);
    }

    // 7d2. Stacked character EQs run in series, each with its own settings
    {
        ChainSettings d;
        check (! d.charEq[0].on && d.charEq[1].on && ! d.charEq[2].on && ! d.charEq[3].on,
               "Default: only E-Console active (1 = yes)", 1.0);

        auto s = allOff(); s.eqOn = true;
        for (auto& x : s.charEq) { x.hpfHz = 20.0f; x.presHz = 1000.0f; x.presQ = 1.0f; x.presDb = 0.0f; x.on = false; }
        s.charEq[1].on = true; s.charEq[1].presDb = 3.0f;      // E-Console +3 at 1 kHz
        s.charEq[3].on = true; s.charEq[3].presDb = 3.0f;      // Airy      +3 at 1 kHz
        s.charEq[3].silk = 0.0f;
        double both = runSine (s, 1000.0, -24.0f) + 24.0;
        check (std::abs (both - 6.0) < 0.5, "E-Console + Airy stacked, +3 dB each at 1 kHz (dB)", both);

        for (auto& x : s.charEq) x.on = true;                 // + Tube P1 (flat) + Class A 81 (+3)
        s.charEq[2].presDb = 3.0f;
        s.charEq[0].p1HighHz = 3000.0f; s.charEq[0].p1HighBoost = 0.0f;
        double all4 = runSine (s, 1000.0, -24.0f) + 24.0;
        check (std::abs (all4 - 9.0) < 1.0, "All four stacked (P1 flat, 3 x +3 dB) at 1 kHz (dB)", all4);

        s.eqOn = false;
        double off = runSine (s, 1000.0, -24.0f) + 24.0;
        check (std::abs (off) < 0.05, "Character EQ section off: 1 kHz change (dB)", off);
    }

    // 7d2b. Tube P1 (Pultec-style) curves
    {
        auto s = allOff();
        CharEqSettings p; use (s, EqType::TubeP1, p);
        double flat100 = runSine (s, 100.0, -24.0f) + 24.0, flat1k = runSine (s, 1000.0, -24.0f) + 24.0, flat10k = runSine (s, 10000.0, -24.0f) + 24.0;
        check (std::abs (flat100) < 0.3 && std::abs (flat1k) < 0.3 && std::abs (flat10k) < 0.3, "Tube P1 all dials at 0: flat (max dB off at 1 kHz)", flat1k);

        p.p1LowHz = 60.0f; p.p1LowBoost = 10.0f; use (s, EqType::TubeP1, p);
        double boost = runSine (s, 50.0, -30.0f, 1.5) + 30.0;
        check (boost > 11.0 && boost < 15.0, "Tube P1 Low Boost 10 @60: lift at 50 Hz (dB)", boost);

        p.p1LowAtten = 10.0f; use (s, EqType::TubeP1, p);
        double trickLow = runSine (s, 50.0, -30.0f, 1.5) + 30.0, trickDip = runSine (s, 180.0, -30.0f, 1.5) + 30.0;
        check (trickLow > 2.0 && trickLow < 7.0, "Tube P1 trick (boost+atten 10): lows at 50 Hz (dB)", trickLow);
        check (trickDip < -3.5, "Tube P1 trick: dip at 180 Hz (dB)", trickDip);

        CharEqSettings h; h.p1HighHz = 10000.0f; h.p1HighBoost = 10.0f; h.p1HighBw = 5.0f; use (s, EqType::TubeP1, h);
        double hb = runSine (s, 10000.0, -30.0f) + 30.0;
        check (hb > 14.0 && hb < 17.5, "Tube P1 High Boost 10 @10k (dB)", hb);
        auto sharp = h; sharp.p1HighBw = 0.0f; auto broad = h; broad.p1HighBw = 10.0f;
        const double wSharp = VocalEQ::magnitudeDb (VocalEQ::design (sharp, EqType::TubeP1, sr), 5000.0, sr);
        const double wBroad = VocalEQ::magnitudeDb (VocalEQ::design (broad, EqType::TubeP1, sr), 5000.0, sr);
        check (wBroad > wSharp + 4.0, "Tube P1 Bandwidth: broad lifts 5 kHz more than sharp (dB)", wBroad - wSharp);

        CharEqSettings c; c.p1AttenHz = 10000.0f; c.p1HighAtten = 10.0f; use (s, EqType::TubeP1, c);
        double cut = runSine (s, 16000.0, -30.0f) + 30.0, keep = runSine (s, 1000.0, -30.0f) + 30.0;
        check (cut < -8.0 && std::abs (keep) < 0.5, "Tube P1 High Atten 10 @10k: cut at 16 kHz (dB)", cut);
    }

    // 7d3. Compressor and limiter Mix
    {
        auto s = allOff(); s.compOn = true;
        auto& fet = s.comps[0]; fet.on = true; fet.threshDb = -18.0f; fet.ratio = 4.0f;
        fet.mix = 0.0f; double c0 = runSine (s, 1000.0, -6.0f);
        fet.mix = 0.5f; double c5 = runSine (s, 1000.0, -6.0f);
        fet.mix = 1.0f; double c1 = runSine (s, 1000.0, -6.0f);
        check (std::abs (c0 + 6.0) < 0.15, "Comp Mix 0%: output = dry -6 dBFS (dB)", c0);
        check (c5 < c0 - 1.0 && c5 > c1 + 1.0, "Comp Mix 50%: between dry and compressed (dB)", c5);

        // Limiter Mix 0%: exactly the dry input, delayed by the look-ahead
        auto l = allOff(); l.limOn = true; l.limThreshDb = -12.0f; l.limMix = 0.0f; l.limMode = LimMode::Classic;
        VocalChain chain; chain.prepare (sr, block);
        const int lat = chain.getLatencySamples();
        juce::Random rng (5);
        std::vector<float> in ((size_t) sr), out ((size_t) sr);
        for (auto& v : in) v = (rng.nextFloat() * 2.0f - 1.0f) * 0.5f;
        juce::AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos + block <= (int) in.size(); pos += block)
        {
            for (int i = 0; i < block; ++i) { buf.setSample (0, i, in[(size_t) (pos + i)]); buf.setSample (1, i, in[(size_t) (pos + i)]); }
            chain.process (buf, l);
            for (int i = 0; i < block; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
        }
        double err = 0.0;
        for (size_t i = (size_t) lat + 2048; i < 40000; ++i) err = juce::jmax (err, (double) std::abs (out[i] - in[i - (size_t) lat]));
        check (err < 1e-5, "Limiter Mix 0%: dry path time-aligned, nulls (max error)", err);

        // Multiband modes: at Mix 0% the dry path only carries the crossovers' phase, so the level is flat
        for (auto mode : { LimMode::Multi })
            for (double hz : { 120.0, 1000.0, 5000.0, 10500.0 })
            {
                auto m = l; m.limMode = mode; m.limMix = 0.0f;
                const double lvl = runSine (m, hz, -20.0f) + 20.0;
                char label[96]; std::snprintf (label, sizeof label, "Limiter %s Mix 0%%: level at %.0f Hz (dB)",
                                               "Multi", hz);
                check (std::abs (lvl) < 0.15, label, lvl);
            }

        l.limThreshDb = -6.0f;
        for (auto mode : { LimMode::Classic, LimMode::Multi })
        {
            l.limMode = mode;
            l.limMix = 0.5f; double l5 = runSine (l, 500.0, -20.0f);
            l.limMix = 1.0f; double l1 = runSine (l, 500.0, -20.0f);
            const char* nm = mode == LimMode::Classic ? "Classic" : "Multi";
            char label[96]; std::snprintf (label, sizeof label, "Limiter %s Mix 50%%: between dry -20 and wet (dB)", nm);
            check (l5 > -20.0 && l5 < l1, label, l5);
        }
    }

    // 7e. Resonance suppressor: pulls a ringing tone out of noise, leaves the noise alone
    {
        auto render = [] (const ResoSettings& rs, bool withTone, bool withNoise)
        {
            ResonanceSuppressor r; r.prepare (sr);
            juce::Random rng (3);
            juce::AudioBuffer<float> buf (2, block);
            std::vector<float> out;
            double ph = 0.0; float lp = 0.0f;
            for (int pos = 0; pos < (int) sr * 2; pos += block)
            {
                for (int i = 0; i < block; ++i)
                {
                    lp += 0.3f * ((rng.nextFloat() - 0.5f) - lp);
                    float v = withNoise ? 0.3f * lp : 0.0f;
                    if (withTone) v += 0.1f * (float) std::sin (ph);
                    ph += juce::MathConstants<double>::twoPi * 3000.0 / sr;
                    buf.setSample (0, i, v); buf.setSample (1, i, v);
                }
                r.process (buf, rs);
                if (pos >= (int) sr) for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
            }
            return out;
        };
        auto rmsDb = [] (const std::vector<float>& x)
        {
            double e = 0.0; for (auto v : x) e += (double) v * v;
            return juce::Decibels::gainToDecibels (std::sqrt (e / (double) x.size()), -200.0);
        };

        ResoSettings off; off.on = false;
        ResoSettings on;  on.depth = 1.0f;
        const double toneOff = toneDb (render (off, true, true), 3000.0);
        const double toneOn  = toneDb (render (on,  true, true), 3000.0);
        check (toneOn - toneOff < -6.0, "Reso depth 100%: ringing 3 kHz tone pulled down (dB)", toneOn - toneOff);

        const double noiseOff = rmsDb (render (off, false, true));
        const double noiseOn  = rmsDb (render (on,  false, true));
        check (std::abs (noiseOn - noiseOff) < 1.0, "Reso depth 100%: smooth noise level change (dB)", noiseOn - noiseOff);

        ResoSettings outside = on; outside.lowHz = 5000.0f; outside.highHz = 12000.0f;
        const double toneOut = toneDb (render (outside, true, true), 3000.0);
        check (std::abs (toneOut - toneOff) < 0.5, "Reso focus 5-12 kHz: 3 kHz tone untouched (dB)", toneOut - toneOff);

        ResoSettings delta = on; delta.delta = true;
        const double removed = toneDb (render (delta, true, true), 3000.0);
        const double expectedRemoved = juce::Decibels::gainToDecibels (
            1.0 - juce::Decibels::decibelsToGain (toneOn - toneOff)) + toneOff;
        check (std::abs (removed - expectedRemoved) < 1.5, "Reso Delta: plays exactly what was removed (dB error)", removed - expectedRemoved);

        ResoSettings half = on; half.mix = 0.5f;
        const double toneHalf = toneDb (render (half, true, true), 3000.0);
        check (toneHalf > toneOn && toneHalf < toneOff, "Reso Mix 50%: in between (dB)", toneHalf - toneOff);
    }

    // 8. Clean-up EQ
    {
        // Low cut slopes: one octave below the corner should be ~ -12/-24/-36/-48 dB
        for (int slope = 0; slope < 4; ++slope)
        {
            auto s = allOff();
            auto& b = s.clean[0];
            b.on = true; b.type = BandType::LowCut; b.freq = 400.0f; b.q = 0.7071f; b.slope = slope;
            double out = runSine (s, 200.0, -6.0f, 1.0) + 6.0;
            const double expected = -12.3 * (slope + 1);
            char label[96]; std::snprintf (label, sizeof label, "Clean-up low cut %d dB/oct, octave below (dB)", 12 * (slope + 1));
            check (std::abs (out - expected) < 1.5, label, out);
        }
        // Bell and notch
        {
            auto s = allOff();
            s.clean[2].on = true; s.clean[2].type = BandType::Bell; s.clean[2].freq = 2500.0f; s.clean[2].gain = -8.0f; s.clean[2].q = 4.0f;
            double bell = runSine (s, 2500.0, -12.0f) + 12.0;
            check (std::abs (bell + 8.0) < 0.3, "Clean-up bell -8 dB at 2.5 kHz measures (dB)", bell);
            s.clean[2].type = BandType::Notch;
            double notch = runSine (s, 2500.0, -12.0f) + 12.0;
            check (notch < -30.0, "Clean-up notch at 2.5 kHz measures (dB)", notch);
        }
        // Dynamic bell: no cut when quiet, cuts when that frequency gets loud
        {
            auto s = allOff();
            auto& b = s.clean[3];
            b.on = true; b.type = BandType::Bell; b.freq = 3000.0f; b.gain = 0.0f; b.q = 2.0f;
            b.dyn = true; b.dynThresh = -24.0f; b.dynRange = -9.0f;
            double quiet = runSine (s, 3000.0, -40.0f) + 40.0;
            double loud  = runSine (s, 3000.0, -6.0f) + 6.0;
            check (std::abs (quiet) < 0.3, "Dynamic bell, quiet 3 kHz: change (dB)", quiet);
            check (loud < -7.0 && loud > -9.5, "Dynamic bell, loud 3 kHz: change (dB)", loud);
        }
        // All 8 bands on at once: still finite and sensible
        {
            auto s = allOff();
            const BandType types[8] { BandType::LowCut, BandType::LowShelf, BandType::Bell, BandType::Bell,
                                      BandType::Notch, BandType::Bell, BandType::HighShelf, BandType::HighCut };
            const float freqs[8] { 70, 150, 300, 800, 1600, 3500, 9000, 18000 };
            for (int i = 0; i < 8; ++i)
            {
                auto& b = s.clean[(size_t) i];
                b.on = true; b.type = types[i]; b.freq = freqs[i]; b.gain = (i % 2) ? 3.0f : -3.0f; b.q = 1.0f; b.slope = 3; b.dyn = (i == 5);
            }
            double out = runSine (s, 1000.0, -12.0f);
            check (std::isfinite (out) && out > -20.0 && out < -4.0, "Clean-up, 8 bands active: 1 kHz output (dBFS)", out);
        }
    }

    // 9. Full chain at defaults: sane level, no NaNs
    {
        ChainSettings s;
        double out = runSine (s, 220.0, -3.0f);
        check (std::isfinite (out) && out <= -1.0 + 0.001, "Full chain default output peak (dBFS)", out);
    }

    // ---- Tracking mode (low latency) ----------------------------------------
    {
        std::printf ("\n-- Tracking mode --\n");
        VocalChain chain; chain.prepare (sr, block);
        check (chain.latencyFor (true) == 0, "Tracking mode latency (samples)", chain.latencyFor (true));
        // all off: output = input, no delay at all
        ChainSettings s = allOff(); s.trackMode = true;
        juce::AudioBuffer<float> buf (2, block); double phase = 0.0, err = 0.0;
        for (int blk = 0; blk < 40; ++blk)
        {
            juce::AudioBuffer<float> ref (2, block);
            for (int i = 0; i < block; ++i) { const float v = 0.4f * (float) std::sin (phase); phase += 0.05; buf.setSample (0, i, v); buf.setSample (1, i, v); ref.setSample (0, i, v); }
            chain.process (buf, s);
            if (blk > 2) for (int i = 0; i < block; ++i) err = juce::jmax (err, (double) std::abs (buf.getSample (0, i) - ref.getSample (0, i)));
        }
        check (err < 1e-5, "Tracking mode, all off: passes straight through (max error)", err);
        // limiter on, hot input: ceiling still never exceeded without look-ahead
        ChainSettings l = allOff(); l.trackMode = true; l.limOn = true; l.limThreshDb = -10.0f; l.limCeilingDb = -1.0f; l.peakCatch = true; l.tapeOn = true;
        VocalChain c2; c2.prepare (sr, block); double peak = 0.0; phase = 0.0;
        for (int blk = 0; blk < 80; ++blk)
        {
            for (int i = 0; i < block; ++i) { const float v = 0.9f * (float) std::sin (phase) + ((blk * block + i) % 3000 < 30 ? 0.5f : 0.0f); phase += 0.03; buf.setSample (0, i, v); buf.setSample (1, i, v); }
            c2.process (buf, l);
            if (blk > 2) peak = juce::jmax (peak, (double) buf.getMagnitude (0, 0, block));
        }
        check (juce::Decibels::gainToDecibels (peak) <= -1.0 + 0.01, "Tracking mode limiter holds the ceiling (dBFS)", juce::Decibels::gainToDecibels (peak));
    }

    // ---- Vocal rider, breath control, sidechain high-pass ---------------------
    {
        std::printf ("\n-- Rider / Breath / Sidechain HPF --\n");
        auto dynOnly = []
        {
            ChainSettings s = allOff();
            s.compOn = true;
            for (auto& u : s.comps) u.on = false;   // no compressors: hear just the new stages
            s.gateThreshDb = -100.0f;
            return s;
        };
        // a voice-like tone (220 Hz + harmonics)
        auto voice = [] (int t, float amp)
        {
            const float ph = juce::MathConstants<float>::twoPi * 220.0f * (float) t / (float) sr;
            return amp * (0.6f * std::sin (ph) + 0.3f * std::sin (2.0f * ph) + 0.15f * std::sin (3.0f * ph));
        };

        // Rider: phrases alternate 12 dB apart (1 s each); after settling, the gap should shrink a lot
        {
            ChainSettings s = dynOnly(); s.riderRangeDb = 12.0f;
            VocalChain chain; chain.prepare (sr, block);
            juce::AudioBuffer<float> buf (2, block);
            double eLoud = 0, eQuiet = 0; int nL = 0, nQ = 0;
            const int total = (int) (16.0 * sr);
            for (int pos = 0; pos < total; pos += block)
            {
                for (int i = 0; i < block; ++i)
                {
                    const int t = pos + i;
                    const bool loud = (t / (int) sr) % 2 == 0;
                    const float v = voice (t, juce::Decibels::decibelsToGain (loud ? -8.0f : -20.0f));
                    buf.setSample (0, i, v); buf.setSample (1, i, v);
                }
                chain.process (buf, s);
                if (pos > (int) (10.0 * sr))
                    for (int i = 0; i < block; ++i)
                    {
                        const int t = pos + i;
                        if ((t % (int) sr) < (int) (sr * 0.4)) continue;   // judge the settled part of each phrase
                        const double y = buf.getSample (0, i);
                        if ((t / (int) sr) % 2 == 0) { eLoud += y * y; ++nL; } else { eQuiet += y * y; ++nQ; }
                    }
            }
            const double gap = 10.0 * std::log10 ((eLoud / nL) / (eQuiet / nQ));
            check (gap < 5.0, "Rider evens out phrases 12 dB apart (gap left, dB)", gap);
        }

        // Breath control: sung tone, then an airy breath 28 dB quieter, repeating
        {
            auto run = [&] (float breathDb, double& vowelDb, double& breathOut)
            {
                ChainSettings s = dynOnly(); s.breathDb = breathDb;
                VocalChain chain; chain.prepare (sr, block);
                juce::Random rnd (7);
                juce::dsp::IIR::Filter<float> hp; hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 2500.0, 0.7f);
                juce::AudioBuffer<float> buf (2, block);
                double eV = 0, eB = 0; int nV = 0, nB = 0;
                const int cycle = (int) (1.2 * sr), total = (int) (8.0 * sr);
                for (int pos = 0; pos < total; pos += block)
                {
                    for (int i = 0; i < block; ++i)
                    {
                        const int t = pos + i, ph = t % cycle;
                        float v;
                        if (ph < (int) (0.8 * sr)) v = voice (t, juce::Decibels::decibelsToGain (-10.0f));
                        else v = juce::Decibels::decibelsToGain (-34.0f) * hp.processSample (rnd.nextFloat() * 2.0f - 1.0f) * 2.0f;
                        buf.setSample (0, i, v); buf.setSample (1, i, v);
                    }
                    chain.process (buf, s);
                    if (pos > (int) (3.0 * sr))
                        for (int i = 0; i < block; ++i)
                        {
                            const int ph = (pos + i) % cycle;
                            const double y = buf.getSample (0, i);
                            if (ph > (int) (0.1 * sr) && ph < (int) (0.75 * sr)) { eV += y * y; ++nV; }
                            else if (ph > (int) (0.9 * sr) && ph < (int) (1.15 * sr)) { eB += y * y; ++nB; }
                        }
                }
                vowelDb = 10.0 * std::log10 (eV / nV); breathOut = 10.0 * std::log10 (eB / nB);
            };
            double v0 = 0, b0 = 0, v1 = 0, b1 = 0;
            run (0.0f, v0, b0);
            run (-15.0f, v1, b1);
            std::printf ("      vowel %.1f -> %.1f dB, breath %.1f -> %.1f dB\n", v0, v1, b0, b1);
            check (b0 - b1 > 10.0, "Breath control turns breaths down (dB)", b0 - b1);
            check (std::abs (v0 - v1) < 0.5, "Breath control leaves the singing alone (dB change)", std::abs (v0 - v1));
        }

        // Sidechain high-pass: a boomy low end shouldn't drive the compressor
        {
            auto gr = [&] (int hpf)
            {
                ChainSettings s = dynOnly(); s.compScHpf = hpf;
                s.comps[0].on = true; s.comps[0].type = CompType::FetPurple; s.comps[0].threshDb = -20.0f; s.comps[0].ratio = 4.0f;
                VocalChain chain; chain.prepare (sr, block);
                juce::AudioBuffer<float> buf (2, block); float worst = 0.0f;
                for (int pos = 0; pos < (int) sr; pos += block)
                {
                    for (int i = 0; i < block; ++i)
                    {
                        const float t = (float) (pos + i) / (float) sr;
                        const float v = 0.5f * std::sin (juce::MathConstants<float>::twoPi * 55.0f * t) + 0.05f * std::sin (juce::MathConstants<float>::twoPi * 1000.0f * t);
                        buf.setSample (0, i, v); buf.setSample (1, i, v);
                    }
                    chain.process (buf, s);
                    if (pos > (int) (0.5 * sr)) worst = juce::jmax (worst, chain.meters.compGr.load());
                }
                return (double) worst;
            };
            const double off = gr (0), on = gr (3);
            std::printf ("      compressor GR on a boomy signal: SC HPF off %.1f dB, 160 Hz %.1f dB\n", off, on);
            check (off - on > 6.0, "Sidechain high-pass stops the low end over-triggering (dB less GR)", off - on);
        }
    }

    // ---- Loudness (LUFS) ------------------------------------------------------
    {
        std::printf ("\n-- Loudness (LUFS) --\n");
        auto runTone = [] (LoudnessMeter& m, float dbfs, double seconds, double& phase)
        {
            juce::AudioBuffer<float> b (2, block);
            const float a = juce::Decibels::decibelsToGain (dbfs);
            for (int pos = 0; pos < (int) (seconds * sr); pos += block)
            {
                for (int i = 0; i < block; ++i) { const float v = a * (float) std::sin (phase); phase += juce::MathConstants<double>::twoPi * 1000.0 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
                m.process (b);
            }
        };
        LoudnessMeter m; m.prepare (sr); double ph = 0.0;
        runTone (m, -23.0f, 10.0, ph);
        check (std::abs (m.integrated.load() + 23.0f) < 0.1f, "EBU test: 1 kHz at -23 dBFS reads -23 LUFS (integrated)", m.integrated.load());
        check (std::abs (m.shortTerm.load() + 23.0f) < 0.1f, "EBU test: short-term -23 LUFS", m.shortTerm.load());
        runTone (m, -200.0f, 10.0, ph);   // silence: gated out
        check (std::abs (m.integrated.load() + 23.0f) < 0.1f, "Silence doesn't pull the integrated down (gate)", m.integrated.load());
        LoudnessMeter m2; m2.prepare (sr); ph = 0.0;
        runTone (m2, -20.0f, 10.0, ph);
        runTone (m2, -40.0f, 10.0, ph);   // more than 10 LU quieter: relative gate drops it
        check (std::abs (m2.integrated.load() + 20.0f) < 0.3f, "Relative gate ignores very quiet passages (LUFS)", m2.integrated.load());
        m2.resetRequest = true; runTone (m2, -30.0f, 5.0, ph);
        check (std::abs (m2.integrated.load() + 30.0f) < 0.2f, "Reset starts a fresh integrated reading (LUFS)", m2.integrated.load());
    }

    // ---- Soft saturation (Lavry-style) ------------------------------------------
    {
        std::printf ("\n-- Soft saturation --\n");
        auto run = [] (int setting, float dbfs, double* gainDb, double* peakDb, double* h3Db)
        {
            juce::AudioBuffer<float> b (2, 48000);
            const float a = juce::Decibels::decibelsToGain (dbfs);
            for (int i = 0; i < 48000; ++i) { const float v = a * (float) std::sin (juce::MathConstants<double>::twoPi * 500.0 * i / sr); b.setSample (0, i, v); b.setSample (1, i, v); }
            SoftSaturation::process (b, setting);
            double re = 0, im = 0, re3 = 0, im3 = 0;
            for (int i = 0; i < 48000; ++i)
            {
                const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / 48000.0), y = b.getSample (0, i);
                const double ph = juce::MathConstants<double>::twoPi * 500.0 * i / sr;
                re += w * y * std::cos (ph); im += w * y * std::sin (ph);
                re3 += w * y * std::cos (3 * ph); im3 += w * y * std::sin (3 * ph);
            }
            const double f1 = 4.0 * std::sqrt (re * re + im * im) / 48000.0, f3 = 4.0 * std::sqrt (re3 * re3 + im3 * im3) / 48000.0;
            if (gainDb) *gainDb = juce::Decibels::gainToDecibels (f1 / a);
            if (peakDb) *peakDb = juce::Decibels::gainToDecibels ((double) b.getMagnitude (0, 0, 48000));
            if (h3Db) *h3Db = juce::Decibels::gainToDecibels (f3 / f1, -200.0);
        };
        double g = 0, pk = 0, h3 = 0;
        run (2, -20.0f, &g, nullptr, &h3);
        check (std::abs (g - 3.0) < 0.05, "3 dB mode: quiet material gains exactly +3 dB", g);
        check (h3 < -100.0, "3 dB mode: quiet material is untouched (3rd harmonic, dB)", h3);
        run (4, -20.0f, &g, nullptr, nullptr);
        check (std::abs (g - 6.0) < 0.05, "6 dB mode: quiet material gains exactly +6 dB", g);
        run (2, 0.0f, nullptr, &pk, &h3);
        check (pk <= 0.001, "3 dB mode: a 0 dBFS peak never exceeds full scale (dBFS)", pk);
        std::printf ("      3 dB mode at 0 dBFS: 3rd harmonic %.1f dB\n", h3);
        run (4, 6.0f, nullptr, &pk, nullptr);
        check (pk <= 0.001, "6 dB mode: even +6 dBFS in stays under full scale (dBFS)", pk);
        run (0, -3.0f, &g, &pk, nullptr);
        check (std::abs (g) < 0.01, "Off: no change (dB)", g);
    }

    // ---- Tahor EQ phase modes (Natural / Linear / Linear HQ) + Natural oversampling ----
    {
        std::printf ("\n-- Tahor EQ phase modes --\n");
        auto bandSettings = [] (int phase, int os, CleanBand b0, CleanBand b1 = {}) {
            ChainSettings s = allOff(); s.cleanOn = true; s.cleanPhase = phase; s.cleanOs = os;
            for (auto& b : s.clean) b.on = false;
            s.clean[0] = b0; s.clean[1] = b1; return s; };
        auto gainAt = [] (const ChainSettings& s, double hz, float ampDb = -30.0f) {
            const auto y = renderSine (s, hz, ampDb);
            return toneDb (y, hz) - (double) ampDb; };

        // 1. the analog shapes match the IIR designs where the IIR isn't cramped (around 1 kHz at 8x the rate)
        {
            double worst = 0.0;
            for (int t = 0; t < 6; ++t)
            {
                CleanBand b; b.on = true; b.type = (BandType) t; b.freq = 1000.0f; b.gain = 6.0f; b.q = 1.4f; b.slope = 2;
                const auto d = CleanupEQ::design (b, b.freq, b.gain, b.q, 48000.0 * 8.0);
                for (double hz : { 300.0, 800.0, 1000.0, 1300.0, 3000.0 })
                {
                    const double iir = juce::Decibels::gainToDecibels (CleanupEQ::bandMagnitude (d, hz, 48000.0 * 8.0), -200.0);
                    const double ana = tahorAnalogBandDb (b, b.freq, b.gain, b.q, hz);
                    if (ana > -100.0) worst = juce::jmax (worst, std::abs (iir - ana));
                }
            }
            check (worst < 0.05, "Analog band shapes match the filter designs (worst dB off, all 6 types)", worst);
        }

        CleanBand bell; bell.on = true; bell.type = BandType::Bell; bell.freq = 1000.0f; bell.gain = 6.0f; bell.q = 1.0f;
        // 2. linear mode hits the target gain
        const double g1 = gainAt (bandSettings (1, 0, bell), 1000.0), g2 = gainAt (bandSettings (2, 0, bell), 1000.0);
        const double g3 = gainAt (bandSettings (1, 0, bell), 3000.0), want3 = tahorAnalogBandDb (bell, 1000.0, 6.0, 1.0, 3000.0);
        std::printf ("      +6 dB bell at 1 kHz: Linear %.2f dB, Linear HQ %.2f dB; at 3 kHz %.2f dB (target %.2f)\n", g1, g2, g3, want3);
        check (std::abs (g1 - 6.0) < 0.1 && std::abs (g2 - 6.0) < 0.1, "Linear / Linear HQ: bell hits its gain (worst dB off)", juce::jmax (std::abs (g1 - 6.0), std::abs (g2 - 6.0)));
        check (std::abs (g3 - want3) < 0.15, "Linear: the whole curve follows the analog shape (dB off at 3 kHz)", std::abs (g3 - want3));

        // 3. cramping: a +8 dB bell at 14 kHz, measured at 19 kHz against the analog shape
        {
            CleanBand hb; hb.on = true; hb.type = BandType::Bell; hb.freq = 14000.0f; hb.gain = 8.0f; hb.q = 0.8f;
            const double target = tahorAnalogBandDb (hb, 14000.0, 8.0, 0.8, 19000.0);
            const double e0 = std::abs (gainAt (bandSettings (0, 0, hb), 19000.0) - target);
            const double e2 = std::abs (gainAt (bandSettings (0, 1, hb), 19000.0) - target);
            const double e4 = std::abs (gainAt (bandSettings (0, 2, hb), 19000.0) - target);
            const double eL = std::abs (gainAt (bandSettings (1, 0, hb), 19000.0) - target);
            std::printf ("      +8 dB bell at 14 kHz, error at 19 kHz vs analog: Natural %.2f, 2x %.2f, 4x %.2f, Linear %.2f dB\n", e0, e2, e4, eL);
            check (e2 < e0 * 0.6 && e4 < e2, "Natural oversampling removes high-frequency cramping (dB off at 4x)", e4);
            check (eL < 0.3, "Linear phase has no cramping (dB off)", eL);
        }

        // 4. linear phase really is linear: the impulse response is symmetric around the reported latency
        auto impulse = [] (const ChainSettings& s, int& lat) {
            VocalChain chain; chain.prepare (sr, block);
            const int total = 48000;
            std::vector<float> out ((size_t) total, 0.0f);
            juce::AudioBuffer<float> buf (2, block);
            for (int pos = 0; pos + block <= total; pos += block)
            {
                buf.clear();
                if (pos == 8192) { buf.setSample (0, 0, 1.0f); buf.setSample (1, 0, 1.0f); }
                chain.process (buf, s);
                for (int i = 0; i < block; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
            }
            lat = chain.getLatencySamples();
            return out; };
        {
            CleanBand lc; lc.on = true; lc.type = BandType::LowCut; lc.freq = 100.0f; lc.q = 0.71f; lc.slope = 2;
            for (int phase : { 1, 2 })
            {
                int lat = 0;
                const auto h = impulse (bandSettings (phase, 0, bell, lc), lat);
                const int c = 8192 + lat - 1112 + 1112;   // impulse position + total latency (the chain's other stages are part of lat)
                double peak = 0.0, asym = 0.0;
                for (int k = 0; k < 2000; ++k) peak = juce::jmax (peak, (double) std::abs (h[(size_t) (c + k)]));
                for (int k = 1; k < 2000; ++k) asym = juce::jmax (asym, (double) std::abs (h[(size_t) (c + k)] - h[(size_t) (c - k)]));
                const double asymDb = juce::Decibels::gainToDecibels (asym / peak, -200.0);
                check (asymDb < -60.0, phase == 1 ? "Linear: impulse response is symmetric (no phase shift, dB)" : "Linear HQ: impulse response is symmetric (dB)", asymDb);
            }
        }

        // 5. flat linear mode is a pure delay of exactly the reported latency (and so is the EQ switched off)
        auto nullDb = [] (ChainSettings s) {
            VocalChain chain; chain.prepare (sr, block);
            const int total = 96000;
            std::vector<float> in ((size_t) total), out ((size_t) total);
            for (int i = 0; i < total; ++i) in[(size_t) i] = 0.3f * (float) std::sin (0.05 * i) + 0.2f * (float) std::sin (0.731 * i);
            juce::AudioBuffer<float> buf (2, block);
            for (int pos = 0; pos + block <= total; pos += block)
            {
                for (int i = 0; i < block; ++i) { buf.setSample (0, i, in[(size_t) (pos + i)]); buf.setSample (1, i, in[(size_t) (pos + i)]); }
                chain.process (buf, s);
                for (int i = 0; i < block; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
            }
            const int lat = chain.getLatencySamples();
            double e = 0.0;
            for (int i = total / 2; i < total - block; ++i) e = juce::jmax (e, (double) std::abs (out[(size_t) i] - in[(size_t) (i - lat)]));
            return juce::Decibels::gainToDecibels (e / 0.5, -200.0); };
        {
            ChainSettings flat = bandSettings (1, 0, CleanBand {});
            ChainSettings off  = bandSettings (2, 0, bell); off.cleanOn = false;
            ChainSettings nat4 = bandSettings (0, 2, CleanBand {});
            const double nf = nullDb (flat), no = nullDb (off), n4 = nullDb (nat4);
            std::printf ("      nulls vs reported latency: Linear flat %.1f dB, Linear HQ switched off %.1f dB, Natural 4x flat %.1f dB\n", nf, no, n4);
            check (juce::jmax (nf, no) < -70.0, "Linear modes: flat / off is an exact delay of the reported latency (dB)", juce::jmax (nf, no));
            check (n4 < -40.0, "Natural 4x: time-aligned with the reported latency (dB)", n4);
        }

        // 6. latencies
        {
            VocalChain chain; chain.prepare (sr, block);
            ChainSettings s = allOff();
            const int base = chain.latencyFor (s);
            s.cleanPhase = 1; const int lin = chain.latencyFor (s) - base;
            s.cleanPhase = 2; const int hq  = chain.latencyFor (s) - base;
            s.trackMode = true; const int tr = chain.latencyFor (s);
            std::printf ("      added latency at 48 kHz: Linear %d samples (%.0f ms), Linear HQ %d samples (%.0f ms), TRACK total %d\n",
                         lin, lin / 48.0, hq, hq / 48.0, tr);
            check (lin == 2304 && hq == 8448 && tr == 0, "Phase-mode latencies (Linear, samples)", lin);
        }

        // 7. dynamic bands still work in linear mode (loud = turned down, quiet = untouched)
        {
            CleanBand db = bell; db.freq = 3000.0f; db.gain = 0.0f; db.q = 2.0f; db.dyn = true; db.dynThresh = -30.0f; db.dynRange = -6.0f;
            const double loud = gainAt (bandSettings (1, 0, db), 3000.0, -6.0f);
            const double quiet = gainAt (bandSettings (1, 0, db), 3000.0, -60.0f);
            std::printf ("      dynamic bell (-6 dB range) in Linear: loud %.2f dB, quiet %.2f dB\n", loud, quiet);
            check (loud < -5.0 && std::abs (quiet) < 0.2, "Linear: dynamic band turns loud tones down, leaves quiet ones (dB)", loud);
        }

        // 8. switching the EQ on/off and dragging a band in linear mode: no clicks
        {
            ChainSettings s = bandSettings (1, 0, bell);
            VocalChain chain; chain.prepare (sr, block);
            juce::AudioBuffer<float> buf (2, block); double ph = 0.0; float prev = 0.0f, worst = 0.0f;
            for (int blk = 0; blk < 300; ++blk)
            {
                s.cleanOn = (blk / 60) % 2 == 0;
                s.clean[0].gain = (float) (12.0 * std::sin (blk * 0.05));
                s.clean[0].freq = (float) (1000.0 * std::pow (2.0, std::sin (blk * 0.03)));
                for (int i = 0; i < block; ++i) { const float v = 0.3f * (float) std::sin (ph); ph += juce::MathConstants<double>::twoPi * 200.0 / sr; buf.setSample (0, i, v); buf.setSample (1, i, v); }
                chain.process (buf, s);
                for (int i = 0; i < block; ++i) { if (blk > 20) worst = juce::jmax (worst, std::abs (buf.getSample (0, i) - prev)); prev = buf.getSample (0, i); }
            }
            check (worst < 0.05f, "Linear: switching and dragging stays click-free (largest sample step)", worst);
        }
    }

    // ---- Oversampling (tape + limiter stage) ------------------------------------
    {
        std::printf ("\n-- Oversampling --\n");
        // latency at every setting, and the output really is delayed by exactly that much
        auto nullAt = [] (int tapeOs, int limOs, bool tapeOn, int* latOut) -> double
        {
            ChainSettings s = allOff(); s.tapeOs = tapeOs; s.limOs = limOs; s.tapeOn = tapeOn; s.tapeDrive = 0.0f; s.tapeMix = 0.0f;
            VocalChain chain; chain.prepare (sr, block);
            const int total = (int) sr;
            std::vector<float> in ((size_t) total), out ((size_t) total);
            for (int i = 0; i < total; ++i) in[(size_t) i] = 0.25f * (float) std::sin (juce::MathConstants<double>::twoPi * 200.0 * i / sr);
            juce::AudioBuffer<float> buf (2, block);
            for (int pos = 0; pos + block <= total; pos += block)
            {
                for (int i = 0; i < block; ++i) { buf.setSample (0, i, in[(size_t) (pos + i)]); buf.setSample (1, i, in[(size_t) (pos + i)]); }
                chain.process (buf, s);
                for (int i = 0; i < block; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
            }
            const int lat = chain.getLatencySamples();
            if (latOut) *latOut = lat;
            double worst = 0.0;
            for (int i = total / 2; i < total - block; ++i) worst = juce::jmax (worst, (double) std::abs (out[(size_t) i] - in[(size_t) (i - lat)]));
            return juce::Decibels::gainToDecibels (worst / 0.25, -200.0);
        };
        int l11 = 0, l21 = 0, l12 = 0, l22 = 0, l00 = 0;
        const double n00 = nullAt (0, 0, false, &l00);
        const double n11 = nullAt (1, 0, false, &l11);   // the default
        const double n21 = nullAt (2, 1, false, &l21);
        const double n12 = nullAt (1, 2, false, &l12);
        const double n22 = nullAt (2, 2, false, &l22);
        std::printf ("      latency (samples): tape 1x/lim 1x %d, default (2x/1x) %d, 4x/2x %d, 2x/4x %d, 4x/4x %d\n", l00, l11, l21, l12, l22);
        check (l11 == 1112, "Default oversampling keeps the old latency (samples)", l11);
        check (l00 < l11 && l22 > l21 && l22 > l12, "Latency grows with oversampling (4x/4x, samples)", l22);
        const double worstNull = std::max ({ n00, n11, n21, n12, n22 });
        check (worstNull < -40.0, "Every oversampling setting is time-aligned with its reported latency (worst null, dB)", worstNull);
        {
            ChainSettings s = allOff(); s.trackMode = true; s.tapeOs = 2; s.limOs = 2;
            VocalChain chain; chain.prepare (sr, block);
            check (chain.latencyFor (s) == 0, "TRACK mode ignores oversampling (latency, samples)", chain.latencyFor (s));
        }

        // aliasing: a 7 kHz tone pushed into 6 dB of soft saturation. Its 5th harmonic (35 kHz) folds to 13 kHz
        // and its 7th (49 kHz) folds to 1 kHz at 1x; oversampling should push those fold-backs way down.
        auto aliasDb = [] (int limOs, double* peak)
        {
            ChainSettings s = allOff(); s.limOn = true; s.limSoft = 4; s.limThreshDb = 0.0f; s.limCeilingDb = -1.0f; s.limOs = limOs;
            const auto y = renderSine (s, 7000.0, -1.0f);
            if (peak) { double m = 0; for (auto v : y) m = juce::jmax (m, (double) std::abs (v)); *peak = juce::Decibels::gainToDecibels (m, -200.0); }
            return juce::jmax (toneDb (y, 13000.0), toneDb (y, 1000.0));
        };
        double p1 = 0, p2 = 0, p4 = 0;
        const double a1 = aliasDb (0, &p1), a2 = aliasDb (1, &p2), a4 = aliasDb (2, &p4);
        std::printf ("      7 kHz into 6 dB soft saturation, worst fold-back: 1x %.1f dB, 2x %.1f dB, 4x %.1f dB\n", a1, a2, a4);
        check (a2 < a1 - 15.0, "2x oversampling cuts aliasing (dB less fold-back)", a1 - a2);
        check (a4 < a1 - 25.0, "4x oversampling cuts aliasing too (dB less fold-back)", a1 - a4);
        check (juce::jmax (p1, p2, p4) <= -1.0 + 0.001, "Ceiling still holds at every oversampling setting (peak dBFS)", juce::jmax (p1, p2, p4));

        // tape: an 11 kHz tone (bright cymbal / "s" territory) driven hard at 30 ips (open top). At 1x its 3rd, 5th
        // and 7th harmonics fold back to 15, 7 and 19 kHz; at 2x the 7th (77 kHz) still folds to 19 kHz; 4x clears it.
        auto tapeAliasDb = [] (int tapeOs)
        {
            ChainSettings s = allOff(); s.tapeOn = true; s.tapeStyle = 0; s.tapeDrive = 1.0f; s.tapeMix = 1.0f; s.tapeOs = tapeOs;
            const auto y = renderSine (s, 11000.0, -3.0f);
            return std::max ({ toneDb (y, 3000.0), toneDb (y, 7000.0), toneDb (y, 15000.0), toneDb (y, 19000.0) });
        };
        const double t1 = tapeAliasDb (0), t2 = tapeAliasDb (1), t4 = tapeAliasDb (2);
        std::printf ("      11 kHz driven into 30 ips tape, worst fold-back: 1x %.1f dB, 2x %.1f dB, 4x %.1f dB\n", t1, t2, t4);
        check (t2 < t1 - 6.0, "Tape 2x cuts aliasing on bright material (dB less fold-back)", t1 - t2);
        check (t4 < t2 - 6.0, "Tape 4x cuts it further (dB less than 2x)", t2 - t4);
    }

    // ---- Tape --------------------------------------------------------------
    {
        std::printf ("\n-- Tape --\n");
        // harmonic level (dB relative to the fundamental) of a tone through the tape alone
        int tapeWidthForRun = 2;   // 1" (reference) unless a test changes it
        auto tapeRun = [&tapeWidthForRun] (int style, float drive, double hz, float ampDb, bool on, double* h2, double* h3, double* gainDb)
        {
            ChainSettings s = allOff();
            s.tapeOn = on; s.tapeStyle = style; s.tapeDrive = drive; s.tapeMix = 1.0f; s.tapeWidth = tapeWidthForRun;
            VocalChain chain; chain.prepare (sr, block);
            const float amp = juce::Decibels::decibelsToGain (ampDb);
            juce::AudioBuffer<float> buf (2, block);
            std::vector<float> out; double phase = 0.0;
            for (int pos = 0; pos < (int) sr; pos += block)
            {
                for (int i = 0; i < block; ++i) { const float v = amp * (float) std::sin (phase); phase += juce::MathConstants<double>::twoPi * hz / sr; buf.setSample (0, i, v); buf.setSample (1, i, v); }
                chain.process (buf, s);
                if (pos >= (int) sr / 2) for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
            }
            auto goertzel = [&] (double f)
            {
                double re = 0, im = 0;
                for (size_t i = 0; i < out.size(); ++i) { const double ph = juce::MathConstants<double>::twoPi * f * (double) i / sr; const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * (double) i / (double) out.size()); re += w * out[i] * std::cos (ph); im += w * out[i] * std::sin (ph); }
                return 4.0 * std::sqrt (re * re + im * im) / (double) out.size();   // Hann window, amplitude-corrected
            };
            const double f1 = goertzel (hz);
            if (h2) *h2 = juce::Decibels::gainToDecibels (goertzel (2 * hz) / f1, -200.0);
            if (h3) *h3 = juce::Decibels::gainToDecibels (goertzel (3 * hz) / f1, -200.0);
            if (gainDb) *gainDb = juce::Decibels::gainToDecibels (f1 / amp, -200.0);
        };
        double g1k = 0, g70 = 0, g15k = 0, h2 = 0, h3 = 0, h2v = 0, h3v = 0, h2off = 0, h3off = 0, gv = 0;
        tapeRun (0, 0.5f, 1000.0, -30.0f, true, nullptr, nullptr, &gv);
        check (std::abs (gv) < 0.5, "Tape 30 ips: quiet 1 kHz passes at unity (dB)", gv);
        tapeRun (1, 0.5f, 1000.0, -30.0f, true, nullptr, nullptr, &g1k);
        tapeRun (1, 0.5f, 55.0,   -30.0f, true, nullptr, nullptr, &g70);
        check (g70 - g1k > 1.5 && g70 - g1k < 2.5, "Tape 15 ips: head bump at 55 Hz (dB above 1 kHz)", g70 - g1k);
        double b30 = 0, b30k = 0;
        tapeRun (0, 0.5f, 90.0, -30.0f, true, nullptr, nullptr, &b30);
        tapeRun (0, 0.5f, 1000.0, -30.0f, true, nullptr, nullptr, &b30k);
        check (b30 - b30k > 0.4 && b30 - b30k < 1.2, "Tape 30 ips: small head bump at 90 Hz (dB)", b30 - b30k);
        tapeRun (2, 0.5f, 15000.0, -30.0f, true, nullptr, nullptr, &g15k);
        check (g15k < -3.0, "Tape 7.5 ips: smooth top, 15 kHz rolled off (dB)", g15k);
        tapeRun (1, 0.5f, 220.0, -6.0f, true, &h2, &h3, nullptr);
        tapeRun (0, 0.5f, 220.0, -6.0f, true, &h2v, &h3v, nullptr);
        tapeRun (1, 0.5f, 220.0, -6.0f, false, &h2off, &h3off, nullptr);
        std::printf ("      220 Hz at -6 dBFS: 15 ips 2nd %.1f / 3rd %.1f dB, 30 ips 2nd %.1f / 3rd %.1f dB, off 3rd %.1f dB\n", h2, h3, h2v, h3v, h3off);
        check (h3 > -40.0 && h3 < -15.0, "Tape 15 ips adds audible but musical harmonics (3rd, dB)", h3);
        check (h3v < h3 - 3.0, "Tape 30 ips is gentler than 15 ips (3rd, dB)", h3v);
        check (h3off < -100.0, "Tape off adds nothing (3rd, dB)", h3off);
        double h3hi = 0, h3lo = 0;
        tapeRun (1, 1.0f, 220.0, -6.0f, true, nullptr, &h3hi, nullptr);
        tapeRun (1, 0.0f, 220.0, -6.0f, true, nullptr, &h3lo, nullptr);
        check (h3hi > h3 + 6.0 && h3lo < h3 - 6.0, "Tape Drive changes the amount of tape character (dB span)", h3hi - h3lo);

        // tape width: 1/4" saturates soonest, 1/2" is cleanest; 2" has the fullest bump
        {
            double h3q = 0, h3h = 0, b1 = 0, b2 = 0, ref1k = 0;
            tapeWidthForRun = 0; tapeRun (1, 0.5f, 220.0, -6.0f, true, nullptr, &h3q, nullptr);
            tapeWidthForRun = 1; tapeRun (1, 0.5f, 220.0, -6.0f, true, nullptr, &h3h, nullptr);
            std::printf ("      15 ips 3rd harmonic: 1/4\" %.1f dB, 1\" %.1f dB, 1/2\" %.1f dB\n", h3q, h3, h3h);
            check (h3q > h3 + 2.0 && h3h < h3 - 2.0, "Tape width: 1/4\" grittiest, 1/2\" cleanest (dB span)", h3q - h3h);
            tapeWidthForRun = 2; tapeRun (1, 0.5f, 55.0, -30.0f, true, nullptr, nullptr, &b1); tapeRun (1, 0.5f, 1000.0, -30.0f, true, nullptr, nullptr, &ref1k);
            tapeWidthForRun = 3; tapeRun (1, 0.5f, 50.0, -30.0f, true, nullptr, nullptr, &b2);
            check ((b2 - ref1k) > (b1 - ref1k) + 0.5, "Tape width: 2\" has a fuller low-end bump (dB)", b2 - ref1k);
            tapeWidthForRun = 2;
        }

        // switching the tape on mid-signal: no click
        {
            ChainSettings s = allOff(); s.tapeStyle = 1;
            VocalChain chain; chain.prepare (sr, block);
            juce::AudioBuffer<float> buf (2, block); double phase = 0.0; float prev = 0.0f, worst = 0.0f;
            for (int blk = 0; blk < 60; ++blk)
            {
                s.tapeOn = blk >= 30;
                for (int i = 0; i < block; ++i) { const float v = 0.3f * (float) std::sin (phase); phase += juce::MathConstants<double>::twoPi * 150.0 / sr; buf.setSample (0, i, v); buf.setSample (1, i, v); }
                chain.process (buf, s);
                for (int i = 0; i < block; ++i) { if (blk > 2) worst = juce::jmax (worst, std::abs (buf.getSample (0, i) - prev)); prev = buf.getSample (0, i); }
            }
            check (worst < 0.05f, "Tape switches in smoothly (largest sample step)", worst);
        }

        // tape noise: hiss on silence, level follows speed and width, stereo is decorrelated, off = silent
        {
            auto hissRun = [&] (int style, int width, bool on, double* corr) -> double
            {
                ChainSettings s = allOff(); s.tapeOn = true; s.tapeStyle = style; s.tapeWidth = width; s.tapeNoise = on;
                VocalChain chain; chain.prepare (sr, block);
                juce::AudioBuffer<float> buf (2, block);
                double e = 0.0, ll = 0.0, rr = 0.0, lr = 0.0; long cnt = 0;
                for (int blk = 0; blk < 200; ++blk)
                {
                    buf.clear();
                    chain.process (buf, s);
                    if (blk < 40) continue;
                    for (int i = 0; i < block; ++i)
                    {
                        const double l = buf.getSample (0, i), r = buf.getSample (1, i);
                        e += l * l + r * r; ll += l * l; rr += r * r; lr += l * r; cnt += 2;
                    }
                }
                if (corr) *corr = lr / std::sqrt (juce::jmax (1.0e-30, ll * rr));
                return 10.0 * std::log10 (juce::jmax (1.0e-30, e / (double) cnt));
            };
            double corr = 0.0;
            const double fast = hissRun (0, 1, true, &corr);    // 30 ips, 1/2"
            const double slow = hissRun (2, 0, true, nullptr);  // 7.5 ips, 1/4"
            const double mid  = hissRun (1, 2, true, nullptr);  // 15 ips, 1"
            const double off  = hissRun (2, 0, false, nullptr);
            std::printf ("      hiss: 30 ips 1/2\" %.1f dB, 15 ips 1\" %.1f dB, 7.5 ips 1/4\" %.1f dB, off %.1f dB, L/R correlation %.3f\n", fast, mid, slow, off, corr);
            check (fast < -66.0 && fast > -80.0, "Tape noise 30 ips 1/2\": quiet hiss (dBFS RMS)", fast);
            check (slow > -60.0 && slow < -48.0, "Tape noise 7.5 ips 1/4\": louder hiss (dBFS RMS)", slow);
            check (slow > mid + 3.0 && mid > fast + 3.0, "Tape noise rises as the tape slows and narrows (dB span)", slow - fast);
            check (off < -140.0, "Tape noise off: silence stays silent (dBFS)", off);
            check (std::abs (corr) < 0.05, "Tape noise is independent per channel (correlation)", corr);
        }
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
