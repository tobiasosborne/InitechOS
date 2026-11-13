/* Temporary resident COMMAND.COM boundary. No kernel data exposed. */
#ifndef INITECH_COMMAND_ENTRY_H
#define INITECH_COMMAND_ENTRY_H
#include <stdint.h>
void command_repl(int permanent);
void command_exec_secondary(const char *tail, uint32_t len);
#endif
