program SaveFile;
var
  Store: file;
  Memo, Back: string;
  Count: integer;
begin
  Memo := 'Remember the new cover sheets';
  Assign(Store, 'MEMO.TXT');
  Rewrite(Store, 1);
  BlockWrite(Store, Memo[1], Length(Memo),
             Count);
  writeln('saved ', Count, ' characters');
  Back := Memo;
  Back[1] := '?';
  Reset(Store, 1);
  BlockRead(Store, Back[1], Length(Back),
            Count);
  writeln('read back: ', Back)
end.
