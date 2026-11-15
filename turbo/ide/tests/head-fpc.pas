{$MODE DELPHI}{$B+}{$H-}
program IdeHost;
uses Dos;
{$I-}
{ Host oracle adapter to the published DOS AH=43h attribute query. }
function FileAttributes(var F: file): integer;
var A: word;
begin
  GetFAttr(F, A);
  if DosError = 0 then FileAttributes := A else if DosError = 3 then FileAttributes := -2 else FileAttributes := -DosError
end;
