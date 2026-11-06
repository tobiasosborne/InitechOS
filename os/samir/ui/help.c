/* help.c -- authored InitechBase help, not a copy of HELP.DBS.
 * Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 3
 * (menu/information screens, ENTER >, PgUp/PgDn, Esc); command syntax from
 * specs/commands/navigation-query-display.md and data-definition-and-manipulation.md.
 * EXPERIMENT initech-7cbu8: only linked into SAMIRX (-DSAMIR_W4).
 */
#include "samir/kit.h"
#include "samir/rt.h"
#include "ui_apps.h"

typedef struct help_topic {
    const char *name;
    const char *summary;
    const char *syntax;
    const char *description;
    const char *example;
    const char *see;
} help_topic;

/* These short explanations were written for InitechBase. No corpus text is
 * embedded; HELP.DBS is not required on the user's disk. */
static const help_topic help_topics[] = {
    {"USE", "Open a database file",
     "USE [<file>] [INDEX <index list>] [ALIAS <name>]",
     "Open a database in the selected work area.\n"
     "Its fields become available to commands and expressions.\n"
     "USE alone closes the file in this work area.",
     "USE CLIENTS\nUSE CLIENTS INDEX NAMES", "LIST, INDEX, ASSIST"},
    {"LIST", "List records and expressions",
     "LIST [<scope>] [<expressions>] [FOR <condition>]",
     "Show the records of the active database. Without a scope,\n"
     "LIST visits all records. Name fields to choose the columns.\n"
     "Use FOR to show only records that meet a condition.",
     "LIST\nLIST NAME,CITY FOR BAL > 0", "DISPLAY, USE, REPORT"},
    {"DISPLAY", "Display selected records",
     "DISPLAY [<scope>] [<expressions>] [FOR <condition>]",
     "Show fields or expressions from the active database.\n"
     "The default scope is the current record. DISPLAY ALL shows\n"
     "the whole file; RECORD selects a particular record.",
     "DISPLAY\nDISPLAY ALL NAME,CITY", "LIST, GO, BROWSE"},
    {"APPEND", "Add database records",
     "APPEND / APPEND BLANK / APPEND FROM <file>",
     "APPEND opens the record entry screen. Type field values and\n"
     "finish with Ctrl-W. APPEND BLANK adds one empty record\n"
     "without opening the screen. FROM copies compatible records.",
     "APPEND\nAPPEND BLANK", "EDIT, BROWSE, REPLACE"},
    {"EDIT", "Change a record in a form",
     "EDIT [<scope>] [FIELDS <fields>] [FOR <condition>]",
     "Edit the fields of a record on a full screen. Move between\n"
     "fields with Enter or the arrow keys. PgUp and PgDn move\n"
     "between records; Ctrl-W saves and leaves the editor.",
     "EDIT\nEDIT RECORD 2", "BROWSE, APPEND, REPLACE"},
    {"BROWSE", "Edit a table of records",
     "BROWSE [FIELDS <fields>] [NOAPPEND] [NOMENU]",
     "Show several records in columns. Arrow keys move through\n"
     "fields and records. Ctrl-U marks a record for deletion.\n"
     "Ctrl-W saves the changes and returns to the command prompt.",
     "BROWSE\nBROWSE FIELDS NAME,CITY", "EDIT, APPEND, DELETE"},
    {"REPLACE", "Assign values to fields",
     "REPLACE [<scope>] <field> WITH <value> [FOR <condition>]",
     "Write an expression's value into a database field.\n"
     "Without a scope only the current record changes. Use ALL\n"
     "when every matching record should receive the new value.",
     "REPLACE CITY WITH 'AUSTIN'\nREPLACE ALL BAL WITH 0", "EDIT, BROWSE, GO"},
    {"DELETE", "Mark records for removal",
     "DELETE [<scope>] [FOR <condition>] [WHILE <condition>]",
     "Mark records as deleted. Their data stays in the file until\n"
     "PACK removes them. RECALL clears a deletion mark.\n"
     "Without a scope, DELETE marks only the current record.",
     "DELETE RECORD 2\nRECALL RECORD 2", "PACK, BROWSE, REPLACE"},
    {"PACK", "Remove deleted records",
     "PACK",
     "Remove records that have deletion marks and compact the\n"
     "database. Record numbers may change. Make a backup before\n"
     "packing; removed records can no longer be recalled.",
     "USE CLIENTS\nPACK", "DELETE, USE, INDEX"},
    {"INDEX", "Build an index file",
     "INDEX ON <expression> TO <file> [UNIQUE]",
     "Create an index for sorting and searching a database.\n"
     "The expression supplies the key. An active index changes\n"
     "the order in which commands visit records.",
     "INDEX ON NAME TO NAMES\nSEEK 'PESTON'", "SEEK, USE, SKIP"},
    {"SEEK", "Find an indexed value",
     "SEEK <expression>",
     "Search the active index for a key value. The expression\n"
     "must have the same type as the index key. FOUND() reports\n"
     "success; always check it before changing a found record.",
     "USE CLIENTS INDEX NAMES\nSEEK 'PESTON'", "INDEX, GO, SKIP"},
    {"GO", "Move to a record number",
     "GO <record number> / GO TOP / GO BOTTOM",
     "Move the record pointer. A number selects a physical record\n"
     "even when an index is active. TOP and BOTTOM select the\n"
     "first and last records in the active order. GOTO also works.",
     "GO 2\nDISPLAY", "SKIP, SEEK, DISPLAY"},
    {"SKIP", "Move by a number of records",
     "SKIP [<number>]",
     "Move forward through the active record order. The default\n"
     "is one record; a negative number moves backward. EOF()\n"
     "becomes true when you move past the last record.",
     "SKIP\nSKIP -1", "GO, SEEK, USE"},
    {"REPORT", "Print a formatted report",
     "REPORT FORM <file> [<scope>] [FOR <condition>]",
     "Use a saved report definition to lay out database records.\n"
     "CREATE REPORT makes the definition. TO PRINT sends the\n"
     "report to the printer; TO FILE saves the generated text.",
     "REPORT FORM CLIENTS\nREPORT FORM CLIENTS TO FILE CLIENTS.TXT", "LIST, USE, ASSIST"},
    {"ASSIST", "Use the menu Assistant",
     "ASSIST",
     "Choose a pad on the menu bar, then an option beneath it.\n"
     "The Assistant builds an ordinary database command. Open a\n"
     "file with Set Up before retrieving or updating records.",
     "ASSIST\nChoose Set Up, then Database file.", "USE, LIST, BROWSE"}
};

#define HELP_COUNT ((int)(sizeof(help_topics) / sizeof(help_topics[0])))

static void help_lines(kit *k, int row, const char *s)
{
    while (*s && row < 20) {
        const char *end = s;
        while (*end && *end != '\n') end++;
        kit_put(k, row++, 6, s, (int)(end - s));
        s = *end ? end + 1 : end;
    }
}

static void help_copy_topic(char *out, const char *in)
{
    int n = 0;
    while (*in == ' ' || *in == '\t') in++;
    while (*in && n < 32) {
        char c = *in++;
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        out[n++] = c;
    }
    while (n && (out[n-1] == ' ' || out[n-1] == '\t')) n--;
    out[n] = 0;
}

/* -1 = main menu, -2 = a named topic which is unavailable. */
static int help_find(const char *name)
{
    int i, n = 0;
    if (!*name || !rt_strncmp(name, "MAIN MENU", 10)) return -1;
    while (name[n] >= '0' && name[n] <= '9') n++;
    if (n && !name[n]) {
        int value = 0;
        for (i = 0; i < n && value <= HELP_COUNT; i++)
            value = value * 10 + name[i] - '0';
        if (value > 0 && value <= HELP_COUNT) return value - 1;
    }
    if (!rt_strncmp(name, "GOTO", 5)) return 11;
    if (rt_kw_match(name, "CHANGE")) return 4;
    for (i = 0; i < HELP_COUNT; i++)
        if (rt_kw_match(name, help_topics[i].name)) return i;
    return -2;
}

static void help_draw(kit *k, int topic, int selected, const char *unknown,
                      const char *input)
{
    int i;
    char number[4];
    kit_clear(k);
    kit_fill(k, 0, 0, KIT_COLS, ' ', KIT_ATTR_REVERSE);
    kit_put_attr(k, 0, 2, KIT_ATTR_REVERSE, "InitechBase III PLUS - Help", -1);
    kit_box(k, 2, 2, 21, 77, 0);
    kit_put_attr(k, 2, 5, KIT_ATTR_BRIGHT,
                 topic >= 0 ? help_topics[topic].name : "MAIN MENU", -1);
    if (topic == -1) {
        for (i = 0; i < HELP_COUNT; i++) {
            uint8_t attr = i == selected ? KIT_ATTR_REVERSE : KIT_ATTR_NORMAL;
            number[0] = i + 1 >= 10 ? '1' : ' ';
            number[1] = (char)('0' + (i + 1) % 10);
            number[2] = '.'; number[3] = 0;
            kit_fill(k, 4+i, 5, 69, ' ', attr);
            kit_put_attr(k, 4+i, 6, attr, number, -1);
            kit_put_attr(k, 4+i, 11, attr, help_topics[i].name, -1);
            kit_put_attr(k, 4+i, 25, attr, help_topics[i].summary, -1);
        }
    } else if (topic >= 0) {
        const help_topic *t = &help_topics[topic];
        kit_put_attr(k, 4, 5, KIT_ATTR_BRIGHT, "Syntax :", -1);
        help_lines(k, 5, t->syntax);
        kit_put_attr(k, 8, 5, KIT_ATTR_BRIGHT, "Description :", -1);
        help_lines(k, 9, t->description);
        kit_put_attr(k, 13, 5, KIT_ATTR_BRIGHT, "Example :", -1);
        help_lines(k, 14, t->example);
        kit_put_attr(k, 17, 5, KIT_ATTR_BRIGHT, "See also :", -1);
        help_lines(k, 18, t->see);
    } else {
        kit_put(k, 6, 6, "No help available for", -1);
        kit_put(k, 8, 6, unknown, -1);
        kit_put(k, 11, 6, "Type a command name, or press F10 for the main menu.", -1);
    }
    kit_put(k, 20, 5, "ENTER >", -1);
    kit_put_attr(k, 20, 13, KIT_ATTR_BRIGHT, input, -1);
    kit_message(k, 23, "Select - up/down, Enter.  Previous/next screen - PgUp/PgDn.");
    kit_message(k, 24, "F10 - main menu.  Esc - leave Help.  ENTER > accepts a topic or number.");
    kit_cursor(k, 20, 13 + (int)rt_strlen(input));
}

int ui_help(xb_interp *ip, const char *args, int *ec)
{
    kit k;
    char topic_name[33], input[33];
    int topic, selected = 0, key, n = 0;
    *ec = 0;
    help_copy_topic(topic_name, args);
    topic = help_find(topic_name);
    input[0] = 0;
    kit_open(&k, ip);
    for (;;) {
        help_draw(&k, topic, selected, topic_name, input);
        key = kit_key(&k);
        if (key == KIT_K_ESC || key == KIT_K_ABORT || key == KIT_K_EOF) break;
        if (key == KIT_K_F10 || key == KIT_K_HOME || key == KIT_K_F1) {
            topic = -1; n = 0;
        } else if (topic == -1 && (key == KIT_K_UP || key == KIT_K_DOWN)) {
            selected = (selected + (key == KIT_K_UP ? HELP_COUNT-1 : 1)) % HELP_COUNT;
        } else if (key == KIT_K_PGDN || key == KIT_K_PGUP) {
            if (key == KIT_K_PGDN) topic = topic < 0 ? 0 : (topic+1) % HELP_COUNT;
            else topic = topic <= 0 ? -1 : topic-1;
            n = 0;
        } else if (key == KIT_K_ENTER) {
            if (!n && topic == -1) topic = selected;
            else if (n) {
                help_copy_topic(topic_name, input);
                topic = help_find(topic_name);
            }
            n = 0;
        } else if (key == KIT_K_BKSP && n) n--;
        else if (key >= 32 && key <= 126 && n < 32) {
            if (key >= 'a' && key <= 'z') key -= 'a' - 'A';
            input[n++] = (char)key;
        }
        input[n] = 0;
    }
    kit_close(&k);
    return CMD_OK;
}
