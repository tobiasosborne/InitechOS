/* NEWWINDOW over sdk/host. FLAIR's gate takes WDEF byte 4, the Mac's
 * noGrowDocProc, a document window without grow box or scroll bars
 * (flair/README.md, 3eda89aa); sdk/host still accepts only WDEF 0 and RDOC.
 * TEMPORARY until the SDK names it TBX_WDEF_NOGROW and its host fake accepts
 * it (phase-3 frame-owner-2 request to the SDK owner): this shim hands
 * sdk/host the window without the WDEF byte and records it.
 * Content rects use FLAIR's measured insets, never Calc's: the document
 * window's content starts (6,22) inside its frame and stops (21,21) short of
 * the bottom right (CalcDocContentRect, s8_doc_window_active); the no-grow
 * window's starts (6,22) in and stops (6,6) short (Mac OS 8.1 Get Info,
 * s8_get_info_doc.png: content 13..299 x 47..364 for outline 7..305 x
 * 25..370). */
#ifndef CALC_TEST_HOST_WINDOWS_H
#define CALC_TEST_HOST_WINDOWS_H
#define HOST_WDEF_NOGROW (4u<<TBX_WDEF_SHIFT)
static inline int32_t host_new_window(int32_t l,int32_t t,int32_t r,int32_t b,int32_t g)
{
    if(((uint32_t)g&~255u)!=HOST_WDEF_NOGROW) return tbx_new_window(l,t,r,b,g);
    int32_t w=tbx_new_window(l,t,r,b,(int32_t)((uint32_t)g&255u));
    if(w>0) tbx_host.windows[w-1].goaway=g;
    return w;
}
static inline int host_plain(int32_t w) { return ((uint32_t)tbx_host.windows[w-1].goaway&~255u)==HOST_WDEF_NOGROW; }
static inline int host_inset_w(int32_t w) { return host_plain(w)?12:27; }   /* left + right */
static inline int host_inset_h(int32_t w) { return host_plain(w)?28:43; }   /* top + bottom */
#define tbx_new_window host_new_window
#endif
