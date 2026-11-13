# Test programs are built here but installed only on the separate smoke disk.
SMOKE_PROGRAMS := MULTI FCBX NEST GREET TERM20 DEPTH CONIN SERVICES WRITE
smoke-programs: $(addprefix $(OUT)/test-programs/,$(addsuffix .COM,$(SMOKE_PROGRAMS)))
define test_program
$(OUT)/test-programs/$(1).COM: $(TOP)/kernel/tests/programs/$(2).asm
	@mkdir -p $$(@D)
	$(NASM) -f bin $(3) $$< -o $$@
endef
$(eval $(call test_program,MULTI,multiopen_program))
$(eval $(call test_program,FCBX,fcbx_program))
$(eval $(call test_program,NEST,nested_exec_program))
$(eval $(call test_program,TERM20,nested_exec_program,-DNEST_TERM20))
$(eval $(call test_program,DEPTH,nested_exec_program,-DNEST_DEPTH))
$(eval $(call test_program,GREET,greet_program))
$(eval $(call test_program,CONIN,conin_program))
$(eval $(call test_program,SERVICES,services_program))
$(eval $(call test_program,WRITE,write_program))
