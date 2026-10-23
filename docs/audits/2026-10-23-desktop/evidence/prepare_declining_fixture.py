#!/usr/bin/env python3
"""Use the archived test-only tenant, without building repository sources."""
import pathlib,shutil,subprocess
r=pathlib.Path(__file__).resolve().parents[1]
shutil.copyfile(r/'images/flair_data.img',r/'images/declining-fixture.img')
subprocess.run(['mcopy','-o','-i',str(r/'images/declining-fixture.img'),str(r/'evidence/ctenant_decline.exe'),'::APPS/CTENANT.EXE'],check=True)
