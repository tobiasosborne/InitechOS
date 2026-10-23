#!/usr/bin/env python3
"""Explicit QMP make/break pairs, avoiding HMP sendkey timer overlap."""
import pathlib,importlib.util
R=pathlib.Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('driver',R/'audit_driver.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);d=m.Driver()
keys=list('abcdefghijklmnopqrstuvwxyz0123456789')+['up','down','left','right','home','end','pgup','pgdn','tab','ret','esc','backspace','delete','insert']+[f'f{i}' for i in range(1,13)]+['spc','minus','equal','bracket_left','bracket_right','backslash','semicolon','apostrophe','comma','dot','slash','grave_accent']
try:
 d.start('directinputs','images/flair_tenants_interactive.img','images/flair_data.img')
 d.click(600,64,2)
 for mod in ['ctrl','alt','shift']:
  d.keyheld(mod,True)
  for key in keys:
   if mod=='ctrl' and key in ['a','n','o','w','y']:continue
   d.keyheld(key,True);d.keyheld(key,False)
  d.keyheld(mod,False);d.pause(.3);d.shot('244-direct-'+mod)
 d.key('ctrl-n',.3);d.shot('245-direct-command-check')
 d.key('ctrl-w',.3);d.shot('246-direct-close-check')
 d.click(210,70)
 for i,x in enumerate([20,62,110,170,225,280,340,400]):
  d.heldmenu(x,29,'250-hello-'+str(i));d.selectmenu(x,29,47);d.selectmenu(x,29,63)
 d.click(410,130)
 for i,(x,rows) in enumerate([(20,[47,63,79,95]),(62,[47,63,79,95]),(110,[47])]):
  d.heldmenu(x,29,'260-notes-'+str(i))
  for row in rows:d.selectmenu(x,29,row)
 d.shot('263-notes-end')
 d.click(600,64,2)
 for x in [53,96,142,200]:d.selectmenu(x,10,27);d.selectmenu(x,10,43)
 d.shot('264-upper-noop');d.click(0,0);d.click(639,479);d.shot('265-corners')
finally:
 if d.proc is not None:d.quit()
