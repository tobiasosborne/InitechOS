from pathlib import Path
import json
E=Path(__file__).resolve().parent;F=json.loads((E/'findings-final-draft.json').read_text())
def add(key,area,title,steps,saw,expected,files):F.append(dict(key=key,area=area,severity='P2',title=title,steps=steps,observed=saw,expected=expected,evidence=files,source_guess=''))
add('relation','Related work areas','SET RELATION accepts a link but never moves the child',
 'SELECT 2; USE DEPTS ALIAS D INDEX DEPTS; SELECT 1; USE STAFF; SET RELATION TO DEPT INTO D; GO 1; ? DEPT,D->DEPT; GO 5; ? DEPT,D->DEPT.',
 'The two queries print SOFT ACCT and FACL ACCT. The indexed child stays on its first ACCT record instead of following the parent key.',
 'Move the child to the matching department as the parent changes. Named reference: dbase3-decomp/specs/commands/set-commands.md, SET RELATION. Shipped Quick Reference qr-set.tex:14 explicitly labels it "recorded, not maintained by this version"; this is a tested historical feature gap, not an undisclosed manual promise.',
 ['shots/last-controls-relation.png','evidence/last-controls-serial.log','evidence/fixtures/STAFF.DBF','evidence/fixtures/DEPTS.DBF','evidence/fixtures/DEPTS.NDX'])
add('talk','Command feedback','SET TALK ON gives no replacement count',
 'USE CLIENTS; SET TALK ON; REPLACE ALL BAL WITH BAL+1.',
 'Three records are changed, but the command returns directly to the dot prompt without a count or other completion feedback.',
 'Show the processed-record count under TALK ON. This expectation comes from my memory of dBASE result counts, corroborated by the HELP-derived SET TALK purpose in dbase3-decomp/specs/commands/set-commands.md. Shipped qr-set.tex:17 openly says TALK is "recorded, not acted on by this version".',
 ['shots/last-controls-talk.png','evidence/last-controls-serial.log'])
for f in F:
 if f['key']=='index-length':f['evidence'][-1]='shots/artifact-controls-index-length.png';f['evidence']+=['evidence/artifact-controls-serial.log']
 if f['key']=='no-table-error':f['evidence'][-1]='shots/artifact-controls-no-table.png';f['evidence']+=['evidence/artifact-controls-serial.log']
# Stable severity-first numbering, preserving order within severity.
F.sort(key=lambda f:f['severity'])
for i,f in enumerate(F,1):f['id']='L%03d'%i
(E/'findings.json').write_text(json.dumps(F,indent=2));print(len(F))
