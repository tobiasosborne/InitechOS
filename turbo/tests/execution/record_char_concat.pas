{$mode delphi}{$B+}{$H-}
program RecordCharConcat;
type Item=record c: char; end;
var r: array[0..1] of Item; s,u: string;
begin
 r[0].c:='A'; r[1].c:='B'; s:='X';
 u:=r[ord(s+'Z'='XZ')].c+'!!';
 writeln(u); writeln('done')
end.
