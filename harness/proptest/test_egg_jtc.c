/* Independent transcription of operator-authored JTC contract. Law 2:
 * expected labels and coordinates are literal; never read the renderer's JSON.
 * The real table came through JSON -> C packer -> bin2c, so this grades both. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egg_jtc_model.h"
#include "event.h"
#define CHECK(c, s) do { if (!(c)) { puts("FAIL JTC " s); return 1; } } while (0)
int main(void)
{
    static const char *const labels[] = {
        "It's a Bad Idea.", "Ask Bob.", "PC LOAD LETTER", "Re-Do the Cover Sheet",
        "Nobody Will Notice", "Take it to Lumbergh", "Move Your Desk",
        "Just a Moment.", "That Would Be Great.", "Is This Good for the Company?"
    };
    rgn_rect_t c = {102,116,407,509};
    CHECK(egg_jtc_count() == 10, "pad count (expected 10)");
    CHECK(strcmp(egg_jtc_title(), "Jump to Conclusions") == 0, "title");
    CHECK(strcmp(egg_jtc_heading(), "CONCLUSIONS") == 0, "heading");
    const unsigned values[] = {5,2,180,44,8,12,28,110,80,420,348,3};
    for (unsigned i = 0; i < JTC_WORDS; i++) CHECK(egg_jtc_value(i) == values[i], "JSON word");
    for (int i = 0; i < 10; i++) {
        rgn_rect_t p = egg_jtc_pad(c, i);
        int x = 128 + (i % 2)*188, y = 142 + (i / 2)*52;
        CHECK(strcmp(egg_jtc_label(i), labels[i]) == 0, "JSON label");
        CHECK(p.left == x && p.top == y && p.right == x+180 && p.bottom == y+44, "pad rectangle");
    }
    /* Exhaust the content and its surround, including every gap and edge. */
    for (int y = 90; y < 420; y++) for (int x = 100; x < 540; x++) {
        int want = -1;
        for (int i = 0; i < 10; i++) {
            int l = 128+(i%2)*188, t = 142+(i/2)*52;
            if (x >= l && x < l+180 && y >= t && y < t+44) want = i;
        }
        CHECK(egg_jtc_hit(c,x,y) == want, "half-open hit/gap");
    }
    c.left += 17; c.top -= 9;
    CHECK(egg_jtc_hit(c,145,133) == 0, "dragged layout");
    unsigned n = 0;
    EventRecord e = {0}; e.what = mouseDown; e.where.h = 16; e.where.v = 10;
    CHECK(!egg_jtc_trigger(&n,&e) && n == 0, "ordinary Apple click");
    e.modifiers = FLAIR_EVT_MOD_OPTION_KEY;
    CHECK(!egg_jtc_trigger(&n,&e) && n == 1, "first hidden click");
    CHECK(!egg_jtc_trigger(&n,&e) && n == 2, "second hidden click");
    CHECK(egg_jtc_trigger(&n,&e) && n == 3, "third hidden click");
    e.what = keyUp; e.modifiers = 0;
    CHECK(!egg_jtc_trigger(&n,&e) && n == 0, "release resets");
    e.what = mouseDown; e.modifiers = FLAIR_EVT_MOD_OPTION_KEY;
    (void)egg_jtc_trigger(&n,&e); e.where.h = 33;
    CHECK(!egg_jtc_trigger(&n,&e) && n == 0, "outside resets");
    flair_raw_ring_t ring;
    flair_event_init(&ring);
    flair_event_warp(218,372);
    flair_raw_event_t raw = {FLAIR_RAW_MOUSE,0,1u | (1u << 8) | (2u << 16)};
    CHECK(flair_raw_post(&ring,&raw), "warp input queued");
    (void)WaitNextEvent(&ring,everyEvent,&e,0);
    CHECK(e.where.h == 219 && e.where.v == 374, "relative input follows jumped hotspot");
    flair_event_warp(-1,999);
    (void)WaitNextEvent(&ring,everyEvent,&e,0);
    CHECK(e.where.h == 0 && e.where.v == 479, "warp clamps to screen");
    puts(">>> test-egg-jtc: green (10 JSON pads, labels, exhaustive layout/hits, hidden trigger)");
    return 0;
}
