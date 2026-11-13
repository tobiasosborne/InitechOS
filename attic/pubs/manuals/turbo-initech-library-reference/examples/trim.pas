program TrimEx;
var
  Text: string;

{ Removes blanks from both ends of S. }
procedure Trim(var S: string);
var
  First, Last, I: integer;
  Done: boolean;
  Kept: string;
begin
  First := 1;
  Done := false;
  while not Done do
    if First > Length(S) then
      Done := true
    else if S[First] = ' ' then
      First := First + 1
    else
      Done := true;
  Last := Length(S);
  Done := false;
  while not Done do
    if Last < First then
      Done := true
    else if S[Last] = ' ' then
      Last := Last - 1
    else
      Done := true;
  Kept := '';
  for I := First to Last do
    Kept := Kept + S[I];
  S := Kept
end;

begin
  Text := '   cover sheet   ';
  Trim(Text);
  writeln('[', Text, '] ', Length(Text));
  Text := '    ';
  Trim(Text);
  writeln('[', Text, '] ', Length(Text))
end.
