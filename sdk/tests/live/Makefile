TOP := $(abspath ../../..)
COMPONENT := sdk
include $(TOP)/mk/config.mk
TENANT := sdkcheck
TENANT_EXE := SDKCHECK.EXE
TENANT_SRCS := tests/live/tenant.c
ifneq ($(ABI),)
TENANT := sdkabi
TENANT_EXE := SDKABI.EXE
TENANT_SRCS := tests/live/abi.c
else ifneq ($(FILE_DRIVE),)
TENANT := sdkdrive
TENANT_EXE := SDKDRIVE.EXE
TENANT_SRCS := tests/live/file-drive.c
endif
include $(TOP)/mk/tenant.mk
