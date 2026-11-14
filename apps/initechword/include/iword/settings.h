/* LOCKED InitechWord settings record, initech-qvgd, WL-0146 Part 2.
 * PM third-launch authority: an Initech spec record, NOT WP{WP}.SET bytes.
 * Navigation/names: WP Reference pp. 42, 160, 574-575, 698-700.
 * I/O: ADR-0003 Appendix A 3Ch..40h; IBM DOS 3.30 Technical Reference
 * 6-131..6-140. Data volume root, eight bytes, no host path or timestamp.
 * 0..3 "IWS1"; 4 entry units; 5 status units; 6 original-backup 0/1;
 * 7 reserved zero. Status: i/c/p/w/u, or the default inch mark '"'
 * (Reference pp. 699-700 and raw status-units captures). Entry is fixed i;
 * legacy valid entry choices load without effect and normalize on save. Invalid or
 * non-exact-length records use defaults. This is preference persistence;
 * timed backup is unavailable in the minimal P3 Setup menu. */
#ifndef IWORD_SETTINGS_H
#define IWORD_SETTINGS_H
#define IW_SETTINGS_PATH "IWORD.SET"
#define IW_SETTINGS_SIZE 8u
#define IW_SETTINGS_MAGIC "IWS1"
#define IW_SETTINGS_DEFAULT_UNIT 'i'
#endif
