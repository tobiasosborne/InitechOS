# attic

Legacy material that has not been claimed by a component yet: the old
test harness (`harness/`), the locked specs (`spec/`), fixtures, tools
and the manuals (`pubs/`).

- Nothing here is built. Do not add a build that reaches into it.
- Claim what your component needs with `git mv` into the component (or
  into `goldens/`, `factory/` or `sdk/`). Do not edit files in place.
- Makefile fragments (`*.mk`) here are reference only. They describe the
  retired factory. Read them to learn what a test needs; do not port
  their structure.
- Whatever is still here when the import is finished is deleted.

The complete legacy tree, including its Makefile, is at the tag
`legacy-final` (checked out at `../InitechOS-legacy`).
