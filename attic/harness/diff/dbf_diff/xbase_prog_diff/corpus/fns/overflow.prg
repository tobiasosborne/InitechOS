? "[FNS-BEGIN]"
ON ERROR ? "TRAPPED", ERROR()
? 1/0
N=1/0
? N, STR(N,6,2), TYPE("N")
? 5+N
ON ERROR
? "[FNS-END]"
RETURN
