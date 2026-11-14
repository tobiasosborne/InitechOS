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
endif
include $(TOP)/mk/tenant.mk
