program SymTab;
{ An insertion-ordered symbol table: rows are
  appended; a lookup searches from the newest
  row back, so a local hides a global. }
const
  Rows = 16;
var
  Name: array[1..Rows] of char;
  Level: array[1..Rows] of integer;
  Count: integer;

procedure Declare(C: char; L: integer);
begin
  Count := Count + 1;
  Name[Count] := C;
  Level[Count] := L
end;

function Find(C: char): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := Count;
  while I >= 1 do
  begin
    if Found = 0 then
      if Name[I] = C then Found := I;
    I := I - 1
  end;
  Find := Found
end;

begin
  Count := 0;
  Declare('a', 0);
  Declare('b', 0);
  Declare('a', 1);
  writeln('a is row ', Find('a'), ' level ',
          Level[Find('a')]);
  writeln('b is row ', Find('b'));
  Count := 2;
  writeln('after the routine: a is row ',
          Find('a'))
end.
