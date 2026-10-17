program IntToStrEx;
var
  Text: string;

procedure IntToStr(N: integer; var S: string);
var
  Digit: char;
  Negative: boolean;
begin
  S := '';
  Negative := N < 0;
  if Negative then N := -N;
  repeat
    Digit := Chr(Ord('0') + N mod 10);
    S := Digit + S;
    N := N div 10
  until N = 0;
  if Negative then S := '-' + S
end;

begin
  IntToStr(2504, Text);
  writeln('[', Text, '] ', Length(Text));
  IntToStr(-570, Text);
  writeln('[', Text, ']');
  IntToStr(0, Text);
  writeln('[', Text, ']')
end.
