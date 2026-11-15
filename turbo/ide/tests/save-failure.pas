var
  P, B, S: string;
  Code, I, Got: integer;
  F: file;

procedure Require(Yes: boolean; Msg: string);
begin
  if not Yes then begin Writeln('FAIL ', Msg); Halt(1) end
end;

procedure Expect(var Path: string; Expected: string);
var Data: string;
    N: integer;
begin
  Assign(F, Path); Reset(F, 1); Require(IOResult() = 0, 'file exists: ' + Path);
  System.BlockRead(F, Data[1], 255, N); Close(F); I := IOResult();
  SetLength(Data, N);
  Require(Data = Expected, 'bytes: ' + Path)
end;

begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:';
  IdeStart();
  P := 'DOC.PAS'; B := 'DOC.BAK';
  DoLoad(P); TypeChar('X');
  FaultKind := ParamStr(1); Val(ParamStr(2), FaultAt);
  DoSave();
  Code := ErrCode;
  FaultKind := '';
  if (ParamStr(1) = 'target') and (UiMode <> ModeError) then
  begin
    Expect(P, 'XOLD' + chr(26));
    Require(not EdModified, 'successful staged save clears dirty');
    Halt(0)
  end;
  if ParamStr(1) = 'rename-double' then
  begin
    Require(UiMode = ModeError, 'double rename failure reported');
    Require(EdModified, 'double failure remains dirty');
    Expect(B, 'OLD');
    S := 'DOC.' + chr(36) + chr(36) + chr(36);
    Expect(S, 'XOLD' + chr(26));
    Halt(0)
  end;
  Require(UiMode = ModeError, 'save must report injected failure');
  Require(EdModified, 'failed save remains dirty');
  Expect(P, 'OLD');
  Writeln('PASS save failure preserves the named old bytes, code=', Code)
end.
