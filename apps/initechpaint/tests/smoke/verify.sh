#!/bin/sh
set -eu
out=$2
case $out in *qemu-*)
 make -s -C apps/initechpaint "$PWD/build/apps/initechpaint/host/pixels"
 build/apps/initechpaint/host/pixels canvas "${out%qemu-*}canvas.ppm"
 ;; esac
