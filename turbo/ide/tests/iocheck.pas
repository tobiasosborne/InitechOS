{ The default $I+ state in the booted image: a failed Reset prints the
  runtime diagnostic and ends the program (iocheck.expect). }
program IoCheck;
var
  F: file;
begin
  Assign(F, 'NOFILE.BIN');
  Writeln('BEFORE');
  Reset(F, 1);
  Writeln('AFTER')
end.
