{$MODE DELPHI}{$B+}{$H-}
{ Existing public OEM AH=FBh page contract: 4000 cell bytes, row, column. }
program PageRoundtrip;
var P, Q: array[1..4002] of char;
    I: integer;
begin
  ScreenPage(0, P[1]);
  I := 1;
  while I <= 4002 do begin Q[I] := P[I]; I := I + 1 end;
  P[1] := 'X'; P[2] := chr(30); P[4001] := chr(3); P[4002] := chr(7);
  ScreenPage(1, P[1]); ScreenPage(0, P[1]);
  if (P[1] = 'X') and (ord(P[2]) = 30) and
     (ord(P[4001]) = 3) and (ord(P[4002]) = 7) then Writeln('PAGE-OK');
  ScreenPage(1, Q[1])
end.
