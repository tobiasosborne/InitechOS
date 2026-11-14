{$mode delphi}{$B+}{$H-}
program LengthConcat;
var s: string;
begin
 s := 'ab';
 writeln(length(s + 'cd'));
 writeln(length('xx' + s + 'yy'))
end.
