{$mode delphi}{$B+}{$H-}
program StringRelations;
var s,t: string; c: char; i: integer;
begin
 s:='AB'; t:='ZZ'; c:='A';
 writeln(c<s,' ',c=s,' ',c>t,' ',s>c,' ',s<>c);
 writeln(c<(s+t),' ',c=(s+t),' ',c>(s+t));
 writeln((s+t)>c,' ',(s+t)<c,' ',(s+t)<>c);
 s:=''; c:=chr(255);
 writeln(c>s,' ',s<c,' ',c=s);
 s:=c; t:='XX'; t[1]:=chr(254);
 writeln(s>t,' ',s<t,' ',s=t);
 for i:=0 to 255 do begin
   c:=chr(i); s:=c;
   if (c<>s) or (s<>c) or (c<s) or (s<c) then writeln(i)
 end;
 writeln('done')
end.
