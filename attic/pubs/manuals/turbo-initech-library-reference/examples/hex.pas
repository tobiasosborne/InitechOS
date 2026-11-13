program HexEx;
var
  Text: string;

{ Writes N, not negative, in hexadecimal. }
procedure IntToHex(N: integer; var S: string);
var
  D: integer;
begin
  S := '';
  repeat
    D := N mod 16;
    if D < 10 then
      S := Chr(Ord('0') + D) + S
    else
      S := Chr(Ord('A') + D - 10) + S;
    N := N div 16
  until N = 0
end;

begin
  IntToHex(262400, Text);
  writeln(Text, 'h');
  IntToHex(255, Text);
  writeln(Text, 'h');
  IntToHex(0, Text);
  writeln(Text, 'h')
end.
