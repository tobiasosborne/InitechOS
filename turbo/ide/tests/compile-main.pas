begin
  FileMode := 0; CompilerDefaults();
  FileDrive := 'C'; FileDir := 'C:' + chr(92) + 'TP55'; IdeStart();
  ShotName := 'DEMO.PAS'; DoLoad(ShotName); ShotNo := 0;
  ProductKey(KeyF9); FinishBuild();
  if (FirstKind <> 2) or (FirstExpected <> TkSemi) or
     (EdLine <> 4) or (EdCol <> 3) or ProgramReady then
  begin Writeln('FAIL actual first-error record ', FirstKind, ' ', FirstExpected, ' at ', EdLine, ':', EdCol, ' detail ', FirstDetail, ' msg ', ErrLineText); Halt(1) end;
  DumpScreen();
  ProductKey(KeyEsc); ProductKey(KeyUp); ProductKey(KeyEnd); ProductKey(ord(';'));
  DoSave(); ProductKey(KeyAltF9); FinishBuild();
  if (FirstKind <> 0) or (not CompileDone) or (not ProgramReady) or (CompileLines <> 5) then
  begin Writeln('FAIL actual compilation'); Halt(1) end;
  DumpScreen(); ProductKey(KeyEsc); DumpScreen();
  ShotName := ReadyName; Digits := 'BEFORE.COM'; FileCopy(ShotName, Digits);
  ProductKey(KeyEnd); ProductKey(KeyBack); ProductKey(KeyF9); FinishBuild();
  if ProgramReady or (not ErrLineShown) then
  begin Writeln('FAIL stale executable accepted'); Halt(1) end;
  Writeln('PASS compiler, real Error 85, Compile/Make and fresh output')
end.
