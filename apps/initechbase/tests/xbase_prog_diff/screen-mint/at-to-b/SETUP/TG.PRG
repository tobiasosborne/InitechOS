CLEAR
ON ERROR LG=LG+STR(N,3)+":"+STR(ERROR(),3)
LG=""
N=1
@ 25,0 TO 26,5
N=2
@ 0,80 TO 1,85
N=3
@ 0,0 TO 25,5
N=4
@ 1,1 TO 5,80
N=5
@ -1,0 TO 2,2
N=6
@ 0,-1 TO 2,2
N=7
@ "a",0 TO 1,1
N=8
@ 0,0 TO "b",1
N=9
@ 0,0 TO
N=10
@ 0,0 TO 1
N=11
@ 0,0 TO 1,
N=12
@ 0,0 TO 1,1 FOO
N=13
@ 0,0 TO 1,1 DOUBLE FOO
N=14
@ 0 TO 1,1
N=15
@ 0,1,2 TO 3,4
N=16
@ 0,0 TO 1,2,3
N=17
@ 0,0 TO 1,.T.
N=18
@ .F.,0 TO 1,1
N=19
@ 3,0 TO -1,5
N=20
@ 3,5 TO 3,-1
N=21
@ ,0 TO 1,1
N=22
@ 0, TO 1,1
N=23
@ 0,0 TO ,1
N=24
@ NOSUCH,0 TO 1,1
N=25
@ 0,0 TO 1,NOSUCH
ON ERROR
CLEAR
? LEFT(LG,70)
? SUBSTR(LG,71,70)
? SUBSTR(LG,141,70)

DO WHILE INKEY()=0
ENDDO
