{ FPC -Mtp reference: initech-ichdv. Reads must zero extend and writes
  must preserve adjacent bytes for every possible var-char actual. }
program CharAlias;
type Item = record c: char; n: integer; end;
var s: string; short: string[4]; sentinel: integer;
    edge: string[3]; guard: integer;
    c: char; a: array[-1..1] of char; r: Item;
function ReadChar(var ch: char): integer;
begin ReadChar := ord(ch) end;
function CharResult(dummy: integer): char;
begin CharResult := chr(253) end;
procedure Change(var ch,alias: char);
begin ch := chr(254); writeln(ord(alias), ' ', ReadChar(ch)) end;
procedure ForwardChar(var ch: char);
begin Change(ch,ch) end;
procedure Run(dummy: integer);
var before: integer; local: char; after: integer;
begin
 writeln(ord(CharResult(0)));
 before := 777; after := 888;
 local := 'Q'; ForwardChar(local);
 writeln(ord(local),' ',before,' ',after)
end;
begin
 s := 'ABCD'; writeln(ReadChar(s[1])); ForwardChar(s[1]);
 writeln(ord(s[1]),' ',ord(s[2]),' ',ord(s[3]),' ',ord(s[4]));
 sentinel := 777; short := 'ABCD'; ForwardChar(short[4]);
 writeln(ord(short[3]),' ',ord(short[4]),' ',sentinel);
 edge := 'XYZ'; guard := 777; ForwardChar(edge[3]);
 writeln(ord(edge[2]),' ',ord(edge[3]),' ',guard);
 c := 'C'; ForwardChar(c); writeln(ord(c));
 a[-1] := 'A'; a[0] := 'B'; a[1] := 'C'; ForwardChar(a[0]);
 writeln(ord(a[-1]),' ',ord(a[0]),' ',ord(a[1]));
 r.c := 'R'; r.n := 999; ForwardChar(r.c); writeln(ord(r.c),' ',r.n);
 Run(0)
end.
