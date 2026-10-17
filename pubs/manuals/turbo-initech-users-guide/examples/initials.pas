program Initials;
var
  Name, Result: string;
  I: integer;
  NewWord: boolean;
begin
  Name := 'Michael Bolton';
  Result := '';
  NewWord := true;
  for I := 1 to Length(Name) do
  begin
    if Name[I] = ' ' then
      NewWord := true
    else if NewWord then
    begin
      Result := Result + Name[I] + '.';
      NewWord := false
    end
  end;
  writeln(Name, ' -> ', Result)
end.
