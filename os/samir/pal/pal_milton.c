/*
 * os/samir/pal/pal_milton.c -- SAMIR PAL InitechDOS binding (INT 21h + arena).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib).
 * This is the SOLE translation unit that issues `int 0x21` in the entire SAMIR
 * codebase. Engine code (core/, fs/, cmd/, ui/) touches the OS ONLY through
 * the samir_pal vtable (pal.h). No libc. Depends ONLY on <stdint.h>.
 *
 * MANDATE: ADR-0009 DEC-06 ("pal_milton.c is the sole int 0x21 site; S8.2
 * needs no console primitives"). Implements every slot of the 15-slot vtable
 * declared in os/samir/include/samir/pal.h.
 *
 * CALLING CONVENTION (DEC-04a flat, spec/int21h_calling_convention.json):
 *   - AH (bits 15:8 of EAX) selects the function.
 *   - Primary pointer in EDX (flat 32-bit linear address).
 *   - Byte count in ECX; handle in EBX.
 *   - Return value in EAX (AX for 16-bit legacy values).
 *   - CF=1 in saved EFLAGS on error; AX=error code. CF=0 on success.
 *   - All pointers are FLAT 32-bit linear addresses -- no segment:offset.
 *
 * INT 21h REGISTER ABI per slot (citing int21.c + spec/int21h_calling_convention.json):
 *   AH=3Dh OPEN:    AL=access(0=r,1=w,2=rw), EDX=path   -> AX=handle or CF+AX=err
 *   AH=3Ch CREAT:   EDX=path, CX=0            -> AX=handle or CF+AX=err
 *   AH=3Eh CLOSE:   EBX=handle                -> CF or CF+AX=err
 *   AH=3Fh READ:    EBX=handle,ECX=n,EDX=buf  -> AX=bytes_read or CF+AX=err
 *   AH=40h WRITE:   EBX=handle,ECX=n,EDX=buf  -> AX=bytes_written or CF+AX=err
 *   AH=41h UNLINK:  EDX=path                  -> CF or CF+AX=err
 *   AH=42h LSEEK:   AL=whence,EBX=handle,     -> AX=low16_of_pos or CF+AX=err
 *                   ECX=high16,EDX=low32_off
 *   AH=56h RENAME:  EDX=old_path,EDI=new_path -> CF or CF+AX=err
 *   AH=07h CONIN_RAW: (no input)              -> AL=char
 *   AH=2Ah GETDATE: (no input)                -> CX=year,DH=month,DL=day
 *   AH=48h ALLOC:   BX=paragraphs             -> AX=segment or CF+AX=err+BX=largest
 *
 * DOS error code -> PAL_* mapping (mirrors pal_host.c errno_to_pal):
 *   0x02 FILE_NOT_FOUND  -> PAL_ENOENT (2)
 *   0x03 PATH_NOT_FOUND  -> PAL_ENOENT (2)
 *   0x05 ACCESS_DENIED   -> PAL_EACCES (5)
 *   0x06 INVALID_HANDLE  -> PAL_EACCES (5)
 *   0x1C DISK_FULL (28d) -> PAL_ENOSPC (28)
 *   all others           -> PAL_EIO    (29)
 *
 * LSEEK NOTE: The flat ABI (int21.c do_lseek) maps EBX=handle, AL=whence,
 * EDX=low-32-bit offset (ECX=high16 is 0 this milestone), and returns the
 * new position in EAX (full 32-bit). The return in AX is therefore the low
 * 16 bits; we compose the full 32-bit position from EAX directly.
 *
 * RENAME NOTE: AH=56h takes the OLD path in EDX and the NEW path in EDI
 * (spec/int21h_calling_convention.json AH=56h -- the flat-kernel adaptation
 * of the real-DOS ES:DI convention; int21.c do_rename reads f->edi).
 *
 * ARENA: pal_milton requests one block via AH=48h at construction (see
 * PAL_MILTON_HEAP_PARAS below). The returned fake-segment is converted to a
 * flat linear base (seg << 4). The bump pointer advances on every alloc;
 * reset(mark) rewinds to a previously returned alloc pointer or to base.
 *
 * CON READ (conin_line): AH=3Fh on handle 0 delivers a cooked line including
 * a trailing CR+LF (int21.c do_read CON branch; Microsoft KB Q113058). We
 * strip the trailing CR and LF (and any leading/embedded CR as insurance)
 * before returning to match pal_host's fgets-strip semantics. Return value
 * is the line length WITHOUT the newline, as required by the pal.h contract.
 *
 * gotoxy / set_attr: NO-OP stubs. These are console primitives, NOT INT 21h,
 * and are exercised only at S8.4 (@SAY/GET/READ forms) which is gated on
 * S8.3/M4. ADR-0009 DEC-06 explicitly allows no-ops at S8.2.
 *
 * SOFT-FLOAT: this TU is compiled with -msoft-float -mno-80387 so no x87
 * instructions are emitted. The engine uses libgcc soft-float helpers
 * (__adddf3 etc.) per ADR-0009 DEC-01. No FPU init needed in the kernel.
 *
 * Ref (Law 1):
 *   ADR-0009 DEC-06 (pal_milton.c mandate; slot->INT21h map; gotoxy/set_attr
 *     no-op; arena via AH=48h; conin_line strips CR/LF; conin_char=07h).
 *   ADR-0009 DEC-01 (soft-float; no kernel FPU init).
 *   ADR-0009 DEC-03 (FLOW_MAX_REGISTRY=1; flat .COM in conventional memory).
 *   ADR-0009 DEC-04 (AH=48h arena MUST be disjoint from program; the kernel
 *     fix in bead 1q4u ensures the block returned by 48h is above the image).
 *   os/samir/include/samir/pal.h (the full 15-slot vtable contract).
 *   spec/int21h_calling_convention.json (flat ABI; CF error; segment convention).
 *   os/milton/int21.c (confirmed register behavior for each handler called).
 *   spec/memory_map.h (PROGRAM_BASE, PROGRAM_STACK_BOT -- arena ceiling).
 *   os/samir/pal/pal_host.c (mirror structure: state-first, state_of cast,
 *     fmode logic, arena bump pattern).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Fail loud (Rule 2): every error is negative, never a silent no-op.
 */

#include <stdint.h>

#include "samir/pal.h"
#include "samir/interp.h"

/* ---- Arena sizing constants ----------------------------------------------- *
 *
 * The SAMIR engine at FLOW_MAX_REGISTRY=1 (ADR-0009 DEC-03) has:
 *   ~81 KiB text + ~26 KiB BSS (genuine footprint; ADR-0009 Sec 2 measurement).
 * The interpreter's internal arenas (interp.h tunables) + workarea structures +
 * REPL scratch need additional runtime memory. We request a generous but honest
 * 32 KiB heap: enough for the evaluator scratch arenas, record buffers (32
 * fields * ~256 bytes), and the work-area environment, with headroom. This is
 * well inside the available window: the MCB arena covers image+BSS..
 * PROGRAM_STACK_BOT which is > 100 KiB after the image.
 *
 * 32 KiB = 2048 paragraphs (1 paragraph = 16 bytes). The constant is documented
 * as a deliberate locked decision here (Rule 8 -- "a deliberate act with a
 * worklog note"). Increasing it requires verifying it still fits below
 * PROGRAM_STACK_BOT = 0x70000 (spec/memory_map.h); the S8.2 oracle forces a
 * real AH=48h call so a mis-sizing fails at the oracle, not in production
 * (ADR-0009 DEC-04 + DEC-08).
 *
 * Ref: ADR-0009 DEC-03 (footprint); spec/memory_map.h PROGRAM_STACK_BOT;
 *      os/samir/include/samir/interp.h (interp arena tunables).
 */
#define PAL_MILTON_HEAP_PARAS  2048u    /* 32 KiB = 2048 * 16 bytes */
#define PAL_MILTON_HEAP_BYTES  (PAL_MILTON_HEAP_PARAS * 16u)

/* ---- Freestanding panic --------------------------------------------------- *
 * Rule 2 (fail loud). Emit nothing (no sink available here), then halt via the
 * inline asm cli+hlt sequence. The kernel will display PC LOAD LETTER on the
 * next interrupt (PRD Appendix B). This is intentionally minimal: the panic
 * path in pal_milton is only reachable if AH=48h fails at construction (an
 * OOM condition that cannot be recovered from) or if reset() receives a mark
 * outside the arena (a corrupted pointer -- also unrecoverable).
 */
static void __attribute__((noreturn)) milton_panic(void)
{
    __asm__ __volatile__(
        "cli\n\t"
        "hlt\n\t"
        ".1: jmp .1\n\t"
        ::: "memory"
    );
    __builtin_unreachable();
}

/* ---- DOS error -> PAL error mapping --------------------------------------- *
 * Converts a DOS INT 21h error code (returned in AX when CF=1) to a PAL
 * error code. Mirrors pal_host.c errno_to_pal structure, with DOS codes.
 * The raw AX value is NEVER returned through the contract (pal.h principle).
 * Source: DOS 3.3 PRM error code table; int21.c INT21_ERR_* constants.
 */
static pal_err dos_err_to_pal(uint32_t dos_ax)
{
    uint16_t e = (uint16_t)(dos_ax & 0xFFFFu);
    switch (e) {
    case 0x0002u:   /* FILE_NOT_FOUND */
    case 0x0003u:   /* PATH_NOT_FOUND */
        return PAL_ENOENT;
    case 0x0005u:   /* ACCESS_DENIED */
    case 0x0006u:   /* INVALID_HANDLE */
    case 0x0004u:   /* TOO_MANY_OPEN */
        return PAL_EACCES;
    case 0x001Cu:   /* DISK_FULL (28 decimal) -- */
        return PAL_ENOSPC;
    default:
        return PAL_EIO;
    }
}

/* ---- Milton PAL state ----------------------------------------------------- */

typedef struct {
    samir_pal_t  vtable;    /* MUST be first: engine casts p->vtable (pal_host pattern) */

    /* Bump arena: one AH=48h block obtained at construction.
     * heap_base = (returned_segment << 4) as a flat linear address.
     * heap_ptr  = next free byte (bumps forward on alloc).
     * heap_end  = heap_base + PAL_MILTON_HEAP_BYTES. */
    uint8_t     *heap_base;
    uint8_t     *heap_ptr;
    uint8_t     *heap_end;
} pal_milton_state_t;

/* Recover the state block from the vtable pointer (vtable is first). */
static pal_milton_state_t *state_of(samir_pal_t *p)
{
    return (pal_milton_state_t *)p;
}

/* ---- Inline-asm INT 21h wrappers ----------------------------------------- *
 *
 * All wrappers use GCC extended inline asm with __volatile__ to prevent the
 * compiler from reordering or eliding the syscall. The `int $0x21` opcode is
 * the 32-bit TRAP gate installed by the InitechDOS IDT at vector 0x21
 * (spec/int21h_calling_convention.json "gate" section; DEC-04a ratification).
 *
 * Clobber discipline: EAX (return), EFLAGS (CF). Caller-save registers that
 * the kernel dispatcher may modify are listed explicitly. We declare "cc" to
 * tell the compiler EFLAGS is clobbered.
 *
 * SEGMENT REGISTERS: in the flat protected model the dispatcher restores CS/DS
 * automatically via the TRAP gate + IRETD; ES/FS/GS are not touched by the
 * handlers, so we do not clobber them. The convention document confirms "All
 * pointer arguments are FLAT 32-bit LINEAR addresses -- no segment:offset."
 */

/* Execute INT 21h with AH=func (bits 15:8), AL=al_val (bits 7:0).
 * EDX = pointer/data, EBX = handle/BX-value, ECX = count/CX-value.
 * EDI = optional second pointer (AH=56h RENAME only; others pass 0 and it
 * is harmless since the dispatcher ignores it for non-rename AHs).
 * Returns the full 32-bit EAX and sets *cf_out to 1 if CF was set, 0 otherwise.
 *
 * Ref: spec/int21h_calling_convention.json abi section; os/milton/int21.c. */
static uint32_t int21(uint8_t func, uint8_t al_val,
                      uint32_t edx_val, uint32_t ebx_val,
                      uint32_t ecx_val, uint32_t edi_val,
                      int *cf_out)
{
    uint32_t eax_in  = ((uint32_t)func << 8) | (uint32_t)al_val;
    uint32_t eax_out = 0u;
    uint32_t flags   = 0u;

    __asm__ __volatile__(
        "int $0x21\n\t"
        "pushfl\n\t"
        "popl %1\n\t"
        : "=a"(eax_out), "=r"(flags)
        : "a"(eax_in),
          "d"(edx_val),
          "b"(ebx_val),
          "c"(ecx_val),
          "D"(edi_val)
        : "cc", "memory"
    );

    if (cf_out) {
        /* CF = bit 0 of EFLAGS (Intel SDM Vol 1 Sec 3.4.3; CF_BIT in int21.c). */
        *cf_out = (int)(flags & 0x1u);
    }
    return eax_out;
}

/* Variant that also returns ECX and EDX after the call (needed for AH=42h
 * LSEEK, which returns the full 32-bit offset in EAX; and AH=2Ah GET DATE
 * which returns CX=year, DH=month, DL=day). */
static uint32_t int21_cxdx(uint8_t func, uint8_t al_val,
                            uint32_t edx_val, uint32_t ebx_val,
                            uint32_t ecx_val,
                            uint32_t *ecx_out, uint32_t *edx_out,
                            int *cf_out)
{
    uint32_t eax_in  = ((uint32_t)func << 8) | (uint32_t)al_val;
    uint32_t eax_out = 0u;
    uint32_t ecx_r   = 0u;
    uint32_t edx_r   = 0u;
    uint32_t flags   = 0u;

    __asm__ __volatile__(
        "int $0x21\n\t"
        "pushfl\n\t"
        "popl %3\n\t"
        : "=a"(eax_out), "=c"(ecx_r), "=d"(edx_r), "=r"(flags)
        : "a"(eax_in),
          "d"(edx_val),
          "b"(ebx_val),
          "c"(ecx_val)
        : "cc", "memory"
    );

    if (cf_out)  *cf_out  = (int)(flags & 0x1u);
    if (ecx_out) *ecx_out = ecx_r;
    if (edx_out) *edx_out = edx_r;
    return eax_out;
}

/* ---- File I/O slots ------------------------------------------------------- */

/*
 * milton_open: map PAL mode flags onto INT 21h AH=3Dh (open existing) or
 * AH=3Ch (create/truncate).
 *
 *   PAL_CREATE or PAL_TRUNC set -> AH=3Ch CREAT (create/truncate to zero).
 *   Otherwise -> AH=3Dh OPEN with AL=access (PAL_RD=0/PAL_WR=1/PAL_RDWR=2).
 *
 * Ref: pal.h open slot contract; ADR-0009 DEC-06;
 *      spec/int21h_calling_convention.json (AH=3Dh AL=access; AH=3Ch EDX=path).
 *      os/milton/int21.c do_open/do_creat.
 */
static pal_fd milton_open(samir_pal_t *p, const char *name, int mode)
{
    int create  = (mode & PAL_CREATE) != 0;
    int trunc   = (mode & PAL_TRUNC)  != 0;
    int access  = mode & 3;  /* PAL_RD=0, PAL_WR=1, PAL_RDWR=2 */
    int cf = 0;
    uint32_t result;
    (void)p;

    if (create || trunc) {
        /* AH=3Ch CREAT: EDX=path, CX=attribute (0 = normal).
         * AL is ignored by do_creat (CX carries the attribute, not AL access). */
        result = int21(0x3Cu, 0x00u,
                       (uint32_t)(uintptr_t)name,
                       0u,       /* EBX unused */
                       0u,       /* CX=attr=0 (normal file) */
                       0u, &cf);
    } else {
        /* AH=3Dh OPEN: AL=access(0/1/2), EDX=path.
         * Ref: do_open reads frame_al(f) for the mode byte. */
        result = int21(0x3Du, (uint8_t)access,
                       (uint32_t)(uintptr_t)name,
                       0u, 0u, 0u, &cf);
    }

    if (cf) {
        return -(pal_fd)dos_err_to_pal(result);
    }
    /* AX = handle (low 16 bits; do_open sets EAX low 16).
     * Valid handles are JFT indices (0-based, small non-negative numbers). */
    return (pal_fd)(result & 0xFFFFu);
}

/*
 * milton_close: AH=3Eh CLOSE, EBX=handle.
 * Ref: pal.h close slot; do_close checks handle in EBX; CF set on bad handle.
 */
static int milton_close(samir_pal_t *p, pal_fd fd)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    if (fd < 0) {
        return -(int)PAL_EACCES;
    }
    result = int21(0x3Eu, 0x00u,
                   0u,               /* EDX unused */
                   (uint32_t)fd,     /* EBX=handle */
                   0u, 0u, &cf);
    if (cf) {
        return -(int)dos_err_to_pal(result);
    }
    return 0;
}

/*
 * milton_read: AH=3Fh READ, EBX=handle, ECX=count, EDX=buf.
 * Returns the byte count ACTUALLY read (>= 0; 0 = EOF) or -(pal_err).
 * NEVER synthesizes a full read on short -- report what the kernel gave
 * (Rule 2: do NOT report a short read as a full one; pal.h read contract).
 * Ref: do_read returns EAX=bytes_read or CF+AX=err.
 *
 * Mutation hook (Rule 6): -DPAL_MILTON_MUTATE_SHORT_READ shaves ONE byte off
 * every multi-byte read request (ECX = n-1). The .dbf header/record reads then
 * come up one byte short, so the on-target dbf codec mis-decodes: the LIST rows
 * the S8.2 emu gate (bead hdlb) asserts garble (or the open fails) -- the gate's
 * serial + screendump assertions go RED for the RIGHT reason (wrong data on the
 * on-target I/O path), not a crash. NEVER define in a real build.
 */
static int32_t milton_read(samir_pal_t *p, pal_fd fd, void *buf, uint32_t n)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    if (fd < 0) {
        return -(int32_t)PAL_EACCES;
    }
    if (n == 0u) {
        return 0;
    }
#ifdef PAL_MILTON_MUTATE_SHORT_READ
    /* MUTANT: request one byte fewer so reads come up short (>1 only, so a
     * single-byte read still moves). The dbf codec then mis-frames records. */
    if (n > 1u) {
        n -= 1u;
    }
#endif
    result = int21(0x3Fu, 0x00u,
                   (uint32_t)(uintptr_t)buf,   /* EDX=buffer flat ptr */
                   (uint32_t)fd,                /* EBX=handle */
                   n,                           /* ECX=count */
                   0u, &cf);
    if (cf) {
        return -(int32_t)dos_err_to_pal(result);
    }
    /* AX = bytes read. 0 = EOF (clean, not an error). */
    return (int32_t)(result & 0xFFFFu);
}

/*
 * milton_write: AH=40h WRITE, EBX=handle, ECX=count, EDX=buf.
 * Returns bytes actually written (>= 0) or -(pal_err).
 * A short write (< n) means the device is full; engine treats < n as PAL_ENOSPC
 * per the pal.h contract. We return the actual count, not an error.
 * Ref: do_write returns EAX=bytes_written or CF+AX=err.
 *
 * Mutation hook (Rule 6): -DPAL_MILTON_MUTATE_DROP_WRITE makes a WRITE to a real
 * FILE handle (fd >= 3 -- handles 0/1/2 are CON/stdout/stderr) a NO-OP that
 * REPORTS full success (return n) WITHOUT issuing the INT 21h WRITE. The engine
 * then believes dbf_flush wrote the mutated image, but NOTHING reaches the FAT
 * volume: the on-disk .dbf stays at the original 3 records with the original
 * BALs, so the post-run dbf_ref.py persistence assertion (4 records, BAL=9999.99,
 * BAL=5555.55) goes RED -- the .dbf is stale, not garbled, and there is NO crash
 * (a clean data-not-persisted RED for the right reason, test-samir-write-mutant).
 * Console writes (fd <= 2) are LEFT INTACT so the dot prompt + the in-emu LIST
 * still render -- the bite is the on-disk persistence, not the live session.
 * NEVER define in a real build.
 */
static int32_t milton_write(samir_pal_t *p, pal_fd fd, const void *buf, uint32_t n)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    if (fd < 0) {
        return -(int32_t)PAL_EACCES;
    }
    if (n == 0u) {
        return 0;
    }
#ifdef PAL_MILTON_MUTATE_DROP_WRITE
    /* MUTANT: drop the WRITE to a real file (fd >= 3) but claim n bytes written,
     * so the .dbf flush never hits the disk. CON handles (0/1/2) pass through so
     * the session still renders. */
    if (fd >= 3) {
        return (int32_t)n;
    }
#endif
    result = int21(0x40u, 0x00u,
                   (uint32_t)(uintptr_t)buf,   /* EDX=buffer flat ptr */
                   (uint32_t)fd,                /* EBX=handle */
                   n,                           /* ECX=count */
                   0u, &cf);
    if (cf) {
        return -(int32_t)dos_err_to_pal(result);
    }
    return (int32_t)(result & 0xFFFFu);
}

/*
 * milton_seek: AH=42h LSEEK, AL=whence, EBX=handle, ECX:EDX=offset.
 *
 * The flat ABI carries the offset in EDX (low 32 bits; int21.c do_lseek reads
 * f->edx as the full 32-bit offset, ECX high 16 is 0 this milestone). Whence
 * maps PAL_SEEK_SET/CUR/END -> AL=0/1/2, matching the pal.h enum values
 * exactly (both enums start at 0 and are in the same order). Returns the new
 * absolute position (full 32 bits from EAX) or -(pal_err).
 *
 * The file-SIZE idiom is seek(fd, 0, PAL_SEEK_END) -> returns file length
 * (pal.h PAL_SEEK_END comment; ADR-0008 DEC-02 rev; do_lseek base=file_size
 * when whence==2).
 *
 * AMBIGUITY NOTE: The calling convention JSON (AH=42h not yet listed as a
 * separate entry) is inferred from the int21.c source which shows:
 *   "whence = frame_al(f); off = f->edx; f->eax = base + off"
 * This means the full 32-bit result lands in EAX. We read the full EAX.
 * The JSON "return_value: EAX (AL for single-byte; AX for 16-bit legacy;
 * EAX for 32-bit)" confirms EAX for 32-bit returns. Choice: read full EAX.
 *
 * Ref: os/milton/int21.c do_lseek; spec/int21h_calling_convention.json abi.
 */
static int32_t milton_seek(samir_pal_t *p, pal_fd fd, int32_t off, int whence)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    if (fd < 0) {
        return -(int32_t)PAL_EACCES;
    }
    if (whence < 0 || whence > 2) {
        return -(int32_t)PAL_EACCES;
    }
    /* AL = whence (0/1/2 maps directly to PAL_SEEK_SET/CUR/END = 0/1/2).
     * EDX = low 32-bit offset; ECX = 0 (high 16 bits, zero this milestone).
     * EBX = handle. */
    result = int21(0x42u, (uint8_t)whence,
                   (uint32_t)(int32_t)off,   /* EDX = signed offset, flat cast */
                   (uint32_t)fd,              /* EBX = handle */
                   0u,                        /* ECX = high word (0) */
                   0u, &cf);
    if (cf) {
        return -(int32_t)dos_err_to_pal(result);
    }
    return (int32_t)result;
}

/*
 * milton_remove: AH=41h UNLINK, EDX=path.
 * Returns 0 (PAL_OK) or -(pal_err).
 * Ref: os/milton/int21.c do_unlink; pal.h remove slot.
 */
static int milton_remove(samir_pal_t *p, const char *name)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    result = int21(0x41u, 0x00u,
                   (uint32_t)(uintptr_t)name,  /* EDX = path flat ptr */
                   0u, 0u, 0u, &cf);
    if (cf) {
        return -(int)dos_err_to_pal(result);
    }
    return 0;
}

/*
 * milton_rename: AH=56h RENAME, EDX=old_path, EDI=new_path.
 *
 * FIRST INT 21h handler in the SAMIR PAL that reads a SECOND flat pointer.
 * The flat-kernel adaptation: real DOS uses ES:DI for the new name; in this
 * 32-bit protected-flat kernel the segment is collapsed and EDI carries the
 * flat linear address directly (spec/int21h_calling_convention.json AH=56h:
 * "EDI = flat ptr to the NEW ASCIIZ 8.3 path"; os/milton/int21.c do_rename
 * reads f->edi for new_path). We pass it in the "D" (EDI) constraint.
 *
 * Ref: pal.h rename slot ("same directory only"; ADR-0008 DEC-02 known
 * constraint); os/milton/int21.c do_rename; spec/int21h_calling_convention.json
 * AH=56h.
 */
static int milton_rename(samir_pal_t *p, const char *from, const char *to)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    result = int21(0x56u, 0x00u,
                   (uint32_t)(uintptr_t)from,  /* EDX = old path */
                   0u,                          /* EBX unused */
                   0u,                          /* ECX unused */
                   (uint32_t)(uintptr_t)to,     /* EDI = new path */
                   &cf);
    if (cf) {
        return -(int)dos_err_to_pal(result);
    }
    return 0;
}

/* ---- Console slots -------------------------------------------------------- */

/*
 * milton_conout: write n bytes to console via AH=40h WRITE on handle 1 (stdout).
 * This is the CON device via the standard DOS stdout handle (SFT_DEV_CON at
 * JFT index 1; do_write fans to con_putc for the CON device leg).
 * The slot contract says "no return code -- console writes do not fail in-universe."
 * If the kernel returns an error we silently drop it (the contract is void).
 * Ref: pal.h conout slot; ADR-0009 DEC-06; do_write handle 1 -> CON.
 */
static void milton_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    int cf = 0;
    (void)p;

    if (n == 0u || s == (const char *)0) {
        return;
    }
    /* AH=40h, EBX=1 (stdout/CON), ECX=n, EDX=s */
    (void)int21(0x40u, 0x00u,
                (uint32_t)(uintptr_t)s,   /* EDX=buffer */
                1u,                        /* EBX=handle 1 (stdout/CON) */
                n,                         /* ECX=count */
                0u, &cf);
    /* void return; errors silently ignored per pal.h contract */
    (void)cf;
}

/*
 * milton_conin_line: read one cooked line via AH=3Fh on handle 0 (CON/stdin).
 *
 * INT 21h AH=3Fh on a CON device (SFT_DEV_CON, handle 0) calls the shared
 * conin_cooked_line editor and returns the line bytes INCLUDING CR+LF at the
 * end; EAX = total bytes returned (line + CR + LF; Microsoft KB Q113058;
 * os/milton/int21.c do_read CON branch). The returned buffer contains the
 * line chars followed by 0x0D (CR) and 0x0A (LF), and EAX includes those.
 *
 * We strip the trailing CR and LF to match pal_host.c's fgets-strip semantics
 * (pal_host host_conin_line strips the trailing '\n'). The REPL line parser
 * expects a bare line with no trailing newline (samir_main.c repl_trim strips
 * leading/trailing whitespace, but the contract says we should strip it).
 *
 * Buffer capacity: we request min(cap, 255) bytes from the kernel. The DOS
 * CON read limit is 128 bytes per line (INT21_CON_LINE_MAX in do_read), so
 * any cap > 130 (128 + CR + LF) is effectively the same. We allocate a local
 * staging buffer of 258 bytes to receive the kernel's raw bytes (line + CR +
 * LF), then copy the stripped result into the caller's buf.
 *
 * Returns the line length (>= 0, no trailing newline), or < 0 on EOF/error.
 * 0 bytes read from the kernel (without CF) means EOF or Ctrl-C abort.
 *
 * Ref: pal.h conin_line slot; ADR-0009 DEC-06; os/milton/int21.c do_read CON
 *      branch (KB Q113058); pal_host.c host_conin_line (fgets-strip pattern).
 */
#define CONIN_STAGE_MAX  258u    /* 128-char max line + CR + LF + 2 spare bytes */

static int32_t milton_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    uint8_t stage[CONIN_STAGE_MAX];
    uint32_t want;
    int cf = 0;
    uint32_t got;
    uint32_t len;
    uint32_t i;
    (void)p;

    if (cap == 0u || buf == (char *)0) {
        return -(int32_t)PAL_EACCES;
    }

    /* Request at most CONIN_STAGE_MAX bytes so the kernel does not overwrite
     * our local staging buffer. The cooked editor is capped at 128 chars +
     * CR + LF anyway (INT21_CON_LINE_MAX = 128). */
    want = (cap < CONIN_STAGE_MAX) ? cap : (CONIN_STAGE_MAX - 1u);

    /* AH=3Fh READ on handle 0 (CON/stdin). EDX=stage, EBX=0, ECX=want. */
    got = int21(0x3Fu, 0x00u,
                (uint32_t)(uintptr_t)stage,   /* EDX=buffer */
                0u,                             /* EBX=handle 0 (stdin/CON) */
                want,                           /* ECX=count */
                0u, &cf);
    (void)cf;  /* CON reads: 0 bytes == EOF/^C; CF rare; treat both as EOF */

    got &= 0xFFFFu;   /* AX is low 16 bits */

    if (got == 0u) {
        return -1;    /* EOF or Ctrl-C abort (do_read returns 0, CF clear) */
    }

    /* Strip trailing CR (0x0D) and LF (0x0A).
     * The kernel appends CR+LF after the line chars; we strip both.
     * We also strip any trailing CR that appears without a LF as insurance. */
    len = got;
    while (len > 0u &&
           (stage[len - 1u] == (uint8_t)'\n' || stage[len - 1u] == (uint8_t)'\r')) {
        len--;
    }

    /* Copy at most (cap - 1) chars into the caller's buffer, NUL-terminate. */
    if (len > cap - 1u) {
        len = cap - 1u;
    }
    for (i = 0u; i < len; i++) {
        buf[i] = (char)stage[i];
    }
    buf[len] = '\0';

    return (int32_t)len;
}

/* ---- Terminal extension stubs (S8.4) -------------------------------------- */

/*
 * milton_conin_char: AH=07h DIRECT CONSOLE INPUT WITHOUT ECHO, NO Ctrl-C CHECK.
 * Returns the char (>= 0) or < 0 on error.
 * Ref: pal.h conin_char slot; ADR-0009 DEC-06; os/milton/int21.c do_conin_raw.
 */
static int32_t milton_conin_char(samir_pal_t *p)
{
    int cf = 0;
    uint32_t result;
    (void)p;

    result = int21(0x07u, 0x00u, 0u, 0u, 0u, 0u, &cf);
    if (cf) {
        return -1;
    }
    /* AL = character (bit 7:0 of EAX). */
    return (int32_t)(result & 0xFFu);
}

/*
 * milton_gotoxy: NO-OP stub for S8.2.
 * Console primitive, NOT INT 21h. Exercised only at S8.4 (@SAY/GET/READ forms)
 * gated on S8.3/M4. ADR-0009 DEC-06: "gotoxy/set_attr bind as no-ops...
 * Document the no-op."
 */
static void milton_gotoxy(samir_pal_t *p, uint8_t row, uint8_t col)
{
    /* NO-OP: FLAIR text console cursor placement is not available at S8.2.
     * Exercised at S8.4 (FLAIR forms layer, gated on M4). ADR-0009 DEC-06. */
    (void)p; (void)row; (void)col;
}

/*
 * milton_set_attr: NO-OP stub for S8.2.
 * Console primitive, NOT INT 21h. Exercised only at S8.4.
 * ADR-0009 DEC-06: "set_attr bind as no-ops... Document the no-op."
 */
static void milton_set_attr(samir_pal_t *p, uint8_t attr)
{
    /* NO-OP: FLAIR text attribute control is not available at S8.2.
     * Exercised at S8.4 (FLAIR forms layer, gated on M4). ADR-0009 DEC-06. */
    (void)p; (void)attr;
}

/* ---- Clock slot ----------------------------------------------------------- */

/*
 * milton_today: AH=2Ah GET DATE -> CX=year(full), DH=month(1-12), DL=day(1-31).
 *
 * The pal.h contract returns two-digit YY (00-99 = 1900s, century handling
 * above the PAL), 1-based MM, 1-based DD. We compute YY = year % 100 to match
 * pal_host.c's packing (pal_host stores date_yy which is already 2-digit).
 *
 * Mapping: CX=year (full 4-digit), DH=month, DL=day (from do_getdate which
 * packs DX as ((mon<<8)|day) and CX as year). We extract:
 *   - year  from low 16 bits of CX (returned in ECX output from int21_cxdx)
 *   - month from DH (bits 15:8 of EDX low 16)
 *   - day   from DL (bits 7:0 of EDX low 16)
 *
 * Ref: pal.h today slot; ADR-0009 DEC-06; os/milton/int21.c do_getdate:
 *   "set_cx(f, year); set_dx(f, (uint16_t)(((uint16_t)mon<<8)|(uint16_t)day));"
 *   "set_al(f, dow); cf_clear(f);"
 */
static void milton_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd)
{
    uint32_t ecx_r = 0u, edx_r = 0u;
    (void)p;

    /* AH=2Ah GET DATE; no meaningful input registers.
     * Output: ECX=year(CX), EDX packed as DH=month/DL=day. */
    (void)int21_cxdx(0x2Au, 0x00u,
                     0u, 0u, 0u,
                     &ecx_r, &edx_r, (int *)0);

    if (yy) {
        /* Two-digit YY: year % 100. e.g. 1993 -> 93, 2001 -> 01. */
        uint16_t year = (uint16_t)(ecx_r & 0xFFFFu);
        *yy = (uint8_t)(year % 100u);
    }
    if (mm) {
        /* DH = month (bits 15:8 of the low 16 of EDX). */
        *mm = (uint8_t)((edx_r >> 8) & 0xFFu);
    }
    if (dd) {
        /* DL = day (bits 7:0 of the low 16 of EDX). */
        *dd = (uint8_t)(edx_r & 0xFFu);
    }
}

/* ---- Arena slots ---------------------------------------------------------- */

/*
 * milton_alloc: bump-allocate n bytes from the arena.
 *
 * The arena is a contiguous block obtained at construction via AH=48h ALLOC.
 * The returned fake-segment is converted to a flat linear base (seg << 4).
 * Subsequent allocs advance heap_ptr; reset(mark) rewinds to a saved mark.
 *
 * Alignment: 4-byte align every allocation (matching pal_host.c host_alloc).
 * This ensures struct members land on natural boundaries even when the engine
 * bumps in odd sizes.
 *
 * Returns a pointer, or NULL on exhaustion. The engine MUST fail loud on NULL
 * (pal.h alloc contract: "engine fails loud on NULL").
 *
 * Ref: pal.h alloc slot; ADR-0009 DEC-06; pal_host.c host_alloc (mirror).
 *      ADR-0009 DEC-04 (the AH=48h block returned is above the loaded program
 *      image + BSS after the kernel DEC-04 fix in bead 1q4u).
 */
static void *milton_alloc(samir_pal_t *p, uint32_t n)
{
    pal_milton_state_t *st = state_of(p);
    void *ptr;
    uint32_t avail;

    if (n == 0u) {
        /* Zero alloc returns the current mark (matching pal_host behavior). */
        return (void *)st->heap_ptr;
    }

    /* 4-byte alignment. */
    n = (n + 3u) & ~3u;

    avail = (uint32_t)(st->heap_end - st->heap_ptr);
    if (n > avail) {
        /* Exhausted. The engine is expected to fail loud on NULL (Rule 2). */
        return (void *)0;
    }

    ptr = (void *)st->heap_ptr;
    st->heap_ptr += n;
    return ptr;
}

/*
 * milton_reset: rewind the bump arena to `mark` (or to base if NULL).
 *
 * `mark` must be a pointer previously returned by alloc (or NULL for full
 * reset). An out-of-range mark indicates a corrupted caller; we panic
 * (Rule 2 fail loud) rather than silently corrupting the arena.
 *
 * Ref: pal.h reset slot; pal_host.c host_reset (mirror pattern).
 */
static void milton_reset(samir_pal_t *p, void *mark)
{
    pal_milton_state_t *st = state_of(p);
    uint8_t *m = (uint8_t *)mark;

    if (m == (uint8_t *)0) {
        /* Reset to base: free everything. */
        st->heap_ptr = st->heap_base;
        return;
    }

    if (m < st->heap_base || m > st->heap_ptr) {
        /* Out-of-range mark: corrupted caller pointer. Fail loud (Rule 2). */
        milton_panic();
    }

    st->heap_ptr = m;
}

/* ---- Constructor ---------------------------------------------------------- *
 *
 * pal_milton_make: construct the Milton PAL binding.
 *
 * The state struct itself must be stored somewhere. Since we have no malloc,
 * we embed the state in a static (BSS) variable. There is exactly ONE
 * pal_milton instance per SAMIR session (the SAMIR.COM is the only process),
 * so a single static is correct. The BSS is zeroed by the samir_crt0 entry
 * stub (ADR-0009 DEC-05) before this function is called.
 *
 * Heap acquisition: a single AH=48h ALLOC. The loader has ALREADY bound a
 * disjoint heap arena ABOVE the program image+BSS (and below the env/stack)
 * via int21_mcb_bind_program (ADR-0009 DEC-04, bead 1q4u) -- so the program
 * does NOT shrink its own block first; it simply allocates PAL_MILTON_HEAP_PARAS
 * paragraphs from that arena. The returned segment is converted to a flat base
 * (seg << 4) -- the "fake-paragraph = linear >> 4" convention (spec/memory_map.h;
 * os/milton/int21.c do_alloc / arena_seg_base).
 *
 * Why no AH=4Ah SETBLOCK (the older model is gone): the loader used to lay the
 * arena OVER the whole program window (int21_mcb_reset), so a heap-using program
 * had to SETBLOCK its own block down to free the tail. DEC-04 replaced that with
 * a loader-COMPUTED arena that is disjoint from the loaded image by construction,
 * so the SETBLOCK step is unnecessary and -- against the new model -- wrong (the
 * program's MCB is no longer the arena). AH=48h alone is correct.
 *
 * If AH=48h fails (insufficient memory / arena unbound), we panic (Rule 2 --
 * no arena = the engine cannot run at all; a non-recoverable stop condition).
 *
 * Returns a pointer to the embedded samir_pal_t (the first field of the state).
 *
 * Ref: ADR-0009 DEC-06 (pal_milton_make mirrors pal_host_make signature);
 *      ADR-0009 DEC-04 (AH=48h arena disjoint from program; loader bind, bead 1q4u);
 *      spec/memory_map.h (paragraph conv); os/milton/int21.c do_alloc /
 *      int21_mcb_bind_program (the loader-bound disjoint arena).
 */

/* Single static state instance (BSS-zeroed by samir_crt0 before pal_milton_make
 * is called; ADR-0009 DEC-05). The loader binds the heap arena disjointly
 * (int21_mcb_bind_program, DEC-04) -- no program-side SETBLOCK constant needed. */
static pal_milton_state_t s_milton_state;

samir_pal_t *pal_milton_make(void)
{
    pal_milton_state_t *st = &s_milton_state;
    int cf = 0;
    uint32_t result;
    uint32_t heap_seg;

    /* Wire the vtable. */
    st->vtable.open       = milton_open;
    st->vtable.close      = milton_close;
    st->vtable.read       = milton_read;
    st->vtable.write      = milton_write;
    st->vtable.seek       = milton_seek;
    st->vtable.remove     = milton_remove;
    st->vtable.rename     = milton_rename;
    st->vtable.conout     = milton_conout;
    st->vtable.conin_line = milton_conin_line;
    st->vtable.conin_char = milton_conin_char;
    st->vtable.gotoxy     = milton_gotoxy;
    st->vtable.set_attr   = milton_set_attr;
    st->vtable.today      = milton_today;
    st->vtable.alloc      = milton_alloc;
    st->vtable.reset      = milton_reset;

    /* AH=48h ALLOC -- request PAL_MILTON_HEAP_PARAS paragraphs from the loader-
     * bound DISJOINT arena (DEC-04, bead 1q4u). No AH=4Ah SETBLOCK first: the
     * loader (int21_mcb_bind_program) already placed the arena ABOVE the program
     * image+BSS, so AH=48h draws straight from it.
     * BX = paragraphs requested. Returns AX = DOS segment of the block (CF=0)
     * or CF=1, AX=0x0008, BX=largest free (on failure).
     * The flat ABI maps BX to EBX (int21.c do_alloc reads (uint16_t)(f->ebx)
     * for want). */
    result = int21(0x48u, 0x00u,
                   0u,                           /* EDX unused */
                   PAL_MILTON_HEAP_PARAS,        /* EBX = paragraphs wanted */
                   0u, 0u, &cf);
    if (cf) {
        /* Insufficient memory. Panic -- no arena = no engine (Rule 2 fail loud). */
        milton_panic();
    }

    /* Convert the DOS segment to a flat linear base: seg << 4.
     * This is the "fake-paragraph = linear >> 4" convention documented in
     * spec/memory_map.h and os/milton/int21.c arena_seg_base / do_alloc. */
    heap_seg = result & 0xFFFFu;
    st->heap_base = (uint8_t *)(uintptr_t)(heap_seg << 4);
    st->heap_ptr  = st->heap_base;
    st->heap_end  = st->heap_base + PAL_MILTON_HEAP_BYTES;

    return &st->vtable;
}

/* ---- Entry point ---------------------------------------------------------- *
 *
 * samir_milton_entry: the symbol the samir_crt0 calls after zeroing BSS and
 * initializing ESP. Constructs the PAL, creates the interpreter, runs the REPL,
 * and returns (the crt0 then issues INT 21h AH=4Ch to exit cleanly).
 *
 * Entry sequence:
 *   1. pal_milton_make() -- construct the PAL (acquires the heap via AH=48h).
 *   2. xb_interp_make(pal) -- construct the SAMIR interpreter over the PAL.
 *      Uses the PAL's arena for all interpreter-internal allocations. If it
 *      returns NULL (arena exhausted building the interp), we panic (Rule 2).
 *   3. samir_repl(pal, ip) -- run the dot-prompt REPL until QUIT/EXIT/EOF.
 *   4. xb_interp_free(ip) -- close all open work areas (samir_main.c contract).
 *   5. return -- the crt0 emits INT 21h AH=4Ch AL=0.
 *
 * The SAMIR_MAIN_STANDALONE seam (samir_main.c) shows the host pattern; this
 * is the identical sequence adapted for the freestanding target.
 *
 * Ref: ADR-0009 DEC-05 (BSS zeroing in the crt0 entry stub, before this call);
 *      ADR-0009 DEC-06 (samir_milton_entry as the crt0's call target);
 *      os/samir/samir_main.c SAMIR_MAIN_STANDALONE block (the mirror pattern);
 *      os/samir/include/samir/interp.h (xb_interp_make / xb_interp_free /
 *        samir_repl declarations).
 */
void samir_milton_entry(void)
{
    samir_pal_t *pal;
    xb_interp   *ip;

    pal = pal_milton_make();
    /* pal_milton_make panics on failure; it never returns NULL. */

    ip = xb_interp_make(pal);
    if (!ip) {
        /* Arena exhausted constructing the interpreter. Fail loud (Rule 2). */
        milton_panic();
    }

    (void)samir_repl(pal, ip);

    xb_interp_free(ip);
    /* Return to samir_crt0, which will INT 21h AH=4Ch AL=0. */
}
