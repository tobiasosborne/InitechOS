begin
  FileMode := 0; CompilerDefaults();
  FileDrive := 'C'; FileDir := 'C:' + chr(92) + 'TP55'; IdeStart();
  ShotName := 'EXIT7.PAS'; DoLoad(ShotName); ShotNo := 0;
  RunTrace := ''; RunStatus := 7;
  ProductKey(KeyCtrlF9); FinishBuild();
  if (UiMode <> ModeEditor) or (not ProgramReady) or
     (RunTrace <> '1E0') or (RunPath <> 'EXIT7.COM') then
  begin Writeln('FAIL Run ', UiMode, ' trace=', RunTrace, ' path=', RunPath); Halt(1) end;
  DumpScreen();
  ProductKey(KeyAltF5); ProductPaint();
  if (UiMode <> ModeUser) or (RunTrace <> '1E01') then
  begin Writeln('FAIL user screen ', RunTrace); Halt(1) end;
  ProductKey(ord(' '));
  if UiMode <> ModeEditor then begin Writeln('FAIL return from user screen'); Halt(1) end;
  DumpScreen(); RunTrace := ''; RunStatus := 0;
  ProductKey(KeyCtrlF9); FinishBuild();
  if (UiMode <> ModeEditor) or (RunTrace <> '1E0') then
  begin Writeln('FAIL zero exit ', RunTrace); Halt(1) end;
  ProductCleanup(); Writeln('PASS Run returns to editor at exit 7 and 0')
end.
