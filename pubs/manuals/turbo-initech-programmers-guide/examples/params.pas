program Params;
type
  Account = record
    Number, Balance: integer;
  end;
var
  A, B: integer;
  Name: string;
  Acct: account;

procedure Swap(var X, Y: integer);
var
  T: integer;
begin
  T := X;
  X := Y;
  Y := T
end;

procedure Bump(N: integer);
begin
  N := N + 1;
  writeln('Bump sees ', N)
end;

procedure Credit(var R: Account;
                 Amount: integer);
begin
  R.Balance := R.Balance + Amount
end;

procedure Shout(var S: string);
begin
  S := S + '!'
end;

begin
  A := 1;
  B := 2;
  Swap(A, B);
  Bump(A);
  writeln(A, ' ', B);
  Acct.Balance := 100;
  Credit(Acct, 25);
  Name := 'Milton';
  Shout(Name);
  writeln(Acct.Balance, ' ', Name)
end.
