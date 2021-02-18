#ifndef DMAIN
#define DMAIN
#include <sys/types.h>

extern struct dprocess_list plist;
extern char backgroundCommandsEnabled;
extern struct backgroundStatusMessage * head;

struct lastForegroundStatus {
    int status;
    char exited; // 1 = yes, 0 = no (terminated)
};

struct backgroundStatusMessage {
    pid_t pid;
    int status;
    char exited; // 1 = yes, 0 = no (terminated)
    struct backgroundStatusMessage * next;
};

struct dprocess {
    pid_t pid;
    struct dprocess * next;
    struct dprocess * last;
};

struct dprocess_list {
    struct dprocess *foregroundp;
    struct dprocess *backgroundp;
};

void addForegroundProcess(pid_t pid);
void addBackgroundProcess(pid_t pid);
void removeProcess(pid_t pid, int exitStatus);
void flipBackgroundStatus(int);
void queueBackgroundStatusMessage(struct backgroundStatusMessage *);
void printBackgroundMessageq();
#endif