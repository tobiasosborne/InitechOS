program Overtime;
const
  RateCents = 1250;
  Normal = 40;
var
  Hours, Gross, Extra: integer;
begin
  Hours := 46;
  if Hours > Normal then
  begin
    Extra := Hours - Normal;
    Gross := Normal * RateCents
             + Extra * RateCents * 3 div 2;
    writeln('Overtime hours: ', Extra)
  end
  else
    Gross := Hours * RateCents;
  writeln('Gross in cents: ', Gross)
end.
