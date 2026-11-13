# Mutation hooks awaiting patch conversion

303 guard/macro locations, 255 distinct wrong-code macros.
This is not a completed mutation sweep. The set is too large for import-time
conversion: leave the original guards, then replace each fault with a patch
under mutants/ in a follow-up. mk/mutants.mk is included, but no patch sweep is
claimed. Catchers below are intended legacy host suite families or explicitly
deferred boot smokes; individual guard-to-suite conversion must demonstrate red.
Repeated macros must be converted together. Wrong-branch excerpts and names
describe the concrete change; line numbers are at the imported revision.
Functional overlay/profile/platform guards are excluded.

| Location | Macro | Wrong behaviour / branch | Intended catcher |
|---|---|---|---|
| core/eval.c:152 | `IBASE_CORE_MUTATE_CONCAT_PLUS` | core concat plus; `if (tot > 254u) return fail(err, XBEE_CONCAT_PLUS);` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:189 | `IBASE_CORE_MUTATE_CONCAT_MINUS` | core concat minus; `if (tot > 254u) return fail(err, XBEE_CONCAT_MINUS);` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:409 | `XB_MUTATE_BLANKDATE_HIGH` | xb blankdate high; `if (ba && bb)      c = 0;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:489 | `XB_MUTATE_EVAL` | xb eval; `if ((ta == XB_C // ta == XB_M) && tb == XB_N) {` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:781 | `IBASE_MUTATE_UNDEFINED_ERROR1` | undefined error1; `return fail(err, XBEE_UNBOUND); /* restore old error 1/Internal error */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:814 | `IBASE_FNS_MUTANT_division` | fns division; `if(n->op==XBT_SLASH) {` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/eval.c:853 | `IBASE_W2_MUTANT_set_fixed` | w2 set fixed; `return; /* MUTANT: FIXED ON changes nothing */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:112 | `XB_MUTATE_FN_CTOD_CALVALID` | xb fn ctod calvalid; `` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:252 | `XB_MUTATE_FN_SUBSTR` | xb fn substr; `start += 1;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:389 | `IBASE_DT_MUTANT_valstar` | dt valstar; `if (args[0].u.c.len && args[0].u.c.p[0] == '*')` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:468 | `XB_MUTATE_SETFMT_IGNORE` | xb setfmt ignore; `(void)ctx;      /* ctx not read under mutation; suppress -Wunused-parameter */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:549 | `XB_MUTATE_FN_CTOD_CALVALID` | xb fn ctod calvalid; `if (mm < 1 // mm > 12 // dd < 1 // dd > xb_days_in_month(mm, yy)) {` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:607 | `XB_MUTATE_SETFMT_IGNORE` | xb setfmt ignore; `fmt     = (uint8_t)XB_DATE_AMERICAN;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:620 | `IBASE_DT_MUTANT_blankdtoc` | dt blankdtoc; `if (fn_date_blank(args[0].u.d)) rt_memset(dst, ' ', width);` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:697 | `IBASE_FNS_MUTANT_type` | fns type; `code='C';` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:722 | `XB_MUTATE_FN_ABS` | xb fn abs; `if (v < 0.0) v = -v;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:830 | `XB_MUTATE_FN_ROUND_TRUNC` | xb fn round trunc; `` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:928 | `XB_MUTATE_FN_DOW` | xb fn dow; `dow = (dow % 7) + 1;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1055 | `XB_MUTATE_FN_LEFT` | xb fn left; `n = n + 1;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1468 | `XB_MUTATE_TR_DOTWIDTH` | xb tr dotwidth; `fmt_w = n_digit + (n_dec > 0 ? 1 : 0);` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1475 | `XB_MUTATE_TR_ROUNDING` | xb tr rounding; `char full[256];` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1570 | `XB_MUTATE_TRANSFORM_PAREN` | xb transform paren; `dst[0]        = '(';` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1773 | `XB_MUTATE_TRANSFORM_PAREN` | xb transform paren; `if (at_flags & TR_AT_PAREN)  { if (is_neg)  suffix_extra = 2; } /* "(" prefix + ")" suffix */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:1831 | `XB_MUTATE_FN_RECNO_EOF` | xb fn recno eof; `if (ctx->dbcur->eof(ctx->dbcur_user) && r > 0u) {` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2004 | `IBASE_CORE_MUTATE_FILE_NEEDS_USE` | core file needs use; `if (!need_dbcur(ctx, err)) return *err;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2172 | `IBASE_FNS_MUTANT_limitpic` | fns limitpic; `if (integer_columns < 1) integer_columns = 1;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2178 | `IBASE_FNS_MUTANT_mathpicture` | fns mathpicture; `if (arg->u.n < 0.0 && arg->num_width > 4u) width++;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2223 | `IBASE_FNS_MUTANT_EXP` | fns exp; `result += 1.0; /* Rule 6: the EXP transcript must catch this perturbation. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2223 | `IBASE_FNS_MUTANT_exp` | fns exp; `result += 1.0; /* Rule 6: the EXP transcript must catch this perturbation. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2223 | `IBASE_FNS_MUTANT_exppicture` | fns exppicture; `result += 1.0; /* Rule 6: the EXP transcript must catch this perturbation. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2262 | `IBASE_FNS_MUTANT_logguards` | fns logguards; `return fn_fail(err, 58);` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2269 | `IBASE_FNS_MUTANT_LOG` | fns log; `value += 0.01;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2269 | `IBASE_FNS_MUTANT_log` | fns log; `value += 0.01;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2289 | `IBASE_FNS_MUTANT_SQRT` | fns sqrt; `return fn_fail(err, 61); /* MUTANT: raise before the result prints. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2289 | `IBASE_FNS_MUTANT_sqrt` | fns sqrt; `return fn_fail(err, 61); /* MUTANT: raise before the result prints. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2323 | `IBASE_FNS_MUTANT_TIME` | fns time; `dst[2] = ';'; /* The named gate passes the lower-case case name. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2323 | `IBASE_FNS_MUTANT_time` | fns time; `dst[2] = ';'; /* The named gate passes the lower-case case name. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2345 | `IBASE_FNS_MUTANT_lupdate` | fns lupdate; `if (date > 0.0) date += 1.0;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2371 | `IBASE_FNS_MUTANT_ndx_deferred` | fns ndx deferred; `return fn_fail(err, 87);       /* MUTANT: output must precede the error */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2386 | `IBASE_FNS_MUTANT_ndx` | fns ndx; `slot++;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2417 | `IBASE_FNS_MUTANT_row` | fns row; `out->u.n+=1; /* Each independent real capture must catch this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2432 | `IBASE_FNS_MUTANT_col` | fns col; `out->u.n+=1; /* Each independent real capture must catch this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2447 | `IBASE_FNS_MUTANT_prow` | fns prow; `out->u.n+=1; /* Each independent real capture must catch this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2462 | `IBASE_FNS_MUTANT_pcol` | fns pcol; `out->u.n+=1; /* Each independent real capture must catch this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2502 | `IBASE_FNS_MUTANT_inkey` | fns inkey; `key++;                         /* Rule 6: empty queue must return zero. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2522 | `IBASE_FNS_MUTANT_readkey` | fns readkey; `out->u.n = 1.0;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2548 | `IBASE_FNS_MUTANT_ISCOLOR` | fns iscolor; `color = !color; /* Rule 6: invert only the capability result. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2548 | `IBASE_FNS_MUTANT_iscolor` | fns iscolor; `color = !color; /* Rule 6: invert only the capability result. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2573 | `IBASE_FNS_MUTANT_FKLABEL` | fns fklabel; `if (key == 1) key = 2;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2573 | `IBASE_FNS_MUTANT_fklabel` | fns fklabel; `if (key == 1) key = 2;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2603 | `IBASE_FNS_MUTANT_OS` | fns os; `text[14] = '1'; /* Rule 6: wrong version, 3.31 instead of 3.30. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2603 | `IBASE_FNS_MUTANT_os` | fns os; `text[14] = '1'; /* Rule 6: wrong version, 3.31 instead of 3.30. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2622 | `IBASE_FNS_MUTANT_VERSION` | fns version; `return 0;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2622 | `IBASE_FNS_MUTANT_version` | fns version; `return 0;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2651 | `IBASE_FNS_MUTANT_getenv` | fns getenv; `if (n > 0) n--;` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2671 | `IBASE_FNS_MUTANT_DISKSPACE` | fns diskspace; `bytes = 0u; /* Rule 6: the independent III+ >0 line must reject this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2671 | `IBASE_FNS_MUTANT_diskspace` | fns diskspace; `bytes = 0u; /* Rule 6: the independent III+ >0 line must reject this. */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/fn_builtins.c:2717 | `IBASE_FNS_MUTANT_recsize` | fns recsize; `case FN_RECSIZE:` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/lex.c:407 | `XB_MUTATE_LEX` | xb lex; `` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/ovl.c:57 | `IBASE_OVL_MUTATE_NO_STAMP` | ovl no stamp; `if (rt_memcmp(h + IBASE_OVL_HDR_SIG_OFF, IBASE_OVL_SIG, IBASE_OVL_SIG_LEN) //` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:101 | `IBASE_OVL_MUTATE_NO_RELOC` | ovl no reloc; `v += base;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:295 | `IBASE_OVL_MUTATE_NO_CRC` | ovl no crc; `if (crc != ovl_rd32(hdr + IBASE_OVL_HDR_STAMP_OFF)) { ovl_close(); return IBASE_OVL_CORRUPT; }` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:336 | `IBASE_OVL_MUTATE_SKIP_OPEN` | ovl skip open; `return 0;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:371 | `IBASE_OVL_MUTATE_NO_CRC` | ovl no crc; `if (crc != ecrc) return -1;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:384 | `IBASE_OVL_MUTATE_NO_STAMP` | ovl no stamp; `if(id==IBASE_OVL_SET && k==1)return 0;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:384 | `IBASE_OVL_MUTATE_SKIP_OPEN` | ovl skip open; `if(id==IBASE_OVL_SET && k==1)return 0;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/ovl.c:400 | `IBASE_OVL_MUTATE_NO_SWAP` | ovl no swap; `if (O.loaded == OVL_NONE) {` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| core/parse.c:372 | `XB_MUTATE_PARSE` | xb parse; `` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/parse.c:412 | `PARSE_MUTATE_ADDMUL_SWAP` | parse addmul swap; `` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/rt.c:433 | `IBASE_DT_MUTANT_cutoff` | dt cutoff; `if (!(mag * (double)pow10_tbl[dec] + 0.5 < 9223372036854775808.0)) {` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/rt.c:448 | `IBASE_FNS_MUTANT_numconvert` | fns numconvert; `double x = v * (double)pow10_tbl[dec];` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/rt.c:573 | `IBASE_DT_MUTANT_numericfit` | dt numericfit; `return width; /* mutant: discard a number whose integer part fits */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/rt.c:592 | `IBASE_FNS_MUTANT_matrixb` | fns matrixb; `return width; /* regression: star-fill instead of scientific fallback */` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| core/screen.c:39 | `IBASE_MUTATE_STATUS` | status; `rt_memset(s->status,' ',80);rt_memset(s->guidance,' ',80);attr=0x07;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:51 | `IBASE_DOTPROMPT_MUTANT_scoreboard` | dotprompt scoreboard; `top=0;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:81 | `IBASE_DOTPROMPT_MUTANT_clear` | dotprompt clear; `rt_memset(s->text[0]+39,' ',41);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:87 | `IBASE_DOTPROMPT_MUTANT_bottom` | dotprompt bottom; `if(s->row>=24) { s->row=24;next_row(s); }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:90 | `IBASE_DOTPROMPT_MUTANT_blank79` | dotprompt blank79; `n=80;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:114 | `IBASE_REVIEW_MUTANT_CONTINUATION` | review continuation; `col=s->col;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:135 | `IBASE_DOTPROMPT_MUTANT_saywrap` | dotprompt saywrap; `if(s->row<24) { s->row++;return; }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:152 | `IBASE_DOTPROMPT_MUTANT_saycontrol` | dotprompt saycontrol; `char c=text[i];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/screen.c:161 | `IBASE_DOTPROMPT_MUTANT_saycontrol` | dotprompt saycontrol; `if(text[i]=='\r' // text[i]=='\n')break;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| core/value.c:76 | `IBASE_DT_MUTANT_blankkey` | dt blankkey; `if (xb_date_blank(jdn))` | test_xbase_*; test_ibase_fns/datatype; prog_diff |
| cmd/flow.c:345 | `IBASE_CORE_MUTATE_MEMVAR_RECLAIM` | core memvar reclaim; `static int memvar_reclaim(flow_state *fs, int slot, xb_val *value)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:388 | `IBASE_CORE_MUTATE_MEMVAR_RECLAIM` | core memvar reclaim; `(void)value;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:401 | `PROC_MUTATE_ASSIGN_SHADOW_OUTER` | proc assign shadow outer; `` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:427 | `IBASE_CORE_MUTATE_MEMVAR_RECLAIM` | core memvar reclaim; `if (memvar_reclaim(fs, slot, &value) != INTERP_OK)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:445 | `IBASE_CORE_MUTATE_MEMVAR_RECLAIM` | core memvar reclaim; `if (memvar_reclaim(fs, slot, &value) != INTERP_OK)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:452 | `IBASE_CORE_MUTATE_MEMVAR_RECLAIM` | core memvar reclaim; `if (v->t == XB_C // v->t == XB_M) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:473 | `IBASE_SIZE_MUTATE_MEMVAR_WIDTH` | size memvar width; `if (fs->vars[slot].val.t == XB_N) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:477 | `IBASE_FNS_MUTANT_argpicture` | fns argpicture; `union { double n; uint64_t bits; } picture;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:553 | `FLOW_MUTATE_GUARD_TRUTHY` | flow guard truthy; `xb_val v;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1053 | `IBASE_DML_MUTANT_restore` | dml restore; `if(!rc && !add)xb_interp_release_all(ip);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1101 | `IBASE_DML_MUTANT_save` | dml save; `h[11]='C'/128; /* corrupt the saved scalar type, not the oracle */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1225 | `IBASE_DML_MUTANT_clear` | dml clear; `xb_interp_release_all(ip);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1280 | `IBASE_DML_MUTANT_macro` | dml macro; `char next[FLOW_MAX_LINELEN+1];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1349 | `IBASE_FNS_MUTANT_retryconfirm` | fns retryconfirm; `return INTERP_OK; /* MUTANT: ignore RETRY during argument completion. */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1372 | `FLOW_MUTATE_NO_RETRY` | flow no retry; `if (rc == INTERP_RC_RETRY) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1383 | `FLOW_MUTATE_NO_RETRY` | flow no retry; `if (t == 2) *retry = 1;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1448 | `FLOW_MUTATE_IF_INVERT` | flow if invert; `` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1487 | `FLOW_MUTATE_DOCASE_ALL` | flow docase all; `int ran_any = 0;       /* MUTANT bookkeeping */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1525 | `FLOW_MUTATE_DOCASE_ALL` | flow docase all; `if (truth) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1567 | `FLOW_MUTATE_DOCASE_ALL` | flow docase all; `if (!ran_any && otherwise_at >= 0) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1648 | `IBASE_FNS_MUTANT_struct` | fns struct; `if (rc == -INTERP_ERR_STRUCT) return -rc;   /* MUTANT: the status leaks */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1651 | `IBASE_FNS_MUTANT_errors` | fns errors; `if (rc == -INTERP_ERR_STRUCT && xb_interp_ctx(ip)->dot_command) return 95;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1691 | `FLOW_MUTATE_NO_ESCAPE` | flow no escape; `xb_ctx *c = xb_interp_ctx(fp->ip);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1909 | `IBASE_DML_MUTANT_memory` | dml memory; `out->is_public = !out->is_public; /* expose the wrong scope */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:1953 | `PROC_MUTATE_PRIVATE_NORESTORE` | proc private norestore; `fs->vars[i].live = 0;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/flow.c:2167 | `FLOW_MUTATE_STACK_LINES` | flow stack lines; `flow_line  small_lines[FLOW_STACK_LINES];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:274 | `IBASE_DML_MUTANT_abbrscope` | dml abbrscope; `static int m_after_replace_expr(const char *s, const char *end)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:337 | `IBASE_DML_MUTANT_abbrscope` | dml abbrscope; `&& (p == s // m_after_replace_expr(s, p))` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:991 | `IBASE_MUTATE_ZAP_SAFETY` | zap safety; `return 1; /* mutant: stored SAFETY has no destructive consumer */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1118 | `IBASE_DML_MUTANT_reindex` | dml reindex; `if(count)count--; /* omit last record from every rebuilt index */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1147 | `IBASE_DML_MUTANT_index` | dml index; `if(count)count--; /* last physical record has no index entry */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1216 | `IBASE_DML_MUTANT_sort` | dml sort; `cmp=-cmp; /* descending instead of the documented ascending default */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1267 | `IBASE_DML_MUTANT_total` | dml total; `group[i].u.n+=row[i].u.n;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1302 | `IBASE_DML_MUTANT_erase` | dml erase; `return INTERP_OK; /* leave the file present */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1310 | `IBASE_DML_MUTANT_rename` | dml rename; `return INTERP_OK; /* skip the actual filesystem rename */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1323 | `IBASE_DML_MUTANT_type` | dml type; `j=0; /* suppress ASCII contents */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1329 | `IBASE_DML_MUTANT_copyfile` | dml copyfile; `buf[0]^=1; /* copied bytes differ even though state is preserved */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1361 | `IBASE_DML_MUTANT_run` | dml run; `(void)pal;(void)args;(void)ec;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/mutate.c:1397 | `IBASE_DML_MUTANT_append` | dml append; `if(r==dbf_nrec(src))break; /* drop the last incoming record */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/nav.c:285 | `NAV_MUTATE_SKIP` | nav skip; `rc = wa_goto(env, area, 1u);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/nav.c:402 | `NAV_MUTATE_SKIP` | nav skip; `if (n > 0) n++;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/nav.c:447 | `NAV_MUTATE_SKIP` | nav skip; `if (n >= 0) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:520 | `PROC_MUTATE_PARAM_ORDER` | proc param order; `v = pb->args[pb->nargs - 1 - pb->idx];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:580 | `REPL_MUTATE_DO_TRUNC` | repl do trunc; `total /= 2;                 /* existing canon DO-read mutation */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:642 | `PROC_MUTATE_KEEP_LIBRARY_CLOSE` | proc keep library close; `if (ps && ps->library` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:644 | `PROC_MUTATE_CLOSE_ACTIVE` | proc close active; `&& ps->library->refs == 1` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:655 | `PROC_MUTATE_SKIP_LIBRARY_FREE` | proc skip library free; `proc_state *ps = proc_state_for(ip);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:778 | `PROC_MUTATE_NO_DO_CAP` | proc no do cap; `if (ps->do_depth >= P_MAX_DO_NEST` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:780 | `PROC_MUTATE_DO_OFF_BY_ONE` | proc do off by one; `+ 1` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:799 | `PROC_MUTATE_SKIP_DISK` | proc skip disk; `if (depth_before) { if (ec) *ec = 16; return -INTERP_ERR_UNKNOWN_CMD; }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:858 | `PROC_MUTATE_MASTER_ONE_LEVEL` | proc master one level; `if (rc == PROC_RC_RETURN_MASTER && depth_before <= ps->master_depth)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:921 | `IBASE_FNS_MUTANT_logdoarg` | fns logdoarg; `int crc = ibase_finish_transfer(ip, ec);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:921 | `IBASE_FNS_MUTANT_ndxdoarg` | fns ndxdoarg; `int crc = ibase_finish_transfer(ip, ec);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:921 | `IBASE_FNS_MUTANT_sqrtdoarg` | fns sqrtdoarg; `int crc = ibase_finish_transfer(ip, ec);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:921 | `IBASE_FNS_MUTANT_sqrtpass` | fns sqrtpass; `int crc = ibase_finish_transfer(ip, ec);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:958 | `PROC_MUTATE_STACK_ARGS` | proc stack args; `with_frame stack_frame, *frame=&stack_frame; /* Rule 6: obsolete stack frame */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:981 | `IBASE_FNS_MUTANT_argorder` | fns argorder; `ctx->deferred_error = 0;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:997 | `IBASE_FNS_MUTANT_with` | fns with; `v.u.c.p=frame->argblk[nblk];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1012 | `PROC_MUTATE_STACK_ARGS` | proc stack args; `pal->release(pal,frame);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1069 | `IBASE_FNS_MUTANT_ndx_handler_name` | fns ndx handler name; `&& rt_kw_word(p_skip_ws(p+2), "GUARDTRAP\0", (int *)0) == 0` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1073 | `IBASE_FNS_MUTANT_ndx_handler_depth` | fns ndx handler depth; `&& ps->do_depth == 1` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1157 | `PROC_MUTATE_NO_QUIT` | proc no quit; `return INTERP_RC_QUIT;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1266 | `IBASE_FNS_MUTANT_returnarg` | fns returnarg; `return 1; /* MUTANT: lose the inline handler's RETURN outcome. */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/proc.c:1362 | `PROC_MUTATE_KEEP_STALE` | proc keep stale; `if (!ps)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:242 | `IBASE_W2_MUTANT_memowidth_ignore` | w2 memowidth ignore; `(void)ctx; return 50u;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:295 | `DEC_MUTATE_IGNORE_SETDEC` | dec ignore setdec; `if(dec)dec=2;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:430 | `IBASE_FNS_MUTANT_listoff` | fns listoff; `leading_off = q_match_kw(s, "OFF");` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:461 | `IBASE_LIST_MUTATE_SCOPE` | list scope; `p == s &&` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:495 | `IBASE_FNS_MUTANT_listoff` | fns listoff; `if(h==off_at) h=q_skip_ws(h+3);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:495 | `IBASE_LIST_MUTATE_OFF` | list off; `if(h==off_at) h=q_skip_ws(h+3);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:590 | `IBASE_NUMERIC_MUTATE_FRACTION` | numeric fraction; `if(v->u.n==(double)(int64_t)v->u.n) dec=0;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:594 | `IBASE_NUMERIC_MUTATE_LEFT` | numeric left; `{ uint32_t lead=0,i; while(lead<width && cell[lead]==' ') lead++;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:601 | `IBASE_FNS_MUTANT_exprcol` | fns exprcol; `if(v->t==XB_N) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:639 | `IBASE_MUTATE_HEADING` | heading; `(void)ip;(void)qc;(void)ec;(void)tbl;(void)p;(void)first;(void)ok;(void)fi;(void)expr;(void)line;(void)po` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:666 | `IBASE_FNS_MUTANT_exprcol` | fns exprcol; `// (!f && v.t==XB_N)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:680 | `IBASE_LIST_MUTATE_HEADING` | list heading; `while(pos>prefix && line[pos-1]==' ') pos--;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:711 | `IBASE_MUTATE_DELETE_MARK` | delete mark; `rc=wa_record_deleted(env,area);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:717 | `IBASE_LIST_MUTATE_COLUMNS` | list columns; `q_append_uint_rj(line, &pos, sizeof(line), wa_recno(env, area), 8);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:725 | `IBASE_FNS_MUTANT_numlist` | fns numlist; `else if(!pos) q_append_cstr(line,&pos,sizeof(line)," ");` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:826 | `IBASE_LIST_MUTATE_OPENING` | list opening; `if(!xb_interp_ctx(ip)->dot_command) q_putn(ip,"\r\n",2);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:827 | `IBASE_LIST_MUTATE_DOT_OPENING` | list dot opening; `if(!xb_interp_ctx(ip)->dot_command) q_putn(ip,"\r\n",2);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:852 | `IBASE_LIST_MUTATE_EOF` | list eof; `if(wa_eof(env,area)) { if(ec) *ec=0;return INTERP_OK; }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:927 | `IBASE_MUTATE_NEXT_PAST` | next past; `if (kind == QS_NEXT && visited >= limit)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:961 | `IBASE_PRINT_MUTATE_LF` | print lf; `q_append_cstr(line, &pos, sizeof(line), "\n");` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1230 | `QUERY_MUTATE_CONTINUE_SCOPE` | query continue scope; `` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1542 | `IBASE_FNS_MUTANT_ndxdisplay` | fns ndxdisplay; `preserve=0; /* MUTANT: STATUS must preserve supplied index spelling. */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1606 | `IBASE_DML_MUTANT_status` | dml status; `if(a==2)continue; /* omit one real open work area */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1631 | `IBASE_DML_MUTANT_statusinto` | dml statusinto; `(into==rel // q_isspace(into[-1])) &&` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1692 | `IBASE_W2_MUTANT_histlist` | w2 histlist; `const char *e=ss->hist[(ss->hist_next+i)%ss->hist_cap];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1771 | `IBASE_FORMS_MUTANT_narrow` | forms narrow; `if(*out<0//*out>=limit)*raw_ok=0;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1819 | `IBASE_FORMS_MUTANT_glyph` | forms glyph; `char tl=dbl?(char)0xC9:(char)0xDA,tr=dbl?(char)0xBB:(char)0xBF;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1840 | `IBASE_FORMS_MUTANT_cursor` | forms cursor; `if(x->screen_col>=80) { x->screen_col=0;x->screen_row++; }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1908 | `IBASE_FORMS_MUTANT_subcmd` | forms subcmd; `r=rt_kw_word(q_skip_ws(args),"ALL\0FIELDS\0GETS\0TYPEAHEAD\0",(int *)0);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1917 | `IBASE_FORMS_MUTANT_clear` | forms clear; `for(r=0;r<25;r++) if(qf_fill(pal,r,0,' ',80))return qf_fail(ec,29);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/query.c:1958 | `IBASE_DML_MUTANT_abbrev` | dml abbrev; `if (rt_strlen(verb) != 7) return CMD_UNKNOWN;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/report.c:68 | `IBASE_OVL_MUTATE_CROSS` | ovl cross; `extern void q_render_val(char *buf, uint32_t *pos, uint32_t cap,` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/report.c:197 | `IBASE_REPORT_MUTATE_PAGE` | report page; `n--;   /* MUTANT: off-by-one room check */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/report.c:215 | `IBASE_REPORT_MUTATE_NO_SUBTOTAL` | report no subtotal; `if (k < 2) return;   /* MUTANT: subtotal bands dropped */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/report.c:292 | `IBASE_REPORT_MUTATE_COLUMN` | report column; `+ (c == 1)   /* MUTANT: second column shifted right */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/report.c:563 | `IBASE_DML_MUTANT_label` | dml label; `take=0; /* suppress label text while retaining the band geometry */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/report.c:593 | `IBASE_OVL_MUTATE_CROSS` | ovl cross; `if (k == 99) return (int)(uintptr_t)(void *)ndx_rebuild;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/report.c:598 | `IBASE_OVL_MUTATE_REENTER` | ovl reenter; `if (k == 99) return ibase_do(ip, "");` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/report.c:614 | `IBASE_OVL_MUTATE_RESREF` | ovl resref; `if (!verb[0]) return (int)(uintptr_t)(void *)rp_run;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:100 | `IBASE_OVL_MUTATE_ESCAPE_CALLBACK` | ovl escape callback; `static IBASE_OVL_TEXT(set) void alternate_output(` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:109 | `IBASE_FNS_MUTANT_alternate` | fns alternate; `if(pal->write(pal,g_val[i].alternate,s,n)!=(int32_t)n) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:160 | `IBASE_W2_MUTANT_dohistory` | w2 dohistory; `if (!ss // !ss->dohistory // !ss->history_on // !ss->hist_cap) return INTERP_OK;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:422 | `IBASE_FNS_MUTANT_index` | fns index; `{` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:553 | `IBASE_OVL_MUTATE_ESCAPE_CALLBACK` | ovl escape callback; `ibase_bind_output(xb_interp_ctx(ip),alternate_output,ip);` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:563 | `SET_MUTATE_EXACT_DEFAULT` | set exact default; `xb_interp_ctx(ip)->set_exact = 1;    /* MUTANT: wrong default */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:668 | `SET_COLOR_MUTANT_BG` | set color bg; `bg=0; /* Rule 6: drop only the parsed background nibble. */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:846 | `IBASE_W2_MUTANT_unique_set` | w2 unique set; `on = 0;   /* mutant: SET UNIQUE ON has no effect */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:900 | `IBASE_W2_MUTANT_histcap` | w2 histcap; `return CMD_OK;   /* MUTANT: SET HISTORY TO n never resizes */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:915 | `IBASE_W2_MUTANT_histshrink` | w2 histshrink; `ss->hist_next = 0u;   /* MUTANT: a shrink restarts the write slot */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:917 | `IBASE_W2_MUTANT_histgrow` | w2 histgrow; `ss->hist_next = n ? ss->hist_next % n : 0u;   /* MUTANT: a grow keeps the write slot */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:927 | `IBASE_W2_MUTANT_histtoggle` | w2 histtoggle; `ss->hist_next = 0u;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:939 | `IBASE_MUTATE_SET_HEADINGS` | set headings; `if(which==0) on=!on;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:978 | `IBASE_W2_MUTANT_memowidth_nowrap256` | w2 memowidth nowrap256; `xb_interp_ctx(ip)->memo_width1 = (uint16_t)((d>255.0 ? 255u : (uint32_t)d) + 1u);` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:1039 | `IBASE_W2_MUTANT_margin` | w2 margin; `w=(uint32_t)(d+0.5)&0xFFFFu;       /* rounds instead of truncating */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:1093 | `IBASE_OVL_MUTATE_ESCAPE_HOOK` | ovl escape hook; `static IBASE_OVL_TEXT(set) int ovl_bad_hook(void *u,xb_interp *ip,const char *v,const char *a,int *e)` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1097 | `IBASE_OVL_MUTATE_ESCAPE_PAL` | ovl escape pal; `static IBASE_OVL_TEXT(set) void ovl_bad_pal(ibase_pal_t *p,const char *s,uint32_t n)` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1101 | `IBASE_OVL_MUTATE_ESCAPE_SAY` | ovl escape say; `static IBASE_OVL_TEXT(set) int ovl_bad_say(ibase_pal_t *p,uint8_t r,uint8_t c,const char *s,uint32_t n)` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1105 | `IBASE_OVL_MUTATE_ESCAPE_PRINT` | ovl escape print; `static IBASE_OVL_TEXT(set) int ovl_bad_print(ibase_pal_t *p,const char *s,uint32_t n)` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1109 | `IBASE_OVL_MUTATE_ESCAPE_CURSOR` | ovl escape cursor; `static IBASE_OVL_TEXT(set) uint32_t ovl_bad_cursor(void *u) { (void)u;return 1; }` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1112 | `IBASE_OVL_MUTATE_ESCAPE_ERROR` | ovl escape error; `static IBASE_OVL_TEXT(set) int ovl_bad_error(void *u,int e) { (void)u;(void)e;return 0; }` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1115 | `IBASE_OVL_MUTATE_ESCAPE_SUSPEND` | ovl escape suspend; `static IBASE_OVL_TEXT(set) int ovl_bad_suspend(xb_interp *ip) { (void)ip;return 0; }` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1133 | `IBASE_OVL_MUTATE_ESCAPE_HOOK` | ovl escape hook; `xb_interp_add_cmd_hook(ip,ovl_bad_hook,ip);` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1136 | `IBASE_OVL_MUTATE_ESCAPE_PAL` | ovl escape pal; `{ ibase_pal_t hooks=*xb_interp_pal(ip);hooks.conout=ovl_bad_pal;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1140 | `IBASE_OVL_MUTATE_ESCAPE_SAY` | ovl escape say; `{ ibase_pal_t hooks=*xb_interp_pal(ip);hooks.say_screen=ovl_bad_say;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1144 | `IBASE_OVL_MUTATE_ESCAPE_PRINT` | ovl escape print; `{ ibase_pal_t hooks=*xb_interp_pal(ip);hooks.printout=ovl_bad_print;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1148 | `IBASE_OVL_MUTATE_ESCAPE_CURSOR` | ovl escape cursor; `{ static xb_dbcursor bad;xb_ctx *c=xb_interp_ctx(ip);bad=*c->dbcur;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1152 | `IBASE_OVL_MUTATE_ESCAPE_ERROR` | ovl escape error; `{ xb_ctx *c=xb_interp_ctx(ip);ibase_bind_errors(c,ovl_bad_error,c->on_escape,c->on_error_text,c->on_escap` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1155 | `IBASE_OVL_MUTATE_ESCAPE_SUSPEND` | ovl escape suspend; `proc_set_suspend(ip,ovl_bad_suspend);` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1158 | `IBASE_OVL_MUTATE_ESCAPE_CALLBACK` | ovl escape callback; `ibase_bind_output(xb_interp_ctx(ip),alternate_output,ip);` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1162 | `IBASE_OVL_MUTATE_ESCAPE_RAW` | ovl escape raw; `xb_interp_ctx(ip)->mirror_user=(void *)(uintptr_t)set_ovl_entry;` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1165 | `IBASE_OVL_MUTATE_LIVE` | ovl live; `xb_interp_ctx(ip)->on_error(ip,10);` | test_ibase_ovl, test_ibase_ovl_lifetime; manager faults: deferred overlay smokes |
| cmd/set.c:1207 | `IBASE_FNS_MUTANT_unknown` | fns unknown; `if (err_code) *err_code = 16;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/set.c:1253 | `SET_MUTATE_KEEP_STALE` | set keep stale; `if (is_new)` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:462 | `IBASE_MUTATE_DUPLICATE_OPEN` | duplicate open; `if(wa_name_is_open(env,name,area)) return -WA_ERR_ALREADY_OPEN;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:750 | `WA_MUTATE_SELECT` | wa select; `env->cur = area;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:829 | `WA_MUTATE_RESOLVE_AREA` | wa resolve area; `wa = &env->area[0];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:838 | `IBASE_FNS_MUTANT_alias` | fns alias; `{` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1458 | `IBASE_FNS_MUTANT_errors` | fns errors; `static const uint8_t g_lex_msg[7]   = {10, 10, 36, 36, 35, 36, 67};` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1465 | `IBASE_FNS_MUTANT_errors` | fns errors; `if (rc == -INTERP_ERR_LEX) e = g_lex_msg[(unsigned)e < 7u ? e : 0];` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1521 | `IBASE_MUTATE_DELETED_VIEW` | deleted view; `if (env->deleted_on && !env->include_deleted) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1528 | `IBASE_MUTATE_FILTER_VIEW` | filter view; `if (wa->filter[0]) {` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1577 | `IBASE_FNS_MUTANT_console` | fns console; `if(!xb_interp_console_visible(ip)) return;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| cmd/workarea.c:1603 | `IBASE_MUTATE_CONSOLE` | console; `if(!xb_interp_console_visible(ip)) return;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| fs/dbf.c:101 | `DBF_MUTATE_STRIDE` | dbf stride; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:117 | `DBF_MUTATE_RECOFF` | dbf recoff; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:133 | `DBF_MUTATE_VERSION` | dbf version; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:167 | `IBASE_MUTATE_OWNER_RESET` | owner reset; `void *scratch_mark;` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:267 | `IBASE_MUTATE_OWNER_RESET` | owner reset; `pal->reset(pal,tbl->scratch_mark); /* mutant: rewind across a later live owner */` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:334 | `IBASE_MUTATE_OWNER_RESET` | owner reset; `tbl->scratch_mark=pal->alloc(pal,0u);` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:440 | `DBF_MUTATE_RECLEN` | dbf reclen; `if ((uint32_t)record_length != sum_lens + 1u)` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:470 | `IBASE_CORE_MUTATE_DBF_SIZE` | core dbf size; `if (nrec > (UINT32_MAX - (uint32_t)header_length) / record_length)` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:591 | `DBF_MUTATE_OPENRW_RO` | dbf openrw ro; `tbl->writable = 0u;          /* MUTANT: opened RW but flagged read-only */` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:654 | `DBF_MUTATE_OPENRW_EAGER_HDRLEN` | dbf openrw eager hdrlen; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:865 | `IBASE_DT_MUTANT_starread` | dt starread; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:915 | `IBASE_DT_MUTANT_blankread` | dt blankread; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1069 | `IBASE_DT_MUTANT_blankwrite` | dt blankwrite; `if (v && v->t == XB_D && !xb_date_blank(v->u.d)) {` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1208 | `IBASE_MUTATE_OWNER_RESET` | owner reset; `tbl->scratch_mark=pal->alloc(pal,0u);` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1510 | `DBF_MUTATE_DELFLAG` | dbf delflag; `` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1613 | `IBASE_MUTATE_APPEND_REWRITE` | append rewrite; `return dbf_flush(tbl); /* mutant: damages prior records and +2 geometry */` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1630 | `IBASE_MUTATE_APPEND_COUNT` | append count; `if (tbl->pal->seek(tbl->pal,tbl->fd,DBF_HDR_NREC_OFF,PAL_SEEK_SET) != DBF_HDR_NREC_OFF //` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1639 | `IBASE_MUTATE_APPEND_MEMORY` | append memory; `tbl->nrec=old;` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1701 | `IBASE_DT_MUTANT_blankwrite` | dt blankwrite; `if (v->t == XB_D && !xb_date_blank(v->u.d)) {` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1721 | `IBASE_CORE_MUTATE_DATE_WIDTH` | core date width; `rt_memcpy(dst, digits, 8u);` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1753 | `IBASE_CORE_MUTATE_MEMO_POINTER` | core memo pointer; `return -DBF_ERR_IO;` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbf.c:1796 | `IBASE_CORE_MUTATE_MEMO_POINTER` | core memo pointer; `if (f->type == 'M') {` | test_dbf_read/mutate/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbt.c:99 | `DBT_MUTATE_BLOCKSIZE` | dbt blocksize; `` | test_dbt_read/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbt.c:225 | `DBT_MUTATE_WRITE_PTR_ENDIAN` | dbt write ptr endian; `buf[0] = (uint8_t)((nf >> 24) & 0xFFu);` | test_dbt_read/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbt.c:385 | `IBASE_CORE_MUTATE_DBT_SPAN` | core dbt span; `if (len > (uint32_t)INT32_MAX - DBT_BLOCK_SIZE - 1u // start == 0u //` | test_dbt_read/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbt.c:546 | `IBASE_CORE_MUTATE_DBT_SPAN` | core dbt span; `if (blockno > (uint32_t)INT32_MAX / DBT_BLOCK_ACTUAL)` | test_dbt_read/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/dbt.c:589 | `IBASE_CORE_MUTATE_DBT_SPAN` | core dbt span; `if (scanned == work_cap) {` | test_dbt_read/roundtrip, test_ibase_core/datatype/dbsafe |
| fs/ndx.c:329 | `IBASE_CORE_MUTATE_NDX_SIZE` | core ndx size; `if (fsize_raw <= 0) {` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:591 | `NDX_MUTATE_SUBLAYOUT` | ndx sublayout; `const uint32_t child_off_in_grp = (uint32_t)NDX_GRP_CHILD_PAGE_OFF;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:846 | `NDX_MUTATE_KEY_SIGNFLIP` | ndx key signflip; `va = read_le_double(a);` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:932 | `NDX_MUTATE_ZERO_SEP_LEAF` | ndx zero sep leaf; `is_branch = node->entry_count ? node->entries[0].child_page != 0u` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:941 | `NDX_MUTATE_SEEK_CHILD` | ndx seek child; `uint32_t child = node->entries[i].child_page;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1179 | `NDX_MUTATE_SEEK_CHILD` | ndx seek child; `next_page = node->entries[i].child_page;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1465 | `NDX_MUTATE_BULK_NO_ELIDE` | ndx bulk no elide; `if (c == 1u) {` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1539 | `IBASE_CORE_MUTATE_NDX_BUILD_SIZE` | core ndx build size; `if (nrec > UINT32_MAX / key_len // nrec > UINT32_MAX / 4u //` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1578 | `IBASE_W2_MUTANT_unique_dedupe` | w2 unique dedupe; `if (0) {   /* mutant: duplicates NOT omitted */` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1606 | `NDX_MUTATE_NO_ROOT_SPLIT` | ndx no root split; `if (nleaf > 1u && (nleaf - 1u) > kpp) {` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1665 | `NDX_MUTATE_SPLIT_5050` | ndx split 5050; `fill = (leaf + 1u < nleaf) ? kpp : remaining;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:1904 | `NDX_MUTATE_INSERT_NOSORT` | ndx insert nosort; `ins = (uint32_t)count;   /* default: append at end */` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2017 | `IBASE_CORE_MUTATE_NDX_DUP_DESCENT` | core ndx dup descent; `static int subtree_high(ndx_index *idx, uint32_t page_no, uint8_t *key_out,` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2062 | `NDX_MUTATE_DELETE_NO_SEP_UPDATE` | ndx delete no sep update; `(void)idx; (void)path_pg; (void)path_ci; (void)depth; (void)key;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2103 | `IBASE_CORE_MUTATE_NDX_DUP_DESCENT` | core ndx dup descent; `(void)hk;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2130 | `IBASE_CORE_MUTATE_NDX_DUP_DESCENT` | core ndx dup descent; `if (cmp == 0) {` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2172 | `NDX_MUTATE_LEAF_SPLIT_LOW` | ndx leaf split low; `L = (kpp + 2u) / 2u;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2339 | `NDX_MUTATE_DELETE_NOSPAN` | ndx delete nospan; `return -NDX_ERR_NOTFOUND;` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| fs/ndx.c:2516 | `IBASE_W2_MUTANT_unique_rebuild` | w2 unique rebuild; `0);   /* mutant: REINDEX forgets the index's own uniqueness */` | test_ndx_build/maintain/incremental, test_ibase_core, FNS/W2 indexes |
| pal/heap.h:44 | `IBASE_MUTATE_HEAP_FREE` | heap free; `(void)head;(void)memory;return 0; /* mutant: closed storage never reusable */` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| pal/line_input.h:20 | `IBASE_MUTATE_STDIN_LINES` | stdin lines; `st->pos=st->len;` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| pal/pal_dos.c:191 | `PAL_DOS_MUTATE_SHORT_READ` | pal dos short read; `uint32_t short_read_fds; /* DBF handles only; overlays and command input intact. */` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:377 | `PAL_DOS_MUTATE_SHORT_READ` | pal dos short read; `{` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:404 | `PAL_DOS_MUTATE_SHORT_READ` | pal dos short read; `if ((uint32_t)fd < 32u) state_of(p)->short_read_fds &= ~(1u << fd);` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:443 | `PAL_DOS_MUTATE_SHORT_READ` | pal dos short read; `if (n > 1u && (uint32_t)fd < 32u && (state_of(p)->short_read_fds & (1u << fd))) {` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:493 | `PAL_DOS_MUTATE_DROP_WRITE` | pal dos drop write; `if (fd >= 3) {` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:742 | `IBASE_STREAM_MUTATE_DROP_CRLF` | stream drop crlf; `uint8_t page[4002];` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:768 | `IBASE_REVIEW_MUTANT_COLOR_RESET` | review color reset; `screen->attr=attr;` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:789 | `IBASE_REVIEW_MUTANT_KEYS` | review keys; `if(c==0) { if(dos_conin_char(p)<0)return -1;continue; }` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:866 | `IBASE_DOTPROMPT_MUTANT_enter` | dotprompt enter; `return dos_terminal_line(p,buf,cap,1);` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1023 | `IBASE_FORMS_MUTANT_wrap` | forms wrap; `dos_cursor(p,row,col);` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1035 | `IBASE_FORMS_MUTANT_wrap` | forms wrap; `if(cells && dos_write(p,fd,"\033[=7l",5)!=5) result=-PAL_EIO;` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1045 | `IBASE_REVIEW_MUTANT_REDIRECT` | review redirect; `if(dos_is_console(1))` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1054 | `IBASE_REVIEW_MUTANT_REDIRECT` | review redirect; `return dos_con_at(p,row,col,text,n,0);` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1061 | `IBASE_CURSOR25_MUTANT` | cursor25 mutant; `if(row==25)row=24;` | deferred boot/input/console/safety smokes |
| pal/pal_dos.c:1086 | `IBASE_REVIEW_MUTANT_COLOR` | review color; `state_of(p)->screen.attr=attr;` | deferred boot/input/console/safety smokes |
| boot/softfp.c:212 | `SOFTFP_MUT_ROUND` | softfp round; `(void)sticky;` | test_ibase_softfp |
| include/ibase/messages.h:24 | `IBASE_SIZE_MUTATE_MSG_PAIR` | size msg pair; `if (!token) { if (n + 1 >= cap) return 0; out[n++] = '!'; continue; }` | test_interp_*, test_ibase_dml/fns/w2/ctl/report; see IMPORT gate mapping |
| ibase_main.c:166 | `REPL_MUTATE_ERR_RENDER` | repl mutate err render | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:338 | `IBASE_MUTATE_USE_EXTENSION` | ibase mutate use extension | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:344 | `IBASE_MUTATE_USE_NDX_EXTENSION` | ibase mutate use ndx extension | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:356 | `IBASE_MUTATE_DUPLICATE_OPEN` | ibase mutate duplicate open | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:375 | `IBASE_MUTATE_USE_ERROR` | ibase mutate use error | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:402 | `IBASE_MUTATE_USE_INDEX_TOP` | ibase mutate use index top | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:420 | `IBASE_DML_MUTANT_close` | ibase dml mutant close | test_ibase_repl/audit and deferred console/boot smokes |
| ibase_main.c:472 | `REPL_MUTATE_NO_MUTATE_MODULE` | repl mutate no mutate module | test_ibase_repl/audit and deferred console/boot smokes |
