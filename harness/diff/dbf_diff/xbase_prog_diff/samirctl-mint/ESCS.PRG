SET TALK OFF
SET ALTERNATE TO ESCS.TXT
SET ALTERNATE ON
N=0
RUN AUTOTYPE -w 0 -p 0.1 esc s r e s u m e enter esc c q u i t enter
DO WHILE .T.
N=N+1
ENDDO
? "NOTREACHED"
