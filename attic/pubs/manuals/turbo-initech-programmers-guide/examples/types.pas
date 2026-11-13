program Types;
type
  Employee = record
    Id, Grade: integer;
    Active: boolean;
    Initial: char;
  end;
var
  Big: integer;
  Code: char;
  Short: string[4];
  Wide: string;
  Temp: array[-2..2] of integer;
  Staff: array[1..2] of Employee;
begin
  Big := 2147483647;
  Code := 'A';
  Short := 'Initech';
  Wide := Short + ' Systems';
  Temp[-2] := -40;
  Staff[1].Id := 2504;
  Staff[1].Active := true;
  Staff[1].Initial := Code;
  Staff[2] := Staff[1];
  Staff[2].Grade := 9;
  writeln(Big, ' ', ord(Code), ' ', Short);
  writeln(Wide, ' ', length(Wide));
  write(Temp[-2], ' ', Staff[2].Id);
  writeln(Staff[2].Initial, Staff[2].Grade, ' ',
          Staff[2].Active)
end.
