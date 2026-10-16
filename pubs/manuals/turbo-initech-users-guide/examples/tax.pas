program Tax;
var
  Gross, Net: integer;

function Withholding(Cents: integer): integer;
begin
  if Cents > 50000 then
    Withholding := Cents div 4
  else
    Withholding := Cents div 5
end;

procedure Report(Tag: char; Cents: integer);
begin
  write(Tag, ': ', Cents div 100, ' dollars ');
  writeln(Cents mod 100, ' cents')
end;

begin
  Gross := 57500;
  Net := Gross - Withholding(Gross);
  Report('G', Gross);
  Report('N', Net)
end.
