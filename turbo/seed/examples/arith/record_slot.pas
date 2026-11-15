{$MODE DELPHI}
program RecordSlot;
type OneField = record Value: integer; end;
var Before: integer;
    R: OneField;
    After: integer;
begin
  Before := 111; After := 222;
  R.Value := 37;
  Writeln('FIELD ', R.Value, ' BEFORE ', Before, ' AFTER ', After)
end.
