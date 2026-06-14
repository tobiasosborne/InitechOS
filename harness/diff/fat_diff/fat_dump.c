/*
 * harness/diff/fat_diff/fat_dump.c -- FAT12 manifest/content dumper (FACTORY).
 *
 * FACTORY host tool (CLAUDE.md Law 3): hosted, libc OK; NOT shipped in the
 * artifact. It mounts a disk image with the REAL artifact reader
 * (os/milton/fat12.c) via the host file-backed blockdev and emits a
 * DETERMINISTIC, normalized, machine-diffable view of what our reader sees --
 * this is "the artifact-under-test's output" that the differential oracle
 * (`make test-fat`, beads initech-5cu) diffs against two independent
 * references: mtools (mdir/mcopy) and an independent python3 reader
 * (fat12_ref.py).
 *
 * Two modes (both stdout):
 *   --list           : one "NAME.EXT <size>" line per REGULAR file, sorted
 *                      ascending by name. Volume-label / directory / LFN
 *                      entries are excluded -- only regular files, so the
 *                      manifest matches `mcopy`-extractable content. Timestamps
 *                      and the volume serial are NEVER printed (normalized away
 *                      per docs/research/fat12-ground-truth.md Sec 5).
 *   --cat NAME.EXT   : write the named file's EXACT content bytes to stdout
 *                      (binary-safe). file_size is authoritative (RISK-5).
 *   --list-path PATH : like --list, but for the directory at a backslash-
 *                      separated PATH (a subdirectory; "\\" / "" is the root).
 *                      Resolves the path with the artifact fat12_resolve_path
 *                      and walks the subdir chain with fat12_read_dir (beads
 *                      initech-ti8). Fail loud on a missing / non-dir component.
 *
 * Fail loud (CLAUDE.md Rule 2): any mount/read/find error prints a diagnostic
 * to stderr and exits NON-ZERO. A silently-empty or partial dump is the worst
 * outcome for an oracle (Law 2), so every failure is loud.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 (dir read semantics),
 *   Sec 5 (reference commands + normalization). os/milton/fat12.h (reader API).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked into output (Rule 11).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fat12.h"          /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"  /* fat_diff host backend       */

/* A 1.44 MB volume holds at most 224 root entries; cap the collected list at
 * that. Each name is at most 12 chars + NUL (FAT12_NAME83_MAX). */
#define MAX_FILES 224

typedef struct file_rec {
	char     name[FAT12_NAME83_MAX];
	uint32_t size;
} file_rec_t;

typedef struct collector {
	file_rec_t files[MAX_FILES];
	int        count;
	int        overflow; /* set if more than MAX_FILES regular files seen */
} collector_t;

/* Collect REGULAR files only: skip volume-label and directory entries (LFN and
 * deleted are already filtered by fat12_read_root_dir). This mirrors the set
 * `mcopy` can extract -- so the --list manifest is content-comparable. */
static int collect_cb(const dir_entry_t *e, void *user)
{
	collector_t *c = (collector_t *)user;

	if ((e->attribute & DIR_ATTR_VOLLABEL) != 0u) {
		return 0; /* volume label -- not a file */
	}
	if ((e->attribute & DIR_ATTR_DIRECTORY) != 0u) {
		return 0; /* subdirectory -- not a regular file */
	}
	if (c->count >= MAX_FILES) {
		c->overflow = 1;
		return 1; /* stop: refuse to silently drop entries (Rule 2) */
	}
	fat12_format_83(e, c->files[c->count].name);
	c->files[c->count].size = e->file_size;
	c->count++;
	return 0;
}

/* qsort comparator: ascending ASCII by formatted 8.3 name (deterministic
 * ordering, Rule 11). Names are unique in a flat root dir. */
static int rec_cmp(const void *a, const void *b)
{
	const file_rec_t *ra = (const file_rec_t *)a;
	const file_rec_t *rb = (const file_rec_t *)b;
	return strcmp(ra->name, rb->name);
}

static void usage(const char *argv0)
{
	fprintf(stderr,
	        "usage: %s <image> --list\n"
	        "       %s <image> --cat NAME.EXT\n"
	        "       %s <image> --list-path PATH\n",
	        argv0, argv0, argv0);
}

/* Mount `img` into *vol via *bf. Returns 0 on success; prints a loud
 * diagnostic and returns non-zero on any failure (Rule 2). sector_buf is the
 * caller's >=512-byte scratch. */
static int mount_image(const char *img, blockdev_file_t *bf,
                       fat12_volume_t *vol, uint8_t *sector_buf)
{
	int rc;

	if (blockdev_file_open(bf, img) != 0) {
		fprintf(stderr, "fat_dump: cannot open image '%s'\n", img);
		return 1;
	}
	rc = fat12_mount(vol, &bf->dev, sector_buf);
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_mount('%s') failed: rc=%d\n", img, rc);
		blockdev_file_close(bf);
		return 1;
	}
	return 0;
}

static int do_list(const char *img)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	collector_t     c;
	int             i;
	int             rc;

	if (mount_image(img, &bf, &vol, sector_buf) != 0) {
		return 1;
	}

	memset(&c, 0, sizeof(c));
	rc = fat12_read_root_dir(&vol, sector_buf, collect_cb, &c);
	/* collect_cb only returns non-zero on overflow; that is an error, not a
	 * clean early stop. A negative rc is a read error. Either way: fail loud. */
	if (rc < 0) {
		fprintf(stderr, "fat_dump: fat12_read_root_dir failed: rc=%d\n", rc);
		blockdev_file_close(&bf);
		return 1;
	}
	if (c.overflow) {
		fprintf(stderr, "fat_dump: too many root entries (> %d)\n", MAX_FILES);
		blockdev_file_close(&bf);
		return 1;
	}

	/* Deterministic ordering: sort ascending by name (Rule 11). */
	qsort(c.files, (size_t)c.count, sizeof(c.files[0]), rec_cmp);

	for (i = 0; i < c.count; i++) {
		/* "NAME.EXT <size>" -- names/sizes only; timestamps and the volume
		 * serial are normalized away (never printed). */
		printf("%s %u\n", c.files[i].name, c.files[i].size);
	}

	blockdev_file_close(&bf);
	return 0;
}

static int do_cat(const char *img, const char *name)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	uint8_t         fat_buf[9 * 512];     /* whole FAT for 1.44 MB (9 sectors) */
	uint8_t         cluster_buf[512];     /* sectors_per_cluster(1) * 512       */
	static uint8_t  out_buf[2 * 1024 * 1024]; /* up to a full floppy of data    */
	dir_entry_t     e;
	uint32_t        out_bytes = 0u;
	int             rc;
	size_t          w;

	if (mount_image(img, &bf, &vol, sector_buf) != 0) {
		return 1;
	}

	rc = fat12_read_fat(&vol, fat_buf, (uint32_t)sizeof(fat_buf));
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_read_fat failed: rc=%d\n", rc);
		blockdev_file_close(&bf);
		return 1;
	}

	rc = fat12_find(&vol, sector_buf, name, &e);
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_find('%s') failed: rc=%d\n", name, rc);
		blockdev_file_close(&bf);
		return 1;
	}

	rc = fat12_read_file(&vol, fat_buf, (uint32_t)sizeof(fat_buf), &e,
	                     out_buf, (uint32_t)sizeof(out_buf),
	                     cluster_buf, &out_bytes);
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_read_file('%s') failed: rc=%d\n",
		        name, rc);
		blockdev_file_close(&bf);
		return 1;
	}

	/* Write exactly out_bytes (== file_size) raw bytes to stdout. */
	w = fwrite(out_buf, 1u, (size_t)out_bytes, stdout);
	if (w != (size_t)out_bytes) {
		fprintf(stderr, "fat_dump: short write to stdout (%zu of %u)\n",
		        w, out_bytes);
		blockdev_file_close(&bf);
		return 1;
	}

	blockdev_file_close(&bf);
	return 0;
}

/* List the REGULAR files of the directory at `path` (a backslash-separated
 * subdirectory path; "\\" / "" is the root), using the REAL artifact
 * fat12_resolve_path + fat12_read_dir (beads initech-ti8). Same normalized
 * "NAME.EXT <size>" sorted output as do_list; '.'/'..'/subdir entries are
 * skipped by collect_cb. Fail loud on a missing/non-dir component (Rule 2). */
static int do_list_path(const char *img, const char *path)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	uint8_t         fat_buf[9 * 512];   /* whole FAT for 1.44 MB (9 sectors) */
	fat12_dir_t     dir;
	dir_entry_t     e;
	collector_t     c;
	int             i;
	int             rc;

	if (mount_image(img, &bf, &vol, sector_buf) != 0) {
		return 1;
	}

	rc = fat12_read_fat(&vol, fat_buf, (uint32_t)sizeof(fat_buf));
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_read_fat failed: rc=%d\n", rc);
		blockdev_file_close(&bf);
		return 1;
	}

	rc = fat12_resolve_path(&vol, sector_buf, fat_buf,
	                        (uint32_t)sizeof(fat_buf), path, &dir, &e);
	if (rc != FAT12_OK) {
		fprintf(stderr, "fat_dump: fat12_resolve_path('%s') failed: rc=%d\n",
		        path, rc);
		blockdev_file_close(&bf);
		return 1;
	}
	/* The resolved entry must be a directory (the path must name a dir). */
	if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u) {
		fprintf(stderr, "fat_dump: '%s' is not a directory\n", path);
		blockdev_file_close(&bf);
		return 1;
	}
	/* Descend into it: out_dir is the CONTAINING dir; build the cursor for the
	 * resolved directory itself from the entry's start_cluster (0 => root). */
	dir.start_cluster = e.start_cluster;
	dir.is_root       = (e.start_cluster == 0u) ? 1 : 0;

	memset(&c, 0, sizeof(c));
	rc = fat12_read_dir(&vol, &dir, sector_buf, fat_buf,
	                    (uint32_t)sizeof(fat_buf), collect_cb, &c);
	if (rc < 0) {
		fprintf(stderr, "fat_dump: fat12_read_dir('%s') failed: rc=%d\n",
		        path, rc);
		blockdev_file_close(&bf);
		return 1;
	}
	if (c.overflow) {
		fprintf(stderr, "fat_dump: too many entries (> %d)\n", MAX_FILES);
		blockdev_file_close(&bf);
		return 1;
	}

	qsort(c.files, (size_t)c.count, sizeof(c.files[0]), rec_cmp);
	for (i = 0; i < c.count; i++) {
		printf("%s %u\n", c.files[i].name, c.files[i].size);
	}

	blockdev_file_close(&bf);
	return 0;
}

int main(int argc, char **argv)
{
	const char *img;
	const char *mode;

	if (argc < 3) {
		usage(argv[0]);
		return 2;
	}
	img  = argv[1];
	mode = argv[2];

	if (strcmp(mode, "--list") == 0) {
		if (argc != 3) {
			usage(argv[0]);
			return 2;
		}
		return do_list(img);
	}
	if (strcmp(mode, "--cat") == 0) {
		if (argc != 4) {
			usage(argv[0]);
			return 2;
		}
		return do_cat(img, argv[3]);
	}
	if (strcmp(mode, "--list-path") == 0) {
		if (argc != 4) {
			usage(argv[0]);
			return 2;
		}
		return do_list_path(img, argv[3]);
	}

	usage(argv[0]);
	return 2;
}
