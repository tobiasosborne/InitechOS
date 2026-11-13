program Chunks;
{ Reading a file a block at a time, as the
  compiler reads its source. }
const
  ChunkMax = 16;
  LF = 10;
var
  Data: file;
  Text, Chunk: string;
  Wrote, Got, I, Lines, Total: integer;
begin
  Text := 'one' + Chr(LF) + 'two' + Chr(LF);
  Text := Text + 'three' + Chr(LF);
  Text := Text + 'and a longer fourth';
  Text := Text + Chr(LF);
  Assign(Data, 'CHUNKS.TXT');
  Rewrite(Data, 1);
  BlockWrite(Data, Text[1], Length(Text),
             Wrote);
  Reset(Data, 1);
  Chunk := '';
  while Length(Chunk) < ChunkMax do
    Chunk := Chunk + ' ';
  Lines := 0;
  Total := 0;
  Got := ChunkMax;
  while Got = ChunkMax do
  begin
    BlockRead(Data, Chunk[1], ChunkMax, Got);
    Total := Total + Got;
    for I := 1 to Got do
      if Ord(Chunk[I]) = LF then
        Lines := Lines + 1
  end;
  writeln(Total, ' bytes, ', Lines, ' lines')
end.
