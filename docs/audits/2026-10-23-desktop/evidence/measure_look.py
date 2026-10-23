#!/usr/bin/env python3
"""Pixel samples from actual audit PNGs and copied native Mac captures."""
from pathlib import Path
from collections import Counter
from PIL import Image
import json
R=Path(__file__).resolve().parents[1]
def im(s):return Image.open(R/s).convert('RGB')
def colors(image,rect):return {str(k):v for k,v in Counter(image.crop(rect).getdata()).items()}
a=im('shots/002-root.png');b=im('evidence/references/s8_doc_window_active.png')
m={'title_band':{'current_col':90,'current_rows':[60,82],'reference_col':100,'reference_rows':[70,92],'equal':a.crop((90,60,91,82)).tobytes()==b.crop((100,70,101,92)).tobytes()},'body_frame':{'current_row200_x20_25':[a.getpixel((x,200)) for x in range(20,26)],'reference_row150_x65_70':[b.getpixel((x,150)) for x in range(65,71)]}}
a=im('shots/017-file-menu.png')
m['menu_title_colors']=colors(a,(33,20,75,39))
m['menu_text']={'frame_left':33,'first_New_Folder_ink':min(x for x in range(34,120) for y in range(43,52) if a.getpixel((x,y))==(0,0,0))}
m['shortcut_right_gap']={'last_ink':max(x for x in range(145,182) for y in range(43,52) if a.getpixel((x,y))==(0,0,0)),'frame':183,'gap':14}
a=im('shots/041-many.png')
for name,rect in [('thumb',(380,117,394,132)),('arrow',(380,102,394,116))]:m[name]={'shot':'041-many.png','rect':rect,'colors':colors(a,rect)}
m['arrow_tile_equal']=a.crop((380,102,394,116)).tobytes()==im('evidence/references/s8_doc_window_inactive.png').crop((608,296,622,310)).tobytes()
a=im('shots/075-empty-confirm.png')
m['alert']={'left_edge_x133_to143_at_y140':[a.getpixel((x,140)) for x in range(133,144)],'caution_yellow_pixels':sum(c[0]>180 and c[1]>180 and c[2]<100 for c in a.crop((150,95,195,139)).getdata())}
m['button_bevel']={str(y):colors(a,(365,y,419,y+1)) for y in [160,175]}
m['button_corner_equal']=a.crop((363,158,367,162)).tobytes()==im('evidence/references/s8_alert_modal.png').crop((363,158,367,162)).tobytes()
(R/'evidence/look-metrics.json').write_text(json.dumps(m,indent=2))
