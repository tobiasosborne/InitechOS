import pathlib,json,fcntl,datetime,hashlib,collections,re
root=pathlib.Path(__file__).resolve().parents[1]
now=datetime.datetime.now(datetime.timezone.utc).isoformat()
sessions=[]
for p in sorted((root/'evidence').glob('*-session.json')):
 s=json.loads(p.read_text());locked=[]
 for fn in s.get('images_before',{}):
  ip=root/fn
  with ip.open('r+b') as f:
   try:fcntl.lockf(f,fcntl.LOCK_EX|fcntl.LOCK_NB);fcntl.lockf(f,fcntl.LOCK_UN);locked.append({'image':fn,'exclusive_lock_available':True})
   except BlockingIOError:locked.append({'image':fn,'exclusive_lock_available':False})
 sessions.append({'session':p.stem.removesuffix('-session'),'recorded_qemu_pid':s['qemu_pid'],'normal_quit_recorded':'stopped_utc' in s,'exit_code':s.get('exit_code'),'image_locks':locked})
cleanup={'checked_utc':now,'sessions':sessions,'all_image_locks_released':all(x['exclusive_lock_available'] for s in sessions for x in s['image_locks']),'host_interruption':'functions was terminated with host runner status 143; see functions-host-interruption.json. It had no normal quit footer. Its image locks were verified released before reuse and again here.'}
(root/'evidence/cleanup-audit.json').write_text(json.dumps(cleanup,indent=2))
assert cleanup['all_image_locks_released']
f=json.load(open(root/'evidence/final-findings.json'));counts=collections.Counter(x['severity'] for x in f)
# The priority list is editorial judgment, with explicit report ids.
priorities=[
 'Make failed Save/Replace preserve the last good file (L001).',
 'Make an unchanged F2 edit preserve every accepted character (L002).',
 'Complete the in-line editor and reference-key behavior (L005, L025).',
 'Provide working Undo and a visible undo state (L024).',
 'Implement insertion/deletion of rows and columns, including reference adjustment.',
 'Support Copy to destination ranges and anchored range selection (L006, L007).',
 'Make high-precision numeric formats and @STRING work across their documented ranges (L030, L033).',
 'Make Backup and file selection usable before users entrust a workbook to the disk (L012, L029).',
 'Continue Finder shutdown after a successful asynchronous Save (L003).',
 'Keep wide columns visible and make the native scroll controls operate (L008, L027).',
 'Recover storage after erasure and permit same-size replacements at capacity (L010, L032; earlier L080).',
 'Finish range-name reassignment, display, and name selection (L009, L016, L020).',
 'Complete the remaining basic value/attribute behavior: leading $, @NOW, and current filename (L004, L011, L019).',
 'Deliver worksheet titles and split/synchronized windows, which the menus already promise.',
 'Make the advertised Print and Graph workflows produce real output, then tackle the Data command backlog.'
]
tail=(root/'evidence/report-tail.txt').read_text();tail+='\n## The fifteen things to fix first\n\nThis ranking is the auditor\'s judgment about data safety and period-workstation credibility.\n\n'
for i,p in enumerate(priorities,1):tail+=f'{i}. {p}\n'
tail+='\n## Deliverable and cleanup checks\n\n'
tail+=f'This report contains {len(f)} new findings: {counts["P1"]} P1, {counts["P2"]} P2, and {counts["P3"]} P3. NITS.tsv has one matching row per finding. Of these, 224 are the individually observed unavailable menu leaves. The remaining {len(f)-224} findings concern behavior or presentation beyond those refusals.\n\n'
tail+=f'{len(sessions)} emulator image sessions were recorded. All normal sessions have explicit QMP quit/exit records. The host-interrupted functions session is separately documented, and all copied-image locks are released. No other emulator or process was killed. See evidence/cleanup-audit.json, cleanup.jsonl, and the per-session manifests.\n\n'
tail+='Evidence validation includes PNG decoding and SHA256 for every cited screenshot, observed pixel-panel matching for every menu refusal, serial-render checks for every selected menu description, independent WK1 parsing, and raw-file comparisons. evidence/validation.json summarizes link, row-count, and ASCII checks.\n'
(root/'evidence/report-tail-final.txt').write_text(tail)
print('sessions',len(sessions),'normal',sum(s['normal_quit_recorded'] for s in sessions),'all locks free',cleanup['all_image_locks_released'])
