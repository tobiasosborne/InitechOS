var
  TestPath, ResultPath: string;
  OldLen, OldOrig, OldSide, OldStart, OldPgLen: integer;
  Failures: integer;

procedure Require(Yes: boolean; Msg: string);
begin
  if not Yes then begin Writeln('FAIL ', Msg); Failures := Failures + 1 end
end;

begin
  FileMode := 0;
  TestPath := 'DOC.PAS'; ResultPath := 'RESULT.PAS';
  WorkName0 := 'TURBOWK0.$$$'; WorkName1 := 'TURBOWK1.$$$';
  BufLoad(TestPath);
  PageLoad(2500);
  PageInsert(1, 'X', 2500);
  OldLen := DocLen; OldOrig := PgOrig; OldSide := WorkSide;
  OldStart := PgStart; OldPgLen := PgLen;
  FaultKind := ParamStr(1); Val(ParamStr(2), FaultAt);
  if ParamStr(3) = 'save' then BufSave(ResultPath) else PageLoad(1);
  Require(BufErr <> 0, 'fault must be reported');
  Require(PgDirty, 'dirty page retained');
  Require((DocLen = OldLen) and (PgOrig = OldOrig) and
          (WorkSide = OldSide) and (PgStart = OldStart) and
          (PgLen = OldPgLen) and (Pg[1] = 'X'), 'page state retained');
  FaultKind := ''; BufErr := 0;
  BufSave(ResultPath);
  Require(BufErr = 0, 'retry saves');
  Writeln('fault operations=', FaultSeen);
  if Failures <> 0 then Halt(1)
end.
