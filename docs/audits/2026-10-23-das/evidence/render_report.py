from pathlib import Path
import json
r=Path(__file__).resolve().parents[1]
fs=json.loads((r/'evidence/findings.json').read_text())
blocks=[]
for f in fs:
 blocks.append(f"### {f['id']} - {f['title']}\n\nArea: {f['area']}. Severity: {f['severity']}.\n\nSteps: {f['steps']}\n\nObserved: {f['observed']}\n\nExpected and basis: {f['expected']}\n\nEvidence: "+', '.join(f'[{p}]({p})' for p in f['evidence'])+'.'+ ('\n\n'+f['guess'] if f['guess'] else '')+'\n')
report=(r/'evidence/report_sections.md').read_text().replace('@@FINDINGS@@','\n'.join(blocks))
report.encode('ascii')
(r/'REPORT.md').write_text(report)
(r/'NITS.tsv').write_text('id\tarea\tseverity\tsummary\n'+''.join('\t'.join([f['id'],f['area'],f['severity'],f['title']])+'\n' for f in fs))
print(len(fs),'findings;',len(report),'report bytes')
