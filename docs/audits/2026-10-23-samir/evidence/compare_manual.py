from pathlib import Path
import json,difflib
E=Path(__file__).resolve().parent
mapping=json.loads((E/'manualfn-expectations.json').read_text());steps=[json.loads(l) for l in (E/'manualfn-steps.jsonl').read_text().splitlines()];raw=(E/'manualfn-serial.log').read_bytes();out=[];prev=[]
def norm(s):return '\n'.join(l.rstrip() for l in s.replace('\r\n','\n').replace('\r','').splitlines()).strip()
for st in steps:
 a=st['action']
 if isinstance(a,list) and a[0]=='shot' and a[1] in mapping:
  ref=mapping[a[1]];n=len(ref['input_text'].splitlines());part=[x for x in prev if isinstance(x['action'],str)][-n:]
  if not part:continue
  actual='. '+raw[part[0]['start']:part[-1]['end']].decode('ascii',errors='replace')
  if actual.endswith('. '):actual=actual[:-2]
  same=norm(actual)==norm(ref['expected_text'])
  out.append({'name':a[1],'matches':same,'expected_source':ref['expected'],'actual':actual,'expected':ref['expected_text'],'diff':'\n'.join(difflib.unified_diff(norm(ref['expected_text']).splitlines(),norm(actual).splitlines(),fromfile='manual',tofile='guest',lineterm=''))})
  prev=[]
 else: prev.append(st)
(E/'manual-comparison.json').write_text(json.dumps(out,indent=2))
print('completed',len(out),'matched',sum(x['matches'] for x in out))
for x in out:
 if not x['matches']:print(x['name'],x['diff'][:1000])
