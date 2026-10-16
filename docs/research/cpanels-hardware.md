# R4.5 speaker contract (initech-tdnl.19)

Local source note for `os/flair/cp_service.h`'s IBM PC/AT citation. These are
hardware facts, not a pixel oracle or a proposed Mac panel layout.

IBM's Personal Computer XT Technical Reference, System Unit 1-20,
Speaker Interface / Speaker Drive System Block Diagram, assigns port 0061
bit 0 to timer channel 2's gate and bit 1 to the other speaker-drive input.
The diagram combines the two inputs. Its timer input is 1.19318 MHz.
Primary IBM page, reproduced at:
https://www.manualslib.com/manual/4563968/Ibm-Personal-Computer-Xt.html?page=30

IBM's Personal Computer AT Technical Reference, September 1985, System
Board 1-22, System Timers, retains channel 2 for the speaker and port 61
bit 0 for its gate, with an Intel 8254-2 timer:
https://minuszerodegrees.net/manuals/IBM_5170_Technical_Reference_6280070_SEP85.pdf

The existing local `os/milton/pit.c` documents the counter/control ports,
low-byte/high-byte programming and mode-3 bit fields. Channel 2 selects
port 42 and changes the channel-select bits of the existing mode-3 control
word from 00 to 10: 36h becomes B6h. Speaker enable preserves unrelated
port-61 bits; stopping clears bits 0 and 1. Timing uses the existing 100-Hz
pump counter. The selected tones and short/full durations are authored
Initech product choices recorded in `spec/cpanels/record.json`; the driver
does not claim sampled Mac audio or analog volume control.
