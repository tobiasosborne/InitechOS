INITECH SYSTEMS CORPORATION
APPLICATION EXAMPLES

Change to the APPS directory before using these examples.

INITECHBASE
  Start INITBASE. At the dot prompt enter:
    USE CLIENTS
    LIST
    USE PARTS
    LIST
    USE INVOICE
    DO Y2KACCT
  QUIT returns to InitechDOS. The accounting report uses two-digit years.
  Inspect the reporting date and aging columns before acting on a balance.

TURBO INITECH
  HELLO.PAS is a short introductory program. The compiler reads the fixed
  input name TURBOIN.PAS; initially this contains the same program.
    COPY HELLO.PAS TURBOIN.PAS
    TURBO
    TURBOOUT
  Small programs are written as TURBOOUT.COM. Larger programs are written
  as TURBOOUT.EXE. TURBOOUT.S is an assembly listing.
  TURBO.PAS contains the complete compiler source. To compile it, copy
  TURBO.PAS to TURBOIN.PAS and run TURBO. Keep the original TURBO.COM.

TPS REPORT GENERATOR
  From this directory enter TPSREP TPS.DBF. TPSPARM.DAT supplies the form
  title, division, memorandum number and page length. Advance the form
  seven positions with SPACE, then press RETURN. For the supplied example,
  enter report number 004417 twice, acknowledge memorandum 88-114 and
  confirm the cover sheet with Y. The report is written to TPSREPT.TXT.
