program Directory;
{ The staff directory: saves the staff to a
  file as lines of text, reads them back,
  sorts them by name and prints a report. }
const
  MaxStaff = 8;
  NameLen = 12;
  LF = 10;

type
  Person = record
    Badge, Desk: integer;
  end;

var
  Staff: array[1..MaxStaff] of Person;
  Names: array[1..MaxStaff] of integer;
  Pool: string;
  Count: integer;
  Data: file;

{ Names are kept in one string, NameLen
  characters each: a string cannot be an
  array element. }
procedure GetName(Slot: integer; var S: string);
var
  I: integer;
  C: char;
begin
  S := '';
  for I := 1 to NameLen do
  begin
    C := Pool[(Slot - 1) * NameLen + I];
    if C <> ' ' then S := S + C
  end
end;

procedure PutName(Slot: integer; var S: string);
var
  I: integer;
begin
  for I := 1 to NameLen do
    if I <= Length(S) then
      Pool[(Slot - 1) * NameLen + I] := S[I]
    else
      Pool[(Slot - 1) * NameLen + I] := ' '
end;

procedure IntToStr(N: integer; var S: string);
begin
  S := '';
  repeat
    S := Chr(Ord('0') + N mod 10) + S;
    N := N div 10
  until N = 0
end;

procedure Add(Badge, Desk: integer;
              var Name: string);
begin
  Count := Count + 1;
  Staff[Count].Badge := Badge;
  Staff[Count].Desk := Desk;
  Names[Count] := Count;
  PutName(Count, Name)
end;

procedure Save();
var
  I, Wrote: integer;
  Line, Part: string;
begin
  Rewrite(Data, 1);
  for I := 1 to Count do
  begin
    IntToStr(Staff[I].Badge, Line);
    IntToStr(Staff[I].Desk, Part);
    Line := Line + ',' + Part + ',';
    GetName(I, Part);
    Line := Line + Part + Chr(LF);
    BlockWrite(Data, Line[1], Length(Line),
               Wrote)
  end
end;

{ Reads one line; Done when none is left. }
procedure ReadLine(var S: string;
                   var Done: boolean);
var
  Buf: string[1];
  Got: integer;
  Stop: boolean;
begin
  S := '';
  Buf := ' ';
  Stop := false;
  while not Stop do
  begin
    BlockRead(Data, Buf[1], 1, Got);
    if Got = 0 then
      Stop := true
    else if Ord(Buf[1]) = LF then
      Stop := true
    else
      S := S + Buf[1]
  end;
  Done := Length(S) = 0
end;

{ The number at S[At]; At moves past the
  comma that follows it. }
function Field(var S: string;
               var At: integer): integer;
var
  N: integer;
  Stop: boolean;
begin
  N := 0;
  Stop := false;
  while not Stop do
    if At > Length(S) then
      Stop := true
    else if S[At] = ',' then
    begin
      At := At + 1;
      Stop := true
    end
    else
    begin
      N := N * 10 + Ord(S[At]) - Ord('0');
      At := At + 1
    end;
  Field := N
end;

procedure Load();
var
  Line, Name: string;
  Done: boolean;
  At, Badge, Desk: integer;
begin
  Count := 0;
  Reset(Data, 1);
  ReadLine(Line, Done);
  while not Done do
  begin
    At := 1;
    Badge := Field(Line, At);
    Desk := Field(Line, At);
    Name := '';
    while At <= Length(Line) do
    begin
      Name := Name + Line[At];
      At := At + 1
    end;
    Add(Badge, Desk, Name);
    ReadLine(Line, Done)
  end
end;

{ Sorts the index array Names, not the
  records themselves. }
procedure SortByName();
var
  I, J, Hold: integer;
  A, B: string;
  Placed: boolean;
begin
  for I := 2 to Count do
  begin
    Hold := Names[I];
    GetName(Hold, B);
    J := I - 1;
    Placed := false;
    while not Placed do
      if J < 1 then
        Placed := true
      else
      begin
        GetName(Names[J], A);
        if A > B then
        begin
          Names[J + 1] := Names[J];
          J := J - 1
        end
        else
          Placed := true
      end;
    Names[J + 1] := Hold
  end
end;

procedure Report();
var
  I, K: integer;
  Name: string;
begin
  writeln('Name         Badge  Desk');
  for I := 1 to Count do
  begin
    K := Names[I];
    GetName(K, Name);
    write(Name);
    while Length(Name) < 13 do
    begin
      write(' ');
      Name := Name + ' '
    end;
    writeln(Staff[K].Badge, '    ',
            Staff[K].Desk)
  end
end;

var
  Name: string;

begin
  Pool := '';
  while Length(Pool) < MaxStaff * NameLen do
    Pool := Pool + ' ';
  Count := 0;
  Name := 'Samir'; Add(103, 12, Name);
  Name := 'Peter'; Add(101, 10, Name);
  Name := 'Milton'; Add(109, 0, Name);
  Name := 'Michael'; Add(102, 11, Name);
  Assign(Data, 'STAFF.TXT');
  Save();
  Load();
  SortByName();
  Report();
  writeln(Count, ' people')
end.
