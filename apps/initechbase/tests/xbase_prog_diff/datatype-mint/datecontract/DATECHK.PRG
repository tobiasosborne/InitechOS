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
? "Q037 ", CTOD("") = CTOD("")
? "Q038 ", CTOD("") <> CTOD("")
? "Q039 ", CTOD("") < CTOD("")
? "Q040 ", CTOD("") > CTOD("")
? "Q041 ", CTOD("") <= CTOD("")
? "Q042 ", CTOD("") >= CTOD("")
? "Q043 ", CTOD("") = MBLANK
? "Q044 ", CTOD("") <> MBLANK
? "Q045 ", CTOD("") < MBLANK
? "Q046 ", CTOD("") > MBLANK
? "Q047 ", CTOD("") <= MBLANK
? "Q048 ", CTOD("") >= MBLANK
? "Q049 ", CTOD("") = WHEN
? "Q050 ", CTOD("") <> WHEN
? "Q051 ", CTOD("") < WHEN
? "Q052 ", CTOD("") > WHEN
? "Q053 ", CTOD("") <= WHEN
? "Q054 ", CTOD("") >= WHEN
? "Q055 ", MBLANK = CTOD("")
? "Q056 ", MBLANK <> CTOD("")
? "Q057 ", MBLANK < CTOD("")
? "Q058 ", MBLANK > CTOD("")
? "Q059 ", MBLANK <= CTOD("")
? "Q060 ", MBLANK >= CTOD("")
? "Q061 ", MBLANK = MBLANK
? "Q062 ", MBLANK <> MBLANK
? "Q063 ", MBLANK < MBLANK
? "Q064 ", MBLANK > MBLANK
? "Q065 ", MBLANK <= MBLANK
? "Q066 ", MBLANK >= MBLANK
? "Q067 ", MBLANK = WHEN
? "Q068 ", MBLANK <> WHEN
? "Q069 ", MBLANK < WHEN
? "Q070 ", MBLANK > WHEN
? "Q071 ", MBLANK <= WHEN
? "Q072 ", MBLANK >= WHEN
? "Q073 ", WHEN = CTOD("")
? "Q074 ", WHEN <> CTOD("")
? "Q075 ", WHEN < CTOD("")
? "Q076 ", WHEN > CTOD("")
? "Q077 ", WHEN <= CTOD("")
? "Q078 ", WHEN >= CTOD("")
? "Q079 ", WHEN = MBLANK
? "Q080 ", WHEN <> MBLANK
? "Q081 ", WHEN < MBLANK
? "Q082 ", WHEN > MBLANK
? "Q083 ", WHEN <= MBLANK
? "Q084 ", WHEN >= MBLANK
? "Q085 ", WHEN = WHEN
? "Q086 ", WHEN <> WHEN
? "Q087 ", WHEN < WHEN
? "Q088 ", WHEN > WHEN
? "Q089 ", WHEN <= WHEN
? "Q090 ", WHEN >= WHEN
SET EXACT ON
? "Q091 ", CTOD("") = MDATE
? "Q092 ", MDATE = CTOD("")
? "Q093 ", CTOD("") <> MDATE
? "Q094 ", MDATE <> CTOD("")
? "Q095 ", CTOD("") < MDATE
? "Q096 ", MDATE < CTOD("")
? "Q097 ", CTOD("") > MDATE
? "Q098 ", MDATE > CTOD("")
? "Q099 ", CTOD("") <= MDATE
? "Q100 ", MDATE <= CTOD("")
? "Q101 ", CTOD("") >= MDATE
? "Q102 ", MDATE >= CTOD("")
? "Q103 ", MBLANK = MDATE
? "Q104 ", MDATE = MBLANK
? "Q105 ", MBLANK <> MDATE
? "Q106 ", MDATE <> MBLANK
? "Q107 ", MBLANK < MDATE
? "Q108 ", MDATE < MBLANK
? "Q109 ", MBLANK > MDATE
? "Q110 ", MDATE > MBLANK
? "Q111 ", MBLANK <= MDATE
? "Q112 ", MDATE <= MBLANK
? "Q113 ", MBLANK >= MDATE
? "Q114 ", MDATE >= MBLANK
? "Q115 ", WHEN = MDATE
? "Q116 ", MDATE = WHEN
? "Q117 ", WHEN <> MDATE
? "Q118 ", MDATE <> WHEN
? "Q119 ", WHEN < MDATE
? "Q120 ", MDATE < WHEN
? "Q121 ", WHEN > MDATE
? "Q122 ", MDATE > WHEN
? "Q123 ", WHEN <= MDATE
? "Q124 ", MDATE <= WHEN
? "Q125 ", WHEN >= MDATE
? "Q126 ", MDATE >= WHEN
? "Q127 ", CTOD("") = CTOD("")
? "Q128 ", CTOD("") <> CTOD("")
? "Q129 ", CTOD("") < CTOD("")
? "Q130 ", CTOD("") > CTOD("")
? "Q131 ", CTOD("") <= CTOD("")
? "Q132 ", CTOD("") >= CTOD("")
? "Q133 ", CTOD("") = MBLANK
? "Q134 ", CTOD("") <> MBLANK
? "Q135 ", CTOD("") < MBLANK
? "Q136 ", CTOD("") > MBLANK
? "Q137 ", CTOD("") <= MBLANK
? "Q138 ", CTOD("") >= MBLANK
? "Q139 ", CTOD("") = WHEN
? "Q140 ", CTOD("") <> WHEN
? "Q141 ", CTOD("") < WHEN
? "Q142 ", CTOD("") > WHEN
? "Q143 ", CTOD("") <= WHEN
? "Q144 ", CTOD("") >= WHEN
? "Q145 ", MBLANK = CTOD("")
? "Q146 ", MBLANK <> CTOD("")
? "Q147 ", MBLANK < CTOD("")
? "Q148 ", MBLANK > CTOD("")
? "Q149 ", MBLANK <= CTOD("")
? "Q150 ", MBLANK >= CTOD("")
? "Q151 ", MBLANK = MBLANK
? "Q152 ", MBLANK <> MBLANK
? "Q153 ", MBLANK < MBLANK
? "Q154 ", MBLANK > MBLANK
? "Q155 ", MBLANK <= MBLANK
? "Q156 ", MBLANK >= MBLANK
? "Q157 ", MBLANK = WHEN
? "Q158 ", MBLANK <> WHEN
? "Q159 ", MBLANK < WHEN
? "Q160 ", MBLANK > WHEN
? "Q161 ", MBLANK <= WHEN
? "Q162 ", MBLANK >= WHEN
? "Q163 ", WHEN = CTOD("")
? "Q164 ", WHEN <> CTOD("")
? "Q165 ", WHEN < CTOD("")
? "Q166 ", WHEN > CTOD("")
? "Q167 ", WHEN <= CTOD("")
? "Q168 ", WHEN >= CTOD("")
? "Q169 ", WHEN = MBLANK
? "Q170 ", WHEN <> MBLANK
? "Q171 ", WHEN < MBLANK
? "Q172 ", WHEN > MBLANK
? "Q173 ", WHEN <= MBLANK
? "Q174 ", WHEN >= MBLANK
? "Q175 ", WHEN = WHEN
? "Q176 ", WHEN <> WHEN
? "Q177 ", WHEN < WHEN
? "Q178 ", WHEN > WHEN
? "Q179 ", WHEN <= WHEN
? "Q180 ", WHEN >= WHEN
USE
SET ALTERNATE TO
QUIT
