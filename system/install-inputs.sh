#!/bin/sh
# Only source-owned inputs; neither the read-only legacy tree nor attic is read.
set -eu
out=$1
mkdir -p "$out"
(cd samples && sha256sum -c SHA256SUMS >/dev/null)
"$2" "$out"
for file in HELLO.PAS Y2KACCT.PRG README.TXT; do
 awk '{sub(/\r$/, ""); printf "%s\r\n", $0}' "samples/$file" > "$out/$file"
done
cp "$out/HELLO.PAS" "$out/TURBOIN.PAS"
cp samples/TPS.DBF samples/TPSPARM.DAT "$out/"
cp ../turbo/compiler/turbo.pas "$out/TURBO.PAS"
cp ../flair/desktop/data/DESKTOP.DB "$out/DESKTOP.DB"
cp ../apps/initechcalc/data/LEDGER.TXT ../apps/initechcalc/data/MICHAEL.CFG "$out/"
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\nDEVICE=ANSI.SYS\r\n' > "$out/CONFIG.SYS"
printf '%s\r\n' '@ECHO OFF' 'PATH A:\;A:\APPS' 'FLAIR' > "$out/AUTOEXEC.BAT"
# Approved GM8 delivery design: boot to the prompt, then type FLAIR.
# The separate c-boot directory expires at C: activation (plan step 20).
mkdir -p "$out/c-boot"
cp "$out/CONFIG.SYS" "$out/c-boot/CONFIG.SYS"
printf '%s\r\n' '@ECHO OFF' 'PATH C:\;C:\APPS' 'SET COMSPEC=C:\COMMAND.COM' 'PROMPT $P$G' > "$out/c-boot/AUTOEXEC.BAT"
printf 'Hello from InitechDOS.\r\n' > "$out/HELLO.TXT"
