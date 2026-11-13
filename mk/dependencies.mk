# Authoritative build dependencies. `make dependencies` prints this graph.
# READMEs describe the contracts; build ordering and audits use these edges.
DEPENDS_boot := kernel
DEPENDS_kernel := dos
DEPENDS_dos := kernel
DEPENDS_flair := flair/region sdk
DEPENDS_flair/desktop := kernel flair flair/region sdk
DEPENDS_sdk := factory/mz flair
DEPENDS_apps/initech123 := sdk factory/mz
DEPENDS_apps/initechword := sdk factory/mz
DEPENDS_apps/calculator := sdk factory/mz
DEPENDS_apps/notepad := sdk flair flair/region factory/mz
DEPENDS_apps/initechtext := sdk flair flair/region factory/mz
DEPENDS_apps/controlpanels := sdk flair flair/region factory/mz
DEPENDS_apps/initechcalc := sdk apps/initech123 factory/mz
DEPENDS_apps/initechpaint := sdk flair apps/initechcalc factory/mz
DEPENDS_apps/fonts := sdk factory/mz
DEPENDS_apps/ctenant := sdk factory/mz
DEPENDS_apps/initechbase := kernel factory/mz
DEPENDS_turbo := turbo/seed turbo/compiler kernel dos factory
DEPENDS_turbo/seed := factory dos
DEPENDS_turbo/compiler := turbo/seed factory/mz
DEPENDS_system := kernel dos flair flair/desktop flair/region sdk apps/initech123 factory
DEPENDS_factory := boot kernel dos system apps/initech123
DEPENDS_factory/mz := kernel sdk
# The resident shell/catalogue cycle is built by the component recipes.
# SDK's ABI source audit reads FLAIR source, not FLAIR build outputs.
TEST_DEPENDS_kernel :=
TEST_DEPENDS_sdk :=
TEST_DEPENDS_turbo := kernel dos sdk
TEST_DEPENDS_system := kernel dos flair flair/desktop sdk apps/initech123
