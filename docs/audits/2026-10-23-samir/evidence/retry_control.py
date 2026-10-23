from pathlib import Path
import shutil,subprocess,json,sys
R=Path(__file__).resolve().parents[1];F=R/'evidence/fixtures';p=F/'RETRYFIX.PRG';p.write_bytes(b'PUBLIC XX,TRYCNT\r\nXX="bad"\r\nTRYCNT=0\r\nON ERROR DO ERH\r\n? XX+1\r\n? "DONE",TRYCNT\r\nON ERROR\r\nRETURN\r\nPROCEDURE ERH\r\nXX=6\r\nTRYCNT=TRYCNT+1\r\nRETRY\r\nRETURN\r\n');img=R/'images/retry-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',img);subprocess.run(['mcopy','-i',str(img),str(p),'::RETRYFIX.PRG'],check=True)
plan={'name':'retry-control','data':'retry-fixture.img','actions':['DO RETRYFIX','? XX,TRYCNT',['shot','retry-control-result'],'QUIT']};j=R/'evidence/retry-control.json';j.write_text(json.dumps(plan,indent=2));subprocess.run([sys.executable,'-u',str(R/'evidence/run_suite.py'),str(j)],check=True)
