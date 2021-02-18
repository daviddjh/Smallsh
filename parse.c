#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "parse.h"

void getInput(char * inputBuffer){

    char * newline;
    memset(inputBuffer, '\0', 2048);
    printf(" : ");
    fgets(inputBuffer, 2048, stdin);

    // the below code block was inspired by a stack overflow post here: https://stackoverflow.com/questions/38767967/clear-input-buffer-after-fgets-in-c
    // it clears the stdin buffer so there are no left over chars to process
    // also removes new line from user input
    if(newline = strstr(inputBuffer, "\n")) {
        *newline = 0;
    } else {
        scanf("%*[^\n]");scanf("%*c");  //clear upto newline
    }
}

char clearCommand(struct command *newCommand){
    newCommand->doesChangeInput = 0;
    newCommand->doesChangeOutput = 0;
    newCommand->doesExecInBackground = 0;
    for (int i = 0; i < 513; i++){
        newCommand->argPtrs[i] = NULL;
    } 
    memset(newCommand->args, '\0', 2048);
    memset(newCommand->inputFile, '\0', 256);
    memset(newCommand->outputFile, '\0', 256);
}

// parses the buffer arg into a command struct, byte by byte
char parseCommand(char * commandBuffer, struct command *newCommand){
    // sets the current byte
    char * currentBufferChar = commandBuffer;
    // count of the number of args that are in the command
    int argCount = 0;           // doesn't include last null, needed for execvp
    // current index of newCommand->args
    int argsIndex = 0;
    // string of smallsh pid
    char smallshPID[7] = {0};
    sprintf(smallshPID, "%d", getpid());

    // loop until end of string in buffer 
    while (*currentBufferChar != '\0'){

        // first char of an arg
        if(*currentBufferChar != ' '){
            // logic for inserting PID of process from $$
            if(*currentBufferChar == '$'){
                currentBufferChar++;
                if(*currentBufferChar == '$'){
                    newCommand->argPtrs[argCount] = &(newCommand->args[argsIndex]);
                    for (int i = 0; i < strlen(smallshPID); i++){
                        // add pid char to arg char array
                        newCommand->args[argsIndex] = smallshPID[i];
                        argsIndex++;
                    }
                    currentBufferChar++;
                } else {
                    currentBufferChar--;
                    // add current char to arg char array
                    newCommand->args[argsIndex] = *currentBufferChar;
                    argsIndex++;
                    currentBufferChar++;
                }
            } else {
                // add current char to arg char array
                newCommand->args[argsIndex] = *currentBufferChar;
                // sets a pointer to the current string 
                newCommand->argPtrs[argCount] = &(newCommand->args[argsIndex]);
                // bumps indicies
                argsIndex++;
                currentBufferChar++;
            }

            // parsing rest of command / args
            while (*currentBufferChar != ' ' && *currentBufferChar != '\0'){
                // logic for inserting PID of process from $$
                if(*currentBufferChar == '$'){
                    currentBufferChar++;
                    if(*currentBufferChar == '$'){
                        for (int i = 0; i < strlen(smallshPID); i++){
                            newCommand->args[argsIndex] = smallshPID[i];
                            argsIndex++;
                        }
                        currentBufferChar++;
                    } else {
                        currentBufferChar--;
                        // add current char ($) to arg char array
                        newCommand->args[argsIndex] = *currentBufferChar;
                        // bumps indicies
                        argsIndex++;
                        currentBufferChar++;
                    }
                } else {
                    // add current char to arg char array
                    newCommand->args[argsIndex] = *currentBufferChar;
                    // bumps indicies
                    argsIndex++;
                    currentBufferChar++;
                }
            }
            // sets the 0 byte for the end of the arg/command string
            newCommand->args[argsIndex] = '\0';
            argsIndex++;

            // tests for if the last arg is < or >
            if(strcmp(newCommand->argPtrs[argCount], "<") == 0){
                // if true, continue parsing, but add chars to strings in command struct

                currentBufferChar++;

                // flip flag for future use
                newCommand->doesChangeInput = 1;

                // < wasnt an actual arg, dont keep track of it (exec)
                newCommand->argPtrs[argCount] = NULL;

                // index of inputFile string
                int inputFileIndex = 0;
                // NEED TO CHECK FILE SIZE HERE IDK IF I CAN ASSUME THIS WORK IS 512 long
                while (*currentBufferChar != ' ' && *currentBufferChar != '\0'){
                    // $$ logic
                    if(*currentBufferChar == '$'){
                        currentBufferChar++;
                        if(*currentBufferChar == '$'){
                            for (int i = 0; i < strlen(smallshPID); i++){
                                newCommand->inputFile[inputFileIndex] = smallshPID[i];
                                inputFileIndex++;
                            }
                            currentBufferChar++;
                        } else {
                            currentBufferChar--;
                            newCommand->inputFile[inputFileIndex] = *currentBufferChar;
                            inputFileIndex++;
                            currentBufferChar++;
                        }
                    } else {
                        // parse chars into input file string in command struct
                        newCommand->inputFile[inputFileIndex] = *currentBufferChar;
                        currentBufferChar++;
                        inputFileIndex++;
                    }
                }
                // end string
                newCommand->inputFile[inputFileIndex] = '\0';

            } else if(newCommand->argPtrs[argCount] != NULL){
                if(strcmp(newCommand->argPtrs[argCount], ">") == 0){
                    currentBufferChar++;
                    newCommand->doesChangeOutput = 1;
                    newCommand->argPtrs[argCount] = NULL;
                    int outputFileIndex = 0;
                    // NEED TO CHECK FILE SIZE HERE IDK IF I CAN ASSUME THIS WORK IS 512 long
                    while (*currentBufferChar != ' ' && *currentBufferChar != '\0'){
                        if(*currentBufferChar == '$'){
                            currentBufferChar++;
                            if(*currentBufferChar == '$'){
                                for (int i = 0; i < strlen(smallshPID); i++){
                                    newCommand->outputFile[outputFileIndex] = smallshPID[i];
                                    outputFileIndex++;
                                }
                                currentBufferChar++;
                            } else {
                                currentBufferChar--;
                                newCommand->outputFile[outputFileIndex] = *currentBufferChar;
                                outputFileIndex++;
                                currentBufferChar++;
                            }
                        } else {
                            newCommand->outputFile[outputFileIndex] = *currentBufferChar;
                            currentBufferChar++;
                            outputFileIndex++;
                        }
                    }
                    newCommand->outputFile[outputFileIndex] = '\0';
                } else {
                    argCount++; // shouldn't go up if > or < 
                }
            }

        }
        if( *currentBufferChar != '\0')
            currentBufferChar++;

    }
    
    // check for empty command or comment command
    if(newCommand->argPtrs[0] != NULL && *newCommand->argPtrs[0] != '#'){
        // check for & to make background process
        if(strcmp(newCommand->argPtrs[argCount-1], "&") == 0){
            newCommand->doesExecInBackground = 1;
            newCommand->argPtrs[argCount-1] = NULL;
            argCount--;
        }
        newCommand->args[argsIndex] = '\0';
        return 0;
    } else {
        return 1; // aka don't run
    }
}