#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "parse.h"
#include "main.h"
#include "dexec.h"
#include "sssighand.h"

// list of currently running processes (one foreground, double linked list of background)
struct dprocess_list plist;

// flag whether background commands are able to run in background (SIGTSTP)
char backgroundCommandsEnabled = 1;

// ll of background messages to be printed before next prompt
struct backgroundStatusMessage * bmqHead = NULL;

// runs through the queue of background messages that need to be printed out before next :
void printBackgroundMessageq(){
    // used to help free messages
    struct backgroundStatusMessage * temp = NULL;
    while (bmqHead != NULL){
        // diffrent messages whether it was exited or terminated
        if(bmqHead->exited){
            printf("Background pid: %d exited with code: %d\n", bmqHead->pid, bmqHead->status);
        } else {
            printf("Background pid: %d terminated with code: %d\n", bmqHead->pid, bmqHead->status);
        }
        temp = bmqHead;
        bmqHead = bmqHead->next; 
        free(temp);
    }
}

// add the passed background status message to queue (bmqHead)
void queueBackgroundStatusMessage(struct backgroundStatusMessage * newMessage){
    if (bmqHead == NULL){
        bmqHead = newMessage;
    } else {
        struct backgroundStatusMessage * currentM = bmqHead;
        while(currentM->next != NULL){
            currentM = currentM->next;
        }
        currentM->next = newMessage;
        newMessage->next = NULL;
    }
}

// called when SIGTSTP is caught
void flipBackgroundStatus(int code){
    backgroundCommandsEnabled = !backgroundCommandsEnabled;
}

// called to add a foreground process's pid to the process list
// used with SIGCHLD to kill zombies
void addForegroundProcess(pid_t pid){

    plist.foregroundp = malloc(sizeof(struct dprocess));
    plist.foregroundp->pid = pid; 
    plist.foregroundp->next = NULL; 
    plist.foregroundp->last = NULL; 

}

// called to add a background process to process list
// used with SIGCHLD to kill zombies
void addBackgroundProcess(pid_t background_pid){
    
    struct dprocess * currentProcess;
    currentProcess = plist.backgroundp;

    // build ll of background processes
    if(currentProcess == NULL){
        plist.backgroundp = malloc(sizeof(struct dprocess));
        currentProcess = plist.backgroundp;
        currentProcess->next = NULL;
        currentProcess->last = NULL;
        currentProcess->pid = background_pid;
        return;
    }

    // get to last process to do our shtuf
    while (currentProcess->next != NULL){
        currentProcess = currentProcess->next;
    }

    currentProcess->next = malloc(sizeof(struct dprocess));
    currentProcess->next->last = currentProcess;
    currentProcess = currentProcess->next;
    currentProcess->next = NULL;
    currentProcess->pid = background_pid;

}

void removeProcess(pid_t pid, int childExitStatus){

    // check if the process being removed is a foreground process
    if (plist.foregroundp != NULL){
        if (plist.foregroundp->pid == pid){
            // if so, just free the process cause its already been waitpid'd in dexec.c
            free(plist.foregroundp);
            plist.foregroundp = NULL;
            return;
        }
    }

    // if we've gotten here, means we just waitpid'd a background procees
    // so we got to print out a message before next prompt \|/
    struct backgroundStatusMessage * newMessage = malloc(sizeof(struct backgroundStatusMessage));
    newMessage->next = NULL;
    if( WIFEXITED(childExitStatus) ){
        // child was exited
        int exit_status = WEXITSTATUS(childExitStatus);
        newMessage->status = exit_status;
        newMessage->exited = 1;
    } else {
        // child was terminated
        int term_signal = WTERMSIG(childExitStatus);
        newMessage->status = term_signal;
        newMessage->exited = 0;
    }
    newMessage->pid = pid;

    // queue the message to be printed
    queueBackgroundStatusMessage(newMessage);

    struct dprocess * currentProcess;
    currentProcess = plist.backgroundp;

    // search linked list for the process, remove if found
    if(plist.backgroundp == NULL){
        // should never be output
        write(1, "backgroundp is null\n", 22);
    } else {
        while (currentProcess->next != NULL){
            currentProcess = currentProcess->next;
        }
        if(currentProcess->last != NULL){
            currentProcess->last->next = currentProcess->next;
        }
        if(currentProcess->next != NULL){
            currentProcess->next->last = currentProcess->last;
        }
        free(currentProcess);
        if (currentProcess == plist.backgroundp){
            plist.backgroundp = NULL;
        }
        currentProcess = NULL;
    }
}

int main(int argc, char *argv[]){

    // init plist vars
    plist.foregroundp = NULL;
    plist.backgroundp = NULL;

    // buffer user input goes into and is parsed
    char inputBuffer[2048];

    // command struct, stores output of parsed buffer
    struct command dCommand;

    //  stores the status of the last foreground command
    struct lastForegroundStatus status;

    // inits ^'s vars
    status.status = 0;
    status.exited = 0;

    // put some of these in a single var as bit flags?
    // exit var, used to signal an exit
    char ttexit = 0;

    // sets up signal handlers
    // set the SIGINT signal to be ignored
    ignoreSig(SIGINT);
    // sets the child handler
    mainChildSig();
    // sets the foreground mode handler
    mainStopSig();

    // main loop
    while(ttexit == 0){
        // resets command var
        clearCommand(&dCommand);
        // gets the users input and stores it in the appropriate buffer
        getInput(inputBuffer);
        // parse the buffer into the command struct
        if(parseCommand(inputBuffer, &dCommand) == 0){
            // test for multiple built in commands

            // test for "exit command"
            if(strcmp(dCommand.argPtrs[0], "exit") == 0){
                ttexit = 1;
                break;

            // test for "cd command"
            } else if(strcmp(dCommand.argPtrs[0], "cd") == 0){
                if(dCommand.argPtrs[1] != NULL){
                    // change directory to first arg (just ignore the others lol)
                    // should have some error handling here, but i dont, oops
                    chdir(dCommand.argPtrs[1]);
                } else {
                    // finds the home dir env var and cds there
                    char * home = getenv("HOME");
                    chdir(home);
                }

            // test for status command
            } else if(strcmp(dCommand.argPtrs[0], "status") == 0){

                // print status of last exited foreground child
                if(status.exited){
                    printf("Exited with status: %d\n", status.status);
                } else {
                    printf("Terminated by code: %d\n", status.status);
                }

            // tests if parsed commaned should be run in background
            // and if were in foreground-only mode
            } else if(dCommand.doesExecInBackground && backgroundCommandsEnabled){
                dExecBackgroundCommand(&dCommand);
    
            // all else fails, this command should be run in the foreground
            } else {
                dExecForegroundCommand(&dCommand, &status);
            }
        }
        // prints any background messages in the queue before next prompt
        printBackgroundMessageq();
    }

    return EXIT_SUCCESS;

}