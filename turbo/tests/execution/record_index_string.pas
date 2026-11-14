{$mode delphi}{$B+}{$H-}
program RecordIndexString;
type Pair=record a,b: integer; end;
var r: array[0..1] of Pair; dest: Pair; s: string;
begin
 r[0].a:=10; r[0].b:=11; r[1].a:=20; r[1].b:=21; s:='X';
 dest:=r[ord(s+'Z'='XZ')];
 writeln(dest.a,' ',dest.b)
end.
