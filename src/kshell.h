#ifndef KSHELL_H
#define KSHELL_H

#include "types.h"
#include "keyboard.h"

#define READLINE_SIZE 256
#define PROMPT "$> "

void kshell();
void kshell_run(u8 scancode);

#endif
