# Gate / mutant ordering for parallel vectors (initech-1x6x2.1).
#
# A gate and its mutant leg(s) re-mint and mutate the SAME scratch: fixed
# build/ images, work directories, serial/report names. Under one parallel
# make (test-par, test-merge-samir) they interleave and either side goes
# falsely RED or GREEN -- measured in a cold `make -j32 test-par`:
# test-b53d-mutant "NO_FLUSH mutant PASSED", test-chkdsk-emu(-mutant),
# test-block-device-emu(-mutant), test-bochs-legs(-mutant), ... each green
# when run alone. Every <gate>-mutant / <gate>-mutants in the vector therefore
# runs after <gate>: the ordering most mutant legs already declare by hand
# (a mutation proof is only meaningful once its gate is green, Rule 6).
#
# This file must stay the LAST include of the Makefile: GATE_VECTOR is read
# here, after every TEST_UNIT_GATES / TEST_EMU_GATES += of every fragment.
GATE_VECTOR := $(sort $(TEST_UNIT_GATES) $(TEST_EMU_GATES))
$(foreach g,$(GATE_VECTOR),$(foreach m,$(filter $(g)-mutant $(g)-mutants,$(GATE_VECTOR)),$(eval $(m): $(g))))
# An emulator mutant of a gate that has no separate -emu leg shares the gate's
# scratch the same way (test-conkeys-emu-mutant vs test-conkeys: both drive
# conkeys.sh into build/conkeys/ under the harness name "conkeys").
$(foreach g,$(GATE_VECTOR),$(if $(filter $(g)-emu,$(GATE_VECTOR)),,$(foreach m,$(filter $(g)-emu-mutant,$(GATE_VECTOR)),$(eval $(m): $(g)))))
