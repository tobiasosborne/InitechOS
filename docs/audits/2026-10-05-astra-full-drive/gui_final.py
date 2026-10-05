from audit_driver import Driver,ROOT
import time,json
D=Driver()
try:
 D.start('gui-final','images/flair_tenants_interactive.img','images/flair_data.img')
 D.click(430,130)
 for k,(x,rows) in enumerate([(18,[48,64,80,96]),(55,[48,64,80,96]),(100,[48])]):
  D.move(x,28);D.button(True);D.pause(.4);D.shot('gf-notes-'+str(k));D.button(False)
  for y in rows:D.menu(x,28,x+20,y)
 D.type('text should appear',False);D.key('ctrl-s');D.key('ctrl-q');D.pause(.5);D.shot('gf-notes-after')
 D.click(600,64,2);D.click(54,101,2);D.key('ctrl-o');D.pause(.5);D.shot('gf-readme-open')
 D.click(123,101,2);D.click(75,124,2);D.click(143,124,2);D.pause(.5);D.shot('gf-second-disk-app-refused')
 D.click(250,170);D.key('alt-tab');D.pause(.5);D.shot('gf-alt-tab')
 D.menu(140,28,174,48);D.menu(177,28,204,48);D.pause(.5);D.shot('gf-tenant-info')
 D.key('ctrl-q');D.click(143,124,2);D.menu(38,28,55,48);D.pause(.5);D.shot('gf-ctenant-menu-quit')
 D.click(211,124,2);D.menu(38,28,60,48);D.pause(.5);D.shot('gf-123-menu-quit')
 # Last Finder close, geometry persistence, and foreground.
 D.key('ctrl-w');D.drag(370,270,-140,-90);D.pause(.5);D.shot('gf-size-before-close');D.key('ctrl-w');D.click(600,64,2);D.pause(.5);D.shot('gf-size-reopen');D.key('ctrl-w');D.pause(.5);D.shot('gf-last-finder-closed')
finally:
 if D.proc:D.quit()
