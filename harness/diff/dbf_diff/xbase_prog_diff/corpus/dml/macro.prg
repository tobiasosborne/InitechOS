?? "[DML-BEGIN]"
USE BASE
N=1000
CMD="? N+5"
&CMD
WHO="N"
? &WHO
SUFFIX="BASE"
USE &SUFFIX..DBF
? RECCOUNT()
WORD="HELLO"
? 'value &WORD.'

?? "[DML-END]"
