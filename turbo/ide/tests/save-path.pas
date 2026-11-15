var P, Q, D: string;
    F: file;
    N, I: integer;
procedure Expect(Path, Expected: string);
begin
  Assign(F, Path); Reset(F, 1); BlockRead(F, D[1], 255, N);
  Close(F); I := IOResult(); SetLength(D, N);
  if D <> Expected then begin Writeln('FAIL path bytes: ', Path); Halt(1) end
end;
begin
  FileMode := 0; FileDrive := 'C'; FileDir := 'C:';
  IdeStart();
  P := ParamStr(1); Q := ParamStr(2);
  DoLoad(P); TypeChar('X'); DoSave();
  if EdModified then begin Writeln('FAIL path save err=', ErrCode); Halt(1) end;
  Expect(P, 'XOLD' + chr(26));
  Expect('DOC.PAS', 'ROOT-SENTINEL');
  NameKind := NameWrite; WriteTo(Q, false);
  TypeChar('Y'); DoSave();
  if EdModified then begin Writeln('FAIL Write to then F2'); Halt(1) end;
  Expect(Q, 'XYOLD' + chr(26));
  Expect(P, 'XOLD' + chr(26)); Expect('DOC.PAS', 'ROOT-SENTINEL');
  Writeln('PASS full identity through Load, Write to and F2')
end.
