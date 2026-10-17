# WL-0149: SSIM guide tool (initech-w06)

## Context and change

Law 4 / PRD sections 3, 6.3 and 8 require per-window guidance, never a
numeric acceptance threshold. ADR-0010 CD-7 / HER-11 require independent
rendered decomp goldens. This factory-only C tool replaces the Makefile
stub and the pre-FLAIR crop placeholders. CLAUDE.md, HANDOFF.md, beads and
Git were left untouched as directed by the lane owner.

`harness/ssim.c` reads P6/P3 PPM (maxval 255) and PNG via libpng, computes
full-range RGB luma and Wang's 11x11 Gaussian SSIM, and reports the mean,
valid-map dimensions/count, minimum, maximum, standard deviation and worst
local window centre. It accepts separate crop offsets of equal dimensions.
There is no resizing, gamma fitting, border padding, score threshold or
aggregate reward. Invalid/missing images, invalid crops, and unknown frames
return exit 2; a low or negative similarity score returns success.

Run `make ssim FRAME=desktop`. Additional frames are `grow-box`,
`scroll-look`, `alert`, `finder`, `icons`, `controls`. These read existing
gate dumps; missing input reports the precise capture gate to run with
`INITECH_QMP_STDIO=1`. The crop header names each source PNG and gate.

The crop change is a deliberate Rule-8 act under initech-w06. Coordinates
were checked against the actual QEMU dumps and independent corpus images
and measurements. Finder's list capture supplies matching title chrome;
the current APPS dump is an icon view, so list content is not compared.
The controls capture supplies a face-only patch, not an assertion about
unexercised control shapes. Titles, labels and wallpaper differ across the
two systems; their effects remain visible. The corpus HD icon is supplied
by Basilisk's disk driver, not Apple's generic disk art.

## Red, green and mutation proof

The independent C CLI/analytic oracle and both Makefile test targets were
written first. Initial RED was `No rule to make target 'harness/ssim.c',
needed by 'build/ssim'`. The gate imports neither the implementation nor its
parameter header: it checks constant-patch and sparse-impulse closed forms,
identity, negative covariance, codecs, independent crop offsets and invalid
inputs. Both targets are in `TEST_UNIT_GATES`; `ssim` itself is not a gate.

Final gate tails:

```text
test-ssim: green (0 failures)
>>> SSIM_MUT_WINDOW_9 correctly RED: RED GAUSSIAN one-pixel perturbation analytic value <1
>>> SSIM_MUT_K1 correctly RED: RED K1 constant-patch analytic value
>>> SSIM_MUT_K2 correctly RED: RED K2 impulse255 analytic value
>>> SSIM_MUT_SIGMA correctly RED: RED GAUSSIAN one-pixel perturbation analytic value <1
>>> SSIM_MUT_LUMA correctly RED: RED LUMA RGB red uses BT.601
>>> test-ssim-mutant: green (five named math mutants RED)
>>> test-makefile-vars: green
test_flair_headers: 14 checks, 0 failures
>>> test-flair-headers: green
```

Identity=1; inverse checkerboard=-0.996406468357; central one-pixel +1
perturbation=0.998109076370 (closed form agrees to 1e-10).
Mutants change kernel 11 to 9, K1 .01 to .02, K2 .03 to .06, sigma 1.5 to
2.0, and BT.601 luma to channel averaging, respectively.

## Current desktop guide

All crops are `(x,y,width,height)`, native pixels; no threshold.

| Window | QEMU crop | Independent PNG and crop | Mean SSIM |
|---|---|---|---:|
| MENU_BAR | 0,0,640,20 | s8_desktop_menubar.png: 0,0,640,20 | 0.566179761 |
| DOCUMENT_TITLE (inactive) | 60,80,260,22 | s8_doc_window_inactive.png: 13,29,260,22 | 0.746824068 |
| FRONT_TITLE | 300,120,260,22 | s8_doc_window_active.png: 65,70,260,22 | 0.218348527 |
| CLOSE_BOX (active) | 304,124,13,13 | s8_doc_window_active.png: 69,74,13,13 | 1.000000000 |

Other guide runs succeeded: grow-box 1.000000000; scroll-look 0.924040637;
alert top band 0.808713591, default button 0.561427567; Finder title chrome
0.865949093; volume 0.073131285, Trash -0.023659407; dialog face 1.000000000.
These values are reports, not GREEN/RED classifications.

## Emulator/build acceptance and friction

Checked `free -g` (20 GiB available) and `df -h /` (120 GiB available)
before building. Every QEMU invocation used `INITECH_QMP_STDIO=1`. No full
`make test`, Bochs, shipped-source changes or new shipped program.

```text
make image:
>>> image: build/initech.img (flagship alias of build/flair_tenants.img -- FLAIR desktop + HELLO/NOTES co-resident tenants)
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
test-flair-desktop:
VERDICT   : PASS -- the LIVE FLAIR chimera desktop is the Office Space frame
test-flair-grow-box:
>>> test-flair-grow-box: green (APPS +40,+40; 400x260 serial marker and frame; 324/324 golden pixels)
test-flair-scroll-look:
>>> test-flair-scroll-look: green (shrink enabled; click scrolls16; restore disabled; minimum96x88 draws all parts; no SCROLL-IGNORED)
test-flair-alert-chrome:
>>> test-flair-alert-chrome: green
test-flair-app-launch:
>>> test-flair-app-launch: green
test-flair-locked-purge:
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
test-flair-finder-service:
>>> test-flair-finder-service: green
```

Cold `make image` initially failed: `os/milton/kmain.c:70:10: fatal error:
flair_dac.h: No such file or directory`. The existing `tools/flair_dac_gen.c`
has no Makefile rule. To unblock captures without expanding this lane:

```sh
cc -std=c11 -Wall -Wextra -Werror -o build/flair_dac_gen tools/flair_dac_gen.c
build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json > build/flair_dac.h
```

The subsequent image build and gates passed. That existing cold-build
dependency defect remains for the PM; it is not repaired by this lane.
The primary SSIM PDF could not be downloaded through shell DNS; acquired
equation/parameter notes are local at `docs/research/ssim-wang2004.md`.

## Law-1 pointers

- `docs/research/ssim-wang2004.md`: primary paper pp. 604-606, equations
  7/13-17; valid-window convention from the authors' algorithm documentation.
- `../system7-decomp/INDEX.md` and `specs/sys8/INDEX-sys8.md`: capture roles.
- Corpus `specs/sys8/window-chrome.md` sections 1/2/5; `scrollbars.md`
  section 2; `controls.md` sections 1/5/6; `re/mint-results-008.md` sections
  2/7: independent native-pixel geometry and icon/list provenance.
- `spec/flair_grow_box_trace.mk`, `spec/flair_scroll_look_traces.mk`,
  `spec/flair_desktop_icons_traces.mk`, and the corresponding `tools/ppm_*`
  graders: the actual emulator crop geometry.
- DOS 3.3 command manuals / ADR-0003 Appendix A do not define this factory
  image metric; no DOS behavior or FAT implementation was changed.
