#ifndef SSSIGHAND
#define SSSIGHAND

void ignoreSig(int signum);
void selfTermNPrint(int code);
void defaultSig(int signum);
void mainStopSig();
void mainChildSig();
void childSigHandler(int code, siginfo_t* info, void* );

#endif