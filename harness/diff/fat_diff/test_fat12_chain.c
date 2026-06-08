/*
 * harness/diff/fat_diff/test_fat12_chain.c -- FAT12 decode + chain-walk oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * RED->GREEN gate for os/milton/fat12.c::{fat12_read_fat, fat12_next_cluster,
 * fat12_walk_chain} (beads initech-adf, FAT12 decode + chain-walk slice).
 *
 * It mounts the same minted 1.44 MB FAT12 image as test_fat12_bpb.c through
 * the REAL artifact fat12.c via the host file-backed blockdev, reads the whole
 * FAT, and asserts:
 *
 *   (a) Hand-computed / known-reserved entries: FAT[0] low byte == media 0xF0,
 *       and fat12_next_cluster for the worked-example clusters 4..7 yields the
 *       chain 5,6,7,EOC (brief Sec 3 worked example -- INDEPENDENT of any code
 *       under test: these are literal byte values verified by the orchestrator
 *       and re-verified with python3 against the minted image).
 *
 *   (b) CHAIN.TXT (1600 bytes => ceil(1600/512) = 4 clusters) walks as the
 *       contiguous ascending run 4,5,6,7 and terminates at EOC. This is
 *       independent and deterministic: chain length is fixed by the fixture
 *       size and the mformat allocation order (clusters 2,3 are HELLO/SECOND,
 *       so CHAIN starts at 4), re-verified with python3 in this session.
 *
 *   (c) RISK-1 boundary straddle: a hand-built scratch FAT with a known 12-bit
 *       value at cluster 341 (byte_offset 511, straddling the 512-byte FAT
 *       sector boundary) decodes correctly from the contiguous whole-FAT
 *       buffer -- the regression guard for the bug RISK-1 warns about.
 *
 *   (d) Anti-hang guard (Rule 2, non-negotiable): a hand-built cyclic FAT
 *       (4 -> 5 -> 4 -> ...) makes fat12_walk_chain RETURN AN ERROR rather
 *       than loop forever.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (decode rule +
 *   worked example) and RISK-1 (boundary straddle). Image path is argv[1]
 *   from the Makefile mint rule (no host path baked in -> Rule 11).
 *
 * ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"      /* seed/, on -Iseed           */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend       */

TEST_HARNESS();

int main(int argc, char **argv)
{
	const char     *img;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	int             rc;

	/* Whole-FAT buffer: 9 sectors * 512 = 4608 bytes for the 1.44 MB floppy
	 * (brief RISK-1). Size generously to the standard FAT12 floppy FAT. */
	uint8_t  fat_buf[9 * 512];
	uint16_t next;
	uint16_t chain[16];
	uint32_t count;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <fat12-image>\n", argv[0]);
		return 2;
	}
	img = argv[1];

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open should succeed on the minted image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_chain");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount should return FAT12_OK on the 1.44MB image");

	/* ---- Read the whole first FAT contiguously (brief RISK-1) ---- */
	rc = fat12_read_fat(&vol, fat_buf, sizeof(fat_buf));
	CHECK(rc == FAT12_OK, "fat12_read_fat should fill the whole-FAT buffer");

	/* fat12_read_fat must fail loud if the buffer is too small (Rule 2). */
	{
		uint8_t small[256];
		rc = fat12_read_fat(&vol, small, sizeof(small));
		CHECK(rc == FAT12_ERR_BUFFER,
		      "fat12_read_fat must return FAT12_ERR_BUFFER for a short buffer");
	}

	/* (a) Known-reserved entry: FAT[0] low byte is the media descriptor 0xF0
	 * (brief Sec 1/3). This is a raw byte of the FAT region, independent of
	 * the decoder under test. */
	CHECK(fat_buf[0] == 0xF0, "FAT[0] low byte == media descriptor 0xF0");

	/* (a) Worked-example clusters 4..7 (brief Sec 3): chain 4->5->6->7->EOC.
	 * Hand-computed expected next-cluster values, independent of code. */
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 4u, &next);
	CHECK(rc == FAT12_OK && next == 5u, "next(cluster 4) == 5 (even decode)");
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 5u, &next);
	CHECK(rc == FAT12_OK && next == 6u, "next(cluster 5) == 6 (odd decode)");
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 6u, &next);
	CHECK(rc == FAT12_OK && next == 7u, "next(cluster 6) == 7 (even decode)");
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 7u, &next);
	CHECK(rc == FAT12_OK && fat12_is_eoc(next),
	      "next(cluster 7) is EOC (>= 0xFF8) -- odd decode of F0 FF -> 0xFFF");

	/* fat12_next_cluster must reject reserved clusters 0/1 (Rule 2). */
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 0u, &next);
	CHECK(rc == FAT12_ERR_CLUSTER, "next(cluster 0) -> FAT12_ERR_CLUSTER (reserved)");
	rc = fat12_next_cluster(&vol, fat_buf, sizeof(fat_buf), 1u, &next);
	CHECK(rc == FAT12_ERR_CLUSTER, "next(cluster 1) -> FAT12_ERR_CLUSTER (reserved)");

	/* (b) Walk CHAIN.TXT's chain from start cluster 4: expect 4,5,6,7 then
	 * EOC. 1600 bytes / 512 = 4 clusters; start cluster fixed at 4 by the
	 * mint order (HELLO=2, SECOND=3, CHAIN=4) -- re-verified with python3. */
	rc = fat12_walk_chain(&vol, fat_buf, sizeof(fat_buf), 4u, chain,
	                      (uint32_t)(sizeof(chain) / sizeof(chain[0])), &count);
	CHECK(rc == FAT12_OK, "fat12_walk_chain(start=4) should succeed (no hang)");
	CHECK(count == 4u, "CHAIN.TXT spans 4 clusters (1600 bytes / 512, ceil)");
	CHECK(count == 4u && chain[0] == 4u && chain[1] == 5u &&
	      chain[2] == 6u && chain[3] == 7u,
	      "CHAIN.TXT chain is the ascending run 4,5,6,7");

	/* fat12_walk_chain must reject a reserved start cluster (Rule 2). */
	rc = fat12_walk_chain(&vol, fat_buf, sizeof(fat_buf), 1u, chain,
	                      (uint32_t)(sizeof(chain) / sizeof(chain[0])), &count);
	CHECK(rc == FAT12_ERR_CLUSTER, "walk_chain(start=1) -> FAT12_ERR_CLUSTER");

	/* ---- (c) RISK-1: boundary-straddle decode (cluster 341) ---- *
	 * Cluster 341 is odd; byte_offset = (341*3)/2 = 511, so its two bytes are
	 * fat[511] and fat[512] -- straddling the 512-byte FAT sector boundary.
	 * Plant a known 12-bit value (0xABC) there and assert the contiguous
	 * whole-FAT buffer decodes it correctly across the boundary. Odd decode:
	 *   entry = (b[511] >> 4) | (b[512] << 4)  (masked 12 bits)
	 * For entry 0xABC: low nibble C goes to b[511] high nibble; 0xAB goes to
	 * b[512]. So b[511] = 0xC0 | (existing low nibble), b[512] = 0xAB.
	 * Use a fresh scratch FAT so we do not disturb the minted one. */
	{
		uint8_t  scratch[9 * 512];
		uint16_t got;

		memset(scratch, 0, sizeof(scratch));
		scratch[511] = 0xC0; /* high nibble C = low 4 bits of 0xABC          */
		scratch[512] = 0xAB; /* top 8 bits of 0xABC                          */

		rc = fat12_next_cluster(&vol, scratch, sizeof(scratch), 341u, &got);
		CHECK(rc == FAT12_OK && got == 0xABCu,
		      "RISK-1: cluster 341 decodes 0xABC across the 512-byte boundary");
	}

	/* ---- (d) Anti-hang guard: a cyclic FAT must error, not loop ---- *
	 * Build a tiny synthetic FAT by hand: cluster 4 -> 5, cluster 5 -> 4.
	 * Walking from 4 cycles forever unless the guard bites. Lay the bytes
	 * with the even/odd packing rule:
	 *   cluster 4 (even): off=6  -> entry 5
	 *   cluster 5 (odd):  off=7  -> entry 4
	 * even off6: b[6]=0x05, b[7] low nibble=0; odd off7: entry4 ->
	 *   b[7] high nibble = (4 & 0xF) = 4 -> b[7]=0x40; b[8] = (4>>4)=0x00.
	 * So b[6]=0x05, b[7]=0x40, b[8]=0x00 encodes 4->5 and 5->4. */
	{
		uint8_t  cyc[9 * 512];

		memset(cyc, 0, sizeof(cyc));
		cyc[6] = 0x05; /* cluster 4 (even) -> 5  */
		cyc[7] = 0x40; /* cluster 5 (odd)  -> 4  */
		cyc[8] = 0x00;

		/* Sanity: the hand-laid bytes really encode the cycle. */
		rc = fat12_next_cluster(&vol, cyc, sizeof(cyc), 4u, &next);
		CHECK(rc == FAT12_OK && next == 5u, "synthetic cyclic FAT: next(4) == 5");
		rc = fat12_next_cluster(&vol, cyc, sizeof(cyc), 5u, &next);
		CHECK(rc == FAT12_OK && next == 4u, "synthetic cyclic FAT: next(5) == 4");

		/* The load-bearing assertion: walking the cycle must terminate with
		 * an error (FAT12_ERR_CHAIN), bounded by max_clusters -- NOT hang. */
		rc = fat12_walk_chain(&vol, cyc, sizeof(cyc), 4u, chain,
		                      (uint32_t)(sizeof(chain) / sizeof(chain[0])),
		                      &count);
		CHECK(rc == FAT12_ERR_CHAIN,
		      "anti-hang: cyclic chain returns FAT12_ERR_CHAIN, does not loop");
	}

	/* (d') A free cluster mid-chain is corruption -> error (Rule 2). Build
	 * cluster 4 -> 5, cluster 5 -> 0 (free). */
	{
		uint8_t free_mid[9 * 512];

		memset(free_mid, 0, sizeof(free_mid));
		free_mid[6] = 0x05; /* cluster 4 -> 5            */
		/* cluster 5 (odd) -> 0: high nibble of b[7]=0, b[8]=0 -> already 0. */

		rc = fat12_walk_chain(&vol, free_mid, sizeof(free_mid), 4u, chain,
		                      (uint32_t)(sizeof(chain) / sizeof(chain[0])),
		                      &count);
		CHECK(rc == FAT12_ERR_CHAIN,
		      "corruption: a free cluster mid-chain returns FAT12_ERR_CHAIN");
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_chain");
}
