{$MODE DELPHI}{$B+}{$H-}
program ExecuteChild;
var Path: string;
    Status: integer;
begin
  Path := 'CHILD.COM'; Status := Execute(Path);
  Writeln('CHILD-STATUS ', Status);
  Path := 'MISSING.COM'; Status := Execute(Path);
  Writeln('MISSING-STATUS ', Status)
end.
