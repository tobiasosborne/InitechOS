program BlockRdEx;
const
  Size = 16;
var
  Data: file;
  Buffer: string[Size];
  Got, Total: integer;
  Done: boolean;
begin
  Buffer := 'Yeah, if you could';
  Assign(Data, 'MEMO.DAT');
  Rewrite(Data, 1);
  BlockWrite(Data, Buffer[1], Length(Buffer),
             Got);
  Reset(Data, 1);
  Total := 0;
  Done := false;
  while not Done do
  begin
    BlockRead(Data, Buffer[1], 5, Got);
    Total := Total + Got;
    write(Got, ' ');
    if Got < 5 then Done := true
  end;
  writeln('total ', Total)
end.
