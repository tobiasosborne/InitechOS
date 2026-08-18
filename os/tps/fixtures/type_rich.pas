program TypeRich;
const
  Lo = -1;
  Hi = 2;
  Letter = 'Q';
  Flag = true;
  Msg = 'OK';
type
  Pair = record
    Left, Right: integer;
    Ok: boolean;
  end;
var
  Count: integer;
  Ready: boolean;
  Ch: char;
  Name: string[8];
  DataFile: file;
  Grid: array[Lo..Hi] of integer;
  P: Pair;

procedure Swap(var A, B: integer); forward;

function IsReady(V: integer): boolean;
var
  Temp: integer;
begin
  Temp := V;
  IsReady := Temp > 0
end;

procedure Swap(var X, Y: integer);
var
  Temp: integer;
begin
  Temp := X;
  X := Y;
  Y := Temp
end;

begin
  Count := 1;
  Ready := IsReady(Count);
  Ch := Letter;
  Name := Msg + Ch;
  Grid[Lo] := Count;
  P.Left := Grid[Lo];
  P.Right := P.Left;
  P.Ok := Ready;
  Swap(P.Left, P.Right);
  Assign(DataFile, Name);
  Reset(DataFile, 1);
  BlockRead(DataFile, Name[1], 1, Count)
end.
