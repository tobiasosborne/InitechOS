program UpCaseEx;
var
  Name: string;
  I: integer;

function UpCase(C: char): char;
begin
  UpCase := C;
  if C >= 'a' then
    if C <= 'z' then
      UpCase := Chr(Ord(C) - 32)
end;

begin
  Name := 'Initech Systems, room 2504';
  for I := 1 to Length(Name) do
    Name[I] := UpCase(Name[I]);
  writeln(Name)
end.
