/*
 * os/apps/itext/te_fonts.c -- the ONE copy of the font metric tables inside
 * TEXT.EXE (THE ARTIFACT; bead initech-tdnl.16).
 *
 * os/flair/textedit.c measures with the Font Manager's header-only advances
 * (os/flair/text.h text_face_advance), whose tables are `extern const` in a
 * freestanding build (spec/assets/font_linkage.h: the kernel defines them once
 * in os/flair/text.c). A disk application links its own copy: this unit
 * defines them, exactly as text.c does for the kernel, and the link's
 * --gc-sections drops what the image never reads (the glyph rows).
 * ASCII-clean (Rule 12).
 */
#define FONT_TABLES_DEFINE 1
#include "chicago12.h"
#include "geneva9.h"
#include "geneva12.h"   /* the Geneva 12 advances (bead initech-uelk) */
