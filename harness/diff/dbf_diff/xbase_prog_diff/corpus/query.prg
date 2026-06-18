* query.prg -- read-only display + navigation over a USEd table (PARTS, provided
*              by the driver as a /tmp copy of corpus/parts.schema).
* Table PARTS: CODE C(3), QTY N(5,0), OK L(1); 4 rows the driver seeds:
*     1 AAA 100 T / 2 BBB 200 F / 3 CCC 100 T / 4 DDD 300 T
* Corpus citation: ../dbase3-decomp/specs/commands/navigation-query-display.md
*   sec on LIST (recno column + fields), GO/GOTO, LOCATE FOR / FOUND(), RECNO(),
*   ? expression display; FOR visits every record (no early stop).
* LIST renders: recno right-justified width 8, space, then each field in display
*   width (CODE left-just 3, QTY right of its column, OK as T/F).
USE PARTS
? RECNO()
LIST
LOCATE FOR QTY = 200
? RECNO()
? FOUND()
GO 3
? CODE
LIST FOR QTY = 100 OFF
