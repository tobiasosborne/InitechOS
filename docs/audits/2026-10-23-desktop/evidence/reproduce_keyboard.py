#!/usr/bin/env python3
"""Minimal independent reproducer for keyboard paint and horizontal reveal."""
import importlib.util,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('audit_driver',ROOT/'audit_driver.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m);d=m.Driver()
try:
 d.start('keyboard-repro','images/flair_tenants_interactive.img','images/edge-data.img')
 d.click(600,64,2);d.click(60,154);d.shot('repro-key-before')
 d.key('right');d.pause(2);d.shot('repro-key-stale')
 d.key('ctrl-o');d.shot('repro-key-opens-B');d.key('ctrl-w')
 d.click(194,154,2);d.click(78,122);d.drag(389,289,-260,0)
 d.key('right');d.drag(84,90,1,0);d.shot('repro-key-hidden')
 d.drag(130,289,260,0);d.shot('repro-key-revealed')
finally:
 if d.proc is not None:d.quit()
