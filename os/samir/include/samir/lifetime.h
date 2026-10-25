/* Overlay lifetime boundaries. Ref: docs/design/samir-overlays.md Sec 6;
 * operator review resolution: retained code/data must never name the slot.
 * Public ctx/PAL structs also receive a boundary audit on overlay return,
 * before any subsequent eviction, so direct field writes cannot bypass it.
 * Host builds have no overlay slot; bindings have identical semantics there. */
#ifndef SAMIR_LIFETIME_H
#define SAMIR_LIFETIME_H
#include <stdint.h>
#include "samir/eval.h"
#include "samir/pal.h"
#ifdef SAMIR_OVERLAYS
void samir_ovl_require_resident(uintptr_t p, const char *where);
void samir_ovl_validate_ctx(const xb_ctx *ctx);
void samir_ovl_validate_pal(const samir_pal_t *pal);
#else
static inline void samir_ovl_require_resident(uintptr_t p, const char *where)
{ (void)p; (void)where; }
static inline void samir_ovl_validate_ctx(const xb_ctx *ctx) { (void)ctx; }
static inline void samir_ovl_validate_pal(const samir_pal_t *pal) { (void)pal; }
#endif
#define SAMIR_RETAIN(p) samir_ovl_require_resident((uintptr_t)(p), #p)
static inline void samir_bind_output(xb_ctx *c,
    void (*fn)(void *, const char *, uint32_t), void *user)
{
    SAMIR_RETAIN(c); SAMIR_RETAIN(fn); SAMIR_RETAIN(user);
    c->mirror_output=fn; c->mirror_user=user;
}
static inline void samir_bind_resolver(xb_ctx *c,
    int (*fn)(void *,const char *,uint16_t,xb_val *), void *user)
{
    SAMIR_RETAIN(c); SAMIR_RETAIN(fn); SAMIR_RETAIN(user);
    c->resolve=fn; c->user=user;
}
static inline void samir_bind_cursor(xb_ctx *c,const xb_dbcursor *v,void *user)
{
    SAMIR_RETAIN(c); SAMIR_RETAIN(v); SAMIR_RETAIN(user);
    /* Validate the nested vtable before publishing it. */
    if(v) {
#define CURSOR_CHECK(f) SAMIR_RETAIN(v->f)
        CURSOR_CHECK(recno); CURSOR_CHECK(reccount); CURSOR_CHECK(eof);
        CURSOR_CHECK(bof); CURSOR_CHECK(found); CURSOR_CHECK(deleted);
        CURSOR_CHECK(field_name); CURSOR_CHECK(dbf_name);
        CURSOR_CHECK(file_exists); CURSOR_CHECK(last_update);
#undef CURSOR_CHECK
    }
    c->dbcur=v; c->dbcur_user=user;
}
static inline void samir_bind_errors(xb_ctx *c,int (*error)(void *,int),
    int (*escape)(void *),const char *error_text,const char *escape_text)
{
    SAMIR_RETAIN(c); SAMIR_RETAIN(error); SAMIR_RETAIN(escape);
    SAMIR_RETAIN(error_text); SAMIR_RETAIN(escape_text);
    c->on_error=error; c->on_escape=escape;
    c->on_error_text=error_text; c->on_escape_text=escape_text;
}
static inline void samir_bind_pal(samir_pal_t *dest,const samir_pal_t *hooks)
{
    SAMIR_RETAIN(dest); samir_ovl_validate_pal(hooks); *dest=*hooks;
}
#endif
