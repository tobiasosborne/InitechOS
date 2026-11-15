{$I-}
{ The IDE checks its own file errors with IOResult (TP 5.5 Reference Guide):
  the switch above is off for everything that follows. }
{ screen.pas -- the IDE's 80x25 screen: characters, attributes, cursor.

  Everything the IDE shows is drawn into these cells first; the product
  copies them to the console and the tier-0 driver dumps them for the
  comparison with the TP 5.5 captures (turbo/README.md "IDE").
  Attributes are VGA (background*16 + foreground), as in the captures.
  Source discipline as turbo.pas: complete boolean evaluation, so every
  bounds-sensitive access sits under nested ifs. }
const
  ScrRows = 25;
  ScrCols = 80;
  AtMenu = 112;        { 0x70 black on grey: menu bar, pulldowns, boxes }
  AtMenuKey = 116;     { 0x74 red on grey: hotkey letters }
  AtSelect = 15;       { 0x0F white on black: selected title or item }
  AtInfo = 23;         { 0x17 grey on blue: info line }
  AtText = 30;         { 0x1E yellow on blue: source text }
  AtError = 78;        { 0x4E yellow on red: error line }
  AtVerify = 127;      { 0x7F bright white on grey: Verify frame }
  AtErrFrame = 71;     { 0x47 grey on red: Error box frame }
  AtErrText = 79;      { 0x4F bright white on red: Error box text }
  AtBarText = 31;      { 0x1F bright white on blue: compile box result }
  AtBlink = 159;       { 0x9F blinking bright white on blue: Press any key }

var
  ScrCh: array[0..1999] of char;
  ScrAt: array[0..1999] of integer;
  ScrCurRow, ScrCurCol: integer;
  ScrCurOn: boolean;

procedure ScrPut(Row, Col: integer; C: char; A: integer);
begin
  if (Row >= 0) and (Row < ScrRows) then
    if (Col >= 0) and (Col < ScrCols) then
    begin
      ScrCh[Row * ScrCols + Col] := C;
      ScrAt[Row * ScrCols + Col] := A
    end
end;

procedure ScrFill(Row, Col0, Col1: integer; C: char; A: integer);
var I: integer;
begin
  I := Col0;
  while I <= Col1 do
  begin
    ScrPut(Row, I, C, A);
    I := I + 1
  end
end;

procedure ScrText(Row, Col: integer; var S: string; A: integer);
var I: integer;
begin
  I := 1;
  while I <= Length(S) do
  begin
    ScrPut(Row, Col + I - 1, S[I], A);
    I := I + 1
  end
end;

procedure ScrAttr(Row, Col: integer; A: integer);
begin
  if (Row >= 0) and (Row < ScrRows) then
    if (Col >= 0) and (Col < ScrCols) then
      ScrAt[Row * ScrCols + Col] := A
end;

procedure ScrCursor(Row, Col: integer; On: boolean);
begin
  ScrCurRow := Row;
  ScrCurCol := Col;
  ScrCurOn := On
end;

{ Decimal digits of a non-negative number, left-aligned. }
procedure NumText(N: integer; var S: string);
var D: string;
begin
  S := '';
  if N = 0 then S := '0';
  while N > 0 do
  begin
    D := chr(ord('0') + N mod 10);
    S := D + S;
    N := N div 10
  end
end;

{ A single-line frame (pulldowns and name boxes) or a double-line frame
  (Verify, Error and compile boxes), CP437 as captured. }
procedure ScrBox(Row0, Col0, Row1, Col1: integer; Double: boolean; A: integer);
var R: integer;
begin
  if Double then
  begin
    ScrPut(Row0, Col0, chr(201), A); ScrFill(Row0, Col0 + 1, Col1 - 1, chr(205), A);
    ScrPut(Row0, Col1, chr(187), A);
    ScrPut(Row1, Col0, chr(200), A); ScrFill(Row1, Col0 + 1, Col1 - 1, chr(205), A);
    ScrPut(Row1, Col1, chr(188), A)
  end
  else
  begin
    ScrPut(Row0, Col0, chr(218), A); ScrFill(Row0, Col0 + 1, Col1 - 1, chr(196), A);
    ScrPut(Row0, Col1, chr(191), A);
    ScrPut(Row1, Col0, chr(192), A); ScrFill(Row1, Col0 + 1, Col1 - 1, chr(196), A);
    ScrPut(Row1, Col1, chr(217), A)
  end;
  R := Row0 + 1;
  while R < Row1 do
  begin
    if Double then
    begin ScrPut(R, Col0, chr(186), A); ScrPut(R, Col1, chr(186), A) end
    else
    begin ScrPut(R, Col0, chr(179), A); ScrPut(R, Col1, chr(179), A) end;
    R := R + 1
  end
end;

{ A box title centred in the top edge with one space each side. }
procedure ScrTitle(Row, Col0, Col1: integer; var S: string; A: integer);
var C: integer;
    T: string;
begin
  C := Col0 + (Col1 - Col0 + 1 - Length(S) - 2) div 2;
  T := ' ' + S + ' ';
  ScrText(Row, C, T, A)
end;
