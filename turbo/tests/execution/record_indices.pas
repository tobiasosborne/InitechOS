{ FPC -Mtp reference: initech-uaz6c. Separate source/destination effects
  avoid asserting a mutual evaluation order. Include self-copy. }
program RecordIndices;
type Pair = record a,b: integer; end;
var src,dst: array[1..4] of Pair; calls: integer;
function Index(dummy: integer): integer;
begin calls := calls + 1; Index := calls end;
begin
 src[1].a := 11; src[1].b := 12; src[2].a := 21; src[2].b := 22;
 dst[2].a := 0; dst[2].b := 0;
 calls := 0; dst[Index(0)] := src[1];
 writeln(calls,' ',dst[1].a,' ',dst[1].b,' ',dst[2].a,' ',dst[2].b);
 calls := 0; dst[2] := src[Index(0)];
 writeln(calls,' ',dst[2].a,' ',dst[2].b);
 calls := 0; src[1] := src[Index(0)];
 writeln(calls,' ',src[1].a,' ',src[1].b)
end.
