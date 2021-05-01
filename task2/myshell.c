#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include "LineParser.h"

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

int debugFlag = 0;
process** myShellPL;

int isQuit (cmdLine* cmdLine);
int isCd (cmdLine* cmdLine);
int isProcs (cmdLine* cmdLine);
void changeDir(char* path);
void execute(cmdLine* pCmdLine, process** processList);

void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process* newProcess = (struct process*)malloc(sizeof(struct process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING; // init for 1?
    if (process_list == NULL) {
        newProcess->next = NULL;
        myShellPL = &newProcess;
    }
    else {
        newProcess->next = *process_list;
        *myShellPL = newProcess;
    }
    
}

void printProcessList(process** process_list) {
    process* currProc = *process_list;
    printf("PID\t Command\t STATUS\n");
    while (currProc != NULL && currProc->cmd-> /* SIG ERROR */) {
        printf("%d\t %s\t %d\n", currProc->pid, currProc->cmd->arguments[0], currProc->status);
        currProc = currProc->next;
    }
}

void execCommand(cmdLine* cmdLine, process** processList) {
    if (isCd(cmdLine)) changeDir(cmdLine->arguments[1]);
    else if (isProcs(cmdLine)) printProcessList(processList);

    else execute(cmdLine, processList);
}

void execute(cmdLine* pCmdLine, process** processList) {
    int pid, status;
    if (!(pid=fork())) {
        if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("");
        exit(1);
    }
    addProcess(processList, pCmdLine, pid);
    if (pCmdLine->blocking == 1) waitpid(pid, &status, 0);
}

int isQuit (cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "quit") == 0;
}

int isCd (cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "cd") == 0;
}

int isProcs (cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "procs") == 0;
}
void changeDir(char* path) {
    int status = chdir(path);
    if (status == -1) perror("Failed to change directory: ");
}

int main(int argc, char **argv) {
    char dirBuff[PATH_MAX];
    char userBuff[2048];
    cmdLine* currentLine;
    int quitFlag = 0;

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!quitFlag) {
        getcwd(dirBuff, PATH_MAX);
        printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        currentLine = parseCmdLines(userBuff);
        quitFlag = isQuit(currentLine);
        if (!quitFlag) execCommand(currentLine, myShellPL);
        freeCmdLines(currentLine);
    }
    return 0;
}