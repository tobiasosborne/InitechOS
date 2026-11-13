* MINT-ERRORS:
* Ref: ../dbase3-decomp/specs/commands/navigation-query-display.md:765
* III+ returns None for invalid drive/path; the probes raise no ON ERROR code.
? "[W2-BEGIN]"
? "[DEFAULT DATABASE FILES]"
DIR
? "[SKELETON FILTER]"
DIR M*.DBF
? "[DIFFERENT DIRECTORY]"
DIR C:\DATA\*.DBF
? "[NO MATCHING FILE]"
DIR NOSUCH*.DBF
? "[TRAILING WORDS PROBE]"
DIR BAD OPERAND
? "[INVALID DRIVE]"
DIR Q:\*.DBF
? "[NONEXISTENT DIRECTORY]"
DIR C:\NOSUCH\*.DBF
? "[W2-END]"
RETURN
