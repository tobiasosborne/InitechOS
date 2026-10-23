from pathlib import Path
import subprocess,shutil,re,json
R=Path(__file__).resolve().parents[1]; dst=R/'images/full-fixture.img';shutil.copyfile(R/'images/samir_list.img',dst)
text=subprocess.check_output(['mdir','-i',str(dst),'::']).decode(); free=int(re.search(r'([\d ]+) bytes free',text).group(1).replace(' ',''))
f=R/'evidence/FILLER.BIN';f.write_bytes(b'F'*free)
subprocess.run(['mcopy','-i',str(dst),str(f),'::FILLER.BIN'],check=True);f.unlink()
(R/'evidence/full-preparation.json').write_text(json.dumps({'free_before':free,'filler_bytes':free,'after':subprocess.check_output(['mdir','-i',str(dst),'::']).decode()},indent=2))
