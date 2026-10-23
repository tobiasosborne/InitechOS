import pathlib,subprocess,struct,json,sys,math,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
def parse(p):
 b=p.read_bytes();off=0;recs=[];cells=[]
 while off+4<=len(b):
  op,n=struct.unpack_from('<HH',b,off);data=b[off+4:off+4+n];r={'offset':off,'opcode':op,'length':n,'body_hex':data.hex()};recs.append(r);off+=4+n
  if len(data)!=n:r['error']='truncated';break
  if op in [12,13,14,15,16,51]:
   fmt,c,row=struct.unpack_from('<BHH',data)
   col='';cc=c+1
   while cc:cc,m=divmod(cc-1,26);col=chr(65+m)+col
   cell={'cell':col+str(row+1),'format':fmt,'opcode':op,'body_hex':data.hex()}
   if op==13:cell['value']=struct.unpack_from('<h',data,5)[0]
   if op in [14,16]:
    val=struct.unpack_from('<d',data,5)[0];cell['value']=val if math.isfinite(val) else repr(val)
   if op in [15,51]:cell['text']=data[5:].split(b'\0')[0].decode('latin1')
   if op==16:cell['formula_hex']=data[15:].hex()
   cells.append(cell)
  if op==1:break
 return {'path':str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p),'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'consumed':off,'records':recs,'cells':cells}
if __name__=='__main__':
 for label in sys.argv[1:]:
  dest=ROOT/'evidence'/'extracted'/label;dest.mkdir(parents=True,exist_ok=True)
  img=ROOT/'images'/f'{label}-data.img'
  out=subprocess.run(['mdir','-i',str(img),'::'],capture_output=True,text=True)
  (dest/'directory.txt').write_text(out.stdout)
  subprocess.run(['mcopy','-o','-i',str(img),'::*.WK1',str(dest)],check=True)
  for p in dest.glob('*.WK1'):
   try:(p.with_suffix('.json')).write_text(json.dumps(parse(p),indent=2))
   except Exception as e:p.with_suffix('.error.txt').write_text(repr(e))
