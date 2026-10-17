program StrToIntEx;
var
  Text: string;
  Value: integer;
  Ok: boolean;

procedure StrToInt(var S: string;
                   var N: integer;
                   var Valid: boolean);
var
  I, Start: integer;
  Negative: boolean;
begin
  N := 0;
  Valid := Length(S) > 0;
  Negative := false;
  Start := 1;
  if Valid then
    if S[1] = '-' then
    begin
      Negative := true;
      Start := 2;
      Valid := Length(S) > 1
    end;
  for I := Start to Length(S) do
    if (S[I] < '0') or (S[I] > '9') then
      Valid := false
    else
      N := N * 10 + Ord(S[I]) - Ord('0');
  if Negative then N := -N
end;

begin
  Text := '1992';
  StrToInt(Text, Value, Ok);
  writeln(Ok, ' ', Value + 1);
  Text := '-45';
  StrToInt(Text, Value, Ok);
  writeln(Ok, ' ', Value);
  Text := '12a';
  StrToInt(Text, Value, Ok);
  writeln(Ok)
end.
