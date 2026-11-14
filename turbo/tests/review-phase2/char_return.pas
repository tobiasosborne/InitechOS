{$mode delphi}{$B+}{$H-}
program CharReturn;
type Pair=record c: char; i: integer; end;
var s: string; r: Pair;
function Letter(n: integer): char;
begin Letter:=chr(n) end;
procedure Change(var c: char; var i: integer);
begin c:=Letter(255); i:=i+1 end;
begin s:='ABC'; r.c:='X'; r.i:=123; Change(s[2],r.i);
 writeln(ord(s[1]),' ',ord(s[2]),' ',ord(s[3]),' ',r.i);
 Change(r.c,r.i); writeln(ord(r.c),' ',r.i,' ',ord(Letter(128)))
end.
