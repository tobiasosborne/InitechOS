program Peek;
{ A sentinel accessor: Peek returns Chr(0)
  past the end, so the scanner needs no
  bounds test of its own. }
var
  Text: string;
  At, Sum, N: integer;

function Peek(): char;
begin
  Peek := Chr(0);
  if At <= Length(Text) then
    Peek := Text[At]
end;

function IsDigit(C: char): boolean;
begin
  IsDigit := false;
  if C >= '0' then
    if C <= '9' then IsDigit := true
end;

begin
  Text := 'pages 12, 7 and 30';
  At := 1;
  Sum := 0;
  while Peek() <> Chr(0) do
    if IsDigit(Peek()) then
    begin
      N := 0;
      while IsDigit(Peek()) do
      begin
        N := N * 10 + Ord(Peek()) - Ord('0');
        At := At + 1
      end;
      Sum := Sum + N
    end
    else
      At := At + 1;
  writeln('sum of numbers: ', Sum)
end.
