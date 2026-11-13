/* Flat link/load contract from loader spike; no section GC or overlays. */


ENTRY(_start)

KERNEL_BASE = 0x00500000;

SECTIONS
{
    . = KERNEL_BASE;
    _kernel_start = .;

    
    .text : {
        
        *kstart.o(.text*)
        *(.text._start)
        *isr.o(.text*)
        *(.text*)
    }

    .rodata : {
        
        *(.rodata.g_exc_stubs)
        *(.rodata*)
    }
    .data   : { *(.data*) }

    _kernel_image_end = .;
    .bss : {
        _bss_start = .;
        *(.bss.g_idt .bss.g_idt_ptr)
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    }

    _kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}


ASSERT(_kernel_start == KERNEL_BASE && ADDR(.text) == KERNEL_BASE,
       "kernel floor mismatch -- .text/_kernel_start must equal KERNEL_BASE (0x00500000)")
ASSERT(_kernel_end <= 0x005F0000,
       "kernel image busts KERNEL_IMAGE_CEIL (0x005F0000, kernel stack base)")
