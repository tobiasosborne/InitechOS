{$mode delphi}{$B+}{$H-}
program StringIndex;
var s,t: string; a: array[0..2] of integer;
begin
 s := 'AB'; t := 'X'; a[0]:=10; a[1]:=20; a[2]:=30;
 writeln(ord(s[ord(t+'Z'='XZ')+1]));
 writeln(a[ord(t+'Z'='XZ')]);
 s[ord(t+'Z'='XZ')+1] := 'Q';
 writeln(s)
end.
