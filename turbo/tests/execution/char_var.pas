program probe;
var s: string;
procedure Change(var c: char);
begin c := 'Z' end;
begin s := 'ABCD'; Change(s[1]);
  writeln(ord(s[1]), ' ', ord(s[2]), ' ', ord(s[3]), ' ', ord(s[4]))
end.
