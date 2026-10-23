from pathlib import Path
import json
E=Path(__file__).resolve().parent
comparisons=json.loads((E/'manual-comparison.json').read_text());mapping=json.loads((E/'manualfn-expectations.json').read_text());actions=[];meta={}
for c in comparisons:
 if c['matches'] or c['name'] in ['manualfn-fn-date','manualfn-fn-date-2','manualfn-fn-dtoc-2']:continue
 k=c['name'];r=mapping[k];actions+=['ON ERROR','USE','SET EXACT OFF','SET DATE AMERICAN','SET CENTURY OFF','SET DECIMALS TO 2']+r['input_text'].splitlines()+[['shot','manualcheck-'+k[9:]]];meta[k]=r
# Confirm the printed date-index example against the unmodified sample table.
actions+=['ON ERROR','USE STAFF','INDEX ON HIRED TO STAFFHIR','LIST NEXT 4 LASTNAME, FIRSTNAME, HIRED',['shot','manualcheck-date-index'],'QUIT']
(E/'manualcheck.json').write_text(json.dumps({'name':'manualcheck','actions':actions},indent=2));(E/'manualcheck-expectations.json').write_text(json.dumps(meta,indent=2));print(len(meta),'isolated rechecks')
