#!/usr/bin/env python3
"""Generate audit fixture disks from supplied immutable images using mtools."""
from pathlib import Path
import shutil,subprocess,json
ROOT=Path(__file__).resolve().parents[1]
F=ROOT/'evidence/fixtures';F.mkdir(exist_ok=True)
def run(*a):return subprocess.run(a,cwd=ROOT,check=True,capture_output=True).stdout
fixtures={'LF.TXT':b'ONE\n\nTHREE\n','CRLF.TXT':b'ONE\r\n\r\nTHREE\r\n','EIGHT.TXT':b'ASCII '+bytes(range(128,160))+b' END\r','K4.TXT':b'word '*819+b'X','K32.TXT':(b'0123456789 '*3000)[:32768],'K64.TXT':(b'0123456789 '*6000)[:65536],'WRAP.TXT':b'A line with words for resize testing and selection. '*10+b'\rEND OF DOCUMENT\r','LINES25.TXT':b''.join(('Line %02d\r'%i).encode() for i in range(1,26)),'LONG.TXT':b''.join(('Row %02d for long scrapbook clip\r'%i).encode() for i in range(1,30))}
shutil.copyfile(ROOT/'images/flair_data.img',ROOT/'images/fixture-data.img')
for name,b in fixtures.items():
 (F/name).write_bytes(b);run('mcopy','-i','images/fixture-data.img',str(F/name),'::/'+name)
(F/'manifest.json').write_text(json.dumps({k:len(v) for k,v in fixtures.items()},indent=2))
# This part depends on the stopped text session disk, preserving its saved files.
if (ROOT/'images/text-data.img').exists():
 import re
 shutil.copyfile(ROOT/'images/text-data.img',ROOT/'images/full-data.img')
 (F/'KEEP.TXT').write_bytes(b'KEEP THIS ORIGINAL DATA.\r'*30)
 run('mcopy','-i','images/full-data.img',str(F/'KEEP.TXT'),'::/KEEP.TXT')
 s=run('mdir','-i','images/full-data.img','::/').decode()
 free=int(re.search(r'([\d ]+) bytes free',s)[1].replace(' ',''))
 (F/'FILL.BIN').write_bytes(b'F'*free)
 run('mcopy','-i','images/full-data.img',str(F/'FILL.BIN'),'::/FILL.BIN')
 (ROOT/'evidence/full-before.txt').write_bytes(run('mdir','-i','images/full-data.img','::/'))
