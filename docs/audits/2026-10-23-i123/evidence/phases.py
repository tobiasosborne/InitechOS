import sys,json,time,pathlib,subprocess,struct
from drive123 import Audit,ROOT,MENU

def menus(a):
 rows=[]
 def walk(level,indices,names):
  for i,it in enumerate(MENU[level]):
   inds=indices+[i];names2=names+[it['name']]
   rows.append((inds,names2,it))
   if it['child']!=65535:walk(it['child'],inds,names2)
 walk(0,[],[])
 (ROOT/'evidence/menu-plan.json').write_text(json.dumps(rows,indent=2))
 results=[]
 for n,(indices,names,it) in enumerate(rows):
  # Do not terminate an otherwise clean sheet in the inventory pass.
  a.key('ctrl-scroll_lock');a.key('slash');level=0
  mode='initial' if n%2==0 else 'arrows'
  for depth,idx in enumerate(indices):
   if depth==len(indices)-1:
    a.key('home')
    for _ in range(idx):a.key('right')
   else:
    siblings=MENU[level];initial=siblings[idx]['name'][0]
    if mode=='initial' and sum(x['name'][0]==initial for x in siblings)==1:a.type(initial,False)
    else:
     a.key('home')
     for _ in range(idx):a.key('right')
     a.key('ret')
    level=MENU[level][idx]['child']
  tag=f'menu-{n:03}'
  a.shot(tag+'-selected')
  if names==['Quit','Yes']:
   a.key('esc');a.key('esc');out='exit-avoided-in-inventory; exercised in quit phase'
  else:
   a.key('ret');a.shot(tag+'-result')
   out='activated'
   # Exercise one Escape at every submenu/leaf; next iteration resets with Break.
   a.key('esc')
  results.append({'n':n,'path':names,'indices':indices,'description':it['description'],'navigation':mode,'outcome':out})
  (ROOT/'evidence/menu-results.json').write_text(json.dumps(results,indent=2))
  print('MENU',n,'/'.join(names),flush=True)

def long(a):
 a.goto('A1');a.type('x'*239+'Z',False);a.pause(3);a.state('long-240')
 a.type('Q',False);a.pause(1);a.state('long-241');a.key('ret');a.pause(2);a.state('long-committed')
 a.saveas('LONG');a.pause(1);a.state('long-saved')
 a.key('f2');a.state('long-f2');a.key('esc');a.key('esc')
 a.cell('A2','UNSAVED');a.state('dirty-before-retrieve');a.get('MINT01');a.pause(1);a.state('dirty-after-retrieve')
 a.saveas('M1ROUND');a.state('mint-roundtrip')
 a.cell('A3','KEEPME');a.get('BAD');a.state('malformed-preserves');a.key('esc');a.goto('A3');a.state('malformed-cell')
 a.get('MISSING');a.state('missing-preserves')

def functions(a):
 setup={'D1':'1','D2':'2','D3':'3','E1':'10','E2':'20','E3':'30','F1':'3','F2':'300','F3':'3000','G1':'-100','G2':'60','G3':'60','J1':'Name','K1':'Value','J2':'a','K2':'10','J3':'a','K3':'20','J4':'b','K4':'99','M1':'Name','M2':'a'}
 for c,v in setup.items():a.cell(c,v)
 cases=json.loads((ROOT/'evidence/function-cases.json').read_text())
 cases[78]='@TRIM(" a  b ")'
 for i,f in enumerate(cases):
  a.key('ctrl-scroll_lock');a.cell('A'+str(i+1),f)
  a.shot('func-%02d'%(i+1))
  print('FUNCTION',i+1,f,flush=True)
 a.key('ctrl-scroll_lock');a.saveas('FUNCS');a.state('functions-saved')
 for i,f in enumerate(cases):
  name=f.split('(')[0];wrong=name+'()' if '(' in f else name+'(1)'
  a.key('ctrl-scroll_lock');a.cell('C'+str(i+1),wrong);a.shot('arity-%02d'%(i+1))
 a.key('ctrl-scroll_lock');a.saveas('ARITY');a.state('arity-saved')
 extras=['@SUM(A200..IV8192)','@AVG(A200..IV8192)','@MIN(A200..IV8192)','@MAX(A200..IV8192)','@COUNT(A200..IV8192)','@SUM("x")','@SQRT("x")','@SUM(@ERR,1)','@SUM(@NA,1)','@IF(0,@ERR,42)','1/0','@ROUND(1.25,1)','+B120','+A119','+C121','+A122']
 for i,f in enumerate(extras):
  a.key('ctrl-scroll_lock');a.cell('A'+str(110+i),f);a.shot('edge-%02d'%i)
 a.key('ctrl-scroll_lock');a.goto('A110');a.state('function-edges');a.saveas('EDGES')
def transfers(a):
 for case,steps in [('COPY',[('C1..C6','E1','COPYB'),('C1..C6','D8','COPYC'),('C3','C20','COPYD')]),('MOVE',[('A1','D1','MOVEB'),('B2','F5','MOVEC'),('A2..C2','A8','MOVED')])]:
  a.get(case+'01A');a.state(case+'-loaded')
  for src,dst,out in steps:
   a.slash('c' if case=='COPY' else 'm');a.type(src);a.type(dst);a.state(out);a.saveas(out)
 a.clear()
 for c,v in {'A1':'1','A2':'2','A3':'3','B1':'+A1','B2':'+$A$2','B3':'+A3'}.items():a.cell(c,v)
 a.slash('c');a.type('A1..B3');a.type('A2');a.state('copy-overlap');a.saveas('OVERCP')
 a.slash('m');a.type('A2..B4');a.type('A1');a.state('move-overlap');a.saveas('OVERMV')
 a.slash('c');a.type('A1');a.type('D1..D3');a.state('copy-target-range-refused');a.key('esc')
 a.goto('A1');a.slash('c');a.key('dot');a.key('down');a.key('right');a.state('point-dot-range');a.key('ret');a.state('point-dot-result');a.key('esc')
 a.goto('A1');a.slash('wcs');a.type('1');a.state('width-1')
 a.slash('wcs');a.type('240');a.state('width-240');a.key('right');a.state('width-240-right')
 a.goto('A1');a.slash('wcr');a.slash('wcs');a.type('0');a.state('width-0-refused');a.key('esc')
 a.slash('wcs');a.type('241');a.state('width-241-refused');a.key('esc')
 a.clear();a.cell('A1','Rate');a.cell('B1','5');a.slash('rncrates');a.key('ret');a.type('B1');a.goto('rates');a.state('name-goto')
 a.cell('C1','+rates*2');a.slash('rncrates');a.key('ret');a.type('B2');a.state('name-redefine');a.key('esc')
 a.slash('rnd');a.type('rates');a.goto('C1');a.state('name-delete-formula');a.saveas('NAMES')
 a.clear();a.cell('A1','Item');a.cell('B1','7');a.slash('rnlr');a.type('A1');a.goto('Item');a.state('name-labels');a.saveas('LABELNAM')
 a.clear();a.cell('A1','10');a.cell('B1','+A1*2');a.slash('wgrm');a.cell('A1','20');a.state('manual-stale');a.key('f9');a.state('manual-recalc')
 for order in ['c','r','n']:
  a.slash('wgr'+order);a.state('recalc-order-'+order)
 a.slash('wgra');a.cell('A1','30');a.state('automatic-recalc')
 a.cell('C1','+C1+1');a.state('circular');a.slash('wgri');a.type('50');a.key('f9');a.state('circular-50');a.saveas('CIRCOUT')
 a.clear();a.cell('A1','erase-no');a.slash('wen');a.state('worksheet-erase-no');a.slash('wey');a.state('worksheet-erase-yes')
 a.cell('A1','protected');a.slash('rp');a.type('A1');a.slash('wgpe');a.cell('A1','changed');a.state('protected-entry');a.key('ctrl-scroll_lock');a.slash('re');a.type('A1');a.state('protected-erase');a.key('esc');a.slash('wgpd');a.slash('re');a.type('A1');a.state('range-erase');a.saveas('ERASE')

def capacity(a):
 a.get('CAP2047');a.state('capacity-2047');a.cell('A2048','8');a.state('capacity-2048');a.cell('A2049','9');a.state('capacity-2049');a.key('ctrl-scroll_lock');a.cell('A1','2');a.state('capacity-replace');a.saveas('CAPOUT')
 a.slash('re');a.type('A1');a.cell('A2049','9');a.state('capacity-after-erase');a.key('ctrl-scroll_lock');a.get('CAP2049');a.state('capacity-oversize-get');a.key('esc');a.goto('A2048');a.state('capacity-preserved');a.saveas('CAPERA')
 a.clear();a.get('MINT03');a.state('mint03-loaded');a.key('f9');a.state('mint03-recalc');a.saveas('M3ROUND')
 a.get('MINT04');a.key('f9');a.state('mint04-recalc');a.saveas('M4ROUND');a.get('MINT05');a.key('f9');a.state('mint05-recalc');a.saveas('M5ROUND')

def files(a):
 a.cell('A1','original');a.saveas('SAVE');a.cell('A1','changed');a.saveas('SAVE');a.state('replace-menu');a.type('c',False);a.state('replace-cancel');a.saveas('SAVE');a.type('b',False);a.state('backup-refused');a.key('esc');a.saveas('SAVE');a.type('r',False);a.state('replace-success')
 for name in ['TOOLONGNAME','A B','BAD?','..','A:/SUB/TEST','12345678.1234','\\','C:TEST']:
  a.key('ctrl-scroll_lock');a.saveas(name);a.state('filename-'+str(['TOOLONGNAME','A B','BAD?','..','A:/SUB/TEST','12345678.1234','\\','C:TEST'].index(name)))
 a.key('ctrl-scroll_lock');a.saveas('PASS p');a.state('password-refused');a.key('esc')
 a.clear();a.cell('A1','x'*239+'Z');a.saveas('LONG0');a.key('f2');a.key('ret');a.saveas('LONGEDIT');a.state('long-edit-data');a.cell('A2','+1.23456789012345');a.saveas('NUM0');a.key('f2');a.key('ret');a.saveas('NUMEDIT');a.state('numeric-edit-data')

def full(a):
 a.get('FULL');a.cell('A1','replacement')
 for i in range(2,14):a.cell('A'+str(i),'x'*220+str(i))
 a.saveas('NEWFULL');a.state('full-new-refused');a.key('esc');a.saveas('FULL');a.type('r',False);a.state('full-replace-failed');a.key('esc');a.get('FULL');a.state('full-retrieve-after-fail');a.key('esc');a.key('ctrl-q');a.state('full-quit-protection')

def formats(a):
 a.cell('A1','1234.5678912345')
 for code in ['f','s','c',',','p']:
  for dec in range(16):
   a.slash('rf'+code);a.type(str(dec));a.type('A1');a.state('format-'+{'f':'fixed','s':'sci','c':'currency',',':'comma','p':'percent'}[code]+'-'+str(dec))
 for code,tag in [('g','general'),('+','plusminus'),('t','text'),('h','hidden'),('r','reset'),('d1','date1'),('d2','date2'),('d3','date3'),('d4','date4'),('d5','date5'),('dt1','time1'),('dt2','time2'),('dt3','time3'),('dt4','time4')]:
  a.key('ctrl-scroll_lock');a.slash('rf'+code)
  if tag not in ['date4','date5','time1','time2','time3','time4']:a.type('A1')
  a.state('format-'+tag)
 a.key('ctrl-scroll_lock');a.slash('rff');a.type('16');a.state('format-16-refused');a.key('esc');a.slash('rff');a.type('-1');a.state('format-minus-refused');a.key('esc');a.saveas('FORMATS')
def keyslook(a):
 a.key('f5');a.key('esc');a.state('goto-escape-menu');a.key('ctrl-scroll_lock')
 a.saveas('LOOK');a.cell('C2','@CELL("filename",A1)');a.state('cell-filename-saved')
 a.slash('rncrates');a.key('ret');a.type('B1');a.key('f5');a.key('f3');a.state('f3-named-range');a.key('ctrl-scroll_lock')
 a.cell('F1','+E1');a.cell('E1','+D1');a.cell('D1','9');a.state('forward-chain')
 a.type('pending',False);a.key('scroll_lock');a.state('scroll-lock-cancels')
 for c,v in {'A1':'a','B1':'b','C1':'c','D1':'d','A2':'2','A3':'3','A4':'4'}.items():a.cell(c,v)
 a.goto('A1');a.key('end');a.state('end-indicator');a.key('right');a.state('end-right')
 a.goto('A1');a.key('caps_lock');a.state('caps-indicator');a.key('caps_lock');a.key('num_lock');a.state('num-indicator');a.key('num_lock')
 a.cell('A1','undo');a.key('alt-f4');a.state('undo-absent')
 for k in ['f1','f3','f4','f6','f7','f8','f10']:
  a.key(k);a.state('key-'+k)
 a.goto('B3');a.type('+A1',False);a.key('f4');a.state('f4-absolute');a.key('esc')
 a.goto('A1');a.key('f2');a.key('home');a.type('X',False);a.key('left');a.key('delete');a.state('edit-cursor-delete');a.key('esc');a.key('esc')
 a.slash('s');a.state('unsupported-before-enter');a.key('ret');a.state('unsupported-after-enter');a.key('esc')
 a.click(360,181);a.state('cell-click');a.goto('A100');a.state('scroll-before');a.click(627,69);a.state('scroll-arrow');a.drag(627,120,0,150);a.state('scroll-thumb')
 a.key('home');a.drag(632,472,-288,-194);a.goto('E1');a.state('resize-pointer');a.click(140,166);a.state('resize-hit');a.click(325,50);a.state('resize-zoom')
 a.key('ctrl-q');a.state('quit-dialog-look')
def hotkeys(a):
 rows=json.loads((ROOT/'evidence/menu-plan.json').read_text())
 out=[]
 for n,(inds,names,it) in enumerate(rows):
  if it['child']!=65535 or names==['Quit','Yes']:continue
  a.key('ctrl-scroll_lock');a.key('slash');lev=0;unique=True
  for idx in inds:
   row=MENU[lev];ch=row[idx]['name'][0]
   if sum(x['name'][0]==ch for x in row)!=1:
    unique=False;a.key('home')
    for _ in range(idx):a.key('right')
    a.key('ret')
   else:a.type(ch,False)
   lev=row[idx]['child']
  a.shot('hotkey-%03d'%n);a.key('esc');out.append({'n':n,'path':names,'all_initials_unique':unique})
  (ROOT/'evidence/hotkey-results.json').write_text(json.dumps(out,indent=2))
  print('HOTKEY',n,'/'.join(names),flush=True)
def typeerrors(a):
 import re
 spec=(pathlib.Path('/home/tobias/Projects/initech-os/spec/i123/wk1_formula.h')).read_text()
 rows=re.findall(r'\{ "(@[^"]+)"\s*,WKF_K_(FIXED|COUNTED),\s*(\d+),\s*0,\s*(0x[0-9A-Fa-f]+|0),',spec.replace('WKF_K_','WKF_K_'))
 # Whitespace differs before the class token in the source.
 rows=re.findall(r'\{ "(@[^"]+)"\s*,\s*WKF_K_(FIXED|COUNTED),\s*(\d+),\s*0,\s*(0x[0-9A-Fa-f]+|0),',spec)
 cases=[]
 for name,cl,arity,mask in rows:
  if not int(arity):continue
  args=['A300..B302' if int(mask,0)&(1<<i) else '"x"' for i in range(int(arity))]
  cases.append(name+'('+','.join(args)+')')
 a.cell('D1','42');a.cell('D5','D1')
 cases+=['@@(D5)','@SUM(A300..IV8192)','@AVG(A300..IV8192)','@MIN(A300..IV8192)','@MAX(A300..IV8192)','@COUNT(A300..IV8192)','@VAR(A300..IV8192)','@STD(A300..IV8192)','@REPEAT("x",10000)','@ROUND(1,1E99)','@DATE(0,2,29)','@SQRT(-1)','@LN(0)','@ASIN(2)','@INDEX(A300..B302,100,100)','@CHAR(0)','@CHAR(256)']
 (ROOT/'evidence/typeerror-cases.json').write_text(json.dumps(cases,indent=2))
 for i,f in enumerate(cases):
  a.key('ctrl-scroll_lock');a.cell('A'+str(i+1),f);a.shot('typeerror-%03d'%i);print('TYPE',i,f,flush=True)
 a.key('ctrl-scroll_lock');a.saveas('TYPES');a.state('typeerrors-saved')

def function_errors(a):
 a.get('FUNCS')
 cases=json.loads((ROOT/'evidence/function-cases.json').read_text())
 for i,f in enumerate(cases):
  name=f.split('(')[0];wrong=name+'()' if '(' in f else name+'(1)'
  a.key('ctrl-scroll_lock');a.cell('C'+str(i+1),wrong);a.shot('arity-%02d'%(i+1))
 a.key('ctrl-scroll_lock');a.saveas('ARITY');a.state('arity-saved')
 extras=['@SUM(A200..IV8192)','@AVG(A200..IV8192)','@MIN(A200..IV8192)','@MAX(A200..IV8192)','@COUNT(A200..IV8192)','@SUM("x")','@SQRT("x")','@SUM(@ERR,1)','@SUM(@NA,1)','@IF(0,@ERR,42)','1/0','@ROUND(1.25,1)','+B120','+A119','+C121','+A122']
 for i,f in enumerate(extras):
  a.key('ctrl-scroll_lock');a.cell('A'+str(110+i),f);a.shot('edge-%02d'%i)
 a.key('ctrl-scroll_lock');a.goto('A110');a.state('function-edges');a.saveas('EDGES')

def quits(a):
 def trigger(route):
  if route=='ctrlq':a.key('ctrl-q')
  elif route=='file':a.menu(53,30,64,53)
  elif route=='close':a.click(10,50)
  else:a.slash('qy')
 def reopen():a.click(211,124,2);a.pause(.4)
 for ri,route in enumerate(['ctrlq','file','close','slash']):
  for rep in range(2):
   tag='quit-'+route+'-'+str(rep);name='Q'+str(ri)+str(rep)
   a.cell('A1',tag);trigger(route);a.state(tag+'-guard');a.key('esc');a.state(tag+'-cancel')
   trigger(route);a.key('ret');a.state(tag+'-save-prompt');a.key('esc');a.state(tag+'-save-cancel')
   trigger(route);a.key('ret');a.type(name);a.pause(.3);a.state(tag+'-saved-exit');reopen();a.get(name);a.state(tag+'-reloaded')
   a.cell('A1','discard');trigger(route);a.click(252,168);a.pause(.3);a.state(tag+'-discard-exit');reopen();a.get(name);a.state(tag+'-discard-reloaded');a.key('ctrl-q');reopen()
 a.type('1+',False);a.key('ctrl-q');a.key('ret');a.state('quit-invalid-entry');a.key('esc');a.key('esc')
 a.type('pending',False);a.key('ctrl-q');a.key('esc');a.state('quit-pending-cancel');a.key('ctrl-q');a.key('ret');a.type('QPEND');a.state('quit-pending-saved')

def formats_wide(a):
 a.slash('wcs');a.type('30')
 old=a.state
 a.state=lambda name:old('wide-'+name)
 formats(a)

def heavy(a):
 a.get('HEAVY');a.state('heavy-loaded')
 before=time.monotonic();a.cmd('human-monitor-command',{'command-line':'sendkey f9 20'});time.sleep(.03);a.cmd('human-monitor-command',{'command-line':'sendkey ctrl-scroll_lock 20'});a.pause(1);a.state('heavy-break');duration=time.monotonic()-before
 (ROOT/'evidence/heavy-timing.json').write_text(json.dumps({'seconds_until_capture':duration,'note':'F9 and Ctrl-Break 30 ms apart; screenshot after 1 s. This does not prove Break interrupted rather than followed completion.'},indent=2))
 a.saveas('HEAVYOUT');a.state('heavy-saved')

def full2(a):
 a.state('full2-launch')
 old=a.state;a.state=lambda name:old('full2-'+name)
 full(a)

def full_repeat(a):
 a.get('FULL');a.cell('A1','REPEAT')
 for i in [3,4,5]:a.cell('A'+str(i),'z'*239+str(i))
 a.saveas('FULL');a.type('r',False);a.state('repeat-full-replace');a.key('esc')
 a.key('ctrl-q');a.key('ret');a.key('ret');a.type('c',False);a.state('repeat-full-save-cancel');a.key('ctrl-q');a.key('esc');a.state('repeat-full-dirty-preserved')

def edge_more(a):
 cases=['@STRING(1234.5678912345,15)','@STRING(0,15)','@VLOOKUP(1,A300..B302,1)','@HLOOKUP(1,A300..B302,1)','@NPV(.1,A300..A302)','@IRR(.1,A300..A302)','@INDEX(A300..B302,0,0)','@COLS(A300..B302)','@ROWS(A300..B302)','@S(A300)','@N(A300)','@CELL("width",A300)','@DSUM(A300..B302,1,D300..D301)','@DAVG(A300..B302,1,D300..D301)','@DCOUNT(A300..B302,1,D300..D301)','@DMIN(A300..B302,1,D300..D301)','@DMAX(A300..B302,1,D300..D301)','@DVAR(A300..B302,1,D300..D301)','@DSTD(A300..B302,1,D300..D301)']
 for i,f in enumerate(cases):a.cell('A'+str(i+1),f);a.shot('empty-extra-%02d'%i)
 a.saveas('EMPTY');a.state('empty-extra-saved')
 a.cell('IV8192','17');a.cell('B300','1');a.cell('A30','@SUM(B1..IV8192)');a.state('whole-sheet-sum')
 a.saveas('WHOLE')
 # Probe the minimum-scale resized geometry and formula text editing.
 a.cell('D1','123.5');a.cell('E1','+D1*2');a.goto('E1');a.key('f2');a.key('home');a.key('right');a.type('0',False);a.state('edit-formula-cursor');a.key('esc');a.key('esc')
 a.goto('D1');a.type('pending',False);a.key('ctrl-c');a.state('ctrl-c-literal');a.key('ctrl-scroll_lock')

def power(a):
 for branch in ['cancel','save','discard']:
  for rep in range(2):
   if branch!='cancel' or rep:a.open123()
   a.cell('A1','POWER-'+branch+'-'+str(rep))
   a.drag(632,472,-288,-194);a.click(380,90)
   before=a.serial.stat().st_size
   a.menu(170,30,170,125);a.state('power-'+branch+'-'+str(rep)+'-guard')
   if branch=='cancel':
    a.key('esc');a.click(150,50);a.state('power-'+branch+'-'+str(rep)+'-result');a.key('ctrl-q');a.click(252,168)
   elif branch=='save':
    a.key('ret');a.state('power-'+branch+'-'+str(rep)+'-prompt');a.click(150,50);a.type('SD'+str(rep));a.pause(.5);a.state('power-'+branch+'-'+str(rep)+'-result')
   else:
    a.click(252,168);a.pause(.5);a.state('power-'+branch+'-'+str(rep)+'-result')
   data=a.serial.read_bytes()[before:]
   if b'FLAIR-SHUTDOWN-SAFE' in data:a.key('ret');a.pause(3)
   else:a.note('No FLAIR-SHUTDOWN-SAFE during this power trial: '+branch+' '+str(rep))

def pool(a):
 a.get('POOL135');a.state('pool-loaded');a.cell('A136','y'*240);a.state('pool-new-full');a.key('ctrl-scroll_lock')
 a.cell('A1','y'*240);a.state('pool-replace-full');a.key('ctrl-scroll_lock');a.goto('A1');a.state('pool-old-preserved')
 a.cell('A1','y');a.state('pool-small-replace');a.saveas('POOLOUT')

def operations(a):
 for c,v in {'A1':'1','B1':'2','A2':'+A1+B1','B2':'4'}.items():a.cell(c,v)
 a.slash('rv');a.type('A1..B2');a.type('D1');a.state('range-value')
 a.slash('rt');a.type('A1..B2');a.type('G1');a.state('range-transpose');a.saveas('RANGES')
 a.clear();a.cell('B2','RightName');a.cell('C2','12');a.slash('rnlr');a.type('B2');a.goto('RightName');a.state('names-label-right')
 a.cell('B4','DownName');a.cell('B5','15');a.slash('rnld');a.type('B4');a.goto('DownName');a.state('names-label-down')
 a.cell('D2','LeftName');a.slash('rnll');a.type('D2');a.goto('LeftName');a.state('names-label-left')
 a.cell('D4','UpName');a.cell('D3','13');a.slash('rnlu');a.type('D4');a.goto('UpName');a.state('names-label-up')
 a.slash('rnt');a.type('F1');a.state('names-table');a.saveas('NAMETAB');a.slash('rnr');a.goto('RightName');a.state('names-reset');a.key('esc')
 a.clear();a.cell('A1','label');
 for k in ['r','c','l']:
  a.slash('rl'+k);a.type('A1');a.state('range-label-'+k)
 a.cell('B1','123.45');a.slash('wgff');a.type('2');a.state('global-format');a.slash('wgc');a.type('15');a.state('global-width')
 a.slash('ru');a.type('A1');a.slash('wgpe');a.cell('A1','allowed');a.state('range-unprotect');a.slash('wgpd')
 a.slash('rfh');a.type('B1');a.goto('B1');a.state('hidden-unprotected');a.slash('rp');a.type('B1');a.slash('wgpe');a.state('hidden-protected');a.slash('wgpd')
 for v in ['0','51','1','50']:
  a.key('ctrl-scroll_lock');a.slash('wgri');a.type(v);a.state('iteration-'+v)
 a.key('ctrl-scroll_lock');a.saveas('SETTINGS')

phase=sys.argv[1];a=Audit()
try:
 a.launch(phase, "images/full-fixture2.img" if phase in ["full2","full_repeat"] else "images/full-fixture.img" if phase=="full" else "images/functions-data.img" if phase=="function_errors" else "images/fixture.img")
 globals()[phase](a)
finally:
 if a.proc is not None:a.quit()
