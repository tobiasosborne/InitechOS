from pathlib import Path
import shutil, subprocess, struct, json, hashlib
ROOT=Path(__file__).resolve().parents[1]
F=ROOT/'evidence/fixtures'; F.mkdir(exist_ok=True)
S=Path('/home/tobias/Projects/initech-os/build/pubs/screens-check/initechbase-volume-one/samir.d')
manifest=[]
for p in S.iterdir():
 if p.is_file() and p.suffix.upper() in ['.DBF','.DBT','.NDX','.FRM','.LBL','.MEM','.PRG']:
  shutil.copyfile(p,F/p.name); manifest.append({'source':str(p),'name':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
# DBF fixture with every III+ type and boundary widths, independently encoded.
def dbf(name,fields,records):
 h=bytearray(32); h[0]=0x83 if any(f[1]=='M' for f in fields) else 3; h[1:4]=bytes([92,3,1]); rlen=1+sum(f[2] for f in fields); hlen=33+32*len(fields)
 struct.pack_into('<IHH',h,4,len(records),hlen,rlen)
 out=h
 for n,t,w,d in fields:
  f=bytearray(32); f[:len(n)]=n.encode(); f[11]=ord(t); f[16]=w; f[17]=d; out+=f
 out+=b'\r'
 for row in records:
  out+=b' '
  for (_,t,w,_),v in zip(fields,row):
   v=str(v).encode(); out+=v.rjust(w,b' ') if t in 'NM' else v.ljust(w,b' ')
 out+=b'\x1a'; (F/name).write_bytes(out)
dbf('EDGE.DBF',[('ID','N',4,0),('NAME','C',12,0),('WIDE','C',254,0),('PRECISE','N',19,17),('SMALL','N',4,1),('WHEN','D',8,0),('OK','L',1,0),('MEMO','M',10,0)],[[1,'alpha','X'*254,'0.12345678901234567','1.2','19920229','T','1'],[2,'alpha','tail','0.00000000000000001','-9.9','19000101','F','0'],[3,'beta','','0.00000000000000000','0.0','        ','?','0']])
dbt=bytearray(512); struct.pack_into('<I',dbt,0,2); dbt+=b'Original memo\r\nsecond line\x1a\x1a'; dbt+=b'\0'*(1024-len(dbt)); (F/'EDGE.DBT').write_bytes(dbt)
dbf('EMPTY.DBF',[('NAME','C',12,0)],[])
(F/'BAD.NDX').write_bytes(b'not an index\0'*43)
(F/'HELLO.TXT').write_bytes(b'SAMIR AUDIT SENTINEL\r\n')
programs={
 'FLOW.PRG':'''PUBLIC RESULT\nRESULT=0\nI=0\nDO WHILE I < 5\n I=I+1\n IF I=2\n  LOOP\n ENDIF\n DO CASE\n CASE I=3\n  RESULT=RESULT+30\n OTHERWISE\n  RESULT=RESULT+I\n ENDCASE\n IF I=4\n  EXIT\n ENDIF\nENDDO\n? "FLOW",RESULT,I\nDO CHILD WITH 7\n? "CHILD",RESULT\nRETURN\nPROCEDURE CHILD\nPARAMETERS X\nPRIVATE I\nI=99\nRESULT=RESULT+X\nRETURN\n''',
 'BADLOOP.PRG':'I=0\nDO WHILE I<3\nI=I+1\n',
 'RECURSE.PRG':'DO RECURSE\nRETURN\n',
 'LONG.PRG':'PUBLIC SUMX\nSUMX=0\n'+('SUMX=SUMX+1\n'*1000)+'? "LONG",SUMX\nRETURN\n',
 'QUITPRG.PRG':'? "BEFORE QUIT"\nQUIT\n? "AFTER QUIT"\nRETURN\n',
 'LOOP.PRG':'ON ESCAPE STORE 99 TO N\nN=0\nDO WHILE .T.\nN=N+1\nENDDO\n? N\nRETURN\n',
 'FORM.PRG':'NAMEVAR="old"\n@ 2,3 SAY "Name:" GET NAMEVAR\nREAD\n? "RESULT",NAMEVAR\nRETURN\n',
 'TEXT.PRG':'TEXT\nText line one\nText line two\nENDTEXT\nRETURN\n',
 'ONERR.PRG':'ON ERROR STORE ERROR() TO ERRNO\n? MISSING\n? "CAUGHT",ERRNO,MESSAGE()\nON ERROR\nRETURN\n',
 'USEPRG.PRG':'USE CLIENTS\nGO TOP\n? NAME\nRETURN\n',
 'DOSRUN.PRG':'RUN ECHO FROMPROGRAM\n? "RETURNED"\nRETURN\n'
}
for n,t in programs.items(): (F/n).write_bytes(t.replace('\n','\r\n').encode())
img=ROOT/'images/rich-fixture.img'; shutil.copyfile(ROOT/'images/samir_list.img',img)
for p in sorted(F.iterdir()):
 if p.name not in {m['name'] for m in manifest}|set(programs)|{'EDGE.DBF','EDGE.DBT','EMPTY.DBF','BAD.NDX','HELLO.TXT'}: continue
 subprocess.run(['mcopy','-o','-i',str(img),str(p),'::'+p.name],check=True)
(ROOT/'evidence/fixture-manifest.json').write_text(json.dumps(manifest,indent=2))
(ROOT/'evidence/rich-directory.txt').write_text(subprocess.check_output(['mdir','-i',str(img),'::']).decode())
