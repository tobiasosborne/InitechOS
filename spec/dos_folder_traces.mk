# initech-zc68 / 2kgh, locked before the data-volume change (Rule 1/8).
# Ref: PRD 6.1/6.5; ADR-0003 DEC-11, Appendix A 3Dh/3Fh/40h/4Bh/4Eh.
# MS-DOS 3.3 User's Guide printed pp.94,97,135 (PDF179,182,220):
# MORE < source, PATH, AUTOEXEC (rendered scans, Law 1).
# Root order README, APPS, DOS, AUTOEXEC: cell 2 centre (191,102).
# From (600,64): sum -409,+38; every component <=100. DOS opens slot 1.
DOS_FOLDER_FINDER_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:9,m-100:9,m-9:0,l1,l0,l1,l0
# No date/time Enters: AUTOEXEC exists (manual p.135). Wait on the shell,
# then inject the locked literal commands through the existing keys encoder.
DOS_FOLDER_KEYS = $(shell sh pubs/screens/keys.sh "$$(cat spec/dos_folder_commands.txt)")
DOS_FOLDER_EXIT_SPEC = $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,k$(subst $(comma),$(comma)k,$(DOS_FOLDER_KEYS))
