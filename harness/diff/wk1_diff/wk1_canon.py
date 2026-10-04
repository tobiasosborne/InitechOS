#!/usr/bin/env python3
"""wk1_canon.py -- the INDEPENDENT side of test-i123-wk1-roundtrip (factory).

bead: initech-9u8w (I123 P1). Decodes a .WK1 with the corpus' own independent
reader, $LOTUS123_DECOMP/tools/wk1_ref.py (written from first principles,
validated record-by-record against the real 1-2-3 R2.2 goldens; ADR-0008
DEC-05 independence barrier -- it shares no constant with spec/i123/), and
prints the canonical listing the C oracle also prints from the C model. This
script parses NO .WK1 bytes itself: every value below comes out of
wk1_ref.decode(); only the formatting into the shared listing is ours.

  wk1_canon.py compare FILE CDUMP   exit 0 iff wk1_ref's reading of FILE
                                    equals the C model's listing CDUMP
                                    (the C listing carries formula RPN as hex;
                                    it is rendered through wk1_ref.rpn here)

Listing lines (one per model-owned record or cell, file order):
  U8 <NAME> <hh>                       a one-byte header record
  RANGE <c0> <r0> <c1> <r1>
  HIDVEC1 <hex as wk1_ref prints it>   (wk1_ref shows 48 bytes, then '...')
  WINDOW1 <c> <r> fmt=<hh> colw=<n> ncols=<n> nrows=<n> left=<n> top=<n>
  COLW1 <col> <width>
  CELL <c> <r> BLANK|INT|NUM|LABEL|FORMULA|STRING fmt=<hh> <payload>
ASCII-clean (Rule 12); deterministic (Rule 11).
"""
import os, re, struct, sys

corpus = os.environ.get('LOTUS123_DECOMP', '')
sys.path.insert(0, os.path.join(corpus, 'tools'))
try:
    import wk1_ref
except ImportError:
    sys.stderr.write('wk1_canon.py: cannot import wk1_ref from %s/tools\n' % corpus)
    sys.exit(2)

# wk1_ref names formats; invert its own decoder to recover the byte.
FMT = {}
for _i in range(256):
    _n = wk1_ref.fmt_decode(_i)
    if _n in FMT:
        sys.stderr.write('wk1_canon.py: wk1_ref.fmt_decode is not injective at %d\n' % _i)
        sys.exit(2)
    FMT[_n] = _i

U8_NAMES = ('CALCMODE', 'CALCORDER', 'SPLIT', 'SYNC', 'PROTEC', 'LABELFMT',
            'CALCCOUNT', 'UNFORMATTED')
W1 = re.compile(r'cur\(col=(\d+),row=(\d+)\) fmt=(\S+) colw=(\d+) ncols=(\d+) '
                r'nrows=(\d+) left=(\d+) top=(\d+)$')


def hex8(v):
    return struct.pack('<d', v).hex()


def ref_listing(fn):
    _b, _end, recs, cells = wk1_ref.decode(fn)
    out = []
    for op, name, ln, d in recs:
        if name == 'BOF':
            out.append('BOF %s' % d.split('=')[1])
        elif name in U8_NAMES:
            out.append('U8 %s %s' % (name, d.split('=0x')[1]))
        elif name == 'RANGE':
            v = [int(x) for x in d.split('[')[1].rstrip(']').split(',')]
            out.append('RANGE %d %d %d %d' % tuple(v))
        elif name == 'HIDVEC1':
            out.append('HIDVEC1 %s' % d)
        elif name == 'WINDOW1':
            m = W1.match(d)
            if not m:
                sys.stderr.write('wk1_canon.py: WINDOW1 text not understood: %r\n' % d)
                sys.exit(2)
            g = m.groups()
            out.append('WINDOW1 %s %s fmt=%02x colw=%s ncols=%s nrows=%s left=%s top=%s'
                       % (g[0], g[1], FMT[g[2]], g[3], g[4], g[5], g[6], g[7]))
        elif name == 'COLW1':
            m = re.match(r'col=(\d+) width=(\d+)$', d)
            out.append('COLW1 %s %s' % m.groups())
    for cell in cells:
        c, r, kind, v, fname = cell[:5]
        f = FMT[fname]
        if kind == 'BLANK':
            out.append('CELL %d %d BLANK fmt=%02x' % (c, r, f))
        elif kind == 'INT':
            out.append('CELL %d %d INT fmt=%02x int=%d' % (c, r, f, v))
        elif kind == 'NUM':
            out.append('CELL %d %d NUM fmt=%02x num=%s' % (c, r, f, hex8(v)))
        elif kind == 'LABEL':
            out.append('CELL %d %d LABEL fmt=%02x text=%s' % (c, r, f, v.encode('latin1').hex()))
        elif kind == 'FORMULA':
            out.append('CELL %d %d FORMULA fmt=%02x cache=%s rpn=%s' % (c, r, f, hex8(v), cell[5]))
        elif kind == 'STRING':
            out.append('CELL %d %d STRING fmt=%02x text=%s' % (c, r, f, v.encode('latin1').hex()))
    return out


def c_listing(fn):
    out = []
    for line in open(fn, 'r', encoding='ascii').read().splitlines():
        m = re.match(r'(CELL \d+ \d+ FORMULA fmt=\S+ cache=\S+) code=([0-9a-f]*)$', line)
        if m:
            line = '%s rpn=%s' % (m.group(1), wk1_ref.rpn(bytes.fromhex(m.group(2))))
        out.append(line)
    # the RECORD lines of the C listing are in file order; wk1_ref lists the
    # header records first and the cells after, exactly as the C side does.
    return out


def main():
    if len(sys.argv) != 4 or sys.argv[1] != 'compare':
        sys.stderr.write(__doc__)
        return 2
    ref = ref_listing(sys.argv[2])
    got = c_listing(sys.argv[3])
    if ref == got:
        return 0
    for i in range(max(len(ref), len(got))):
        a = ref[i] if i < len(ref) else '<end>'
        b = got[i] if i < len(got) else '<end>'
        if a != b:
            sys.stderr.write('wk1_canon.py: line %d differs\n  wk1_ref: %s\n  C model: %s\n'
                             % (i + 1, a, b))
            break
    return 1


if __name__ == '__main__':
    sys.exit(main())
