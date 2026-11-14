/* The real gate's finish/slot_clear discards tenant state, while the volume,
 * global scrap and OS settings survive. Reset also initializes gate state
 * added by newer fakes, without a conditional dependency on their layout. */
static void restart_host_tenant(void)
{
    static tbx_host_state_t previous;
    previous=tbx_host;
    /* Transfer the file allocations across reset; they belong to the volume. */
    memset(tbx_host.files,0,sizeof tbx_host.files);
    tbx_host_reset();
#define KEEP(field) memcpy(&tbx_host.field,&previous.field,sizeof tbx_host.field)
    KEEP(files);KEEP(deleted);KEEP(ndeleted);
    KEEP(scrap);KEEP(scrap_len);KEEP(scrap_puts);
    KEEP(cw);KEEP(ch);KEEP(origin_h);KEEP(origin_v);
    KEEP(clock);KEEP(settings);KEEP(memory);KEEP(free_bytes);
    /* These are injected backend faults, not tenant-owned state. */
    KEEP(fail_create);KEEP(fail_write);KEEP(read_limit);KEEP(write_limit);
    KEEP(write_budget);KEEP(fail_close);KEEP(fail_close_handle);KEEP(file_error);
#undef KEEP
}
