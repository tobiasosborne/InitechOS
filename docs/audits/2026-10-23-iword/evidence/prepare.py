from pathlib import Path
import shutil,subprocess,json,hashlib
root=Path.cwd(); dest=root/'images/prepared.img'; shutil.copyfile(root/'images/flair_data.img',dest)
fixtures=root/'evidence/fixtures';fixtures.mkdir(exist_ok=True)
gold=Path('/home/tobias/Projects/wordperfect51-decomp/goldens/minted')
for name in ['MINT01','MINT03','MINT04','MINT09','MINT12','MINT14','MINT20']:
 shutil.copyfile(gold/(name+'.WP5'),fixtures/(name+'.WP5'))
for n in [21,25,28,33,38,39,44,45]:
 p=Path('/home/tobias/Projects/wordperfect51-decomp/mint/work/WP51')/('MINT'+str(n))
 if p.exists(): shutil.copyfile(p,fixtures/(p.name+'.WP5'))
(fixtures/'EMPTY.WP5').write_bytes(b'')
(fixtures/'BROKEN.WP5').write_bytes(b'\xffWPC\xff\xff\xff\xff'+b'BAD'*20)
(fixtures/'PLAIN.TXT').write_text('Plain text import.\r\nSecond line.\r\n')
prefix=(gold/'MINT02.WP5').read_bytes()[:327]
(fixtures/'LONG.WP5').write_bytes(prefix+b''.join(f'Line {i:03d} audit document'.encode()+b'\n' for i in range(1,351)))
(fixtures/'CODES.WP5').write_bytes(prefix+b'\xc3\x0c\xc3\xc4\x0c\xc4')
for p in fixtures.iterdir(): subprocess.run(['mcopy','-o','-i',str(dest),str(p),'::'+p.name],check=True)
(root/'evidence/fixtures.json').write_text(json.dumps({p.name:{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in fixtures.iterdir()},indent=2))
