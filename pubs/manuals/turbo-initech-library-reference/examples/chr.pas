program ChrEx;
var
  Code: integer;
  Line: string[26];
begin
  Line := '';
  for Code := 65 to 70 do
    Line := Line + Chr(Code);
  writeln(Line);
  writeln(Chr(97), Chr(65 + 256), Chr(48 + 7))
end.
