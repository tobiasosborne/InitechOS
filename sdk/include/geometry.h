/* Public QuickDraw coordinates, split from spec/grafport.h (IM-I I-127)
 * and region_algebra.h. Field order and widths are unchanged. */
#ifndef INITECH_SDK_GEOMETRY_H
#define INITECH_SDK_GEOMETRY_H
#include <stdint.h>
#include <stddef.h>
typedef struct flair_point {
    int16_t v;
    int16_t h;
} flair_point_t;
typedef struct tbx_rect {
    int16_t top, left, bottom, right;
} tbx_rect_t;
_Static_assert(sizeof(flair_point_t) == 4, "Point size");
_Static_assert(offsetof(flair_point_t, h) == 2, "Point.h");
_Static_assert(sizeof(tbx_rect_t) == 8, "Rect size");
_Static_assert(offsetof(tbx_rect_t, right) == 6, "Rect.right");
#endif
