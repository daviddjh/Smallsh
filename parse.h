#ifndef DPARSE
#define DPARSE

struct command {
    char args[2048];            // first arg is command name
    char inputFile[256];
    char outputFile[256];
    char *argPtrs[513];          // array of char*s 512 proper args + command
    char doesChangeInput;
    char doesChangeOutput;
    char doesExecInBackground;
};

void getInput(char * inputBuffer);
char parseCommand(char * commandBuffer, struct command *newCommand);
char clearCommand(struct command *newCommand);

#endif