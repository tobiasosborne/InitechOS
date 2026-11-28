/* Temporary resident COMMAND.COM boundary. No kernel data exposed. */
#ifndef INITECH_COMMAND_ENTRY_H
#define INITECH_COMMAND_ENTRY_H
#include <stdint.h>
enum { COMMAND_ENV_DEFAULT_BYTES = 160, COMMAND_ENV_MAX_BYTES = 3840 };
void command_repl(int permanent, uint16_t env_bytes);
void command_exec_secondary(const char *tail, uint32_t len);
#endif
