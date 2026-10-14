#!/usr/bin/env python3
"""wk1_formula_ref.py -- the INDEPENDENT side of test-i123-formula (factory).

bead: initech-94ah (I123 P2). For one .WK1, prints every FORMULA cell's
cached result -- the value the real Lotus 1-2-3 R2.2 computed and stored --
as the corpus' own independent reader $LOTUS123_DECOMP/tools/wk1_ref.py
decodes it (ADR-0008 DEC-05 independence barrier: wk1_ref shares no
constant with spec/i123/). This script parses NO .WK1 bytes itself; every
value comes out of wk1_ref.decode(), only the formatting is ours.

  wk1_formula_ref.py FILE
    F <col> <row> NUM <f64 as 16 hex digits, little-endian>
    F <col> <row> ERR                 (cache +inf: ERR, wk1-format.md 4.7)
    F <col> <row> NA                  (cache -inf: NA)
    F <col> <row> STR <text as hex>   (cache NaN + the STRING record)
    F <col> <row> NAN                 (cache NaN, no STRING record)
ASCII-clean (Rule 12); deterministic (Rule 11).
"""
import math, os, struct, sys

corpus = os.environ.get('LOTUS123_DECOMP', '')
sys.path.insert(0, os.path.join(corpus, 'tools'))
try:
    import wk1_ref
except ImportError:
    sys.stderr.write('wk1_formula_ref.py: cannot import wk1_ref from %s/tools\n' % corpus)
    sys.exit(2)


def main():
    if len(sys.argv) != 2:
        sys.stderr.write(__doc__)
        return 2
    _b, _end, _recs, cells = wk1_ref.decode(sys.argv[1])
    for i, cell in enumerate(cells):
        c, r, kind, v = cell[:4]
        if kind != 'FORMULA':
            continue
        nxt = cells[i + 1] if i + 1 < len(cells) else None
        if math.isnan(v):
            if nxt is not None and nxt[2] == 'STRING' and nxt[0] == c and nxt[1] == r:
                print('F %d %d STR %s' % (c, r, nxt[3].encode('latin1').hex()))
            else:
                print('F %d %d NAN' % (c, r))
        elif math.isinf(v):
            print('F %d %d %s' % (c, r, 'ERR' if v > 0 else 'NA'))
        else:
            print('F %d %d NUM %s' % (c, r, struct.pack('<d', v).hex()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
