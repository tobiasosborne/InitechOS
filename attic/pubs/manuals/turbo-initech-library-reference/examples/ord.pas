program OrdEx;
var
  C: char;
  Digit: integer;
begin
  C := '7';
  Digit := Ord(C) - Ord('0');
  writeln(Ord('A'), ' ', Ord(' '), ' ', Digit);
  write(Ord(false), ' ', Ord(true), ' ');
  writeln(Ord(-12))
end.
