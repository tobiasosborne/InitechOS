dbf_coerce_fuzz_SRCS := tests/dbf_coerce_fuzz.c
dbf_coerce_fuzz_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
dbf_coerce_fuzz_CFLAGS := -D_POSIX_C_SOURCE=200809L
ibase_numeric_format_SRCS := tests/numeric_format_driver.c
ibase_numeric_format_OBJS := $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
ibase_numeric_format_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_canon_salami_SRCS := tests/test_canon_salami.c ibase_main.c
test_canon_salami_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_canon_salami_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_canon_y2k_SRCS := tests/test_canon_y2k.c ibase_main.c
test_canon_y2k_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_canon_y2k_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbase_roundtrip_SRCS := tests/test_dbase_roundtrip.c
test_dbase_roundtrip_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_dbase_roundtrip_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbf_fields_SRCS := tests/test_dbf_fields.c
test_dbf_fields_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbf_fields_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbf_header_SRCS := tests/test_dbf_header.c
test_dbf_header_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbf_header_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbf_mutate_SRCS := tests/test_dbf_mutate.c
test_dbf_mutate_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbf_mutate_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbf_read_SRCS := tests/test_dbf_read.c
test_dbf_read_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbf_read_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbf_roundtrip_SRCS := tests/test_dbf_roundtrip.c
test_dbf_roundtrip_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbf_roundtrip_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbt_read_SRCS := tests/test_dbt_read.c
test_dbt_read_OBJS := $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbt_read_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_dbt_roundtrip_SRCS := tests/test_dbt_roundtrip.c
test_dbt_roundtrip_OBJS := $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_dbt_roundtrip_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_decimals_SRCS := tests/test_interp_decimals.c
test_interp_decimals_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_decimals_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_do_file_SRCS := tests/test_interp_do_file.c ibase_main.c
test_interp_do_file_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_do_file_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_flow_SRCS := tests/test_interp_flow.c
test_interp_flow_OBJS := $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_flow_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_list_SRCS := tests/test_interp_list.c
test_interp_list_OBJS := $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_list_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_nav_SRCS := tests/test_interp_nav.c
test_interp_nav_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_nav_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_proc_SRCS := tests/test_interp_proc.c
test_interp_proc_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_proc_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_replace_SRCS := tests/test_interp_replace.c
test_interp_replace_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_replace_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_set_SRCS := tests/test_interp_set.c
test_interp_set_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_set_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_setfmt_SRCS := tests/test_interp_setfmt.c
test_interp_setfmt_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_setfmt_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_interp_use_SRCS := tests/test_interp_use.c
test_interp_use_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_interp_use_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_build_SRCS := tests/test_ndx_build.c
test_ndx_build_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ndx_build_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_incremental_SRCS := tests/test_ndx_incremental.c
test_ndx_incremental_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_incremental_CFLAGS :=
test_ndx_keys_SRCS := tests/test_ndx_keys.c
test_ndx_keys_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_keys_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_maintain_SRCS := tests/test_ndx_maintain.c
test_ndx_maintain_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_maintain_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_multilevel_SRCS := tests/test_ndx_multilevel.c
test_ndx_multilevel_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_multilevel_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_pack_SRCS := tests/test_ndx_pack.c
test_ndx_pack_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ndx_pack_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_parse_SRCS := tests/test_ndx_parse.c
test_ndx_parse_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_parse_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ndx_seek_SRCS := tests/test_ndx_seek.c
test_ndx_seek_OBJS := $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
test_ndx_seek_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_use_SRCS := tests/test_ibase_audit.c ibase_main.c
test_ibase_use_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_use_CFLAGS := -D_POSIX_C_SOURCE=200809L -DTEST_AUDIT_USE
test_ibase_core_SRCS := tests/test_ibase_core.c
test_ibase_core_OBJS := $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/pal/pal_host.o
test_ibase_core_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_ctl_SRCS := tests/test_ibase_ctl.c ibase_main.c
test_ibase_ctl_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_ctl_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_datatype_SRCS := tests/test_ibase_datatype.c ibase_main.c
test_ibase_datatype_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_datatype_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_dbsafe_SRCS := tests/test_ibase_dbsafe.c ibase_main.c
test_ibase_dbsafe_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_dbsafe_CFLAGS := -DDBASE3_DECOMP='"$(DBASE3_DECOMP)"'
test_ibase_deferred_errors_SRCS := tests/test_ibase_deferred_errors.c ibase_main.c
test_ibase_deferred_errors_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_deferred_errors_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_dml_SRCS := tests/test_ibase_dml.c ibase_main.c
test_ibase_dml_OBJS := $(OUT)/host-core/dml/cmd/workarea.o $(OUT)/host-core/dml/cmd/nav.o $(OUT)/host-core/dml/cmd/flow.o $(OUT)/host-core/dml/cmd/query.o $(OUT)/host-core/dml/cmd/mutate.o $(OUT)/host-core/dml/cmd/set.o $(OUT)/host-core/dml/cmd/proc.o $(OUT)/host-core/dml/cmd/report.o $(OUT)/host-core/dml/fs/dbf.o $(OUT)/host-core/dml/fs/dbt.o $(OUT)/host-core/dml/fs/ndx.o $(OUT)/host-core/dml/core/eval.o $(OUT)/host-core/dml/core/parse.o $(OUT)/host-core/dml/core/lex.o $(OUT)/host-core/dml/core/value.o $(OUT)/host-core/dml/core/rt.o $(OUT)/host-core/dml/core/fn_builtins.o $(OUT)/host-core/dml/pal/pal_host.o
test_ibase_dml_CFLAGS := -D_POSIX_C_SOURCE=200809L -DFLOW_MEMVAR_ARENA=6000
test_ibase_fns_SRCS := tests/test_ibase_fns.c ibase_main.c
test_ibase_fns_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_fns_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_input_SRCS := tests/test_ibase_input.c
test_ibase_input_OBJS :=
test_ibase_input_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_memocells_SRCS := tests/test_ibase_memocells.c ibase_main.c
test_ibase_memocells_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_memocells_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_messages_SRCS := tests/test_ibase_messages.c
test_ibase_messages_OBJS :=
test_ibase_messages_CFLAGS :=
test_ibase_ndx_names_SRCS := tests/test_ibase_ndx_names.c ibase_main.c
test_ibase_ndx_names_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_ndx_names_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_ovl_SRCS := tests/test_ibase_ovl.c
test_ibase_ovl_OBJS := $(OUT)/host-core/core/ovl.o $(OUT)/host-core/core/rt.o
test_ibase_ovl_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_ovl_lifetime_SRCS := tests/test_ibase_ovl_lifetime.c
test_ibase_ovl_lifetime_OBJS := $(OUT)/host-core/lifetime/core/ovl.o $(OUT)/host-core/lifetime/core/rt.o
test_ibase_ovl_lifetime_CFLAGS := -DIBASE_OVERLAYS
test_ibase_ovl_lifetime_LIBS := -Wl,--gc-sections,--defsym=__ovl_slot_end=__ovl_slot+256
test_ibase_pal_contract_SRCS := tests/test_ibase_pal_contract.c
test_ibase_pal_contract_OBJS := $(OUT)/host-core/pal/pal_null.o
test_ibase_pal_contract_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_pal_host_SRCS := tests/test_ibase_pal_host.c
test_ibase_pal_host_OBJS := $(OUT)/host-core/pal/pal_host.o
test_ibase_pal_host_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_query_SRCS := tests/test_ibase_query.c ibase_main.c
test_ibase_query_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_query_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_repl_SRCS := tests/test_ibase_repl.c ibase_main.c
test_ibase_repl_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_repl_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_report_SRCS := tests/test_ibase_report.c ibase_main.c
test_ibase_report_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_report_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_rt_SRCS := tests/test_ibase_rt.c
test_ibase_rt_OBJS := $(OUT)/host-core/core/rt.o
test_ibase_rt_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_say_SRCS := tests/test_ibase_say.c
test_ibase_say_OBJS := $(OUT)/host-core/core/screen.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_say_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_forms_SRCS := tests/test_ibase_forms.c tests/w2_host.c
test_ibase_forms_OBJS := $(test_ibase_say_OBJS)
test_ibase_forms_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_set_color_SRCS := tests/test_ibase_set_color.c
test_ibase_set_color_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_set_color_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_softfp_SRCS := tests/test_ibase_softfp.c
test_ibase_softfp_OBJS := $(OUT)/host-core/boot/softfp.o
test_ibase_softfp_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_spec_SRCS := tests/test_ibase_spec.c
test_ibase_spec_OBJS :=
test_ibase_spec_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_status_SRCS := tests/test_ibase_status.c
test_ibase_status_OBJS := $(OUT)/host-core/core/screen.o $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_status_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_value_SRCS := tests/test_ibase_value.c
test_ibase_value_OBJS := $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o
test_ibase_value_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_use_rw_SRCS := tests/test_use_rw.c
test_use_rw_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_use_rw_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_w2_host_SRCS := tests/test_w2_host.c tests/w2_host.c
test_w2_host_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_w2_host_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_eval_SRCS := tests/test_xbase_eval.c
test_xbase_eval_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
test_xbase_eval_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_fn_a_SRCS := tests/test_xbase_fn_a.c
test_xbase_fn_a_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
test_xbase_fn_a_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_fn_b_SRCS := tests/test_xbase_fn_b.c
test_xbase_fn_b_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
test_xbase_fn_b_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_fn_c_SRCS := tests/test_xbase_fn_c.c
test_xbase_fn_c_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
test_xbase_fn_c_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_fn_d_SRCS := tests/test_xbase_fn_d.c
test_xbase_fn_d_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_xbase_fn_d_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_lex_SRCS := tests/test_xbase_lex.c
test_xbase_lex_OBJS := $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/rt.o
test_xbase_lex_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_parse_SRCS := tests/test_xbase_parse.c
test_xbase_parse_OBJS := $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/rt.o
test_xbase_parse_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_xbase_transform_SRCS := tests/test_xbase_transform.c
test_xbase_transform_OBJS := $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o
test_xbase_transform_CFLAGS := -D_POSIX_C_SOURCE=200809L
prog_diff_SRCS := tests/xbase_prog_diff/prog_diff.c
prog_diff_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
prog_diff_CFLAGS := -D_POSIX_C_SOURCE=200809L
test_ibase_view_SRCS := tests/test_ibase_audit.c ibase_main.c
test_ibase_view_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_view_CFLAGS :=
test_ibase_w2_SRCS := tests/test_ibase_fns.c tests/w2_host.c
test_ibase_w2_OBJS := $(OUT)/host-core/cmd/workarea.o $(OUT)/host-core/cmd/nav.o $(OUT)/host-core/cmd/flow.o $(OUT)/host-core/cmd/query.o $(OUT)/host-core/cmd/mutate.o $(OUT)/host-core/cmd/set.o $(OUT)/host-core/cmd/proc.o $(OUT)/host-core/cmd/report.o $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/fs/ndx.o $(OUT)/host-core/core/eval.o $(OUT)/host-core/core/parse.o $(OUT)/host-core/core/lex.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/core/fn_builtins.o $(OUT)/host-core/pal/pal_host.o
test_ibase_w2_CFLAGS := -DIBASE_W2
w2_fixture_SRCS := tests/w2_fixture.c
w2_fixture_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
w2_fixture_CFLAGS := -D_POSIX_C_SOURCE=200809L
mint_clients_dbf_SRCS := tests/mint_clients_dbf.c
mint_clients_dbf_OBJS := $(OUT)/host-core/fs/dbf.o $(OUT)/host-core/fs/dbt.o $(OUT)/host-core/core/value.o $(OUT)/host-core/core/rt.o $(OUT)/host-core/pal/pal_host.o
mint_clients_dbf_CFLAGS :=
ibase_core_ref_SRCS := tests/ibase_core_ref.c
ibase_core_ref_OBJS :=
ibase_core_ref_CFLAGS := -D_POSIX_C_SOURCE=200809L

conformance_host_SRCS := tests/conformance/host.c tests/w2_host.c
conformance_host_OBJS := $(test_w2_host_OBJS)
conformance_host_CFLAGS := -D_POSIX_C_SOURCE=200809L
conformance_host_DEPS := ibase_main.c
