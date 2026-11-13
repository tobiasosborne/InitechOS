program Assign;
type
  Pair = record
    Left, Right: integer;
  end;
var
  N: integer;
  S: string[10];
  C: char;
  P, Q: Pair;
begin
  N := 6 * 7;
  C := 'z';
  S := C;
  S := S + 'ebra';
  S[1] := 'Z';
  P.Left := N;
  P.Right := -N;
  Q := P;
  writeln(N, ' ', S, ' ', Q.Left, ' ', Q.Right)
end.
