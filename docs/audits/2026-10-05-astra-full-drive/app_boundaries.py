from audit_driver import Driver,ROOT
import time,json
D=Driver()
try:
 D.start('app-boundaries','images/flair_tenants_interactive.img','images/db-fixture.img')
 D.click(600,64,2)
 # DB disk root: SAMIR.COM, CLIENTS.DBF, APPS; third slot.
 D.click(192,101,2);D.click(211,124,2);D.pause(.7);D.shot('ab-123-start')
 D.type('Keep me');D.key('right');D.key('backslash');D.pause(1);D.shot('ab-backslash-launches-database')
 D.type('QUIT');D.pause(1);D.shot('ab-123-resumed')
 D.key('ctrl-q');D.click(143,124,2);D.pause(.4);D.menu(38,28,60,48);D.pause(.7);D.shot('ab-ctenant-menu-quit')
 D.click(211,124,2);D.pause(.4);D.menu(38,28,60,48);D.pause(.7);D.shot('ab-123-menu-quit')
 D.click(75,124,2);D.pause(.4);D.menu(140,28,174,48);D.pause(.4);D.menu(177,28,204,48);D.pause(.7);D.shot('ab-tenant-info')
 D.key('alt-tab');D.pause(.7);D.shot('ab-alt-tab')
 D.click(430,130);D.pause(.3);D.click(250,230);D.pause(.7);D.shot('ab-inactive-click-exit')
finally:
 if D.proc:D.quit()
