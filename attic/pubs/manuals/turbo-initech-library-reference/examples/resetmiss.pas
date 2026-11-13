program ResetMiss;
var
  Old: file;
begin
  writeln('looking for LEDGER.OLD');
  Assign(Old, 'LEDGER.OLD');
  Reset(Old, 1);
  writeln('found it')
end.
