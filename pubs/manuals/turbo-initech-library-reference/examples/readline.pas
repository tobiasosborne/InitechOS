program ReadLineEx;
const
  LF = 10;
var
  Data: file;
  Text, Line: string;
  Count, Number: integer;
  AtEnd: boolean;

{ Data is global: no file parameters }
procedure ReadLine(var S: string;
                   var Eof: boolean);
var
  Buf: string[1];
  Got: integer;
  Done: boolean;
begin
  S := '';
  Buf := ' ';
  Done := false;
  Eof := false;
  while not Done do
  begin
    BlockRead(Data, Buf[1], 1, Got);
    if Got = 0 then
    begin
      Done := true;
      Eof := Length(S) = 0
    end
    else if Ord(Buf[1]) = LF then
      Done := true
    else
      S := S + Buf[1]
  end
end;

begin
  Text := 'first' + Chr(LF) + 'second';
  Text := Text + Chr(LF);
  Assign(Data, 'LINES.TXT');
  Rewrite(Data, 1);
  BlockWrite(Data, Text[1], Length(Text),
             Count);
  Reset(Data, 1);
  Number := 0;
  ReadLine(Line, AtEnd);
  while not AtEnd do
  begin
    Number := Number + 1;
    writeln(Number, ': ', Line);
    ReadLine(Line, AtEnd)
  end
end.
