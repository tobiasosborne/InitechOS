program probe;
var i,count: integer;
begin
  count := 0;
  for i := 2147483647 to 2147483647 do
  begin
    count := count + 1;
    if count <= 2 then writeln('iteration ', count)
  end;
  writeln('done')
end.
