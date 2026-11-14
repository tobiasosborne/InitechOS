{$mode delphi}{$B+}{$H-}
program RecursionAlias;
var x: integer; s: string;
function Recurse(n: integer; var v: integer): integer;
var local: integer; text: string;
begin
 text:='abc'; local:=n;
 if n>0 then begin
   v:=v+1; local:=local+Recurse(n-1,v);
 end;
 if text<>'abc' then local:=-999;
 Recurse:=local
end;
procedure Alias(var a,b: integer);
begin a:=a+1; b:=b+a end;
begin
 x:=0; writeln(Recurse(150,x),' ',x);
 x:=4; Alias(x,x); writeln(x)
end.
