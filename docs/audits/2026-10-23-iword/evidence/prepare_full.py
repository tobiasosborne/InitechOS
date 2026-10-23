from pathlib import Path
import subprocess,shutil,re,json
root=Path.cwd(); disk=root/'images/full-input.img';shutil.copyfile(root/'images/prepared.img',disk)
prefix=(root/'evidence/fixtures/MINT01.WP5').read_bytes()[:327]
orig=root/'evidence/fixtures/ORIGINAL.WP5';orig.write_bytes(prefix+b'ORIGINAL SAFE TEXT. DO NOT LOSE THIS VERSION.\n')
for name in ['ORIGINAL.WP5','READONLY.WP5']:
 subprocess.run(['mcopy','-o','-i',str(disk),str(orig),'::'+name],check=True)
subprocess.run(['mattrib','-i',str(disk),'+r','::READONLY.WP5'],check=True)
s=subprocess.check_output(['mdir','-i',str(disk),'::'],text=True)
free=int(re.search(r'([\d ]+) bytes free',s).group(1).replace(' ',''))
f=root/'evidence/full-filler.bin';f.write_bytes(b'F'*free)
subprocess.run(['mcopy','-o','-i',str(disk),str(f),'::FILLER.BIN'],check=True)
(root/'evidence/full-before-directory.txt').write_text(subprocess.check_output(['mdir','-i',str(disk),'::'],text=True))
(root/'evidence/full-preparation.json').write_text(json.dumps({'free_before_filler':free,'filler_bytes':f.stat().st_size,'source':'images/prepared.img'},indent=2))
