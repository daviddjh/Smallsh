#ifndef DEXEC
#define DEXEC
#include "parse.h"
#include "main.h"

void dExecForegroundCommand(struct command * dCommand, struct lastForegroundStatus *status);
void dExecBackgroundCommand(struct command * dCommand);
#endif