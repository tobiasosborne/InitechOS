var
  P, B: string;
  F: file;
  D: string;
  N, I: integer;
begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:';
  IdeStart();
  P := ParamStr(1);
  DoLoad(P); TypeChar('X'); DoSave();
  Assign(F, P); Reset(F, 1); BlockRead(F, D[1], 255, N);
  Close(F); I := IOResult(); SetLength(D, N);
  if (UiMode = ModeError) or EdModified or (D <> 'XOLD' + chr(26)) then
  begin Writeln('FAIL save identity: ', P, ' mode=', UiMode, ' err=', ErrCode, ' dirty=', EdModified, ' bytes=', D); Halt(1) end;
  Writeln('PASS saved ', P)
end.
