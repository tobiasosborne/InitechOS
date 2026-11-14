/* Shipping projection of the WP5.1 template. The full historical names and
 * aliases remain in fkeys.h/fkeys.json. Approved GM6 omission list: README. */
#ifndef IWORD_COMMANDS_H
#define IWORD_COMMANDS_H
#include "fkeys.h"
static const unsigned char IW_COMMAND_KEYS[4][12] = {
 {IW_FK_undelete,IW_FK_search_forward,IW_FK_unmapped,IW_FK_indent,IW_FK_list,IW_FK_bold,IW_FK_exit,IW_FK_underline,IW_FK_unmapped,IW_FK_save,IW_FK_reveal,IW_FK_block},
 {IW_FK_setup,IW_FK_search_backward,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_format,IW_FK_unmapped,IW_FK_retrieve,IW_FK_unmapped,IW_FK_unmapped},
 {IW_FK_unmapped,IW_FK_replace,IW_FK_reveal,IW_FK_block,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped},
 {IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_move,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_font,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped,IW_FK_unmapped}
};
#endif
