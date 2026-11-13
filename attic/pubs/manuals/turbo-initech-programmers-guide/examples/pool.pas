program Pool;
{ A name pool: every distinct name is stored
  once, in one character array; a name is
  known by its offset. }
const
  PoolMax = 64;
  NameMax = 8;
var
  Chars: array[1..PoolMax] of char;
  Start, Len: array[1..NameMax] of integer;
  Used, Names: integer;
  Failed: boolean;

function Same(K: integer;
              var S: string): boolean;
var
  I: integer;
  Equal: boolean;
begin
  Equal := Len[K] = Length(S);
  for I := 1 to Length(S) do
    if Equal then
      if Chars[Start[K] + I - 1] <> S[I] then
        Equal := false;
  Same := Equal
end;

function Intern(var S: string): integer;
var
  K, Found, I: integer;
begin
  Found := 0;
  for K := 1 to Names do
    if Found = 0 then
      if Same(K, S) then Found := K;
  if Found = 0 then
    if Names >= NameMax then
      Failed := true
    else if Used + Length(S) > PoolMax then
      Failed := true
    else
    begin
      Names := Names + 1;
      Start[Names] := Used + 1;
      Len[Names] := Length(S);
      for I := 1 to Length(S) do
        Chars[Used + I] := S[I];
      Used := Used + Length(S);
      Found := Names
    end;
  Intern := Found
end;

var
  Name: string;
  A, B, C: integer;

begin
  Name := 'total'; A := Intern(Name);
  Name := 'count'; B := Intern(Name);
  Name := 'total'; C := Intern(Name);
  writeln(A, ' ', B, ' ', C, ' ', (A = C));
  writeln(Names, ' names in ', Used, ' chars');
  writeln('failed: ', Failed)
end.
