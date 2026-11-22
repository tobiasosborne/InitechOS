# Source from the repository root in a test driver using set -e.
# media_prepare stages disposable copies from an already-built distribution.
# Staged names and detection of the legacy A: system expire at cutover (step 20).
media_prepare() {
 case $1 in
  staged) media_master_name=c.img; media_slave_name=a-data.img;;
  canonical) media_master_name=boot.img; media_slave_name=data.img;;
  *) echo "Unknown media pair: $1" >&2; return 2;;
 esac
 media_dir=$(mktemp -d "$3/media.XXXXXX") || return 2
 media_boot=$media_dir/master.img
 media_data=
 cp "${5:-${4:-build/image}/$media_master_name}" "$media_boot" || return 2
 case $2 in
  with-data) media_data=$media_dir/slave.img
   cp "${4:-build/image}/$media_slave_name" "$media_data" || return 2;;
  alone) ;;
  *) echo "Unknown ATA attachment: $2" >&2; return 2;;
 esac
 # COMMAND.COM is the installed-system marker used by system/tests/work-volume.sh.
 mdir -a -b -i "$media_boot@@32256" ::/ > "$media_dir/master.list" || return 2
 if grep -Fxiq '::/COMMAND.COM' "$media_dir/master.list"; then
  media_system=C
  media_work=$media_boot@@32256
 elif [ "$1" = canonical ] && [ -n "$media_data" ]; then
  media_system=A
  media_work=$media_data
 else
  echo 'Missing installed system on master' >&2; return 2
 fi
}

# media_run EMULATOR DRIVER [driver options]; caller owns stdout/stderr.
media_run() (
 emulator=$1 driver=$2; shift 2
 [ -z "$media_data" ] || set -- --disk2 "$media_data" "$@"
 case $emulator in
  qemu) set -- --boot-rw "$@";;
  bochs) ;;
  *) echo "Unknown emulator: $emulator" >&2; exit 2;;
 esac
 exec "$driver" --disk "$media_boot" "$@"
)

# media_readback EMULATOR REPORT. media_volume is a raw FAT file suitable
# for both fsck.fat and mtools. Bochs publishes the path of its written master.
media_readback() {
 case $1 in
  qemu) media_written=$media_boot;;
  bochs) media_written=$(sed -n 's/^\[bochs\] private_boot=//p' "$2");;
  *) echo "Unknown emulator: $1" >&2; return 2;;
 esac
 [ -f "$media_written" ] || { echo 'Missing written master' >&2; return 2; }
 if [ "$media_system" = C ]; then
  media_volume=$media_dir/system.fat
  dd if="$media_written" of="$media_volume" bs=512 skip=63 count=16384 status=none || return 2
  [ "$(wc -c < "$media_volume")" -eq 8388608 ] || { echo 'Short system partition' >&2; return 2; }
 else
  media_volume=$media_data
 fi
}
