program ResetEx;
var
  Notes: file;
  Text: string[8];
  Count: integer;
begin
  Text := 'ABCDEFGH';
  Assign(Notes, 'NOTES.TXT');
  Rewrite(Notes, 1);
  BlockWrite(Notes, Text[1], 8, Count);
  Reset(Notes, 1);
  BlockRead(Notes, Text[1], 3, Count);
  write(Text, ' ');
  Reset(Notes, 1);
  BlockRead(Notes, Text[5], 2, Count);
  writeln(Text)
end.
