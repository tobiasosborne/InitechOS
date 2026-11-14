INITECH SYSTEMS CORPORATION
APPLICATION EXAMPLES

The APPS directory contains the desktop applications. SAMPLES contains
the database and report examples. TURBO contains the Pascal compiler.

INITECHBASE
  Enter CD \APPS, then INITBASE. At the dot prompt enter:
    USE \SAMPLES\CLIENTS
    LIST
    USE \SAMPLES\PARTS
    LIST
    USE \SAMPLES\INVOICE
    DO \SAMPLES\Y2KACCT
  QUIT returns to InitechDOS. The accounting report uses two-digit years.
  Inspect the reporting date and aging columns before acting on a balance.

TURBO INITECH
  Enter CD \TURBO before using the compiler.
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
  Enter CD \SAMPLES, then TPSREP TPS.DBF. TPSPARM.DAT supplies the form
  title, division, memorandum number and page length. Advance the form
  seven positions with SPACE, then press RETURN. For the supplied example,
  enter report number 004417 twice, acknowledge memorandum 88-114 and
  confirm the cover sheet with Y. The report is written to TPSREPT.TXT.
