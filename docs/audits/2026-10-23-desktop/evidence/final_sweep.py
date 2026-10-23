#!/usr/bin/env python3
"""Final input, scrolling, built-in menus and many-file Trash boundary sweep."""
import importlib.util,pathlib,sys
from PIL import Image
ROOT=pathlib.Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('audit_driver',ROOT/'audit_driver.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m);d=m.Driver()
try:
 d.start('inputfinish' if '--resume' in sys.argv else 'inputfinal-replay','images/flair_tenants_interactive.img','images/flair_data.img')
 if '--resume' not in sys.argv:
  d.click(600,64,2);d.key('ctrl-n');d.drag(369,269,0,-86);d.shot('220-one-overflow')
  d.click(367,165);d.shot('221-one-overflow-down')
  im=Image.open(ROOT/'shots/221-one-overflow-down.png').convert('RGB')
  ys=[y for y in range(97,158) if im.getpixel((367,y))==(141,220,220)]
  if ys:
   y=(min(ys)+max(ys))//2;d.note('no-motion thumb probe at 367,'+str(y));d.click(367,y)
  d.shot('222-thumb-no-motion');d.drag(369,183,0,86);d.shot('223-grow-bars')
  d.click(369,70);d.shot('224-collapsed');d.click(369,70)
  d.move(200,70);d.button(True);d.key('x');d.key('shift-a');d.key('ctrl-n');d.rel(1,1);d.button(False);d.pause(.2);d.shot('225-keys-during-drag')
  d.drag(201,71,-1,-1)
  d.click(126,102);d.pause(2);d.click(126,102);d.shot('226-slow-double')
  d.move(126,102)
  for i in range(3):d.button(True);d.pause(.025);d.button(False);d.pause(.025)
  d.pause(.3);d.shot('227-fast-triple');d.key('ctrl-w')
 else:
  d.click(600,64,2);d.key('ctrl-n')
 keys=list('abcdefghijklmnopqrstuvwxyz0123456789')+['up','down','left','right','home','end','pgup','pgdn','tab','ret','esc','backspace','delete','insert']+[f'f{i}' for i in range(1,13)]+['spc','minus','equal','bracket_left','bracket_right','backslash','semicolon','apostrophe','comma','dot','slash','grave_accent']
 for mod in ['ctrl','alt','shift']:
  d.click(600,64,2)
  for key in keys:
   if mod=='ctrl' and key in ['a','n','o','w','y']:continue # exercised as commands in other sessions
   d.key(mod+'-'+key,.04)
  d.shot('228-'+mod+'-key-sweep')
 for x in [53,96,142,200]:
  d.selectmenu(x,10,27);d.selectmenu(x,10,43)
 d.shot('229-upper-quit-noop')
 d.key('ctrl-w');d.click(210,70)
 for x in [20,62,110,170,225,280,340,400]:
  d.heldmenu(x,29,'230-hello-menu-'+str(x));d.selectmenu(x,29,47);d.selectmenu(x,29,63)
 d.click(410,130)
 for x,rows in [(20,[47,63,79,95]),(62,[47,63,79,95]),(110,[47])]:
  d.heldmenu(x,29,'231-notes-menu-'+str(x))
  for row in rows:d.selectmenu(x,29,row)
 d.shot('232-notes-final');d.click(0,0);d.click(639,479);d.shot('233-screen-corners')
 d.quit()
 d.start('manytrash','images/flair_tenants_interactive.img','images/edge-data.img')
 d.click(600,64,2);d.drag(194,154,406,266);d.click(600,420,2)
 d.selectmenu(200,29,64);d.shot('234-many-trash-confirm')
 d.move(391,168);d.button(True);d.pause(.4);d.shot('235-cancel-held');d.rel(0,70);d.button(False);d.shot('236-cancel-drag-off')
 d.key('esc');d.selectmenu(200,29,64);d.click(464,168);d.shot('237-many-trash-emptied')
 d.heldmenu(200,29,'238-empty-twice-disabled')
finally:
 if d.proc is not None:d.quit()
