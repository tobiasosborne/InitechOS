{ FPC platform adapter for the independent host bootstrap. Not shipped or
  read by Turbo Initech. CLI compilation never uses screen/EXEC services. }
unit HostRTL;
{$MODE DELPHI}{$H-}
interface
function FileAttributes(var F: file): integer;
procedure ScreenPage(Op: integer; var First: char);
function Execute(var Path: string): integer;
function ReadKey(): char;
function DosMemory(): integer;
implementation
uses Dos;
function FileAttributes(var F: file): integer;
var A: word;
begin
  GetFAttr(F, A);
  if DosError = 0 then FileAttributes := A
  else if DosError = 3 then FileAttributes := -2
  else FileAttributes := -DosError
end;
procedure ScreenPage(Op: integer; var First: char);
begin
  { The host bootstrap has no InitechDOS display. A host IDE request fails
    explicitly; target screen journeys use the actual published gate. }
  Writeln('Host bootstrap has no InitechDOS screen page.'); Halt(201)
end;
function Execute(var Path: string): integer;
begin
  Dos.Exec(Path, '');
  if DosError <> 0 then Execute := -DosError else Execute := DosExitCode
end;
function DosMemory(): integer;
begin
  DosMemory := GetHeapStatus().TotalFree
end;
function ReadKey(): char;
begin
  Writeln('Host bootstrap has no InitechDOS keyboard.'); Halt(201); ReadKey := chr(0)
end;
initialization
  FileMode := 0
end.
