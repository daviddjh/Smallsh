#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "sssighand.h"
#include "main.h"

// ignore the sig arg
void ignoreSig(int signum){
    struct sigaction ignoreSig = {0};

    ignoreSig.sa_handler = SIG_IGN;

    sigaction(signum, &ignoreSig, NULL);
}

// use default responce to sig arg
void defaultSig(int signum){
    struct sigaction defaultSig = {0};
    sigfillset(&defaultSig.sa_mask);
    defaultSig.sa_flags = 0;

    defaultSig.sa_handler = SIG_DFL;

    sigaction(signum, &defaultSig, NULL);
}

// catch SIGTSTP with a command that flips a flag that controlls foreground mode
void mainStopSig(){
    struct sigaction stopSig = {0};
    sigfillset(&stopSig.sa_mask);
    stopSig.sa_flags = SA_RESTART;

    // replace with custom fuction in main.c that manipulates a variable that controls background job avaliblitliy and prints
    stopSig.sa_handler = flipBackgroundStatus; 
    sigaction(SIGTSTP, &stopSig, NULL);
}

// catch child sigs and dispose of properly
void mainChildSig(){
    struct sigaction childSig = {0};
    sigfillset(&childSig.sa_mask);
    childSig.sa_flags = SA_SIGINFO | SA_RESTART;
    childSig.sa_sigaction = childSigHandler;
    sigaction(SIGCHLD, &childSig, NULL);
}

// remove child from process list in main.c
void childSigHandler(int sig, siginfo_t* info, void* context){
    int childExitStatus;
    waitpid(info->si_pid, &childExitStatus, 0);
    removeProcess(info->si_pid, childExitStatus);
}