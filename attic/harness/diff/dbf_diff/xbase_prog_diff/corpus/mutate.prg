* mutate.prg -- mutation verbs writing a RESULT .dbf (the driver opens a /tmp COPY
*               of PARTS writable; NEVER a corpus golden -- wave-9 lesson).
* Table PARTS (writable copy): CODE C(3), QTY N(5,0), OK L(1); seeded 4 rows:
*     1 AAA 100 T / 2 BBB 200 F / 3 CCC 100 T / 4 DDD 300 T
* Corpus citation: ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md
*   (REPLACE <field> WITH <expr> [, ...]; APPEND BLANK; DELETE; assignment coercion),
*   ../dbase3-decomp/specs/functions/system-and-database-functions.md (RECNO/DELETED).
* The RESULT .dbf is diffed by dbf_ref.py (--records) as well as stdout (Tier-0 +
* the independent-reader leg).
GO 1
REPLACE CODE WITH 'ZZ'
? CODE
APPEND BLANK
REPLACE CODE WITH 'XX', QTY WITH 999
? RECNO()
? CODE
? QTY
GO 2
DELETE
? DELETED()
GO 1
? CODE
