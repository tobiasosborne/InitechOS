program AssignEx;
var
  Log: file;
  Name: string[12];
  Count: integer;
  Line: string[20];
begin
  Name := 'TPS';
  Assign(Log, Name + '.LOG');
  Rewrite(Log, 1);
  Line := 'cover sheet';
  BlockWrite(Log, Line[1], Length(Line), Count);
  writeln(Count, ' bytes written to ', Name,
          '.LOG')
end.
