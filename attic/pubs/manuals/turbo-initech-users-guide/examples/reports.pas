program Reports;
{ Checks a batch of reports for cover sheets
  and keeps a count file. }
const
  MaxReports = 5;
  Missing = 0;

type
  Report = record
    Number, Pages: integer;
    HasCover: boolean;
  end;

var
  Batch: array[1..MaxReports] of Report;
  Count: integer;

procedure Add(Number, Pages: integer;
              Cover: boolean);
begin
  Count := Count + 1;
  Batch[Count].Number := Number;
  Batch[Count].Pages := Pages;
  Batch[Count].HasCover := Cover
end;

function FirstMissing(): integer;
var
  I, Found: integer;
begin
  Found := Missing;
  I := Count;
  while I >= 1 do
  begin
    if not Batch[I].HasCover then Found := I;
    I := I - 1
  end;
  FirstMissing := Found
end;

function TotalPages(): integer;
var
  I, Sum: integer;
begin
  Sum := 0;
  for I := 1 to Count do
    Sum := Sum + Batch[I].Pages;
  TotalPages := Sum
end;

procedure SaveCount();
var
  Log: file;
  Text: string[8];
  Written: integer;
begin
  Text := 'COUNT=';
  Text := Text + Chr(Ord('0') + Count);
  Assign(Log, 'TPSCOUNT.TXT');
  Rewrite(Log, 1);
  BlockWrite(Log, Text[1], Length(Text),
             Written)
end;

var
  Bad: integer;

begin
  Count := 0;
  Add(4501, 12, true);
  Add(4502, 3, true);
  Add(4503, 7, false);
  Add(4504, 20, true);
  writeln(Count, ' reports, ', TotalPages(),
          ' pages');
  Bad := FirstMissing();
  if Bad = Missing then
    writeln('every report has a cover sheet')
  else
    writeln('report ', Batch[Bad].Number,
            ' has no cover sheet');
  SaveCount();
  writeln('count saved')
end.
