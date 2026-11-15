var TestPath, ResultPath: string;
    OldStart, OldLen, OldOrig, OldLine, OldSide, First: integer;
    Failures: integer;
procedure Require(Yes: boolean; Msg: string);
begin
  if not Yes then begin Writeln('FAIL ', Msg); Failures := Failures + 1 end
end;
begin
  FileMode := 0; TestPath := 'DOC.PAS'; ResultPath := 'RESULT.PAS';
  WorkName0 := 'TURBOWK0.$$$'; WorkName1 := 'TURBOWK1.$$$';
  BufLoad(TestPath); PageLoad(2500); PageInsert(1, 'X', 2500); PageFlush();
  if BufErr <> 0 then Halt(2);
  OldStart := PgStart; OldLen := PgLen; OldOrig := PgOrig;
  OldLine := PgLine; OldSide := WorkSide;
  FaultKind := ParamStr(1); Val(ParamStr(2), FaultAt); Val(ParamStr(3), First);
  PageLoad(First);
  Require(BufErr <> 0, 'reload fault reported');
  Require((PgStart = OldStart) and (PgLen = OldLen) and (PgOrig = OldOrig) and
          (PgLine = OldLine) and (WorkSide = OldSide) and (not PgDirty) and
          (Pg[1] = 'X'), 'previous committed page retained');
  Require(IOResult() = 0, 'reload consumes pending I/O error');
  FaultKind := ''; BufErr := 0; PageLoad(First);
  Require(BufErr = 0, 'reload retry succeeds');
  BufSave(ResultPath); Require(BufErr = 0, 'save after reload retry succeeds');
  if Failures <> 0 then Halt(1)
end.
