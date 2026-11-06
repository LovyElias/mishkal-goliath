# Mishkal Goliath — mixing and mastering plugin

*Mishkal* (מִשְׁקָל) is Hebrew for weight or balance. It also means meter or rhythm in poetry and music.

One plugin, eight stages, in this order:

**Peak catch → Resonance suppressor → Tahor EQ → Character EQ → Gate → Breath control → Vocal rider → Compressor → De-esser → Zerem → Tape → Limiter**

The Character EQ can be moved after the compressors, and the units inside the Character EQ and Compressor
sections can be put in any order (see "Changing the order" below).

Formats: **VST3** and **AU** for Ableton Live, **AAX** for Pro Tools, plus a Standalone app for quick tests.
Mono and stereo tracks are supported.

---

## 1. Build it on your Mac (one time, about 15 minutes)

You need two free tools:

1. **Xcode** from the Mac App Store. Open it once so it finishes installing components.
2. **CMake** from https://cmake.org/download (the macOS `.dmg`). After installing, open the CMake app and choose
   *Tools → How to Install For Command Line Use*, then follow the one-line instruction it shows.
   If you use Homebrew, `brew install cmake` works too.

Then open **Terminal**, drag the `Mishkal` folder into the window after typing `cd ` and press Return:

```bash
cd /path/to/Mishkal
cmake -B build -G Xcode
cmake --build build --config Release
```

The first build downloads JUCE (the plugin framework) and takes 5–15 minutes. Later builds are much faster.

When it finishes:

- **VST3** and **AU** are already copied into `~/Library/Audio/Plug-Ins/`
- **AAX** is waiting in `build/AAX-ready-to-install/`
- The **Standalone** app is in `build/Mishkal_artefacts/Release/Standalone/`

## 2. Ableton Live

1. Live → Settings → **Plug-Ins**
2. Turn on **Use VST3 Plug-In System Folders** (and **Use Audio Units** if you'd rather use the AU)
3. Click **Rescan**
4. Find **Mishkal Goliath Strip** under Plug-Ins → **Mishkal Audio**

## 3. Pro Tools (AAX)

Copy the AAX into the Avid folder (this asks for your Mac password):

```bash
sudo cp -R "build/AAX-ready-to-install/Mishkal Goliath Strip.aaxplugin" "/Library/Application Support/Avid/Audio/Plug-Ins/"
```

**Important:** regular Pro Tools only loads AAX plugins that are digitally signed through PACE.
You have two options:

- **For yourself:** join the free Avid Developer program (developer.avid.com) and install
  **Pro Tools Developer**, which loads unsigned plugins. Easiest way to test.
- **For regular Pro Tools / sharing:** apply for PACE signing through the Avid developer portal.
  PACE then gives you their `wraptool` and an iLok-based signing account. Sign the plugin with:
  ```bash
  wraptool sign --verbose --account YOUR_PACE_ACCOUNT --wcguid YOUR_WRAP_GUID \
    --signid "Developer ID Application: YOUR NAME" \
    --in "build/AAX-ready-to-install/Mishkal Goliath Strip.aaxplugin" \
    --out "build/AAX-ready-to-install/Mishkal Goliath Strip.aaxplugin"
  ```

Until then, use the VST3/AU in Ableton to dial it in.

---

## 4. Top bar, presets and skins

**Window size:** drag the bottom-right corner to any size (60–160%; the shape stays locked so nothing stretches), or
pick one from ⋮ menu → Size. 100% is 1332 × 970; 75% (999 × 728) fits a laptop screen. The size is saved with your session.


| Control | What it does |
|---|---|
| **Undo / Redo** | Steps back and forward through your changes |
| **◀ preset ▶** | Browse presets. Click the name for the full list. A `*` means you've changed it since loading |
| **SAVE / SAVE AS** | Save your own presets. SAVE overwrites the current user preset; factory presets always go to SAVE AS |
| **A > B** | Two complete settings to compare. Click **A** or **B** to switch, **>** copies the active one to the other |
| **⋮ menu** | Size (75 / 90 / 100 / 115 / 125%), Skin, Reset to Default, Open Presets Folder |
| **TRACK** | Low-latency tracking mode, for singers recording *through* Mishkal. Latency drops from about 23 ms to zero: the resonance suppressor pauses (it needs 21 ms to listen) and the peak catcher, tape and limiter switch to zero-latency versions (instant attack, no look-ahead; the limiter still never passes its ceiling). The lamp glows green. Switch it off for mixing; best changed while stopped |
| **MATCH** | Level match for fair before/after comparisons. It keeps measuring how much louder or quieter the processed vocal is than the raw one (K-weighted, like LUFS, over about 3 s) and shows it on the button, e.g. `MATCH +2.4`. With MATCH on, pressing BYPASS plays the raw vocal at the processed loudness, so you hear only what the plugin changes, not just "louder". Your processed output is never changed |
| **BYPASS** | Takes the whole plugin out (the lamp glows red and the sections dim). Fades over 30 ms and stays in time, so you can A/B against the raw vocal cleanly. Your DAW's own bypass button does the same thing |

**A fresh insert is a clean slate:** a flat, unity-gain, all-purpose mixing and mastering chain. Every section is ready but
switched off (the Tahor EQ is on but flat, the console EQ's high-pass sits at 20 Hz, Zerem waits on its Mix Bus style),
except the resonance suppressor (Reso), which starts on and only touches harsh resonances. Switch Reso off and the plugin
is perfectly transparent: tested to match its input exactly, just delayed by the reported latency. That's also what the **Default** preset loads. The original vocal-strip starting point is
**Lead Vocals → Vocal Starter**.

Factory presets: **91 starting points** (plus Default), grouped into submenus in the preset menu. All of them are
set from scratch (every section's power switch included), so loading one never leaves leftovers from the last.

| Category | Presets |
|---|---|
| **Lead Vocals** (15) | Vocal Starter, Pop Vocal, Modern Pop Rider, Rap Upfront, Rap Tape Grit, Gentle Polish, R&B Silk, Trap Lead Bright, Rock Vocal Grit, Country Clear, Indie Intimate, Gospel Power, Afrobeats Lead, Female Pop Air, Male Pop Body |
| **Backing Vocals** (5) | Stacked Harmonies, Airy Doubles, Gang Shouts, Ad-libs Thin, Choir Wash |
| **Ballads & Warmth** (5) | Worship Ballad, Tube Warmth, Soul Warm, Jazz Croon, Acoustic Singer-Songwriter |
| **Spoken Word** (5) | Podcast Voice, Radio Host, Audiobook Narration, Voiceover Punchy, Interview Cleanup |
| **Drums** (12) | Kick Punch, Kick Sub Boom, Snare Crack, Snare Fat, Toms Tight, Hi-Hat Smooth, Overheads Air, Room Smash, Drum Bus Glue, Drum Bus Parallel Crush, Percussion Shine, Breakbeat Vintage |
| **Bass** (5) | Bass DI Solid, Bass Amp Growl, 808 Control, Synth Bass Tight, Upright Bass Woody |
| **Guitars** (8) | Acoustic Sparkle, Acoustic Strum Bed, Fingerpicked Warmth, Electric Rhythm Crunch, Electric Lead Smooth, Clean Funk, Heavy Rhythm Wall, Guitar Bus Tape |
| **Keys & Synths** (8) | Piano Clear, Piano Pop Bright, Piano Vintage Warm, Rhodes Warmth, Organ Body, Synth Pad Smooth, Synth Lead Bite, Pluck Sparkle |
| **Strings, Brass & Winds** (6) | Strings Silky, Solo Violin, Cello Rich, Brass Bite, Sax Smooth, Flute Air |
| **Buses** (5) | Mix Bus Glue, Mix Bus Tape Glue, Vocal Bus Sheen, Music Bus Warm, Parallel Vocal Crush |
| **Mastering** (11) | Master Transparent, Master Loud Pop, Master Hip-Hop Punch, Master Warm Tape, Master Streaming -14 LUFS, Master Rock Glue, Master Acoustic Gentle, Master EDM Loud, Master Vintage Vinyl, Master Gospel Live, Master De-Harsh |
| **Creative FX** (6) | Telephone Voice, AM Radio, Lo-Fi Cassette, Megaphone, Dark Underwater, Parallel Saturation Sauce |

How they're built:
- **Instrument presets** switch off the vocal-only tools (de-esser, breath control, rider) and open up the low end
  (kick, bass and mastering presets set the console EQ's high-pass to 20 Hz). Drums use the gate, slower FET attacks
  that let hits through, and the 1964 PK peak catcher on spiky sources.
- **Bus and mastering presets** use the LE-660 vari-mu with a sidechain HPF (no low-end pumping), gentle Zerem Mix Bus,
  tape where it fits, and the limiter's Multi mode and soft saturation for loudness. For mastering, also switch the
  Tahor EQ to **Linear** and the Limiter OS to **2x**: those are session settings, so presets don't change them.
- Presets that use **LE-RC** set its Gain to roughly restore the level the compression takes away; trim with **Output**
  or turn on **MATCH** to compare fairly.
- Every preset is checked automatically: real parameters only, values in range, and real audio through each one with
  no clipping and a sensible level.

**Save As** has an optional **Category** field: presets saved with one appear in their own submenu.
Presets never change TRACK, MATCH, BYPASS or the oversampling / phase-mode settings (they're session settings, not part of a sound).
⋮ menu → **Keep Input/Output when changing presets** keeps your gain staging while you browse.
User presets are saved as `.mishkalpreset` files in `~/Library/Application Support/Mishkal Goliath Strip/Presets`,
so you can back them up or share them. Loading a preset never changes the skin.

Skins (⋮ menu → Skin; saved with your session):

| Skin | Look |
|---|---|
| **Goliath** (default) | The Rack Edition designed in Figma. A black display across the top, with the Tahor EQ graph, the input and output meters and a scrolling limiter view (grey is the output level, red is what the limiter takes off). Below it sits a row of brushed-black 500-series modules: Tahor band controls, Character EQ, Dynamics with a big Threshold knob, De-esser, Zerem and Tape stacked together, and the Limiter. The modules have cross-head screws, sage titles on black tabs, black knobs with numbered dot scales, and amber LED selectors and bat-handle toggle switches. Text uses the built-in Barlow (display) and Fira Sans (modules) fonts. The window is 1440 x 1130 at 100% (use 75% or 90% on a laptop) |
| **Onyx** | Matte black and gunmetal gray with two accents: red for power lamps, gain reduction and resonance cuts; yellow for selected modes, lit buttons and band labels. Vintage black skirted knobs with knurled caps and white index lines, in the style of classic boutique EQs, set in printed white scales |
| **Rack** | Black rack unit around the analyzer, charcoal faceplates with slanted dividers, colour-coded grip knobs with number scales (red lows, yellow mids, blue highs, silver output), jewel lamps, slide switches, orange LED meters |
| **Hardware** | Brushed gunmetal faceplates with screws, cream legends, black bakelite knobs, LED ladders |
| **Modern** | Deep ink background with soft glows, glass panels, glowing arcs |
| **Royal** | Deep purple with warm gold, gold-rimmed knobs, candlelight glow |
| **British Console** | Grey-blue enamel faceplates, white legends, grey skirted knobs with red caps: the classic big-desk channel-strip feel |
| **Mastering Silver** | Brushed aluminium, black engraving, machined silver knobs, amber meters |
| **Vintage Cream** | Cream enamel panels on a black frame, black bakelite knobs, valve-glow lamps, like 1950s–60s tube gear |
| **Broadcast Green** | Army-green enamel, black knobs, cream legends, like old broadcast and tape gear |
| **Studio White** | Light, flat and clean, with dark screens and bright accents: for bright rooms and daytime sessions |
| **Midnight Glow** | Near-black with neon cyan, magenta and yellow glows |

The last six are trial skins: try them all, then we keep the favourites.

**Mode lights.** Each mode keeps the skin's black look and shows its identity through its light colour:
the EQ tab lights are LE-EP1 deep blue, LE-E4K green, LE-N81 red and LE-SP1 cyan, and the compressor's
selected mode glows violet (LE-76), orange (LE-RC) or gold (LE-660).

Hover a node on the graph to see its type, frequency and gain.
Colours for every skin live in `Source/Skin.h`.

## 5. What each section does

### Input and Output meters (window edges)
Tall stereo meters run down the **left edge (Input)** and **right edge (Output)**, each with its knob on top.
- **Input** sets the level going into the chain; its meter shows the level *after* the knob, so you can see the
  effect of your trim right away. **Output** is the final trim; its meter shows exactly what leaves the plugin.
- Green up to −12 dB, yellow up to −3 dB, red above. A thin line holds each peak for 1.5 s.
- A red **clip light** above each bar latches if that side reaches 0 dBFS. The number at the bottom is the
  **highest peak** since you last looked. **Click the meter** to reset both.
- A good target: input peaks around −10 to −6 dB on the loudest words.
- **LUFS** (bottom of the Output strip): loudness to the ITU BS.1770 / EBU R128 standard, the way streaming services
  measure it. **S** = short-term (last 3 s), **I** = integrated (the whole song since the last reset, with the standard's
  gating so silences and very quiet parts don't drag it down). Click the LUFS box to reset the integrated reading.
- **1964 PK** (under the Input knob): an ultra-fast input peak catcher in the spirit of the Spectra Sonics 610
  Complimiter's limiter. It sits right after the Input knob, before the resonance suppressor and Tahor EQ, with a
  fixed catch point at −6 dB, so it works off your input level exactly like the hardware: drive the Input knob
  harder to catch more. A 0.25 ms look-ahead grabs each peak before it arrives, with no overshoot. Release is fast
  (about 10 ms) after a lone spike and slows down when it's working constantly, so it never pumps.
  The lamp glows yellow when on and flashes red each time it grabs a peak. The Input meter shows the level going
  *into* the catcher, so you can see the peaks it's taking care of.

### Resonance suppressor (soothe style, very first)
Click **RESO** at the top right of the Tahor EQ section. It finds frequencies that ring out above the
voice's natural spectrum (boxy notes, nasal rings, harsh 2–5 kHz spikes) and turns down only those,
moment to moment. What it's removing is drawn live in pink on the graph.

| Control | What it does |
|---|---|
| **Depth** | How hard resonances are pulled down (up to 24 dB at 100%). Start at 30–50% |
| **Sharpness** | Higher = only narrow, ringing peaks. Lower = broader buildups too |
| **Selectivity** | Higher = only the most obvious resonances. Lower = treats more |
| **Attack / Release** | How fast each frequency reacts and recovers |
| **Low / High** | Focus range. Nothing outside it is touched (default 300 Hz–12 kHz) |
| **Mix** | Blend with the untreated signal |
| **DELTA** | Hear only what's being removed. If you hear the voice itself, back Depth off |
| **ON** | Its own on/off, separate from the Tahor EQ power |

It works on short FFT frames, so it adds 1024 samples of latency (about 21 ms at 48 kHz), which your DAW
compensates for automatically. It keeps running when switched off so the latency never changes mid-session.

### Tahor EQ (Pro-Q style)
*Tahor* (טָהוֹר) is Hebrew for pure or clean: this is the surgical EQ that cleans the vocal up.

Eight surgical bands on an interactive graph with a live spectrum analyzer
(grey line = what comes in, blue fill = what goes out).

- **Double-click** the graph to add a band. Near the far left it makes a low cut, near the far right a high cut.
- **Drag** a node to move frequency and gain. **Scroll** over it to change Q.
- **Right-click** a node for type, slope, Dynamic, or Delete. **Double-click** a node to remove it.
- Types: Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch. Cuts go 12 / 24 / 36 / 48 dB/oct.
- **Dynamic** (bells and shelves): the band only moves when that frequency gets loud. Set **Threshold**
  and **Range** (e.g. −6 dB) to tame a resonance or a harsh "ee" that only shows up on loud notes.
  The ring on the graph shows it moving live.
- The panel on the right edits the selected band.
- **SOLO** (band panel, next to ON): hear only the frequencies around that band, to pinpoint a boxy, ringing or
  harsh spot. Bells and notches solo their own width (Q); shelves and cuts solo a region around their corner.
  The slice you're hearing is highlighted on the graph. While solo is on it follows whichever band you select.
  Shortcut: **Option-click and hold** a band's dot to solo it while you drag, released when you let go.
  Solo is never saved with a session, so you can't leave it on by accident.
- **Character EQ curves:** each active character EQ is drawn as a glowing line in its own colour (LE-EP1 blue, LE-E4K green,
  LE-N81 red, LE-SP1 cyan), with a key in the top-left corner. When several are stacked, a dashed line shows
  their combined effect.

#### Phase mode and oversampling (Tahor EQ header)
| Mode | What it does | Added latency at 48 kHz |
|---|---|---|
| **Natural** (default) | Zero-latency filters that behave like analog EQ: a little phase shift around each band | none |
| **Linear** | No phase shift at all, so lows and highs stay perfectly in time: drums, bass, buses, mastering, parallel chains | 2304 samples (48 ms) |
| **Linear HQ** | Same, with 4x the low-frequency resolution for steep low cuts and narrow low bands | 8448 samples (176 ms) |

- **OS Off / 2x / 4x** (Natural mode only): oversampling removes high-frequency "cramping", where digital bells and
  shelves near 20 kHz get squashed. Measured on a +8 dB bell at 14 kHz, 3.7 dB off the true shape at 19 kHz with OS off,
  0.6 dB at 2x and 0.1 dB at 4x. Adds 4 / 6 samples of latency.
- **Linear modes** are built from the exact analog band shapes (0.00 dB off), so they need no oversampling: OS greys out.
  They run as a partitioned FFT convolution; new curves crossfade in, so dragging bands never clicks. The impulse
  response is perfectly symmetric (no phase shift, measured −146 dB asymmetry).
- **Dynamic bands** keep working in Linear modes: the detectors listen to the input, and their gain moves are delayed
  to land exactly on the delayed audio.
- Linear phase can add a little **pre-ring** (a soft "pre-echo") on very sharp, heavy boosts: that's the physics of linear
  phase, so use Natural for big, narrow boosts on transient material.
- Like the other quality settings, presets and A/B never change these, they're best changed while playback is
  stopped, and **TRACK** always uses Natural with OS off (zero latency). The graph always draws the exact curve of the
  mode you're in.

### Character EQ (four flavours, second slot, stackable)
The four EQs carry LE names: **LE-EP1** (Pultec-style tube program EQ), **LE-E4K** (British E-series console style),
**LE-N81** (Class A 1081 style) and **LE-SP1** (AirEQ style, with Silk).

Each EQ has **its own settings** and **its own activation light**:
- **Click the light** on a tab to switch that EQ in or out. Use one, any two, three, or all four.
- **Click the name** to show and edit that EQ's knobs. The knobs dim if the EQ you're editing is switched out.
- Active EQs run in series, **left to right in the order of the tabs** (default **LE-EP1 → LE-E4K → LE-N81 → LE-SP1**).
  By default only LE-E4K is on.
- Each active character EQ shows on the Tahor EQ graph in its own colour, so you can see what each one is doing.

| Mode | Behaviour |
|---|---|
| **LE-EP1** | Pultec EQP-1A-style program EQ with its own panel. **Low**: Freq (20/30/60/100 Hz), Boost and Atten (0–10). Turn both up for the classic trick: lows swell while the mud just above dips. **High Boost**: Freq (3–16 kHz), Bandwidth (sharp → broad), Boost. **High Atten**: Freq (5/10/20 kHz), Atten. Tube warmth built in |
| **LE-E4K** | British console style: 18 dB/oct high-pass, constant-Q bells, firmer shelves, faint console grit |
| **LE-N81** | 1081 style: bells get *broader* at small boosts (proportional Q), shelves have the classic dip/bump, transformer warmth |
| **LE-SP1** | AirEQ style: gentle cuts, a slightly resonant low ("Earth") shelf, bells that boost broad and cut narrow, and a very wide Air band that opens the top without harshness. No colour |

**Silk** (LE-SP1 only, next to the Air knobs) adds soft, tube-like harmonics to just the content above ~6 kHz,
at 2x oversampling so it stays smooth instead of fizzy. The original signal isn't changed, only harmonics are added.
25% (default) is a subtle sheen; 100% is about 1–2% harmonics on typical air content. The knob dims in the other EQ types.
At high Silk settings you may want the de-esser threshold 1–2 dB lower.

Bands: **High-pass** · **Body** (low shelf) · **Mud** (bell) · **Presence** (bell) · **Air** (high shelf)

#### Changing the order
- **Drag a tab sideways** (on the Character EQ or Compressor bar) to move it. The other tabs shift along, and the
  small number in each tab's corner shows its place in the chain (1 runs first). The tab order *is* the processing order.
- **Before Comp / After Comp** (right side of the Character EQ header) moves the whole Character EQ section to
  before or after the gate and compressors. The gate always stays right in front of the compressors.
  - *Before Comp* (default): the compressor reacts to your EQ moves, so boosts get partly evened out. Smooth, glued.
  - *After Comp*: the EQ shapes the already-controlled vocal, so boosts (especially Air and Silk) come through in full.
    Classic for adding air or a Pultec low end after heavy compression.
- Orders are saved with the session and presets, can be undone, and can be automated
  ("Character EQ Order", "Compressor Order", "Character EQ Position").
- Ideas: LE-660 first to level gently, then LE-76 to grab what's left; or LE-RC first for
  podcast speech, then LE-76 for consistency.

### Breath control and vocal rider (before the compressors)
The compressor section's left column holds the tools that work *before* the compressors: **Gate → Breath → Rider**.

- **Breath**: finds breaths (airy, mostly high-frequency, clearly quieter than the singing) and turns them down by
  this much, instead of gating them out, so phrases still sound natural. The light beside it glows while a breath is
  being turned down. Off at the far right; try −8 to −12 dB.
- **Rider**: an automatic fader, like an engineer riding the vocal. It compares each moment's level (about ¼ s)
  with the vocal's own average (about 8 s) and pushes quiet phrases up and pulls loud ones down, up to this far
  either way. It holds still in silences and breaths, so it never boosts noise. The bar in its slot shows it live:
  green to the right = pushing up, yellow to the left = pulling down. Off at the far left; try ±4 to ±6 dB.
  With the rider evening things out first, the compressors work less and sound more natural.

### Gate (RVox style)
One knob. Set it just above the noise between phrases. It's a soft expander with hold and
hysteresis, so word tails don't chatter. **Off** at full left. The **GATE** light shows when it's closing.

### Compressor (three characters, stackable)
The three compressors carry LE names: **LE-76** (1176-style FET), **LE-RC** (Renaissance-style compressor) and
**LE-660** (Fairchild-style vari-mu).

Each compressor has **its own settings** and **its own activation light** on its tab:
- **Click the light** to switch that compressor in or out. Use one, any two, or all three.
- **Click the name** to edit that compressor. Its controls dim if it's switched out.
- Active compressors run in series, **left to right in the order of the tabs** (default **LE-76 → LE-RC →
  LE-660**). The GR meter shows their combined reduction. A classic chain: LE-76 to catch peaks, then LE-660 to glue (the **Pop Vocal** preset does this).
- The Gate, Breath, Rider and the section power button are shared.
- **SC off / 80 / 120 / 160** (in the header): a sidechain high-pass for the compressors. The detector ignores the
  low end, so plosives ("p", "b") and proximity boom don't cause extra compression. 80 Hz is subtle, 160 Hz is strong.

| Mode | Behaviour | Controls |
|---|---|---|
| **LE-76** | 1176 style: attack down to 20 µs, punchy program-dependent release, grit that grows with gain reduction | Threshold, Ratio, Attack, Release, Make-up |
| **LE-RC** | Renaissance style: clean, versatile compression for any source. **AUTO REL** (auto release: stretches under sustained compression, stays quick on peaks), **Glide** (release slows near threshold: smooth) or **Snap** (release speeds up near threshold: transparent and louder), **Color** (gentle harmonics as it compresses) or **Pure** (clean) | Threshold, Ratio (1-50:1), Attack (0.5-500 ms), Release (5-3000 ms), Make-up (Gain), AUTO REL, Glide/Snap, Color/Pure |
| **LE-660** | Fairchild style: ratio rises the harder you push, tube warmth, six time constants (TC 5–6 are program dependent) | Threshold, Time Constant, Make-up |

The compressor's controls are **long horizontal sliders** with silver ribbed caps: drag left and right
(double-click to reset). A thin line in the selected mode's colour runs up to each cap: violet (LE-76),
orange (LE-RC) or gold (LE-660). The **Gate slider has a live level meter in its slot**: the vocal's level is drawn on the same scale as
the slider, **red** where it's below the gate (being gated) and **green** where it passes through, with a
white peak-hold tick. Set the cap just right of the red you see between phrases. The small light left of GATE
glows red when the gate is closing. The gain-reduction meter lights in the same colour.
**Mix** is a knob that matches the outboard unit of the selected mode: black with a chrome ring (LE-76),
an ivory chicken-head on a black base, vintage British studio style (LE-RC), or big black bakelite (LE-660).
The **de-esser's** Freq uses the same slider, with a **spectrum meter in its slot** instead of a colour line:
each little cell is the level of the vocal at the frequency it sits under. Cells left of the cap are dim (the de-esser
ignores them); cells right of the cap are what it listens to, **green** while it lets them pass and **red** while
it's turning the "s" down. Slide the cap to just left of where the cells flash brightest on "s" sounds.
Threshold and Range are knobs.

**Mix** (all compressor modes) blends compressed and dry signal for parallel ("New York") compression.
Try 50–70% with a fast, heavy FET setting to keep transients and body while adding density.

### De-esser (two styles)
Choose the style with **Split / Wide** in the de-esser's header:

| Style | What it does | When to use it |
|---|---|---|
| **Split** | Only the band above **Freq** is turned down; the body of the voice is untouched | Transparent, modern; heavy sibilance |
| **Wide** | The whole voice dips briefly on each "s", keyed from the band above Freq | Classic hardware feel; often more natural on gentle esses |

- **AUTO** makes it threshold-less: sibilance is judged relative to the vocal's own level, so it keeps
  working when the singer gets louder or softer. The Threshold knob turns into **Sensitivity**.
- **Range** caps how much it can turn down, up to 24 dB. **Freq** is a slider, so you can sweep it quickly while LISTEN is on.
- **LISTEN** solos the sibilance band so you can tune Freq (typical: 5–8 kHz).

### Zerem (styled 5-band stage, before the tape)
*Zerem* (זֶרֶם) is Hebrew for flow or current. Inspired by one-knob processors like Magic Flow: a 5-band processor (Boom < 180 Hz, Mud, Body,
Harsh, Air > 6.5 kHz). Each **style** sets every band's compression, timing and tone:

| Style | Character |
|---|---|
| **Pop Vocal** | Polished and forward: tight lows, smooth top, gentle sheen |
| **Rap Vocal** | Punchy and upfront: fast control, firm body, crisp top |
| **Mid Forward** | Mid-range boost: pushes the body of the voice to the front |
| **R&B Smooth** | Silky and warm: slow, gentle control and soft highs |
| **Airy Ballad** | Open and breathy: light touch with lots of air |
| **Podcast** | Clear speech: controlled lows, even level, tamed top |
| **Dense** | Raise RMS: every band held tight for a thick, loud vocal |
| **Drums** | Punch and snap: slower attacks let hits through, ring and boxiness tamed |
| **Bass** | Even and solid: lows locked in, growl kept, top tidied |
| **Guitar** | Focused and present: boom and fizz tamed, mids forward |
| **Keys** | Wide and clear: gentle control, less mud, soft sparkle |
| **Mix Bus** | Glue: light, slow control on every band for a polished mix |

- Browse styles with the **◀ ▶** arrows on the display, or click the name for the list.
- **Amount**: how much of the style. 50% is the style as designed; 0% is nearly transparent; 100% pushes harder
  and adds more of its tone.
- **Mix**: blends with the dry signal (phase-matched, so blending never sounds hollow).
- The five bars on the display show how much each band is being turned down, live.

### Tape (just before the limiter)
Switch on with the TAPE power button, then pick a tape speed in the header. Each speed is a full set of
"best settings". Like real tape, the slower the speed, the lower the head bump, the softer the top and the
more saturation:

| Speed | Best for | What it does |
|---|---|---|
| **30 ips** | Vocals | Gentle saturation that smooths harsh highs and esses first, a small bump around 90 Hz, open top. Glue without dulling the vocal |
| **15 ips** | Drums | Rounds transients for punch and glue, a fuller head bump (+2 dB around 55 Hz), slightly softer top (16 kHz) |
| **7.5 ips** | Guitars | Warmest, richest harmonics, a low bump around 40 Hz, a touch of mid presence, smooth top (12 kHz) |

**Tape width** (the selector above Drive and Mix) shapes the character on top of the speed:

| Width | Character |
|---|---|
| **1/4"** | Narrow tape, least headroom: saturates soonest, slightly softer top. Gritty, vintage |
| **1/2"** | The classic mastering format, most headroom: cleanest, wide extended low end |
| **1"** (default) | Balanced: the speeds' reference sound |
| **2"** | The big multitrack sound: punchiest, fullest low-end bump, most glue |

- **Drive**: how hard the tape is hit. **50% is the recommended setting** for the speed; it spans ±12 dB around it.
- **Mix**: blends the saturation with the clean signal. The tape's tone (bump and top end) stays, so blending never sounds phasey.
- **NOISE** (under the VU meter): adds real-tape hiss, shaped by the tape's own EQ. Like real tape, slower speeds and
  narrower tape hiss more: about −72 dBFS at 30 ips on 1/2", −65 dBFS at 15 ips on 1", −57 dBFS at 7.5 ips on 1/4".
  Left and right hiss independently for a natural stereo bed. It plays even in silence, so switch it off when printing stems.
- **OS Off / 2x / 4x** (next to the width selector): oversampling for the tape saturation. Default 2x.
- **LED VU meter**: a tape-deck style LED ladder with real VU ballistics (0 VU = −18 dBFS RMS): yellow up to 0, red
  above, a peak-hold LED and the live reading underneath. Aim for around 0 on the loud parts.
- The saturation runs at 2x the sample rate so it stays smooth, not fizzy. It adds 4 samples of latency (compensated
  automatically) and is perfectly clean when switched off.

### Limiter (L1 / L3 style maximizer)
- **Threshold** pulls down to raise loudness by the same amount (−6 = 6 dB louder)
- **Ceiling** is the absolute max output (tested to never be exceeded)
- **Release**, or **AUTO REL** (auto release): fast on short peaks, slow under sustained limiting
- **Classic** = single band like the L1. **Multi** = L3-style: lows, mids and highs are tamed separately
  before the final brickwall, so the voice pumps less when pushed hard.

**Mix** blends the limited signal with the dry input. The dry path is delayed to match the look-ahead and,
in Multi, passes through matching crossovers, so blending never causes phasing or a hollow sound.
The **Ceiling is only guaranteed at 100% Mix**; below that, untouched dry peaks pass through.

The limiter adds 1.5 ms of latency, which your DAW compensates for automatically.

**Soft saturation (Lavry-style)**: the selector above the limiter knobs. Like the soft-saturation
switch on Lavry AD converters, it rounds peaks off instantly on a smooth curve, with no attack or release
and therefore no pumping. Audio below its threshold is untouched, and make-up gain then adds the selected
amount of loudness. The brickwall limiter after it catches anything left over. It works while the limiter
is on, and its gain reduction shows on the GR meter.

| Setting | Curve starts at | Loudness added |
|---|---|---|
| **Soft off** | (bypassed) | 0 dB |
| **2 dB** | −4 dBFS | +2 dB |
| **3 dB** | −6 dBFS | +3 dB |
| **4 dB** | −8 dBFS | +4 dB |
| **6 dB** | −12 dBFS | +6 dB |

Start at 2 or 3 dB for vocals. 6 dB gets louder and denser, and adds a little warmth on loud notes.

**OS Off / 2x / 4x** (next to the soft-saturation selector): oversamples the soft saturation. Default Off. The look-ahead
limiter itself always runs at the normal rate, after the down-sampling filter: its gain moves are smooth enough not to
alias, and running it last keeps the ceiling guaranteed without hard-clipping any filter overshoot.

#### Oversampling (Tape and Limiter)
Saturation creates harmonics above the audible range. Without oversampling, the ones above half the sample rate fold
back down as inharmonic "aliasing" (a grainy, metallic fizz on bright sources). Oversampling runs the stage at 2x or 4x
the sample rate, then filters those harmonics away before coming back down.

| Setting | Use it for | Extra latency at 48 kHz |
|---|---|---|
| **Off** | Lowest CPU, gentle settings | none |
| **2x** | Most material (Tape default) | +4 samples |
| **4x** | Bright, pushed sources: cymbals, sibilant vocals, hard drive, Soft at 6 dB | +6 samples |

Measured: an 11 kHz tone driven hard into 30 ips tape folds back at −25 dB with OS off, −40 dB at 2x and −90 dB at 4x.
A 7 kHz tone into 6 dB soft saturation: −32 dB with OS off, −85 dB at 2x/4x.

Oversampling is a quality setting, so presets and A/B never change it. Changing it changes the plugin's latency (your DAW
compensates automatically); it's best changed while playback is stopped. TRACK mode always turns oversampling off for zero latency.
Total plugin latency is about 23 ms at 48 kHz (peak catcher + resonance suppressor + tape + limiter look-ahead),
or zero in TRACK mode.

---

## 6. Starting points

- **Resonance first:** Depth 40%, Sharpness 40%, Selectivity 50%, focus 300 Hz–12 kHz. Toggle DELTA to make
  sure you only hear rings and harshness, not the voice.
- **Tahor EQ first, every time:** low cut 24 dB/oct around 80–100 Hz, then sweep a narrow bell (Q 4–6)
  with +10 dB to find any ringing or boxy note, then pull it down 3–6 dB or make it dynamic.
- **Rap / upfront pop vocal:** LE-E4K or LE-SP1, HPF 100 Hz, Mud −3 dB @ 350, Presence +3 @ 3.5k, Air +2.
  LE-76 4:1, attack 0.8 ms, release 120 ms, about 6 dB GR. De-ess 6.5k. Zerem Pop Vocal (Rap Vocal for rap), Limiter Classic, threshold −3, AUTO REL on.
- **Worship / ballad vocal:** LE-N81, HPF 80, Body +1.5, Air +3. LE-660 TC 2, 3–4 dB GR.
  Gate around −50. Limiter Multi, threshold −2.
- **Podcast / spoken word:** Clean EQ, LE-RC (Glide, 3:1) with Gate at −45, De-ess 5.5k, Limiter ceiling −1.

---

## 7. Customising

- **Mode names:** `Source/PluginProcessor.h` (top of file)
- **Skin colours:** `Source/Skin.h`
- **Brand / plugin name:** `COMPANY_NAME`, `PRODUCT_NAME`, `BUNDLE_ID` and the two 4-letter codes in `CMakeLists.txt`.
  If you change the codes after using the plugin in sessions, the DAW treats it as a new plugin.
- **Sound of each mode:** `Source/VocalChain.h` (curves, timing tables, saturation amounts are all commented)
- **Tahor EQ:** `Source/CleanupEQ.h` (DSP) and `Source/CleanupEqView.cpp` (graph and band panel)
- **Resonance suppressor:** `Source/ResonanceSuppressor.h` (the detection and timing are commented)

## 8. Offline tests (optional)

```bash
cmake -B build-tests -DVOX_BUILD_TESTS=ON
cmake --build build-tests --config Release --target MishkalDspTest
./build-tests/MishkalDspTest_artefacts/Release/MishkalDspTest
```

Checks the limiter ceiling in every mode, the de-esser, gate, each compressor and EQ character,
the Tahor EQ's slopes, notch and dynamic bands, the resonance suppressor (tone in noise, focus, Delta, Mix),
and that the fully bypassed plugin nulls perfectly against its input.

## Notes

- The calligraphy for "Lovy Elias" uses **Parisienne** by Astigmatic, under the SIL Open Font Licence
  (`Assets/Parisienne-OFL.txt`). It's built into the plugin, so it shows the same on every Mac.

- The mode names describe *styles*. oeksound/soothe, FabFilter/Pro-Q, Pultec, Eiosis/AirEQ, Waves/C6, SSL, Neve, Universal Audio/1176, Waves, RVox, L1/L3 and Fairchild are
  trademarks of their owners. These are character-inspired designs, not circuit models.
- Not included from Pro-Q: linear-phase mode, mid/side per band, and band solo. Ask if you want any of them.
- JUCE licensing: JUCE 8 is free to use under its Starter licence up to a revenue limit, or under AGPLv3.
  Check juce.com/legal before selling the plugin.
