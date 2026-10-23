1: /*
2:  * os/iword/core/listfiles.c -- InitechWord's List Files (F5) rows, composed
3:  * exactly as the real WordPerfect 5.1 lays them out, at WP's own 80 columns,
4:  * then fitted to the face's cells by the locked column deletes (THE ARTIFACT;
5:  * bead initech-qvgd, IWORD P3 remainder).
6:  *
7:  * Ref: spec/iword/listfiles.h -- every column, label and order from the real
8:  *      WP 5.1 mint ../wordperfect51-decomp mint/work/LISTF/mint-results-005.md
9:  *      (grids LF11_06 list, LF11_09 Look, LF11_01 prompt);
10:  *      spec/toolbox_gate.h Sec 10a (the 22-byte find record);
11:  *      DOS 3.3 PRM AH=4Eh (the packed date: bits 15-9 year - 1980, 8-5 month,
12:  *      4-0 day; time: 15-11 hour, 10-5 minute).
13:  * The two-digit year is WordPerfect's own arithmetic -- '0' + (year - 1900) / 10,
14:  * '0' + (year - 1900) % 10 -- so 1992 is "92" and a 2026 clock prints "<6"
15:  * as the real WP 5.1 does (mint 005 ledger, row 0). Canon, not fixed.
16:  * Pure: no Toolbox call, no I/O; the face draws what these compose.
17:  * Freestanding C, ASCII-clean (Rule 12), deterministic (Rule 11).
18:  */
19: #include "iword.h"
20: #include "iword/listfiles.h"
21: #include "toolbox_gate.h"     /* Sec 10a: the 22-byte find record */
22: 
23: static void put(char *row, uint32_t col, const char *s)
24: {
25:     for (uint32_t i = 0; s[i] && col + i < IW_LF_WP_COLS; i++) row[col + i] = s[i];
26: }
27: 
28: void iw_lf_blank(char *row)
29: {
30:     for (uint32_t i = 0; i < IW_LF_WP_COLS; i++) row[i] = ' ';
31:     row[IW_LF_WP_COLS] = 0;
32: }
33: 
34: uint32_t iw_lf_group(uint32_t v, char *out)
35: {
36:     char t[16];
37:     uint32_t n = 0, k = 0, i;
38:     do {
39:         if (k == 3u) { t[n++] = ','; k = 0; }
40:         t[n++] = (char)('0' + v % 10u); v /= 10u; k++;
41:     } while (v);
42:     for (i = 0; i < n; i++) out[i] = t[n - 1u - i];
43:     out[n] = 0;
44:     return n;
45: }
46: 
47: /* the value comma-grouped, right-aligned to end at column `end` */
48: static void put_right(char *row, uint32_t end, uint32_t v)
49: {
50:     char num[16];
51:     uint32_t n = iw_lf_group(v, num);
52: #ifdef IWORD_MUT_LF_COLUMN
53:     /* MUTANT (Rule 6; test-iword-listfiles-mutant, test-flair-iword-listfiles-
54:      * mutant): every right-aligned figure one column right. NEVER real. */
55:     end++;
56: #endif
57:     if (end + 1u >= n && end < IW_LF_WP_COLS) put(row, end + 1u - n, num);
58: }
59: 
60: static void two(char *at, uint32_t v) { at[0] = (char)('0' + v / 10u % 10u); at[1] = (char)('0' + v % 10u); }
61: 
62: void iw_lf_put_date(char *at, uint32_t year, uint32_t month, uint32_t day)
63: {
64:     uint32_t yy = year >= 1900u ? year - 1900u : 0u;   /* WP's arithmetic */
65:     two(at, month); at[2] = '-';
66:     two(at + 3, day); at[5] = '-';
67:     at[6] = (char)('0' + yy / 10u); at[7] = (char)('0' + yy % 10u);
68: }
69: 
70: void iw_lf_put_time(char *at, uint32_t hour, uint32_t minute)
71: {
72:     uint32_t h = hour % 12u;
73:     two(at, h ? h : 12u); at[2] = ':';
74:     two(at + 3, minute);
75:     at[5] = hour >= 12u ? 'p' : 'a';
76: }
77: 
78: /* A zero date word shows no stamp at all (mint 005 LF14_02: the kernel-made
79:  * DESKTOP.DB and TRASH, date and time 0, list with blank date and time). */
80: static void put_dos_stamp(char *row, uint32_t col, uint16_t date, uint16_t time)
81: {
82: #ifndef IWORD_MUT_LF_ZERO_DATE
83:     if (date == 0u) return;
84: #endif
85:     iw_lf_put_date(row + col, 1980u + (date >> 9), (date >> 5) & 15u, date & 31u);
86:     iw_lf_put_time(row + col + IW_LF_E_TIME - IW_LF_E_DATE, time >> 11, (time >> 5) & 63u);
87: }
88: 
89: void iw_lf_rec(const uint8_t *rec, iw_lf_ent_t *e)
90: {
91:     uint32_t i;
92:     e->attr = rec[TBX_FIND_REC_ATTR_OFF];
93:     e->time = (uint16_t)(rec[TBX_FIND_REC_TIME_OFF] | rec[TBX_FIND_REC_TIME_OFF + 1u] << 8);
94:     e->date = (uint16_t)(rec[TBX_FIND_REC_DATE_OFF] | rec[TBX_FIND_REC_DATE_OFF + 1u] << 8);
95:     e->size = (uint32_t)rec[TBX_FIND_REC_SIZE_OFF] | (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 1u] << 8 |
96:               (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 2u] << 16 | (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 3u] << 24;
97:     for (i = 0; i < 12u && rec[TBX_FIND_REC_NAME_OFF + i]; i++) e->name[i] = (char)rec[TBX_FIND_REC_NAME_OFF + i];
98:     e->name[i] = 0;
99: }
100: 
101: /* the 8-cell name and 3-cell extension of "NAME.EXT" */
102: static void split(const char *nm, char base[9], char ext[4])
103: {
104:     uint32_t i = 0, k = 0;
105:     for (; nm[i] && nm[i] != '.' && k < 8u; i++) base[k++] = nm[i];
106:     base[k] = 0;
107:     while (nm[i] && nm[i] != '.') i++;
108:     k = 0;
109:     if (nm[i] == '.') for (i++; nm[i] && k < 3u; i++) ext[k++] = nm[i];
110:     ext[k] = 0;
111: }
112: 
113: /* WP's order (spec "the list"): directories before files, each by the
114:  * space-padded 8-cell name, then the space-padded 3-cell extension. */
115: static void sort_key(const char *nm, char key[11])
116: {
117:     char base[9], ext[4];
118:     uint32_t i;
119:     split(nm, base, ext);
120:     for (i = 0; i < 11u; i++) key[i] = ' ';
121:     for (i = 0; base[i]; i++) key[i] = base[i];
122:     for (i = 0; ext[i]; i++) key[8u + i] = ext[i];
123: }
124: static int before(const iw_lf_ent_t *a, const iw_lf_ent_t *b)
125: {
126:     char ka[11], kb[11];
127:     int ad = (a->attr & IW_LF_ATTR_DIR) != 0, bd = (b->attr & IW_LF_ATTR_DIR) != 0;
128:     if (ad != bd) return ad;
129:     sort_key(a->name, ka); sort_key(b->name, kb);
130:     for (uint32_t i = 0; i < 11u; i++)
131:         if (ka[i] != kb[i]) return (unsigned char)ka[i] < (unsigned char)kb[i];
132:     return 0;
133: }
134: 
135: uint32_t iw_lf_load(const uint8_t *recs, uint32_t nrec, iw_lf_ent_t *out, uint32_t cap,
136:                     uint32_t *used, uint32_t *files)
137: {
138:     uint32_t n = 0, i, j;
139:     *used = 0; *files = 0;
140:     for (i = 0; i < nrec && n < cap; i++) {
141:         iw_lf_ent_t e;
142:         iw_lf_rec(recs + i * TBX_FIND_REC_BYTES, &e);
143:         if (e.name[0] == '.') continue;          /* "." and "..": slots 0 and 1 */
144:         if (!(e.attr & IW_LF_ATTR_DIR)) { *used += e.size; (*files)++; }
145:         /* insertion into the sorted run (stable; n is small) */
146:         for (j = n; j > 0 && before(&e, &out[j - 1u]); j--) out[j] = out[j - 1u];
147:         out[j] = e;
148:         n++;
149:     }
150:     return n;
151: }
152: 
153: void iw_lf_row_header0(char *row, const iw_lf_clock_t *c, const char *pattern)
154: {
155:     iw_lf_blank(row);
156:     iw_lf_put_date(row + IW_LF_DATE_COL, c->year, c->month, c->day);
157:     iw_lf_put_time(row + IW_LF_TIME_COL, c->hour, c->minute);
158:     put(row, IW_LF_DIR_COL, IW_LF_DIR_LABEL);
159:     put(row, IW_LF_DIR_COL + sizeof IW_LF_DIR_LABEL - 1u, pattern);
160: }
161: 
162: void iw_lf_row_header1(char *row, uint32_t docsize, uint32_t free_bytes, uint32_t used, uint32_t files)
163: {
164:     iw_lf_blank(row);
165:     put(row, IW_LF_DOCSIZE_COL, IW_LF_DOCSIZE_LABEL); put_right(row, IW_LF_DOCSIZE_END, docsize);
166:     put(row, IW_LF_FREE_COL, IW_LF_FREE_LABEL);       put_right(row, IW_LF_FREE_END, free_bytes);
167:     put(row, IW_LF_USED_COL, IW_LF_USED_LABEL);       put_right(row, IW_LF_USED_END, used);
168:     put(row, IW_LF_FILES_COL, IW_LF_FILES_LABEL);     put_right(row, IW_LF_FILES_END, files);
169: }
170: 
171: /* slot 0 ".  Current", slot 1 "..  Parent", slot k >= 2 the entry k - 2 */
172: static void put_slot(char *row, uint32_t lead, const iw_lf_ent_t *ents, uint32_t slot)
173: {
174:     if (slot < 2u) {
175:         put(row, lead, slot ? ".." : ".");
176:         put(row, lead + IW_LF_E_WORD, slot ? IW_LF_PARENT : IW_LF_CURRENT);
177:         put(row, lead + IW_LF_E_DIR, IW_LF_DIR_TAG);
178:         return;
179:     }
180:     {
181:         const iw_lf_ent_t *e = &ents[slot - 2u];
182:         char base[9], ext[4];
183:         split(e->name, base, ext);
184:         put(row, lead, base);
185:         row[lead + IW_LF_E_DOT] = '.';
186:         put(row, lead + IW_LF_E_EXT, ext);
187:         if (e->attr & IW_LF_ATTR_DIR) put(row, lead + IW_LF_E_DIR, IW_LF_DIR_TAG);
188:         else put_right(row, lead + IW_LF_E_SIZE_END, e->size);
189:         put_dos_stamp(row, lead + IW_LF_E_DATE, e->date, e->time);
190:     }
191: }
192: 
193: void iw_lf_row_list(char *row, const iw_lf_ent_t *ents, uint32_t nents, uint32_t list_row)
194: {
195:     uint32_t slots = nents + 2u, s = list_row * 2u;
196:     iw_lf_blank(row);
197:     if (s < slots) put_slot(row, IW_LF_LEAD_LEFT, ents, s);
198:     if (s + 1u < slots) {
199:         row[IW_LF_BAR_COL] = IW_LF_BAR_CHAR;
200:         put_slot(row, IW_LF_LEAD_RIGHT, ents, s + 1u);
201:     }
202: }
203: 
204: void iw_lf_row_prompt(char *row, const char *path, int hint)
205: {
206:     iw_lf_blank(row);
207:     put(row, 0, IW_LF_DIR_PROMPT);
208:     put(row, IW_LF_PATH_COL, path);
209:     if (hint) put(row, IW_LF_HINT_COL, IW_LF_HINT);
210: }
211: 
212: void iw_lf_row_look(char *row, const char *path, int wp51, uint16_t date, uint16_t time)
213: {
214:     uint32_t col = IW_LF_WP_COLS - (sizeof IW_LF_LOOK_REVISED - 1u) - 15u;
215:     iw_lf_blank(row);
216:     put(row, 0, IW_LF_LOOK_FILE);
217:     put(row, sizeof IW_LF_LOOK_FILE - 1u, path);
218:     if (wp51) put(row, IW_LF_LOOK_WP51_COL, IW_LF_LOOK_WP51);
219:     put(row, col, IW_LF_LOOK_REVISED);
220:     col += sizeof IW_LF_LOOK_REVISED - 1u;
221:     iw_lf_put_date(row + col, 1980u + (date >> 9), (date >> 5) & 15u, date & 31u);
222:     row[col + 8u] = ' ';
223:     iw_lf_put_time(row + col + 9u, time >> 11, (time >> 5) & 63u);
224: }
225: 
226: uint32_t iw_lf_fit(const char *row, const uint8_t *del, uint32_t ndel, char *out, uint32_t width)
227: {
228:     uint32_t c, n = 0, k;
229:     for (c = 0; c < IW_LF_WP_COLS && n < width; c++) {
230:         for (k = 0; k < ndel && del[k] != c; k++) {}
231:         if (k < ndel) continue;
232:         out[n++] = row[c];
233:     }
234:     for (k = n; k < width; k++) out[k] = ' ';
235:     out[width] = 0;
236:     return n;
237: }
238: 
239: uint32_t iw_lf_face_col(uint32_t wp_col, const uint8_t *del, uint32_t ndel)
240: {
241:     uint32_t k, n = wp_col;
242:     for (k = 0; k < ndel; k++) if (del[k] < wp_col) n--;
243:     return n;
244: }
