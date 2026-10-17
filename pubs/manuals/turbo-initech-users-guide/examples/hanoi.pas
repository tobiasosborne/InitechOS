program Hanoi;
var
  Moves: integer;

procedure Move(N: integer;
               From, Onto, Spare: char);
begin
  if N > 0 then
  begin
    Move(N - 1, From, Spare, Onto);
    Moves := Moves + 1;
    write('disk ', N, ': ');
    writeln(From, ' to ', Onto);
    Move(N - 1, Spare, Onto, From)
  end
end;

begin
  Moves := 0;
  Move(3, 'A', 'C', 'B');
  writeln(Moves, ' moves')
end.
