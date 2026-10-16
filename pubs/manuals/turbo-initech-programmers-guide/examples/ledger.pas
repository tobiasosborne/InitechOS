program Ledger;
{ The quarterly ledger: a complete program in the
  order declarations are usually written. }
const
  Accounts = 4;
  Width = 24;

type
  Entry = record
    Number, Amount: integer;
    Closed: boolean;
  end;

var
  Book: array[1..Accounts] of Entry;
  Title: string[Width];
  Total, I: integer;

procedure Open(Index, Number, Amount: integer);
begin
  Book[Index].Number := Number;
  Book[Index].Amount := Amount;
  Book[Index].Closed := false
end;

function Balance(): integer;
var
  K, Sum: integer;
begin
  Sum := 0;
  for K := 1 to Accounts do
    if not Book[K].Closed then
      Sum := Sum + Book[K].Amount;
  Balance := Sum
end;

procedure Report(var Heading: string);
begin
  writeln(Heading);
  for I := 1 to Accounts do
    if not Book[I].Closed then
    begin
      write('  ', Book[I].Number);
      writeln(' ', Book[I].Amount)
    end
end;

var
  Heading: string;

begin
  Title := 'Ledger, third quarter';
  Open(1, 101, 4000);
  Open(2, 102, 3500);
  Open(3, 103, -570);
  Open(4, 104, 1400);
  Book[2].Closed := true;
  Heading := Title;
  Report(Heading);
  Total := Balance();
  writeln('Total ', Total)
end.
