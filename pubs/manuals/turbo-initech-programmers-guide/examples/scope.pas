program Scope;
var
  Count: integer;
  Mark: char;

procedure Show();
var
  Count: char;          { hides the global Count }
begin
  Count := 'L';
  Mark := '*';          { the global Mark }
  writeln('in Show: ', Count, Mark)
end;

begin
  Count := 7;
  Mark := ' ';
  Show();
  writeln('in program: ', Count, Mark)
end.
