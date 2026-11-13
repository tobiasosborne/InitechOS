program Pay;
{ Weekly pay in cents: no real numbers are
  needed when money is counted in cents. }
const
  RateCents = 1250;      { 12.50 an hour }
var
  Hours, Gross: integer;
begin
  Hours := 40;
  Gross := Hours * RateCents;
  writeln('Hours: ', Hours);
  writeln('Gross: ', Gross div 100, '.',
          Gross mod 100 div 10, Gross mod 10)
end.
