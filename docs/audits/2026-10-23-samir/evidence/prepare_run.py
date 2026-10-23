from pathlib import Path
import shutil,subprocess,hashlib,json
R=Path(__file__).resolve().parents[1];s=Path('/home/tobias/Projects/initech-os/build/transient_flairtenants_int.bin');d=R/'evidence/fixtures/COMMAND.COM';shutil.copyfile(s,d)
i=R/'images/run-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',i);subprocess.run(['mcopy','-i',str(i),str(d),'::COMMAND.COM'],check=True)
(R/'evidence/command-fixture.json').write_text(json.dumps({'source':str(s),'sha256':hashlib.sha256(d.read_bytes()).hexdigest(),'bytes':d.stat().st_size},indent=2))
