* CLEANUP.PRG -- Initech Accounting System: Remove Paid Invoices
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  Run after the month-end programs and the
* month's copy of the files.  Marks the paid invoices for deletion, lists
* them, and removes them only when the supervisor confirms.
DELETE FOR PAID
? 'PAID INVOICES MARKED FOR REMOVAL:'
LIST INVNO, CUST FOR DELETED()
WAIT 'Remove them from the ledger (Y/N)? ' TO MANSWER
IF UPPER(MANSWER) = 'Y'
  PACK
  ? 'REMOVED.  INVOICES IN THE LEDGER:' + STR(RECCOUNT(), 4)
ELSE
  RECALL ALL
  ? 'NOTHING REMOVED'
ENDIF
RETURN
