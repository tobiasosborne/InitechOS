# Target mutation proofs

Tier 2 only. `make target-mutants` first rejects the no-fault and compiler-error
controls, then proves each patch twice in isolated component copies. Baseline
packaging and the 50 pair checks must pass. All baseline target objects must be
present for a static rejection to count; a generic build failure earns no kill.

- cross: the target linker rejects REPORT referencing FILE's ndx_rebuild.
- resref: the target linker rejects resident code referencing REPORT's rp_run.
- reenter: the ELF call-graph checker rejects an overlay reaching ibase_do and
  the overlay manager, which could evict its own return address.

The oracle is the overlay lifetime property described in README.md and
boot/ibase.ld. The linker is independent of the product; the packer traverses
ELF relocation edges. These three deliberate violations cannot be shipped.
No crash or timeout is accepted as evidence. Logs retain the exact diagnostics.
