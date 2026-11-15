{ Host-only DOS boundary recorder. The real target gate is checked in smoke. }
unit RunRTL;
{$MODE DELPHI}{$H-}
interface
var RunTrace, RunPath: string;
    RunStatus: integer;
procedure ScreenPage(Op: integer; var First: char);
function Execute(var Path: string): integer;
function FileAttributes(var F: file): integer;
function DosMemory(): integer;
function ReadKey(): char;
implementation
uses HostRTL;
function FileAttributes(var F: file): integer;
begin FileAttributes := HostRTL.FileAttributes(F) end;
function DosMemory(): integer;
begin DosMemory := 500000 end;
function ReadKey(): char;
begin Halt(202); ReadKey := chr(0) end;
procedure ScreenPage(Op: integer; var First: char);
begin
  RunTrace := RunTrace + chr(ord('0') + Op)
end;
function Execute(var Path: string): integer;
begin
  RunTrace := RunTrace + 'E'; RunPath := Path; Execute := RunStatus
end;
end.
