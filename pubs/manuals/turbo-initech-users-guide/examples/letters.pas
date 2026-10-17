program Letters;
var
  Count: array[0..25] of integer;
  Text: string;
  I, Code: integer;
  C: char;
begin
  Text := 'TPS REPORTS NEED COVER SHEETS';
  for I := 0 to 25 do Count[I] := 0;
  for I := 1 to Length(Text) do
  begin
    Code := Ord(Text[I]) - Ord('A');
    if Code >= 0 then
      if Code <= 25 then
        Count[Code] := Count[Code] + 1
  end;
  for I := 0 to 25 do
    if Count[I] > 2 then
    begin
      C := Chr(I + Ord('A'));
      writeln(C, ': ', Count[I])
    end
end.
