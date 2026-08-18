program ParseRich;
const
  N = 2;
type
  Rec = record
    x: integer;
    ok: boolean;
    ch: char;
  end;
var
  i: integer;
  a: array[0..N] of Rec;
  s: string[8];
  f: file;
function Recur(n: integer; var r: Rec): integer;
begin
  if n > 0 then
    Recur := ord(chr(n)) + Recur(n - 1, r)
  else
    Recur := r.x
end;
procedure Step(var x: integer);
forward;
procedure Step(var x: integer);
begin
  x := x + 1
end;
begin
  a[i].x := Recur(1 + 2 * 3, a[i]);
  if not a[i].ok or (i = N) and true then
    if i = N then Step(i) else writeln(s);
  while i < N do i := i + 1;
  for i := 0 to N do Step(i);
  for i := N downto 0 do Step(i);
  repeat i := i - 1 until i = 0;
  Assign(f, s);
  Reset(f, 1);
  Rewrite(f, 1);
  BlockRead(f, s[1], 1, i);
  BlockWrite(f, s[1], 1, i)
end.
