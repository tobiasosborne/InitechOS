program LengthEx;
var
  Name: string[10];
  Full: string;
begin
  Name := '';
  write(Length(Name), ' ');
  Name := 'Bill Lumbergh';
  write(Length(Name), ' ');
  Full := Name + ', Vice President';
  writeln(Length(Full));
  writeln(Full)
end.
