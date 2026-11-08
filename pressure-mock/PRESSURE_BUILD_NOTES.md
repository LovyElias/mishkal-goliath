# Pressure - build notes (decisions from the design sessions)

## HELPER
- **1964 (Spectra 1964 610 Complimiter style): super clean.** No saturation, no distortion, no THD, no colour stage. Gain changes only:
  very fast peak limiting/compression (attack program-dependent, ~100 ns-1.2 ms in the original), oversampled detection, no clipping or waveshaping.
  Controls: Input (threshold at -40 dBu), Output, Slope (~1.1:1 to 100:1), Release 0-10 (~50 ms to >10 s), Meter GR/+4/+8, Pad Off/10/20, THR (white) and OVER (red) lights.
- Lift: two-band upward compressor / dynamic EQ (Upward | Dyn EQ), Low and High bands with Freq, Thresh, Lift; Tone drawer shows the bands live.
- Rider and Breath on the right under the 1964. Flow and the high pass were removed.
- Every HELPER module (Lift, 1964, Rider, Breath) has its own on/off button; an off module dims its controls.
- HELPER colour: bronze (#b08d57), one accent for every module; the waveform tinted with it. Knobs are the compressors' own knob.
- HELPER layout: left = Lift (2-band, Upward | Dyn EQ, Low/High with Freq, Thresh, Lift); right = 1964 Peak, Rider, Breath. Tone drawer on the right edge shows Lift's bands live.
