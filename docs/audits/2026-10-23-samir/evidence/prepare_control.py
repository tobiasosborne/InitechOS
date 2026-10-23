from pathlib import Path
import shutil,subprocess,json
R=Path(__file__).resolve().parents[1];F=R/'evidence/fixtures'
progs={
 'RETRY.PRG':'PUBLIC TRYCNT\nTRYCNT=0\nON ERROR DO ERH\n? XX\n? "DONE",TRYCNT\nON ERROR\nRETURN\nPROCEDURE ERH\nXX=7\nTRYCNT=TRYCNT+1\nRETRY\nRETURN\n',
 'MASTER.PRG':'? "OUTER"\nDO SUB\n? "OUTER CONTINUED"\nRETURN\nPROCEDURE SUB\n? "INNER"\nRETURN TO MASTER\n? "INNER CONTINUED"\nRETURN\n',
 'OUTER.PRG':'DO USEPRG\n? "CHILD RETURNED"\nRETURN\n'
}
i=R/'images/control-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',i)
for n,t in progs.items():
 (F/n).write_bytes(t.replace('\n','\r\n').encode());subprocess.run(['mcopy','-i',str(i),str(F/n),'::'+n],check=True)
a=['DO MASTER',['shot','control-master'],'DO OUTER',['shot','control-nested'],'DO RETRY',['shot','control-retry'],'ON ERROR','? "RECOVERED"',['shot','control-recovery'],'QUIT']
(R/'evidence/control.json').write_text(json.dumps({'name':'control','data':'control-fixture.img','actions':a},indent=2))
