#!/usr/bin/env python3
"""Two colliding Trash names, one multi-selection Put Away, preserved output."""
import importlib.util,pathlib
R=pathlib.Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('driver',R/'audit_driver.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);d=m.Driver()
try:
 d.start('repro-multi','images/flair_tenants_interactive.img','images/edge-data.img')
 d.click(600,64,2);d.click(60,154,2);d.drag(145,122,455,298);d.key('ctrl-w')
 d.click(126,154,2);d.drag(78,122,522,298);d.key('ctrl-w');d.click(600,420,2)
 d.key('ctrl-a');d.shot('repro-multi-selected');d.key('ctrl-y');d.shot('repro-multi-result')
finally:
 if d.proc is not None:d.quit()
