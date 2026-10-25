/* Ref: docs/design/samir-overlays.md Sec 6 rule 4; Phase B retained-field
 * contract. Poison every original field independently, including the last
 * cursor/PAL slots. Compare the exact original diagnostic and offending
 * address, not just the fact that an audit refused something. The QEMU guard
 * gate separately proves activation context, UD2 and completed panic traces.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "samir/lifetime.h"
#include "test_assert.h"
TEST_HARNESS();

/* The link defines __ovl_slot_end as exactly this array's one-past address. */
char __ovl_slot[256];
extern char __ovl_slot_end[];
static jmp_buf panic_return;
static const char *expected;
static uintptr_t bad_pointer;
static xb_ctx binding;

void pal_milton_overlay_panic(const char *why,uint32_t loaded,
    uint32_t requested,uint32_t depth,uintptr_t p)
{
    (void)loaded; (void)requested; (void)depth;
    CHECK(expected && !strcmp(why,expected),"exact retained-field diagnostic");
    CHECK(p == bad_pointer,"original offending pointer preserved");
    longjmp(panic_return,1);
}

typedef struct { size_t offset; const char *name; } field;
#define PAL(f) {offsetof(samir_pal_t,f), "pal->" #f}
static const field pal_contract[] = {
    PAL(open), PAL(close), PAL(read), PAL(write), PAL(seek), PAL(remove),
    PAL(rename), PAL(conout), PAL(conin_line), PAL(conin_char), PAL(gotoxy),
    PAL(set_attr), PAL(today), PAL(alloc), PAL(reset), PAL(acquire),
    PAL(release), PAL(status), PAL(conin_line_noecho), PAL(exec),
    PAL(conin_escape), PAL(curdrive), PAL(getenv), PAL(os_version),
    PAL(diskspace), PAL(conin_poll), PAL(time), PAL(iscolor), PAL(say_screen),
    PAL(printout)
};
#define CTX(f) {offsetof(xb_ctx,f), "c->" #f}
static const field ctx_contract[] = {
    CTX(mirror_output), CTX(mirror_user), CTX(resolve), CTX(user),
    CTX(scratch), CTX(dbcur), CTX(dbcur_user), CTX(file_pal), CTX(record_size),
    CTX(on_error), CTX(message), CTX(on_escape), CTX(program_name),
    CTX(on_error_text), CTX(on_escape_text)
};
#define CUR(f) {offsetof(xb_dbcursor,f), #f}
static const field cursor_contract[] = {
    CUR(recno), CUR(reccount), CUR(eof), CUR(bof), CUR(found), CUR(deleted),
    CUR(field_name), CUR(dbf_name), CUR(file_exists), CUR(last_update),
    CUR(index_name)
};
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))
static void audit_pal(void *p) { samir_ovl_validate_pal(p); }
static void audit_ctx(void *p) { samir_ovl_validate_ctx(p); }
static void bind_cursor(void *p) { samir_bind_cursor(&binding,p,NULL); }
static void audit_cursor(void *p)
{
    xb_ctx c = {0};
    c.dbcur = p;
    samir_ovl_validate_ctx(&c);
}
static void audit_nested_pal(void *p)
{
    xb_ctx c = {0};
    c.file_pal = p;
    samir_ovl_validate_ctx(&c);
}

static void expect_panic(void (*audit)(void *),void *p,const char *why)
{
    expected = why;
    if (!setjmp(panic_return)) {
        audit(p);
        fprintf(stderr,"missing lifetime refusal: %s\n",why);
        CHECK(0,"retained slot pointer refused");
    }
}
static void poison_fields(void *object,size_t size,const field *fields,
    size_t n,void (*audit)(void *),const char *prefix)
{
    /* Both ends of the half-open slot interval must be rejected. */
    for (size_t i=0; i<n; ++i) {
        char why[64];
        snprintf(why,sizeof why,"%s%s",prefix,fields[i].name);
        for (unsigned end=0; end<2; ++end) {
            bad_pointer = (uintptr_t)__ovl_slot + (end ? 255 : 0);
            memset(object,0,size);
            memset(&binding,0,sizeof binding);
            memcpy((char *)object + fields[i].offset,&bad_pointer,sizeof bad_pointer);
            expect_panic(audit,object,why);
            CHECK(!binding.dbcur,"bad cursor never published");
        }
    }
}
static void range_check(void *p)
{
    samir_ovl_require_resident((uintptr_t)p,"range");
}
int main(void)
{
    samir_pal_t pal = {0};
    xb_ctx c = {0};
    xb_dbcursor v = {0};
    CHECK((uintptr_t)__ovl_slot_end == (uintptr_t)__ovl_slot + 256,
        "test slot has exact half-open bounds");
    samir_ovl_validate_pal(NULL);
    samir_ovl_validate_ctx(NULL);
    samir_ovl_validate_pal(&pal);
    samir_ovl_validate_ctx(&c);
    samir_bind_cursor(&binding,NULL,NULL);
    samir_bind_cursor(&binding,&v,NULL);
    CHECK(binding.dbcur == &v,"resident cursor published");
    range_check(NULL);
    range_check((void *)((uintptr_t)__ovl_slot - 1));
    range_check(__ovl_slot_end);
    range_check((void *)UINTPTR_MAX);
    bad_pointer = (uintptr_t)__ovl_slot;
    expect_panic(range_check,__ovl_slot,"range");
    bad_pointer = (uintptr_t)__ovl_slot_end - 1;
    expect_panic(range_check,(void *)bad_pointer,"range");
    bad_pointer = (uintptr_t)__ovl_slot;
    expect_panic(audit_pal,__ovl_slot,"pal");
    expect_panic(audit_ctx,__ovl_slot,"c");
    expect_panic(bind_cursor,__ovl_slot,"v");
    poison_fields(&pal,sizeof pal,pal_contract,COUNT(pal_contract),audit_pal,"");
    poison_fields(&pal,sizeof pal,pal_contract,COUNT(pal_contract),audit_nested_pal,"");
    poison_fields(&c,sizeof c,ctx_contract,COUNT(ctx_contract),audit_ctx,"");
    poison_fields(&v,sizeof v,cursor_contract,COUNT(cursor_contract),bind_cursor,"v->");
    poison_fields(&v,sizeof v,cursor_contract,COUNT(cursor_contract),audit_cursor,"c->dbcur->");
    /* Multiple faults keep the original audit precedence. */
    memset(&c,0,sizeof c);
    memset(&v,0,sizeof v);
    memset(&pal,0,sizeof pal);
    bad_pointer = (uintptr_t)__ovl_slot;
    memcpy(&pal.printout,&bad_pointer,sizeof bad_pointer);
    c.dbcur = &v; c.file_pal = &pal;
    c.mirror_user = __ovl_slot;
    v.recno = (uint32_t (*)(void *))(uintptr_t)__ovl_slot;
    bad_pointer = (uintptr_t)__ovl_slot;
    expect_panic(audit_ctx,&c,"c->mirror_user");
    c.mirror_user = NULL;
    expect_panic(audit_ctx,&c,"pal->printout");
    memset(&pal,0,sizeof pal);
    expect_panic(audit_ctx,&c,"c->dbcur->recno");
    return TEST_SUMMARY("samir-ovl-lifetime");
}
