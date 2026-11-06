# screen2txt.py RAW -> 25 text rows (attr 0x70 cells shown as-is; see .attr)
import sys
d=open(sys.argv[1],'rb').read()
for r in range(25):
    print(''.join(chr(d[(r*80+c)*2]) if 32<=d[(r*80+c)*2]<127 else ('#' if d[(r*80+c)*2]>127 else ' ') for c in range(80)).rstrip())
