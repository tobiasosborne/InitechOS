program Tokens;
{ Identifiers are not case-sensitive:
  TOTAL, Total and total are one variable. }
const
  Quote = 'It''s done';  (* '' is one ' *)
  Empty = '';
  Letter = 'A';
var
  Total, Line_Count: integer;
begin
  Total := 100;
  TOTAL := total + 2147483600;
  Line_Count := 3;
  writeln(Quote);
  writeln('[', Empty, ']', Letter, Line_count);
  WriteLn(Total)
end.
