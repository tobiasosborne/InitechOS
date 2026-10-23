from pathlib import Path
import subprocess,shutil,re
r=Path(__file__).resolve().parents[1];f=r/'evidence/tps-fixtures'
def run(*a):return subprocess.run(a,cwd=r,check=True,capture_output=True).stdout
s='images/tps-full-source.img';shutil.copyfile(r/'images/full-data.img',r/s)
run('mdel','-i',s,'::/FILL.BIN')
for name in ['TPSREPT.TXT','TPSREPT.BAK']:
 p=f/('old-'+name);p.write_bytes(('PREVIOUS '+name+' MUST BE RETAINED\r\n').encode()*5)
 run('mcopy','-i',s,str(p),'::/APPS/'+name)
out=run('mdir','-i',s,'::/').decode();n=int(re.search(r'([\d ]+) bytes free',out)[1].replace(' ',''))
p=f/'full-filler.bin';p.write_bytes(b'F'*n);run('mcopy','-i',s,str(p),'::/FILL.BIN')
(r/'evidence/tps-full-before.txt').write_bytes(run('mdir','-i',s,'::/APPS'))
