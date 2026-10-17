program BlockWrEx;
var
  Copy: file;
  Text, Back: string[30];
  Written, Got: integer;
begin
  Text := 'TPS report';
  Assign(Copy, 'REPORT.TXT');
  Rewrite(Copy, 1);
  BlockWrite(Copy, Text[1], 3, Written);
  write(Written, ' ');
  BlockWrite(Copy, Text[4], 7, Written);
  writeln(Written);
  Back := '----------';
  Reset(Copy, 1);
  BlockRead(Copy, Back[1], 10, Got);
  writeln(Back)
end.
