from pathlib import Path
import shutil,subprocess,json
R=Path(__file__).resolve().parents[1];F=R/'evidence/fixtures';txt='? "MASTER"\nDO FIRST\n? "MASTER RESUMED"\nRETURN\nPROCEDURE FIRST\n? "FIRST"\nDO SECOND\n? "FIRST RESUMED"\nRETURN\nPROCEDURE SECOND\n? "SECOND"\nRETURN TO MASTER\nRETURN\n'
(F/'MASTER2.PRG').write_bytes(txt.replace('\n','\r\n').encode());i=R/'images/master-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',i);subprocess.run(['mcopy','-i',str(i),str(F/'MASTER2.PRG'),'::MASTER2.PRG'],check=True)
(R/'evidence/master-deep.json').write_text(json.dumps({'name':'master-deep','data':'master-fixture.img','actions':['DO MASTER2',['shot','master-deep-result'],'QUIT']},indent=2))
