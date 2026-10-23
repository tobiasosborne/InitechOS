"""Rebuild replay disks from archived small inputs; no repository access/build."""
from pathlib import Path
import json,shutil,subprocess,re,tempfile
R=Path(__file__).resolve().parents[1];E=R/'evidence';F=E/'fixtures';I=R/'images'
base={x['name'] for x in json.loads((E/'fixture-manifest.json').read_text())}
base.update('EDGE.DBF EDGE.DBT EMPTY.DBF BAD.NDX HELLO.TXT FLOW.PRG BADLOOP.PRG RECURSE.PRG LONG.PRG QUITPRG.PRG LOOP.PRG FORM.PRG TEXT.PRG ONERR.PRG USEPRG.PRG DOSRUN.PRG'.split())
def build(name,src,names):
 p=I/name;shutil.copyfile(I/src,p)
 for n in sorted(names):subprocess.run(['mcopy','-o','-i',str(p),str(F/n),'::'+n],check=True)
build('rich-fixture.img','samir_list.img',base)
variants={
 'program-fixture.img':'SHORT1K.PRG RECINT.PRG CONSOLE.PRG CHARARG.PRG',
 'run-fixture.img':'COMMAND.COM',
 'paging-fixture.img':'BIGSTAFF.DBF BIGSTAFF.DBT',
 'control-fixture.img':'RETRY.PRG MASTER.PRG OUTER.PRG',
 'master-fixture.img':'MASTER2.PRG',
 'redirect-fixture.img':'REDIR.TXT',
 'retry-fixture.img':'RETRYFIX.PRG'}
for n,v in variants.items():build(n,'rich-fixture.img',v.split())
p=I/'full-fixture.img';shutil.copyfile(I/'samir_list.img',p)
s=subprocess.check_output(['mdir','-i',str(p),'::']).decode();free=int(re.search(r'([\d ]+) bytes free',s).group(1).replace(' ',''))
with tempfile.NamedTemporaryFile(dir=E) as f:
 f.write(b'F'*free);f.flush();subprocess.run(['mcopy','-i',str(p),f.name,'::FILLER.BIN'],check=True)
print('Rebuilt',len(variants)+2,'fixture images from archived inputs.')
