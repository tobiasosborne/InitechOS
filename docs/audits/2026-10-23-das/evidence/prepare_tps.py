from pathlib import Path
import subprocess,shutil,struct,json
root=Path(__file__).resolve().parents[1]; f=root/'evidence/tps-fixtures';f.mkdir(exist_ok=True)
def run(*a):return subprocess.run(a,cwd=root,check=True,capture_output=True).stdout
src='images/flair_data.img';dst='images/tps-cases.img';shutil.copyfile(root/src,root/dst)
for n in ['TPSREP.COM','TPS.DBF','TPSPARM.DAT']:(f/n).write_bytes(run('mtype','-i',src,'::/APPS/'+n))
parm=(f/'TPSPARM.DAT').read_bytes();db=(f/'TPS.DBF').read_bytes();hdr=struct.unpack_from('<H',db,8)[0];rec=struct.unpack_from('<H',db,10)[0]
cases={'MISSING':None,'EMPTY':b'','BADNUM':parm.replace(b'LINES=6',b'LINES=banana'),'ZERO':parm.replace(b'LINES=6',b'LINES=0'),'HUNDRED':parm.replace(b'LINES=6',b'LINES=100'),'LONGPARM':b'X=ignored\r\n'*1000+parm,'EMPTYDB':parm,'THOUSAND':parm.replace(b'LINES=6',b'LINES=100'),'BADPAGE':parm,'TRUNC':parm}
for name,p in cases.items():
 run('mmd','-i',dst,'::/'+name)
 run('mcopy','-i',dst,str(f/'TPSREP.COM'),'::/'+name+'/TPSREP.COM')
 if p is not None:
  (f/(name+'.PAR')).write_bytes(p);run('mcopy','-i',dst,str(f/(name+'.PAR')),'::/'+name+'/TPSPARM.DAT')
 b=db
 if name=='EMPTYDB':b=b''
 if name=='THOUSAND':
  h=bytearray(db[:hdr]);struct.pack_into('<I',h,4,1000);b=bytes(h)+db[hdr:hdr+rec]*1000+b'\x1a'
 if name=='BADPAGE':
  b=bytearray(db);b[hdr+1+6+20+12:hdr+1+6+20+12+3]=b'XYZ';b=bytes(b)
 if name=='TRUNC':b=db[:32] # missing descriptor terminator; probe last, bounded observation
 (f/(name+'.DBF')).write_bytes(b);run('mcopy','-i',dst,str(f/(name+'.DBF')),'::/'+name+'/TPS.DBF')
(f/'manifest.json').write_text(json.dumps({'header':hdr,'record':rec,'cases':list(cases)},indent=2))
