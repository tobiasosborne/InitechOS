* MINT-ERRORS: 
? "[W2-BEGIN]"
SET HISTORY TO 0
SET HISTORY TO 20
SET DOHISTORY ON
    ? "indented"
	? "tab"
? "trail"   
? "amp" && trailing comment
NOTE a note
IF .T.
  ? "in if"
ELSE
  ? "no"
ENDIF
DO WHILE .F.
ENDDO
x = 1
DO CASE
CASE x = 1
? "c1"
ENDCASE
SET DOHISTORY OFF
? "[LIST]"
LIST HISTORY
? "[W2-END]"
RETURN
