program Later;

procedure First();
begin
  Second()
end;

procedure Second();
begin
  writeln('second')
end;

begin
  First()
end.
