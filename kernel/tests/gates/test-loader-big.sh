set -eu
printf '>>> test-loader-big: FAT in-place loader -- >64 KiB .COM loads (PROGRAM_IMAGE_MAX sole bound; za4m)\n'
build/test_loader_big
