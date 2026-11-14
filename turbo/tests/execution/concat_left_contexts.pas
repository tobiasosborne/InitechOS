{$mode delphi}{$B+}{$H-}
program ConcatLeftContexts;
type Item=record c: char; end;
var s,u,v: string; r: array[0..1] of Item;
function Check(n: integer): integer;
var text: string; sentinel: integer;
begin
 sentinel:=777;
 text:='PRE'+(chr(length(s+'cd'))+'ZZ');
 writeln(sentinel,' ',length(text),' ',ord(text[4]),' ',text[5],text[6]);
 Check:=ord(text[4])
end;
begin
 s:='ab'; u:=chr(4);
 writeln(Check(0));
 writeln(('P'+(chr(length(s+'cd'))+'ZZ'))=('P'+u+'ZZ'));
 writeln(('P'+u+'ZZ')=('P'+(chr(length(s+'cd'))+'ZZ')));
 r[0].c:='A'; r[1].c:='B'; s:='X';
 v:='P'+(r[ord(s+'Z'='XZ')].c+'!!');
 writeln(v);
 u:='PB';
 writeln(('P'+(r[ord(s+'Z'='XZ')].c+'!!'))=(u+'!!'));
 writeln((u+'!!')=('P'+(r[ord(s+'Z'='XZ')].c+'!!')));
 writeln('done')
end.
