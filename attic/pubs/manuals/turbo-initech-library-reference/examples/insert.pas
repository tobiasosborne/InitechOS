program InsertEx;
var
  Line, Word: string;

{ Inserts Part into S before position At. }
procedure Insert(var Part: string;
                 var S: string; At: integer);
var
  Head, Tail: string;
  I: integer;
begin
  Head := '';
  Tail := '';
  for I := 1 to Length(S) do
    if I < At then
      Head := Head + S[I]
    else
      Tail := Tail + S[I];
  S := Head + Part + Tail
end;

{ Deletes Count characters from position At. }
procedure Delete(var S: string;
                 At, Count: integer);
var
  Kept: string;
  I: integer;
begin
  Kept := '';
  for I := 1 to Length(S) do
    if (I < At) or (I >= At + Count) then
      Kept := Kept + S[I];
  S := Kept
end;

begin
  Line := 'the report';
  Word := 'TPS ';
  Insert(Word, Line, 5);
  writeln(Line);
  Delete(Line, 1, 4);
  writeln(Line)
end.
