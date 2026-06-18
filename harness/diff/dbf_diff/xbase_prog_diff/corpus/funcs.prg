* funcs.prg -- built-in functions: STR (tie->+inf), UPPER/LOWER/TRIM/SUBSTR/LEN,
*              VAL, CTOD/DTOC/YEAR, IIF, MOD/INT.
* Corpus citations:
*   ../dbase3-decomp/specs/functions/string-functions.md (UPPER/LOWER/TRIM/SUBSTR/LEN/STR/VAL)
*   ../dbase3-decomp/specs/functions/numeric-and-date-functions.md (MOD/INT; CTOD/DTOC/YEAR)
*   ../dbase3-decomp/re/mint-results-004 (rounding ties toward +inf:
*       STR(2.5,2,0)=' 3', STR(-2.5,2,0)='-2') -- folded into plan sec 3.3.
* DTOC of CTOD('12/31/99') renders MM/DD/YY (SET DATE AMERICAN default; plan sec 3.3).
? STR(2.5,2,0)
? STR(-2.5,2,0)
? UPPER('abc')
? LOWER('XYZ')
? '[' + TRIM('hi   ') + ']'
? SUBSTR('HELLO',2,3)
? LEN('hello')
? VAL('42')
? DTOC(CTOD('12/31/99'))
? YEAR(CTOD('12/31/99'))
? IIF(1 = 1, 'YES', 'NO')
? MOD(7, 3)
? INT(3.9)
