# InitechBase lanes: how to add a function / SET option / command (F4)

The fns and manual registries use one file per item; SET registration uses one
row per option. Distinct SET additions use `merge=union` to avoid positional
renumbering conflicts. Union preserves text, not correctness: `test-samir`
validates row syntax, duplicate keys and mutually exclusive case markers.
Handlers still share C files, and command verb tables still need manual merges.

1. Accept a fns case: rename `harness/diff/dbf_diff/xbase_prog_diff/fns-mint/<case>/PENDING` to `ACCEPTED`
   (new case: mint the dir, add an empty `PENDING`). The Makefile derives SAMIR_FNS_CASES/PENDING from these.
   Markers must be regular empty files; both markers are an error. Neither marker means evidence only.
2. Manual entry: add ONE file, no edit to any chapter. Reference: `pubs/manuals/initechbase-volume-one/entries/{cmd,fn}/<key>.tex`
   (the `XbaseEntry` block, comment header included); Quick Reference: `pubs/manuals/initechbase-quick-reference/entries/{cmd,set,fn}/<key>.tex`
   (one `\QREntry`/`\QRFunction` line). `pubs/entry-index.sh` builds the sorted `\input` index into `build/pubs/gen`.
   Keep each group's tracked `.gitkeep` so an empty group survives clean checkouts and generates an empty index.
3. Key = lower-case name, blanks to `-`; SET options in the volume are `set-<name>`; `?` `??` `&` are `0q` `0qq` `0amp`.
   Keys must contain only `[A-Za-z0-9_-]`; unsafe names are rejected before generating TeX.
   Files sort in C-locale byte order of the key. `reccount-lupdate.tex` (LUPDATE) is a legacy mis-order kept for byte-identical PDFs.
4. SET option: write `static int set_opt_<name>(ip, ss, s, err_code)` in `os/samir/cmd/set.c` and add ONE row
   `SET_OPT("KEYWORD", set_opt_<name>)` to `os/samir/cmd/set_options.def` (merge=union in `.gitattributes`). Nothing is numbered;
   keywords are uppercase identifiers of at most 31 characters, with one complete row per physical line;
   row order only breaks ties between 4-letter abbreviations (earlier row wins, as rt_kw_index), so append unless you mean a tie-break.
5. Command verb (initech-1x6x2.4): every hook is a per-verb table now, like SET options. Pick the file by area and add ONE row:
   `os/samir/cmd/query_verbs.def` (`QUERY_VERB("KW", qv_x)`, handler `static int qv_x(ip, qs, verb, args, err_code)` in query.c),
   `mutate_verbs.def` (`MUTATE_VERB`, `mv_x(ip, args, err_code)` in mutate.c), `proc_verbs.def` (`PROC_VERB`, `pv_x(ip, ps, args, err_code)` in proc.c),
   `flow_verbs.def` (`FLOW_VERB`, `fv_x(ip, fs, args, ec)` in flow.c; memvar/work-area verbs only, IF/DO/LOOP stay structural in line_classify).
   Steps: (a) write the static handler returning the hook's final code (CMD_OK == INTERP_OK == 0; return CMD_UNKNOWN to decline a verb that is
   not yours; a cold verb goes through the overlay: add a MUT_OP_* id in mutate.c, never use the row position); (b) append the row to the .def;
   (c) the keyword is an uppercase identifier or a symbol verb (`?` `??` `!` `@` `&`), at most 31 bytes, one complete row per physical line,
   handler name must carry the file's prefix (`qv_` `mv_` `pv_` `fv_`); a shared handler body is fine (GO and GOTO share qv_go);
   (d) row order is the 4-letter abbreviation tie-break (earlier row wins, as rt_kw_index), so APPEND unless you mean a tie-break, and run
   `make test-samir-abbrev`; (e) nothing is numbered, so two lanes adding different verbs merge with `merge=union` (.gitattributes) and a duplicate
   key or malformed row is caught by `make test-samir-registries` (harness/check_registries.c, mutants in test-samir-registries-mutant). Do not
   edit the keyword strings in the hooks: they no longer exist there. A NEW verb table needs a schema in check_registries.c AND a line in .gitattributes.
6. Gates after a change: `make test-samir`, `test-samir-fns-<case>` (+`-mutant`), `test-samir-abbrev`, `test-pubs-build`, `test-pubs-examples`.
   A new union path in `.gitattributes` needs an explicit validator schema before the merge vector accepts it.
