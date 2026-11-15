{ editor.pas -- the Turbo Initech IDE: chrome, menus, prompts and editing.

  Layout, attributes and behaviour follow the TP 5.5 captures named in
  turbo/README.md "IDE" (turbopascal-decomp MINT06, cited by shot number).
  Keys arrive as DOS reports them through AH=07h: a character code, or
  256 + scan code for an extended key. }
const
  KeyEnter = 13; KeyEsc = 27; KeyBack = 8; KeyTab = 9; KeyCtrlY = 25;
  KeyF2 = 316; KeyF3 = 317; KeyF9 = 323; KeyF10 = 324;
  KeyAltF9 = 368; KeyCtrlF9 = 358; KeyAltF5 = 364;
  KeyAltX = 301; KeyAltE = 274; KeyAltF = 289; KeyAltR = 275; KeyAltC = 302;
  KeyHome = 327; KeyUp = 328; KeyPgUp = 329; KeyLeft = 331; KeyRight = 333;
  KeyEnd = 335; KeyDown = 336; KeyPgDn = 337; KeyIns = 338; KeyDel = 339;
  KeyCtrlEnd = 373; KeyCtrlPgDn = 374; KeyCtrlHome = 375; KeyCtrlPgUp = 388;

  ModeEditor = 0; ModeBar = 1; ModeMenu = 2; ModeName = 3; ModeVerify = 4;
  ModeQuit = 5; ModeError = 6; ModeOverwrite = 7;
  MenuFile = 1; MenuEdit = 2; MenuRun = 3; MenuCompile = 4;
  NameRename = 1; NameLoad = 2; NameWrite = 3;
  VerifyNew = 1; VerifyLoad = 2; VerifyQuit = 3;
  ViewRows = 22;

var
  EdLine, EdCol, EdTop, EdLeft: integer;
  EdInsert, EdModified, EdOnDisk: boolean;
  FileName, FileDrive, FileDir: string;
  UiMode, MenuSel, LastMenu, NameKind, NameRow, NameCol, VerifyKind: integer;
  ItemSel: array[1..4] of integer;
  NameText, PendingName: string;
  NameFresh: boolean;
  ErrCode, ErrReturn: integer;       { Error box (MINT06 51, 54) }
  ErrLineShown: boolean;             { compiler error on the info row (07) }
  ErrLineText: string;
  Tx: string;

{ ---- line geometry: tabs at 8-column stops (MINT06 36, 34) ---- }

function DispLen(var S: string): integer;
var I, C: integer;
begin
  C := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = chr(9) then C := (C div 8 + 1) * 8 else C := C + 1;
    I := I + 1
  end;
  DispLen := C
end;

{ Byte index (1-based) of the first byte whose display position is at or
  after column Col; Length+1 past the end. }
function ColByte(var S: string; Col: integer): integer;
var I, C, Found: integer;
begin
  C := 1; I := 1; Found := 0;
  while I <= Length(S) do
  begin
    if Found = 0 then
      if C >= Col then Found := I;
    if S[I] = chr(9) then C := ((C - 1) div 8 + 1) * 8 + 1 else C := C + 1;
    I := I + 1
  end;
  if Found = 0 then Found := Length(S) + 1;
  ColByte := Found
end;

function ByteCol(var S: string; B: integer): integer;
var I, C: integer;
begin
  C := 1; I := 1;
  while I < B do
  begin
    if I <= Length(S) then
    begin
      if S[I] = chr(9) then C := ((C - 1) div 8 + 1) * 8 + 1 else C := C + 1
    end;
    I := I + 1
  end;
  ByteCol := C
end;

procedure Expand(var S, D: string);
var I: integer;
    C: string;
begin
  D := ''; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = chr(9) then
    begin
      D := D + ' ';
      while Length(D) mod 8 <> 0 do D := D + ' '
    end
    else
    begin
      C := S[I];
      D := D + C
    end;
    I := I + 1
  end
end;

{ ---- view keeping ---- }

procedure ScrollTo;
begin
  if EdLine < EdTop then EdTop := EdLine;
  if EdLine > EdTop + ViewRows - 1 then EdTop := EdLine - ViewRows + 1;
  if EdCol < EdLeft then EdLeft := EdCol;
  if EdCol - EdLeft > 78 then EdLeft := EdCol - 78
end;

{ A jump far away puts the line on view row 11 (MINT06 34) without
  scrolling past the last line (33). }
procedure JumpTo;
begin
  EdTop := EdLine - 11;
  if EdTop > DocLines - ViewRows + 2 then EdTop := DocLines - ViewRows + 2;
  if EdTop < 1 then EdTop := 1;
  ScrollTo()
end;

{ ---- drawing ---- }

procedure DrawText;
var R, L, C: integer;
    S, D: string;
begin
  PageCover(EdTop, ViewRows);
  if BufErr <> 0 then
  begin
    { A failed page replacement leaves the old page live. Stay on it;
      never use the requested line numbers to index another page. }
    EdTop := PgLine;
    if not PageHas(EdLine) then EdLine := PgLine;
    ErrCode := BufErr; ErrReturn := ModeEditor; UiMode := ModeError
  end;
  R := 0;
  while R < ViewRows do
  begin
    ScrFill(R + 2, 0, 79, ' ', AtText);
    L := EdTop + R;
    if L <= DocLines then
    begin
      LineText(L, S);
      Expand(S, D);
      C := 0;
      while C < 80 do
      begin
        if EdLeft + C <= Length(D) then ScrPut(R + 2, C, D[EdLeft + C], AtText);
        C := C + 1
      end
    end;
    R := R + 1
  end
end;

procedure DrawInfo;
var N: string;
begin
  if ErrLineShown then
  begin
    { MINT06 07: the error replaces the info row, 0x4E across it }
    ScrFill(1, 0, 79, ' ', AtError);
    ScrText(1, 0, ErrLineText, AtError)
  end
  else
  begin
  ScrFill(1, 0, 79, ' ', AtInfo);
  Tx := 'Line'; ScrText(1, 6, Tx, AtInfo);
  NumText(EdLine, N); ScrText(1, 11, N, AtInfo);
  Tx := 'Col'; ScrText(1, 17, Tx, AtInfo);
  NumText(EdCol, N); ScrText(1, 21, N, AtInfo);
  if EdInsert then begin Tx := 'Insert'; ScrText(1, 25, Tx, AtInfo) end;
  if EdModified then ScrPut(1, 57, '*', AtInfo);
  Tx := FileDrive + ':' + FileName;
  ScrText(1, 59, Tx, AtInfo)
  end
end;

procedure DrawTitle(Which, Col: integer);
begin
  if Which = 1 then Tx := 'File'
  else if Which = 2 then Tx := 'Edit'
  else if Which = 3 then Tx := 'Run'
  else Tx := 'Compile';
  if ((UiMode = ModeBar) or (UiMode = ModeMenu)) and (MenuSel = Which) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeVerify) and (MenuSel = Which) and (VerifyKind <> VerifyQuit) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeName) and (MenuSel = Which) and (NameKind <> NameRename) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeOverwrite) and (MenuSel = Which) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeError) and (MenuSel = Which) and (ErrReturn = ModeMenu) then
    ScrText(0, Col, Tx, AtSelect)
  else
  begin
    ScrText(0, Col, Tx, AtMenu);
    ScrAttr(0, Col, AtMenuKey)
  end;
end;

procedure DrawBar;
begin
  ScrFill(0, 0, 79, ' ', AtMenu);
  DrawTitle(1, 4); DrawTitle(2, 12); DrawTitle(3, 20); DrawTitle(4, 27)
end;

procedure DrawStatus;
begin
  ScrFill(24, 0, 79, ' ', AtMenu);
  if (UiMode = ModeVerify) or (UiMode = ModeError) or (UiMode = ModeOverwrite) then
  begin
    Tx := 'Esc'; ScrText(24, 1, Tx, AtMenuKey);
    Tx := '-Abort'; ScrText(24, 4, Tx, AtMenu)
  end
  else
  begin
    Tx := 'F9'; ScrText(24, 1, Tx, AtMenuKey);
    Tx := '-Make'; ScrText(24, 3, Tx, AtMenu);
    Tx := 'F10'; ScrText(24, 10, Tx, AtMenuKey);
    Tx := '-Menu'; ScrText(24, 13, Tx, AtMenu)
  end
end;

function ItemCount(Menu: integer): integer;
begin
  if Menu = MenuFile then ItemCount := 5 else ItemCount := 2
end;

procedure ItemText(Menu, Item: integer; var S: string);
begin
  S := '';
  if Menu = MenuFile then
  begin
    if Item = 1 then S := ' Load      F3 '
    else if Item = 2 then S := ' New          '
    else if Item = 3 then S := ' Save      F2 '
    else if Item = 4 then S := ' Write to     '
    else S := ' Quit   Alt-X '
  end
  else if Menu = MenuRun then
  begin
    if Item = 1 then S := ' Run           Ctrl-F9 ' else S := ' User screen    Alt-F5 '
  end
  else
  begin
    if Item = 1 then S := ' Compile  Alt-F9                 '
    else S := ' Make         F9                 '
  end
end;

{ Pulldown frames as captured: File (3,1)-(18,..) MINT06 19, Run (19,1)
  20, Compile (26,1) 21; absent items are removed, the rest keep order. }
function MenuLeft(Menu: integer): integer;
begin
  if Menu = MenuFile then MenuLeft := 3
  else if Menu = MenuRun then MenuLeft := 19
  else MenuLeft := 26
end;

procedure DrawMenu;
var I, L: integer;
    S: string;
begin
  L := MenuLeft(MenuSel);
  ItemText(MenuSel, 1, S);
  ScrBox(1, L, ItemCount(MenuSel) + 2, L + Length(S) + 1, false, AtMenu);
  I := 1;
  while I <= ItemCount(MenuSel) do
  begin
    ItemText(MenuSel, I, S);
    if I = ItemSel[MenuSel] then
      ScrText(I + 1, L + 1, S, AtSelect)
    else
    begin
      ScrText(I + 1, L + 1, S, AtMenu);
      ScrAttr(I + 1, L + 2, AtMenuKey)
    end;
    I := I + 1
  end
end;

procedure DrawName;
var Len: integer;
begin
  ScrBox(NameRow, NameCol, NameRow + 2, NameCol + 43, false, AtMenu);
  ScrFill(NameRow + 1, NameCol + 1, NameCol + 42, ' ', AtMenu);
  if NameKind = NameRename then Tx := 'Rename NONAME'
  else if NameKind = NameLoad then Tx := 'Load File Name'
  else Tx := 'New Name';
  ScrTitle(NameRow, NameCol, NameCol + 43, Tx, AtMenu);
  ScrText(NameRow + 1, NameCol + 2, NameText, AtSelect);
  Len := Length(NameText);
  ScrCursor(NameRow + 1, NameCol + 2 + Len, true)
end;

procedure DrawVerify;
var W: integer;
begin
  Tx := ' ' + FileName + ' not saved.  Save? (Y/N) ';
  W := Length(Tx) + 2;
  ScrBox(5, 10, 7, 10 + W - 1, true, AtVerify);
  ScrText(6, 11, Tx, AtMenu);
  Tx := 'Verify';
  ScrTitle(5, 10, 10 + W - 1, Tx, AtVerify)
end;

procedure LeafName(var S, L: string); forward;

{ Overwrite prompt (MINT06 49): a Verify box where the name box was. }
procedure DrawOverwrite;
var L: string;
    W: integer;
begin
  LeafName(PendingName, L);
  Tx := ' Overwrite ' + L + '? (Y/N) ';
  W := Length(Tx) + 2;
  ScrBox(NameRow, NameCol, NameRow + 2, NameCol + W - 1, true, AtVerify);
  ScrText(NameRow + 1, NameCol + 1, Tx, AtMenu);
  Tx := 'Verify';
  ScrTitle(NameRow, NameCol, NameCol + W - 1, Tx, AtVerify)
end;

{ Error box (MINT06 51, 54): DOS 3 and 5 as captured; others not captured. }
procedure DrawError;
var W: integer;
    N: string;
begin
  { creating a file: 2 or 3 both mean the path cannot be reached }
  if (ErrCode = 2) or (ErrCode = 3) then Tx := 'Invalid drive or directory.'
  else if ErrCode = 5 then Tx := 'File is read only.'
  else
  begin
    NumText(ErrCode, N);
    Tx := 'File error ' + N + '.'
  end;
  Tx := ' ' + Tx + '  Press ';
  W := Length(Tx) + 5 + 2;
  ScrBox(10, 20, 12, 20 + W - 1, true, AtErrFrame);
  ScrText(11, 21, Tx, AtErrText);
  N := 'ESC';
  ScrText(11, 21 + Length(Tx), N, AtError);
  N := '. ';
  ScrText(11, 24 + Length(Tx), N, AtErrText);
  N := 'Error';
  ScrTitle(10, 20, 20 + W - 1, N, AtErrText)
end;

procedure DrawAll;
begin
  DrawText();
  DrawInfo();
  DrawBar();
  DrawStatus();
  ScrCursor(EdLine - EdTop + 2, EdCol - EdLeft, UiMode = ModeEditor);
  if (UiMode = ModeMenu) or ((UiMode = ModeVerify) and (VerifyKind <> VerifyQuit)) then
    DrawMenu()
  else if (UiMode = ModeName) and (NameKind <> NameRename) then
    DrawMenu()
  else if UiMode = ModeOverwrite then
    DrawMenu()
  else if (UiMode = ModeError) and (ErrReturn = ModeMenu) then
    DrawMenu();
  if UiMode = ModeName then DrawName();
  if UiMode = ModeVerify then DrawVerify();
  if UiMode = ModeOverwrite then DrawOverwrite();
  if UiMode = ModeError then DrawError()
end;

{ ---- file commands ---- }

procedure Upper(var S: string);
var I: integer;
begin
  I := 1;
  while I <= Length(S) do
  begin
    if (S[I] >= 'a') and (S[I] <= 'z') then S[I] := chr(ord(S[I]) - 32);
    I := I + 1
  end
end;

procedure BakName(var S, B: string);
var I, Dot: integer;
    C: string;
begin
  Dot := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = '.' then Dot := I;
    if S[I] = chr(92) then Dot := 0;
    I := I + 1
  end;
  B := ''; I := 1;
  if Dot = 0 then Dot := Length(S) + 1;
  while I < Dot do
  begin
    C := S[I];
    B := B + C;
    I := I + 1
  end;
  B := B + '.BAK'
end;

procedure CursorHome;
begin
  EdLine := 1; EdCol := 1; EdTop := 1; EdLeft := 1
end;

procedure DoNew;
begin
  BufNew();
  FileName := 'NONAME.PAS';
  EdModified := false; EdOnDisk := false;
  CursorHome();
  UiMode := ModeEditor
end;

procedure ShowFileError(Code, Back: integer);
begin
  ErrCode := Code;
  ErrReturn := Back;
  UiMode := ModeError
end;

{ A missing file opens as a new, empty buffer of that name (TP 5.5 F3;
  not in the captures). Other errors leave the buffer as it was. }
procedure DoLoad(var Name: string);
var L: string;
begin
  BufLoad(Name);
  LeafName(Name, L);
  if BufErr = 2 then
  begin
    BufNew();
    FileName := L;
    EdModified := false; EdOnDisk := false;
    CursorHome();
    UiMode := ModeEditor
  end
  else if BufErr <> 0 then ShowFileError(BufErr, ModeMenu)
  else
  begin
    FileName := L;
    EdModified := false; EdOnDisk := true;
    CursorHome();
    UiMode := ModeEditor
  end
end;

procedure ExtName(var S, Ext, B: string);
var I, Dot: integer;
    C: string;
begin
  Dot := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = '.' then Dot := I;
    if S[I] = chr(92) then Dot := 0;
    I := I + 1
  end;
  B := ''; I := 1;
  if Dot = 0 then Dot := Length(S) + 1;
  while I < Dot do
  begin
    C := S[I];
    B := B + C;
    I := I + 1
  end;
  B := B + Ext
end;

{ DOS names compare without case; generated siblings must never alias
  the source. .BAK sources use .BK1 for their previous version. }
function SameName(var A, B: string): boolean;
var X, Y: string;
begin
  X := A; Y := B; Upper(X); Upper(Y);
  SameName := X = Y
end;

{ Write aside, close and check, then use only DOS renames to publish.
  MINT06 54 refuses a read-only source; 49 confirms overwrites. The
  operator's save ruling permits the residual double rename failure:
  leave both the backup and stage intact if rollback also fails. }
function SaveAs(var Path: string; Existing: boolean): integer;
var Temp, Bak, Ext: string;
    Code, I, Attr: integer;
    F: file;
    Moved, KeepStage, StageOwned: boolean;
begin
  Ext := '.$$$'; ExtName(Path, Ext, Temp);
  Ext := '.BAK'; ExtName(Path, Ext, Bak);
  if SameName(Path, Temp) then
  begin Ext := '.$$1'; ExtName(Path, Ext, Temp) end;
  if SameName(Path, Bak) then
  begin Ext := '.BK1'; ExtName(Path, Ext, Bak) end;
  Code := 0; Moved := false; KeepStage := false; StageOwned := false;
  { A pre-existing stage belongs to someone else. Never truncate or erase
    it, including when a prior rollback left recovery material behind. }
  Assign(F, Temp); Attr := FileAttributes(F);
  if Attr >= 0 then Code := 80
  else if Attr <> -2 then Code := -Attr;
  if SameName(Path, WorkName0) or SameName(Path, WorkName1) then Code := 5;
  if Existing and (Code = 0) then
  begin
    Assign(F, Path);
    Attr := FileAttributes(F);
    if Attr < 0 then Code := -Attr
    else if Attr mod 2 = 1 then Code := 5
  end;
  if Code = 0 then
  begin
    StageOwned := true;
    BufSave(Temp);
    Code := BufErr
  end;
  if (Code = 0) and Existing then
  begin
    Assign(F, Bak); Erase(F); I := IOResult();
    if (I <> 0) and (I <> 2) then Code := I;
    if Code = 0 then
    begin
      Assign(F, Path); Rename(F, Bak); Code := IOResult();
      Moved := Code = 0
    end
  end;
  if Code = 0 then
  begin
    Assign(F, Temp); Rename(F, Path); Code := IOResult();
    if (Code <> 0) and Moved then
    begin
      Assign(F, Bak); Rename(F, Path); I := IOResult();
      KeepStage := I <> 0
    end
  end;
  if (Code <> 0) and StageOwned and (not KeepStage) then
  begin
    Assign(F, Temp); Erase(F); I := IOResult()
  end;
  SaveAs := Code
end;

function FileExists(var Path: string): boolean;
var F: file;
    Code: integer;
begin
  BufErr := 0;
  Assign(F, Path);
  Reset(F, 1);
  Code := IOResult();
  if Code = 0 then
  begin
    Close(F);
    Code := IOResult();
    FileExists := Code = 0
  end
  else FileExists := false;
  { Only FILE NOT FOUND proves absence. Access/close errors must not turn
    an existing destination into an unconfirmed new-file save. }
  if (Code <> 0) and (Code <> 2) then BufErr := Code
end;

procedure DoSave;
var Code: integer;
begin
  Code := SaveAs(FileName, EdOnDisk);
  if Code = 0 then
  begin
    EdOnDisk := true;
    EdModified := false;
    UiMode := ModeEditor
  end
  else ShowFileError(Code, ModeEditor)
end;

procedure OpenName(Kind: integer);
begin
  NameKind := Kind;
  NameFresh := true;
  if Kind = NameRename then
  begin
    NameRow := 3; NameCol := 7;
    NameText := FileDir + chr(92) + FileName
  end
  else
  begin
    NameCol := 5;
    NameRow := ItemSel[MenuFile] + 2;
    if Kind = NameLoad then NameText := '*.PAS' else NameText := ''
  end;
  UiMode := ModeName
end;

function Wild(var S: string): boolean;
var I: integer;
    W: boolean;
begin
  W := false; I := 1;
  while I <= Length(S) do
  begin
    if (S[I] = '*') or (S[I] = '?') then W := true;
    I := I + 1
  end;
  Wild := W
end;

{ Strip a typed drive and directory for the info line and file name. }
procedure LeafName(var S, L: string);
var I: integer;
    C: string;
begin
  L := ''; I := 1;
  while I <= Length(S) do
  begin
    if (S[I] = chr(92)) or (S[I] = ':') then L := ''
    else
    begin
      C := S[I];
      L := L + C
    end;
    I := I + 1
  end
end;

{ Save under a new name; on success the buffer takes that name. }
procedure WriteTo(var Path: string; Existing: boolean);
var L: string;
    Code: integer;
begin
  Code := SaveAs(Path, Existing);
  if Code = 0 then
  begin
    LeafName(Path, L);
    FileName := L;
    EdOnDisk := true;
    EdModified := false;
    { after the Rename box TP shows the file from its start (MINT06 06) }
    if NameKind = NameRename then CursorHome();
    UiMode := ModeEditor
  end
  else if NameKind = NameRename then ShowFileError(Code, ModeEditor)
  else ShowFileError(Code, ModeMenu)
end;

procedure NameAccept;
var L: string;
begin
  Upper(NameText);
  if Length(NameText) = 0 then UiMode := ModeEditor
  else if Wild(NameText) then NameFresh := false
  else if NameKind = NameLoad then
  begin
    if EdModified then
    begin
      PendingName := NameText;
      VerifyKind := VerifyLoad;
      UiMode := ModeVerify
    end
    else DoLoad(NameText)
  end
  else
  begin
    if FileExists(NameText) then
    begin
      PendingName := NameText;
      UiMode := ModeOverwrite
    end
    else if BufErr <> 0 then
    begin
      if NameKind = NameRename then ShowFileError(BufErr, ModeEditor)
      else ShowFileError(BufErr, ModeMenu)
    end
    else WriteTo(NameText, false)
  end
end;

procedure SaveCommand;
begin
  if FileName = 'NONAME.PAS' then OpenName(NameRename)
  else
    DoSave()
end;

procedure VerifyDone(Saved: boolean);
begin
  if Saved then DoSave();
  if UiMode <> ModeError then
  begin
    if VerifyKind = VerifyNew then DoNew()
    else if VerifyKind = VerifyLoad then DoLoad(PendingName)
    else UiMode := ModeQuit
  end
end;

procedure RunItem;
var I: integer;
begin
  I := ItemSel[MenuSel];
  LastMenu := MenuSel;
  if MenuSel = MenuFile then
  begin
    if I = 1 then OpenName(NameLoad)
    else if I = 2 then
    begin
      if EdModified then begin VerifyKind := VerifyNew; UiMode := ModeVerify end
      else DoNew()
    end
    else if I = 3 then SaveCommand()
    else if I = 4 then OpenName(NameWrite)
    else
    begin
      if EdModified then begin VerifyKind := VerifyQuit; UiMode := ModeVerify end
      else UiMode := ModeQuit
    end
  end
  else UiMode := ModeEditor
end;

{ ---- editing ---- }

procedure CurLineBytes(var S: string; var Start: integer);
var Len: integer;
begin
  PageCover(EdLine, 1);
  if not PageHas(EdLine) then EdLine := PgLine;
  LineSpan(EdLine, Start, Len);
  LineText(EdLine, S)
end;

procedure TypeChar(C: char);
var S: string;
    Start, B, Len: integer;
begin
  CurLineBytes(S, Start);
  Len := DispLen(S);
  if Len < LineMax then
  begin
    while DispLen(S) + 1 < EdCol do
    begin
      PageInsert(Start + Length(S), ' ', EdLine);
      CurLineBytes(S, Start)
    end;
    B := ColByte(S, EdCol);
    if (not EdInsert) and (B <= Length(S)) then PageDelete(Start + B - 1, EdLine);
    CurLineBytes(S, Start);
    PageInsert(Start + B - 1, C, EdLine);
    EdCol := EdCol + 1;
    EdModified := true
  end
end;

procedure NewLine;
var S: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  B := ColByte(S, EdCol);
  PageInsert(Start + B - 1, chr(10), EdLine);
  EdLine := EdLine + 1;
  EdCol := 1;
  EdModified := true
end;

procedure BackSpace;
var S, P: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  if EdCol > DispLen(S) + 1 then EdCol := EdCol - 1
  else if EdCol > 1 then
  begin
    B := ColByte(S, EdCol);
    EdCol := ByteCol(S, B - 1);
    PageDelete(Start + B - 2, EdLine);
    EdModified := true
  end
  else if EdLine > 1 then
  begin
    EdLine := EdLine - 1;
    CurLineBytes(P, Start);
    EdCol := DispLen(P) + 1;
    PageDelete(Start + Length(P), EdLine);
    EdModified := true
  end
end;

procedure DeleteChar;
var S: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  if EdCol <= DispLen(S) then
  begin
    B := ColByte(S, EdCol);
    PageDelete(Start + B - 1, EdLine);
    EdModified := true
  end
  else if EdLine < DocLines then
  begin
    while DispLen(S) + 1 < EdCol do
    begin
      PageInsert(Start + Length(S), ' ', EdLine);
      CurLineBytes(S, Start)
    end;
    PageDelete(Start + Length(S), EdLine);
    EdModified := true
  end
end;

procedure DeleteLine;
var S: string;
    Start, I: integer;
begin
  CurLineBytes(S, Start);
  I := Length(S);
  if EdLine < DocLines then I := I + 1;
  while I > 0 do
  begin
    PageDelete(Start, EdLine);
    I := I - 1
  end;
  EdCol := 1;
  EdModified := true
end;

procedure EditKey(K: integer);
var S: string;
    Start: integer;
begin
  if (K >= 32) and (K < 256) then TypeChar(chr(K))
  else if K = KeyEnter then NewLine()
  else if K = KeyBack then BackSpace()
  else if K = KeyDel then DeleteChar()
  else if K = KeyCtrlY then DeleteLine()
  else if K = KeyIns then EdInsert := not EdInsert
  else if K = KeyLeft then begin if EdCol > 1 then EdCol := EdCol - 1 end
  else if K = KeyRight then begin if EdCol < LineMax then EdCol := EdCol + 1 end
  else if K = KeyUp then begin if EdLine > 1 then EdLine := EdLine - 1 end
  else if K = KeyDown then begin if EdLine < DocLines then EdLine := EdLine + 1 end
  else if K = KeyHome then EdCol := 1
  else if K = KeyEnd then
  begin
    CurLineBytes(S, Start);
    EdCol := DispLen(S) + 1
  end
  else if K = KeyPgDn then
  begin
    { MINT06 27, 28: 21 lines, the cursor keeps its view row }
    if EdTop + ViewRows - 1 <= DocLines then
    begin
      EdTop := EdTop + ViewRows - 1;
      EdLine := EdLine + ViewRows - 1;
      if EdLine > DocLines then EdLine := DocLines
    end
    else EdLine := DocLines
  end
  else if K = KeyPgUp then
  begin
    EdTop := EdTop - ViewRows + 1;
    EdLine := EdLine - ViewRows + 1;
    if EdTop < 1 then EdTop := 1;
    if EdLine < 1 then EdLine := 1
  end
  else if K = KeyCtrlHome then EdLine := EdTop
  else if K = KeyCtrlEnd then
  begin
    EdLine := EdTop + ViewRows - 1;
    if EdLine > DocLines then EdLine := DocLines
  end
  else if K = KeyCtrlPgUp then begin EdLine := 1; EdTop := 1 end
  else if K = KeyCtrlPgDn then begin EdLine := DocLines; JumpTo() end;
  ScrollTo()
end;

{ ---- key dispatch ---- }

procedure OpenMenu(Menu: integer);
begin
  MenuSel := Menu;
  LastMenu := Menu;
  if Menu = MenuEdit then UiMode := ModeEditor else UiMode := ModeMenu
end;

function HotMenu(K: integer): integer;
var M: integer;
begin
  M := 0;
  if (K = ord('f')) or (K = ord('F')) or (K = KeyAltF) then M := MenuFile
  else if (K = ord('e')) or (K = ord('E')) or (K = KeyAltE) then M := MenuEdit
  else if (K = ord('r')) or (K = ord('R')) or (K = KeyAltR) then M := MenuRun
  else if (K = ord('c')) or (K = ord('C')) or (K = KeyAltC) then M := MenuCompile;
  HotMenu := M
end;

function HotItem(K: integer): integer;
var I, Found: integer;
    S: string;
begin
  Found := 0; I := 1;
  if (K >= ord('a')) and (K <= ord('z')) then K := K - 32;
  while I <= ItemCount(MenuSel) do
  begin
    ItemText(MenuSel, I, S);
    if ord(S[2]) = K then Found := I;
    I := I + 1
  end;
  HotItem := Found
end;

procedure GlobalKey(K: integer);
begin
  if (K = KeyAltF) or (K = KeyAltR) or (K = KeyAltC) then OpenMenu(HotMenu(K))
  else if K = KeyAltE then UiMode := ModeEditor
  else if K = KeyF10 then begin MenuSel := LastMenu; UiMode := ModeBar end
  else if K = KeyF2 then SaveCommand()
  else if K = KeyF3 then
  begin
    MenuSel := MenuFile; ItemSel[MenuFile] := 1; LastMenu := MenuFile;
    OpenName(NameLoad)
  end
  else if K = KeyAltX then
  begin
    if EdModified then begin VerifyKind := VerifyQuit; UiMode := ModeVerify end
    else UiMode := ModeQuit
  end
end;

function IsGlobal(K: integer): boolean;
begin
  IsGlobal := (K = KeyAltF) or (K = KeyAltR) or (K = KeyAltC) or (K = KeyAltE) or
              (K = KeyF10) or (K = KeyF2) or (K = KeyF3) or (K = KeyAltX)
end;

{ Show a compiler error (MINT06 07, 34): the message replaces the info
  row and the cursor goes to the token; Col is the source byte column,
  shown through the tabs. A line off screen is placed on view row 11. }
procedure IdeError(Line, Col: integer; var Msg: string);
var S: string;
begin
  if Line > DocLines then Line := DocLines;
  if Line < 1 then Line := 1;
  EdLine := Line;
  PageCover(EdLine, 1);
  LineText(EdLine, S);
  EdCol := ByteCol(S, Col);
  if (EdLine < EdTop) or (EdLine > EdTop + ViewRows - 1) then JumpTo()
  else ScrollTo();
  ErrLineText := Msg;
  ErrLineShown := true;
  UiMode := ModeEditor
end;

procedure IdeKey(K: integer);
var M, I: integer;
    C: string;
begin
  if (UiMode = ModeEditor) and ErrLineShown then
  begin
    { the next key restores the info row (MINT06 08); Esc does no more }
    ErrLineShown := false;
    if K <> KeyEsc then
    begin
      if IsGlobal(K) then GlobalKey(K) else EditKey(K)
    end
  end
  else if UiMode = ModeEditor then
  begin
    if IsGlobal(K) then GlobalKey(K) else EditKey(K)
  end
  else if UiMode = ModeError then
  begin
    if K = KeyEsc then UiMode := ErrReturn
  end
  else if UiMode = ModeOverwrite then
  begin
    if (K = ord('y')) or (K = ord('Y')) then WriteTo(PendingName, true)
    else if (K = ord('n')) or (K = ord('N')) or (K = KeyEsc) then
    begin
      if NameKind = NameRename then UiMode := ModeEditor else UiMode := ModeMenu
    end
  end
  else if UiMode = ModeBar then
  begin
    M := HotMenu(K);
    if M > 0 then OpenMenu(M)
    else if K = KeyLeft then begin MenuSel := (MenuSel + 2) mod 4 + 1; LastMenu := MenuSel end
    else if K = KeyRight then begin MenuSel := MenuSel mod 4 + 1; LastMenu := MenuSel end
    else if (K = KeyEnter) or (K = KeyDown) then OpenMenu(MenuSel)
    else if (K = KeyEsc) or (K = KeyF10) then UiMode := ModeEditor
    else if IsGlobal(K) then GlobalKey(K)
  end
  else if UiMode = ModeMenu then
  begin
    I := HotItem(K);
    if I > 0 then begin ItemSel[MenuSel] := I; RunItem() end
    else if K = KeyDown then ItemSel[MenuSel] := ItemSel[MenuSel] mod ItemCount(MenuSel) + 1
    else if K = KeyUp then
      ItemSel[MenuSel] := (ItemSel[MenuSel] + ItemCount(MenuSel) - 2) mod ItemCount(MenuSel) + 1
    else if K = KeyEnter then RunItem()
    else if K = KeyEsc then UiMode := ModeBar
    else if K = KeyLeft then
    begin
      MenuSel := (MenuSel + 2) mod 4 + 1; LastMenu := MenuSel;
      if MenuSel = MenuEdit then UiMode := ModeBar
    end
    else if K = KeyRight then
    begin
      MenuSel := MenuSel mod 4 + 1; LastMenu := MenuSel;
      if MenuSel = MenuEdit then UiMode := ModeBar
    end
    else if IsGlobal(K) then GlobalKey(K)
  end
  else if UiMode = ModeName then
  begin
    if K = KeyEnter then NameAccept()
    else if K = KeyEsc then
    begin
      if NameKind = NameRename then UiMode := ModeEditor else UiMode := ModeMenu
    end
    else if K = KeyBack then
    begin
      NameFresh := false;
      if Length(NameText) > 0 then
      begin
        C := '';
        I := 1;
        while I < Length(NameText) do
        begin
          C := C + NameText[I];
          I := I + 1
        end;
        NameText := C
      end
    end
    else if (K > 32) and (K < 127) then
    begin
      if NameFresh then NameText := '';
      NameFresh := false;
      if Length(NameText) < 40 then
      begin
        C := chr(K);
        NameText := NameText + C
      end
    end
  end
  else if UiMode = ModeVerify then
  begin
    if (K = ord('y')) or (K = ord('Y')) then VerifyDone(true)
    else if (K = ord('n')) or (K = ord('N')) then VerifyDone(false)
    else if K = KeyEsc then
    begin
      { a cancelled Alt-X leaves the bar on File (MINT06 57) }
      if VerifyKind = VerifyQuit then
      begin
        MenuSel := MenuFile;
        LastMenu := MenuFile;
        UiMode := ModeBar
      end
      else UiMode := ModeMenu
    end
  end
end;

{ The state after start (MINT06 02_editor): an empty NONAME.PAS, the menu
  bar active on File, the cursor hidden. }
procedure IdeStart;
begin
  WorkName0 := 'TURBOWK0.$$$';
  WorkName1 := 'TURBOWK1.$$$';
  EdInsert := true;
  ItemSel[1] := 1; ItemSel[2] := 1; ItemSel[3] := 1; ItemSel[4] := 1;
  DoNew();
  MenuSel := MenuFile;
  LastMenu := MenuFile;
  UiMode := ModeBar
end;
