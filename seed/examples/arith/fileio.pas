{$MODE DELPHI}{$B+}{$H-}
{ B8 exact-output oracle: thin byte/block file I/O only.
  Ref: ADR-0007 DEC-02/DEC-05/DEC-07; beads initech-ogxv.

  Both untyped files use record size 1, so BlockRead/BlockWrite counts are
  byte counts. TARGET is opened before SINK on purpose: the wrong-handle
  Rule-6 mutant writes to the next live JFT handle (SINK), producing wrong
  counts/content without a crash. The first one-byte operation and the
  remaining eight-byte operation exercise byte and block transfers through
  the same DOS AH=3Fh/40h contract.

  Hand-computed stdout:
    FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR }
program FileIO;
var
  TargetFile, SinkFile: file;
  Payload, Echoed: string[9];
  WroteOne, WroteBlock, ReadOne, ReadBlock: integer;

procedure ExerciseLocalFile;
var
  LocalFile: file;
begin
  Assign(LocalFile, 'B8LOCAL.BIN');
  Rewrite(LocalFile, 1)
end;

begin
  Payload := 'NORTHSTAR';
  Echoed := '?????????';

  Assign(TargetFile, 'B8FILE.BIN');
  Assign(SinkFile, 'B8SINK.BIN');
  Rewrite(TargetFile, 1);
  Rewrite(SinkFile, 1);

  BlockWrite(TargetFile, Payload[1], 1, WroteOne);
  BlockWrite(TargetFile, Payload[2], Length(Payload) - 1, WroteBlock);

  Reset(TargetFile, 1);
  BlockRead(TargetFile, Echoed[1], 1, ReadOne);
  BlockRead(TargetFile, Echoed[2], Length(Echoed) - 1, ReadBlock);

  ExerciseLocalFile();

  Writeln('FILEIO W1=', WroteOne,
          ' WB=', WroteBlock,
          ' R1=', ReadOne,
          ' RB=', ReadBlock,
          ' DATA=', Echoed)
end.
