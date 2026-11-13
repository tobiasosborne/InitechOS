program Layout;
type
  Cell = record
    Row, Col: integer;
    Marked: char;
  end;
var
  Count: integer;
  Done: boolean;
  Grid: array[1..3] of Cell;
  Name: string[10];
  Log: file;

function Area(W, H: integer;
              var Cells: Cell): integer;
var
  Product: integer;
  Tag: string[3];
begin
  Tag := 'ok';
  Product := W * H;
  Cells.Row := W;
  Area := Product
end;

begin
  Count := Area(4, 5, Grid[1]);
  writeln(Count, ' ', Grid[1].Row, ' ', Done)
end.
