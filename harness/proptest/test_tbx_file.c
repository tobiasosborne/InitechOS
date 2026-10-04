/* initech-tdnl.91: independent expectations over the real DOS dispatcher.
 * Ref: spec/toolbox_gate.h Sec 10; test_fileio.c low-address mock precedent.
 * Factory C; the backend captures calls/bytes, not gate-calculated expectations. */
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include "test_assert.h"
#include "tbxfile.h"
#include "devices.h"
TEST_HARNESS();
#define ASSERT_TRUE(msg, cond) CHECK(cond, msg)
#define ASSERT_EQ(msg, got, want) CHECK((got) == (want), msg)
static uint8_t bytes[64];
static uint32_t size, calls;
static uint16_t open_file(const char *name, uint16_t dir, dir_entry_t *e, uint32_t *slot)
{
    (void)name; calls++;
    if (dir != 0) return 3;
    memset(e, 0, sizeof *e); e->file_size = size; *slot = 0; return 0;
}
static uint16_t create_file(const char *name, uint16_t dir, dir_entry_t *e, uint32_t *slot)
{ size = 0; return open_file(name, dir, e, slot); }
static uint16_t read_file(const dir_entry_t *e, uint32_t off, uint8_t *b, uint32_t n, uint32_t *got)
{
    (void)e; calls++; *got = off < size ? size - off : 0;
    if (*got > n) *got = n;
    memcpy(b, bytes + (off < size ? off : 0), *got); return 0;
}
static uint16_t write_file(uint16_t dir, uint32_t slot,
                          uint32_t off, const uint8_t *b, uint32_t n, uint32_t *wrote, dir_entry_t *e)
{
    (void)dir; (void)slot; calls++;
    if (off + n > sizeof bytes) return 5;
    memcpy(bytes + off, b, n); size = off + n; e->file_size = size; *wrote = n; return 0;
}
static uint16_t unlink_file(const char *name, uint16_t dir)
{ (void)name; (void)dir; calls++; return 2; }
static int32_t call(psp_t *p, uint32_t lo, uint32_t code, uint32_t a, uint32_t b, uint32_t c)
{ uint32_t args[3] = { a, b, c }; return tbxf_call(p, lo, 4096, code, args); }
int main(void)
{
    psp_t owner, other;
    int21_file_backend_t backend = { 0 };
    uint8_t *mem = mmap(0, 8192, PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    ASSERT_TRUE("low memory", mem != MAP_FAILED);
    if (mem == MAP_FAILED) return 1;
    uint32_t lo = (uint32_t)(uintptr_t)mem;
    strcpy((char *)mem, "TEST.DAT"); memcpy(mem + 2048, "abcde", 5);
    backend.open = open_file; backend.create = create_file;
    backend.read_at = read_file; backend.write_at = write_file; backend.unlink = unlink_file;
    sft_init(); tbxf_init(&owner); tbxf_init(&other);
    int21_set_file_backend(&backend); int21_set_psp(&other);
    int32_t h = call(&owner, lo, TBX_FILE_CREATE, lo, 0, 0);
    ASSERT_TRUE("F1 create returns owned file", h >= 5);
    ASSERT_EQ("F1 write BSS bytes", call(&owner, lo, TBX_FILE_WRITE, h, lo+2048, 5), 5);
    ASSERT_EQ("F1 independent bytes", memcmp(bytes, "abcde", 5), 0);
    ASSERT_EQ("F1 read denied on write handle", call(&owner, lo, TBX_FILE_READ, h, lo+2050, 2), TBX_ERR_BADARG);
    ASSERT_EQ("F1 close", call(&owner, lo, TBX_FILE_CLOSE, h, 0, 0), 0);
    ASSERT_EQ("F1 double close refused", call(&owner, lo, TBX_FILE_CLOSE, h, 0, 0), TBX_ERR_BADARG);
    h = call(&owner, lo, TBX_FILE_OPEN, lo, 0, 0);
    ASSERT_EQ("F1 end seek signed", call(&owner, lo, TBX_FILE_SEEK, h, (uint32_t)-2, 2), 3);
    ASSERT_EQ("F1 read tail", call(&owner, lo, TBX_FILE_READ, h, lo+3000, 5), 2);
    ASSERT_EQ("F1 tail bytes", memcmp(mem+3000, "de", 2), 0);
    ASSERT_EQ("F1 seek underflow", call(&owner, lo, TBX_FILE_SEEK, h, (uint32_t)-6, 2), TBX_ERR_BADARG);
    ASSERT_EQ("F1 seek overflow", call(&owner, lo, TBX_FILE_SEEK, h, 0x7fffffff, 2), TBX_ERR_BADARG);
    ASSERT_EQ("F1 bad origin", call(&owner, lo, TBX_FILE_SEEK, h, 0, 3), TBX_ERR_BADARG);
    uint32_t before = calls;
    ASSERT_EQ("F2 bounds below allocation", call(&owner, lo, TBX_FILE_READ, h, lo-1, 1), TBX_ERR_BADARG);
    ASSERT_EQ("F2 bounds past allocation", call(&owner, lo, TBX_FILE_READ, h, lo+4095, 2), TBX_ERR_BADARG);
    ASSERT_EQ("F2 bounds wrapping span", call(&owner, lo, TBX_FILE_READ, h, 0xfffffff0, 32), TBX_ERR_BADARG);
    ASSERT_EQ("F2 invalid path", call(&owner, lo, TBX_FILE_OPEN, lo-1, 0, 0), TBX_ERR_BADARG);
    memset(mem+3968, 'X', 128);
    ASSERT_EQ("F2 unterminated path", call(&owner, lo, TBX_FILE_DELETE, lo+3968, 0, 0), TBX_ERR_BADARG);
    ASSERT_EQ("F2 no backend access", calls, before);
    ASSERT_EQ("F2 empty span", call(&owner, lo, TBX_FILE_READ, h, 0, 0), 0);
    ASSERT_EQ("F2 bad mode", call(&owner, lo, TBX_FILE_OPEN, lo, 3, 0), TBX_ERR_BADARG);
    ASSERT_EQ("F3 standard handle", call(&owner, lo, TBX_FILE_CLOSE, 1, 0, 0), TBX_ERR_BADARG);
    ASSERT_EQ("F3 foreign handle", call(&other, lo, TBX_FILE_READ, h, lo+2048, 1), TBX_ERR_BADARG);
    ASSERT_TRUE("F3 caller PSP restored", int21_get_psp() == &other);
    int21_cwd_snapshot_t cwd = { 7, "OTHER" }; int21_cwd_restore(&cwd);
    ASSERT_EQ("F3 DOS error encoding", call(&owner, lo, TBX_FILE_DELETE, lo, 0, 0), TBX_ERR_DOS(2));
    int21_cwd_snapshot_t now = int21_cwd_save();
    ASSERT_EQ("F3 caller CWD restored", now.start_cluster, 7);
    ASSERT_EQ("F3 caller CWD text", strcmp(now.path, "OTHER"), 0);
    devices_init(); int21_set_devices(devices_head(),0);
    strcpy((char *)mem+200,"A:\\NUL.TXT");
    uint32_t live = tbxf_live(&owner);
    ASSERT_EQ("F3 devices refused", call(&owner,lo,TBX_FILE_OPEN,lo+200,0,0),TBX_ERR_BADARG);
    ASSERT_EQ("F3 device slot released", tbxf_live(&owner),live);
    before = calls;
    strcpy((char *)mem+400,"nul.txt");
    int32_t reserved = call(&owner,lo,TBX_FILE_CREATE,lo+400,0,0);
    ASSERT_EQ("F3 CREATE refuses device basename",reserved,TBX_ERR_BADARG);
    ASSERT_EQ("F3 CREATE device never touches backend",calls,before);
    if (reserved >= 5) (void)call(&owner,lo,TBX_FILE_CLOSE,reserved,0,0);
    int32_t alien = call(&other,lo,TBX_FILE_OPEN,lo,0,0);
    ASSERT_TRUE("F4 unrelated owner open", alien >= 5);
    tbxf_reap(&owner);
    ASSERT_EQ("F4 cleanup owns no handles", tbxf_live(&owner), 0);
    ASSERT_EQ("F4 unrelated owner retained", tbxf_live(&other), 1);
    ASSERT_EQ("F4 unrelated owner usable",call(&other,lo,TBX_FILE_READ,alien,lo+3000,2),2);
    for (uint32_t cycle = 0; cycle < 32; cycle++) {
        tbxf_init(&owner);
        for (uint32_t i = 0; i < 15; i++)
            ASSERT_TRUE("F4 repeated open", call(&owner, lo, TBX_FILE_OPEN, lo, 0, 0) >= 5);
        ASSERT_EQ("F4 exhaustion encoded", call(&owner, lo, TBX_FILE_OPEN, lo, 0, 0), TBX_ERR_DOS(4));
        tbxf_reap(&owner); tbxf_reap(&owner);
        ASSERT_EQ("F4 cleanup owns no handles", tbxf_live(&owner), 0);
    }
    ASSERT_EQ("F4 unrelated owner survives all cycles",tbxf_live(&other),1);
    tbxf_reap(&other);
    for (uint32_t i = SFT_FIRST_FILE; i < SFT_MAX_ENTRIES; i++)
        ASSERT_EQ("F4 no SFT leak", g_sft[i].kind, SFT_KIND_FREE);
    munmap(mem,8192); return TEST_SUMMARY("test-tbx-file");
}
