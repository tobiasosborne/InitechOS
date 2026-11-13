# Preference repaint references (initech-c8w7 / B05)

These 640x40 RGB strips were captured before changing the artifact at
commit `254a72445592673d9296e46b5f31759b0c1d2850`, from the existing
`flair_tenants_mut_cpanels_gate.img` and stock `flair_data.img`.
The harness used `--rtc-base 1980-01-01T00:56:00 --rtc-deterministic`.

`cpanels.png` is the untouched Control Panels chooser after Apple launch.
`hello.png` is after selecting that application's File > Quit with the mouse.
Neither trace changes a preference. See `CP_BARS_BASE_CP` and
`CP_BARS_BASE_HELLO` in `harness/emu/cpanels_bars.mk`. The pointer is parked
below the bars. `ppm_flair_cpanels_bars_check ... capture` extracts only the
first 40 rows, without resampling or re-rendering them. Do not regenerate
these references from the repaired preference redraw.

The two installed bars are deliberate: PRD 6.3 and `os/flair/shell.h`.
The regression is documented in DAS audit L027 / shots 062, 069, 071, 249.
The clock face/anchor also has the independent Mac OS 8 reference
`system7-decomp/goldens/captures/s8_desktop_menubar.png`, graded by
`test-flair-menubar-clock`. For clock hide, the checker restores only its
reserved face interval using a blank column of this captured system bar;
all 25,600 pixels are still compared, including edges, corners and baseline.

SHA-256:

```
2e8826d1227ae375effe9e004c8e5117ce890f3f4a275b92addc2dc08160e836  cpanels.png
f2b498cd13c04dcde6e9cd8d3dbdf649d8f23b3d7c6a6dddc786bf1ba554cacc  hello.png
```
