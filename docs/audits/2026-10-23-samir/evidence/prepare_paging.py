from pathlib import Path
import shutil,struct,subprocess,json
R=Path(__file__).resolve().parents[1];F=R/'evidence/fixtures';b=(F/'STAFF.DBF').read_bytes();n,h,r=struct.unpack_from('<IHH',b,4);head=bytearray(b[:h]);struct.pack_into('<I',head,4,n*4)
(F/'BIGSTAFF.DBF').write_bytes(head+b[h:h+n*r]*4+b'\x1a');shutil.copyfile(F/'STAFF.DBT',F/'BIGSTAFF.DBT');i=R/'images/paging-fixture.img';shutil.copyfile(R/'images/rich-fixture.img',i)
for name in ['BIGSTAFF.DBF','BIGSTAFF.DBT']:subprocess.run(['mcopy','-i',str(i),str(F/name),'::'+name],check=True)
