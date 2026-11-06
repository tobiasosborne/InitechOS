/* assist.c -- the InitechBase Assistant command front-end.
 * Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md sections
 * 1-3, 10 (menu tree, action/status/message rows, command composition).
 * EXPERIMENT initech-7cbu8: only linked into SAMIRX (-DSAMIR_W4).
 */
#include "samir/kit.h"
#include "samir/rt.h"
#include "samir/workarea.h"
#include "ui_apps.h"

#define ASSIST_COMMAND_CAP 512

enum { A_QUIET, A_OUTPUT, A_EDITOR, A_CONFIRM, A_DATABASE, A_QUIT };

typedef struct assist_action {
    const char *command;
    const char *prompt;
    const char *message;
    int mode;
} assist_action;

static const char *const assist_titles[] = {
    "Set Up", "Create", "Update", "Position", "Retrieve", "Organize", "Modify", "Tools"
};
static const char *const assist_items[] = {
    "Database file\0Format for Screen\0Query\0Catalog\0View\0Quit dBASE III PLUS\0",
    "Database file\0Format\0View\0Query\0Report\0Label\0",
    "Append\0Edit\0Display\0Browse\0Replace\0Delete\0Recall\0Pack\0",
    "Seek\0Locate\0Continue\0Skip\0Goto Record\0",
    "List\0Display\0Report\0Label\0Sum\0Average\0Count\0",
    "Index\0Sort\0Copy\0",
    "Database file\0Format\0View\0Query\0Report\0Label\0",
    "Set drive\0Copy file\0Directory\0Rename\0Erase\0List structure\0Import\0Export\0"
};
static const int assist_counts[] = {6,6,8,5,7,3,6,8};

/* Empty operand prompts mean the command is complete as written. More
 * involved commands accept the ordinary dBASE operand syntax in one field. */
static const assist_action assist_actions[8][8] = {
    {
        {"USE", "Database file name:", "Select the active database file and its indexes.", A_DATABASE},
        {"SET FORMAT TO", "Screen format file:", "Select a screen format for record entry.", A_QUIET},
        {"SET FILTER TO", "Filter condition or FILE query:", "Select records using a condition or a saved query.", A_QUIET},
        {"SET CATALOG TO", "Catalog file:", "Select the catalog of working files.", A_QUIET},
        {"SET VIEW TO", "View file:", "Restore a saved database environment.", A_QUIET},
        {"QUIT", "", "Close files and return to the operating system.", A_QUIT}
    }, {
        {"CREATE", "New database file:", "Define the fields of a new database.", A_EDITOR},
        {"CREATE SCREEN", "New screen file:", "Design a screen format.", A_EDITOR},
        {"CREATE VIEW", "New view file:", "Save a database environment as a view.", A_EDITOR},
        {"CREATE QUERY", "New query file:", "Build and save a query condition.", A_EDITOR},
        {"CREATE REPORT", "New report file:", "Define columns, groups, and totals for a report.", A_EDITOR},
        {"CREATE LABEL", "New label file:", "Define the layout of mailing labels.", A_EDITOR}
    }, {
        {"APPEND", "", "Enter new records at the end of the database.", A_EDITOR},
        {"EDIT", "", "Edit the active record on a full screen.", A_EDITOR},
        {"DISPLAY", "", "Display the current record.", A_OUTPUT},
        {"BROWSE", "", "View and edit records in a table.", A_EDITOR},
        {"REPLACE", "Field WITH value (and optional scope):", "Replace field values in selected records.", A_CONFIRM},
        {"DELETE", "Scope/condition (blank = current):", "Mark records for later removal by PACK.", A_CONFIRM},
        {"RECALL", "Scope/condition (blank = current):", "Clear the deletion marks on selected records.", A_CONFIRM},
        {"PACK", "", "Permanently remove all records marked for deletion.", A_CONFIRM}
    }, {
        {"SEEK", "Index key expression:", "Find a key using the active index.", A_QUIET},
        {"LOCATE FOR", "Search condition:", "Find the first record meeting a condition.", A_QUIET},
        {"CONTINUE", "", "Continue the last LOCATE search.", A_QUIET},
        {"SKIP", "Number of records (blank = 1):", "Move forward or backward in the active record order.", A_QUIET},
        {"GO", "Record number, TOP or BOTTOM:", "Position the database at a selected record.", A_QUIET}
    }, {
        {"LIST", "", "List all records in the active database.", A_OUTPUT},
        {"DISPLAY", "", "Display the current record.", A_OUTPUT},
        {"REPORT FORM", "Report file (and optional clauses):", "Generate a report from a saved definition.", A_OUTPUT},
        {"LABEL FORM", "Label file (and optional clauses):", "Generate labels from a saved definition.", A_OUTPUT},
        {"SUM", "Expressions/scope (blank = all):", "Calculate totals for numeric expressions.", A_OUTPUT},
        {"AVERAGE", "Expressions/scope (blank = all):", "Calculate averages for numeric expressions.", A_OUTPUT},
        {"COUNT", "Scope/condition (blank = all):", "Count the records in the selected scope.", A_OUTPUT}
    }, {
        {"INDEX ON", "Expression TO index file:", "Create an index for searching and ordered access.", A_CONFIRM},
        {"SORT TO", "Output file ON fields:", "Copy records into a new database in sorted order.", A_CONFIRM},
        {"COPY TO", "Output file (and optional clauses):", "Copy records to another database.", A_CONFIRM}
    }, {
        {"MODIFY STRUCTURE", "", "Change the field definitions of the active database.", A_EDITOR},
        {"MODIFY SCREEN", "Screen file:", "Change a saved screen format.", A_EDITOR},
        {"MODIFY VIEW", "View file:", "Change a saved database environment.", A_EDITOR},
        {"MODIFY QUERY", "Query file:", "Change a saved query condition.", A_EDITOR},
        {"MODIFY REPORT", "Report file:", "Change a saved report definition.", A_EDITOR},
        {"MODIFY LABEL", "Label file:", "Change a saved mailing label definition.", A_EDITOR}
    }, {
        {"SET DEFAULT TO", "Drive letter (for example B):", "Select the default drive for database files.", A_QUIET},
        {"COPY FILE", "Source file TO destination file:", "Make a copy of a disk file.", A_CONFIRM},
        {"DIRECTORY", "File mask (blank = database files):", "List the files on the default drive.", A_OUTPUT},
        {"RENAME", "Old file TO new file:", "Change the name of a disk file.", A_CONFIRM},
        {"ERASE", "File to erase:", "Permanently erase a disk file.", A_CONFIRM},
        {"LIST STRUCTURE", "", "List the fields of the active database.", A_OUTPUT},
        {"APPEND FROM", "Input file and TYPE clauses:", "Import compatible records into the active database.", A_CONFIRM},
        {"COPY TO", "Output file and TYPE clauses:", "Export the active database records.", A_CONFIRM}
    }
};

static int assist_append(char *command, const char *text)
{
    uint32_t used = rt_strlen(command), n = rt_strlen(text);
    if (used + n >= ASSIST_COMMAND_CAP) return 0;
    rt_memcpy(command + used, text, n + 1);
    return 1;
}

static void assist_action_line(kit *k, const char *command)
{
    kit_fill(k, 21, 0, KIT_COLS, ' ', KIT_ATTR_NORMAL);
    kit_put(k, 21, 0, "Command: ", -1);
    kit_put(k, 21, 9, command, -1);
}

static void assist_status(kit *k)
{
    wa_env *env = xb_interp_env(k->ip);
    int area = wa_selected(env);
    dbf_table *table = wa_table(env, area);
    kit_status(k, "ASSIST", table ? dbf_name(table) : "",
               wa_recno(env, area), wa_nrec(env, area), 0);
}

static void assist_clock(kit *k)
{
    char clock[9] = "--:--:--";
    uint8_t hour, minute, second;
    if (k->pal->time) {
        k->pal->time(k->pal, &hour, &minute, &second);
        clock[0] = (char)('0'+hour/10); clock[1] = (char)('0'+hour%10);
        clock[3] = (char)('0'+minute/10); clock[4] = (char)('0'+minute%10);
        clock[6] = (char)('0'+second/10); clock[7] = (char)('0'+second%10);
    }
    kit_put_attr(k, 0, 72, KIT_ATTR_REVERSE, clock, 8);
}

static void assist_frame(kit *k, const char *command)
{
    kit_clear(k);
    assist_clock(k);
    assist_action_line(k, command);
    assist_status(k);
    /* The original guidance exceeds 80 columns when arrow glyphs are spelled
     * out. Keep all four actions readable on a single period-sized row. */
    kit_message(k, 23, "Move - up/down.  Select - Enter.  Leave menu - left/right.  Help - F1.");
    kit_message(k, 24, "Choose a menu option.  Esc returns to the dot prompt.");
}

static const char *assist_item(int pad, int item)
{
    const char *s = assist_items[pad];
    while (item-- && *s) s += rt_strlen(s) + 1;
    return s;
}

/* Local menu fallback: kit_menu was still an unimplemented kit.c stub at the
 * first build. This also keeps the clock and per-item explanation visible,
 * and lets F1 enter the real HELP app without changing the shared kit API. */
static int assist_menu(kit *k, int start_pad, const char *command)
{
    int pad = start_pad, item = 0;
    for (;;) {
        int i, col = 1, left = 1, width = 0, key;
        assist_frame(k, command);
        kit_fill(k, 0, 0, KIT_COLS, ' ', KIT_ATTR_REVERSE);
        for (i = 0; i < 8; i++) {
            int n = (int)rt_strlen(assist_titles[i]);
            if (i == pad) left = col;
            kit_put_attr(k, 0, col, i == pad ? KIT_ATTR_BRIGHT : KIT_ATTR_REVERSE,
                         assist_titles[i], n);
            col += n + 2;
        }
        assist_clock(k);
        for (i = 0; i < assist_counts[pad]; i++) {
            int n = (int)rt_strlen(assist_item(pad, i));
            if (n > width) width = n;
        }
        width += 4;
        if (left + width > KIT_COLS) left = KIT_COLS - width;
        kit_box(k, 1, left, assist_counts[pad]+2, left+width-1, 0);
        for (i = 0; i < assist_counts[pad]; i++) {
            uint8_t attr = i == item ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL;
            kit_fill(k, i+2, left+1, width-2, ' ', attr);
            kit_put_attr(k, i+2, left+2, attr, assist_item(pad, i), -1);
        }
        kit_message(k, 24, assist_actions[pad][item].message);
        kit_cursor(k, item+2, left+2);
        key = kit_key(k);
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF || key == KIT_K_F10) return -1;
        if (key == KIT_K_ENTER) return (pad << 8) | item;
        if (key == KIT_K_LEFT || key == KIT_K_RIGHT) {
            pad = (pad + (key == KIT_K_LEFT ? 7 : 1)) % 8;
            item = 0;
        } else if (key == KIT_K_UP || key == KIT_K_DOWN) {
            item = (item + (key == KIT_K_UP ? assist_counts[pad]-1 : 1)) % assist_counts[pad];
        } else if (key == KIT_K_HOME) item = 0;
        else if (key == KIT_K_END) item = assist_counts[pad]-1;
        else if (key == KIT_K_F1) {
            int ec = 0;
            (void)ui_help(k->ip, assist_actions[pad][item].command, &ec);
        } else if (key >= 32 && key <= 126) {
            if (key >= 'a' && key <= 'z') key -= 'a' - 'A';
            for (i = 1; i <= assist_counts[pad]; i++) {
                int next = (item+i) % assist_counts[pad];
                char first = *assist_item(pad, next);
                if (first >= 'a' && first <= 'z') first -= 'a' - 'A';
                if (first == key) { item = next; break; }
            }
        }
    }
}

static int assist_yesno(kit *k, const char *question)
{
    int key;
    kit_clear_rows(k, 18, 20);
    kit_put_attr(k, 19, 2, KIT_ATTR_BRIGHT, question, -1);
    kit_cursor(k, 19, 2 + (int)rt_strlen(question));
    for (;;) {
        key = kit_key(k);
        if (key == 'y' || key == 'Y') return 1;
        if (key == 'n' || key == 'N') return 0;
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) return -1;
    }
}

static int assist_confirm(kit *k)
{
    int key;
    kit_clear_rows(k, 18, 20);
    kit_put_attr(k, 19, 2, KIT_ATTR_BRIGHT, "Execute the command?  Enter - execute.  Esc - cancel.", -1);
    kit_cursor(k, 19, 2);
    for (;;) {
        key = kit_key(k);
        if (key == KIT_K_ENTER || key == 'y' || key == 'Y') return 1;
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF || key == 'n' || key == 'N') return 0;
    }
}

static int assist_build(kit *k, const assist_action *action, char *command)
{
    char operand[161];
    int indexed;
    command[0] = 0;
    (void)assist_append(command, action->command);
    assist_action_line(k, command);
    kit_message(k, 24, action->message);
    if (*action->prompt) {
        operand[0] = 0;
        kit_message(k, 23, "Enter the requested value.  Enter - accept.  Esc - cancel.");
        if (!kit_prompt(k, 18, action->prompt, operand, 44)) return 0;
        /* Empty scope clauses are useful, but a missing required operand must
         * never turn USE into an accidental close or ERASE into a wildcard. */
        if (!*operand && (action->mode == A_DATABASE ||
            (action->mode == A_EDITOR) || action->command[0] == 'G' ||
            !rt_strncmp(action->command, "ERASE", 6))) return 0;
        if (*operand) {
            (void)assist_append(command, " ");
            (void)assist_append(command, operand);
        }
        assist_action_line(k, command);
    }
    if (action->mode == A_DATABASE) {
        indexed = assist_yesno(k, "Is the file indexed? [Y/N] ");
        if (indexed < 0) return 0;
        if (indexed) {
            operand[0] = 0;
            if (!kit_prompt(k, 18, "Index file(s), separated by commas:", operand, 40) || !*operand) return 0;
            (void)assist_append(command, " INDEX ");
            (void)assist_append(command, operand);
            assist_action_line(k, command);
        }
    }
    if (action->mode == A_CONFIRM || action->mode == A_OUTPUT || action->mode == A_QUIT)
        return assist_confirm(k);
    return 1;
}

/* Returns 1 if Esc at the output screen should leave ASSIST, 2 for QUIT. */
static int assist_execute(kit *k, const assist_action *action, const char *command)
{
    int rc, key;
    char error[161];
    kit_clear(k);
    kit_put_attr(k, 0, 0, KIT_ATTR_BRIGHT, command, -1);
    kit_cursor(k, 2, 0);
    if (k->pal->set_attr) k->pal->set_attr(k->pal, KIT_ATTR_NORMAL);
    rc = samir_do(k->ip, command);
    if (rc == INTERP_RC_QUIT) return 2;
    if (rc < 0) {
        unsigned code = (unsigned)samir_last_error(k->ip);
        xb_ctx *ctx = xb_interp_ctx(k->ip);
        error[0] = 0;
        if (ctx->message) ctx->message(code, error, sizeof(error));
        if (!*error) {
            char digits[12];
            unsigned n = 0, i;
            do { digits[n++] = (char)('0'+code%10); code /= 10; } while (code && n < sizeof(digits));
            rt_memcpy(error, "Command error ", 14);
            for (i = 0; i < n; i++) error[14+i] = digits[n-1-i];
            error[14+n] = 0;
        }
        kit_message(k, 20, error);
    }
    if (action->mode == A_OUTPUT || rc < 0) {
        assist_status(k);
        kit_message(k, 23, "Press any key to continue work in ASSIST.");
        kit_message(k, 24, "Esc returns to the dot prompt.");
        key = kit_key(k);
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) return 1;
    }
    return 0;
}

int ui_assist(xb_interp *ip, const char *args, int *ec)
{
    kit k;
    char command[ASSIST_COMMAND_CAP];
    int choice, pad = 0, item, result = 0;
    (void)args;
    *ec = 0;
    command[0] = 0;
    kit_open(&k, ip);
    for (;;) {
        choice = assist_menu(&k, pad, command);
        if (choice < 0) break;
        pad = (choice >> 8) & 255; item = choice & 255;
        if (pad >= 8 || item >= assist_counts[pad]) continue;
        kit_clear_rows(&k, 1, 20);
        if (!assist_build(&k, &assist_actions[pad][item], command)) continue;
        result = assist_execute(&k, &assist_actions[pad][item], command);
        if (result) break;
    }
    kit_close(&k);
    return result == 2 ? INTERP_RC_QUIT : CMD_OK;
}
