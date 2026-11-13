set -eu
truncate -s 1474560 build/data.img
mformat -i build/data.img -f 1440 -v INITECH ::
build/test_dir_disk --fixture build/data.img build/dir_label_root.img root
build/test_dir_disk --fixture build/data.img build/dir_label_boot.img boot
build/test_dir_disk --fixture build/data.img build/dir_label_none.img none
build/test_dir_disk --label build/dir_label_root.img 'ROOT LABEL'
build/test_dir_disk --label build/dir_label_boot.img 'BOOT LABEL'
build/test_dir_disk --label build/dir_label_none.img ''
