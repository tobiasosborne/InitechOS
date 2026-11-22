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
  Enter CD \TURBO. To compile and run the introductory program, enter:
    TURBO HELLO.PAS
    TURBOOUT.COM
  The compiler writes TURBOOUT.EXE and also writes TURBOOUT.COM when
  the program fits in that form.

  Enter TURBO by itself to open the editor. Press F3, enter DEMO.PAS,
  and press F9. Error 85 marks the missing semicolon between the two
  WRITELN statements. Alt-X returns to InitechDOS.

  TURBO.PAS contains the complete compiler source. Enter TURBO TURBO.PAS
  to compile it. Keep the original TURBO.COM.

TPS REPORT GENERATOR
  Enter CD \SAMPLES, then TPSREP TPS.DBF. TPSPARM.DAT supplies the form
  title, division, memorandum number and page length. Advance the form
  seven positions with SPACE, then press RETURN. For the supplied example,
  enter report number 004417 twice, acknowledge memorandum 88-114 and
  confirm the cover sheet with Y. The report is written to TPSREPT.TXT.
