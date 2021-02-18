#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include "dexec.h"
#include "sssighand.h"
#include "main.h"

// called if foreground command needs to be ran
void dExecForegroundCommand(struct command * dCommand, struct lastForegroundStatus *status){
    pid_t spawnPid = -5;
    int childExitStatus = -5;
    int inputFileDescriptor = 0;
    int outputFileDescriptor = 0;

    // fork process
    spawnPid = fork();
    switch((spawnPid)){
        case -1:
            perror("Hull Breach\n");
            exit(1);
            break;
        // child
        case 0:
            // set signal handlers
            defaultSig(SIGINT);
            ignoreSig(SIGTSTP);

            // handle redirected input
            if(dCommand->doesChangeInput){
                inputFileDescriptor = open(dCommand->inputFile, O_RDONLY);
                if(inputFileDescriptor == -1){
                    perror("Failed to open file for input");
                    exit(1);
                    break;
                }
                if(dup2(inputFileDescriptor, 0) == -1){
                    perror("Failed to redirect stdio");
                    exit(1);
                    break;
                }
            }

            // handle redirected output
            if(dCommand->doesChangeOutput){
                outputFileDescriptor = open(dCommand->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(outputFileDescriptor == -1){
                    perror("Failed to open file for output");
                    exit(1);
                    break;
                }
                if(dup2(outputFileDescriptor, 1) == -1){
                    perror("Failed to redirect stdout");
                    exit(1);
                    break;
                }
            }

            // execute command in command struct
            execvp(dCommand->argPtrs[0], dCommand->argPtrs);
            perror("Executing the provided command failed");
            exit(1);
            break;
        default:
        // parent / smallsh
            fflush(stdout);

            // keep track of this dude
            addForegroundProcess(spawnPid);

            // clean up the 
            waitpid(spawnPid, &childExitStatus, 0);

            // find exit status and record it in *status
            if( WIFEXITED(childExitStatus) ){
                // child exited
                int exit_status = WEXITSTATUS(childExitStatus);
                status->status = exit_status;
                status->exited = 1;
            } else {
                // child was terminated
                int term_signal = WTERMSIG(childExitStatus);
                status->status = term_signal;
                status->exited = 0;
            }

            break;
    }
}

// called if background command needs to be ran
void dExecBackgroundCommand(struct command * dCommand){
    pid_t spawnPid = -5;
    int childExitStatus = -5;
    int inputFileDescriptor = 0;
    int outputFileDescriptor = 0;

    spawnPid = fork();
    switch((spawnPid)){
        case -1:
            perror("Hull Breach\n");
            exit(1);
            break;
        case 0:
        //child

            // set signal handlers
            ignoreSig(SIGINT);
            ignoreSig(SIGTSTP);

            // handle redirected input
            if(dCommand->doesChangeInput){
                inputFileDescriptor = open(dCommand->inputFile, O_RDONLY);
                if(inputFileDescriptor == -1){
                    perror("Failed to open file for input");
                    exit(1);
                    break;
                }
                if(dup2(inputFileDescriptor, 0) == -1){
                    perror("Failed to redirect stdio");
                    exit(1);
                    break;
                }
            } else {
            // redirect input to /dev/null if no output redirect was specified
                inputFileDescriptor = open("/dev/null", O_RDONLY);
                if(inputFileDescriptor == -1){
                    perror("Failed to open file for input");
                    exit(1);
                    break;
                }
                if(dup2(inputFileDescriptor, 0) == -1){
                    perror("Failed to redirect stdio");
                    exit(1);
                    break;
                }
            }

            // handle redirected output
            if(dCommand->doesChangeOutput){
                outputFileDescriptor = open(dCommand->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(outputFileDescriptor == -1){
                    perror("Failed to open file for output");
                    exit(1);
                    break;
                }
                if(dup2(outputFileDescriptor, 1) == -1){
                    perror("Failed to redirect stdout");
                    exit(1);
                    break;
                }
            } else {
                // redirect output to /dev/null if no output redirect was specified
                outputFileDescriptor = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(outputFileDescriptor == -1){
                    perror("Failed to open file for output");
                    exit(1);
                    break;
                }
                if(dup2(outputFileDescriptor, 1) == -1){
                    perror("Failed to redirect stdout");
                    exit(1);
                    break;
                }
            }

            // execute background command
            execvp(dCommand->argPtrs[0], dCommand->argPtrs);
            perror("Executing the provided command failed");
            exit(1);
            break;
        default:
            // keep track of background process
            addBackgroundProcess(spawnPid);
            printf("Background pid: %d\n", spawnPid);

            // carry on, waitpid in SIGCHLD catcher
            break;
    }
}