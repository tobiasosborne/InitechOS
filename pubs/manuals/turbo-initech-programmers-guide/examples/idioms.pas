program Idioms;
const
  { an enumeration: one constant per value }
  Red = 0;
  Amber = 1;
  Green = 2;
  { a list in an array: 0 is the end mark }
  Nothing = 0;
var
  Light, Head, Here: integer;
  Item: array[1..4] of char;
  Link: array[1..4] of integer;

{ a set membership test as a function }
function IsVowel(C: char): boolean;
begin
  IsVowel := false;
  if C = 'A' then IsVowel := true;
  if C = 'E' then IsVowel := true;
  if C = 'I' then IsVowel := true;
  if C = 'O' then IsVowel := true;
  if C = 'U' then IsVowel := true
end;

begin
  Light := Amber;
  if Light = Amber then writeln('amber');
  writeln(IsVowel('E'), ' ', IsVowel('S'));
  { the list Item[3] -> Item[1] -> Item[4] }
  Item[1] := 'B'; Item[3] := 'A'; Item[4] := 'D';
  Link[3] := 1; Link[1] := 4; Link[4] := Nothing;
  Head := 3;
  Here := Head;
  while Here <> Nothing do
  begin
    write(Item[Here]);
    Here := Link[Here]
  end;
  writeln
end.
