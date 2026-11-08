# Pressure - build notes (decisions from the design sessions)

## HELPERS (the tab is now called HELPERS)
- Lift's Tone drawer: UPWARD mode shows each band's lift per input level (full lift on quiet parts, easing to none at its threshold, live dot with trail); DYN EQ mode shows the two bands as moving bumps on a frequency graph. The UPWARD / DYN EQ switch picks which.
- Rider: a RIDING bar (+/-6 dB from centre) and a loudness readout IN -> OUT LUFS (OUT lands on the Target).
- Breath: Amount, Sense, Fade knobs; a light that blinks on each caught breath; a CAUGHT strip showing recent breaths as dips with a count.
- A De-esser / Pop bottom drawer was tried and removed (not wanted).
- **1964 (Spectra 1964 610 Complimiter style): super clean.** No saturation, no distortion, no THD, no colour stage. Gain changes only:
  very fast peak limiting/compression (attack program-dependent, ~100 ns-1.2 ms in the original), oversampled detection, no clipping or waveshaping.
  Controls: Input (threshold at -40 dBu), Output, Slope (~1.1:1 to 100:1), Release 0-10 (~50 ms to >10 s), Meter GR/+4/+8, Pad Off/10/20, THR (white) and OVER (red) lights.
- Lift: two-band upward compressor / dynamic EQ (Upward | Dyn EQ), Low and High bands with Freq, Thresh, Lift; Tone drawer shows the bands live.
- Rider and Breath on the right under the 1964. Flow and the high pass were removed.
- Every HELPER module (Lift, 1964, Rider, Breath) has its own on/off button; an off module dims its controls.
- HELPER colour: bronze (#b08d57), one accent for every module; the waveform tinted with it. Knobs are the compressors' own knob.
- HELPER layout: left = 1964 Peak (transparent) and Rider; right = Lift (2-band, Upward | Dyn EQ, Low/High columns) and Breath, next to the Tone drawer.
- HELPER knobs: skirted vintage style (dark skirt with a printed 0-10 scale, bronze pointer, small glossy cap). Lift bands as two columns; Breath under Lift on the left; 1964 and Rider on the right.
- HELPER shows a gain reduction meter (beside the input meter, with the amount in red) and the thin red GR line on screen - the 1964 Peak's reduction.
- HELPER keeps the left glass drawer (like the compressors): it shows the 1964's curve (threshold and slope).
