#!/usr/bin/env python3
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import numpy as np,json,hashlib,struct,datetime
ROOT=Path(__file__).resolve().parent
MAC='/home/tobias/Projects/system7-decomp/goldens/captures/'
NFNT='/home/tobias/Projects/system7-decomp/goldens/resources/NFNT_5478.bin'
DOS='/home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf'
DB='/home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf'
LOT='/home/tobias/Projects/lotus123-decomp/goldens/minted/MINT01.shots/'
F=[];C=[]
def pixels(p,r=None):
 im=Image.open(ROOT/p).convert('RGB')
 return im.crop(r) if r else im

def add(area,sev,title,state,saw,expected,ap,ar,rp,rr,provenance,kind='rgb',note=''):
 ident=f'M{len(F)+1:03}'
 a=pixels(ap,ar);b=pixels(rp,rr)
 a.save(ROOT/'diffs'/f'{ident}-initech.png');b.save(ROOT/'refs'/f'{ident}-reference.png')
 aa=np.array(a);bb=np.array(b)
 metric=None
 if kind in ['rgb','ink']:
  w=max(a.width,b.width);h=max(a.height,b.height)
  na=np.full((h,w,3),255,np.uint8);nb=na.copy()
  na[:a.height,:a.width]=aa;nb[:b.height,:b.width]=bb
  if kind=='ink':
   na=np.where(np.all(na==0,2)[:,:,None],0,255).repeat(3,2).astype('uint8')
   nb=np.where(np.all(nb==0,2)[:,:,None],0,255).repeat(3,2).astype('uint8')
  diff=np.any(na!=nb,2); metric={'different_pixels':int(diff.sum()),'comparison_pixels':w*h,'method':kind,'sizes':[list(a.size),list(b.size)]}
  heat=np.where(diff[:,:,None],np.array([220,0,70]),np.array([240,240,240])).astype('uint8');heat=Image.fromarray(heat)
  ims=[Image.fromarray(na),Image.fromarray(nb),heat]
  scale=max(1,min(8,1000//(w*3+24),360//h));gap=12
  colw=max(w*scale,180);canvas=Image.new('RGB',(colw*3+gap*4,max(h*scale+56,100)),'#d0d0d0');dr=ImageDraw.Draw(canvas)
  labels=['Initech (normalized ink)' if kind=='ink' else 'Initech','Reference','Different pixels (pink)']
  for i,(im,label) in enumerate(zip(ims,labels)):
   x=gap+i*(colw+gap);dr.text((x,8),label,fill='black');canvas.paste(im.resize((w*scale,h*scale),Image.Resampling.NEAREST),(x,34))
 else:
  # Manual illustrations have scanner pixels, not framebuffer pixels. Present
  # annotated comparisons without inventing a direct RGB oracle for them.
  def fit(im,maxw,maxh,nearest):
   f=min(maxw/im.width,maxh/im.height)
   return im.resize((max(1,round(im.width*f)),max(1,round(im.height*f))),Image.Resampling.NEAREST if nearest else Image.Resampling.LANCZOS)
  a=fit(a,850,250,True);b=fit(b,850,320,False)
  canvas=Image.new('RGB',(max(a.width,b.width,550)+24,a.height+b.height+100),'#d0d0d0');dr=ImageDraw.Draw(canvas)
  dr.text((12,8),'Initech: measured guest pixels',fill='black');canvas.paste(a,(12,28));dr.rectangle((11,27,a.width+12,a.height+28),outline='#c00040',width=2)
  dr.text((12,a.height+42),'Reference illustration: semantic/layout comparison; no RGB equality claim',fill='black');canvas.paste(b,(12,a.height+64))
 canvas.save(ROOT/'diffs'/f'{ident}.png')
 rec={'id':ident,'area':area,'severity':sev,'summary':title,'state_steps':state,'initech':saw,'reference':expected,'provenance':provenance,'screenshot':ap,'initech_crop':ar,'reference_file':rp,'reference_crop':rr,'kind':kind,'metric':metric,'note':note,'diff':f'diffs/{ident}.png'}
 F.append(rec);C.append(rec)
 return rec

boot='Fresh flair_tenants_interactive.img + flair_data.img copies. '
root=boot+'Double-click disk (600,64); move pointer to (500,380). '
menu=root+'Move to File (38,30), hold left button, wait 0.3 s. '
many='Use many_source.img (a copy of flair_data.img with thirty F00.TXT..F29.TXT files added by prepare_fixtures.py). Boot tenants; double-click disk (600,64); move pointer clear. '
app=root+'Double-click APPS (123,101), then 123.EXE (211,124). '
alert=app+'Enter Hello, Right, 123, Right, 1.5, Right, -7 (Enter after each entry); Ctrl-Q; move pointer clear. '
dos='Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. '
db='Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. '

add('menu bar','P3','Apple slot and first title sit too far left',root,
 'Lower-bar Apple black ink is x=4..13; File ink x=28..48. The same narrow Apple slot is on the upper bar. File starts 15 px left of the Mac capture; logo starts 13 px left.',
 'Mac OS 8 Apple colour ink x=17..26; File ink x=43..62. Both bars are otherwise 20 px tall.',
 'shots/004-root.png',[0,20,80,40],'refs/s8_desktop_menubar.png',[0,0,80,20],MAC+'s8_desktop_menubar.png, x=0..79 y=0..19; specs/sys8/menus.md 1.3.')
add('menu highlight','P2','Selected menu title uses a pale flat fill',menu,
 'Selected File block x=17..59 has #8DDCDC fill; white letters sit on that pale fill. At x=21 the selected band has a single accent value through y=20..38, then black baseline y=39.',
 'At x=34, Mac selected File uses #8787DA y=0, #5454B3 y=1..17, #0000A5 y=18, black y=19. The accent has a dark face and separate edge shades.',
 'shots/007-finder-file.png',[21,20,22,40],'refs/s8_menu_dropped.png',[34,0,35,20],MAC+'s8_menu_dropped.png, x=34 y=0..19; specs/sys8/menus.md 1.4.',note='The desktop teal is deliberate. This finding concerns the selected-title ink/background contrast and lost accent shading, not the desktop colour. The real capture uses Lavender; no claim that every installation must use Lavender.')
add('menu shortcuts','P3','Caret substitutes for the Command glyph',menu,
 'New Folder shows ^N. Its black shortcut ink is x=144..157 y=43..51.',
 'New Folder shows the looped Command mark and N, x=199..216 y=23..31.',
 'shots/007-finder-file.png',[143,41,160,55],'refs/s8_menu_dropped.png',[198,21,217,35],MAC+'s8_menu_dropped.png, command column x=199..216 y=23..31; Menu Manager commandMark=17.',kind='ink',note='Ctrl remains a usable PC shortcut. This is a glyph-fidelity nit, not a request to change key handling.')
add('menu shortcuts','P3','Shortcut column has excessive right clearance',menu,
 'New Folder N ends at x=157; panel right black frame is x=174: ink-to-frame distance 17 px.',
 'N ends at x=216; right frame x=230: distance 14 px (13 intervening pixels).',
 'shots/007-finder-file.png',[150,41,176,55],'refs/s8_menu_dropped.png',[209,21,233,35],MAC+'s8_menu_dropped.png, x=209..232 y=21..34; specs/sys8/menus.md 2.2.',note='The panels have different item inventories and widths. This compares the last shortcut glyph against each panel right edge, not total panel width.')
add('menu text placement','P3','Item ink begins one pixel too far from the left frame',menu,
 'Panel left frame x=20; New Folder first N ink x=41: offset 21 px. Text cap top y=43 is the correct panel-top +4.',
 'Panel left frame x=33; N ink x=53: offset 20 px. Cap top y=23 is also panel-top +4.',
 'shots/007-finder-file.png',[20,41,50,54],'refs/s8_menu_dropped.png',[33,21,63,34],MAC+'s8_menu_dropped.png x=33/53 y=23..31; specs/sys8/menus.md 2.2.',note='This is a painted-ink comparison. Chicago and Charcoal left bearings differ; it is not evidence that the Chicago advance table is wrong.')
add('menu marks','P3','View check mark is a lowercase v',root+'Hold View (120,30), wait 0.5 s.',
 'The by Icons mark is a 6x7 ink box at x=101..106 y=45..51, 26 black pixels. It is a v-shaped letter.',
 'System 7 checkMark is character 18. NFNT 5478 glyph 18 renders a 9x8 check with 18 ink pixels; its advance is 11 px.',
 'shots/016-view-menu.png',[100,40,111,55],'refs/chicago-checkmark.png',None,NFNT+' glyph 0x12; specs/toolbox/menu-manager.md mark table (checkMark=18). refs/chicago-checkmark.png is rendered directly from the original NFNT location/offset/width tables.',kind='ink')
add('scroll thumb','P2','Enabled thumb face has no bevel ramp',many,
 'The 14x15 thumb interior x=360..373 y=97..111 contains 205 #8DDCDC and 5 #F3F3F3 pixels. No separate leading highlight or trailing shadow is visible.',
 'The 14x15 interior x=608..621 y=311..325 contains 49 #DADAFF, 102 #B3B3FF, 26 #8787DA, 28 #5454B3 and 5 #F3F3F3 pixels. The outer face has distinct light and dark edges.',
 'shots/042-many-root.png',[360,97,374,112],'refs/s8_doc_window_inactive.png',[608,311,622,326],MAC+'s8_doc_window_inactive.png: active Applications thumb x=608..621 y=311..325; specs/sys8/scrollbars.md 2.4.',note='A teal accent could keep the deliberate desktop colour while retaining a three-shade thumb. This is not a demand for a Lavender desktop.')
add('scroll thumb','P2','Thumb grip lines disappear into the face',many,
 'Only five pale #F3F3F3 cap pixels interrupt the flat teal face. The grip region x=363..369 y=100..106 has zero dark grip pixels.',
 'The active Applications thumb has four #5454B3 dark grip lines, 7 px each, 28 dark pixels total, with companion highlight lines.',
 'shots/042-many-root.png',[362,99,372,108],'refs/s8_doc_window_inactive.png',[610,313,620,322],MAC+'s8_doc_window_inactive.png x=610..619 y=313..321; specs/sys8/scrollbars.md 2.4.',note='Separate from M007: this is the interior grip, not the outer bevel. Both states remain functionally draggable.')
add('pointer','P3','Arrow stem and white mask differ from the Mac cursor',boot+'Move pointer to (500,380).',
 'On a 16x16 canvas at (499,379), the cursor has 61 black and 53 white pixels. Black bbox x=500..508 y=380..393 (9x14); the stem reads as a squared vertical tail.',
 'The arrow at (230,29) has 54 black and 43 white pixels on the corresponding 16x16 canvas at (229,28); black bbox x=230..237 y=29..42 (8x14), with an angled tail. Aligned symbol masks differ at 48/256 pixels.',
 'refs/cursor-in-mask.png',None,'refs/cursor-mac-mask.png',None,MAC+'s8_alert_modal.png x=229..244 y=28..43. Reference opacity is obtained by subtracting the same wallpaper patch in s8_desktop_menubar.png; current cursor is shots/001-desktop.png x=499..514 y=379..394.',note='Gray in these two normalized masks means transparent, black means cursor ink, white means cursor paper. The initial cursor screenshot is in shots/, not a synthetic guest screen.')
add('title truncation','P3','Narrow title ends in one period instead of an ellipsis',root+'Drag grow box (370,270) by (-200,-120); move pointer clear.',
 'The 160x100 window draws Drive A Fil. The suffix is one 2x2 period cluster at x=131..132 y=73..74, 4 ink pixels.',
 'An ellipsis is three dot clusters. The Chicago NFNT ellipsis (0xC9) has 12 ink pixels in a 10x2 ink box, advance 14 px. Using an ellipsis to signal title truncation is an expectation from my classic Mac memory.',
 'shots/022-root-small.png',[130,63,134,78],'refs/chicago-ellipsis.png',None,NFNT+' glyph 0xC9; behavior expectation FROM MEMORY, no matching truncated-title Mac golden.',kind='ink',note='The read-only Initech chrome_metrics.json explicitly selects a single period as current product policy. This is a requested appearance-polish deviation, not an undisclosed claim of a captured Mac title rule.')
add('button bevel','P2','Opaque label background erases button highlight and shadow',alert,
 'Cancel is 59x20 at x=363..421 y=158..177. Across x=365..418 at y=160, only 11 pixels stay white and 43 become face #E7E7E7. At y=175, only 10 stay #C0C0C0 and 44 are face. The label creates gaps in both bevel strips.',
 'Same Cancel rect. Row y=160 x=365..418 is all 54 white pixels; row y=175 has 53 #C0C0C0 and one face pixel. The bevel passes continuously above and below the label.',
 'shots/031-123-save-alert.png',[363,158,422,178],'refs/s8_alert_modal.png',[363,158,422,178],MAC+'s8_alert_modal.png x=363..421 y=158..177; specs/sys8/controls.md 5.1.',note='The raw full-button diff also includes the known Chicago/Charcoal label difference (F16). The new claim is restricted to rows 160 and 175, outside the label ink.')
add('button corners','P3','Rounded button corners acquire extra gray fringe pixels',alert,
 'Cancel corner (363,158) is #C0C0C0 and (364,158) is #969696; both sit outside the top black span. Across four 4x4 corner patches, 13/64 pixels differ from the Mac button.',
 'Both cited pixels are surrounding #E7E7E7 face. The captured corner follows the smaller rounded black edge with its local corner shading.',
 'shots/031-123-save-alert.png',[363,158,367,162],'refs/s8_alert_modal.png',[363,158,367,162],MAC+'s8_alert_modal.png top-left x=363..366 y=158..161. Four-corner count also uses x=418..421 and y=174..177.',note='Independent of M011 label erasure; this crop has no label ink. Geometry still matches 59x20 and a +3 px default ring.')
add('scroll arrows','P3','Enabled arrow tiles mispaint two off-diagonal corners',many,
 'In the 14x14 up-arrow tile x=360..373 y=82..95, top-right is #CDCDCD and bottom-left #FFFFFF.',
 'The corresponding tile x=608..621 y=296..309 has face #E7E7E7 at both off-diagonal corners. Only 2/196 pixels differ; the other 194 and the 20-pixel arrow glyph match.',
 'shots/042-many-root.png',[360,82,374,96],'refs/s8_doc_window_inactive.png',[608,296,622,310],MAC+'s8_doc_window_inactive.png x=608..621 y=296..309; specs/sys8/scrollbars.md 2.2.')
add('DOS/database console','P2','Console keeps pale gray text on green',db+'Separately boot tracer_boot.img + flair_data.img and run DIR. In SAMIR try SET COLOR TO and LIST again.',
 'Both consoles use foreground #C0C0C0 and background #6FA08E. The settled DOS DIR screen has 4792 foreground pixels and 302408 background pixels. SET COLOR TO leaves the same palette (and returns a text error).',
 'dBASE III PLUS colour defaults are standard white on black, as specified on printed U5-204 (PDF 316). My default DOS gray/white-on-black expectation is FROM MEMORY; the DOS manual scan cannot establish historical framebuffer RGB.',
 'shots/078-db-default-color.png',[0,0,400,256],'refs/dbase-manual-pdfpage-316.png',[45,270,1035,930],DB+' printed U5-204, PDF 316, Defaults. DOS baseline expectation FROM MEMORY.',kind='manual',note='The deliberate teal DESKTOP is not being reported as a bug. This is the suspended text-console surface. No scan RGB is treated as a VGA palette measurement.')
add('DOS cursor','P2','Ready prompt has no visible blinking cursor',dos+'Type CLS, wait 1 s; capture a complete blink-cycle sample series without input.',
 'After CLS, only the four-character A:\\> prompt is drawn (86 foreground pixels, bbox x=0..30 y=386..395). The next character cell x=32..39 y=384..399 contains zero foreground pixels. Fourteen further frames across the recorded 2+ s interval have identical RGB bytes.',
 'The DOS manual printed p.5 (PDF 18) explicitly shows a cursor following A> and describes a blinking box or flashing underline.',
 'shots/088-after-cls.png',[0,380,48,404],'refs/dos33-manual-pdfpage-18.png',[73,751,208,851],DOS+' printed p.5, PDF 18, The Default Drive and the MS-DOS Prompt, prompt/cursor illustration.',kind='manual',note='Timing and hashes are evidence/cursor-cycle.json. A print illustration is not used as a pixel-exact cursor shape oracle.')
add('DOS CLS','P2','CLS clears pixels but leaves the prompt on row 24',dos+'Type CLS; wait 1 s.',
 'All old text disappears, but the new prompt cell starts at y=384 (row 24 with 16-pixel cells); ink is y=386..395. Top 384 rows are blank.',
 'The clean screen should put the prompt at the upper left (row 0): this homing expectation is FROM MEMORY. The manual printed p.45 (PDF 130) documents CLS leaving only a prompt and cursor, without giving framebuffer coordinates.',
 'shots/088-after-cls.png',[0,0,320,416],'refs/dos33-manual-pdfpage-130.png',[55,300,720,600],DOS+' printed p.45, PDF 130, Cls; row-0 homing expectation FROM MEMORY.',kind='manual')
add('DOS DIR header','P3','Directory listing omits the volume identity line',dos,
 'The DIR block begins with one Directory of A:\\ line at y=98..107. It has zero Volume in drive lines even though the pristine data disk is labeled INITECH.',
 'The illustrated DIR header has Volume in drive A is ... followed by Directory of A:\\: two identity/location lines.',
 'shots/087-dos-settled-dir.png',[0,80,360,120],'refs/dos33-manual-pdfpage-41.png',[345,756,895,806],DOS+' printed p.28, PDF 41, DIR example heading.',kind='manual',note='K10 covers wrong paths/ignored operands. This is the newly measured missing volume line; no operand test is re-filed.')
add('DOS DIR columns','P3','Name and extension are joined with a dot',dos,
 'README.TXT is one ten-character run at column 0, x=0..79 cell span. There is no separate extension column.',
 'The manual table separates basename and extension into aligned columns, e.g. COMMAND and COM, ANSI and SYS.',
 'shots/087-dos-settled-dir.png',[0,124,216,160],'refs/dos33-manual-pdfpage-41.png',[347,819,640,872],DOS+' printed p.28, PDF 41, first two DIR table rows.',kind='manual')
add('DOS DIR columns','P2','DIR rows omit date and time fields',dos,
 'Both directory entries contain only dotted name plus size or <DIR>. The right part x=200..639 y=128..159 is entirely background: 14080 pixels, no timestamp ink.',
 'The same table shows date and time after the size for each entry, e.g. 8-21-87 and 12:00p. There are five displayed fields: name, extension, size, date, time.',
 'shots/087-dos-settled-dir.png',[0,124,640,160],'refs/dos33-manual-pdfpage-41.png',[347,819,907,872],DOS+' printed p.28, PDF 41, first two DIR rows.',kind='manual',note='No claim about timestamp storage (earlier L030) is made; this is absent display columns.')
add('DOS DIR footer','P3','Footer omits the free-space figure',dos,
 'Footer is only 2 file(s), at row 11. There are zero bytes free tokens or free-space figures after the count.',
 'The continued DIR example ends with 37 File(s) and 17408 bytes free on the same footer line; the particular count/byte value depends on the disk.',
 'shots/087-dos-settled-dir.png',[0,174,420,192],'refs/dos33-manual-pdfpage-42.png',[200,521,665,544],DOS+' printed p.29, PDF 42, final DIR footer.',kind='manual',note='The sample byte value is not the expected free space of the Initech disk. The missing field is the finding.')
add('database LIST','P2','LIST lacks field headings',db+'Also try SET HEADING ON before LIST; wait 1 s.',
 'Three records display, but zero headings for NAME, CITY, BAL. The first record follows the LIST command directly. SET HEADING ON returns the already-known text error path.',
 'LIST sample prints field names above the rows (U5-144, PDF 256). SET HEADING is normally ON and supplies one column title for each field (U5-235, PDF 347).',
 'shots/077-db-heading.png',[0,42,400,130],'refs/dbase-manual-pdfpage-256.png',[190,800,1040,1100],DB+' printed U5-144, PDF 256, LIST example; printed U5-235, PDF 347, SET HEADING defaults.',kind='manual',note='The error string is not a new finding: K08 covers inappropriate unsupported-command errors. The appearance finding is the missing table-heading row.')
add('database status','P2','SET STATUS ON never produces the status bar',db+'Issue SET STATUS ON; wait 1 s.',
 'The bottom three native text rows (y=432..479, 30720 pixels) are all #6FA08E; no status or guidance text and no reverse band appear. SET STATUS ON returns a text error.',
 'The manual Figure 5-17 shows a status bar containing Command Line, drive/database and Rec: position/count, plus a guidance line. With STATUS ON it should be visible; U5-16 reserves terminal row 22 for status.',
 'shots/079-db-status.png',[0,416,640,480],'refs/dbase-manual-pdfpage-371.png',[338,584,1044,701],DB+' printed U5-259, PDF 371, Figure 5-17; U5-16, PDF 128, row allocation.',kind='manual',note='The illustrated blue is not graded as a framebuffer RGB oracle. The 30-row Initech console has no status band anywhere, not merely a band at a different bottom row.')
add('DOS DIR footer','P3','File-count label uses lowercase f',dos,
 'The actual footer spells 2 file(s), with lowercase f (a seven-character label; nine characters including count and space).',
 'The original manual example spells 37 File(s), capital F. The counter length varies but the label casing does not in that example.',
 'shots/087-dos-settled-dir.png',[0,174,84,192],'refs/dos33-manual-pdfpage-42.png',[208,521,363,544],DOS+' printed p.29, PDF 42, footer File(s).',kind='manual')

# The film photo is a presence/layout oracle only; its CRT pixels are not
# silently rescaled into native metrics.
film_source='/home/tobias/Projects/initech-os/spec/assets/preview.webp'
Image.open(film_source).convert('RGB').save(ROOT/'refs/film-reference-photo.png')
film_steps='Boot a fresh flair_live_interactive.img + flair_data.img copy, wait for FLAIR-LIVE-READY; move pointer clear to (500,380). Also inspect the sampled boot frames.'
add('film chart','P2','Film chart window is blank',film_steps,
 'The visible chart-window content patch x=306..537 y=142..197 has 12992 pixels, all #FFFFFF. It contains zero non-white chart pixels or percentage-label ink. All steady film captures have the same blank bodies.',
 'The supplied scene reference shows a blue shaded pie with percentage labels 40, 35, 18, 14 and 9, occupying photographed x=824..1128 y=211..457. The 116 percent sum is deliberate and is not flagged.',
 'shots/105-film-steady.png',[306,142,538,198],'refs/film-reference-photo.png',[820,207,1133,461],film_source+' x=824..1128 y=211..457 (1456x819 CRT photograph); user brief identifies flair_live_interactive.img as the film-scene image.',kind='illustration',note='Presence comparison only. The photo is not converted into native window dimensions or a palette target. Existing F16 concerns chrome/icon/font finish; this is missing application content.')
add('film worksheet','P2','Film worksheet body has no grid or expense data',film_steps,
 'The unobscured left document patch x=66..298 y=102..197 is 22368 entirely white pixels: zero row/column lines, labels or numbers. The blank document continues around the foreground saving modal.',
 'The scene reference shows an expense worksheet, with labels, numeric cells and repeated grid lines in photographed x=450..1064 y=450..642. This is the source of the deliberate 570- figure; no complaint about that figure value is made.',
 'shots/105-film-steady.png',[66,102,299,198],'refs/film-reference-photo.png',[450,450,1065,643],film_source+' x=450..1064 y=450..642 (CRT photograph); user brief film-scene expectation.',kind='illustration',note='This is the film underlying document, separate from the working disk-launched 123 worksheet. The two application bodies are different missing visuals; no native grid pitch is inferred from the photographed screen.')
add('film busy cursor','P3','Saving modal retains the arrow instead of the scene hourglass',
 'Boot film-scene copies; move the pointer inside the saving modal to (470,228), wait 0.4 s; capture.',
 'The pointer over the gray modal face is the normal arrow: 61 black and 53 white pixels in the 16x16 canvas x=469..484 y=227..242, the same counts as the idle desktop arrow. No hourglass appears.',
 'The saving state in the supplied reference shows an hourglass at photographed x=805..823 y=356..384. The user explicitly preserves that hourglass instead of the historical Mac wristwatch.',
 'shots/120-film-arrow-on-modal.png',[469,227,485,243],'refs/film-reference-photo.png',[803,353,827,386],film_source+' hourglass x=805..823 y=356..384; deliberate hourglass requirement in the user brief and spec/assets/cursors.h.',kind='illustration',note='The hourglass asset itself was not rendered by the guest, so its strike art remains ungraded. This finding is limited to the visible saving-scene cursor; an actual I/O busy transition was not reached.')

add('database numeric format','P2','LIST drops fixed decimal places for integral numeric fields',db+'Inspect BAL in record 2; DBF descriptor is N(8,2).',
 'The input record stores BAL as two spaces then -42.00 (eight bytes). LIST prints only -42: three character cells at x=264..287, ink x=264..286 y=82..91. The first and third records print 1234.50 and 7777.77 with two decimal places.',
 'The original LIST example prints the integral UNITCOST value 814 as 814.00. Its original TOURS.DBF descriptor is N(10,2); both it and BAL specify two decimal places. Width differs, but precision does not.',
 'shots/077-db-heading.png',[264,80,328,96],'refs/dbase-manual-pdfpage-256.png',[726,840,803,902],DB+' printed U5-144, PDF 256, UNITCOST heading and first two values. /home/tobias/Projects/dbase3-decomp/mint/work/TOURS.DBF UNITCOST descriptor; evidence/CLIENTS-input.DBF BAL descriptor/record 2, offsets in evidence/dbf-input-fields.json.',kind='manual',note='This is a plain stored field display, not a calculation-precision test. The DBF field metadata and stored bytes were read directly; no database mutation or real-dBASE invocation was invented.')
add('database numeric alignment','P3','LIST aligns numeric fields to their left edge',db+'Compare the right edges of the three BAL values.',
 'BAL ink boxes are x=265..319, x=264..286 and x=264..318 for rows 1/2/3 at y=66..75, 82..91 and 98..107. Values begin in the same column and the short negative value ends 32-33 px earlier. Each number sits in an eight-character field starting at x=264.',
 'The first two UNITCOST values in the LIST illustration start at x=745 and x=737, but both end at x=795 (scan threshold mean RGB <100, row bands y=858..879 and 881..901). The narrower number gains leading space; numeric right edges align.',
 'shots/077-db-heading.png',[256,64,336,112],'refs/dbase-manual-pdfpage-256.png',[716,856,805,902],DB+' printed U5-144, PDF 256, first two UNITCOST rows; rendered manual coordinates x=716..804 y=856..901. N(8,2) native field comes from evidence/dbf-input-fields.json.',kind='manual',note='Scanner pixels are used only to locate alignment in the illustration, not to equate its glyph dimensions to native VGA cells. This is separate from M027 precision loss: even a three-character displayed number should be right aligned in its numeric field.')

# Matching and existing-finding comparisons get the same transparent mechanics.
def extra(name,ap,ar,rp,rr,ink=False):
 a=pixels(ap,ar);b=pixels(rp,rr);a.save(ROOT/'diffs'/(name+'-initech.png'));b.save(ROOT/'refs'/(name+'-reference.png'))
 aa=np.array(a);bb=np.array(b)
 if ink: aa=np.where(np.all(aa==0,2)[:,:,None],0,255).repeat(3,2);bb=np.where(np.all(bb==0,2)[:,:,None],0,255).repeat(3,2)
 h=max(aa.shape[0],bb.shape[0]);w=max(aa.shape[1],bb.shape[1]);na=np.full((h,w,3),255,np.uint8);nb=na.copy();na[:aa.shape[0],:aa.shape[1]]=aa;nb[:bb.shape[0],:bb.shape[1]]=bb;dif=np.any(na!=nb,2)
 scale=max(1,min(6,1000//(3*w+36),400//h));colw=max(w*scale,180);out=Image.new('RGB',(3*colw+48,max(h*scale+42,100)),'#d0d0d0');dr=ImageDraw.Draw(out)
 ims=[Image.fromarray(na),Image.fromarray(nb),Image.fromarray(np.where(dif[:,:,None],np.array([220,0,70]),np.array([240,240,240])).astype('uint8'))]
 for i,(im,l) in enumerate(zip(ims,['Initech','Reference','Different pixels'])):
  x=12+i*(colw+12);dr.text((x,5),l,fill='black');out.paste(im.resize((w*scale,h*scale),Image.Resampling.NEAREST),(x,28))
 out.save(ROOT/'diffs'/(name+'.png'))
 return {'name':name,'a':ap,'a_rect':ar,'b':rp,'b_rect':rr,'pixels':w*h,'different_pixels':int(dif.sum()),'method':'ink' if ink else 'rgb'}
E=[]
for spec in [
 ('match-title','shots/001-desktop.png',[90,60,120,82],'refs/s8_doc_window_active.png',[100,70,130,92],False),
 ('match-bar','shots/001-desktop.png',[400,0,401,20],'refs/s8_desktop_menubar.png',[400,0,401,20],False),
 ('match-disabled-arrow','shots/004-root.png',[360,82,374,96],'refs/s8_doc_window_active.png',[461,113,475,127],False),
 ('match-well','shots/042-many-root.png',[360,150,374,151],'refs/s8_doc_window_inactive.png',[608,350,622,351],False),
 ('match-grow','shots/004-root.png',[359,259,376,276],'refs/s8_doc_window_active.png',[460,295,477,312],False),
 ('F16-close','shots/001-desktop.png',[64,64,77,77],'refs/s8_doc_window_active.png',[69,74,82,87],False),
 ('F16-zoom','shots/001-desktop.png',[327,64,340,77],'refs/s8_doc_window_active.png',[448,74,461,87],False),
 ('F16-collapse','shots/001-desktop.png',[343,64,356,77],'refs/s8_doc_window_active.png',[464,74,477,87],False),
 ('F16-body','shots/004-root.png',[20,150,26,180],'refs/s8_doc_window_active.png',[65,150,71,180],False),
 ('F16-HELLO','shots/001-desktop.png',[191,63,228,78],'refs/chicago-HELLO.png',None,True),
 ('F16-File','shots/001-desktop.png',[27,2,50,17],'refs/chicago-File.png',None,True),
 ('F16-New-Folder','shots/007-finder-file.png',[40,40,114,55],'refs/chicago-New-Folder.png',None,True),
 ('F16-folder','shots/004-root.png',[108,86,140,118],'refs/s8_doc_window_active.png',[236,116,268,148],False),
 ('F17-header','shots/004-root.png',[25,82,355,112],'refs/s8_doc_window_active.png',[71,92,401,122],False),
 ('L023-border','shots/108-trash-alert.png',[133,87,145,191],'refs/s8_alert_modal.png',[133,87,145,191],False),
 ('L022-caution','shots/108-trash-alert.png',[159,103,185,129],'refs/s8_alert_modal.png',[159,103,185,129],False),
 ('L117-repaint','shots/064-ctenant.png',[188,202,445,309],'shots/065-ctenant-moved.png',[238,252,495,359],False),
 ('fixed-H04','shots/106-trash-empty.png',[576,395,626,456],'shots/107-trash-full.png',[576,395,626,456],False),
 ('fixed-shutdown-text-click','shots/092-safe-before-click.png',[103,140,539,252],'shots/093-safe-after-text-click.png',[103,140,539,252],False),
]: E.append(extra(*spec))
# A true VGA-cell comparison for Initech 123: remove each ninth reference VGA
# column; normalize the deliberate white/navy adaptation into logical reverse.
r=np.array(pixels('refs/lotus-01_grid.png'))
# Panel READY: 5 cells, 9 pixels each vs current 8 pixels each.
r=r[0:16,675:720];r=r.reshape(16,5,9,3)[:,:,:8].reshape(16,40,3)
a=np.array(pixels('shots/028-123-empty.png',[578,62,618,78]))
# Reference cyan background and black text. Native navy background and white text.
ra=np.where(np.all(r==[0,0,0],2)[:,:,None],0,255).repeat(3,2).astype('uint8')
aa=np.where(np.all(a==[255,255,255],2)[:,:,None],0,255).repeat(3,2).astype('uint8')
Image.fromarray(ra).save(ROOT/'refs/lotus-ready-8px-normalized.png');Image.fromarray(aa).save(ROOT/'diffs/i123-ready-8px.png')
E.append(extra('i123-ready','diffs/i123-ready-8px.png',None,'refs/lotus-ready-8px-normalized.png',None,False))
# Film contact sheet: source PPMs have already been losslessly converted to PNG.
film=[x for x in json.loads((ROOT/'evidence/final-samples.json').read_text())['film_boot'] if x['ms']<650]
out=Image.new('RGB',(960,((len(film)+2)//3)*265),'#d0d0d0');dr=ImageDraw.Draw(out)
for i,x in enumerate(film):
 im=pixels('shots/'+x['file']);im.thumbnail((310,235));xx=(i%3)*320;yy=(i//3)*265;dr.text((xx+4,yy+3),f"{x['ms']} ms",fill='black');out.paste(im,(xx+4,yy+24))
out.save(ROOT/'evidence/film-boot-contact.png')
# Save machine-readable evidence, including exact crop inputs and count semantics.
(ROOT/'evidence/findings.json').write_text(json.dumps(F,indent=2))
(ROOT/'evidence/comparisons.json').write_text(json.dumps(E,indent=2))
(ROOT/'NITS.tsv').write_text('id\tarea\tseverity\tone-line summary\n'+''.join('\t'.join([f['id'],f['area'],f['severity'],f['summary']])+'\n' for f in F),encoding='ascii')

def link(p,label=None):return f'[{label or p}]({p})'
def lumi(c):
 v=[q/255 for q in c];v=[q/12.92 if q<=.04045 else ((q+.055)/1.055)**2.4 for q in v];return .2126*v[0]+.7152*v[1]+.0722*v[2]
contr=(1.05/(lumi((141,220,220))+.05),1.05/(lumi((84,84,179))+.05), (lumi((192,192,192))+.05)/(lumi((111,160,142))+.05))
cycles=json.loads((ROOT/'evidence/cursor-cycle.json').read_text());motion=json.loads((ROOT/'evidence/motion-samples.json').read_text())
metrics=json.loads((ROOT/'evidence/metrics.json').read_text())
report='''# Audit pass 6: InitechOS appearance

Image commit: 3aaf6e0 (images/COMMIT). Measurements below are from this session.
Read-only source HEAD seen at handoff: f7ed504a779ceaf85c39b5a03caf4bb459e00e62.
Guest behavior is attributed to the image commit, not that later source HEAD.
This is a visual audit, not a source-code repair or a build. The source/reference
repositories were read only. No input was sent to X display :0. Every guest
used a local disk copy, -display none, and QMP over stdio. At most one emulator
was active at a time; all emulators started by these scripts were quit by QMP.

The result is mixed. The title-band profile, many sampled grays, disabled
scroll arrows, page well and grow art are remarkably close. The body frames,
thumbs, several menu details, button rendering and console presentation keep
the screen from having the finish of the references. Existing findings retain
their IDs; new measured nits are M001 onward.

## Measurement conventions and references

All rectangles in the report prose are inclusive when written as x=a..b.
JSON/Python crop rectangles [left,top,right,bottom] are half open. PNGs retain
native guest pixels. Enlargements in diffs use nearest-neighbor scaling.
Pink marks differing RGB pixels; ink comparisons normalize non-black to white.
If crops have different dimensions they are padded with white, as recorded in
findings.json. Counts over different text inventories are context, not a
universal rendering score. Every new finding identifies its focal measurement.

The Mac goldens are sampled framebuffer RGB under Mac HiRes Std Gamma. This
pass compares the sampled domain, because Initech spec/chrome_metrics.json
also selects that domain. Nominal #CCCCCC/#DDDDDD are not mistaken for the
sampled #DADADA/#E7E7E7. The palette caveat is documented in
/home/tobias/Projects/system7-decomp/specs/sys8/platinum-palette.md, section 1.

Mac paths below mean real captures under
/home/tobias/Projects/system7-decomp/goldens/captures/. Their full copied PNGs
and the exact crops used are in refs/. System 7 Chicago/Geneva glyph oracles
come from the original NFNT 5478/5415 resources in goldens/resources/. Font
oracle PNGs are reference-only renderings of those resources; they are not
claimed to be additional guest states. Mac OS 8 uses Charcoal in these
captures; Chicago-vs-Charcoal belongs to existing F16.

Manual crops are rendered from original PDF pages at a maximum dimension of
1400 pixels. They establish fields, words and layout, not native VGA RGB.
Their comparison images deliberately have no pixel heat map. The 1456x819
film reference photo (spec/assets/preview.webp) is also used only for scene
content and cursor presence; no CRT-photo pixels become native chrome or
palette expectations. Expectations
without a captured historical state are explicitly marked FROM MEMORY.

The earlier five REPORT.md and TRIAGE.md pairs, plus the later
2026-10-05-astra-full-drive pair, were checked. Their path/hash/heading index
is evidence/prior-report-index.json. No previously reported issue is silently
renumbered as an M finding.

## 1. Per-area verdicts

| Area | Verdict | Measured result and reference |
| --- | --- | --- |
| Menu-bar geometry/palette | close | Both strips are 20 px. At clear x=400: white row, 17 #E7E7E7 rows, #B3B3B3 row, black row: 0/20 different against s8_desktop_menubar.png x=400 y=0..19. Corners match the reference where not covered by the left-shifted Apple. Left slot and highlight are M001/M002. |
| Pull-down geometry | close | Finder File panel is x=20..174 y=39..245: 155x207, for 12 ordinary items and two separators. Reference File is x=33..230 y=19..278: 198x260, for 15 items/three separators. Inventory differences are not a panel-size bug. Ordinary rows are 16 px, separators 6 px; cap top is panel top+4 in both. Frame 1 px, inner bevel white/#B3B3B3, drop shadow #3F3F3F. Disabled ink #A5A5A5 and etched separator #A5A5A5/white match. M003-M006 cover shortcut/mark/ink placement. |
| Active title band | matches | 22 rows: black, white, two #DADADA, 12 alternating white/#969696, four #DADADA, #B3B3B3, black. 0/660 differing pixels in the stripe crop (90,60)-(120,82) against s8_doc_window_active (100,70)-(130,92). Six light/six dark stripes. HELLO light-row runs x=81..185 and 232..321 shift to x=82..186 and 233..322 on dark rows. Its light text gap x=186..231 has 6/5 px padding around ink x=192..226, and shifts +1 on dark rows, matching the Mac rule. |
| Close/zoom/collapse art | close | 12x12 boxes plus highlight footprint 13x13, at top+4, close left+4, zoom right-32, collapse right-16. Aligned 13x13 crops each differ at only 2/169 pixels (interior off-diagonal corners #A5A5A5 instead of #DADADA). This is residual F16, not three new widget findings. Mac s8_doc_window_active x=69/448/464 y=74. |
| Active body frame | wrong | At root y=200, x=20..25 is black then five whites. Reference y=150 x=65..70 is black, white, #DADADA, #DADADA, #B3B3B3, black. Four raised bar columns and the inner line are lost in visible body painting: existing F16. |
| Shadow/inactive chrome | close | Active one-pixel right/bottom black shadow and two-pixel near-corner notch are present. Inactive title is flat #E7E7E7 with #878787 ink, frame/shadow #777777, widgets absent; bars hollow #F3F3F3. Geometry is 22 px in both active and inactive. Reference s8_doc_window_inactive inactive window x=13..325 y=29..198. Different window dimensions/titles prevent a whole-window equality score. |
| Disabled scroll controls | matches | Total band 16 px, interior 14 px, 14x14 arrow tile, 20 #A5A5A5 arrow pixels in 8x4 triangle. Entire upper tile matches: 0/196 RGB differences, current (360,82)-(374,96), Mac active-window disabled tile (461,113)-(475,127). |
| Enabled scroll controls | wrong | Arrow tile 194/196 pixels match (M013). Well cross-section is identical, 0/14: #969696,#A5A5A5, ten #C0C0C0,#CDCDCD,#DADADA. Thumb is 14x15 but flat, with no dark grips (M007/M008). Inactive bars have no arrows/thumb/separators, matching the hollow state. Resize invalidation still shows J01. |
| Grow box | matches | 17x17 aligned crop matches all 289 pixels. Both have three diagonal lines, 18 #969696 grip pixels, 3 #C0C0C0 termination pixels, 54 whites and 183 #DADADA face pixels. Mac crop x=460..476 y=295..311; current x=359..375 y=259..275. |
| Chicago 12 | close | HELLO: 37 px advance, 142 ink pixels, 0/555 normalized-ink differences. File: 23 px advance, 83 vs 84 ink, 1/345 differences. New Folder: 74 px advance, 262 vs 250 ink, 34/1110 differences. Caps use 9 rows; menu cell top 2, baseline 14 (12 ascent). Font choice versus Mac 8 Charcoal remains F16. Bold/italic were not reachable. |
| Geneva 9 / icon labels | wrong | README.TXT advance is 64 px in the shipped strike vs 58 in NFNT 5415. Observed ink is 63x8 (146 pixels) vs 57x7 (134 pixels) in the oracle. Current label ink y=122..129 below icon body y=88..115. Selection is black field/white text and does not invert the entire window. Existing F16 covers font/label/art fidelity; no duplicate M font finding. |
| Icons / desktop | close | Deliberate desktop is a uniform #8DDCDC in the live FLAIR captures; no checker-pattern defect is filed. Folder/document/application/disk art is still sparse versus Mac crops (F16). Full Trash now differs by 75 pixels from empty Trash in a clean 50x61 patch; H04 is not still present. Unlabeled SAMIR volume label is blank (K14). |
| Dialog/alert geometry | wrong | Quit/Trash frame x=133..506 y=87..190 is 374x104, as the alert golden. Its black band is 7 px vs the golden's 1 px outline plus 2 bevel rows (L023). Caution area 26x26 is empty (L022). Buttons are 59x20 and default ring 65x26, +3 on each side; label erasure and corner fringe are M011/M012. No alert-offset defect was observed. |
| Film saving dialog | close | FILE COPY is x=140..499 y=200..279, 360x80, 22-pixel striped title. Message ink begins (155,228). Progress outer rect x=154..485 y=249..268, 332x20; filled interior is 224 px of a 330-px interior (4032 navy pixels = 224x18). These dimensions are authored Initech dialog.c/spec values, not a Mac captured progress-control oracle. The underlying chart/worksheet are blank (M024/M025); percentage and 570- value fidelity cannot be graded. |
| Pointer / text and busy cursors | close | Arrow hot point aligns with the tip but mask/stem differ (M009). No editable Macintosh text field was reachable, so I-beam comparison is uncovered. The steady film modal uses the arrow; the film saving state retains that arrow (M026). The hourglass strike itself and an actual I/O busy transition were not reached. No wristwatch replacement is proposed. |
| DOS screen | wrong | 640x480 graphics console, 8x16 cells (80 columns; 30 cell rows inferred from dimensions), #C0C0C0 on #6FA08E. Prompt, CLS, cursor and DIR are measured in M014-M020/M023. VGA-like fixed glyphs are plausible, but the original DOS manual is not a native font bitmap oracle. No 30-row legality claim is made. |
| Initech 123 | close | Three 16-pixel control-panel rows before column border y=110..125. Border 4 character cells; column widths 9 cells (72 px native, 81 px in the real VGA capture). Eight columns A..H; native shows 19 rows versus real 20 because of its window/chrome. Mode is 5 cells at x=578..617 y=62..77. Pointer D1 is 72x16 at x=250..321 y=126..141. These are coherent native adaptation choices, not new bugs. READY ink differs at 12/640 pixels after removing the real VGA ninth columns and normalizing reverse colours (diffs/i123-ready.png). The clock/UNDO status area is blank (L021). |
| SAMIR/database | wrong | Dot-prompt, three LIST records and database return were reached. Missing headings/status and shared palette are M014/M021/M022; fixed precision/right alignment are M027/M028. The question output leaves 42. with prompt attached (L046). Full-screen BROWSE failed via the known unsupported workflow; no editor/Assistant pixel-parity claim is made. |
| Motion | close, limited | Menu/drag/scroll samples and exact host times are in evidence/motion-samples.json. Menus stayed open under held input; drag outline stayed while held; one transient partial repaint appeared on release. The native collapsed title band is byte-identical to the expanded band (shots 113-115; evidence/collapse-check.json), as the Mac collapsed capture describes. No real Mac video/timing oracle exists in this corpus, so latency/flicker observations are not converted into unsupported historical findings. |

'''
report+=f'''The white-to-selected-title contrast is {contr[0]:.2f}:1 for current #8DDCDC,
versus {contr[1]:.2f}:1 for the captured #5454B3 face. Console text/background
contrast is {contr[2]:.2f}:1. These are computed from measured sRGB values;
they quantify the washed-out appearance, not a modern accessibility-policy audit.

Matching and residual crops: {link('evidence/comparisons.json')}; focal numeric
samples: {link('evidence/metrics.json')}; visual overview:
{link('evidence/chrome-contact.png')}.

## 2. New measured nits

'''
for f in F:
 report+=f"### {f['id']} - {f['summary']}\n\nArea: {f['area']}. Severity: {f['severity']}.\n\nState and steps: {f['state_steps']}\n\nInitech draws: {f['initech']}\n\nReference: {f['reference']}\n\nSource: {f['provenance']}\n\n"
 if f['note']:report+=f"Qualification: {f['note']}\n\n"
 metric=f['metric']
 if metric:report+=f"Comparison: {metric['different_pixels']}/{metric['comparison_pixels']} differing {metric['method']} pixels in the displayed, documented crops. "
 else:report+='Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. '
 report+=f"{link(f['diff'],'Diff/comparison image')}; {link('diffs/'+f['id']+'-initech.png','guest crop')}; {link('refs/'+f['id']+'-reference.png','reference crop')}. Crop coordinates are in {link('evidence/findings.json')}.\n\n"
report+='''## 3. Still present (existing IDs only)

Only freshly observed IDs are included. This is not a status review of all
functional findings in the earlier reports.

| Existing ID | This session's evidence |
| --- | --- |
| F13 | The Apple slot does not open a menu: shots/002-system-menu.png, desktop action log. No real Apple menu panel could be compared. |
| F16 | Residual body frame, typography/Geneva labels, sparse icon art and the two-pixel widget corner differences: diffs/F16-body.png, F16-File.png, F16-New-Folder.png, F16-folder.png, F16-close/zoom/collapse.png, metrics.json. Chicago HELLO now matches its strike completely; the broad original typography description is not copied forward unchanged. |
| F17 | No Finder item-count/free-space header: shots/004-root.png and diffs/F17-header.png. The Mac folder capture has its header between title y=91 and content separator y=112 (20 face rows). |
| J01 | Horizontal bar remains enabled-looking after grow restores the 360x220 root: shots/051-many-grow.png and 118/119; evidence/bars-proof-serial.log has horizontal max=134 after shrink, grow back to 360x220, then FLAIR-SCROLL-IGNORED win 0 h while the thumb still shows. This is the known resize-state invalidation finding, not a new thumb-range finding. |
| K08 | BROWSE remains an error instead of a screen: shots/080-db-browse-unreached.png. Unsupported SET errors seen during appearance checks retain the earlier error-workflow ID. |
| K14 | Blank label on the unlabeled SAMIR volume: shots/112-unlabeled-disk.png. |
| L004 | Trash Cancel dismisses on mouse-down: shots/109-trash-cancel-down.png already shows the underlying desktop before release; 110 is after release. The early 103 quit-button attempt did not have an alert up and is not evidence. No pressed-button look state remains to compare. |
| L021 | Worksheet status line stays blank: shots/028-123-empty.png and 029-123-row1.png, bottom content row y=430..445; real lotus-01_grid.png has clock/date and UNDO at y=384..399. No new clock nit is filed. |
| L022 | Caution area is still blank in Empty Trash: diffs/L022-caution.png, shots/108-trash-alert.png. |
| L023 | Seven-pixel black alert border remains: diffs/L023-border.png, shots/108-trash-alert.png. |
| L046 | ? 42 leaves output and next prompt joined as 42.: shots/111-db-query-prompt.png after a one-second settle. |
| L117 | CTENANT readouts disappear after a 50x50 title drag: shots/064-ctenant.png / 065-ctenant-moved.png and diffs/L117-repaint.png. |

Fresh checks which do NOT support still-present claims: H04 (Trash changes
by 75 pixels after staging); H06 (zoomed TenantFix gutter strip x=21..405,
y=456..458 contains 1152 #F3F3F3 pixels and 3 inactive-outline pixels, not
white overpaint); F05 (safe screen appears and its Restart button boots the desktop; the Special > Restart menu was not retested); L008 (quit warning
appears and cancels); L010 (slash menu is visible in shot 101); L011 (Save
prompt is reached in shot 102, but no on-disk save was needed for this look
pass). These are limited retests, not blanket completion claims.

Two tempting observations were rejected during verification: an apparent
192-pixel alert offset, and a shutdown panel apparently vanishing after a
text click. Native pixel scans disprove both: alert black rows are always
y=87..93 and 184..190; shutdown before/after text-click images are byte
identical. Neither appears in NITS.tsv.

## 4. Coverage and limitations

Compared at native pixel level: both bar strips; a settled upper File panel (121) and
Finder File/View/Special menus; active/inactive HELLO, NOTES, root, APPS,
TenantFix, CTENANT and 123 window chrome; active widgets, held close/zoom,
window outline drag, grow/zoom, disabled/enabled/inactive scrollbars,
thumb drag, arrow hold/repeat, narrow-title truncation; icon/label/selection
surfaces and empty/full Trash; Empty Trash and changed-worksheet alerts;
default ring and Cancel art; film modal/progress surface; shutdown message
and its Restart button; DOS boot/VER/DIR/TYPE/PROMPT/CLS; SAMIR LIST/query/status and
return; Initech 123 empty sheet, row-1 data, LABEL entry, slash menu, Save
prompt and changed-work quit warning.

Reference pairing included Mac OS 8 active/inactive/collapsed/alert/menu/
Appearance controls captures, System 7 menu/window captures, original Chicago
12/Geneva 9 strikes, real Lotus MINT01 grid/entered/save screens, and the
original DOS/dBASE manual pages. Full reference screenshots and small crops
are preserved, with original source hashes in refs/manifest.json.

Not reached or not backed by a matching golden:

- Real Apple menu panel in Initech; a working About box, menu hierarchy,
  alternate Finder views, editable Macintosh text field, checkbox/radio/
  popup preference panels, or a standard Open/Save chooser.
- A live I-beam or rendered hourglass strike; bold/italic text styles; a same-title
  truncated Mac window. The ellipsis expectation is therefore marked memory.
- A real Mac OS 8 pressed arrow/thumb/button capture. The local sys8 specs
  explicitly mark pressed states and selected-menu-item fills as golden gaps.
  System 7 pressed captures were inspected for context, not silently used
  as Mac 8 pixel oracles. Current selected items/held controls are captured. The Trash Cancel commits before a pressed-state snapshot can be retained (L004).
- Pie art and 570- in the supplied film guest: steady captures 038/105 and
  35 boot samples show white underlying windows plus the saving modal. A
  boot contact sheet is evidence/film-boot-contact.png. M024/M025 record the missing application bodies; their deliberate percentages/figure values remain ungraded. M026 records the absent saving-scene hourglass. The supplied reference photo is a presence oracle, never a source of native dimensions.
- Database BROWSE/EDIT/ASSIST screens, because the reached BROWSE command
  failed. No real dBASE screen PNGs were present in that decomp corpus, so
  manual illustrations establish layout and default colours; no full-screen
  RGB diff against an invented dBASE capture was performed.
- The DOS manual does not supply a native VGA font/palette strike. Its
  printed examples are used only for prompt/cursor/field layout. The Mac
  palette RGBs are exact; manual palette statements are semantic colours.
- Original Mac shutdown screen coordinates were not captured in the corpus.
  The user-authorized computer wording is preserved and unflagged. The screen
  is measured (436x112 panel footprint at x=103..538 y=140..251, Restart ring at
  x=465..529 y=220..245); its location is authored product layout, not a
  claimed historical Mac measurement.

No panic or hang was observed in these sessions. No destructive source-repo
work, builds, make targets, commits, tracker writes or unrelated-process kills
were performed. Changed worksheet data and Trash staging were confined to
copied disks.

'''
report+=f'''Motion observations (host wall time, QMP sampling limits): first menu sample
was at {motion['menu_down'][0]['ms']:.1f} ms and showed a partial update; the
fully highlighted panel was sampled at {motion['menu_down'][1]['ms']:.1f} ms
and stayed identical through {motion['menu_down'][-1]['ms']:.1f} ms. Held drag
outline stayed stable from {motion['window_drag_held'][1]['ms']:.1f} through
{motion['window_drag_held'][-1]['ms']:.1f} ms. On release the outline remained
in early samples, one partial repaint was captured, and the final frame was
stable from {motion['window_drag_release'][3]['ms']:.1f} ms onward. Scroll
trace confirms a 16-pixel step; all ten post-click frames were identical.
This sampling can reveal intermediate blits, but cannot prove what a
period CRT user perceived or set a historically correct drop latency.

DOS cursor series: {len(cycles)} frames, first {cycles[0]['ms']:.1f} ms to last
{cycles[-1]['ms']:.1f} ms, {len(set(x['sha256'] for x in cycles))} distinct pixel
hash(es). The earlier three CLS frames are also byte identical.

## 5. Twenty look fixes I would make first

This ranks visible repairs, including existing IDs; it does not refile them.

1. Restore the four-column raised body border and inner outline before content
   paint (F16). It affects almost every window.
2. Give enabled thumbs real light/face/dark edge shades (M007).
3. Restore the four visible dark thumb grips (M008).
4. Use a dark selected-title face with enough white-text contrast (M002).
5. Replace the seven-pixel alert slab with the captured outline/bevel (L023).
6. Preserve button top/bottom bevels when painting labels (M011).
7. Match Geneva 9 label advances and ink weight (F16; README is 64 vs 58 px).
8. Restore the film chart and expense-sheet content (M024/M025), plus the saving-scene hourglass (M026).
9. Draw the caution icon in its reserved alert area (L022).
10. Repaint scrollbar range/state after every resize (J01).
11. Restore a black console background and appropriate light text (M014).
12. Draw a blinking ready cursor and home CLS to row 0 (M015/M016).
13. Add the database heading row and status strip, preserving numeric precision/right alignment (M021/M022/M027/M028).
14. Show DIR date/time fields and align basename/extension columns (M018/M019).
15. Add Finder's item count and available-space header (F17).
16. Restore the 123 clock/date/UNDO status content (L021).
17. Increase the Apple slot margin to match the real bar (M001).
18. Draw Command/check symbols and align the shortcut column (M003/M004/M006).
19. Finish folder/document/app/disk icon art (F16) and preserve CTENANT's visible state during repaint (L117).
20. Replace the remaining small control/cursor corner differences, then the
    ellipsis and DIR header/footer polish (M009/M010/M012/M013/M017/M020/M023).

## Evidence and replay

NITS.tsv has one row per new nit. evidence/findings.json records state,
source, coordinates, difference count and qualifications for every row.
evidence/comparisons.json covers matching/known/fixed comparisons. Full guest
captures are in shots/; reference images/crops in refs/; heat maps and illustration
comparison panels in diffs/. The input CLIENTS.DBF field descriptors (NAME,
CITY, BAL; three records) are in evidence/dbf-input-fields.json.

Scripts: audit_driver.py (adapted from the read-only pass-5 text_driver.py),
prepare_fixtures.py, replay_visual.py, measure.py, motion_capture.py,
finish_capture.py, cursor_samples.py, chrome_finish.py, last_capture.py, upper_menu_check.py, build_report.py, validate_evidence.py. replay_visual.py makes
fresh copies and covers the principal surfaces; evidence/actions.jsonl is
the detailed timestamped record of this session's input.
Some early filenames name the state being attempted rather than achieved;
evidence/screenshot-notes.json identifies those attempts, and they are not
used as positive findings. Capture scripts
have finally cleanup and QMP quit. Logs/session image hashes are in evidence/.

Run python3 prepare_fixtures.py, then python3 replay_visual.py for a fresh
visual replay. The motion/cursor scripts add denser temporal samples. Running
build_report.py regenerates this report and its crops from the existing PNGs.
It does not boot or build anything. validate_evidence.py checks the generated
artifacts and measured focal values; evidence/final-validation.json is the
check result. Source image filenames remain pristine;
all writable guest copies have separate label-boot/label-data names.
'''
report.encode('ascii')
(ROOT/'REPORT.md').write_text(report,encoding='ascii')
# Inventory and source provenance after producing every crop.
refs=[]
for p in sorted((ROOT/'refs').glob('*.png')):
 refs.append({'file':str(p.relative_to(ROOT)),'width':Image.open(p).width,'height':Image.open(p).height,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
originals=[]
for p in sorted(Path(MAC).glob('*.png')):
 originals.append({'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
for p in [Path(NFNT),Path(NFNT).with_name('NFNT_5415.bin'),Path(DOS),Path(DB),Path(LOT+'01_grid.png'),Path(LOT+'02_entered.png'),Path(LOT+'03_save_prompt.png'),Path(film_source),Path('/home/tobias/Projects/dbase3-decomp/mint/work/TOURS.DBF')]:
 originals.append({'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(ROOT/'refs/manifest.json').write_text(json.dumps({'originals':originals,'local_images':refs,'new_finding_crops':C,'other_crops':E},indent=2))
shots=[]
for p in sorted((ROOT/'shots').glob('*.png')):
 im=Image.open(p);shots.append({'file':str(p.relative_to(ROOT)),'size':list(im.size),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(ROOT/'evidence/screenshot-index.json').write_text(json.dumps(shots,indent=2))
source_images=[]
for n in ['flair_tenants_interactive.img','flair_data.img','samir_list.img','tracer_boot.img','flair_live_interactive.img']:
 p=ROOT/'images'/n;source_images.append({'path':'images/'+n,'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(ROOT/'evidence/source-image-manifest.json').write_text(json.dumps({'commit':(ROOT/'images/COMMIT').read_text().strip(),'images':source_images},indent=2))
print(f'Wrote {len(F)} findings, {len(E)} other comparisons, {len(shots)} screenshots; REPORT ASCII verified.')
