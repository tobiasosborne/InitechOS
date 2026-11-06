# Keyboard extended delivery (initech-wuxo0)

IBM Personal Computer AT Technical Reference, first edition September 1985,
part 6280070, System BIOS, "Extended Functions", printed pp.5-18--5-19.
Uncited locally: no reference PDF was present in docs/research. The original
IBM scan was acquired and read before implementation from:
https://www.minuszerodegrees.net/manuals/IBM_5170_Technical_Reference_6280070_SEP85.pdf

The table's second codes are decimal; the following values are hexadecimal.
The ROM listing also has K9 (CTL TABLE SCAN), K12 (UC TABLE SCAN),
K13 (ALT TABLE SCAN) and K15 (BASE CASE TABLE), printed pp.5-191--5-192.
An extended result has ASCII 00 and the second code below. DOS CON byte
reads serialize this as 00 followed by the second code (IBM DOS 3.30
Technical Reference pp.6-11--6-12, the existing kbd.c CON citation).

| Key | Base | Ctrl | Shift | Alt |
| --- | --- | --- | --- | --- |
| F1 | 3B | 5E | 54 | 68 |
| F2 | 3C | 5F | 55 | 69 |
| F3 | 3D | 60 | 56 | 6A |
| F4 | 3E | 61 | 57 | 6B |
| F5 | 3F | 62 | 58 | 6C |
| F6 | 40 | 63 | 59 | 6D |
| F7 | 41 | 64 | 5A | 6E |
| F8 | 42 | 65 | 5B | 6F |
| F9 | 43 | 66 | 5C | 70 |
| F10 | 44 | 67 | 5D | 71 |
| Home | 47 | 77 | | |
| Up | 48 | | | |
| PgUp | 49 | 84 | | |
| Left | 4B | 73 | | |
| Right | 4D | 74 | | |
| End | 4F | 75 | | |
| Down | 50 | | | |
| PgDn | 51 | 76 | | |
| Ins | 52 | | | |
| Del | 53 | | | |

Alt Q,W,E,R,T,Y,U,I,O,P use 10--19; Alt A,S,D,F,G,H,J,K,L use
1E--26; Alt Z,X,C,V,B,N,M use 2C--32. These are physical US Set-1
letter scans, independent of CapsLock and Shift. The ROM tests Alt before
Ctrl before Shift. Right Ctrl/Alt use E0 on the enhanced PS/2 keyboard;
the existing decoder cites IBM PS/2 Hardware Interface TR Figs.8--10.
Grey navigation has the same base/defined Ctrl codes as keypad navigation.

Contract: PRD Sec.5 / Sec.6.1 require PS/2 IRQ1 and DOS console I/O.
spec/int21h_register.json admits AH=01h--0Ch as Resident CON I/O;
spec/dos_structs.h defines data layouts, with no stronger keyboard promise.
AH=07h/08h return the two bytes on successive calls; AH=0Ah consumes
extended pairs as editor commands, rather than appending the scan as text.
The existing harness/conkeys manual vectors independently grade that editor.

The host expectations in test_kbd.c and emulator extended.hex are literal
transcriptions, independent of the artifact table. extended.keys exercises
40 function variants, 20 grey/keypad base navigation keys, 12 grey/keypad
Ctrl navigation keys and all 26 Alt letters, surrounded by d,i,r and CR.
The baked echo client alternates AH=07h/08h, serializing every byte as hex.
Both emulators must match the entire transcript and injection count.

Scope limits: this repair does not add BIOS INT 16h, NumLock state, numeric
keypad mode, Alt numeric entry, Ctrl-Up/Down/Ins/Del extensions from later
BIOSes, or Alt navigation codes from later BIOSes. The AT table does not
define those last Ctrl/Alt navigation results. No new promise is made for
Shift+keypad numeric behavior. The table stays resident: IRQ1 and DOS input
can run before the ADR-0014 COLD overlay is loaded.
