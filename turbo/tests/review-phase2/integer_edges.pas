{$mode delphi}{$B+}{$H-}
program IntegerEdges;
var a,b,c,d,e: integer;
begin
 a:=2147483647; b:=1; a:=a+b; writeln(a);
 c:=-a; d:=a-1; e:=a*2; writeln(c,' ',d,' ',e);
 b:=10; writeln(a div b,' ',a mod b);
 a:=-7; b:=-3; writeln(a div b,' ',a mod b);
 b:=3; writeln(a div b,' ',a mod b);
 a:=7; b:=-3; writeln(a div b,' ',a mod b);
 writeln(ord(chr(2147483647)),' ',ord(chr(-1)))
end.
