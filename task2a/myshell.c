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

void addProcess(process** process_list, cmdLine* cmd, pid_t pid);
void printProcessList(process** process_list);
void freeProcessList(process* process_list);
void updateProcessList(process **process_list);

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* newProcess = (struct process*)malloc(sizeof(struct process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *myShellPL;
    *myShellPL = newProcess;
}

void printProcessList(process** process_list) {
    process* currProc = *process_list;
    char* status = "";
    printf("PID\t\tCommand\t\tSTATUS\n");
    while (currProc != NULL) {
        switch(currProc->status) {
            case (1) : status = "RUNNING"; break;
            case (0) : status = "SUSPENDED"; break;
            case (-1) : status = "TERMINATED"; break;
        }
        printf("%d\t\t%s\t\t%s\n", currProc->pid, currProc->cmd->arguments[0], status);
        currProc = currProc->next;       
    }
}

void execute(cmdLine* pCmdLine) {
    int pid, status;
    if (!(pid=fork())) {
        if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("");
        exit(1);
    }
    if (pCmdLine->blocking == 1) waitpid(pid, &status, 0);
    addProcess(myShellPL, pCmdLine, pid);
}

int shouldQuit(cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "quit") == 0;
}

int isCd(cmdLine* cmdLine) {
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
    int shouldQuitLoop = 0;
    myShellPL = (process**)malloc(sizeof(process));

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!shouldQuitLoop) {
        getcwd(dirBuff, PATH_MAX);
        printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        currentLine = parseCmdLines(userBuff);
        shouldQuitLoop = shouldQuit(currentLine);
        if (!shouldQuitLoop) {
            if (isCd(currentLine)){ changeDir(currentLine->arguments[1]); freeCmdLines(currentLine);}
            else if (isProcs(currentLine)) {printProcessList(myShellPL); freeCmdLines(currentLine);}
            else execute(currentLine);
        }
    }
    return 0;
}