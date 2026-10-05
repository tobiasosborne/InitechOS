from audit_driver import Driver,ROOT
import time,json
D=Driver();results=[]
def snap(n):D.pause(.5);D.shot(n)
def choices(tag,x,y,rows):
 D.move(x,y);D.button(True);snap(tag+'-held');D.button(False)
 for row in rows:
  before=D.serial.stat().st_size;D.menu(x,y,x+30,row);D.pause(.2)
  results.append({'menu':tag,'title':[x,y],'row':row,'trace':D.serial.read_bytes()[before:].decode(errors='replace')})
try:
 D.start('menu-sweep','images/flair_tenants_interactive.img','images/flair_data.img')
 # All 8 HELLO menus, both offered items in each.
 for i,x in enumerate([18,54,103,149,198,241,292,371]):choices('m-hello-'+str(i),x,28,[48,64])
 D.click(430,130);D.type('menu audit',False)
 for i,(x,rows) in enumerate([(38,[48,64,80,96]),(77,[48,64,80,96]),(122,[48])]):choices('m-notes-'+str(i),x,28,rows)
 snap('m-notes-after-all')
 D.click(600,64,2);D.click(54,101)
 # Every nonseparator Finder row. Close/New/Open also tested separately.
 choices('m-finder-file',38,28,[80,118,134,150,166,188,204,220,236])
 choices('m-finder-edit',76,28,[48,64,80,96,112,128,150,172])
 choices('m-finder-view',119,28,[48,64,80,96,112,128,144,160,176,198,214])
 choices('m-finder-special',172,28,[48,64,86,108,124,140])
 choices('m-finder-help',222,28,[48,64])
 choices('m-finder-apple',9,28,[48])
 for i,x in enumerate([38,77,120,171]):choices('m-system-'+str(i),x,10,[30,46])
 D.click(500,380,1,'right');D.keyheld('ctrl',True);D.click(54,101);D.keyheld('ctrl',False);snap('m-context-paths')
 # No-modifier keys and release-inside/outside controls, edges.
 D.keyheld('ctrl',True);D.keyheld('ctrl',False);D.key('n');D.key('w');snap('m-modifier-clear')
 D.move(31,70);D.button(True);D.pause(3);snap('m-close-held');D.rel(1,35);D.button(False);snap('m-close-cancelled')
 D.drag(200,70,-500,-300);snap('m-window-edge');D.drag(50,50,300,200);snap('m-window-recovered')
finally:
 if D.proc:D.quit()
 (ROOT/'evidence/menu-sweep-results.json').write_text(json.dumps(results,indent=2))
