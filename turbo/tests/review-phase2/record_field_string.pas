{$mode delphi}{$B+}{$H-}
program RecordFieldString;
type Item=record value: integer; end;
var r: array[0..1] of Item; s: string;
begin
 r[0].value:=10; r[1].value:=20; s:='X';
 writeln(r[ord(s+'Z'='XZ')].value)
end.
