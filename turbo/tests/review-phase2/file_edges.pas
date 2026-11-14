{$mode delphi}{$B+}{$H-}
program FileEdges;
var f: file; s,t: string; n: integer;
begin
 s:='ABCD'; t:='xxxx';
 Assign(f,'EDGE.BIN'); Rewrite(f,1);
 BlockWrite(f,s[1],0,n); writeln(n);
 BlockWrite(f,s[2],2,n); writeln(n);
 Reset(f,1); BlockRead(f,t[2],2,n); writeln(n,' ',t);
 BlockRead(f,t[1],1,n); writeln(n);
 BlockRead(f,t[1],0,n); writeln(n)
end.
