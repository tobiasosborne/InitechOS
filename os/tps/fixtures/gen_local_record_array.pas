{$MODE DELPHI}{$B+}{$H-}
{ B9.5 focused execution fixture for the B6 frame-local array-of-record hole
  reported by B9.4. Exercise owns Cells[-1..2] on its activation frame, writes
  and reads nested fields, performs a whole-record copy between local array
  elements, mutates the copy, and scans a boolean field.

  Golden provenance: HAND-COMPUTED from Pascal record/array semantics, not
  FPC-minted. Before the copy the values are 7,10,13,16. Cells[0] receives a
  copy of Cells[-1] and is then increased by 20, giving 7,27,13,16 and total
  63. The source remains 7. Only Cells[2].Flag is true, so Flags=1.
  Expected exact stdout:
    LOCALREC=63 COPY=27 SOURCE=7 FLAGS=1
}
program LocalRecordArray;
type
  Cell = record
    Value: integer;
    Flag: boolean;
  end;
var
  CopyValue, SourceValue, FlagCount: integer;

function Exercise(Base: integer): integer;
var
  Cells: array[-1..2] of Cell;
  I, Total, Flags: integer;
begin
  for I := -1 to 2 do
  begin
    Cells[I].Value := Base + I * 3;
    Cells[I].Flag := (I mod 2) = 0
  end;
  Cells[0] := Cells[-1];
  Cells[0].Value := Cells[0].Value + 20;
  Total := 0;
  Flags := 0;
  for I := -1 to 2 do
  begin
    Total := Total + Cells[I].Value;
    if Cells[I].Flag then Flags := Flags + 1
  end;
  CopyValue := Cells[0].Value;
  SourceValue := Cells[-1].Value;
  FlagCount := Flags;
  Exercise := Total
end;

begin
  write('LOCALREC='); write(Exercise(10));
  write(' COPY='); write(CopyValue);
  write(' SOURCE='); write(SourceValue);
  write(' FLAGS='); writeln(FlagCount)
end.
