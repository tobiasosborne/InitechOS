/*
 * harness/diff/fat_diff/test_absdisk.c -- INT 25h/26h ABSOLUTE DISK oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * beads initech-4mq7 (ADR-0003 Amendment DEC-15 / OEA-ADR-0003-A3). Drives the
 * REAL artifact int25_dispatch / int26_dispatch (os/milton/int21.c) through a
 * built int_frame_t -- exactly as test_int21.c drives int21_dispatch -- with a
 * MOCK block-device seam bound from a host file-backed blockdev (blockdev_file).
 * NO QEMU: a pure host oracle on the mock seam (TEST_UNIT_GATES).
 *
 * The contract under test (spec/absdisk_int2526.json, the LOCKED data):
 *   - INT 26h WRITE then INT 25h READ round-trips byte-for-byte;
 *   - an INDEPENDENT blockdev_file_read of the same LBA == the pattern (proves
 *     the WRITE hit lba*512 in the backing store, not a cache);
 *   - NON-CORRUPTION: boot sector + both FATs + root dir are byte-identical
 *     before/after (Stop-Condition: never perturb another FAT oracle's golden);
 *   - the locked AL/AH error pairs on every error leg (invalid drive 0x0F/0x0C;
 *     out-of-range / overflow / wrap 0x08/0x0B; write-protect 0x00/0x0A on a
 *     read-only open; zero-count CF=0 no I/O; CX=0xFFFF packet rejection);
 *   - the IRETD frame is BALANCED: the dispatcher touches ONLY EFLAGS bit 0 (CF)
 *     -- the documented-and-omitted leftover-FLAGS wart (DEC-15.3, mutant M7).
 *
 * SAFE SCRATCH LBA = total_logical_sectors-1, COMPUTED from mounted geometry
 * (never a magic number; self-adjusts if the fixture changes). Its FAT entry is
 * ASSERTED FREE before the write, so the round-trip stays inside unallocated
 * data space and cannot corrupt the boot/FAT/root region or any allocated file.
 *
 * Idempotent (Rule 11): the WRITE oracle mutates the scratch sector, so the
 * Makefile mints a FRESH writable image per run AND this test restores the
 * scratch sector at teardown -- re-running is clean.
 *
 * Deterministic pattern (Rule 11): byte i = (i*0x6D + (LBA & 0xFF)) & 0xFF --
 * a PURE function of (index, LBA); no wall-clock, no rand.
 *
 * Image path is argv[1] (no host path baked in, Rule 11). ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "test_assert.h"
#include "int21.h"
#include "idt.h"
#include "fat12.h"
#include "blockdev_file.h"

TEST_HARNESS();

#define SECTOR 512u
#define CF_BIT 0x1u

/* The dispatcher reads EBX as a FLAT 32-bit linear address (uint32_t). On a
 * 64-bit host a stack/heap pointer does NOT fit in uint32_t, so any buffer the
 * test hands via EBX must live in the low 4 GiB. Mirror test_int21.c's
 * alloc_low (MAP_32BIT) so (uint32_t)(uintptr_t)p round-trips losslessly --
 * exactly the artifact's world where every pointer is < 4 GiB. The snapshot /
 * cross-check buffers go through bf.dev.read_sectors directly (a host call, not
 * the dispatcher), so only the EBX transfer buffers need low memory. */
static void *alloc_low(size_t n)
{
	void *p = MAP_FAILED;
#ifdef MAP_32BIT
	p = mmap(NULL, n, PROT_READ | PROT_WRITE,
	         MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
	if (p == MAP_FAILED) {
		return NULL;
	}
	if ((uintptr_t)p > 0xFFFFFFFFu) {
		munmap(p, n);
		return NULL;
	}
	return p;
}

/* The mounted volume's blockdev for the seam adapter thunks below. */
static const blockdev_t *g_dev = 0;

static int absdisk_read_thunk(uint32_t lba, uint32_t count, void *buf)
{
	if (g_dev == 0 || g_dev->read_sectors == 0) {
		return -1;
	}
	return g_dev->read_sectors(g_dev->ctx, lba, count, buf);
}

static int absdisk_write_thunk(uint32_t lba, uint32_t count, const void *buf)
{
	if (g_dev == 0 || g_dev->write_sectors == 0) {
		return -1;
	}
	return g_dev->write_sectors(g_dev->ctx, lba, count, buf);
}

/* CON sink capture: the CX=0xFFFF packet reject emits a grep-able marker
 * ("ABSDISK-PACKET-REJECT") through con_puts -> g_sink. We capture it so the
 * packet-rejection leg can assert the marker FIRED -- mutant M8 (which skips
 * the reject and treats 0xFFFF as a literal count) emits NO marker, so its
 * assertion goes RED for the RIGHT reason (the surfaced AL/AH alone is the same
 * 0x08/0x0B as a genuine bounds error, so the marker is the distinguishing
 * signal). Mirrors test_int21.c's sink_capture. */
static char   g_sink_buf[256];
static size_t g_sink_len;

static void sink_capture(char c)
{
	if (g_sink_len + 1u < sizeof(g_sink_buf)) {
		g_sink_buf[g_sink_len++] = c;
		g_sink_buf[g_sink_len] = '\0';
	}
}

static void sink_reset(void) { g_sink_len = 0u; g_sink_buf[0] = '\0'; }
static const char *sink_str(void) { return g_sink_buf; }

/* Deterministic 512B pattern (DEC-15 oracle_plan step 3): pure (i, LBA). */
static void fill_pattern(uint8_t *buf, uint32_t lba)
{
	for (uint32_t i = 0; i < SECTOR; i++) {
		buf[i] = (uint8_t)(((i * 0x6Du) + (lba & 0xFFu)) & 0xFFu);
	}
}

/* Build a fresh frame with a KNOWN-junk EFLAGS (high bits set) so we can assert
 * the dispatcher touches ONLY bit 0 (CF) -- the balanced-IRETD/omitted-wart
 * guarantee (DEC-15.3, mutant M7). Mirrors test_int21.c's fresh_frame(). */
static int_frame_t fresh_frame(void)
{
	int_frame_t f;
	memset(&f, 0, sizeof(f));
	f.eflags = 0x00000A02u;   /* IF + bit1 + bit9 + bit11 set; CF clear */
	return f;
}

/* The backing image file size in bytes (-1 on stat failure). A correct single-
 * sector write to the LAST volume sector leaves the file at exactly
 * total_sectors*512; M6 (write count+1) writes one sector PAST the volume end,
 * extending the image -- so an unchanged file size is the intrinsic
 * non-corruption signal for the off-the-end neighbor (which is not covered by
 * the boot/FAT/root snapshot since it lies past the data region). */
static long image_size(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0) {
		return -1;
	}
	return (long)st.st_size;
}

static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }
static uint16_t frame_ax(const int_frame_t *f) { return (uint16_t)(f->eax & 0xFFFFu); }
static uint8_t  frame_al(const int_frame_t *f) { return (uint8_t)(f->eax & 0xFFu); }
static uint8_t  frame_ah(const int_frame_t *f) { return (uint8_t)((f->eax >> 8) & 0xFFu); }

/* Assert the dispatcher left EFLAGS balanced: every bit EXCEPT bit 0 (CF) is
 * byte-identical to the pre-call value. This is the host-observable form of the
 * uniform-IRETD/no-stack-imbalance guarantee -- mutant M7 (which models the
 * leftover-FLAGS wart by scribbling a non-CF EFLAGS bit) goes RED here. */
static int eflags_balanced(uint32_t before, uint32_t after)
{
	return (before & ~CF_BIT) == (after & ~CF_BIT);
}

/* Set AL=drive, CX=count, DX=startLBA, EBX=buffer on a frame (the DEC-15
 * register layout: the buffer is EBX, the start sector is DX -- the role swap). */
static void set_inputs(int_frame_t *f, uint8_t drive, uint16_t count,
                       uint16_t start_lba, const void *buf)
{
	f->eax = (uint32_t)drive;                 /* AL = drive (AH unused) */
	f->ecx = (uint32_t)count;                 /* CX = sector count */
	f->edx = (uint32_t)start_lba;             /* DX = start LBA */
	f->ebx = (uint32_t)(uintptr_t)buf;        /* EBX = flat transfer buffer */
}

int main(int argc, char **argv)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	const char     *img;
	uint8_t         sector_buf[SECTOR];
	uint8_t         fatbuf[12u * SECTOR];
	uint32_t        total_sectors;
	uint32_t        scratch_lba;
	uint16_t        scratch_cluster;
	uint16_t        next;
	long            size_before;
	int             rc;

	/* The EBX transfer buffers MUST live in the low 4 GiB (EBX is 32-bit). */
	uint8_t *pattern;
	uint8_t *readback;
	/* Snapshots for the non-corruption proof + the scratch restore (host-side
	 * read_sectors calls, not through the dispatcher -- normal buffers OK). */
	uint8_t  crosscheck[SECTOR];
	uint8_t  scratch_before[SECTOR];
	uint8_t  boot_before[SECTOR], boot_after[SECTOR];
	uint8_t  fat_before[18u * SECTOR], fat_after[18u * SECTOR];   /* both FATs */
	uint8_t  root_before[14u * SECTOR], root_after[14u * SECTOR]; /* root dir  */

	if (argc < 2) {
		fprintf(stderr, "usage: %s <writable-fat12-image>\n", argv[0]);
		return 2;
	}
	img = argv[1];

	int21_set_sink(sink_capture);   /* capture the packet-reject marker (M8) */

	pattern  = (uint8_t *)alloc_low(SECTOR);
	readback = (uint8_t *)alloc_low(SECTOR);
	CHECK(pattern != NULL && readback != NULL,
	      "alloc_low: EBX transfer buffers must live in the low 4 GiB (32-bit EBX)");
	if (pattern == NULL || readback == NULL) {
		return TEST_SUMMARY("test_absdisk");
	}

	/* ---- open RW + mount + bind the absolute-disk seam (DEC-15 steps 1) ---- */
	rc = blockdev_file_open_rw(&bf, img);
	CHECK(rc == 0, "blockdev_file_open_rw should succeed on the minted image");
	if (rc != 0) {
		return TEST_SUMMARY("test_absdisk");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount on the writable image");
	if (rc != FAT12_OK) {
		blockdev_file_close(&bf);
		return TEST_SUMMARY("test_absdisk");
	}

	g_dev = &bf.dev;
	total_sectors = vol.bpb.total_sectors_16 != 0u
	                  ? (uint32_t)vol.bpb.total_sectors_16
	                  : vol.bpb.total_sectors_32;
	CHECK(total_sectors == 2880u,
	      "1.44MB FAT12 geometry: total_logical_sectors == 2880");

	{
		int21_absdisk_backend_t seam;
		seam.read          = absdisk_read_thunk;
		seam.write         = absdisk_write_thunk;
		seam.total_sectors = total_sectors;
		int21_set_blockdev(&seam);
	}

	/* ---- SAFE scratch LBA = total-1, FAT entry asserted FREE (step 2) ------ */
	scratch_lba = total_sectors - 1u;   /* 2879 on the 1.44MB floppy */
	CHECK(fat12_read_fat(&vol, fatbuf, sizeof(fatbuf)) == FAT12_OK,
	      "fat12_read_fat fills the whole-FAT buffer");
	/* Map the data LBA to its FAT cluster: cluster 2 starts at first_data_sector.
	 * scratch_cluster = (lba - first_data_sector)/spc + 2. */
	scratch_cluster = (uint16_t)(((scratch_lba - vol.first_data_sector)
	                              / vol.bpb.sectors_per_cluster) + 2u);
	next = 0xFFFFu;
	CHECK(fat12_next_cluster(&vol, fatbuf, sizeof(fatbuf), scratch_cluster, &next)
	      == FAT12_OK, "decode the scratch sector's FAT entry");
	CHECK(next == 0x000u,
	      "scratch LBA's FAT entry MUST be free (0x000) before we touch it (Rule 2)");
	if (next != 0x000u) {
		/* Fail loud: refuse to write into allocated space (Stop-Condition). */
		blockdev_file_close(&bf);
		return TEST_SUMMARY("test_absdisk");
	}

	/* ---- snapshot the golden regions + the scratch sector (steps 7 setup) -- */
	CHECK(bf.dev.read_sectors(bf.dev.ctx, 0u, 1u, boot_before) == 0,
	      "snapshot boot sector (LBA 0) before");
	CHECK(bf.dev.read_sectors(bf.dev.ctx, vol.first_fat_sector,
	      (uint32_t)vol.bpb.num_fats * vol.bpb.sectors_per_fat, fat_before) == 0,
	      "snapshot both FATs before");
	CHECK(bf.dev.read_sectors(bf.dev.ctx, vol.root_dir_sector,
	      vol.root_dir_sectors, root_before) == 0,
	      "snapshot root directory before");
	CHECK(bf.dev.read_sectors(bf.dev.ctx, scratch_lba, 1u, scratch_before) == 0,
	      "snapshot the scratch sector before (for idempotent teardown)");
	size_before = image_size(img);
	CHECK(size_before == (long)total_sectors * (long)SECTOR,
	      "image is exactly total_sectors*512 before (geometry sanity)");

	fill_pattern(pattern, scratch_lba);

	/* ===== [4] INT 26h WRITE (AL=0, CX=1, DX=scratch, EBX=pattern) ========= */
	{
		int_frame_t f = fresh_frame();
		uint32_t    eflags0;
		f.eflags |= CF_BIT;   /* preload CF=1 to prove WRITE clears it on success */
		eflags0 = f.eflags;
		set_inputs(&f, 0u, 1u, (uint16_t)scratch_lba, pattern);
		int26_dispatch(&f);
		CHECK(frame_cf(&f) == 0, "INT 26h WRITE success -> CF=0");
		CHECK(eflags_balanced(eflags0, f.eflags),
		      "INT 26h WRITE: dispatcher touches ONLY EFLAGS bit 0 (balanced IRETD; M7)");
	}

	/* ===== [5] INT 25h READ the same LBA, assert == pattern (round-trip) === */
	{
		int_frame_t f = fresh_frame();
		uint32_t    eflags0 = f.eflags;
		memset(readback, 0, SECTOR);
		set_inputs(&f, 0u, 1u, (uint16_t)scratch_lba, readback);
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 0, "INT 25h READ success -> CF=0");
		CHECK(eflags_balanced(eflags0, f.eflags),
		      "INT 25h READ: dispatcher touches ONLY EFLAGS bit 0 (balanced IRETD; M7)");
		CHECK(memcmp(readback, pattern, SECTOR) == 0,
		      "round-trip: INT 25h READ returns EXACTLY what INT 26h WROTE");
	}

	/* ===== [6] CROSS-CHECK via an INDEPENDENT blockdev_file_read =========== */
	{
		memset(crosscheck, 0, sizeof(crosscheck));
		CHECK(bf.dev.read_sectors(bf.dev.ctx, scratch_lba, 1u, crosscheck) == 0,
		      "independent blockdev_file_read of the scratch LBA");
		CHECK(memcmp(crosscheck, pattern, SECTOR) == 0,
		      "cross-check: the WRITE hit lba*512 in the backing store, not a cache");
	}

	/* ===== [7] NON-CORRUPTION: boot + both FATs + root unchanged ========== */
	CHECK(bf.dev.read_sectors(bf.dev.ctx, 0u, 1u, boot_after) == 0,
	      "snapshot boot sector after");
	CHECK(bf.dev.read_sectors(bf.dev.ctx, vol.first_fat_sector,
	      (uint32_t)vol.bpb.num_fats * vol.bpb.sectors_per_fat, fat_after) == 0,
	      "snapshot both FATs after");
	CHECK(bf.dev.read_sectors(bf.dev.ctx, vol.root_dir_sector,
	      vol.root_dir_sectors, root_after) == 0,
	      "snapshot root directory after");
	CHECK(memcmp(boot_before, boot_after, SECTOR) == 0,
	      "NON-CORRUPTION: boot sector byte-identical after the round-trip");
	CHECK(memcmp(fat_before, fat_after,
	             (size_t)vol.bpb.num_fats * vol.bpb.sectors_per_fat * SECTOR) == 0,
	      "NON-CORRUPTION: both FATs byte-identical after the round-trip");
	CHECK(memcmp(root_before, root_after,
	             (size_t)vol.root_dir_sectors * SECTOR) == 0,
	      "NON-CORRUPTION: root directory byte-identical after the round-trip");
	/* The single-sector write to the LAST volume sector must NOT grow the image
	 * (M6 writes one sector PAST the end -- off-the-end neighbor corruption that
	 * the boot/FAT/root snapshot cannot see, but the file size does). */
	CHECK(image_size(img) == size_before,
	      "NON-CORRUPTION: image size unchanged (no off-the-end neighbor write; M6)");

	/* ===== [8] ERROR-PATH legs -- each asserts the LOCKED AL/AH pair ====== */

	/* (a) invalid drive (AL=1) -> AL=0x0F / AH=0x0C. */
	{
		int_frame_t f = fresh_frame();
		set_inputs(&f, 1u, 1u, (uint16_t)scratch_lba, readback);
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "invalid drive (AL=1) -> CF=1");
		CHECK(frame_al(&f) == 0x0Fu, "invalid drive -> AL=0x0F");
		CHECK(frame_ah(&f) == 0x0Cu, "invalid drive -> AH=0x0C");
	}

	/* (b) out-of-range start (DX >= total) -> AL=0x08 / AH=0x0B. total_sectors
	 * (2880 = 0xB40) fits in the 16-bit DX, and DX==total >= total trips the
	 * bounds check at its very first sector. */
	{
		int_frame_t f = fresh_frame();
		set_inputs(&f, 0u, 1u, (uint16_t)total_sectors, readback);
		f.edx = total_sectors;   /* explicit: the start LBA == total (out of range) */
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "out-of-range (DX>=total) -> CF=1");
		CHECK(frame_al(&f) == 0x08u, "out-of-range -> AL=0x08 (sector not found)");
		CHECK(frame_ah(&f) == 0x0Bu, "out-of-range -> AH=0x0B");
	}

	/* (c) count overflow (DX + CX > total) -> AL=0x08 / AH=0x0B. */
	{
		int_frame_t f = fresh_frame();
		set_inputs(&f, 0u, 4u, (uint16_t)(total_sectors - 2u), readback);
		f.edx = total_sectors - 2u;   /* last-2; CX=4 -> DX+CX = total+2 > total */
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "count overflow (DX+CX>total) -> CF=1");
		CHECK(frame_al(&f) == 0x08u, "count overflow -> AL=0x08");
		CHECK(frame_ah(&f) == 0x0Bu, "count overflow -> AH=0x0B");
	}

	/* (d) zero count (CX=0) -> CF=0, no I/O (DOS contract). */
	{
		int_frame_t f = fresh_frame();
		f.eflags |= CF_BIT;   /* preload CF=1 to prove zero-count clears it */
		set_inputs(&f, 0u, 0u, (uint16_t)scratch_lba, readback);
		int26_dispatch(&f);   /* WRITE with CX=0: must touch nothing, succeed */
		CHECK(frame_cf(&f) == 0, "zero count (CX=0) -> CF=0 success");
		/* Prove it touched nothing: the scratch sector still holds the pattern. */
		memset(crosscheck, 0, sizeof(crosscheck));
		bf.dev.read_sectors(bf.dev.ctx, scratch_lba, 1u, crosscheck);
		CHECK(memcmp(crosscheck, pattern, SECTOR) == 0,
		      "zero count performed NO I/O (scratch sector unchanged)");
	}

	/* (e) CX=0xFFFF packet sentinel -> CF=1 reject + the serial diagnostic
	 * marker; NEVER a literal 65535-sector read off the end (DEC-15.5). The
	 * marker is the load-bearing distinguishing signal: a genuine bounds error
	 * surfaces the SAME 0x08/0x0B, so M8 (which treats 0xFFFF as a literal count
	 * and skips the reject) emits NO marker -> this leg goes RED for M8. The
	 * start LBA is 0 so a literal-count read would NOT even hit a bounds error
	 * at DX itself -- it is the DX+CX overflow that the mutant trips, and the
	 * absent marker is what proves the reject path was taken. */
	{
		int_frame_t f = fresh_frame();
		sink_reset();
		set_inputs(&f, 0u, 0xFFFFu, 0u, readback);
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "CX=0xFFFF packet form -> CF=1 (rejected)");
		CHECK(frame_ax(&f) != 0u, "CX=0xFFFF reject sets a non-zero error AX");
		CHECK(strstr(sink_str(), "ABSDISK-PACKET-REJECT") != NULL,
		      "CX=0xFFFF emits the loud packet-reject diagnostic (never a literal count)");
		/* It surfaces the sector-not-found pair (the coherent surfaced error). */
		CHECK(frame_al(&f) == 0x08u && frame_ah(&f) == 0x0Bu,
		      "CX=0xFFFF reject surfaces sector-not-found (never a literal count)");
	}

	/* (f) write-protect: a READ-ONLY backend (write==NULL) -> INT 26h
	 * AL=0x00 / AH=0x0A. Bind a seam whose write member is NULL (the read-only
	 * blockdev_file_open semantics) and confirm. */
	{
		int_frame_t f = fresh_frame();
		int21_absdisk_backend_t ro;
		ro.read          = absdisk_read_thunk;
		ro.write         = 0;                 /* read-only backend */
		ro.total_sectors = total_sectors;
		int21_set_blockdev(&ro);
		set_inputs(&f, 0u, 1u, (uint16_t)scratch_lba, pattern);
		int26_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "write-protect (write==NULL) -> CF=1");
		CHECK(frame_al(&f) == 0x00u, "write-protect -> AL=0x00");
		CHECK(frame_ah(&f) == 0x0Au, "write-protect -> AH=0x0A (MSG-DOS-0008)");
		/* Re-bind the writable seam for teardown. */
		{
			int21_absdisk_backend_t seam;
			seam.read          = absdisk_read_thunk;
			seam.write         = absdisk_write_thunk;
			seam.total_sectors = total_sectors;
			int21_set_blockdev(&seam);
		}
	}

	/* (g) no seam bound at all -> fail loud, never fault (Rule 2). */
	{
		int_frame_t f = fresh_frame();
		int21_set_blockdev(0);   /* clear the seam */
		set_inputs(&f, 0u, 1u, (uint16_t)scratch_lba, readback);
		int25_dispatch(&f);
		CHECK(frame_cf(&f) == 1, "no seam bound -> CF=1 (fail loud, no fault)");
		CHECK(frame_al(&f) == 0x0Cu && frame_ah(&f) == 0x0Bu,
		      "no seam bound -> general failure (0x0C/0x0B)");
		/* Re-bind for teardown. */
		{
			int21_absdisk_backend_t seam;
			seam.read          = absdisk_read_thunk;
			seam.write         = absdisk_write_thunk;
			seam.total_sectors = total_sectors;
			int21_set_blockdev(&seam);
		}
	}

	/* ---- TEARDOWN: restore the scratch sector so a re-run is idempotent --- */
	CHECK(bf.dev.write_sectors(bf.dev.ctx, scratch_lba, 1u, scratch_before) == 0,
	      "teardown: restore the scratch sector (Rule 11 idempotent)");

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_absdisk");
}
