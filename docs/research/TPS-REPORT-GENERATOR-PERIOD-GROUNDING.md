<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->
<!-- Research brief: TPS Report Generator (TPSREP.COM) and its Operating Procedures binder. -->
<!-- Law 1: every claim cites a local page, a fetched URL, or is flagged UNSOURCED. -->
<!-- NO code and NO locked spec is changed by this document (Rule 8). -->

# TPS Report Generator -- Period Grounding Brief

**Issuing Body:** Initech Systems Corporation -- Publications / Platform Engineering
**Document Class:** Research Brief (no code; options for the operator's decision)
**Bead:** initech-9yw6 (blocks initech-5igf, initech-ipu5.14)
**Last Reconciled:** 2026-10-17

> Incoming agent: this brief does not authorise a spec change. The five rituals
> in `spec/tps_ritual.json` stay as shipped until the operator picks options
> (Section 3) and a Rule-8 issue + worklog note exists.

Source tags used below.

| Tag | Source | Where |
|---|---|---|
| ZR23 | IBM PC DOS Operation Training, ZR23-6845-01, First Edition March 1989 | local `initech-manuals-ref/dos33/` |
| DW4 | IBM DisplayWrite 4 Getting Started, 74X9319, Aug 1986 | local `packaging/` |
| UDB | Using dBASE III PLUS (Ashton-Tate 1986), page ids as printed (U5-120 etc.) | local `dbase3plus/` |
| LDB | Learning dBASE III PLUS, ditto (L6-n) | local `dbase3plus/` |
| L123 | Lotus 1-2-3 Release 3 Reference Manual (1989), text layer, printed pages 2-nnn | local `lotus123/Lotus_123_R3_Reference_Manual_1989_text.pdf` |
| WP51 | WordPerfect 5.1 Reference, printed pages | local `wordperfect51/WPDOS51Reference.pdf` |
| D310 | IBM DOS 3.10 Reference (Feb 1985), Appendix A messages | local `dos33/` |
| M33 | MS-DOS 3.3 User's Reference (July 1987) | local `dos33/` |
| 4230 | IBM 4230 Printer User's Guide SA40-0593-04 (5th ed. Aug 1994; (C) 1991, 1994) | URL, Section 1.3 |

---

## 0. Honest limits (read first)

1. Not retrieved: the IBM 3800 / 3211 / 4245 / 4248 operator's guides (GA26-1634
   is named by a 1978 3800 reference but its text was not fetched); the exact
   text of message IEC128D; any documentation of a `REPLY nn,U` response to an
   FCB message (the brief's premise). What IS sourced is that IEC128D exists,
   what it asks, that CANCEL is its documented reply, and IEC125D's repeat-reply
   text. No wording below pretends otherwise; unsourced wording is marked
   (HOUSE).
2. Paradox, Clarion and R&R printing prompts were searched and not found in any
   retrievable source. They are NOT used as authority. Only the existence and
   lineage of R&R is cited (Section 1.2).
3. The Lotus Release 2.2 Reference in `initech-manuals-ref` is an image scan with
   no text layer; the Release 3 text (1989) is cited instead. The /Print Align
   behaviour is the same family; the R2.2 page was not viewed.
4. Corporate "memo control number" and "see your supervisor" language has no
   retrieved primary source beyond the IBM training booklet's coordinator
   language (ZR23) and the film itself. Memo-number controls are (HOUSE).

## 1. Sourced survey

### 1.1 Mainframe and mini side: forms, FCB, carriage control, operator replies

Findings, each with its source.

- **Carriage control tape.** A punched-tape loop as long as the page; channel 1
  marks top of form; "a new form is started with a skip to channel 1"; channel 9
  sits a few lines above the bottom, channel 12 marks the last printable line;
  a manual skip existed: "a form feed switch on the printer control panel allowed
  a manual skip to the top of the page." Tapes "could be easily changed when new,
  continuously fed forms were loaded."
  https://en.wikipedia.org/wiki/Carriage_control_tape
- **FCB.** Starting with the 3211 the tape became a "forms control buffer, an
  electronic image of the physical carriage control tape"; a job may request an
  FCB which is loaded before printing "eliminating operator intervention".
  Same URL.
- **FCB images under JES2/OS.** The image fixes lines per inch and form length.
  Standard 3211 images: STD1 (6 lpi, 8.5 in), STD2 (6 lpi, 11 in); STD3
  (8 lpi, 11 in) is 3800-only. FCB images live in SYS1.IMAGELIB; an image used
  with JES2 "must specify channel 1 in position 1; otherwise, JES2 positions the
  forms in the printer incorrectly." $HASP180 reports an FCB image load error.
  https://www.ibm.com/docs/en/SSLTBW_2.4.0/com.ibm.zos.v2r4.hasa400/has2u600176.htm
  https://www.ibm.com/docs/zh/SSLTBW_2.2.0/com.ibm.zos.v2r2.hasa200/apt.htm
  https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.2.0/com.ibm.zos.v2r2.ieab600/xotfcb.htm
- **Operator verification of the buffer (SETPRT).** For a printer with no carriage
  tape the program can "select the FCB, request operator verification of the
  contents of the buffer, and allow the operator to align the paper"; if the
  image is not found "the operator is asked to specify an alternate image name or
  cancel the request."
  https://www.ibm.com/docs/SSLTBW_2.1.0/com.ibm.zos.v2r1.idad400/idsetprt.htm
- **IEC128D, IEC163A, IEC164A (3800 SETPRT).** IEC128D asks the operator to verify
  forms alignment to the forms control buffer image (the image id is one to four
  characters from the FCB operand); IEC163A asks for a forms overlay frame;
  IEC164A asks for a change of paper threading. Replying CANCEL yields
  `IEC172I dev OPERATOR CANCELED text REQUEST` with text `FCB imageid VERIFY`,
  `FORMS OVERLAY` or `RETHREAD`; operator response: rerun the job if the cancel
  was a mistake, else "notify the programmer."
  https://www.ibm.com/support/knowledgecenter/SSLTBW_2.2.0/com.ibm.zos.v2r2.ieam700/m010652.htm
- **IEC125D.** `IEC125D ERROR - REPEAT REPLY`: an incorrect response to a UCS or FCB
  message; operator response "Re-enter the reply, making sure it is correct."
  https://www.ibm.com/support/knowledgecenter/SSLTBW_2.2.0/com.ibm.zos.v2r2.ieam700/m010621.htm
- **Dynamic allocation FCB check.** DALFCBAV prompts the operator "to check the
  alignment of the printer forms before the data set is printed"; PARM X'04' =
  VERIFY (visually confirm the image shown), X'08' = ALIGN.
  https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.ieaa800/fcbalvr.htm
- **WTOR.** A write-to-operator-with-reply is answered by `REPLY id,text` (verb
  optional); reply ids are assigned sequentially; `D R,L` lists outstanding
  requests. A reply of "U" is NOT confirmed by any retrieved IBM page.
  https://www.ibm.com/docs/SSLTBW_3.1.0/com.ibm.zos.v3r1.ieag100/reply.htm
- **JES2 job identity and messages.** `$HASP100 JOB ON READER` is the job's
  acceptance message and may carry the programmer name from the JOB card;
  messages are documented with the $HASP prefix, an asterisk marking "a request
  for operator action." (Job-number format JOBnnnnn is general JES2 knowledge,
  not fetched.)
  https://www.ibm.com/docs/en/SSLTBW_2.2.0/com.ibm.zos.v2r2.hasa100/msgfor.htm
- **IBM i analogue.** `CPA4002 Verify alignment on printer`, issued only for
  continuous-form writers (FORMFEED *CONT). Reported by search result; page not
  fetched. (Used as wording colour only.)

### 1.2 PC side, 1986-1992

- **dBASE III PLUS EJECT.** "EJECT issues a form feed (ASCII character value of
  12) to the printer. For proper printer operation, the paper must initially be
  set to the top-of-form. Refer to your printer manual for instructions on how to
  set top-of-form." Also warns EJECT on a dead printer "may 'lock' the computer."
  UDB p. U5-120.
- **dBASE III PLUS LABEL FORM ... SAMPLE.** "Use the SAMPLE option to print test
  labels to ensure proper alignment of the labels in the printer. The process may
  be repeated as many times as necessary by responding Y when asked `Do you want
  more samples?` When you respond with N, labels begin printing." UDB p. U5-143.
  This is the closest PC match to the operator ritual: print a test, adjust,
  repeat until accepted.
- **Report options.** Lines per page default 58 (range 1-500); "Page eject before
  printing" default YES; the report "starts printing wherever the paper is
  aligned" if NO; "Page eject after printing" default NO. LDB Table 6-1 (p. L6-7);
  UDB pp. U5-75, U5-76. 58 lines on a 66-line page leaves 8 lines of margin.
- **Printer faults.** `Printer not ready. Retry? (Y/N)`; `Printer is either not
  connected or turned off.` (error 126); SET PRINTER TO selects LPT1-3, COM1-2
  (default LPT1). UDB error appendix; SET PRINTER TO p. U5-252.
- **Lotus 1-2-3 /Print Align.** "/Print [E,F,P] Align ensures that the headers,
  footers, graphs, and page breaks will be in the correct spot when you print ...
  resets the page number to 1 and tells 1-2-3 that it is beginning to print at the
  top of a page." Procedure: "If you are printing on a printer, position the
  printer paper at the top of a page" then select Align. Skipping it "may put page
  breaks in the wrong place, leaving a gap in the middle of the printed page."
  After /Print Cancel: "Before you print again, realign the paper in the
  printer." Wait Yes prompts "to insert the next sheet of paper" then /Print
  Resume. L123 pp. 2-201 (prepare device: printer on, paper aligned at top of
  page), 2-208 (Cancel), 2-210 (Align).
- **WordPerfect 5.1 forms.** Paper Size/Type defines each form (letterhead,
  labels, envelopes), its location (Continuous = tractor-fed, Bin Number, Manual)
  and "Prompt to Load": the computer beeps and a message tells the operator to
  insert the paper on the Printer Control screen; "Insert the paper, then type g
  to continue printing." WP51 pp. 478-485 (Paper Size/Type; Prompt to Load p. 485);
  error appendix p. 751 `Insert Paper-Press "G" to Continue`; Printing, Stop
  p. 542: "reset the forms in the printer (advance the paper to the top of the
  next page), then select Go (Start Printer)."
- **PRINT.COM queue.** First use asks `Name of list device [PRN]:` and prints
  `Resident part of PRINT installed` (about 3200 bytes of memory); `Print queue
  is empty`; `Print queue is full` (limit 10 files, 32 with /Q); MODE reports
  `Printer error` (I/O error, out of paper or POWER OFF, time-out, offline).
  D310 pp. A-81 (queue/resident), A-7 (`No paper`: "Turn the printer ON, press
  the ONLINE switch, or add paper and retry"); M33 `PRINT queue is full`, `Name of
  list device [PRN]:` and `No paper error writing device dev` (message appendix,
  PDF pp. 431, 438).
- **R&R.** Concentric Data Systems (founded 1979) sold the R&R report writer line
  for the IBM PC, working with dBase and later FoxBase/FoxPro. Lineage only; no
  printing behaviour sourced. https://en.wikipedia.org/wiki/Concentric_Data_Systems
- **Physical printer top-of-form (the real "tacit knowledge").** IBM 4230, 1991:
  Stop (display NOT READY), move the form with Micro Up / Micro Down / Line Feed;
  "If you need to adjust the paper downward, adjust at least five steps beyond
  where you want the top of form position; then make your final adjustment in the
  up direction"; press Set Top of Form (alarm beeps; display `TOP OF FORM SET`);
  then press Form Feed and "decide if the form is on the desired line. If
  necessary, repeat Steps 1 through 4." 4230 pp. 1-46, 1-47. The Top of Form
  Alignment test prints "a single line of H's" and shows `Top of Form Alignment /
  Press ENTER to Reprint`; Line Feed advances 4.23 mm (0.17 in); repeat until
  correct, then Set Top of Form. 4230 pp. 5-4, 5-5.

### 1.3 Corporate operations-procedure registers

- **IBM training booklet ZR23 (the F-OPS model).** Preface (p. iii): "sign on the
  FIS terminal" before and after study; errata check first; "enter your answers
  ... enables you to get the course completion certificate. Entering your answers
  also updates your training history." Materials listed by form number (ZR23-6845,
  -6846, -6847, -6848; bill of forms ZBOF-5512) (p. iv). Time Required stated as a
  fact: "six hours" (p. iv). Duties and Tasks as numbered observable actions
  (p. v). Procedures give the exact keystroke and expected screen:
  "Press Enter twice to default the date and time. Key DISKCOPY a: b:" and
  "When the question is prompted again, Enter an 'N' to terminate" (pp. 3-5, 3-6);
  "If you are a skeptic, you can enter DIR ... to compare" (p. 3-6); a fill-in
  record step "ENTER YOUR VDISK NAME HERE: ____" (Lab Project 1, p. 3-8). Close-out:
  "Return the library materials to your coordinator" (p. 4-5); Appendix A Student
  Course Evaluation answered "in boxes 1 through 7 of Area 1 on the FIS Data
  Collection Sheet" and "If the sheet is missing ... contact your education
  coordinator" (p. A-1); Appendix B defers to a wall chart (p. B-1); Appendix F
  tear-out data collection sheets with Area 1-4 boxes, "Total study time ... (FORMAT
  = HH.T)" (p. F-3). Answer key App. C; Final Quiz App. E.
- **DisplayWrite 4 Getting Started (the boxed step model).** Steps are "numbered and
  bordered by a box" (p. iii); print procedure "Printing a Document" pp. 33-34;
  "Printer 1 is the printer you will use"; "the status line indicates that the
  document is in the print queue" (p. 34); install prerequisites enumerated
  "Before you run the Install program, you need:" the printer name and model, port
  names (p. 9 area).
- **Escalation language.** The IBM source language is role-based: "coordinator",
  "education monitor", "Service Coordinator" (ZR23 pp. iii, 4-5, A-1). "See your
  supervisor" is the Initech equivalent (HOUSE; the film's register).
- **Memo control numbers, double-keyed control numbers, sign-off boxes.** UNSOURCED
  as documents. The nearest sourced practice is two-pass verification: the IBM
  056 card verifier had a second operator key the same data and the machine
  compared, notching mismatches. https://en.wikipedia.org/wiki/Two-pass_verification
  and https://ibm1401.computerhistory.org/IBM-056-22-6006-0-Card-Verifier-056.pdf
  (table of contents and opening text only were retrieved).

### 1.4 The film canon (what the rituals must keep)

- A cover sheet was omitted from a TPS report; several people ask about it; the
  protagonist has "eight different bosses"; the director says TPS stood for "Test
  Program Set."
  https://en.wikipedia.org/wiki/TPS_report
- A coworker asks whether he received the memo about the new cover sheets and
  tells him the new cover sheet goes on all TPS reports before they leave.
  https://www.mentalfloss.com/article/57338/what-tps-report (summary of the scene;
  exact dialogue not reproduced here, and not needed: Initech prose is never a
  quotation, VO-12).
- The film therefore implies four rituals: the cover sheet, the memo about it, the
  asked-twice question, the supervisor escalation. It implies nothing about
  printer alignment or double keying; those are the manual-side busywork grounded
  above.

---

## 2. Reading of the shipped rituals

What `spec/tps_ritual.json` and `os/milton/tpsrep_program.asm` do today: R1
mandatory `TPSPARM.DAT` (TITLE, DIVISION, MEMO, LINES); R2 SPACE exactly 7 times
then ENTER, period per press; R3 report number keyed twice; R4 the memo number
keyed against MEMO=; R5 Y/N cover-sheet attestation (N = return code 5). Output is
`TPSREPT.TXT`, no printer I/O. Cover sheet footer: `FORM TPS-1 (REV. 10/89)`.

## 3. Options per ritual

Every option below is something a 1991 operator genuinely did. "Wording" is copied
from the sourced message (tag) or marked (HOUSE) when composed in that register.

### R1  Mandatory parameter file

| Option | What the operator did (period) | Wording to copy | Verdict |
|---|---|---|---|
| A. Keep | A run reads its control record; a missing one is a DOS-level refusal | `File not found` register; Lotus: "refer to Chapter 4 of Setting Up" (L123 p. 2-201) | keep |
| B. Re-ground as a run-parameter card | JCL JOB card carries the programmer name and run values; `$HASP100` echoes it (JES2 URL) | `TPS-001 RUN PARAMETER RECORD NOT FOUND. REFER TO OPERATING PROCEDURE OP-7.3.` (HOUSE) | re-ground |
| C. Replace with printer FCB/forms record | The job names its FCB and form; a missing image prompts the operator (SETPRT URL) | "the operator is asked to specify an alternate image name or cancel the request" | replace |

**Recommended: A (keep).** It is already a real operator chore (a mandatory control
file), the wording already points at an OP number, and nothing period-wise is
improved by changing it.

### R2  Form feed alignment

| Option | What the operator did (period) | Wording to copy | Verdict |
|---|---|---|---|
| A. Keep, minus the count in the prompt (5igf) | Know the number from training: IBM 4230 steps are numeric ("at least five steps beyond"; 4230 p. 1-46) | prompt `ALIGN THE FORM FEED, THEN PRESS ENTER.` (HOUSE); keep `FORM ALIGNED.` | keep |
| B. Alignment test line + accept loop (dBASE SAMPLE / 4230 TOF test) | Print a single line, look at it, adjust, print again until accepted | dBASE: `Do you want more samples?`; 4230: `Top of Form Alignment / Press ENTER to Reprint`, `TOP OF FORM SET` | re-ground |
| C. FCB verify (IEC128D) | Compare the mounted forms to the named FCB image on the printer; CANCEL if wrong | `FCB STD2 VERIFY` text of IEC172I; `IEC125D ERROR - REPEAT REPLY` | re-ground |
| D. Lotus Align / WordPerfect "Go" | Position the paper at top of page, then issue Align / Go | Lotus: "position the printer paper at the top of a page"; WP51 p. 542 "advance the paper to the top of the next page" | replace |

**Recommended: B.** The program prints one test line to PRN, the operator moves the
paper (the manual states the number of line-spaces, which is the "7" kept as
tacit knowledge in OP-7.x), ENTER reprints, and acceptance is `FORM ALIGNED.` It is
exactly the 4230 and dBASE sample loop and keeps the existing per-press and reset
messages as the "not aligned" branch.

### R3  Report number keyed twice

| Option | What the operator did (period) | Wording to copy | Verdict |
|---|---|---|---|
| A. Keep | Second operator re-keys, machine compares, mismatch rejected (IBM 056 verifier, URL in 1.3) | `ENTER TPS REPORT NUMBER:` / `RE-ENTER ...`; `TPS-003 REPORT NUMBERS DO NOT AGREE. PLEASE BEGIN AGAIN.` | keep |
| B. Number from a control log | The next number is read from a pre-numbered log sheet (job-number register); the program checks it | `ENTER TPS REPORT NUMBER FROM CONTROL LOG TPS-2:` (HOUSE) | re-ground |
| C. Check digit | Self-check instead of re-key | none period-sourced | replace |

**Recommended: A with B's paperwork.** Keep the double key (genuine verification
practice) and have the manual issue Form TPS-2 (report number control log); the
program text is unchanged.

### R4  Memo acknowledgement

| Option | What the operator did (period) | Wording to copy | Verdict |
|---|---|---|---|
| A. Keep | Quote the memo's control number back to prove it was read | `KEY THE NUMBER OF THE MEMO ABOUT THE NEW COVER SHEETS:` / `TPS-004 THAT IS NOT THE MEMO NUMBER. SEE YOUR SUPERVISOR.` | keep |
| B. Re-ground as distribution acknowledgement | Initial the distribution list; control number printed on the memo | `MEMO CONTROL NUMBER:` (HOUSE) | re-ground |
| C. Replace with a coordinator contact | Defer to the "coordinator" (ZR23 pp. 4-5, A-1) | `CONTACT YOUR COORDINATOR.` | replace |

**Recommended: A.** It is canon (the memo about the cover sheets) and the
supervisor wording is the film's register; the manual logs the memo number in Form
TPS-3.

### R5  Cover sheet attestation

| Option | What the operator did (period) | Wording to copy | Verdict |
|---|---|---|---|
| A. Keep Y/N | Answer the question; N ends the run with no output | `DID YOU REMEMBER TO ATTACH THE NEW COVER SHEET? (Y/N)` | keep |
| B. Special-forms mount attestation | Confirm the named form is mounted before print (IEC163A/164A mount requests; WP "Prompt to Load") | `INSERT PAPER-PRESS "G" TO CONTINUE` (WP51 p. 751); `MOUNT FORM TPS-1 (REV. 10/89). PRESS Y WHEN MOUNTED.` (HOUSE) | re-ground |
| C. Sign-off box | Sign a printed box on the cover sheet | `SIGNATURE` line already printed by the report | replace |

**Recommended: B, keeping A's canon sentence.** Append the form id (TPS-1 REV.
10/89, already on the cover sheet) so the question is a real forms attestation;
N still ends with return code 5.

Summary of recommendations: R1 keep; R2 alignment test line with accept loop; R3
keep plus control log; R4 keep plus memo log; R5 form-id attestation.

## 4. Proposed printer-side behaviour (for a later, separately approved change)

Not implemented here.

1. **Device.** After the report is built, copy it to the DOS device `PRN` (LPT1) as
   well as `TPSREPT.TXT`; a command-line switch (HOUSE: `/P`) selects it, matching
   `SET PRINTER TO` semantics (UDB p. U5-252). No handle-API leakage into the FCB
   rule: PRN is opened as the FCB device name `PRN`.
2. **Form feed.** First byte of a printed report is not a form feed (dBASE "paper
   must initially be set to the top-of-form", UDB p. U5-120); a form feed (0Ch) is
   written after each page and after the last (Lotus Align, dBASE EJECT).
3. **Page.** 58 text lines of a 66-line page (dBASE default, LDB Table 6-1);
   the existing LINES= parameter stays.
4. **Alignment test pattern (R2-B).** One line of capital H characters, as in the
   4230 Top of Form Alignment test (4230 p. 5-4), followed by `TOP OF FORM` prompts;
   reprinted on each ENTER.
5. **Faults.** On a not-ready printer: `PRINTER NOT READY. RETRY? (Y/N)` (UDB
   appendix); DOS's own `No paper error writing device PRN` / `Abort, Retry,
   Ignore?` surfaces unchanged (D310 A-7).

## 5. What the manual (ipu5.14) must therefore contain

Model: ZR23 sessions plus DW4 step boxes (F-OPS, AN-15, VO-06: no explanation,
no irony).

**Procedures** (TPS-P-nn, each with Purpose / Frequency / Performed by /
Prerequisites / Materials / steps / Verification / Records retained):

1. Leave FLAIR to the InitechDOS prompt.
2. Prepare the parameter record (OP-7.3, `TPSPARM.DAT`).
3. Load and align forms: set top of form, print the alignment test line, advance
   the paper the stated number of line-spaces, accept (the count is written here
   and nowhere else; 5igf).
4. Assign and key the report number twice (Form TPS-2 log).
5. Acknowledge the memo (Form TPS-3 log).
6. Attest the cover sheet (Form TPS-1 REV. 10/89 mounted).
7. Review and retain output (`TPSREPT.TXT`, `TPSREPT.BAK`).
8. Printer faults and restart ("reset the forms ... then Go", WP51 p. 542 model).

**Forms** (each with form number and revision): TPS-1 cover sheet; TPS-2 report
number control log (double-entry columns); TPS-3 memo acknowledgement log; TPS-4
forms alignment record; TPS-5 operator sign-off sheet; Reader's Comment Form
(internal mail), per ZR23 App. A and F.

**Checklists**: pre-run, post-run, escalation (supervisor, coordinator).

**Appendices**: A Operator Course Evaluation; B Terminal Procedures (the exit to
the prompt); C Answers to Study Questions; D Review (DOS commands used); E Final
Quiz; F Data Collection Sheets; G Error Message Appendix (TPS-001..008,
IEC-style repeat-reply wording, DOS device errors).

Every keystroke sequence is replayed by test-pubs; a ritual the program does not
enforce is never invented (DESIGN-REFERENCE Sec 6.2).

## Appendix A. Period message wording (copy bank)

| Text | Source |
|---|---|
| `Do you want more samples?` | UDB p. U5-143 |
| `Printer not ready . Retry? (Y/N)` | UDB error appendix |
| `Printer is either not connected or turned off.` | UDB error appendix |
| `No paper` / "Turn the printer ON, press the ONLINE switch, or add paper and retry." | D310 p. A-7 |
| `No paper error writing device dev` | M33 message appendix |
| `Print queue is full` / `Resident part of PRINT installed` | D310 A-81 |
| `Name of list device [PRN]:` | D310 / M33 |
| `Insert Paper-Press "G" to Continue` | WP51 p. 751 |
| `TOP OF FORM SET` | 4230 pp. 1-46, 5-5 |
| `Top of Form Alignment / Press ENTER to Reprint` | 4230 p. 5-5 |
| `NOT READY` | 4230 p. 1-46 |
| `IEC125D ERROR - REPEAT REPLY` | IBM URL, 1.1 |
| `IEC172I dev OPERATOR CANCELED FCB imageid VERIFY REQUEST` | IBM URL, 1.1 |
| `Verify alignment on printer` (CPA4002) | search result, 1.1 |
| "Return the library materials to your coordinator." | ZR23 p. 4-5 |
| "If the sheet is missing ... contact your education coordinator." | ZR23 p. A-1 |
| "If you are a skeptic, you can enter ... to compare" | ZR23 p. 3-6 (register model only) |
| `ENTER YOUR VDISK NAME HERE:` | ZR23 p. 3-8 (fill-in record step) |

Words in this appendix are for register matching; Initech's own prose is never a
copy (VO-12).
