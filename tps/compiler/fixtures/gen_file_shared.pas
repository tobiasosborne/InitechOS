{$MODE DELPHI}{$B+}{$H-}
{ DEC-07 Rung-2 shared fixture for B8 (beads initech-ogxv).
  The observable is stdout; the file content is also proved by writing and
  reading it back in this same program. Record size 1 pins BlockRead and
  BlockWrite counts to bytes under Free Pascal.

  Golden provenance: HAND-COMPUTED from the nine-byte NORTHSTAR payload and
  four requested transfer counts, not FPC-minted.
  Expected exact stdout:
    FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR }
program FileShared;
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
