SET ALTERNATE TO RESULT
SET ALTERNATE ON
ON ERROR ? "ERR",ERROR(),MESSAGE()
X = 1/0
? "NE", X <> X, X # X
? "LT", X < 5, X <= 5, X >= 5, X > -5
? "SELF", X > X, X < X, X <= X, X >= X
N = -1/0
? "NEG", N < 0, N = N, N
Z = 10^400
? "INF", Z > 5, Z = Z, Z <> Z, Z < 5
? "MIX", Z = X, Z > X
? "ABS", ABS(X), -X, MAX(X,1), MIN(X,1)
? "STR", STR(X,5), STR(X,12,3), LEN(STR(X,12,3))
? "VAL", VAL("1") / 0
STORE 0 TO Q
? "DIVQ", 5 / Q
? "MOD", MOD(5,0)
? "TYPE", TYPE("1/0")
? "IIF", IIF(X = X, "EQ", "NE")
CLOSE DATABASES
SET ALTERNATE OFF
SET ALTERNATE TO
CLEAR
RETURN
