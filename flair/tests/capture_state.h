#ifndef COPY_CAPTURE_STATE_H
#define COPY_CAPTURE_STATE_H
#include <stdint.h>
/* Target's 32-bit cursor layout, observed only by the external debugger.
 * Readiness supplies no expected shape, colour or pixel from the renderer. */
typedef struct {
    uint32_t surface[6], saved[256], depth;
    int32_t x,y,origin_x,origin_y,bound,visible,drawn;
} copy_cursor_state;
static inline int copy_capture_ready(uint32_t pc,uint32_t idle,uint32_t idle_size,
    const copy_cursor_state *s)
{
    return pc>=idle && pc-idle<idle_size && s->bound && s->visible &&
        s->drawn && !s->depth && s->x==520 && s->y==350;
}
#endif
