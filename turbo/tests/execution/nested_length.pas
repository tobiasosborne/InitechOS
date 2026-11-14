{$mode delphi}{$B+}{$H-}
program NestedLength;
var s: string;
function Check(n: integer): integer;
var u: string; sentinel: integer;
begin
 sentinel:=777;
 u:=chr(length(s+'cd'))+'ZZ';
 writeln(sentinel);
 Check:=ord(u[1])
end;
begin s:='ab'; writeln(Check(0)); writeln('done') end.
