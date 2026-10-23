from pathlib import Path
import json,subprocess as sp,hashlib,struct
from PIL import Image,ImageChops
R=Path(__file__).resolve().parent.parent; E=R/'evidence';I=R/'images';result={}
def read(image,path,target):
 p=sp.run(['mcopy','-i',str(image),'::'+path,'-'],capture_output=True);target.write_bytes(p.stdout)
 return {'rc':p.returncode,'bytes':len(p.stdout),'sha256':hashlib.sha256(p.stdout).hexdigest(),'hex':p.stdout.hex() if len(p.stdout)<128 else None,'stderr':p.stderr.decode()}
for label,path in [('resume-xcopyoverwrite','DHOME/ONE.TXT'),('resume-edlineof','T/EEOF.TXT'),('resume-keyboard','T/CON2.TXT')]:
 result[label]=read(I/f'{label}-data.img',path,E/(label+'-extracted.bin'))
result['syscontrol']={}
for name in ['IO.SYS','INITDOS.SYS']:
 old=sp.check_output(['mcopy','-i',str(I/'geometry-boot-fixture.img')+'@@32256','::'+name,'-']);b=(E/('resume-format-D-'+name)).read_bytes();result['syscontrol'][name]={'size':len(b),'equals_original':b==old,'original_sha256':hashlib.sha256(old).hexdigest()}
# Read-only checks of stopped data disks; never run a repair mode.
checks={}
for session in sorted(E.glob('resume-*-session.json')):
 j=json.loads(session.read_text());label=session.name[:-13]
 if 'stopped_utc' not in j:continue
 image=I/f'{label}-data.img'
 if image.exists():
  p=sp.run(['fsck.fat','-n',str(image)],capture_output=True);(E/(label+'-fsck.txt')).write_bytes(p.stdout+p.stderr);checks[label]={'fsck_rc':p.returncode,'qemu_exit':j['exit_code'],'qemu_pid':j['qemu_pid']}
result['sessions']=checks
for label in ['resume-fdiskcreate','resume-fdiskactive','resume-fdiskmenus']:
 b=(I/f'{label}-boot.img').read_bytes();before=(I/('single-boot-fixture.img' if label.endswith('create') else 'geometry-boot-fixture.img')).read_bytes()
 result[label]={'mbr_unchanged':b[:512]==before[:512],'partitions':[{'active':b[o],'type':b[o+4],'lba':struct.unpack_from('<I',b,o+8)[0],'sectors':struct.unpack_from('<I',b,o+12)[0]} for o in range(446,510,16)]}
shots=[Image.open(R/'shots'/f'resume-gaps2-cursor-{i}.png').convert('RGB') for i in range(6)]
result['cursor']={'unique_frames':len({im.tobytes() for im in shots}),'changed_regions':[ImageChops.difference(shots[0],im).getbbox() for im in shots[1:]]}
for n in ['advanced-disktools','resume-fdiskmenus']:
 j=json.loads((E/(n+'-session.json')).read_text());result[n+'-image-change']={path:j['images_before'][path]['sha256']!=v['sha256'] for path,v in j['images_after'].items()}
result['root64real']={'root_entries':struct.unpack_from('<H',(I/'resume-root64real-data.img').read_bytes(),17)[0]}
b=(I/'full512-fixture.img').read_bytes();f=b[512:512+9*512];free=[]
for n in range(2,2849):
 v=struct.unpack_from('<H',f,n+n//2)[0];v=(v>>4 if n%2 else v&4095)
 if v==0:free.append(n)
result['last_cluster_fixture']={'free_cluster_numbers':free}
(E/'resume-final-verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
