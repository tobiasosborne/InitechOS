#!/usr/bin/env python3
# Recreate the principal audit surfaces using the same copied-image QMP driver.
# All output is local; this overwrites only screenshots with replay-* names.
from audit_driver import Driver
import time

d=Driver()
try:
 d.start('replay-desktop','images/flair_tenants_interactive.img','images/flair_data.img')
 d.click(600,64,2);d.move(500,380);d.shot('replay-root')
 for x,n in [(38,'file'),(120,'view')]:
  d.move(x,30);d.button(True);time.sleep(.3);d.shot('replay-menu-'+n);d.rel(500-x,350);d.button(False);time.sleep(.3)
 d.click(123,101,2);d.click(211,124,2)
 d.type('Hello');d.key('right');d.type('123');d.key('right');d.type('1.5');d.key('right');d.type('-7');d.move(500,380);d.shot('replay-123')
 d.key('ctrl-q');time.sleep(.3);d.move(500,380);d.shot('replay-alert');d.key('esc');d.quit()
 d.start('replay-many','images/flair_tenants_interactive.img','images/many_source.img');d.click(600,64,2);d.move(500,380);d.shot('replay-thumb')
 d.drag(370,270,-200,-120);d.move(500,380);d.shot('replay-truncated-title');d.quit()
 d.start('replay-dos','images/tracer_boot.img','images/flair_data.img');d.type('DIR');time.sleep(1);d.shot('replay-DIR');d.type('CLS');time.sleep(1);d.shot('replay-CLS');d.quit()
 d.start('replay-db','images/flair_tenants_interactive.img','images/samir_list.img');d.key('backslash');d.type('USE CLIENTS.DBF');d.type('LIST');time.sleep(1);d.shot('replay-LIST');d.type('SET STATUS ON');time.sleep(1);d.shot('replay-status');d.type('QUIT');d.quit()
finally:
 if d.proc:d.quit()
