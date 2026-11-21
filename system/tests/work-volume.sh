# Sourced from the repository root. The staged filenames and legacy A:
# fallback expire when the C: pair becomes canonical (activation step 20).
boot=${SYSTEM_BOOT_IMAGE:-build/image/c.img}
data=${SYSTEM_DATA_IMAGE:-build/image/a-data.img}
if [ -n "${SYSTEM_WORK_IMAGE:-}" ]; then
 volume=$SYSTEM_WORK_IMAGE
else
 root_listing=$(mdir -a -b -i "$boot@@32256" ::) || {
  echo 'ERROR system volume: cannot read the boot partition' >&2; exit 2;
 }
 # COMMAND.COM marks the installed C: layout during the transition.
 if printf '%s\n' "$root_listing" | grep -Eiq '(^|/)COMMAND\.COM$'; then
  volume=$boot@@32256
 else
  reader_rc=$?
  if [ "$reader_rc" != 1 ]; then
   echo 'ERROR system volume: directory reader failed' >&2; exit 2;
  fi
  volume=$data
 fi
fi
