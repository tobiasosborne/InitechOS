# Same override/default resolution as components, including command-line vars.
TOP := $(abspath .)
COMPONENT := goldens
include $(TOP)/mk/config.mk
export SYSTEM7_DECOMP WIN31_DECOMP DBASE3_DECOMP LOTUS123_DECOMP WORDPERFECT51_DECOMP TURBOPASCAL_DECOMP
.PHONY: verify
verify:
	@sh $(TOP)/goldens/verify.sh
