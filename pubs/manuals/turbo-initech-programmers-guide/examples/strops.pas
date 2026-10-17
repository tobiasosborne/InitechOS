program StrOps;
var
  First, Last, Full: string[20];
  Initial: char;
begin
  First := 'Peter';
  Last := 'Gibbons';
  Initial := Last[1];
  Full := First + ' ' + Last;
  writeln(Full, ' (', length(Full), ')');
  writeln(First + Initial + '.');
  write((First < Last), ' ');
  writeln(('Pete' < First));
  writeln((Initial = 'G'), ' ', ('a' > 'Z'))
end.
