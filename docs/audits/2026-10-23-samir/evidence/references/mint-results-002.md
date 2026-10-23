# MINT results 002 — coercion, operators, SET defaults, PICTURE (behavioral)

**Date:** 2026-06-16. Real dBASE III PLUS 1.1 under dosbox-x, `SET ALTERNATE`
transcripts. Scripts: `mint/work/{E2,C1,C2}.PRG`. Continuation of `re/mint-results-001.md`.

> Harness lessons (fold into re/harness-setup.md):
> - **Use inline `ON ERROR <command>`, NOT `ON ERROR DO <proc>`** — the procedure
>   form (proc defined in the same .prg) HANGS on a real trappable error. Inline
>   form traps and **resumes at the next statement** cleanly.
> - `SET ALTERNATE` output is **buffered**; it flushes only on `SET ALTERNATE TO`
>   (close) / `QUIT`. If a run hangs before close, the transcript is LOST. So
>   close the alternate before any risky op, and bound every run with
>   `timeout -k 5 <n>` (dosbox-x ignores plain SIGTERM).
> - `LIST STATUS` and `SET DEVICE TO PRINT`+`@SAY` HANG headless — avoid them;
>   probe SET state behaviorally and test PICTURE via `TRANSFORM()` instead.

## Coercion / operators (closes coercion-table + expressions open questions)

All cross-type mismatches raise the SAME error: **#9 "Data type mismatch."**
[verified: minted C1.TXT 2026-06-16]

| expression | result |
|---|---|
| `"A" + 1` (C+N) | ERR #9 Data type mismatch. |
| `1 + "A"` (N+C) | ERR #9 |
| `CTOD(..) + CTOD(..)` (D+D) | ERR #9 (dates do not add) |
| `"abc" - 5` (C-N) | ERR #9 |
| `DATE() < "x"` (D vs C) | ERR #9 (cross-type relational) |
| `"x" $ 5` ($ with N) | ERR #9 ($ requires char) |
| `1 / 0` | **`*****`** — overflow asterisk-fill, NOT a trapped error |
| `2^3^2` | **64** -> `^` is **LEFT-associative** ((2^3)^2) |
| `-2^2` | **4** -> unary minus binds **tighter** than `^` ((-2)^2) |
| `TRANSFORM(-570,"9999")` | `-570` -> **TRANSFORM() EXISTS in III+ 1.1** (not in the 406-topic HELP index, but functional) |
| `"" $ "ABC"` | **`.F.`** -> empty string is NOT contained (edge case confirmed) |

- The single `Data type mismatch.` (#9) is the uniform token for ALL the
  arithmetic/relational/substring type mismatches above. `C + N` being this error
  (not auto-stringification) is now byte/transcript-verified, not just documented.
- `1/0` is treated as numeric overflow (`*`-fill), consistent with dbf-3 / mint-001.

## SET defaults (closes part of §N)
- **`SET DECIMALS` default = 2** (`? 1/3` -> `0.33`; `STR(1/3,12,8)` -> `0.33333333`
  shows >=8 digits internal precision). [verified: C2.TXT]
- **`SET EXACT` default = OFF**, and `=` for Character is directional "begins with":
  `"ab" = "a"` -> `.T.`, `"a" = "ab"` -> `.F.` (LEFT must begin with RIGHT).
  [verified: C2.TXT] (confirms coercion-table.md SET EXACT section)

## PICTURE / TRANSFORM negative formats (closes §Q definitively)
Tested via `TRANSFORM(value, picture)` [verified: C2.TXT]:

| picture | value | output |
|---|---|---|
| `9999` | -570 | `-570` (leading minus, default) |
| `@( 9999` | -570 | `( 570)` (parentheses for negatives) |
| `@X 9999` | -570 | `570 DB` (trailing " DB" debit) |
| `@C 9999` | 570 | `570 CR` (trailing " CR" credit) |
| `@B 9999` | -570 | `-570` (left-justify; minus still leading) |
| `99,999.99` | -1234.56 | `-1,234.56` |
| `$$,$$9.99` | -12.5 | `$$$-12.50` (floating $, leading minus) |
| `999-` | -570 | `***-` (the trailing `-` is a LITERAL char; value overflowed 3 nines -> `***`) |

**The "570-" trailing-minus does NOT exist as a native dBASE III+ PICTURE format.**
dBASE's negative/accounting formats are: leading minus (default), `@(` parentheses
`( 570)`, `@X` trailing ` DB`, `@C` trailing ` CR`. A literal `-` in a template is
just a literal character. **Conclusion for the initech-os canon: the Office Space
"570-" is an application-level render, not a dBASE PICTURE output** — closest native
forms are `( 570)` (parens) or `570 DB` (trailing debit). [verified]

## Fold-back
coercion-table.md (error #9 uniform, 1/0->*, ""$x->.F.); expressions-and-operators.md
(^ left-assoc, unary-minus precedence, 1/0); full-screen/picture-function-templates.md
(the negative-format table + trailing-minus resolution); language/data-types.md +
commands/set-commands.md (DECIMALS=2, EXACT=OFF defaults). GAPS.md updated.

## Still open (next batches)
ndx-7 page-split policy (build a large index, observe splits); locking (FLOCK/RLOCK
under SET EXCLUSIVE); control-flow edge cases (proc precedence, nesting, stray
ENDIF); remaining 1.1 SET defaults needing LIST STATUS (try `DISPLAY STATUS TO
PRINT` redirected, or read from DBASE.OVL via Ghidra); the DISASM workstream
(Ghidra now installed at ~/ghidra_12.1.2_PUBLIC).
