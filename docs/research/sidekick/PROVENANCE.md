<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Provenance register for the SideKick period sources (bead initech-6fdt, sub-deliverable 1). -->
<!-- Every file below was fetched 2026-10-20 by the skresearch lane. Downloaded files are untrusted -->
<!-- data; nothing in them was executed on the host (the DOS binaries ran only inside DOSBox-X). -->

# SideKick period sources -- provenance

Companion to `docs/research/SIDEKICK-period-grounding.md`. Source tags (`[HB15]` etc.)
are the ones that document cites.

**Tracking.** The scans, disk images and the MS-DOS boot disk are third-party
copyrighted material. Following the house rule in
`initech-manuals-ref/SOURCES.md` (copyrighted manuals and packaging are kept out of
the public repository), they live under `manual/`, `media/` and `dosboot/`
LOCALLY and are git-ignored (`.gitignore` here). Each is re-fetchable byte-exact:
`curl -L -o <file> <URL>` and compare against the sha256 below. Only this file and
`captures/` (our own emulator captures and their cell transcriptions) are tracked.
No file here is over 20 MB (the largest is 3.96 MB).

**Page numbers.** The `[HB15]` scan has 129 PDF pages; printed page N is PDF
page N+7 throughout (verified on every page header). `[HB10]` has 99 PDF pages;
printed page N is PDF page N+11. The document cites PRINTED page numbers.

## 1. Acquired, local, untracked (copyrighted)

| Tag | Local file | Title | URL | Bytes | sha256 | Cited |
|---|---|---|---|---|---|---|
| HB15 | `manual/Sidekick_Version_1.5_Owners_Manual_Mar85.pdf` | Borland, *SideKick Version 1.5 Owner's Handbook*, first edition March 1985 ("Special Edition for AST Research Inc." on the title page) | https://archive.org/download/bitsavers_borlandsidn1.5OwnersManualMar85_3963623/Sidekick_Version_1.5_Owners_Manual_Mar85.pdf (bitsavers `pdf/borland/sidekick/`) | 3963623 | `3d0f8a575587947ca204e43790a8ad52165febc73ea895f5f80e4921d0446bb2` | pp. 1-10, 13-36, 39-44, 54, 63-66, 69-77, 79, 81-86, 87-88, 93-101, 103-111; Fig. 1-1, 2-1..2-9, 3-1, 3-2, 8-1, B-1..B-3; Table B-1; the bound-in Editor Quick Reference card (PDF p. 128) |
| HB15t | `manual/Sidekick_Version_1.5_Owners_Manual_Mar85_djvu.txt` | archive.org OCR text layer of HB15 (search aid only; keycaps are images and OCR loses them -- every keystroke cited was read off the page image) | same item, `..._djvu.txt` | 170024 | `207262bc1e438f950f6c8b57f0996d8211eb9e46d6998ed714052db04bfbe6b2` | (aid) |
| HB10 | `manual/Sidekick_Version_1_Owners_Manual_1984.pdf` | Borland, *SideKick Version 1 Owner's Handbook*, 1984 | https://archive.org/download/bitsavers_borlandsidn1OwnersManual1984_3248857/Sidekick_Version_1_Owners_Manual_1984.pdf | 3248857 | `eefda5be3fd58c921d054225f60e35291c01346b066ac579980189a0e9560c2a` | p. 8 (activation, both-Shift alternative), p. 17 (calculator memory keys) -- cross-check only |
| HB10t | `manual/Sidekick_Version_1_Owners_Manual_1984_djvu.txt` | OCR text of HB10 | same item, `..._djvu.txt` | 130481 | `25eaf434c532c91bc3dd7d941434e09d9654283c9282c395bd32592ff3a44f13` | (aid) |
| FS15 | `manual/Sidekick_1.5_Fact_Sheet.pdf` | Borland, *SIDEKICK Version 1.5* fact sheet / technical specifications, 1985 (2 pp.) | https://archive.org/download/sidekick-1.5-fact-sheet/Sidekick%201.5%20Fact%20Sheet.pdf | 857886 | `f746b016e21277e3153df5369a8e0cfa3a33c3f70ae65956dc68e6c426a90673` | p. 2 (system requirements, notepad size, calculator digits, directory size, price) |
| FS15t | `manual/Sidekick_1.5_Fact_Sheet_djvu.txt` | OCR text of FS15 | same item | 7393 | `4b42383d0430c6075048bcc7f3a45cf89dbd8cb8069b78d91613b367715bbaad` | (aid) |
| D156 | `media/Sidekick1985_v1.56A.ima` | SideKick 1.56A retail distribution disk, 360 KB FAT12 image (SK.COM 39515 bytes 1985-08-05; READ-ME.SK 13458 bytes) | https://archive.org/download/Sidekick1985/Sidekick%201985.ima | 368640 | `496c304524a225472bf6e3b5b27c0731cdd0322c5a1363b9197ee4263a3e445f` | THE binary every capture in `captures/` ran; READ-ME.SK "New Features" + "Corrections" sections |
| D156x | (inside D156) `SK.COM`, `SKINST.COM`, `READ-ME.SK` | extracted with mtools for reading only | -- | 39515 / 54574 / 13458 | `d3eaf419b4992c4d32e318e1d41e2408fd2e44875b3ffc61c5fad1ba76cfb2e6` / `cf1a979d9d292de980967adb46da67dfb2af3319b45b2649384838f4da1de820` / `49bc7fa7b96dec5d6087cdc7e27f26b2c967ba746d093b0fe0df25a388ce10a2` | strings (messages), READ-ME.SK |
| AST | `media/AST_Sidekick_v1.56A.img` | AST Research OEM SideKick 1.56A disk (clean dump 2018-12-18) | https://archive.org/download/ast_sidekick156a/AST%20-%20Sidekick%20v1.56A%20(Borland%20International,%20Inc.)(1985)%20%5B!%5D.img | 368640 | `f14c7ba314ff40163e56e4bd8e93c5c92f3b2c97c3f18aa5103a1f66740c8192` | cross-check (same file set, OEM banner) |
| ASTs0..3 | `media/AST_Sidekick_v1.56A_Screenshot0{0,1,2,3}.png` | third-party emulator screenshots of the AST disk: SKINST main menu, log-on, main menu, calculator | https://archive.org/download/ast_sidekick156a/AST%20Sidekick%20v1.56A_Screenshot00.png (01, 02, 03 likewise) | 19410 / 21650 / 42206 / 41300 | `7b58503273f19016bcd2e6f288e57e7ff782f20b77c831b393b2b9c57a19dad4` / `e40cda016f8ec8d3e0cc91278157a29eaf42916527c491dbb8a35a46094b5c78` / `2e833e967569ed0e13a97ba77626979163e1d378a55e229e83389bd39737a73f` / `ab91d13b2afabf1120068cd0592f951b52e9a644ada71dc271f146a42641c967` | independent cross-check of our captures (log-on, resident size, menu, calculator) |
| ASTl | `media/AST_Sidekick_v1.56A_label.png` | AST disk label scan | https://archive.org/download/ast_sidekick156a/AST%20-%20Sidekick%20v1.56A%20(Borland%20International,%20Inc.)(1985).png | 3639175 | `8cf626c59d232efd830a1b2654dee0286241dfbd649837a95cc2184a27cd3f24` | not cited |
| D156B | `media/Borland_SideKick_v1.56B.imz` | SideKick 1.56B 720 KB disk (zip of .IMA; SK.COM 39515 bytes 1986-12-08) | https://archive.org/download/BorlandSideKickV1.56BDOS/Borland_SideKick_v1.56B_(DOS).imz | 138471 | `07ca7116fedd11cba61af8fd2a444d87201d9fcc54df6a37f64693bdc948c604` | file-set cross-check only |
| D156Bj | `media/Borland_SideKick_V1.56B.jpg` | 1.56B disk photo | https://archive.org/download/BorlandSideKickV1.56BDOS/Borland_SideKick_V1.56B.jpg | 2496697 | `07ab040465328b021e4421d3a9a26e6447f41a3b0679fe2d3d27bb77d35eb064` | not cited |
| D85j | `media/Sidekick1985_disk_media.jpg` | 1.56A disk photo | https://archive.org/download/Sidekick1985/SideKick_Didk_media_1985.jpg | 218111 | `53d4f5a0d9f6e3247084465d05ca39d82db20ccf6ca1cd9c177d3878e1abb11e` | not cited |
| OBS | `media/obsmsd_sidekick15.img` | user backup floppy, SideKick 1.5x + Copy II PC | https://archive.org/download/obsmsdsidekick9backup/obsmsd%20sidekick%209%20backup.img | 368640 | `834c2301931bd6886481fb07536d19a1b091d75512725d9ac1b7324e393f114e` | file-set cross-check only |
| OBSs | `media/obsmsd_skinst_000.png`, `media/obsmsd_dosbox_001.png` | third-party screenshots of OBS | https://archive.org/download/obsmsdsidekick9backup/skinst_000.png , .../dosbox_001.png | 3032 / 6051 | `fa8d0b1d96ee2c4698dc2e45ae91da44541ea79083b2c1153c64e4ab0e42d1b8` / `9adf451b5d3a679e5222cdd910b2aa8e16d2acc63d0fddcb0a99d889ebacd087` | SKINST main menu cross-check |
| DOS33 | `dosboot/PB_MSDOS33_Sys_and_Util.img` | Packard Bell OEM MS-DOS 3.30 "Sys and Util" disk 1, 360 KB, OEM-ID MSDOS3.3, COMMAND.COM 25308 bytes 1988-02-02 (Applesauce flux dump 2024-02-24) | https://archive.org/download/packard-bell-ms-dos-3.3/Packard%20Bell%20-%20MS-DOS%203.3%20-%20Sys%20and%20Util.img | 368640 | `3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30` | the real DOS 3.3 the captures booted (see Sec. 3) |
| DOS33l | `dosboot/PB_MSDOS33_Sys_and_Util_ImagingLog.txt` | its imaging log | same item, `... - Imaging Log.txt` | 2797 | `808d19030ddbbe1513b0493774cd5ed688db19398787a4d02dfbcd6258712535` | dump-quality evidence |

Rejected during acquisition: archive.org item `dos-33` (`DOS33.img`, "IBM PC DOS 3.3"),
fetched and DELETED unused -- non-IBM OEM-ID "L4\x12", a 1990 `SCAN67.EXE` on the
disk, a volume label "THE LASER" and a 369664-byte (non-geometry) size; not a
trustworthy period golden. Item `bwb_W1-AOQ-809` (a second scan of the 1984
handbook) is a lending-library item (encrypted PDF) and was not fetched.

## 2. Cited only, not acquired

| Tag | Source | Why not acquired |
|---|---|---|
| SKPLUS | Borland, *SideKick Plus Owner's Handbook* and *SideKick Plus for SideKick Users* (1988), bitsavers `pdf/borland/sidekick/` | later product (not 1.5x); not needed for any claim |
| REVIEWS | Byte / PC Magazine reviews of SideKick (1984-85) | not retrieved this lane; no claim depends on them (every behavioural claim is sourced to HB15, FS15, READ-ME.SK or a capture) |

## 3. Captures (tracked) -- our own emulator runs, 2026-10-20

Produced by this lane, not downloaded. Rig: DOSBox-X 2024.03.01 (`/usr/bin/dosbox-x`)
under Xvfb, `machine=svga_s3` (VGA files), `cga` (cga-*), `mda` (mda-*);
`cputype=386`, `cycles=fixed 8000`; drive A: = DOS33 (`boot -l a`, real MS-DOS 3.30
booted, NOT DOSBox's built-in DOS), drive B: = a scratch copy of D156. Keys were
injected with xdotool; frames grabbed with ImageMagick `import` from the emulator
window (720x400 VGA text = 80x25 cells of 9x16; CGA 640x400; MDA 720x350).
SideKick refuses to load under DOSBox-X's built-in DOS ("Sorry, unknown PCDOS
version. Please report to BORLAND.", tried `ver=` 2.11/3.10/3.20/3.30) -- it
fingerprints DOS internals, which is why a real DOS 3.3 boot disk was acquired.

Each VGA/ASCII/SKINST/IVT `.png` has a `.txt` cell transcription: rows `NN|<80 chars>|`
in Unicode-rendered CP437, then one line per row of 160 hex digits (per cell:
background nibble, foreground nibble; `.` = no foreground pixel in the cell). The
transcriber (a one-off research script kept OUT of the repo, Law 3) split each frame
into 9x16 cells, took the background from the cell's 9th pixel column, and matched
the 8x16 ink bitmap against a glyph dictionary it LEARNED from SideKick's own ASCII
table pages (`ascii-p1..p9`, codes 0-255 at known cells; 253 distinct glyphs, the
blank glyphs 0, 32, 255 rendering as space). Cells reported `?` are the blinking
hardware-cursor cell (one per frame where present). The `.txt` is therefore exact
for character and attribute, modulo the space/NUL/255 ambiguity.

| File | sha256 |
|---|---|
| `captures/ascii-p1.png` | `ca913406e3b6bda0885f3afe4e2c417ad652bba599ae292541b672bced3502f8` |
| `captures/ascii-p2.png` | `6a9b6408d57f8d7bff26e42e2e0e59df3148f6ed2dbccbae1e1ef79011cc2e7a` |
| `captures/ascii-p3.png` | `b9e96d8f990cd4d4681a54662dbf0dacac335f3acc605a887eae3befba7fc266` |
| `captures/ascii-p4.png` | `7cce31de50e4e7b1c26fb198ce3354ea413c304d131206e355d1395848c3db01` |
| `captures/ascii-p5.png` | `81c62dd7df7469d8f6152ef27083691da7ec80406776eb1a85c95897603011bb` |
| `captures/ascii-p6.png` | `5f65c4654780cd806614b6f5cddaf0ed826cf7d9700c110464609123834b0163` |
| `captures/ascii-p7.png` | `49f8569409a1f7009f10a572a0e33a113b48d9e704b39ff3f792eb173eb89e6f` |
| `captures/ascii-p8.png` | `94e1ea02a2f7834ebfa4ddd46b66cc91240c208aa30042f9159b3550e84819a2` |
| `captures/ascii-p9.png` | `f49ba070e508c56743a4c8451127217774333521027c895a5d7e3a78b5f25a25` |
| `captures/cga-main-menu.png` | `c87e87c75068f3d26f489b70a43fbbd4ab36c81ad97d89eb898056bd6bc59dd3` |
| `captures/cga-six-windows.png` | `79fd33a6767d075d746a5e404e5a7229da8b546f4f9e40062de6a92a39e95304` |
| `captures/chord-ctrlalt-held.png` | `19e49eb638921dd3bfa33829a4508d4e740432a0b72c5f198527f8077820531f` |
| `captures/chord-ctrlalt-released.png` | `005168679742a1d645b8f04d81a1c795424316b803d920196a2993c0b1264e37` |
| `captures/chord-shifts-held.png` | `bcc96e6ba2c4b8e33543703881ec2b564b4fc70b92c633507196007c105be99c` |
| `captures/chord-shifts-released.png` | `005168679742a1d645b8f04d81a1c795424316b803d920196a2993c0b1264e37` |
| `captures/ivt-after-sk.png` | `e000a98fd75258dbf071c36ef295e2a60cf147111c6ca9ed47995cd79bfdc3a7` |
| `captures/ivt-before-sk.png` | `f5be9d6adb4ba837acdf172238d539ef9ced7ad107612c2122087e279cd41ca1` |
| `captures/mda-main-menu.png` | `b2e246deeb14f8fa4cc58a79f8a2b2c96ef642bfe12aec699f8c3ccf1f2ce0e1` |
| `captures/mda-six-windows.png` | `11a1673dd293e8673c73aa5f630467f2bb69b7b435788235caffeb80c3f8f4c0` |
| `captures/skinst-activate.png` | `6d48f7849eeaf2eab3589cc4ae9f126954ddff758304bcb67720fd1b657263b3` |
| `captures/skinst-dialer.png` | `185c0f5ab53cc81e287822d89ad9f483f3c04da98fa0d9dc72612b6f5db15006` |
| `captures/skinst-main.png` | `9a5f661f77724908f2dbba2dc728a88abc53063fb14b0d408bf13df92187f23c` |
| `captures/skinst-screen.png` | `4e08bff006df47237ed9e0fb0cec98d8b8aa2b2390d2d56a52616c6c08528a7f` |
| `captures/vga-00-logon.png` | `a0f37e2454da6acdb5ff47001fce791479cbb2d3d179084190dda98a28906ad1` |
| `captures/vga-01-foreground.png` | `4dc506e576a46c374ef216c7ef220e6326f8dc0e0be1e8b1c157340c8f023efe` |
| `captures/vga-02-main-menu.png` | `847b3840564e21294c5d86607c2e2c32249b9d9aac8ddf50f48e8b5cf49f7e46` |
| `captures/vga-03-help.png` | `7c8082576fc0f96e467b64f98ba2fdbf19a5f3bfc14ccd04c4cd89bbdf95c046` |
| `captures/vga-03b-help-esc-returns-to-menu.png` | `847b3840564e21294c5d86607c2e2c32249b9d9aac8ddf50f48e8b5cf49f7e46` |
| `captures/vga-04-notepad.png` | `329aef2d199f7fbb5edf5e8d422bf4b07765a362713b2fef0b6a82a87ba3cec7` |
| `captures/vga-05-notepad-typed.png` | `f3f5c7426fd0c873c4e0d0cdbfe2ae081723c7346c17387503be6529149b7b85` |
| `captures/vga-06-calculator-dec.png` | `44b2dc7ab98797d8dfe7d8a67c5a1597118d299610488815c8bc9680c4e4c92f` |
| `captures/vga-07-calculator-hex.png` | `1babfec5fc8978045fa57f3b3b226ff721a7701d701da6210c2654347bc414c4` |
| `captures/vga-08-calculator-bin.png` | `73528dde99e527066c33f5c93610f2efe071c40a28eb298b7515c93b51bb819e` |
| `captures/vga-09-calendar.png` | `2943bb68868725c38617adb5fa16107ef2ae4547b0b0eb6ce2473653f02495f3` |
| `captures/vga-10-appointment.png` | `78f16ead8c93468918da2d860aae55a2b13bed43f1141691339b948f1b303479` |
| `captures/vga-11-dialer.png` | `50f5cdc0cf7abfe160cfe5d3807f39f8cdcf691660bdabb384dcf8bbb741c672` |
| `captures/vga-12-setup.png` | `004fbfd3cb77ea4a155be2c9c4d86f3a0ca470bbeba3a6850d5ef038461aee5e` |
| `captures/vga-13-restored-after-ctrlalt.png` | `5cf743625346938859188d15d11fdb1c53f26620ae38f78448ebcba112852455` |
| `captures/vga-skc-logon.png` | `bc58f2ffa432f5a9ac909903e774318239ce4193c62aa356fbca592f29c75dd3` |
| `captures/vga-skm-logon.png` | `eba05fa02f904edb4a6388736e09d4a9a431400a67183893a57a4088628ec48c` |
| `captures/vga-skn-logon.png` | `8d74e7c8ad0ab48df4821b2c5839669174097e155e37ac67403e9a24279968ae` |
