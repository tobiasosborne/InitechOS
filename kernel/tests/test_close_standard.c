/* DOS 3.30 CLOSE3 capture: tests/fixtures/close3-dos33. */
#define main original_fileio_main
#include "test_fileio.c"
#undef main

static int_frame_t dos_call(uint16_t ax, uint8_t handle, uint8_t target,
                            uint32_t path)
{
    int_frame_t f = fresh_frame();
    f.eax = ax;
    f.ebx = handle;
    f.ecx = target;
    f.edx = path;
    int21_dispatch(&f);
    return f;
}

int main(void)
{
    int21_set_break_flag(0);
    uint32_t path = low_dup("HELLO.TXT");
    if (!path) return 2;
    mock_reset_dir();
    int21_set_file_backend(&g_mock_backend);

    for (uint8_t handle = 0; handle < 5; handle++) {
        bind_standard_process();
        psp_t child;
        psp_params_t params = { .parent_jft = g_test_psp.jft };
        psp_build(&child, &params);
        sft_inherit(&child);
        int21_set_psp(&child);

        uint8_t resident = child.jft[handle];
        sft_entry_t saved;
        memcpy(&saved, &g_sft[resident], sizeof saved);

        int_frame_t f = dos_call(0x3e00, handle, 0, 0);
        CHECK(!frame_cf(&f) && child.jft[handle] == JFT_CLOSED,
              "CLOSE releases the child's standard handle");
        CHECK(g_test_psp.jft[handle] == resident &&
              !memcmp(&saved, &g_sft[resident], sizeof saved),
              "CLOSE preserves the parent's handle and pinned device");

        f = dos_call(0x3e00, handle, 0, 0);
        CHECK(frame_cf(&f) && (uint16_t)f.eax == 6,
              "CLOSE of the closed standard handle returns error 6");

        f = dos_call(0x3d00, 0, 0, path);
        CHECK(!frame_cf(&f) && (uint16_t)f.eax == handle,
              "OPEN reuses the closed standard handle");
        f = dos_call(0x3e00, handle, 0, 0);
        CHECK(!frame_cf(&f) && child.jft[handle] == JFT_CLOSED &&
              sft_file_slots_in_use() == 0,
              "CLOSE of the reused handle releases the last file reference");

        bind_standard_process();
        f = dos_call(0x3d00, 0, 0, path);
        CHECK(!frame_cf(&f) && (uint16_t)f.eax == 5,
              "OPEN with all standard handles occupied returns handle 5");
        f = dos_call(0x4600, 5, handle, 0);
        sft_entry_t *file = sft_from_handle(&g_test_psp, 5);
        CHECK(!frame_cf(&f) && file && file->ref_count == 2 &&
              g_test_psp.jft[handle] == g_test_psp.jft[5],
              "DUP2 assigns a file reference to the standard handle");

        f = dos_call(0x3e00, handle, 0, 0);
        CHECK(!frame_cf(&f) && g_test_psp.jft[handle] == JFT_CLOSED &&
              file && file->kind == SFT_KIND_FILE && file->ref_count == 1,
              "CLOSE of the redirected standard handle preserves the other reference");
        f = dos_call(0x3e00, 5, 0, 0);
        CHECK(!frame_cf(&f) && sft_file_slots_in_use() == 0,
              "CLOSE of the remaining handle releases the file");
    }
    return TEST_SUMMARY("close-standard");
}
