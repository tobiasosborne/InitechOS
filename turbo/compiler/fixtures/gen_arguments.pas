{$MODE DELPHI}{$B+}{$H-}
{ TP 5.5 Reference Guide ParamCount/ParamStr; FPC is the independent
  execution oracle. No assumptions about host executable paths. }
program Arguments;
var S: string;
begin
  Writeln('COUNT ', ParamCount());
  S := ParamStr(1) + '/' + ParamStr(2);
  Writeln('PAIR ', S);
  S := 'a' + (ParamStr(1) + ParamStr(2)); Writeln('NEST ', S);
  Writeln('LENGTH ', Length(ParamStr(1)));
  Writeln('MISSING ', Length(ParamStr(99)));
  Writeln('NEGATIVE ', Length(ParamStr(-1)))
end.
