{ FPC -Mtp reference: initech-4bjxw and initech-rnn2n.
  Counters after loops are undefined; observe only body outputs/counts.
  The initial expression runs before the final expression, both before
  storing the counter. Recursive bounds must be activation-local. }
program ForOrder;
var i,total,calls,order: integer;
function Initial(dummy: integer): integer;
begin order := order*10+1; Initial := 1 end;
function Bound(dummy: integer): integer;
begin order := order*10+2; calls := calls+1; Bound := i end;
function Recursive(n: integer): integer;
var j,s: integer;
begin
 s := 0;
 if n > 0 then
   for j := 1 to n do s := s + Recursive(n-1)
 else s := 1;
 Recursive := s
end;
begin
 i := 1; total := 0;
 for i := 3 downto i do total := total+1;
 writeln(total);
 i := 3; total := 0; calls := 0; order := 0;
 for i := Initial(0) to Bound(0) do total := total+1;
 writeln(total,' ',calls,' ',order);
 total := 0;
 for i := 2 to 1 do total := total+1;
 for i := 1 downto 2 do total := total+1;
 writeln(total,' ',Recursive(4))
end.
