{ smoke.pas -- canonical end-to-end seed codegen smoke (beads initech-znb).
  Ref: PRD Sec 6.7 -- the seed must compile a tiny Pascal program to a
  freestanding binary that prints a known string over serial in QEMU.
  The harness asserts the marker "InitechOS seed OK" on COM1. }
program Smoke;
begin
  writeln('InitechOS seed OK')
end.
