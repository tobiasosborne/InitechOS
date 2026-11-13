program CopyEx;
var
  Line, Part: string;

procedure Copy(var S: string;
               Index, Count: integer;
               var Result: string);
var
  I: integer;
begin
  Result := '';
  I := Index;
  while I < Index + Count do
  begin
    if I >= 1 then
      if I <= Length(S) then
        Result := Result + S[I];
    I := I + 1
  end
end;

function Pos(C: char; var S: string): integer;
var
  I: integer;
  Found: boolean;
begin
  Pos := 0;
  Found := false;
  I := 1;
  while not Found do
    if I > Length(S) then
      Found := true
    else if S[I] = C then
    begin
      Pos := I;
      Found := true
    end
    else
      I := I + 1
end;

begin
  Line := 'Gibbons, Peter';
  Copy(Line, 1, Pos(',', Line) - 1, Part);
  writeln('[', Part, ']');
  Copy(Line, Pos(',', Line) + 2, 99, Part);
  writeln('[', Part, ']');
  writeln(Pos('z', Line))
end.
