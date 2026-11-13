program Files;
var
  Memo: file;
  Text, Back: string[40];
  Wrote, Got: integer;
begin
  Text := 'Did you get the memo?';
  Assign(Memo, 'MEMO.TXT');
  Rewrite(Memo, 1);
  BlockWrite(Memo, Text[1], Length(Text),
             Wrote);
  Reset(Memo, 1);
  Back := '';
  { reserve room for the characters, then read }
  while Length(Back) < Wrote do
    Back := Back + ' ';
  BlockRead(Memo, Back[1], Wrote, Got);
  writeln('wrote ', Wrote, ', read ', Got);
  writeln(Back);
  BlockRead(Memo, Back[1], 1, Got);
  writeln('at the end: ', Got)
end.
