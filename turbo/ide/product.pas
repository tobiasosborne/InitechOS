{ Product integration: compiler + editor, one TURBO.COM. Compile commands
  use the actual compiler record, not a diagnostic-file parser. Geometry
  is TP5.5 MINT06 07/08/13c/34/35. All services use public DOS calls. }
var
  DisplayPage, UserPage: array[1..4002] of char;
  BuildRequest: integer;
  BuildPending, ProgramReady, KeepRecovery: boolean;
  BuildBase, ReadyName, BuildInput: string;

procedure ProductPaint;
var I, Code: integer;
begin
  if UiMode = ModeUser then ScreenPage(1, UserPage[1])
  else
  begin
    DrawAll(); I := 0;
    while I < 2000 do
    begin
      DisplayPage[I * 2 + 1] := ScrCh[I];
      DisplayPage[I * 2 + 2] := chr(ScrAt[I]); I := I + 1
    end;
    if ScrCurOn then
    begin
      DisplayPage[4001] := chr(ScrCurRow); DisplayPage[4002] := chr(ScrCurCol)
    end
    else begin DisplayPage[4001] := chr(25); DisplayPage[4002] := chr(0) end;
    ScreenPage(2, DisplayPage[1])
  end;
  Code := IOResult();
  if Code <> 0 then Halt(1)
end;

procedure RemoveOwned(var Path: string);
var F: file;
    Code: integer;
begin
  Assign(F, Path); Erase(F); Code := IOResult();
  if (Code <> 0) and (Code <> 2) then KeepRecovery := true
end;

procedure CleanBuild;
var Path: string;
begin
  if (Length(BuildBase) > 0) and (not KeepRecovery) then
  begin
    Path := BuildBase + '.S'; RemoveOwned(Path);
    Path := BuildBase + '.COM'; RemoveOwned(Path);
    Path := BuildBase + '.EXE'; RemoveOwned(Path);
    Path := BuildBase + '.CBK'; RemoveOwned(Path);
    Path := BuildBase + '.EBK'; RemoveOwned(Path)
  end
end;

{ Output siblings share the selected file's directory: DOS AH=56h is a
  same-directory rename. Choose names absent before the first create. }
function NewBuildNames(): boolean;
var Prefix, S, Ext, Path, Target: string;
    I, Last, J: integer;
    Free: boolean;
begin
  Prefix := ''; Last := 0; I := 1;
  while I <= Length(FilePath) do
  begin
    if (FilePath[I] = chr(92)) or (FilePath[I] = ':') then Last := I;
    I := I + 1
  end;
  I := 1;
  while I <= Last do begin S := FilePath[I]; Prefix := Prefix + S; I := I + 1 end;
  J := 0; Free := false;
  while (J < 1000) and (not Free) do
  begin
    NumText(J, S); while Length(S) < 3 do S := '0' + S;
    BuildBase := Prefix + 'COMPI' + S;
    Ext := '.S'; Free := FreeSibling(BuildBase, Ext);
    Ext := '.COM'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.EXE'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.CBK'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.EBK'; Free := Free and FreeSibling(BuildBase, Ext);
    Target := FilePath; if Length(Target) = 0 then Target := FileName;
    Ext := '.EXE'; ExtName(Target, Ext, Path);
    S := BuildBase + Ext;
    if SameName(Path, S) then Free := false;
    J := J + 1
  end;
  OutputAsmName := BuildBase + '.S'; OutputComName := BuildBase + '.COM';
  OutputExeName := BuildBase + '.EXE';
  NewBuildNames := Free
end;

function PublishProgram(var From, Into, Backup: string): integer;
var F: file;
    Attr, Code, Rollback: integer;
    Moved: boolean;
begin
  Code := 0; Moved := false;
  if SameName(Into, FilePath) then Code := 5;
  if Code = 0 then
  begin
    Assign(F, Into); Attr := FileAttributes(F);
    if Attr >= 0 then
    begin
      if (Attr mod 2 = 1) or (Attr div 16 mod 2 = 1) then Code := 5
      else
      begin
        Rename(F, Backup); Code := IOResult(); Moved := Code = 0
      end
    end
    else if Attr <> -2 then Code := -Attr
  end;
  if Code = 0 then
  begin
    Assign(F, From); Rename(F, Into); Code := IOResult();
    if (Code <> 0) and Moved then
    begin
      Assign(F, Backup); Rename(F, Into); Rollback := IOResult();
      if Rollback <> 0 then KeepRecovery := true
    end
  end;
  if (Code = 0) and Moved then RemoveOwned(Backup);
  PublishProgram := Code
end;

procedure CompilerMessage(var Msg: string);
var N: string;
begin
  if (FirstKind = 2) and (FirstExpected = TkSemi) then
    Msg := 'Error 85: ";" expected'
  else if (FirstKind = 1) and (FirstDetail = ErrSourceFull) then
    Msg := 'Source exceeds compiler capacity.'
  else
  begin
    NumText(FirstDetail, N); Msg := 'Compiler error ' + N + '.'
  end
end;

procedure FinishBuild;
var Target, Backup, Ext, Msg: string;
    Code, Attr: integer;
    F: file;
begin
  BuildPending := false; ProgramReady := false;
  CleanBuild();
  BuildInput := PrivateBase + '.PAS';
  BufSave(BuildInput); Code := BufErr;
  if Code = 0 then
  begin
    if not NewBuildNames() then Code := 5
  end;
  if Code = 0 then
  begin
    InputName := BuildInput; DumpMode := false; QuietCompile := true;
    CompileSource(); CompileLines := DocLines;
    if FirstKind = 5 then Code := FirstDetail
    else if FirstKind = 4 then Code := 101
    else if FirstKind <> 0 then
    begin
      CompilerMessage(Msg); IdeError(FirstLine, FirstCol, Msg)
    end
    else
    begin
      Target := FilePath; if Length(Target) = 0 then Target := FileName;
      Ext := '.EXE'; ExtName(Target, Ext, ReadyName);
      Backup := BuildBase + '.EBK';
      Code := PublishProgram(OutputExeName, ReadyName, Backup);
      if Code = 0 then
      begin
        Assign(F, OutputComName); Attr := FileAttributes(F);
        if Attr >= 0 then
        begin
          Ext := '.COM'; ExtName(Target, Ext, ReadyName);
          Backup := BuildBase + '.CBK';
          Code := PublishProgram(OutputComName, ReadyName, Backup);
          ProgramReady := Code = 0
        end
      end;
      if Code = 0 then begin CompileDone := true; UiMode := ModeCompile end
    end
  end;
  if Code <> 0 then ShowFileError(Code, ModeEditor)
end;

procedure ProductKey(K: integer);
begin
  IdeKey(K);
  if RequestedCommand <> 0 then
  begin
    if RequestedCommand = CmdUser then UiMode := ModeUser
    else
    begin
      BuildRequest := RequestedCommand; BuildPending := true; ProgramReady := false;
      CompileDone := false; CompileLines := 0; CompileMemory := DosMemory();
      UiMode := ModeCompile
    end;
    RequestedCommand := 0
  end
end;

procedure ProductCleanup;
var Path: string;
begin
  CleanBuild();
  Path := PrivateBase + '.PAS'; RemoveOwned(Path);
  RemoveOwned(WorkName0); RemoveOwned(WorkName1)
end;

procedure ProductMain;
var K: integer;
    C: char;
begin
  BuildBase := ''; KeepRecovery := false; BuildPending := false;
  ProgramReady := false;
  ScreenPage(0, UserPage[1]); K := IOResult(); if K <> 0 then Halt(1);
  FileDrive := 'A'; FileDir := '.';
  IdeStart();
  while UiMode <> ModeQuit do
  begin
    ProductPaint();
    if BuildPending then FinishBuild()
    else
    begin
      C := ReadKey(); K := ord(C);
      if K = 0 then begin C := ReadKey(); K := 256 + ord(C) end;
      ProductKey(K)
    end
  end;
  ProductCleanup(); ScreenPage(1, UserPage[1])
end;
