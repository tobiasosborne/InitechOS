from pathlib import Path
import shutil,subprocess,json
root=Path.cwd(); disk=root/'images/stress-input.img';shutil.copyfile(root/'images/prepared.img',disk)
prefix=(root/'evidence/fixtures/MINT01.WP5').read_bytes()[:327]
f=root/'evidence/stress-fixtures';f.mkdir(exist_ok=True)
(f/'MAX.WP5').write_bytes(prefix+(b'123456789\n'*6521)[:65209])
(f/'OVER.WP5').write_bytes((f/'MAX.WP5').read_bytes()+b'X')
(f/'TEXTLONG.TXT').write_text(''.join(f'Text line {i:03d}\r\n' for i in range(1,81)))
shutil.copyfile('/home/tobias/Projects/wordperfect51-decomp/goldens/minted/MINT05.WP5',f/'MINT05.WP5')
for p in f.iterdir():subprocess.run(['mcopy','-o','-i',str(disk),str(p),'::'+p.name],check=True)
subprocess.run(['mmd','-i',str(disk),'::MANY'],check=True)
small=f/'SMALL.WP5';small.write_bytes(prefix+b'Small document.')
for i in range(270):subprocess.run(['mcopy','-o','-i',str(disk),str(small),f'::MANY/F{i:03d}.WP5'],check=True)
subprocess.run(['mmd','-i',str(disk),'::DOCS'],check=True)
subprocess.run(['mcopy','-o','-i',str(disk),str(small),'::DOCS/DEEP.WP5'],check=True)
(root/'evidence/stress-before-directory.txt').write_text(subprocess.check_output(['mdir','-i',str(disk),'::MANY'],text=True))
