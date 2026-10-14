#!/usr/bin/env python3
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import numpy as np,json,hashlib,struct,shutil
ROOT=Path(__file__).resolve().parent
MAC=Path('/home/tobias/Projects/system7-decomp/goldens/captures')
RES=MAC.parent/'resources'
LOT=Path('/home/tobias/Projects/lotus123-decomp/goldens/minted/MINT01.shots')
def copy_reference_images():
 for p in [*MAC.glob('s8_*.png'),MAC/'s7_menu_file.png',MAC/'s7_menu_view.png',MAC/'s7_menu_special.png',MAC/'s7_menu_apple.png',MAC/'s7_close_pressed.png',MAC/'s7_zoom_pressed.png',LOT/'01_grid.png',LOT/'02_entered.png',LOT/'03_save_prompt.png']:
  dest=ROOT/'refs'/('lotus-'+p.name if p.parent==LOT else p.name)
  shutil.copyfile(p,dest)

def arr(p):return np.array(Image.open(p).convert('RGB'))
def rows(p,x,y0,y1):return ['#%02X%02X%02X'%tuple(v) for v in arr(p)[y0:y1,x]]
def counts(p,rect):
 a=arr(p)[rect[1]:rect[3],rect[0]:rect[2]]
 c,n=np.unique(a.reshape(-1,3),axis=0,return_counts=True)
 return {'#%02X%02X%02X'%tuple(v):int(k) for v,k in zip(c,n)}
def groups(flags):
 out=[];start=None
 for i,b in enumerate(list(flags)+[False]):
  if b and start is None:start=i
  if not b and start is not None:out.append([start,i-1]);start=None
 return out

def bbox(p,r,color=(0,0,0)):
 a=arr(p)[r[1]:r[3],r[0]:r[2]];ys,xs=np.where(np.all(a==color,axis=2))
 return [int(xs.min()+r[0]),int(ys.min()+r[1]),int(xs.max()+r[0]),int(ys.max()+r[1]),int(len(xs))] if len(xs) else None

def font(fid,text):
 raw=(RES/f'NFNT_{fid}.bin').read_bytes(); h=struct.unpack_from('>13h',raw); first,last=h[1:3];height=h[7];rw=h[12]; ascent=h[9]
 bitmap=np.unpackbits(np.frombuffer(raw[26:26+height*rw*2],dtype=np.uint8)).reshape(height,rw*16)
 locoff=26+height*rw*2;owoff=16+2*h[8]
 loc=struct.unpack_from('>'+str(last-first+3)+'H',raw,locoff)
 data=[];w=0
 for ch in text:
  idx=ord(ch)-first;ow=struct.unpack_from('>H',raw,owoff+idx*2)[0];lb=(ow>>8);lb=lb-256 if lb>=128 else lb;aw=ow&255
  data.append((w+lb,bitmap[:,loc[idx]:loc[idx+1]]));w+=aw
 out=np.zeros((height,w),dtype=np.uint8)
 for x,g in data:
  x0=max(0,x);x1=min(w,x+g.shape[1])
  if x1>x0:out[:,x0:x1]=np.maximum(out[:,x0:x1],g[:,x0-x:x1-x])
 return Image.fromarray(np.where(out[:,:,None]>0,0,255).repeat(3,axis=2).astype('uint8')),{'width':w,'ascent':ascent,'height':height}

def main():
 copy_reference_images()
 metrics={}
 s=ROOT/'shots'
 for n in ['001-desktop','004-root','042-many-root','026-scrollbar-inactive']:
  p=s/(n+'.png')
  metrics[n]={'title_x100_y60_82':rows(p,100,60,82),'left_frame_y200':rows(p,100,0,0), 'colors':counts(p,[0,0,640,480])}
 metrics['root_left_frame']=['#%02X%02X%02X'%tuple(v) for v in arr(s/'004-root.png')[200,20:27]]
 metrics['ref_left_frame']=['#%02X%02X%02X'%tuple(v) for v in arr(MAC/'s8_doc_window_active.png')[150,65:72]]
 metrics['menubar_profile']=rows(s/'001-desktop.png',400,0,40)
 metrics['ref_menubar_profile']=rows(MAC/'s8_desktop_menubar.png',400,0,20)
 for n,r in [('in_menutext',[21,2,250,17]),('ref_menutext',[31,2,250,17])]:
  p=s/'001-desktop.png' if n.startswith('in_') else MAC/'s7_menu_file.png';a=arr(p)[r[1]:r[3],r[0]:r[2]];ink=np.all(a==0,axis=2)
  metrics[n]={'bbox':bbox(p,r),'columns':[[a+r[0],b+r[0]] for a,b in groups(ink.any(0))]}
 for n,p,r in [('filetitle',s/'001-desktop.png',[27,2,58,17]),('filemenu',s/'007-finder-file.png',[39,40,160,55]),('shortcut',s/'007-finder-file.png',[140,41,161,56]),('check',s/'016-view-menu.png',[98,42,112,56]),('cursor',s/'001-desktop.png',[498,378,516,397])]:metrics[n]={'bbox':bbox(p,r),'colors':counts(p,r)}
 for fid,t,name in [(5478,'File','chicago-File'),(5478,'New Folder','chicago-New-Folder'),(5478,'HELLO','chicago-HELLO'),(5415,'README.TXT','geneva-README'),(5478,chr(18),'chicago-checkmark'),(5478,chr(0xC9),'chicago-ellipsis')]:
  im,m=font(fid,t);im.save(ROOT/'refs'/(name+'.png'));metrics[name]=m
 metrics['gray_thumb_rows']=rows(s/'042-many-root.png',366,96,114)
 metrics['disabled_vbar_rows']=rows(s/'004-root.png',366,82,106)
 metrics['disabled_arrow_bbox']=bbox(s/'004-root.png',[360,82,375,96],(165,165,165))
 metrics['ref_disabled_arrow_bbox']=bbox(MAC/'s8_doc_window_active.png',[461,113,475,127],(165,165,165))
 metrics['enabled_arrow_bbox']=bbox(s/'042-many-root.png',[360,82,375,96])
 metrics['ref_enabled_arrow_bbox']=bbox(MAC/'s8_doc_window_inactive.png',[608,296,622,310])
 metrics['thumbcolors']=counts(s/'042-many-root.png',[360,98,375,113])
 metrics['refthumbcolors']=counts(MAC/'s8_doc_window_inactive.png',[608,311,622,326])
 metrics['wellcross_in']=['#%02X%02X%02X'%tuple(v) for v in arr(s/'042-many-root.png')[150,360:375]]
 metrics['wellcross_ref']=['#%02X%02X%02X'%tuple(v) for v in arr(MAC/'s8_doc_window_inactive.png')[350,608:622]]
 (ROOT/'evidence/metrics.json').write_text(json.dumps(metrics,indent=2))
 if __name__ == '__main__': print(json.dumps({k:v for k,v in metrics.items() if k not in ['001-desktop','004-root','042-many-root','026-scrollbar-inactive']},indent=2))

if __name__ == '__main__': main()
