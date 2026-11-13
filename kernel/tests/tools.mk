# Tier 0 import wiring; target rules remain in Makefile.
HOST_INCLUDES := -I. -Iinclude -I$(TOP)/kernel -Itests -Itests/disk -I$(TOP)/kernel/tests/disk -I$(TOP)/kernel/include -I$(TOP)/factory/testlib -I$(TOP)/build/kernel
TEST_HDRS := $(shell find . -name '*.h') $(wildcard $(TOP)/kernel/include/*.h) $(wildcard $(TOP)/factory/testlib/*.h)
PYTHON_FAT_REFERENCE ?= python3
PYTHON_MESSAGE_REFERENCE ?= python3
THOROUGH ?= 0
export PYTHON_FAT_REFERENCE PYTHON_MESSAGE_REFERENCE THOROUGH
HOST_TESTS :=
EXTRA_HOST_TESTS :=
EXTRA_HOST_TESTS += tool_test_40oq
tool_test_40oq_SRCS := tests/test_40oq.c
tool_test_40oq_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/config_sys-fd8ca8dc255d.o
tool_test_40oq_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_40oq_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_conin
tool_test_conin_SRCS := tests/test_conin.c
tool_test_conin_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/kbd-11ae29d9aa4e.o
tool_test_conin_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_conin_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_int21_wildcard
tool_test_int21_wildcard_SRCS := tests/test_int21_wildcard.c
tool_test_int21_wildcard_OBJS := $(OUT)/host-objects/int21-f38962d47f22.o $(OUT)/host-objects/devices-48f859b2cd9c.o $(OUT)/host-objects/mcb-f49eee7d9232.o $(OUT)/host-objects/sft-2df5f821c0ba.o $(OUT)/host-objects/psp-cf47cac52a8f.o $(OUT)/host-objects/irq-0a081605dfcd.o $(OUT)/host-objects/config_sys-f09076d85ba2.o
tool_test_int21_wildcard_CFLAGS := -D_POSIX_C_SOURCE=200809L -DINT21_WILDCARD_TESTSEAM
tool_test_int21_wildcard_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_absdisk
tool_test_absdisk_SRCS := tests/test_absdisk.c tests/disk/blockdev_file.c
tool_test_absdisk_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_absdisk_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_absdisk_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_ansi
tool_test_ansi_SRCS := tests/test_ansi.c
tool_test_ansi_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_ansi_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_ansi_bridge
tool_test_ansi_bridge_SRCS := tests/test_ansi_bridge.c
tool_test_ansi_bridge_OBJS := $(OUT)/host-objects/ansi-5d041b26cee4.o $(OUT)/host-objects/console-574c5ac43562.o $(OUT)/host-objects/console_pixels-e495db98b8d6.o
tool_test_ansi_bridge_CFLAGS :=
tool_test_ansi_bridge_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_ansi_wire
tool_test_ansi_wire_SRCS := tests/test_ansi_wire.c
tool_test_ansi_wire_OBJS := $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/config_sys-fd8ca8dc255d.o
tool_test_ansi_wire_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_ansi_wire_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_ata
tool_test_ata_SRCS := tests/test_ata.c
tool_test_ata_OBJS := $(OUT)/host-objects/ata-180cee39c418.o
tool_test_ata_CFLAGS := -D_POSIX_C_SOURCE=200809L -DATA_HOST_TEST -DATA_POLL_LIMIT=16
tool_test_ata_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fileio_subdir
tool_test_fileio_subdir_SRCS := tests/test_fileio_subdir.c tests/disk/blockdev_file.c
tool_test_fileio_subdir_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/fileio_fat-a14fe1ad186a.o $(OUT)/host-objects/fat12-141c06111b76.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_fileio_subdir_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fileio_subdir_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_block_device
tool_test_block_device_SRCS := tests/test_block_device.c
tool_test_block_device_OBJS := $(OUT)/host-objects/block_device-32db98187ce4.o
tool_test_block_device_CFLAGS := -no-pie -DBLOCK_DEVICE_HOST
tool_test_block_device_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_config_sys_fuzz
tool_test_config_sys_fuzz_SRCS := tests/test_config_sys_fuzz.c
tool_test_config_sys_fuzz_OBJS := $(OUT)/host-objects/config_sys-fd8ca8dc255d.o
tool_test_config_sys_fuzz_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_config_sys_fuzz_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_config_sys
tool_test_config_sys_SRCS := tests/test_config_sys.c
tool_test_config_sys_OBJS := $(OUT)/host-objects/config_sys-e23e2142c065.o
tool_test_config_sys_CFLAGS := -D_POSIX_C_SOURCE=200809L -DCONFIG_SYS_BASELINE_PATH='"$(abspath $(TOP))/kernel/tests/data/dos_config_sys_baseline.txt"'
tool_test_config_sys_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_conkeys
tool_test_conkeys_SRCS := tests/test_conkeys.c
tool_test_conkeys_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/kbd-11ae29d9aa4e.o
tool_test_conkeys_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_conkeys_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_console
tool_test_console_SRCS := tests/test_console.c
tool_test_console_OBJS := $(OUT)/host-objects/console-da9be2570afc.o $(OUT)/host-objects/console_pixels-a0f969fa8512.o
tool_test_console_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_console_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_console_look
tool_test_console_look_SRCS := tests/test_console_look.c
tool_test_console_look_OBJS := $(OUT)/host-objects/console-574c5ac43562.o $(OUT)/host-objects/console_pixels-e495db98b8d6.o
tool_test_console_look_CFLAGS :=
tool_test_console_look_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat16_window
tool_test_fat16_window_SRCS := tests/test_fat16_window.c tests/disk/blockdev_file.c
tool_test_fat16_window_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat16_window_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat16_window_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_devices
tool_test_devices_SRCS := tests/test_devices.c
tool_test_devices_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_devices_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_devwire
tool_test_devwire_SRCS := tests/test_devwire.c
tool_test_devwire_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_devwire_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_devwire_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_dos_volumes_host
tool_dos_volumes_host_SRCS := tests/dos_volumes_host.c tests/disk/blockdev_file.c
tool_dos_volumes_host_OBJS := $(OUT)/host-objects/int21-952950b0799a.o $(OUT)/host-objects/devices-bd2a4e967423.o $(OUT)/host-objects/fileio_fat-f97903a2ce53.o $(OUT)/host-objects/fat12-3027de855556.o $(OUT)/host-objects/sft-e21115eecc3f.o $(OUT)/host-objects/psp-1c1de08ad77c.o $(OUT)/host-objects/mcb-79410501c51a.o $(OUT)/host-objects/irq-9e5fd41d6656.o
tool_dos_volumes_host_CFLAGS := -D_POSIX_C_SOURCE=200809L -no-pie
tool_dos_volumes_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_drive_letters
tool_test_drive_letters_SRCS := tests/test_drive_letters.c
tool_test_drive_letters_OBJS := $(OUT)/host-objects/block_device-32db98187ce4.o
tool_test_drive_letters_CFLAGS := -DBLOCK_DEVICE_HOST -no-pie
tool_test_drive_letters_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_int21
tool_test_int21_SRCS := tests/test_int21.c
tool_test_int21_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/config_sys-fd8ca8dc255d.o
tool_test_int21_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_int21_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_exec
tool_test_exec_SRCS := tests/test_exec.c
tool_test_exec_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_exec_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_exec_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_fat_dump
tool_fat_dump_SRCS := tests/disk/fat_dump.c tests/disk/blockdev_file.c
tool_fat_dump_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_fat_dump_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_fat_dump_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_bpb
tool_test_fat12_bpb_SRCS := tests/test_fat12_bpb.c tests/disk/blockdev_file.c
tool_test_fat12_bpb_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_bpb_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_bpb_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_chain
tool_test_fat12_chain_SRCS := tests/test_fat12_chain.c tests/disk/blockdev_file.c
tool_test_fat12_chain_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_chain_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_chain_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_dir
tool_test_fat12_dir_SRCS := tests/test_fat12_dir.c tests/disk/blockdev_file.c
tool_test_fat12_dir_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_dir_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_dir_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_fat12_corrupt_fuzz
tool_fat12_corrupt_fuzz_SRCS := tests/fat12_corrupt_fuzz.c tests/disk/blockdev_file.c
tool_fat12_corrupt_fuzz_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_fat12_corrupt_fuzz_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_fat12_corrupt_fuzz_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_fault_rollback
tool_test_fat12_fault_rollback_SRCS := tests/test_fat12_fault_rollback.c tests/disk/blockdev_file.c
tool_test_fat12_fault_rollback_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_fault_rollback_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_fault_rollback_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_fat12_fuzz
tool_fat12_fuzz_SRCS := tests/fat12_fuzz.c tests/disk/blockdev_file.c
tool_fat12_fuzz_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_fat12_fuzz_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_fat12_fuzz_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_move
tool_test_fat12_move_SRCS := tests/test_fat12_move.c tests/disk/blockdev_file.c
tool_test_fat12_move_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_move_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_move_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_partial
tool_test_fat12_partial_SRCS := tests/test_fat12_partial.c tests/disk/blockdev_file.c
tool_test_fat12_partial_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_partial_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_partial_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_partition
tool_test_partition_SRCS := tests/test_partition.c tests/disk/blockdev_file.c
tool_test_partition_OBJS := $(OUT)/host-objects/fat12-28f3a4ae9c71.o
tool_test_partition_CFLAGS :=
tool_test_partition_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat_spc
tool_test_fat_spc_SRCS := tests/test_fat_spc.c tests/disk/blockdev_file.c
tool_test_fat_spc_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat_spc_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat_spc_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_subdir
tool_test_fat12_subdir_SRCS := tests/test_fat12_subdir.c tests/disk/blockdev_file.c
tool_test_fat12_subdir_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_subdir_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_subdir_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_truncate
tool_truncate_SRCS := tests/disk/truncate.c tests/disk/blockdev_file.c
tool_truncate_OBJS := $(OUT)/host-objects/int21-aed6415d133a.o $(OUT)/host-objects/devices-5bade66f83e6.o $(OUT)/host-objects/fileio_fat-8a832ed35366.o $(OUT)/host-objects/fat12-78f3a9da84a5.o $(OUT)/host-objects/sft-9b9f35447a01.o $(OUT)/host-objects/psp-5a73ec71d93c.o $(OUT)/host-objects/mcb-86d1b777b7f3.o $(OUT)/host-objects/irq-8000063a4aec.o
tool_truncate_CFLAGS := -no-pie
tool_truncate_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_write
tool_test_fat12_write_SRCS := tests/test_fat12_write.c tests/disk/blockdev_file.c
tool_test_fat12_write_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_write_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_write_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_write_partial
tool_test_fat12_write_partial_SRCS := tests/test_fat12_write_partial.c tests/disk/blockdev_file.c
tool_test_fat12_write_partial_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_write_partial_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_write_partial_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_mkdir
tool_test_fat12_mkdir_SRCS := tests/test_fat12_mkdir.c tests/disk/blockdev_file.c
tool_test_fat12_mkdir_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_mkdir_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_mkdir_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat16
tool_test_fat16_SRCS := tests/test_fat16.c tests/disk/blockdev_file.c
tool_test_fat16_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat16_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat16_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fileio
tool_test_fileio_SRCS := tests/test_fileio.c
tool_test_fileio_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_fileio_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fileio_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fpu_probe
tool_test_fpu_probe_SRCS := tests/test_fpu.c
tool_test_fpu_probe_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fpu_probe_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_rename
tool_test_fat12_rename_SRCS := tests/test_fat12_rename.c tests/disk/blockdev_file.c
tool_test_fat12_rename_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_rename_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_rename_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_idt
tool_test_idt_SRCS := tests/test_idt.c
tool_test_idt_OBJS := $(OUT)/host-objects/idt-5f5a63284280.o
tool_test_idt_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_idt_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_int21_dpb_host
tool_int21_dpb_host_SRCS := tests/int21_dpb_host.c tests/disk/blockdev_file.c
tool_int21_dpb_host_OBJS := $(OUT)/host-objects/int21-952950b0799a.o $(OUT)/host-objects/devices-bd2a4e967423.o $(OUT)/host-objects/fileio_fat-f97903a2ce53.o $(OUT)/host-objects/fat12-3027de855556.o $(OUT)/host-objects/sft-e21115eecc3f.o $(OUT)/host-objects/psp-1c1de08ad77c.o $(OUT)/host-objects/mcb-79410501c51a.o $(OUT)/host-objects/irq-9e5fd41d6656.o
tool_int21_dpb_host_CFLAGS := -D_POSIX_C_SOURCE=200809L -no-pie
tool_int21_dpb_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_int21_edge
tool_test_int21_edge_SRCS := tests/test_int21_edge.c
tool_test_int21_edge_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_int21_edge_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_int21_edge_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_int24
tool_test_int24_SRCS := tests/test_int24.c
tool_test_int24_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_int24_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_int24_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_int24_wired
tool_test_int24_wired_SRCS := tests/test_int24_wired.c tests/disk/blockdev_file.c
tool_test_int24_wired_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/ata-ca84e49b6568.o
tool_test_int24_wired_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_int24_wired_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_kbd
tool_test_kbd_SRCS := tests/test_kbd.c
tool_test_kbd_OBJS := $(OUT)/host-objects/kbd-2c85f4abb703.o $(OUT)/host-objects/pit-6c6c50e1cb62.o
tool_test_kbd_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_kbd_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_keep
tool_test_keep_SRCS := tests/test_keep.c
tool_test_keep_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_keep_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_keep_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_loader
tool_test_loader_SRCS := tests/test_loader.c
tool_test_loader_OBJS := $(OUT)/host-objects/loader-ef0d6d554ec7.o $(OUT)/host-objects/psp-451513b9615f.o
tool_test_loader_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_loader_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_loader_big
tool_test_loader_big_SRCS := tests/test_loader_big.c
tool_test_loader_big_OBJS := $(OUT)/host-objects/loader-ef0d6d554ec7.o $(OUT)/host-objects/psp-451513b9615f.o
tool_test_loader_big_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_loader_big_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_mkdir_nested
tool_test_fat12_mkdir_nested_SRCS := tests/test_fat12_mkdir_nested.c tests/disk/blockdev_file.c
tool_test_fat12_mkdir_nested_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_mkdir_nested_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_mkdir_nested_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fat12_mkdir_rollback
tool_test_fat12_mkdir_rollback_SRCS := tests/test_fat12_mkdir_rollback.c tests/disk/blockdev_file.c
tool_test_fat12_mkdir_rollback_OBJS := $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_fat12_mkdir_rollback_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_fat12_mkdir_rollback_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_mcb
tool_test_mcb_SRCS := tests/test_mcb.c
tool_test_mcb_OBJS := $(OUT)/host-objects/mcb-b67117320300.o
tool_test_mcb_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_mcb_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_mcb_int21
tool_test_mcb_int21_SRCS := tests/test_mcb_int21.c
tool_test_mcb_int21_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_mcb_int21_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_mcb_int21_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_mz
tool_test_mz_SRCS := tests/test_mz.c
tool_test_mz_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_mz_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_mzload
tool_test_mzload_SRCS := tests/test_mzload.c
tool_test_mzload_OBJS := $(OUT)/host-objects/loader-ef0d6d554ec7.o $(OUT)/host-objects/psp-451513b9615f.o
tool_test_mzload_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_mzload_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_native
tool_test_native_SRCS := tests/test_native.c
tool_test_native_CFLAGS :=
tool_test_native_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_overlay
tool_test_overlay_SRCS := tests/test_overlay.c
tool_test_overlay_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o $(OUT)/host-objects/loader-ef0d6d554ec7.o $(OUT)/host-objects/fat12-141c06111b76.o
tool_test_overlay_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_overlay_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_psp
tool_test_psp_SRCS := tests/test_psp.c
tool_test_psp_OBJS := $(OUT)/host-objects/psp-451513b9615f.o
tool_test_psp_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_psp_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_rtc
tool_test_rtc_SRCS := tests/test_rtc.c
tool_test_rtc_OBJS := $(OUT)/host-objects/rtc-b6fd0791ab5c.o
tool_test_rtc_CFLAGS := -D_POSIX_C_SOURCE=200809L -DRTC_HOST_TEST
tool_test_rtc_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_sft
tool_test_sft_SRCS := tests/test_sft.c
tool_test_sft_OBJS := $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o
tool_test_sft_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_sft_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_tsr_io
tool_test_tsr_io_SRCS := tests/test_tsr_io.c
tool_test_tsr_io_OBJS := $(OUT)/host-objects/kbd-11ae29d9aa4e.o $(OUT)/host-objects/console-da9be2570afc.o $(OUT)/host-objects/console_pixels-a0f969fa8512.o
tool_test_tsr_io_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_tsr_io_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_tsr
tool_test_tsr_SRCS := tests/test_tsr.c
tool_test_tsr_OBJS := $(OUT)/host-objects/int21-3e377762bc74.o $(OUT)/host-objects/devices-4e30ce85c494.o $(OUT)/host-objects/mcb-b67117320300.o $(OUT)/host-objects/sft-0056d38a60c7.o $(OUT)/host-objects/psp-451513b9615f.o $(OUT)/host-objects/irq-14fd69744e4b.o
tool_test_tsr_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_tsr_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_tenant_load
tool_test_tenant_load_SRCS := tests/test_tenant_load.c
tool_test_tenant_load_OBJS := $(OUT)/host-objects/loader-ef0d6d554ec7.o $(OUT)/host-objects/psp-451513b9615f.o
tool_test_tenant_load_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_tenant_load_DEPS := $(TOP)/build/kernel/dos_messages.h
export HOST_TEST_OUTPUT := $(abspath $(OUT))/host

# Preserve the untuned legacy host compilation (tests include large bounded models).
$(foreach t,$(EXTRA_HOST_TESTS),$(eval $(t)_CFLAGS += -O0))
$(OUT)/host-objects/int21-3e377762bc74.o: int21.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-4e30ce85c494.o: devices.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-b67117320300.o: mcb.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-0056d38a60c7.o: sft.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-451513b9615f.o: psp.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-14fd69744e4b.o: irq.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/config_sys-fd8ca8dc255d.o: config_sys.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/kbd-11ae29d9aa4e.o: kbd.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-f38962d47f22.o: int21.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-48f859b2cd9c.o: devices.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-f49eee7d9232.o: mcb.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-2df5f821c0ba.o: sft.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-cf47cac52a8f.o: psp.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-0a081605dfcd.o: irq.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/config_sys-f09076d85ba2.o: config_sys.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DINT21_WILDCARD_TESTSEAM -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-141c06111b76.o: fat12.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/ansi-5d041b26cee4.o: ansi.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/console-574c5ac43562.o: console.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/console_pixels-e495db98b8d6.o: console_pixels.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/ata-180cee39c418.o: ata.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DATA_HOST_TEST -DATA_POLL_LIMIT=16 -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-a14fe1ad186a.o: fileio_fat.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/block_device-32db98187ce4.o: block_device.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DBLOCK_DEVICE_HOST -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/config_sys-e23e2142c065.o: config_sys.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) $(tool_test_config_sys_CFLAGS) -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/console-da9be2570afc.o: console.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/console_pixels-a0f969fa8512.o: console_pixels.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-952950b0799a.o: int21.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-bd2a4e967423.o: devices.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-f97903a2ce53.o: fileio_fat.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-3027de855556.o: fat12.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-e21115eecc3f.o: sft.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-1c1de08ad77c.o: psp.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-79410501c51a.o: mcb.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-9e5fd41d6656.o: irq.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-28f3a4ae9c71.o: fat12.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-aed6415d133a.o: int21.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-5bade66f83e6.o: devices.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-8a832ed35366.o: fileio_fat.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-78f3a9da84a5.o: fat12.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-9b9f35447a01.o: sft.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-5a73ec71d93c.o: psp.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-86d1b777b7f3.o: mcb.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-8000063a4aec.o: irq.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/idt-5f5a63284280.o: idt.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/ata-ca84e49b6568.o: ata.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/kbd-2c85f4abb703.o: kbd.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/pit-6c6c50e1cb62.o: pit.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/loader-ef0d6d554ec7.o: loader.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/rtc-b6fd0791ab5c.o: rtc.c $(TEST_HDRS) $(MAKEFILE_LIST) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -DRTC_HOST_TEST -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
-include $(wildcard $(OUT)/host-objects/*.d)
TEST_HDRS += $(wildcard *.c) $(wildcard $(TOP)/kernel/*.c) $(wildcard command/*.c) $(wildcard $(TOP)/dos/command/*.c)

export HOST_TEST_SOURCE_ROOT := $(abspath $(TOP))
export HOST_TEST_COMPONENT := kernel

EXTRA_HOST_TESTS += tool_dos_safety_fixture
tool_dos_safety_fixture_SRCS := tests/disk/dos_safety_fixture.c
tool_dos_safety_fixture_CFLAGS := -O0 -no-pie

EXTRA_HOST_TESTS += tool_test_dos_safety
tool_test_dos_safety_SRCS := tests/test_dos_safety.c int21.c devices.c fileio_fat.c fat12.c tests/disk/blockdev_file.c sft.c psp.c mcb.c irq.c
tool_test_dos_safety_CFLAGS := -O0 -no-pie

EXTRA_HOST_TESTS += tool_dos_parity_host
tool_dos_parity_host_SRCS := $(TOP)/dos/command/tests/dos_parity_host.c int21.c devices.c fileio_fat.c fat12.c tests/disk/blockdev_file.c sft.c psp.c mcb.c irq.c
tool_dos_parity_host_CFLAGS := -O0 -no-pie

EXTRA_HOST_TESTS += tool_dosmsg_scan
tool_dosmsg_scan_SRCS := tests/dosmsg_scan.c

EXTRA_HOST_TESTS += tool_test_dos_break
tool_test_dos_break_SRCS := tests/test_dos_break.c int21.c devices.c mcb.c sft.c psp.c irq.c kbd.c
tool_test_dos_break_CFLAGS := -O0 -no-pie

EXTRA_HOST_TESTS += tool_test_dos_not_ready
tool_test_dos_not_ready_SRCS := tests/test_dos_not_ready.c int21.c devices.c mcb.c sft.c psp.c irq.c ata.c tests/disk/blockdev_file.c
tool_test_dos_not_ready_CFLAGS := -O0

EXTRA_HOST_TESTS += tool_test_redir
tool_test_redir_SRCS := tests/test_redir.c int21.c devices.c mcb.c sft.c psp.c irq.c
tool_test_redir_CFLAGS := -O0

EXTRA_HOST_TESTS += tool_overlay_dos33_check tool_overlay_dos33_negative
tool_overlay_dos33_check_SRCS := tests/fixtures/dos33_overlay/check.c
tool_overlay_dos33_negative_SRCS := tests/fixtures/dos33_overlay/check.c
tool_overlay_dos33_negative_CFLAGS := -DMUTATE_EXPECT_LOAD_PATCH

# Generated catalogue must exist before any parallel host compilation.
$(foreach t,$(EXTRA_HOST_TESTS),$(eval $(t)_DEPS += $(TOP)/build/kernel/dos_messages.h))
