#!/bin/sh
# Copy a staging directory into an existing FAT image.
set -eu
source_tree=$1 image=$2
export LC_ALL=C MTOOLS_SKIP_CHECK=1
copy_directory() (
 source_dir=$1 destination=$2
 for entry in "$source_dir"/*; do
  [ -e "$entry" ] || continue
  target=$destination/${entry##*/}
  if [ -d "$entry" ]; then
   mmd -i "$image" "$target"
   copy_directory "$entry" "$target"
   # mmd uses SOURCE_DATE_EPOCH; file mtimes below use the fixed staging date.
  else
   mcopy -m -i "$image" "$entry" "$target"
  fi
 done
)
copy_directory "$source_tree" ::
