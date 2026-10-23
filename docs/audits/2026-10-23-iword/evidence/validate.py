from pathlib import Path
import hashlib,json,re
from PIL import Image
R=Path.cwd();checks={}
def check(name,condition,detail=None):
 checks[name]={'pass':bool(condition),'detail':detail};assert condition,name
def b(s,n):return (R/'evidence'/f'{s}-extracted'/n).read_bytes()
def body(x):return x[int.from_bytes(x[4:8],'little'):]
check('real_MINT01_round_trip',b('s01','ROUND01.WP5')==(R/'evidence/fixtures/MINT01.WP5').read_bytes())
check('adjacent_replace',body(b('s01','ADJ.WP5'))==b'bbbb')
check('forward_backward_block_attributes',body(b('s01','BLOCK.WP5'))==bytes.fromhex('c30cc34142c40cc443c30ec34445c40ec4'))
check('crossing_attributes',body(b('s03','NEST.WP5'))==bytes.fromhex('c30cc3c30ec341c40cc442c40ec443'))
check('copy_duplicates_text',body(b('s03','COPY.WP5'))==b'COPY ME.COPY ME.')
check('cross_code_block_preserves_plain_CD',body(b('s03','CROSS.WP5'))==bytes.fromhex('c30cc3c40cc44344'))
check('code_only_roundtrip_body',body(b('s01','CODEOUT.WP5'))==body((R/'evidence/fixtures/CODES.WP5').read_bytes()))
pages=body(b('s03','PAGES.WP5'));check('hardpage_after_undelete',pages.startswith(b'Page one.\xd4\x00\x1c\x00') and b'\x1c\x00\x00\xd4\x0cPage two.\nLast line.' in pages)
mx=bytearray(b('s03','MAXOUT.WP5'));orig=bytearray((R/'evidence/stress-fixtures/MAX.WP5').read_bytes());check('max_file_size',len(mx)==65536);check('max_cursor_flag',mx[0x103]==8);mx[0x103]=orig[0x103];check('max_unchanged_after_refused_edits',mx==orig)
original=(R/'evidence/fixtures/ORIGINAL.WP5').read_bytes();check('original_before_has_373_bytes',len(original)==373)
check('full_disk_overwrite_loses_original_twice',b('s02','ORIGINAL.WP5')==b'' and b('s04','ORIGINAL.WP5')==b'')
check('readonly_preserved',b('s02','READONLY.WP5')==original)
check('bad_name_was_mangled',body(b('s01','BADNAME.WP5'))==b'Original version changed')
check('compose_and_hardspace_bytes',body(b('s03','CHARS.WP5'))==b"a b2e'")
check('saved_tab_targets_8_inches',bytes.fromhex('c102802580255000c1') in body(b('s01','EDGE.WP5')))
check('indent_targets_8_inches',bytes.fromhex('c2005802802580255000c2') in body(b('s03','INDENT.WP5')))
check('original_backup_enabled_on_disk',b('s01','IWORD.SET')==b'IWS1ic\x01\x00')
check('save_then_stay_saved_content',body(b('s03','STAY.WP5'))==b'save then stay')
check('save_then_quit_saved_content',body(b('s03','QUIT.WP5'))==b'save then quit')
text=(R/'evidence/wp5-independent-decoder.txt').read_text();ends=re.findall(r'END offset=(\d+) filesize=(\d+) trailer-errors=(\d+)',text)
check('independent_decoder_13_files',len(ends)==13 and all(a==z and e=='0' for a,z,e in ends),ends)
# Prefix check for every new selected output: allowed cursor byte differs only.
prefix=(R/'evidence/fixtures/MINT01.WP5').read_bytes()[:327]
for s,n in [('s01','ADJ.WP5'),('s01','BLOCK.WP5'),('s01','FONT.WP5'),('s01','FORMAT.WP5'),('s03','COPY.WP5'),('s03','NEST.WP5'),('s03','PAGES.WP5')]:
 x=bytearray(b(s,n)[:327]);check(f'{s}_{n}_header',x[:4]==b'\xffWPC' and int.from_bytes(x[4:8],'little')==327 and x[8:12]==b'\x01\x0a\x00\x01');x[0x103]=prefix[0x103];check(f'{s}_{n}_default_prefix',x==prefix)
(R/'evidence/byte-validation.json').write_text(json.dumps(checks,indent=2))
findings=json.loads((R/'evidence/findings.json').read_text());ids=[f['id'] for f in findings]
assert ids==[f'L{i:03}' for i in range(1,len(ids)+1)]
report=(R/'REPORT.md').read_text(encoding='ascii');nits=(R/'NITS.tsv').read_text(encoding='ascii').splitlines();assert len(nits)==len(ids)+1
for f in findings:
 assert f['id'] in report and f['steps'] and f['expected'] and f['observed']
 for p in f['evidence']:
  assert (R/p).is_file(),p
  if p.endswith('.png'):
   im=Image.open(R/p);assert im.size==(640,480) or '/references/' in p
rows=[json.loads(x) for x in (R/'evidence/actions.jsonl').read_text().splitlines()]
active=None;lifetimes=[]
for row in rows:
 a=row['action'];kind=a[0]
 if kind=='start':assert active is None;active=row['session'];lifetimes.append([active,row['utc'],None])
 if kind=='quit':assert active==row['session'];lifetimes[-1][2]=row['utc'];active=None
assert active is None
cleanup=[json.loads(x) for x in (R/'evidence/cleanup.jsonl').read_text().splitlines()];assert len(cleanup)==5 and all(x['exit_code']==0 for x in cleanup)
for session in sorted({r['session'] for r in rows}):
 (R/'evidence'/f'{session}-primitive-actions.jsonl').write_text(''.join(json.dumps(r['action'])+'\n' for r in rows if r['session']==session and r['action'][0] in {'start','key','keyheld','rel','button','pause','shot','quit','note'}))
result={'findings':len(findings),'nits_rows':len(nits)-1,'byte_checks':len(checks),'screenshots':len(list((R/'shots').glob('*.png'))),'all_cited_evidence_exists':True,'report_ascii':True,'serial_session_lifetimes_do_not_overlap':True,'sessions':lifetimes,'cleanup':cleanup}
(R/'evidence/deliverable-validation.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
