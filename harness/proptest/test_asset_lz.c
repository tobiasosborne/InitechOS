/* harness/proptest/test_asset_lz.c -- storage equality for the packed strikes
 * (kernel size round 5, WL-0168; the test-chicago-pack shape of WL-0152).
 *
 * Decodes every block of the COMMITTED os/flair/asset_lz_data.h with the
 * kernel's own decoder (os/flair/asset_lz.h, compiled here unchanged) and
 * compares the result, byte for byte, with the UNCHANGED locked tables in
 * spec/assets. The locked tables are the oracle; nothing here is reconstructed
 * through the packer. The strikes' appearance stays graded by the historical
 * pixel gates (test-finder-icon-art, test-flair-finder-icons, the boot-screen
 * gate) against their captures (Law 2).
 */
#define ASSET_LZ_DESK
#define ASSET_LZ_ALERT
#define ASSET_LZ_SMALL
#define ASSET_LZ_LOGO
#include <stdio.h>
#include <string.h>

#include "desk_icons.h"
#include "finder_icons.h"
#include "alert_icons.h"
#include "finder_small_icons.h"
#include "boot_logo.h"
#include "asset_lz.h"
#include "asset_lz_data.h"

static int fails;

static void check(const char *what, const uint8_t *packed, uint32_t raw,
                  const void *const *parts, const unsigned *sizes, unsigned n)
{
    static uint8_t out[8192];
    unsigned off = 0;

    if (raw > sizeof out) {
        printf("!!! asset-lz FAIL: %s raw %u exceeds the harness buffer\n", what, raw);
        fails++;
        return;
    }
    memset(out, 0xA5, sizeof out);
    asset_lz_unpack(out, raw, packed);
    for (unsigned p = 0; p < n; p++) {
        const uint8_t *want = (const uint8_t *)parts[p];
        for (unsigned i = 0; i < sizes[p]; i++) {
            if (out[off + i] != want[i]) {
                printf("!!! asset-lz FAIL: %s part=%u byte=%u got=%02X expected=%02X\n",
                       what, p, i, out[off + i], want[i]);
                fails++;
                return;
            }
        }
        off += sizes[p];
    }
    if (off != raw) {
        printf("!!! asset-lz FAIL: %s decoded %u B, locked parts total %u B\n", what, raw, off);
        fails++;
        return;
    }
    if (out[raw] != 0xA5u) {
        printf("!!! asset-lz FAIL: %s wrote past its %u-byte output\n", what, raw);
        fails++;
        return;
    }
    printf("    %s: %u B decoded == locked\n", what, raw);
}

int main(void)
{
    const void *desk[6] = { &FLAIR_DESK_ICON_VOLUME, &FLAIR_DESK_ICON_TRASH,
                            &FLAIR_DESK_ICON_TRASH_FULL, &FLAIR_FINDER_ICON_FOLDER,
                            &FLAIR_FINDER_ICON_DOC, &FLAIR_FINDER_ICON_APP };
    const unsigned desk_n[6] = { 528u, 528u, 528u, 528u, 528u, 528u };
    const void *alert[1] = { &FLAIR_ALERT_ICON_CAUTION };
    const unsigned alert_n[1] = { 528u };
    const void *small[1] = { finder_small_art };
    const unsigned small_n[1] = { (unsigned)sizeof finder_small_art };
    const void *logo[1] = { boot_logo_rle };
    const unsigned logo_n[1] = { (unsigned)sizeof boot_logo_rle };

    check("desk", asset_lz_desk, ASSET_LZ_DESK_RAW, desk, desk_n, 6u);
    check("alert", asset_lz_alert, ASSET_LZ_ALERT_RAW, alert, alert_n, 1u);
    check("small", asset_lz_small, ASSET_LZ_SMALL_RAW, small, small_n, 1u);
    check("logo", asset_lz_logo, ASSET_LZ_LOGO_RAW, logo, logo_n, 1u);
    if (fails) return 1;
    printf(">>> test-asset-lz: green (4 packed blocks decode == the unchanged locked strikes)\n");
    return 0;
}
