from PIL import Image
import pathlib,json,numpy as np
root=pathlib.Path(__file__).resolve().parents[1]
nat=Image.open(root/'shots/001-launch.png').convert('RGB')
real=Image.open('/home/tobias/Projects/lotus123-decomp/goldens/minted/MINT01.shots/01_grid.png').convert('RGB')
r=np.asarray(real);r=r[:,[x for x in range(720) if x%9!=8]]
r=Image.fromarray(r)
# White glyphs on navy and black glyphs on cyan become black-on-white masks.
def glyph(im,fg):
 a=np.asarray(im);m=(a==np.array(fg)).all(axis=2);return Image.fromarray(np.where(m,0,255).astype('uint8'))
comparisons=[]
for name,nb,rb,nfg,rfg in [
 ('READY',(575,62,615,78),(600,0,640,16),(255,255,255),(0,0,0)),
 ('column-A',(39,110,111,126),(32,48,104,64),(255,255,255),(0,0,0)),
 ('row-border',(7,126,39,430),(0,64,32,368),(255,255,255),(0,0,0))]:
 a=glyph(nat.crop(nb),nfg);b=glyph(r.crop(rb),rfg);diff=np.asarray(a)!=np.asarray(b)
 a.save(root/'evidence/references'/('native-'+name+'.png'));b.save(root/'evidence/references'/('real-'+name+'.png'))
 comparisons.append({'region':name,'native_crop':nb,'real_after_ninth_column_removal':rb,'pixel_count':int(diff.size),'differing_ink_pixels':int(diff.sum())})
(root/'evidence/look-metrics.json').write_text(json.dumps(comparisons,indent=2))
print(json.dumps(comparisons,indent=2))
