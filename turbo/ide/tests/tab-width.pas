var P, S: string;
    F: file;
    N, I, J, Wrote, Want: integer;
begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:'; IdeStart();
  P := 'TABS.PAS';
  for N := 248 to 253 do
  begin
    S := '';
    for I := 1 to N do S := S + 'A';
    S := S + chr(9) + 'X';
    Assign(F, P); Rewrite(F, 1); BlockWrite(F, S[1], Length(S), Wrote); Close(F);
    DoLoad(P); EdLeft := 249; DrawAll();
    Want := (N div 8 + 1) * 8 + 1 - 249;
    if ScrCh[160 + Want] <> 'X' then begin Writeln('FAIL long tab column ', N); Halt(1) end
  end;
  S := ''; for I := 1 to 32 do S := S + chr(9); S := S + 'X';
  Assign(F, P); Rewrite(F, 1); BlockWrite(F, S[1], Length(S), Wrote); Close(F);
  DoLoad(P); EdLeft := 249; DrawAll();
  if ScrCh[168] <> 'X' then begin Writeln('FAIL 32 tabs at column 257'); Halt(1) end;
  Writeln('PASS tab expansion beyond ShortString width')
end.
