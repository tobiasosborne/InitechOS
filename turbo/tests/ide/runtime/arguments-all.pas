{$MODE DELPHI}{$B+}{$H-}
{ MINT07: quotes are ordinary DOS tail characters; the executable path is
  supplied by the DOS 3.x environment trailer, not guessed by the compiler. }
program ArgumentsAll;
var S: string;
    N: integer;
begin
  Writeln('COUNT=', ParamCount()); N := 0;
  while N <= 5 do
  begin S := ParamStr(N); Writeln('ARG', N, '=', S); N := N + 1 end;
  GetDir(0, S); Writeln('DIR=', S)
end.
