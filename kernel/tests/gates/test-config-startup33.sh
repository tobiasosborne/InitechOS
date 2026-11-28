set -eu
. kernel/tests/assertions.sh
truncate -s 1474560 build/default.img
mformat -i build/default.img -f 1440 ::
: > build/COMMAND.COM
mcopy -i build/default.img build/COMMAND.COM ::
failed=0
while read -r scenario state; do
 cp build/default.img build/config.img
 : > build/expected.txt
 ansi=0
 case $scenario in
  absent) ;;
  empty) : > build/CONFIG.SYS;;
  blank) printf '\r\n' > build/CONFIG.SYS;;
  eof) printf 'FILES=5\r\n\032\r\nLASTDRIVE=Z\r\n' > build/CONFIG.SYS;;
  longconfig)
   awk 'BEGIN { for (i=0;i<512;i++) printf "\r\n" }' > build/CONFIG.SYS
   printf 'BREAK=OFF\r\nFILES=5\r\nLASTDRIVE=Z\r\n' >> build/CONFIG.SYS;;
  config65533|config65534)
   size=${scenario#config}
   head -c "$((size - 22))" /dev/zero | tr '\000' '\n' > build/CONFIG.SYS
   printf 'FILES=5\r\nLASTDRIVE=Z\r\n' >> build/CONFIG.SYS
   if [ "$scenario" = config65534 ]; then
    printf 'Unrecognized command in CONFIG.SYS\n' > build/expected.txt
   fi;;
  unknown-next)
   printf 'NOEQUALS\r\nFILES=5\r\n' > build/CONFIG.SYS
   printf 'Unrecognized command in CONFIG.SYS\n' > build/expected.txt;;
  cr) printf 'FILES=5\rBREAK=ON\rLASTDRIVE=Q\r' > build/CONFIG.SYS;;
  lf) printf 'FILES=5\nBREAK=ON\nLASTDRIVE=Q\n' > build/CONFIG.SYS;;
  nodriver)
   printf 'DEVICE=ANSI.SYS /Z\r\n' > build/CONFIG.SYS
   printf 'Bad or missing ANSI.SYS\n' > build/expected.txt;;
  driver)
   printf 'DEVICE=ANSI.SYS\r\n' > build/CONFIG.SYS
   printf 'ANSI driver fixture\r\n' > build/ANSI.SYS
   mcopy -i build/config.img build/ANSI.SYS ::
   ansi=1;;
  *) exit 2;;
 esac
 if [ "$scenario" != absent ]; then
  mcopy -i build/config.img build/CONFIG.SYS ::
 fi
 cp build/config.img build/before.img
 if ! build/test_config_apply build/config.img "state:$state:$ansi" > build/console.raw; then
  printf 'FAIL %s state\n' "$scenario" >&2
  failed=1
 fi
 tr -d '\r' < build/console.raw > build/console.txt
 if ! diff -u build/expected.txt build/console.txt; then
  printf 'FAIL %s console\n' "$scenario" >&2
  failed=1
 fi
 assert cmp build/before.img build/config.img
done < kernel/tests/fixtures/config33/startup-state.txt
test "$failed" -eq 0
finish
