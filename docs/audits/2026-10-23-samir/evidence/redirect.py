from pathlib import Path
import sys,shutil,subprocess,json
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R));from audit_driver import Driver
p=R/'evidence/fixtures/REDIR.TXT';p.write_bytes(b'USE CLIENTS\r\nLIST\r\nQUIT\r\n');img=R/'images/redirect-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',img);subprocess.run(['mcopy','-i',str(img),str(p),'::REDIR.TXT'],check=True)
d=Driver()
try:
 d.start('redirect','images/tracer_boot.img','images/redirect-fixture.img');d.key('ret',.8);d.key('ret',.8);d.type('SAMIR.COM <REDIR.TXT');d.pause(2);d.shot('redirect-result');d.type('ECHO RETURNED');d.pause(1);d.shot('redirect-shell-return');d.tail(25)
finally:
 if d.proc:d.quit()
