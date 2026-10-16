program RewriteEx;
var
  Ledger: file;
  Text: string[12];
  Count: integer;
begin
  Assign(Ledger, 'LEDGER.TXT');
  Rewrite(Ledger, 1);
  Text := 'old contents';
  BlockWrite(Ledger, Text[1], 12, Count);
  Rewrite(Ledger, 1);
  Text := 'new';
  BlockWrite(Ledger, Text[1], 3, Count);
  Reset(Ledger, 1);
  Text := '............';
  BlockRead(Ledger, Text[1], 12, Count);
  writeln(Count, ' ', Text)
end.
