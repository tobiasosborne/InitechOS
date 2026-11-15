{ host.pas -- replay a key file through the IDE and dump the screens.

  IDEKEYS.BIN holds two-byte little-endian key codes as editor.pas defines
  them; 65533 LINE COL shows a compiler error there; 65535 dumps the screen to SHOTnnn.SCR (4000 bytes of character and
  attribute pairs, then cursor column, row and visibility), 65534 stops.
  Only the untyped-file runtime is used, so the same program runs under
  Free Pascal on the host and, compiled by Turbo Initech, inside the OS. }
var
  Keys, Shot: file;
  KeyBuf: string;
  Dump: array[1..4003] of char;
  KeyGot, KeyCode, ShotNo, I: integer;
  Running: boolean;
  ShotName, Digits, ErrMsg: string;

procedure DumpScreen;
var Wrote, J: integer;
begin
  DrawAll();
  I := 0;
  while I < 2000 do
  begin
    Dump[I * 2 + 1] := ScrCh[I];
    Dump[I * 2 + 2] := chr(ScrAt[I]);
    I := I + 1
  end;
  Dump[4001] := chr(ScrCurCol);
  Dump[4002] := chr(ScrCurRow);
  if ScrCurOn then Dump[4003] := chr(1) else Dump[4003] := chr(0);
  ShotNo := ShotNo + 1;
  NumText(ShotNo, Digits);
  while Length(Digits) < 3 do Digits := '0' + Digits;
  ShotName := 'SHOT' + Digits + '.SCR';
  Assign(Shot, ShotName);
  Rewrite(Shot, 1);
  I := 1;
  while I <= 4003 do
  begin
    Chunk[1] := Dump[I];
    J := 1;
    while (J < 250) and (I + J <= 4003) do
    begin
      Chunk[J + 1] := Dump[I + J];
      J := J + 1
    end;
    BlockWrite(Shot, Chunk[1], J, Wrote);
    I := I + J
  end;
  Assign(Shot, 'SHOT.TMP')
end;

begin
  HostSetup();
  FileDrive := 'C';
  FileDir := 'C:' + chr(92) + 'TP55';
  IdeStart();
  ShotNo := 0;
  Assign(Keys, 'IDEKEYS.BIN');
  Reset(Keys, 1);
  Running := true;
  while Running do
  begin
    BlockRead(Keys, KeyBuf[1], 2, KeyGot);
    if KeyGot < 2 then Running := false
    else
    begin
      KeyCode := ord(KeyBuf[1]) + 256 * ord(KeyBuf[2]);
      if KeyCode = 65534 then Running := false
      else if KeyCode = 65535 then DumpScreen()
      else if KeyCode = 65533 then
      begin
        { a compiler error, as TP 5.5 reported it for the same source
          (MINT06 07, 34): line and source column follow }
        BlockRead(Keys, KeyBuf[1], 4, KeyGot);
        ErrMsg := 'Error 85: ";" expected';
        IdeError(ord(KeyBuf[1]) + 256 * ord(KeyBuf[2]),
                 ord(KeyBuf[3]) + 256 * ord(KeyBuf[4]), ErrMsg)
      end
      else
      begin
        IdeKey(KeyCode);
        if UiMode = ModeQuit then Running := false
      end
    end
  end;
  writeln('IDE-HOST-END shots=', ShotNo)
end.
