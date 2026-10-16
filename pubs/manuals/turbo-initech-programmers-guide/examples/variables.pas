program Variables;
type
  Point = record
    X, Y: integer;
  end;
var
  Fresh: integer;
  Empty: string;
  Flag: boolean;
  Word: string[8];
  Path: array[0..3] of Point;
  I: integer;
begin
  writeln(Fresh, ' [', Empty, '] ', Flag);
  Word := 'LUMBERGH';
  Word[1] := 'l';
  write(Word, ' ', Word[8], ' ');
  writeln(ord(Word[0]));
  for I := 0 to 3 do
  begin
    Path[I].X := I * 10;
    Path[I].Y := Path[I].X + 1
  end;
  write(Path[2].X, ',', Path[2].Y);
  writeln(' ', Path[3].Y)
end.
