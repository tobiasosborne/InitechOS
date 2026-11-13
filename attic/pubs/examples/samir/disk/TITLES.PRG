* TITLES.PRG -- Centred titles and rules
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 5.  Centres two title lines in 50 characters and
* draws a rule under them.
MWIDTH = 50
MTITLE = 'INITECH SYSTEMS CORPORATION'
? SPACE(INT((MWIDTH - LEN(MTITLE)) / 2)) + MTITLE
MTITLE = 'ACCOUNTS RECEIVABLE, ' + UPPER(CMONTH(DATE())) + ' ' + LTRIM(STR(YEAR(DATE())))
? SPACE(INT((MWIDTH - LEN(MTITLE)) / 2)) + MTITLE
? REPLICATE('=', MWIDTH)
RETURN
