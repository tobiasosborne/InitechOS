{$mode delphi}{$B+}{$H-}
program StringEdges;
var s,t,u: string; tiny: string[1]; c: char; i: integer;
begin
 s:=''; t:='AB'; c:='Q';
 writeln(s=t, ' ', s<t, ' ', t>s, ' ', s='');
 u := c+'XX'; writeln(u);
 u := (c+'ZZ')+('VV'+c); writeln(u);
 s:='XX'; s[1]:=chr(0); s[2]:=chr(255); t:=s; t[2]:=chr(254);
 writeln(length(s),' ',ord(s[1]),' ',ord(s[2]),' ',s>t,' ',s=t);
 tiny:=s; writeln(length(tiny),' ',ord(tiny[1]));
 s:=''; for i:=1 to 255 do s:=s+'A';
 s:=s+s; writeln(length(s),' ',ord(s[255]));
 s:=s; writeln(length(s));
 u:=''; t:='XX'; t:=t+u; writeln(t)
end.
