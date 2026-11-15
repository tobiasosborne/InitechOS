# Sourced from the repository root by the installed-system test drivers.
# Accepted GM8 layout; mtools independently reads the foreign FAT directory.
boot=${SYSTEM_BOOT_IMAGE:-build/image/boot.img}
data=${SYSTEM_DATA_IMAGE:-build/image/data.img}
drive=A
volume=$data
if [ -n "${SYSTEM_WORK_IMAGE:-}" ]; then
 volume=$SYSTEM_WORK_IMAGE
 if [ "$volume" = "$boot@@32256" ]; then drive=C; else data=$volume; fi
else
 root_listing=$(mdir -a -b -i "$boot@@32256" ::) || {
  echo 'ERROR system volume: cannot read the boot partition' >&2; exit 2;
 }
 if printf '%s\n' "$root_listing" | grep -Eiq '(^|/)COMMAND\.COM$'; then
  drive=C
  volume=$boot@@32256
 else
  reader_rc=$?
  if [ "$reader_rc" != 1 ]; then
   echo 'ERROR system volume: directory reader failed' >&2; exit 2
  fi
 fi
fi
# Temporary legacy A selection above expires with canonical C image activation.
