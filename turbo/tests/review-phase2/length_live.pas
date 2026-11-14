{$mode delphi}{$B+}{$H-}
program LengthLive;
var s: string;
function Count(n: integer): integer;
var sentinel: integer;
begin
 sentinel:=777;
 Count:=length(s+'cd');
 writeln(sentinel)
end;
begin s:='ab'; writeln(Count(0)); writeln('done') end.
