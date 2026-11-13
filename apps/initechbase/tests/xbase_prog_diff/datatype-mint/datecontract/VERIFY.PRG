SET TALK OFF
SET SAFETY OFF
ON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()
SET ALTERNATE TO DATECHK
SET ALTERNATE ON
SET DATE AMERICAN
SET CENTURY OFF
? "B01 ["+DTOC(CTOD(""))+"]"
? "B02 ["+DTOC(CTOD("  /  /  "))+"]"
SET CENTURY ON
? "B03 ["+DTOC(CTOD(""))+"]"
? "C01 ["+DTOC(CTOD("02/30/26"))+"]"
? "C02 ["+DTOC(CTOD("02/31/90"))+"]"
? "C03 ["+DTOC(CTOD("04/31/85"))+"]"
? "C04 ["+DTOC(CTOD("02/29/85"))+"]"
? "C05 ["+DTOC(CTOD("02/30/92"))+"]"
? "C06 ["+DTOC(CTOD("02/29/88"))+"]"
? "C07 ["+DTOC(CTOD("13/40/85"))+"]"
? "C08 ["+DTOC(CTOD("00/10/91"))+"]"
? "C09 ["+DTOC(CTOD("12/00/91"))+"]"
? "C10 ["+DTOC(CTOD("13/01/85"))+"]"
? "C11 ["+DTOC(CTOD("01/32/85"))+"]"
? "C12 ["+DTOC(CTOD("garbage"))+"]"
? "C13 ["+DTOC(CTOD("99/99/85"))+"]"
? "C14 ["+DTOC(CTOD("00/00/00"))+"]"
SET CENTURY OFF
? "C15 ["+DTOC(CTOD("02/30/92"))+"]"
SET DATE ANSI
? "C16 ["+DTOC(CTOD("08/05/85"))+"]"
SET DATE AMERICAN
USE E7
GO 1
REPLACE WHEN WITH CTOD("")
MBLANK = WHEN
MDATE = CTOD("09/07/85")
? "S01 ", TYPE("WHEN"), TYPE("MBLANK"), TYPE("MDATE")
? "S02 ["+DTOC(WHEN)+"] ["+DTOC(MBLANK)+"] ["+DTOC(MDATE)+"]"
? "S03 ", CTOD("08/05/85") < MDATE
? "S04 ", CTOD("08/05/85") > MDATE
SET EXACT OFF
? "Q001 ", CTOD("") = MDATE
? "Q002 ", MDATE = CTOD("")
? "Q003 ", CTOD("") <> MDATE
? "Q004 ", MDATE <> CTOD("")
? "Q005 ", CTOD("") < MDATE
? "Q006 ", MDATE < CTOD("")
? "Q007 ", CTOD("") > MDATE
? "Q008 ", MDATE > CTOD("")
? "Q009 ", CTOD("") <= MDATE
? "Q010 ", MDATE <= CTOD("")
? "Q011 ", CTOD("") >= MDATE
? "Q012 ", MDATE >= CTOD("")
? "Q013 ", MBLANK = MDATE
? "Q014 ", MDATE = MBLANK
? "Q015 ", MBLANK <> MDATE
? "Q016 ", MDATE <> MBLANK
? "Q017 ", MBLANK < MDATE
? "Q018 ", MDATE < MBLANK
? "Q019 ", MBLANK > MDATE
? "Q020 ", MDATE > MBLANK
? "Q021 ", MBLANK <= MDATE
? "Q022 ", MDATE <= MBLANK
? "Q023 ", MBLANK >= MDATE
? "Q024 ", MDATE >= MBLANK
? "Q025 ", WHEN = MDATE
? "Q026 ", MDATE = WHEN
? "Q027 ", WHEN <> MDATE
? "Q028 ", MDATE <> WHEN
? "Q029 ", WHEN < MDATE
? "Q030 ", MDATE < WHEN
? "Q031 ", WHEN > MDATE
? "Q032 ", MDATE > WHEN
? "Q033 ", WHEN <= MDATE
? "Q034 ", MDATE <= WHEN
? "Q035 ", WHEN >= MDATE
? "Q036 ", MDATE >= WHEN
? "Q037 ", CTOD("") < CTOD("")
? "Q038 ", CTOD("") = CTOD("")
? "Q039 ", CTOD("") <= CTOD("")
? "Q040 ", CTOD("") < MBLANK
? "Q041 ", CTOD("") = MBLANK
? "Q042 ", CTOD("") <= MBLANK
? "Q043 ", CTOD("") < WHEN
? "Q044 ", CTOD("") = WHEN
? "Q045 ", CTOD("") <= WHEN
? "Q046 ", MBLANK < CTOD("")
? "Q047 ", MBLANK = CTOD("")
? "Q048 ", MBLANK <= CTOD("")
? "Q049 ", MBLANK < MBLANK
? "Q050 ", MBLANK = MBLANK
? "Q051 ", MBLANK <= MBLANK
? "Q052 ", MBLANK < WHEN
? "Q053 ", MBLANK = WHEN
? "Q054 ", MBLANK <= WHEN
? "Q055 ", WHEN < CTOD("")
? "Q056 ", WHEN = CTOD("")
? "Q057 ", WHEN <= CTOD("")
? "Q058 ", WHEN < MBLANK
? "Q059 ", WHEN = MBLANK
? "Q060 ", WHEN <= MBLANK
? "Q061 ", WHEN < WHEN
? "Q062 ", WHEN = WHEN
? "Q063 ", WHEN <= WHEN
SET EXACT ON
? "Q064 ", CTOD("") = MDATE
? "Q065 ", MDATE = CTOD("")
? "Q066 ", CTOD("") <> MDATE
? "Q067 ", MDATE <> CTOD("")
? "Q068 ", CTOD("") < MDATE
? "Q069 ", MDATE < CTOD("")
? "Q070 ", CTOD("") > MDATE
? "Q071 ", MDATE > CTOD("")
? "Q072 ", CTOD("") <= MDATE
? "Q073 ", MDATE <= CTOD("")
? "Q074 ", CTOD("") >= MDATE
? "Q075 ", MDATE >= CTOD("")
? "Q076 ", MBLANK = MDATE
? "Q077 ", MDATE = MBLANK
? "Q078 ", MBLANK <> MDATE
? "Q079 ", MDATE <> MBLANK
? "Q080 ", MBLANK < MDATE
? "Q081 ", MDATE < MBLANK
? "Q082 ", MBLANK > MDATE
? "Q083 ", MDATE > MBLANK
? "Q084 ", MBLANK <= MDATE
? "Q085 ", MDATE <= MBLANK
? "Q086 ", MBLANK >= MDATE
? "Q087 ", MDATE >= MBLANK
? "Q088 ", WHEN = MDATE
? "Q089 ", MDATE = WHEN
? "Q090 ", WHEN <> MDATE
? "Q091 ", MDATE <> WHEN
? "Q092 ", WHEN < MDATE
? "Q093 ", MDATE < WHEN
? "Q094 ", WHEN > MDATE
? "Q095 ", MDATE > WHEN
? "Q096 ", WHEN <= MDATE
? "Q097 ", MDATE <= WHEN
? "Q098 ", WHEN >= MDATE
? "Q099 ", MDATE >= WHEN
? "Q100 ", CTOD("") < CTOD("")
? "Q101 ", CTOD("") = CTOD("")
? "Q102 ", CTOD("") <= CTOD("")
? "Q103 ", CTOD("") < MBLANK
? "Q104 ", CTOD("") = MBLANK
? "Q105 ", CTOD("") <= MBLANK
? "Q106 ", CTOD("") < WHEN
? "Q107 ", CTOD("") = WHEN
? "Q108 ", CTOD("") <= WHEN
? "Q109 ", MBLANK < CTOD("")
? "Q110 ", MBLANK = CTOD("")
? "Q111 ", MBLANK <= CTOD("")
? "Q112 ", MBLANK < MBLANK
? "Q113 ", MBLANK = MBLANK
? "Q114 ", MBLANK <= MBLANK
? "Q115 ", MBLANK < WHEN
? "Q116 ", MBLANK = WHEN
? "Q117 ", MBLANK <= WHEN
? "Q118 ", WHEN < CTOD("")
? "Q119 ", WHEN = CTOD("")
? "Q120 ", WHEN <= CTOD("")
? "Q121 ", WHEN < MBLANK
? "Q122 ", WHEN = MBLANK
? "Q123 ", WHEN <= MBLANK
? "Q124 ", WHEN < WHEN
? "Q125 ", WHEN = WHEN
? "Q126 ", WHEN <= WHEN
USE
SET ALTERNATE TO
QUIT
