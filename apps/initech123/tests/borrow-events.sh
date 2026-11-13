#!/bin/sh
# TEMPORARY SDK import bridge. Keep the shared EventRecord and its original
# Point typedef, without grafport.h's private drawing implementation includes.
# Remove this when sdk exports a self-contained public event header.
set -eu
spec=$1
printf '#include <stdint.h>\n#include <stddef.h>\n'
sed -n '/^typedef struct flair_point {/,/^} flair_point_t;/p' "$spec/grafport.h"
sed '/^#include "grafport.h"/d' "$spec/event_model.h"
