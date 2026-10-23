* LATEFEE.PRG -- Initech Accounting System: Late Fee of One Invoice
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE, on the invoice.  The procedure FEE
* computes the late fee of an amount for a number of days past due: one
* per cent for each full thirty days.
MDAYS = 75
DO FEE WITH AMOUNT, MDAYS
? INVNO, STR(AMOUNT, 10, 2), STR(MFEE, 8, 2)
RETURN

PROCEDURE FEE
PARAMETERS PAMOUNT, PDAYS
PUBLIC MFEE
MFEE = ROUND(PAMOUNT * INT(PDAYS / 30) / 100, 2)
RETURN
