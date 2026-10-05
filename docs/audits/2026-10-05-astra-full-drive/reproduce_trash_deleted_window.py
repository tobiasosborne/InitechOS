from audit_driver import Driver
D=Driver()
try:
 D.start('trash-independent','images/flair_tenants_interactive.img','images/trash-fixture.img')
 D.click(600,64,2);D.drag(192,101,408,321);D.click(600,422,2);D.click(75,124,2)
 D.menu(172,28,200,65);D.pause(.5);D.shot('trash-independent-alert');D.key('ret');D.pause(1);D.shot('trash-independent-deleted-window')
 D.key('ctrl-n');D.pause(1);D.shot('trash-independent-orphan')
finally:
 if D.proc:D.quit()
