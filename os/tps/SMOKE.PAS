{
  SMOKE.PAS -- SMOKETPS, the Turbo Initech FLAIR smoke tenant (bead
  initech-tdnl.22 R5.2; docs/design/GUI-remediation-D1-D2-D3-design.md D2.7).
  The body that follows FLAIR.PAS:

      COPY /B FLAIR.PAS+SMOKE.PAS TPSIN.PAS

  It calls EVERY binding of FLAIR.PAS at least once and feeds the gate's
  answers back into later calls (a returned window token, origin and local
  point are drawn with), so the ordered trap sequence it produces -- the
  cross-compiler contract of DEC-10 -- carries both directions of every
  binding. test-flairstub-smoke compiles it with the seed, with Turbo
  Initech and with Free Pascal and requires the three traces to agree.

  Behaviour: the main block registers SMOKETPS, installs File (Close ^W,
  a divider, a disabled About) + Edit (Copy ^C, Paste ^V), opens a closable
  document window titled Smoke and returns. updateEvt paints the window;
  a keyDown draws the key in the fixed cells; a mouseDown marks the clicked
  point; Edit > Copy puts the window's caption on the Scrap and Edit > Paste
  reads it back and draws it; File > About raises an alert; the alert's
  answer is drawn; File > Close (or the close box) exits.

  SMOKE_MUT_SKIP_CLICK (Rule 6): the anchored line below is replaced by a
  source mutant that ignores the click -- the trace must then differ.

  Written order-independent under complete boolean evaluation (DEC-03).
  ASCII-clean (Rule 12).
}
const
  SmokeFile = 129;
  SmokeEdit = 130;
  ItemClose = 1;
  ItemAbout = 3;
  ItemCopy = 1;
  ItemPaste = 2;

var
  Win, Rc, Clicks, Keys: integer;
  S, T, U, V: string;

procedure DrawNumber(Y, N: integer);
var
  D, Q: integer;
  Digits: string;
begin
  { the number in the fixed cells, most significant digit first }
  Digits := '';
  Q := N;
  if Q < 0 then Q := -Q;
  repeat
    D := Q mod 10;
    Digits := Chr(Ord('0') + D) + Digits;
    Q := Q div 10
  until Q = 0;
  if N < 0 then Digits := '-' + Digits;
  Rc := FlairDrawCells(Win, 8, Y, Digits, FlairBlack, FlairWhite)
end;

procedure Paint();
var
  Origin: integer;
begin
  Rc := FlairFillRect(Win, 0, 0, 280, 120, FlairWhite);
  Rc := FlairFillRect(Win, 4, 4, 276, 6, FlairGray);
  S := 'Hello from Turbo Initech';
  Rc := FlairTextDraw(Win, 8, 10, S, FlairBlack, FlairWhite);
  S := 'bold';
  Rc := FlairTextDraw(Win, 8, 28, S, FlairBlack + FlairBold, FlairWhite);
  S := 'Geneva 9';
  Rc := FlairTextDraw(Win, 60, 28, S, FlairNavy + FlairGeneva9, FlairWhite);
  Origin := FlairContentOrigin(Win);
  DrawNumber(48, Origin div 65536);
  DrawNumber(64, Origin mod 65536);
  DrawNumber(80, Clicks * 100 + Keys)
end;

procedure Click(var Ev: FlairEvent);
var
  Local, LH, LV: integer;
begin
  Local := FlairGlobalToLocal(Win, Ev.WhereH, Ev.WhereV);
  LV := Local div 65536;
  LH := Local mod 65536;
  Rc := FlairFillRect(Win, LH, LV, LH + 3, LV + 3, FlairBlack);
  Clicks := Clicks + 1 { SMOKE_MUT_SKIP_CLICK }
end;

procedure Menu(Message: integer);
var
  Id, Item: integer;
begin
  Id := Message div 65536;
  Item := Message mod 65536;
  if Id = SmokeFile then
  begin
    if Item = ItemClose then Rc := FlairExit(7);
    if Item = ItemAbout then
    begin
      S := 'SMOKETPS' + Chr(13) + 'a Turbo Initech tenant';
      T := 'OK';
      U := 'Cancel';
      V := '';
      Rc := FlairAlert(S, T, U, V);
      DrawNumber(96, Rc)
    end
  end;
  if Id = SmokeEdit then
  begin
    if Item = ItemCopy then
    begin
      S := 'Smoke on the Scrap';
      Rc := FlairScrapPut(S);
      DrawNumber(96, Rc)
    end;
    if Item = ItemPaste then
    begin
      S := '';
      Rc := FlairScrapGet(S);
      DrawNumber(96, Rc);
      Rc := FlairTextDraw(Win, 120, 96, S, FlairBlack, FlairWhite)
    end
  end
end;

procedure FlairEventHandler(var Ev: FlairEvent);
begin
  if Ev.What = FlairUpdateEvt then Paint();
  if Ev.What = FlairMouseDown then Click(Ev);
  if (Ev.What = FlairKeyDown) or (Ev.What = FlairAutoKey) then
  begin
    Keys := Keys + 1;
    S := Chr(Ev.Message mod 256);
    Rc := FlairDrawCells(Win, 200, 64, S, FlairWhite, FlairBlack)
  end;
  if Ev.What = FlairMenuEvt then Menu(Ev.Message);
  if Ev.What = FlairAlertEvt then DrawNumber(112, Ev.Message);
  if Ev.What = FlairGoAwayEvt then Rc := FlairExit(0)
end;

begin
  Clicks := 0;
  Keys := 0;
  T := 'SMOKETPS';
  Rc := FlairRegister(T);
  T := 'File';
  Rc := FlairNewMenu(SmokeFile, T);
  T := 'Close';
  Rc := FlairAppendItem(T, 'W', true);
  T := '-';
  Rc := FlairAppendItem(T, Chr(0), false);
  T := 'About Smoke';
  Rc := FlairAppendItem(T, Chr(0), true);
  T := 'Edit';
  Rc := FlairNewMenu(SmokeEdit, T);
  T := 'Copy';
  Rc := FlairAppendItem(T, 'C', true);
  T := 'Paste';
  Rc := FlairAppendItem(T, 'V', true);
  Rc := FlairSetMenuBar(true);
  Rc := FlairDrawMenuBar();
  Win := FlairNewWindow(120, 90, 400, 230, FlairGoAwayAsk);
  T := 'Smoke';
  Rc := FlairSetWTitle(Win, T)
end.
