import json,pathlib,re,collections
ROOT=pathlib.Path(__file__).resolve().parents[1]
f=json.loads((ROOT/'evidence/custom-findings.json').read_text())
menus=json.loads((ROOT/'evidence/menu-verified.json').read_text())
for r in menus:
 if not r['visible_unsupported']:continue
 seq=['/']
 for i in r['indices']:seq+=['Home',f'Right x{i}' if i else '(no Right)','Return']
 n=r['n'];path='/'+ '/'.join(r['path']);f.append(dict(title=path+' is unavailable',area='Menu'+('/'+r['path'][0]),severity='P2',steps='On the empty worksheet choose '+path+'. Exact keys: '+', '.join(seq)+'. Press Escape to dismiss the refusal.',observed='The command opens ERROR with "Command not implemented". Its selected-item description was "'+r['description']+'".',expected='Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.',basis='Real Lotus R2.2 installed 123.RI: item at 0x%04x, description at 0x%04x; the strings were independently read from that binary and compared with the actual serial-rendered description.'%(r['ri_name_offset'],r['ri_description_offset']),shots=[f'menu-{n:03}-selected',f'menu-{n:03}-result'],session='menus',files=['evidence/menu-verified.json'],guess='Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.'))
for i,x in enumerate(f,1):x['id']=f'L{i:03}'
(ROOT/'evidence/final-findings.json').write_text(json.dumps(f,indent=2))
with (ROOT/'NITS.tsv').open('w') as o:
 for x in f:o.write('\t'.join([x['id'],x['area'],x['severity'],x['title']])+'\n')
head=(ROOT/'evidence/report-intro.txt').read_text() if (ROOT/'evidence/report-intro.txt').exists() else '# Initech 123 audit\n\nDRAFT - verification ongoing.\n'
body=[head,'\n## New findings\n']
for x in f:
 body+=['\n### '+x['id']+' - '+x['title']+'\n','\nArea: '+x['area']+'. Severity: '+x['severity']+'.\n','\nSteps: '+x['steps']+'\n','\nObserved: '+x['observed']+'\n','\nExpected: '+x['expected']+'\n','\nBasis: '+x['basis']+'\n']
 if x.get('guess'):body+=['\n'+x['guess']+'\n']
 files=['shots/'+s+'.png' for s in x['shots']]+['evidence/'+x['session']+'-serial.log']+x.get('files',[])
 body+=['\nEvidence: '+', '.join('['+p+']('+p+')' for p in files)+'.\n']
if (ROOT/'evidence/report-tail-final.txt').exists():body+=['\n'+(ROOT/'evidence/report-tail-final.txt').read_text()]
elif (ROOT/'evidence/report-tail.txt').exists():body+=['\n'+(ROOT/'evidence/report-tail.txt').read_text()]
s=''.join(body);s.encode('ascii');(ROOT/'REPORT.md').write_text(s)
print('Findings',len(f),dict(collections.Counter(x['severity'] for x in f)))
