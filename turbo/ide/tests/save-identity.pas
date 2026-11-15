var
  P, B: string;
  F: file;
  D: string;
  N, I: integer;
begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:';
  IdeStart();
  if ParamStr(2) <> '' then
  begin WorkName0 := 'TURBOWK0.$$$'; WorkName1 := 'TURBOWK1.$$$' end;
  P := ParamStr(1);
  DoLoad(P); TypeChar('X');
  if ParamStr(2) = 'flush' then PageFlush();
  DoSave();
  Assign(F, P); Reset(F, 1); BlockRead(F, D[1], 255, N);
  Close(F); I := IOResult(); SetLength(D, N);
  if ParamStr(2) <> '' then
  begin
    if UiMode = ModeError then
    begin
      if (not EdModified) or (D <> 'OLD') then
      begin Writeln('FAIL refused alias changed original or dirty state'); Halt(1) end
    end
    else if EdModified or (D <> 'XOLD' + chr(26)) then
    begin Writeln('FAIL alias save reported success with wrong bytes'); Halt(1) end
  end
  else if (UiMode = ModeError) or EdModified or (D <> 'XOLD' + chr(26)) then
  begin Writeln('FAIL save identity: ', P, ' mode=', UiMode, ' err=', ErrCode, ' dirty=', EdModified, ' bytes=', D); Halt(1) end;
  Writeln('PASS saved ', P)
end.
