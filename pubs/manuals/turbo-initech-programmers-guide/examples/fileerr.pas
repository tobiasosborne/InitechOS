program FileErr;
var
  Report: file;
begin
  writeln('opening TPSCOVER.TXT');
  Assign(Report, 'TPSCOVER.TXT');
  Reset(Report, 1);
  writeln('opened')
end.
