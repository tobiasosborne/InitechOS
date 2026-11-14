{ ReadKey in the booted image (turbo/ide/README.md, tier 1). The harness
  types a, F9, Alt-X, Ctrl-F9, Up, q after "run". Expected, from the IBM AT
  BIOS extended codes (0 then the scan code): 97 / 0 67 / 0 45 / 0 102 /
  0 72 / 113, as readkey.expect. }
program ReadKeys;
var
  C: char;
  Done: boolean;
begin
  Done := false;
  while not Done do
  begin
    C := ReadKey();
    Writeln('KEY ', ord(C));
    if C = 'q' then Done := true
  end
end.
