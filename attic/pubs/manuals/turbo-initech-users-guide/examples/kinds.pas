program Kinds;
const
  Limit = 3;
  Title = 'TPS';
type
  Sheet = record
    Pages: integer;
    Filed: boolean;
  end;
var
  Total: integer;
  Stack: array[1..Limit] of Sheet;
  Name: string[8];

procedure Count(var N: integer; Step: integer);
begin
  N := N + Step
end;

function Twice(N: integer): integer;
var
  Work: integer;
begin
  Work := N + N;
  Twice := Work
end;

begin
  Name := Title;
  Total := 0;
  Count(Total, Twice(Limit));
  Stack[1].Pages := Total;
  writeln(Name, ' ', Stack[1].Pages)
end.
