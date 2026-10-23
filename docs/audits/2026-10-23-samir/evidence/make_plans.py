from pathlib import Path
import json
E=Path(__file__).resolve().parent

def plan(name,groups,**kw):
 acts=[]
 for label,commands in groups:
  acts.extend(commands);acts.append(['shot',name+'-'+label])
 (E/(name+'.json')).write_text(json.dumps(dict(name=name,actions=acts,**kw),indent=2))
plan('regress',[
('list',['USE CLIENTS','LIST','GO TOP','DELETE','DISPLAY','RECALL','SET FILTER TO BAL>1000','LIST','SET FILTER TO']),
('close',['CLOSE INDEX','? RECCOUNT()','CLOSE FORMAT','? RECCOUNT()','USE CLIENTS ALIAS ONE','? ONE->NAME','? TYPE("BAL")','? 1/0']),
('date',['? DATE()','SET CENTURY ON','? DATE()','? CTOD("")','? CTOD("02/30/92")','? CTOD("02/29/00")','? CTOD("02/29/1900")']),
('functions',['? EXP(1),LOG(1),SQRT(4)','? TIME(),RECSIZE(),DISKSPACE()','? VERSION(),OS()','? PCOL(),PROW(),ROW(),COL()','MVAR=123','SAVE TO MEMSAVE','CLEAR MEMORY','RESTORE FROM MEMSAVE','? MVAR']),
('index',['USE CLIENTS','INDEX ON NAME TO TESTIDX','GO TOP','LIST','REINDEX','SEEK "WADDAMS"','? FOUND(),RECNO()','APPEND FROM CLIENTS.DBF','? RECCOUNT()']),
('file',['SORT TO SORTED ON NAME','TOTAL ON NAME TO TOTALLED','COPY FILE HELLO.TXT TO COPY.TXT','TYPE COPY.TXT','RENAME COPY.TXT TO RENAMED.TXT','ERASE RENAMED.TXT','? FILE("RENAMED.TXT")']),
('misc',['CMD="? 88"','&CMD','SET FUNCTION 1 TO "LIST"','SET PROCEDURE TO FLOW','SET DEFAULT TO A:','SET PATH TO A:\\','SET ESCAPE ON','SET CONFIRM ON','SET BELL OFF']),
('settings',['SET CONSOLE OFF','? "HIDDEN"','SET CONSOLE ON','? "VISIBLE"','SET HEADING OFF','LIST NEXT 2','SET HEADING ON','SET ALTERNATE TO OUTPUT','SET ALTERNATE ON','? "AUDIT OUTPUT"','SET ALTERNATE OFF','CLOSE ALTERNATE']),
('zap-no',['USE EMPTY','APPEND BLANK','SET SAFETY ON','ZAP','N','? RECCOUNT()']),
('zap-yes',['ZAP','Y','? RECCOUNT()','CLOSE ALL','QUIT',['pause',1]])
])
plan('data',[
('wide',['USE EDGE','? LEN(WIDE),PRECISE,SMALL,WHEN,OK','? MEMO,LEN(MEMO),TYPE("MEMO")','REPLACE SMALL WITH 99999','DISPLAY ID,SMALL','REPLACE WHEN WITH CTOD("02/30/92")','DISPLAY ID,WHEN','REPLACE MEMO WITH "new memo"','? MEMO']),
('overflow',['GO 2','REPLACE PRECISE WITH 0.98765432101234567','REPLACE NAME WITH "abcdefghijklmnopqrst"','REPLACE OK WITH "T"','DISPLAY ID,NAME,PRECISE,OK','USE','USE EDGE','DISPLAY ID,NAME,PRECISE,SMALL,WHEN,OK']),
('bounds',['GO 0','? RECNO(),BOF(),EOF()','GO 999','? RECNO(),BOF(),EOF()','GO TOP','SKIP -1','? RECNO(),BOF(),EOF()','GO BOTTOM','SKIP','? RECNO(),BOF(),EOF()']),
('search',['USE CLIENTS','SEEK "PESTON"','FIND PESTON','CONTINUE','LOCATE FOR BAL>1000','? FOUND(),RECNO()','CONTINUE','? FOUND(),RECNO()','CONTINUE','? FOUND(),RECNO()']),
('scope',['GO TOP','LIST NEXT 2 NAME,BAL OFF','GO 2','LIST REST NAME','GO TOP','LIST NAME WHILE BAL>0','LIST FOR BAL>0','LIST NAME TO PRINT','SUM BAL','AVERAGE BAL','COUNT']),
('copy',['COPY TO BACKUP','COPY STRUCTURE TO STRUCT','COPY STRUCTURE EXTENDED TO EXTSTR','JOIN WITH CLIENTS TO JOINED FOR .T.','UPDATE ON NAME FROM CLIENTS REPLACE BAL WITH BAL','CREATE REPORT NEWREP','MODIFY REPORT STAFFRPT','CREATE LABEL NEWLAB']),
('indexes',['USE CLIENTS','INDEX ON NAME TO BYNAME','INDEX ON BAL TO BYBAL','SET INDEX TO BYNAME,BYBAL','SET ORDER TO 1','GO TOP','LIST','SET ORDER TO 2','GO TOP','LIST']),
('index-mutate',['SET ORDER TO 1','GO 2','REPLACE NAME WITH "AAAA"','SEEK "AAAA"','? FOUND(),RECNO(),NAME','SEEK "WADDAMS"','? FOUND(),RECNO()','APPEND BLANK','REPLACE NAME WITH "ZZZZ",BAL WITH 123','SEEK "ZZZZ"','? FOUND(),RECNO()']),
('index-delete',['GO 1','DELETE','SET DELETED ON','GO TOP','LIST','SET DELETED OFF','RECALL ALL','PACK','REINDEX','GO TOP','LIST']),
('unique',['CLOSE INDEX','USE EDGE','INDEX ON NAME TO DUP UNIQUE','GO TOP','LIST ID,NAME','SET UNIQUE ON','INDEX ON NAME TO DUP2','SET UNIQUE OFF','INDEX ON OK TO LOGIC','INDEX ON WHEN TO DATED','INDEX ON WIDE TO TOO_LONG']),
('badindex',['SET INDEX TO BAD','? RECCOUNT()','USE EMPTY','INDEX ON NAME TO EMPTYIDX','SEEK "NONE"','? FOUND(),BOF(),EOF(),RECNO()','SET ORDER TO 9','CLOSE ALL','QUIT'])
])
plan('programs',[
('flow',['DO USEPRG','DO FLOW','? RESULT,I','DO LONG','? SUMX','DO BADLOOP']),
('traps',['ON ERROR STORE ERROR() TO ERRNUM','? MISSING','? ERRNUM,MESSAGE()','ON ERROR','DO ONERR','? ERRNO','RETRY','RETURN TO MASTER']),
('stray',['ENDIF','ELSE','EXIT','? "AFTER EXIT"']),
],boot='tracer_boot.img',route='dos')
plan('programmore',[
('form',['DO FORM','DO TEXT','@ 2,3 SAY "HELLO"','WAIT "Press a key" TO KEYVAR',['key','x'],'? KEYVAR','ACCEPT "Name:" TO USERNAME','Samir','? USERNAME','INPUT "Amount:" TO AMOUNT','12.5','? AMOUNT']),
('settings',['SET ECHO ON','DO FLOW','SET ECHO OFF','SET TALK OFF','DELETE ALL','SET TALK ON','RECALL ALL','SET FIXED ON','? 3.5','SET FIXED OFF','SET MARGIN TO 5','SET PRINT ON','SET PRINT OFF','SET DEVICE TO SCREEN']),
('newcommands',['SET RELATION TO NAME INTO TWO','SET ORDER TO 0','MODIFY COMMAND FLOW','CREATE SCREEN SCREEN','MODIFY FILE HELLO.TXT','SET KEY TO FLOW','ON KEY LABEL F1 ? "KEY"','ON ESCAPE ? "ESC"','SET PROCEDURE TO FLOW','DO CHILD WITH 9']),
('run',['RUN ECHO FROMDATABASE','! ECHO BANGWORKED','RUN NOSUCH','DO DOSRUN','? "STILL LIVE"']),
('quitprogram',['DO QUITPRG',['pause',2]])
],boot='tracer_boot.img',route='dos')
plan('expressions',[
('math',['? -2^2,2^3^2,2+3*4,MOD(-5,3),MOD(5,-3)','? ROUND(-1.5,0),ROUND(1.5,0),ROUND(1234,-2)','? VAL("  -12.34xyz"),STR(1.999,5,2),STR(9999,3)','? ABS(-3),INT(-1.9),MAX(2,3),MIN(2,3)','? SQRT(-1)','? LOG(0)','? EXP(1000)']),
('strings',['? AT("a","banana"),LEFT("abc",2),RIGHT("abc",2)','? SUBSTR("abc",0,2)','? SUBSTR("abc",-1,2)','? LEN(SPACE(254))','? SPACE(255)','? SPACE(-1)','? REPLICATE("x",255)','? STUFF("abcd",2,2,"XY")']),
('compare',['SET EXACT OFF','? "abcdef"="abc","abc"="abcdef","abc"="",""="abc"','SET EXACT ON','? "abc"="abc ","abc"="ABC"','? "abc"=="abc"','? "b"$"abc",.NOT. .T. .OR. .T.','? "a"+1','? .T.+1','? CTOD("01/01/00")+1']),
('types',['? TYPE("NONSENSE"),TYPE("1+2"),TYPE(".T.")','? IIF(.T.,1,MISSING)','? CHR(256)','? ASC("")','? LEN(TRIM(" abc ")),LOWER(UPPER("AbC"))','? ISALPHA("A"),ISLOWER("a"),ISUPPER("A")','? TRANSFORM(123.4,"999.99")']),
('dates',['SET DATE GERMAN','? DTOC(CTOD("31.12.92"))','SET DATE ANSI','? DTOC(CTOD("92.12.31"))','SET DATE JAPAN','? DTOC(CTOD("92/12/31"))','SET DATE BRITISH','? DTOC(CTOD("31/12/92"))','SET DATE ITALIAN','? DTOC(CTOD("31-12-92"))','SET DATE USA','? DTOC(CTOD("12-31-92"))']),
('decimals',['SET DATE AMERICAN','SET CENTURY ON','? CTOD("02/29/2000"),CTOD("02/29/1900")','? YEAR(CTOD("12/31/00")),DOW(CTOD("01/01/1900"))','? CMONTH(CTOD("01/01/92")),CDOW(CTOD("01/01/92"))','SET DECIMALS TO 15','? 1/3','SET DECIMALS TO 16','SET DECIMALS TO -1','? ERROR(),MESSAGE()','QUIT'])
])
plan('reports',[
('plain',['USE STAFF','REPORT FORM STAFFRPT PLAIN','REPORT FORM STAFFRPT TO FILE PLAIN.TXT PLAIN']),
('scope',['GO 3','REPORT FORM STAFFRPT NEXT 2 TO FILE NEXT.TXT','? RECNO(),EOF()','GO 3','REPORT FORM STAFFRPT WHILE EMPNO<"1005" TO FILE WHILE.TXT','? RECNO(),EOF()']),
('summary',['USE STAFF INDEX STAFFDPT','REPORT FORM STAFFRPT SUMMARY HEADING "MARCH 1992" TO FILE SUMMARY.TXT','REPORT FORM STAFFRPT FOR .F. TO FILE NONE.TXT','REPORT FORM STAFFRPT NOEJECT TO FILE NOEJECT.TXT']),
('label',['LABEL FORM STAFFLBL TO FILE LABELS.TXT','LABEL FORM STAFFLBL SAMPLE','REPORT FORM STAFFRPT TO PRINT','REPORT FORM MISSING','LABEL FORM MISSING','CLOSE ALL','QUIT'])
])
plan('io',[
('copy',['COPY FILE HELLO.TXT TO COPIED.TXT','TYPE HELLO.TXT','TYPE COPIED.TXT','COPY FILE CLIENTS.DBF TO CLONE.DBF','USE CLONE','LIST','USE']),
('overwrite',['COPY FILE HELLO.TXT TO COPIED.TXT','SET SAFETY ON','SORT TO SORTED ON NAME','USE CLIENTS','SORT TO EXIST ON NAME','SORT TO EXIST ON BAL','USE EXIST','LIST']),
('append',['USE CLIENTS','APPEND FROM CLONE','? RECCOUNT()','LIST','USE','USE CLIENTS','? RECCOUNT()','GO TOP','DELETE','PACK','? RECCOUNT()','LIST']),
('zap',['SET SAFETY OFF','ZAP','? RECCOUNT()','USE','USE CLIENTS','LIST','COPY FILE HELLO.TXT TO HELLO.TXT','TYPE HELLO.TXT','QUIT'])
])
plan('exit-two', [('before',['SELECT 1','USE CLIENTS ALIAS ONE','SELECT 2','USE CLIENTS ALIAS TWO','? RECCOUNT()']),('after',['QUIT',['pause',5],['key','ctrl-c'],['key','ret'],['pause',2]])])
plan('dos-status',[('before',['USE CLIENTS','LIST','SET STATUS ON']),('type',['? 2+2']),('off',['SET STATUS OFF','? 2+2','QUIT'])],boot='tracer_boot.img',route='dos')
plan('full',[
('before',['USE CLIENTS','LIST']),
('append',['APPEND BLANK']*18+['? RECCOUNT()']),
('reopen',['USE','USE CLIENTS','? RECCOUNT()','LIST']),
('index',['INDEX ON NAME TO FULLIDX','? ERROR(),MESSAGE()','COPY FILE CLIENTS.DBF TO NEWCOPY.DBF','SORT TO SORTED ON NAME','GO TOP','DELETE','PACK','? RECCOUNT()','USE','USE CLIENTS','LIST','QUIT'])
],data='full-fixture.img')
plan('index-fresh',[
('empty',['USE EMPTY','INDEX ON NAME TO ZERO','? RECCOUNT(),RECNO(),EOF(),BOF()','SEEK "NONE"','? FOUND()','CLOSE INDEX']),
('types',['USE EDGE','INDEX ON NAME TO NAMES','SEEK "alpha"','? FOUND(),RECNO()','SKIP','? RECNO()','INDEX ON WHEN TO DATES','GO TOP','LIST ID,WHEN','INDEX ON OK TO LOGICAL','INDEX ON WIDE TO BIGKEY']),
('expressions',['INDEX ON UPPER(NAME) TO CAPS','SEEK "ALPHA"','? FOUND(),RECNO()','INDEX ON ID+1 TO NUMBER','GO TOP','LIST ID','INDEX ON LEFT(WIDE,100) TO KEY100','INDEX ON LEFT(WIDE,101) TO KEY101','SET INDEX TO BAD.NDX','CLOSE ALL','QUIT'])
])
plan('pack-index',[
('before',['USE CLIENTS','INDEX ON NAME TO NAMES','GO TOP','LIST','PACK']),
('after',['GO TOP','LIST','REINDEX','GO TOP','LIST','CLOSE INDEX','GO TOP','LIST']),
('reopen',['USE','USE CLIENTS INDEX NAMES','GO TOP','LIST','QUIT'])
])
plan('memo',[
('write',['USE EDGE','? MEMO','REPLACE MEMO WITH "changed memo"','? MEMO,LEN(MEMO)','GO 2','GO 1','? MEMO']),
('reopen',['USE','USE EDGE','? MEMO,LEN(MEMO)','REPLACE MEMO WITH "again"','? MEMO','USE','USE EDGE','? MEMO','QUIT'])
])
plan('missing',[
('inspection',['USE CLIENTS','DISPLAY STATUS','DISPLAY MEMORY','DISPLAY STRUCTURE','SET','HELP','ASSIST','MODIFY STRUCTURE']),
('abbrev',['DISP','REPL NAME WITH "TEST"','APPE BLANK','LOCA FOR BAL>1000','SELE 2','INDE ON NAME TO TEST','STOR 5 TO X','? X']),
('functions',['? EXP(1)','? LOG(1)','? SQRT(4)','? TIME()','? RECSIZE()','? DISKSPACE()','? VERSION()','? OS()','? PCOL()','? PROW()','? ROW()','? COL()']),
('newmissing',['CREATE REPORT NEWREP','MODIFY REPORT STAFFRPT','CREATE LABEL NEWLAB','AVERAGE BAL','LIST NAME TO PRINT','COPY STRUCTURE TO STRUCT','JOIN WITH CLIENTS TO JOINED FOR .T.','UPDATE ON NAME FROM CLIENTS REPLACE BAL WITH BAL','QUIT'])
])
plan('escape-loop',[('running',['DO LOOP',['pause',2]]),('escape',[['key','esc'],['pause',2],['key','ctrl-c'],['pause',2],'? "RECOVERED"',['pause',2]])])
plan('recursion',[('recursion',['DO RECURSE',['pause',2],'? "RECOVERED"',['pause',1]])])
plan('keys',[
('history',['? "FIRST"',['key','up'],['key','ret'],'LIST HISTORY','SET HISTORY ON']),
('edit',[['type','? 123',False],['key','left'],['key','backspace'],['type','4',False],['key','ret'],['type','? 56',False],['key','home'],['key','delete'],['key','end'],['key','ret']]),
('254',['? LEN("'+'x'*245+'")']),
('255',['? LEN("'+'y'*246+'")','? "AFTER LONG"']),
('escape',[['type','? "discard"',False],['key','esc'],['key','ret'],'? "AFTER ESC"','QUIT'])
])
plan('run-present',[
('run',['USE CLIENTS','GO 2','RUN ECHO HELLOFROMDOS','? RECNO(),RECCOUNT()','! ECHO HELLOBANG','RUN ECHO SAVED > RUNOUT.TXT','TYPE RUNOUT.TXT','RUN NOSUCH','? "AFTER BAD COMMAND"']),
('program',['DO DOSRUN','? RECNO(),RECCOUNT()','QUIT'])
],data='run-fixture.img')
plan('transform',[
('decimal',['? TRANSFORM(123.4,"999.99")','? TRANSFORM(12.34,"999.99")','? TRANSFORM(1234.5,"9,999.99")','? TRANSFORM(-1234.56,"99,999.99")','? STR(123.4,6,2)','? TRANSFORM(0.12,"9.99")','? TRANSFORM(9.99,"9.99")','QUIT'])
])
plan('desktop-status',[
('status',['USE CLIENTS','SET STATUS ON',['pause',2]]),('typing',['? 2+2',['pause',1]]),('off',['SET STATUS OFF','QUIT'])
])
plan('errors',[
('runtime',['? MISSING','? ERROR(),MESSAGE()','? 1+"x"','? ERROR(),MESSAGE()','? NOSUCH()','? ERROR(),MESSAGE()']),
('files',['USE MISSING','? ERROR(),MESSAGE()','USE HELLO.TXT','? ERROR(),MESSAGE()','USE CLIENTS','GO 0','? ERROR(),MESSAGE()','SET INDEX TO BAD.NDX','? ERROR(),MESSAGE()']),
('type',['REPLACE BAL WITH "x"','? ERROR(),MESSAGE()','LOCATE FOR 1','? ERROR(),MESSAGE()','? (1+2','? ERROR(),MESSAGE()','? "unterminated','? ERROR(),MESSAGE()']),
('trap',['ON ERROR STORE ERROR() TO LASTERR','? MISSING','? LASTERR,ERROR(),MESSAGE()','ON ERROR','QUIT'])
])
plan('date-index',[
('blank',['USE EDGE','INDEX ON WHEN TO BLANKDAT','? ERROR(),MESSAGE()']),
('valid',['REPLACE ALL WHEN WITH CTOD("01/01/92")','INDEX ON WHEN TO VALIDATE','GO TOP','LIST ID,WHEN','CLOSE INDEX','REPLACE WHEN WITH CTOD("")','INDEX ON WHEN TO BLANK2','QUIT'])
])
plan('reentry',[
('state',['USE CLIENTS','GO 2','MARKER=42',['key','ctrl-alt-backslash'],['pause',1],['key','esc'],['key','ret'],'? RECNO(),MARKER']),
('quit',['QUIT',['pause',1],['key','ctrl-alt-backslash'],['pause',1],'? RECCOUNT()','USE CLIENTS','LIST','QUIT'])
])
plan('index-boundary',[
('at-eof',['USE CLIENTS','GO BOTTOM','SKIP','? EOF(),RECNO()','INDEX ON NAME TO EOFIDX','GO TOP','INDEX ON NAME TO TOPIDX','? EOF(),RECNO()']),
('width',['USE EDGE','GO TOP','INDEX ON LEFT(WIDE,100) TO KEY100','GO TOP','INDEX ON LEFT(WIDE,101) TO KEY101','GO TOP','INDEX ON WIDE TO KEY254','SET INDEX TO BAD.NDX','QUIT'])
])
plan('pack-two',[
('before',['USE CLIENTS','INDEX ON NAME TO BYNAME','INDEX ON BAL TO BYBAL','SET INDEX TO BYNAME,BYBAL','SET ORDER TO 1','PACK']),
('after',['REINDEX','GO TOP','LIST','CLOSE INDEX','GO TOP','LIST']),
('reopen',['USE','USE CLIENTS INDEX BYNAME,BYBAL','GO TOP','LIST','QUIT'])
])
plan('paging',[
('pages',['USE BIGSTAFF','REPORT FORM STAFFRPT TO FILE PAGES.TXT']),
('plain',['REPORT FORM STAFFRPT PLAIN TO FILE PLAIN.TXT']),
('groups',['INDEX ON DEPT TO DEPART','REPORT FORM STAFFRPT SUMMARY TO FILE GROUPS.TXT']),
('print',['REPORT FORM STAFFRPT NEXT 1 TO PRINT','LABEL FORM STAFFLBL SAMPLE','QUIT'])
],data='paging-fixture.img')
plan('navigation',[
('list',['USE CLIENTS','LIST','? RECNO(),EOF()','GO 2','DISPLAY ALL','? RECNO(),EOF()','GO TOP','LIST NEXT 2 NAME','? RECNO(),EOF()']),
('hidden',['SET FILTER TO .F.','GO TOP','? RECNO(),EOF(),BOF()','LIST','? RECNO(),EOF(),BOF()','SET FILTER TO','USE EMPTY','? RECCOUNT(),RECNO(),EOF(),BOF()','GO TOP','? EOF(),BOF()','QUIT'])
])
plan('ui-gaps',[
('design',['USE CLIENTS','MODIFY STRUCTURE','CREATE REPORT NEWREP','MODIFY REPORT STAFFRPT','CREATE LABEL NEWLAB','MODIFY COMMAND FLOW']),
('data',['COPY STRUCTURE TO STRUCT','COPY STRUCTURE EXTENDED TO EXTSTR','JOIN WITH CLIENTS TO JOINED FOR .T.','UPDATE ON NAME FROM CLIENTS REPLACE BAL WITH BAL','LIST NAME TO PRINT']),
('unique',['INDEX ON NAME TO UNIQUE1 UNIQUE','SET UNIQUE ON','INDEX ON NAME TO UNIQUE2','SET UNIQUE OFF']),
('finish',['QUIT'])
])
plan('full-recovery',[
('failure',['USE CLIENTS','INDEX ON NAME TO FULLIDX','SORT TO SORTED ON NAME','USE','COPY FILE CLIENTS.DBF TO COPIED.DBF']),
('retry',['ERASE FILLER.BIN','USE CLIENTS','INDEX ON NAME TO FULLIDX','SORT TO SORTED ON NAME','USE SORTED','USE CLIENTS','LIST','QUIT'])
],boot='tracer_boot.img',route='dos',data='full-fixture.img')
plan('settings',[
('trace',['USE CLIENTS','SET ECHO ON','DO USEPRG','SET ECHO OFF','SET TALK OFF','DELETE','SET TALK ON','RECALL']),
('format',['SET FIXED ON','SET DECIMALS TO 2','? 3,3.5','SET FIXED OFF','SET MARGIN TO 5','SET PRINT ON','SET PRINT OFF','SET DEVICE TO PRINT','SET DEVICE TO SCREEN']),
('no-open',['USE','DELETE','RECALL','PACK','REPLACE NAME WITH "x"','QUIT'])
])
plan('verify-final',[
('date-control',['USE EDGE','REPLACE ALL WHEN WITH CTOD("01/01/92")','GO TOP','INDEX ON WHEN TO VALIDATE','GO TOP','LIST ID,WHEN']),
('date-blank',['CLOSE INDEX','GO 1','REPLACE WHEN WITH CTOD("")','GO TOP','INDEX ON WHEN TO BLANK2']),
('manual-date',['USE STAFF','INDEX ON HIRED TO STAFFHIR','LIST NEXT 4 LASTNAME,FIRSTNAME,HIRED']),
('transform',['? TRANSFORM(123.4,"999.99")','? STR(123.4,6,2)','? TRANSFORM(0.12,"9.99")']),
('empty',['USE EMPTY','? RECCOUNT(),EOF(),BOF()','GO TOP','? EOF(),BOF()']),
('quit',['QUIT'])
])
plan('recursion-alone',[
('recursive',['DO RECINT',['pause',3]]),('recovery',[['key','esc'],['key','ctrl-c'],['pause',2],'? "RECOVERED"',['pause',2]])
],data='program-fixture.img')
