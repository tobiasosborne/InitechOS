from audit_driver import Driver
D=Driver()
try:
 D.start('trash-collision','images/flair_tenants_interactive.img','images/trash-fixture.img')
 D.click(600,64,2);D.drag(54,153,207,-52);D.pause(1);D.shot('129-name-collision-refused')
 D.drag(54,153,546,269);D.click(261,101,2);D.drag(75,124,525,298);D.pause(.5)
 D.click(600,422,2);D.pause(1);D.shot('130-trash-two-same-names')
 D.menu(172,28,200,65);D.pause(.5);D.shot('131-trash-collision-count');D.key('esc');D.pause(.5)
 D.key('ctrl-w');D.key('ctrl-w');D.key('ctrl-w');D.click(600,64,2);D.click(192,101,2);D.drag(200,90,350,-20)
 D.drag(192,101,233,3);D.pause(1);D.shot('132-cycle-refused')
finally:
 if D.proc:D.quit()
