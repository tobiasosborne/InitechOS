var
  P, D: string;
  F: file;
  N, I: integer;
procedure Require(Yes: boolean; Msg: string);
begin
  if not Yes then begin Writeln('FAIL ', Msg); Halt(1) end
end;
procedure Expect(Path, Expected: string);
begin
  Assign(F, Path); Reset(F, 1); Require(IOResult() = 0, 'exists: ' + Path);
  BlockRead(F, D[1], 255, N); Close(F); I := IOResult(); SetLength(D, N);
  Require(D = Expected, 'bytes: ' + Path)
end;
begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:';
  IdeStart(); UiMode := ModeEditor; TypeChar('X');
  SaveCommand(); NameText := 'DOC.PAS';
  if ParamStr(1) = 'open' then begin FaultKind := 'open'; FaultAt := 1 end;
  NameAccept(); FaultKind := '';
  if ParamStr(1) = 'open' then
  begin
    Require(UiMode = ModeError, 'probe error is refused');
    Require(EdModified, 'probe failure stays dirty');
    Expect('DOC.PAS', 'OLD')
  end
  else
  begin
    Require(UiMode = ModeOverwrite, 'first F2 confirms overwrite before writing');
    Expect('DOC.PAS', 'OLD');
    if ParamStr(1) = 'yes' then
    begin
      IdeKey(ord('Y'));
      Require(not EdModified, 'confirmed first save succeeds');
      Expect('DOC.PAS', 'X' + chr(26)); Expect('DOC.BAK', 'OLD')
    end
    else
    begin
      if ParamStr(1) = 'no' then IdeKey(ord('N')) else IdeKey(KeyEsc);
      Require(EdModified, 'refused overwrite stays dirty');
      Expect('DOC.PAS', 'OLD')
    end
  end;
  Writeln('PASS first save ', ParamStr(1))
end.
