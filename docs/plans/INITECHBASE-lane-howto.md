# InitechBase lanes: how to add a function / SET option / command (F4)

Everything a lane registers is one new file or one new row; no shared line is edited.

1. Accept a fns case: rename `harness/diff/dbf_diff/xbase_prog_diff/fns-mint/<case>/PENDING` to `ACCEPTED`
   (new case: mint the dir, add an empty `PENDING`). The Makefile derives SAMIR_FNS_CASES/PENDING from these.
2. Manual entry: add ONE file, no edit to any chapter. Reference: `pubs/manuals/initechbase-volume-one/entries/{cmd,fn}/<key>.tex`
   (the `XbaseEntry` block, comment header included); Quick Reference: `pubs/manuals/initechbase-quick-reference/entries/{cmd,set,fn}/<key>.tex`
   (one `\QREntry`/`\QRFunction` line). `pubs/entry-index.sh` builds the sorted `\input` index into `build/pubs/gen`.
3. Key = lower-case name, blanks to `-`; SET options in the volume are `set-<name>`; `?` `??` `&` are `0q` `0qq` `0amp`.
   Files sort in C-locale byte order of the key. `reccount-lupdate.tex` (LUPDATE) is a legacy mis-order kept for byte-identical PDFs.
4. SET option: write `static int set_opt_<name>(ip, ss, s, err_code)` in `os/samir/cmd/set.c` and add ONE row
   `SET_OPT(KEYWORD, set_opt_<name>)` to `os/samir/cmd/set_options.def` (merge=union in `.gitattributes`). Nothing is numbered;
   row order only breaks ties between 4-letter abbreviations (earlier row wins, as rt_kw_index), so append unless you mean a tie-break.
5. Command verb: the other hooks (query.c, mutate.c, proc.c, flow.c) still number verbs positionally; add a verb at the END of the
   keyword string and take the next `case`, and expect to renumber on merge until they get the same table (follow-up).
6. Gates after a change: `make test-samir-fns-<case>` (+`-mutant`), `test-samir-abbrev`, `test-pubs-build`, `test-pubs-examples`.
