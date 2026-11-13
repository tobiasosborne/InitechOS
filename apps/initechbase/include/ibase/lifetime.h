/* Overlay lifetime boundaries. Ref: docs/design/ibase-overlays.md Sec 6;
 * operator review resolution: retained code/data must never name the slot.
 * Public ctx/PAL structs also receive a boundary audit on overlay return,
 * before any subsequent eviction, so direct field writes cannot bypass it.
 * Host builds have no overlay slot; bindings have identical semantics there. */
#ifndef IBASE_LIFETIME_H
#define IBASE_LIFETIME_H
#include <stdint.h>
#include "ibase/eval.h"
#include "ibase/pal.h"
#ifdef IBASE_OVERLAYS
void ibase_ovl_require_resident(uintptr_t p, const char *where);
void ibase_ovl_validate_ctx(const xb_ctx *ctx);
void ibase_ovl_validate_pal(const ibase_pal_t *pal);
void ibase_ovl_validate_cursor(const xb_dbcursor *v);
#else
static inline void ibase_ovl_require_resident(uintptr_t p, const char *where)
{ (void)p; (void)where; }
static inline void ibase_ovl_validate_ctx(const xb_ctx *ctx) { (void)ctx; }
static inline void ibase_ovl_validate_pal(const ibase_pal_t *pal) { (void)pal; }
static inline void ibase_ovl_validate_cursor(const xb_dbcursor *v) { (void)v; }
#endif
#define IBASE_RETAIN(p) ibase_ovl_require_resident((uintptr_t)(p), #p)
static inline void ibase_bind_output(xb_ctx *c,
    void (*fn)(void *, const char *, uint32_t), void *user)
{
    IBASE_RETAIN(c); IBASE_RETAIN(fn); IBASE_RETAIN(user);
    c->mirror_output=fn; c->mirror_user=user;
}
static inline void ibase_bind_resolver(xb_ctx *c,
    int (*fn)(void *,const char *,uint16_t,xb_val *), void *user)
{
    IBASE_RETAIN(c); IBASE_RETAIN(fn); IBASE_RETAIN(user);
    c->resolve=fn; c->user=user;
}
static inline void ibase_bind_cursor(xb_ctx *c,const xb_dbcursor *v,void *user)
{
    IBASE_RETAIN(c); IBASE_RETAIN(v); IBASE_RETAIN(user);
    /* Validate the nested vtable before publishing it. */
    ibase_ovl_validate_cursor(v);
    c->dbcur=v; c->dbcur_user=user;
}
static inline void ibase_bind_errors(xb_ctx *c,int (*error)(void *,int),
    int (*escape)(void *),const char *error_text,const char *escape_text)
{
    IBASE_RETAIN(c); IBASE_RETAIN(error); IBASE_RETAIN(escape);
    IBASE_RETAIN(error_text); IBASE_RETAIN(escape_text);
    c->on_error=error; c->on_escape=escape;
    c->on_error_text=error_text; c->on_escape_text=escape_text;
}
static inline void ibase_bind_pal(ibase_pal_t *dest,const ibase_pal_t *hooks)
{
    IBASE_RETAIN(dest); ibase_ovl_validate_pal(hooks); *dest=*hooks;
}
#endif
