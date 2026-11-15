var P,D: string; F:file; N:integer;
begin
 FileMode:=0; FileDrive:='C'; FileDir:='C:'; IdeStart();
 TypeChar('X'); P:=WorkName0;
 Assign(F,P); Rewrite(F,1); D:='OLD'; BlockWrite(F,D[1],3,N); Close(F);
 DoLoad(P);
 Assign(F,P); Reset(F,1); BlockRead(F,D[1],3,N); Close(F); SetLength(D,N);
 if (UiMode<>ModeError) or (not EdModified) or (D<>'OLD') then
 begin Writeln('FAIL reserved Load conserves current page and file: mode=',UiMode,' dirty=',EdModified,' bytes=',D); Halt(1) end;
 Writeln('PASS reserved Load conservation')
end.
