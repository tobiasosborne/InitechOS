/* Production wiring extracted from legacy-final kmain.c BOOT_SHELL.
 * Ordering and callbacks follow that reference; hardware contracts are in
 * sysinit.c, console.c, block_device.c and the public headers. */
#include "startup.h"
#include "boot_info.h"
#include "memory_map.h"
#include "io.h"
#include "console.h"
#include "int21.h"
#include "sysinit.h"
#include "psp.h"
#include "fat12.h"
#include "fileio_fat.h"
#include "loader.h"
#include "ata.h"
#include "block_device.h"
#include "absdisk_binding.h"
#include "kbd.h"
#include "pit.h"
#include "rtc.h"
#include "tsr_contract.h"
#include "../dos/command/entry.h"
#define COM1_LSR 0x3fdu
#define COM1_THR 0x3f8u
#define LSR_THRE 0x20u
#define SERIAL_SPIN_MAX 100000u
void serial_putc(char c)
{
    uint32_t spins = 0u;
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
        if (++spins >= SERIAL_SPIN_MAX) {
            return;             
        }
    }
    outb(COM1_THR, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s) {
        serial_putc(*s++);
    }
}

void serial_putu(uint32_t v)   
{
    char buf[10];
    int n = 0;

    if (v == 0) {
        serial_putc('0');
        return;
    }
    while (v > 0 && n < (int)sizeof(buf)) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n > 0) {
        serial_putc(buf[--n]);
    }
}

void serial_puthex32(uint32_t v)
{
    static const char H[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(H[(v >> i) & 0xFu]);
    }
}

void serial_puti(int32_t v)
{
    if (v < 0) {
        serial_putc('-');
        
        serial_putu((uint32_t)(-(int64_t)v));
    } else {
        serial_putu((uint32_t)v);
    }
}

extern void isr_selftest(void);

extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

extern void int21_entry(void);

extern void int20_entry(void);

extern void irq0_entry(void);
extern void irq1_entry(void);

extern void irq12_entry(void);

static console_t *g_int21_con = 0;

psp_t g_kernel_psp;

static void int21_con_sink(char c)
{
    if (g_int21_con) {
        console_putc(g_int21_con, c);
    }
    serial_putc(c);
}

static void ansi_put_char(void *ctx, uint8_t ch, uint8_t cga_attr)
{
    console_t *con = (console_t *)ctx;
    if (con) {
        console_set_attr(con, cga_attr);
        console_putc(con, (char)ch);
    }
    serial_putc((char)ch);
}
static void ansi_set_cursor(void *ctx, int row, int col)
{
    if (ctx) {
        console_set_cursor((console_t *)ctx, row, col);
    }
}
#include "ansi_cursor.h"
static void ansi_erase_display(void *ctx, int mode)
{
    if (ctx) {
        console_erase_display((console_t *)ctx, mode);
    }
}
static void ansi_set_wrap(void *ctx, int on)
{
    if (ctx) {
        ((console_t *)ctx)->nowrap = (uint8_t)!on;
        ((console_t *)ctx)->past_end = 0;
    }
}
static void ansi_erase_line(void *ctx, int mode)
{
    if (ctx) {
        console_erase_line((console_t *)ctx, mode);
    }
}
static void ansi_set_attr(void *ctx, uint8_t cga_attr)
{
    if (ctx) {
        console_set_attr((console_t *)ctx, cga_attr);
    }
}
static void ansi_get_cursor(void *ctx, int *row, int *col)
{
    if (ctx) {
        uint32_t r = 0, c = 0;
        console_get_cursor((console_t *)ctx, &r, &c);
        *row = (int)r;
        *col = (int)c;
    }
}

void int21_exit_hook(uint8_t code)
{
    
    serial_puts("INT21-EXIT rc=");
    serial_putu((uint32_t)code);
    serial_putc('\n');
    if (g_int21_con) {
        console_puts(g_int21_con, "\nProgram terminated.\n");
    }
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

static int int21_conin_get_kbd(void)
{
    int c;
    while ((c = kbd_getchar()) < 0) {
        
        __asm__ __volatile__("int %0" : : "i"(TSR_VEC_IDLE) : "memory");
        
        console_cursor_wait(g_int21_con, pit_ticks(), 1);
        
        __asm__ __volatile__("hlt");   
    }
    console_cursor_wait(g_int21_con, pit_ticks(), 0);
    
    return c;
}

void KERNEL_ASM_ABI tsr_screen_dispatch(int_frame_t *f)
{
    uint32_t p = f->edx;
    uint8_t al = (uint8_t)f->eax;
    uint16_t err = 0u;
    if (!g_int21_con || al > 2u) err = 0x0001u;
    else if (p == 0u || p + TSR_SCREEN_BYTES < p) err = 0x0009u;
    else if (al == 0u) console_save_page(g_int21_con, (uint8_t *)(uintptr_t)p);
    else if(al==1u)console_restore_page(g_int21_con, (const uint8_t *)(uintptr_t)p);
    else console_update_page(g_int21_con, (const uint8_t *)(uintptr_t)p);
    if (err) {
        f->eax = (f->eax & 0xFFFF0000u) | err;
        f->eflags |= 1u;
    } else {
        f->eflags &= ~1u;
    }
}

static int int21_conin_poll_kbd(void)
{
    int c = kbd_getchar();   
    
    return c;
}

static int int21_clock_get_rtc(uint16_t *year, uint8_t *month, uint8_t *day,
                               uint8_t *hour, uint8_t *minute, uint8_t *second,
                               uint8_t *dow)
{
    rtc_time_t t;
    if (!rtc_now(&t)) {
        return 0;   
    }
    *year   = t.year;
    *month  = t.month;
    *day    = t.day;
    *hour   = t.hour;
    *minute = t.minute;
    *second = t.second;
    *dow    = t.day_of_week;
    return 1;
}

static int int21_clock_set_rtc(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hour, uint8_t minute, uint8_t second,
                               uint8_t which)
{
    rtc_time_t t;
    t.year = year; t.month = month; t.day = day;
    t.hour = hour; t.minute = minute; t.second = second; t.day_of_week = 0u;
    
    uint8_t rmask = 0u;
    if (which & INT21_CLOCK_SET_DATE) rmask |= RTC_SET_DATE;
    if (which & INT21_CLOCK_SET_TIME) rmask |= RTC_SET_TIME;
    return rtc_set(&t, rmask);
}

void int21_setvect_idt(uint8_t vec, uint32_t handler)
{
    
    if (int21_vector_gate(vec) == 0x8Eu)
        idt_install_irq(vec, (void *)(uintptr_t)handler);
    else
        idt_install_trap(vec, (void *)(uintptr_t)handler);
}

uint32_t int21_getvect_idt(uint8_t vec)
{
    idt_gate_t g = idt_get_gate(vec);
    return (uint32_t)g.offset_lo | ((uint32_t)g.offset_hi << 16);
}

fat12_volume_t g_absdisk_vol;

int int21_absdisk_read(uint32_t lba, uint32_t count, void *buf)
{
    return fat12_read_sectors(&g_absdisk_vol, lba, count, buf);
}

int int21_absdisk_write(uint32_t lba, uint32_t count, const void *buf)
{
    int rc = fat12_write_sectors(&g_absdisk_vol, lba, count, buf);
    
    if (rc == 0) fileio_fat_absdisk_written(lba, count, buf);
    return rc;
}

static int mount_dos_volume(unsigned letter, fat12_volume_t *vol,
                            const blockdev_t *dev, uint8_t *sector)
{
    du_map map;
    if (block_device_map(letter, &map)) {
        
        if (letter == 0u) {
            int rc = fat12_mount(vol, dev, sector);
            if (rc) return rc;
        }
        return FAT12_ERR_PARTITION;
    }
    int rc = map.partition ? fat12_mount_primary(vol, dev, sector, map.partition)
                           : fat12_mount(vol, dev, sector);
    if (!rc && (vol->base_lba != map.base || vol->total_sectors > map.sectors))
        rc = FAT12_ERR_PARTITION;
    return rc;
}

static ata_ctx_t fixed_master;
static blockdev_t fixed_dev;
static fat12_volume_t fixed_volumes[2];
static uint8_t fixed_sector[512];
static void bind_fixed_absdisk(unsigned letter, int mounted);

static void mount_fixed_volume(unsigned letter)
{
    int rc = mount_dos_volume(letter, &fixed_volumes[letter-2], &fixed_dev,
                              fixed_sector);
    if (!rc) rc = fileio_fat_add_drive(letter, &fixed_volumes[letter-2]);
    if (rc) fileio_fat_drop_drive(letter);
    bind_fixed_absdisk(letter, !rc);
    serial_puts(rc ? "DOS-VOLUME-FAIL " : "DOS-VOLUME-OK ");
    serial_putc((char)('A'+letter));
    serial_puts(" rc="); serial_puti(rc); serial_putc('\n');
}

static void fixed_volume_written(unsigned letter, uint32_t lba, uint32_t count,
                                 const void *buf)
{
    if (lba == 0u) mount_fixed_volume(letter);
    else fileio_fat_raw_written(letter, lba, count, buf);
}

static int fixed_write(unsigned letter, uint32_t lba, uint32_t count, const void *buf)
{
    int rc = fat12_write_sectors(&fixed_volumes[letter-2], lba, count, buf);
    if (rc == 0) fixed_volume_written(letter, lba, count, buf);
    return rc;
}
static int abs_read_c(uint32_t l, uint32_t n, void *b)
{ return fat12_read_sectors(&fixed_volumes[0], l, n, b); }
static int abs_read_d(uint32_t l, uint32_t n, void *b)
{ return fat12_read_sectors(&fixed_volumes[1], l, n, b); }
static int abs_write_c(uint32_t l, uint32_t n, const void *b) { return fixed_write(2u, l, n, b); }
static int abs_write_d(uint32_t l, uint32_t n, const void *b) { return fixed_write(3u, l, n, b); }
static void bind_fixed_absdisk(unsigned letter, int mounted)
{
    const fat12_volume_t *v = &fixed_volumes[letter-2];
    int21_absdisk_backend_t backend;
    if (!mounted) { int21_set_absdisk_drive(letter, 0); return; }
    backend.read = letter == 2u ? abs_read_c : abs_read_d;
    backend.write = v->fat_type == FAT_TYPE_FAT12 && fixed_dev.write_sectors
        ? (letter == 2u ? abs_write_c : abs_write_d) : 0;
    backend.total_sectors = v->total_sectors;
    int21_set_absdisk_drive(letter, &backend);
}

static void mount_fixed_volumes(void)
{
    ata_ctx_init_primary_master(&fixed_master);
    ata_blockdev_init(&fixed_dev, &fixed_master);
    for (unsigned letter=2;letter<4;letter++) {
        du_map map;
        if (block_device_map(letter, &map)) continue;
        mount_fixed_volume(letter);
        
        block_device_watch(letter, &map, fixed_volume_written);
    }
}

static void mount_context(int rc, const fat12_volume_t *vol)
{
    serial_puti((int32_t)rc);
    serial_puts(" base_lba="); serial_putu(vol->base_lba);
    serial_puts(" hidden_sectors="); serial_putu(vol->bpb.hidden_sectors);
    serial_putc('\n');
}

static inline void dos_puts(const char *dollar_terminated)
{
    __asm__ __volatile__(
        "int $0x21"
        :
        : "a"(0x0900u), "d"((uint32_t)(uintptr_t)dollar_terminated)
        : "cc", "memory");
}

static void dir_putc(char c)
{
    serial_putc(c);
}

static void dir_puts(const char *s)
{
    while (*s) {
        dir_putc(*s++);
    }
}

static void dir_putu_field(uint32_t v, int width)
{
    char buf[10];
    int n = 0;
    int pad;

    if (v == 0) {
        buf[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(buf)) {
            buf[n++] = (char)('0' + (v % 10u));
            v /= 10u;
        }
    }
    for (pad = width - n; pad > 0; pad--) {
        dir_putc(' ');
    }
    while (n > 0) {
        dir_putc(buf[--n]);
    }
}

static int dir_visit(const dir_entry_t *e, void *user)
{
    char name[FAT12_NAME83_MAX];
    (void)user;

    if (e->attribute & (DIR_ATTR_VOLLABEL | DIR_ATTR_DIRECTORY)) {
        return 0;
    }
    if (fat12_format_83(e, name) != FAT12_OK) {
        return 0; 
    }

    dir_puts(name);
    dir_putu_field(e->file_size, 13);
    dir_putc('\n');
    return 0;
}

static int exec_name_is_comspec(const char *name83)
{
    static const char comspec[] = "COMMAND.COM";
    uint32_t k = 0u;
    while (comspec[k] != '\0') {
        char c = name83[k];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (c != comspec[k]) return 0;
        k++;
    }
    return name83[k] == '\0';
}uint16_t loader_exec_by_name(const char *name83, uint16_t dir_start,
                                    const char *cmd_tail, uint32_t cmd_tail_len,
                                    uint32_t env_block, uint8_t *out_rc)
{
    uint8_t rc = 0;
    
    if (exec_name_is_comspec(name83)) {
        (void)dir_start; (void)env_block;
        command_exec_secondary(cmd_tail, cmd_tail_len);
        if (out_rc) {
            *out_rc = 0u;
        }
        return 0u;
    }
    
    uint32_t fat_len;
    const uint8_t *fat = fileio_fat_fat_buffer(&fat_len);
    loader_bind_fat_volume(fileio_fat_current_volume(), fat, fat_len);
    loader_status_t st = load_program_from_fat(name83, dir_start, cmd_tail,
                                               cmd_tail_len, env_block, &rc);

    switch (st) {
        case LOADER_OK:
            if (out_rc) {
                *out_rc = rc;
            }
            return 0u;                          
        case LOADER_ERR_NO_VOLUME:
        case LOADER_ERR_NOT_FOUND:
            return INT21_ERR_FILE_NOT_FOUND;    
        case LOADER_ERR_BUSY:
        case LOADER_ERR_NOMEM:
        case LOADER_ERR_FIXED_BASE:
            return INT21_ERR_INSUFFICIENT_MEM;  
        case LOADER_ERR_TOO_BIG:
        case LOADER_ERR_ZERO_LEN:
        case LOADER_ERR_BAD_ENV:
        case LOADER_ERR_BAD_FORMAT:
            return INT21_ERR_BAD_FORMAT;        
        case LOADER_ERR_READ:
        default:
            return INT21_ERR_FILE_NOT_FOUND;    
    }
}

static uint16_t overlay_exec(const char *name, uint16_t dir, uint32_t addr, uint32_t reloc)
{
 uint32_t n; const uint8_t *fat=fileio_fat_fat_buffer(&n);
 loader_bind_fat_volume(fileio_fat_current_volume(),fat,n);
 switch(loader_load_overlay(name,dir,addr,reloc,int21_overlay_dest_ok)) {
 case LOADER_OK:return 0; case LOADER_ERR_NOT_OWNED:return INT21_ERR_INVALID_MEMORY;
 case LOADER_ERR_BAD_FORMAT:return INT21_ERR_BAD_FORMAT; default:return INT21_ERR_FILE_NOT_FOUND;
 }
}
void startup(void)
{
 serial_puts("KERNEL\n");
 boot_info_t b=*boot_info();
 sysinit_early(int21_con_sink,int21_exit_hook,&g_kernel_psp,serial_puts);
 int21_set_break_trace(serial_putc);
 static console_t con;
 if(b.lfb_bpp==8) {
  uint8_t shift=b.lfb_dac_bits==8?0:2;
  const uint8_t colors[6]={0x6f,0xa0,0x8e,0xc0,0xc0,0xc0};
  for(unsigned i=0;i<2;i++) {outb(0x3c8,i?CONSOLE_FG_IDX:CONSOLE_BG_IDX);
   for(unsigned j=0;j<3;j++)outb(0x3c9,colors[i*3+j]>>shift);}
 }
 if(console_init(&con,&b)!=CONSOLE_OK) {serial_puts("Console initialization failed\n"); for(;;)__asm__ volatile("cli; hlt");}
 g_int21_con=&con;
 serial_puts("BANNER-BEGIN\n");
 dos_puts("InitechDOS  Version 3.30\n$");
 dos_puts("Copyright (C) 1991 Initech Systems Corporation.  All Rights Reserved.\n$");
 serial_puts("BANNER-END\nBANNER\n");
    static uint8_t sector_buf[BLOCKDEV_SECTOR_SIZE]; 
    static ata_ctx_t      ata_slave;
    static blockdev_t     fatdev;
    static crit_blockdev_t crit_fatdev;   
    static fat12_volume_t vol;
    int            mounted = 0;

    serial_puts("FAT-MOUNT-BEGIN\n");
    {
        int rc;

        ata_ctx_init_primary_slave(&ata_slave);
        ata_blockdev_init(&fatdev, &ata_slave);

        crit_blockdev_init(&crit_fatdev, &fatdev);

        rc = mount_dos_volume(0, &vol, &crit_fatdev.dev, sector_buf);
        if (rc == FAT12_OK) {
            serial_puts("FAT-MOUNT-OK\n");
            mounted = 1;

            crit_blockdev_set_hook(int21_run_critical_error);

            dir_puts("Directory of A:\\\n");
            rc = fat12_read_root_dir(&vol, sector_buf, dir_visit, (void *)0);
            if (rc == FAT12_OK) {
                serial_puts("DIR-OK\n");
            } else {
                serial_puts("DIR-FAIL rc=");
                serial_puti((int32_t)rc);
                serial_putc('\n');
            }

            rc = fileio_fat_bind(&vol);
            if (rc == 0) {
                serial_puts("FILEIO-BIND-OK\n");
            } else {
                serial_puts("FILEIO-BIND-FAIL rc=");
                serial_puti((int32_t)rc);
                serial_putc('\n');
            }

            {
                uint32_t load_fat_len = 0;
                const uint8_t *load_fat =
                    (const uint8_t *)fileio_fat_fat_buffer(&load_fat_len);
                loader_bind_fat_volume(&vol, load_fat, load_fat_len);
            }
            int21_set_exec_backend(loader_exec_by_name);
            int21_set_overlay_backend(overlay_exec);   
            serial_puts("LOADER-FAT-BIND-OK\n");

            bind_absdisk_volume(&vol, &fatdev);
            mount_fixed_volumes();
            serial_puts("ABSDISK-BIND-OK\n");
        } else {
            
            serial_puts("FAT-MOUNT-FAIL rc=");
            mount_context(rc, &vol);
        }
    }
    serial_puts("FAT-MOUNT-END\n");
    (void)mounted;

    int21_set_clock(int21_clock_get_rtc, int21_clock_set_rtc);
    serial_puts("CLOCK-LIVE\n");

    {
        int21_ansi_console_t ansi_ops;
        ansi_ops.put_char      = ansi_put_char;
        ansi_ops.set_cursor    = ansi_set_cursor;
        ansi_ops.cursor_rel    = ansi_cursor_rel;
        ansi_ops.erase_display = ansi_erase_display;
        ansi_ops.erase_line    = ansi_erase_line;
        ansi_ops.set_wrap      = ansi_set_wrap;
        ansi_ops.set_attr      = ansi_set_attr;
        ansi_ops.get_cursor    = ansi_get_cursor;
        ansi_ops.ctx           = g_int21_con;
        int21_set_ansi_console(&ansi_ops);
        int21_set_ansi_gate(sysinit_ansi_enabled);
    }
    serial_puts("ANSI-WIRED\n");

    int21_set_vectortable(int21_setvect_idt, int21_getvect_idt);
    serial_puts("VECT-LIVE\n");

    {
        
        uint32_t cfg_fat_len = 0u;
        void    *cfg_fat = fileio_fat_fat_buffer(&cfg_fat_len);
        (void)sysinit_apply_config(mounted ? &vol : (const fat12_volume_t *)0,
                                   sector_buf, cfg_fat, cfg_fat_len, serial_puts);
    }

 sysinit_enable_irqs(serial_puts);
 int21_set_conin(int21_conin_get_kbd,int21_conin_poll_kbd);
 serial_puts("CONIN-LIVE\n");
}

/* Restore the DOS surface after an optional graphical environment returns. */
void desktop_console_restore(void)
{
    boot_info_t b=*boot_info();
    if (b.lfb_bpp==8) {
        uint8_t shift=b.lfb_dac_bits==8?0:2;
        const uint8_t rgb[6]={0x6f,0xa0,0x8e,0xc0,0xc0,0xc0};
        for(unsigned i=0;i<2;i++) {
            outb(0x3c8,i?CONSOLE_FG_IDX:CONSOLE_BG_IDX);
            for(unsigned j=0;j<3;j++) outb(0x3c9,rgb[i*3+j]>>shift);
        }
    }
    if (!g_int21_con || console_init(g_int21_con,&b)!=CONSOLE_OK) {
        serial_puts("PANIC console restore\nHALTED\n");
        for(;;) __asm__ volatile("cli; hlt");
    }
}
