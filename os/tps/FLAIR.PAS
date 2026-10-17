{$MODE DELPHI}{$B+}{$H-}
{
  FLAIR.PAS -- the Turbo Initech binding to the FLAIR Toolbox: the head of
  every FLAIR application written in Turbo Initech Pascal (beads
  initech-tdnl.22 R5.2, initech-tdnl.23 R5.3).

  A Turbo Initech program is one file (Programmer's Guide Chapter 8), so a
  FLAIR application is this head followed by the application's own
  declarations and main block, joined with the InitechDOS COPY command:

      COPY /B FLAIR.PAS+MYAPP.PAS TPSIN.PAS
      TPS

  MYAPP.PAS therefore has no program heading: it continues the declaration
  part this file begins, defines FlairEventHandler (declared forward below;
  the compiler rejects a program that never defines it), and ends with the
  main block. The main block is the application's entry: it registers,
  installs its menus, opens its window and RETURNS. Every later event is
  pushed to FlairEventHandler; returning from the handler is the cooperative
  yield (ADR-0013 Sec 3.4; spec/toolbox_gate.h Sec 6). A FLAIR application
  never runs an event loop and never calls WaitNextEvent.

  THE BINDING (ADR-0007 DEC-09/DEC-10 as reconciled in
  docs/design/GUI-remediation-ADR-reconciliation.md Part B; D2.1-D2.5 of
  docs/design/GUI-remediation-D1-D2-D3-design.md). Every routine below is an
  `external` routine: its body is a hand-assembled stub in
  seed/rt/flairstub.asm (the start_dos/fileio run-time lineage) that
  executes INT 81h, the Initech Toolbox Gate, with the function code in AX
  and the arguments as one cdecl block (spec/toolbox_gate.h Sec 2-3). The
  stub object is linked into the application; it also carries the entry
  point, the link-synthesized registration record (spec Sec 5) and the
  event buffer (spec Sec 6), so no Pascal source ever forms an address.

  ZERO SUBSET GROWTH (DEC-09): handles are integers; FlairEvent is a flat
  record of integer fields passed by var; strings are var string
  parameters, converted to NUL-terminated copies inside the stub's own
  image storage (the gate refuses pointers outside the image, spec Sec 5/7);
  booleans and chars are value parameters. Errors are RETURNED (negative,
  spec Sec 4), never fatal: every binding is a function whose result the
  application keeps.

  The constants are the locked values of spec/toolbox_gate.h and
  spec/event_model.h; test-flairstub checks every one against those headers
  and every stub's AX code and argument block against the gate's map.

  ASCII-clean (Rule 12). Deterministic (Rule 11).
}
program FlairApp;

const
  { spec/toolbox_gate.h Sec 4: results and errors }
  FlairOK = 0;
  FlairErrBadCode = -1;
  FlairErrNotReg = -2;
  FlairErrBadArg = -3;
  FlairErrBusy = -4;
  FlairErrNoMem = -5;
  FlairErrBadRec = -6;
  FlairErrNotTenant = -7;
  FlairErrNoType = -102;

  { Sec 7: drawing colour tokens (roles, never RGB) }
  FlairWhite = 0;
  FlairBlack = 1;
  FlairGray = 2;
  FlairNavy = 3;

  { Sec 7 STYLE / FONT: added to a TEXTDRAW foreground token }
  FlairBold = 256;
  FlairItalic = 512;
  FlairUnderline = 1024;
  FlairGeneva9 = 65536;

  { Sec 3 / Sec 11: the NEWWINDOW goAway word }
  FlairGoAwayNone = 0;
  FlairGoAwayClose = 1;
  FlairGoAwayAsk = 2;
  FlairRDocProc = 4096;

  { spec/event_model.h Sec 1: the Event Manager codes (MTE Table 2-1) }
  FlairNullEvent = 0;
  FlairMouseDown = 1;
  FlairMouseUp = 2;
  FlairKeyDown = 3;
  FlairKeyUp = 4;
  FlairAutoKey = 5;
  FlairUpdateEvt = 6;
  FlairActivateEvt = 8;

  { spec/toolbox_gate.h Sec 6a, 11: the App Contract deliveries }
  FlairMenuEvt = 81;
  FlairGoAwayEvt = 32;
  FlairAlertEvt = 144;

  { Sec 11: the alert's answers }
  FlairAlertDefault = 1;
  FlairAlertCancel = 2;
  FlairAlertOther = 3;

  { The stub's own menu storage (seed/rt/flairstub.asm): at most this many
    menus and items in the bar FlairSetMenuBar installs; a menu or item
    beyond them is refused with FlairErrBadArg. }
  FlairMaxMenus = 8;
  FlairMaxItems = 32;

type
  { Sec 6: the flat event buffer, six integers in this order }
  FlairEvent = record
    What, Message, Modifiers, When, WhereH, WhereV: integer;
  end;

{ ---- lifecycle (Sec 3: 0x0001 REGISTER, 0x0070 EXIT) ---- }
function FlairRegister(var Name: string): integer; external;
function FlairExit(Code: integer): integer; external;

{ ---- windows (0x0020 NEWWINDOW, 0x0022 SETWTITLE, 0x0023 CONTENTORIGIN,
  0x00C3 GLOBALTOLOCAL) ---- }
function FlairNewWindow(L, T, R, B, GoAway: integer): integer; external;
function FlairSetWTitle(Win: integer; var Title: string): integer; external;
function FlairContentOrigin(Win: integer): integer; external;
function FlairGlobalToLocal(Win, H, V: integer): integer; external;

{ ---- drawing (0x0030 TEXTDRAW, 0x0031 FILLRECT, 0x0033 DRAWCELLS) ---- }
function FlairTextDraw(Win, X, Y: integer; var S: string;
                       Fg, Bg: integer): integer; external;
function FlairFillRect(Win, L, T, R, B, Color: integer): integer; external;
function FlairDrawCells(Win, X, Y: integer; var S: string;
                        Fg, Bg: integer): integer; external;

{ ---- menus (0x0050 SETMBAR, 0x0052 DRAWMENUBAR). FlairNewMenu and
  FlairAppendItem build the bar in the stub's image storage (IM NewMenu /
  AppendMenu; an item whose text is '-' is a dividing line); FlairSetMenuBar
  installs it. A choice arrives as a FlairMenuEvt event whose Message is
  MenuID * 65536 + Item. ---- }
function FlairNewMenu(MenuID: integer; var Title: string): integer; external;
function FlairAppendItem(var Text: string; CmdChar: char;
                         Enabled: boolean): integer; external;
function FlairSetMenuBar(HasApple: boolean): integer; external;
function FlairDrawMenuBar(): integer; external;

{ ---- the alert (0x0090 ALERT): asked here, answered by a FlairAlertEvt
  event whose Message is the button hit. An empty label is no button. ---- }
function FlairAlert(var Text, DefLabel, CancelLabel,
                    OtherLabel: string): integer; external;

{ ---- the Scrap, TEXT flavor (0x00A0 / 0x00A1), ShortString payloads (D2-4):
  FlairScrapGet returns the length it copied into S, or an error ---- }
function FlairScrapPut(var S: string): integer; external;
function FlairScrapGet(var S: string): integer; external;

{ ---- the event handler: the application defines it ---- }
procedure FlairEventHandler(var Ev: FlairEvent); forward;
