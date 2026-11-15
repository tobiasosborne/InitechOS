var PageFailures: integer;

procedure RawShot;
var Code: integer;
    F: file;
begin
  ShotName := 'PAGE.COM'; Code := Execute(ShotName);
  if Code <> 0 then Halt(2);
  ShotNo := ShotNo + 1; NumText(ShotNo, Digits);
  while Length(Digits) < 3 do Digits := '0' + Digits;
  ShotName := 'SHOT' + Digits + '.SCR';
  Assign(F, 'PAGE.BIN'); Rename(F, ShotName);
  if IOResult() <> 0 then Halt(3)
end;

begin
  CompilerDefaults(); ShotNo := 0; PageFailures := 0;
  ScreenPage(0, UserPage[1]);
  { Independently authored full-page cells and attributes exercise packing,
    unpacking and cursor order. RawShot crosses a separately compiled C gate. }
  I := 0;
  while I < 2000 do
  begin
    DisplayPage[I*2+1] := chr(65 + I mod 26);
    DisplayPage[I*2+2] := chr(16 + I mod 16); I := I + 1
  end;
  DisplayPage[4001] := chr(3); DisplayPage[4002] := chr(7);
  ScreenPage(1, DisplayPage[1]); RawShot();
  ScreenPage(0, DisplayPage[1]); I := 0;
  while I < 2000 do
  begin
    if (ord(DisplayPage[I*2+1]) <> 65 + I mod 26) or
       (ord(DisplayPage[I*2+2]) <> 16 + I mod 16) then PageFailures := PageFailures + 1;
    I := I + 1
  end;
  if (ord(DisplayPage[4001]) <> 3) or (ord(DisplayPage[4002]) <> 7) then PageFailures := PageFailures + 1;
  ScreenPage(1, UserPage[1]);
  FileDrive := 'C'; FileDir := 'C:' + chr(92) + 'TP55'; IdeStart();
  ShotName := 'DEMO.PAS'; DoLoad(ShotName);
  ProductKey(KeyF9); FinishBuild(); ProductPaint(); RawShot();
  ProductKey(KeyEsc); ProductKey(KeyUp); ProductKey(KeyEnd); ProductKey(ord(';'));
  DoSave(); ProductKey(KeyAltF9); FinishBuild(); ProductPaint(); RawShot();
  ProductKey(KeyEsc); ProductPaint(); RawShot();
  ShotName := 'EXIT7.PAS'; DoLoad(ShotName);
  ProductKey(KeyCtrlF9); FinishBuild(); ProductPaint(); RawShot();
  ProductKey(KeyAltF5); ProductPaint(); RawShot();
  ProductKey(ord(' ')); ProductPaint(); RawShot();
  ShotName := 'EXIT0.PAS'; DoLoad(ShotName);
  ProductKey(KeyCtrlF9); FinishBuild(); ProductPaint(); RawShot();
  ProductKey(KeyAltF5); ProductPaint(); RawShot();
  ProductKey(ord(' ')); ProductPaint(); RawShot();
  ProductCleanup(); ScreenPage(1, UserPage[1]);
  Writeln('PAGE-READ-FAILURES=', PageFailures);
  Writeln('turbo-native-ide-complete')
end.
