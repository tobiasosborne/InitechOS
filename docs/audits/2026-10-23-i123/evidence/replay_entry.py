#!/usr/bin/env python3
# Compact replays for the interactive entry findings; the original raw sequence
# (including excluded fast-input attempts) is preserved in actions.jsonl.
from drive123 import Audit
import sys
x=Audit()
try:
 x.launch('entry_replay')
 for c,v in [('A1',"'Left"),('B1','"Right'),('C1','^Center'),('D1','\\xy'),('A2','+12'),('B2','-0.5'),('C2','.5'),('D2','1e3'),('A3','A2'),('B3','+A2'),('C3','0.1+0.2')]:x.cell(c,v)
 x.state('replay-prefixes')
 for cell,value,tag in [('A4','1,234','comma'),('B4','$12.34','currency'),('C4','12%','percent'),('D4','1e308','large'),('E4','1e-308','small')]:
  x.cell(cell,value);x.state('replay-'+tag);x.key('ctrl-scroll_lock')
 x.cell('A6','abcdef');x.key('f2');x.key('left');x.type('X',False);x.state('replay-edit-left');x.key('home');x.type('Y',False);x.state('replay-edit-home')
 x.key('esc');x.key('esc');x.goto('IV8192');x.state('replay-max');x.goto('IV8193');x.state('replay-bad-row')
finally:
 if x.proc:x.quit()
