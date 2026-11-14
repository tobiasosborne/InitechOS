{ FPC -Mtp reference: initech-08m74, initech-6154u.
  Preserve aliases and both neighbors across local/global/array var calls. }
program RecordLayout;
type Pair = record a,b: integer; end;
var global: Pair; ga: array[-2..0] of Pair;
procedure Change(var r,alias: Pair);
begin r.b := 99; alias.a := r.b + 1; writeln(r.a, ' ', alias.b) end;
procedure ForwardPair(var r: Pair);
begin Change(r,r) end;
procedure AssignPair(var dst,src: Pair);
begin dst := src end;
procedure Run(dummy: integer);
var before: integer; r: Pair; arr: array[-2..0] of Pair; after: integer;
begin
 before := 777; after := 888;
 r.a := 11; r.b := 22; ForwardPair(r);
 arr[-2] := r; arr[-1].a := 21; arr[-1].b := 22;
 arr[0] := arr[-1]; ForwardPair(arr[0]);
 global := arr[-2]; ForwardPair(global);
 ga[-2] := arr[0]; arr[-1] := ga[-2];
 ForwardPair(ga[-2]);
 ga[0].a := 51; ga[0].b := 52; AssignPair(r,ga[0]);
 writeln(r.a,' ',r.b,' ',arr[-2].a,' ',arr[-2].b);
 writeln(arr[-1].a,' ',arr[-1].b,' ',arr[0].a,' ',arr[0].b);
 writeln(global.a,' ',global.b,' ',ga[-2].a,' ',ga[-2].b);
 writeln(before,' ',after)
end;
begin Run(0) end.
