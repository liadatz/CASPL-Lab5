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
void removeTerminatedProcess(process** process_list);
void printProcessList(process** process_list);
void freeProcessList(process* process_list);
void updateProcessList(process **process_list);
void updateProcessStatus(process* process_list, int pid, int status);


void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* newProcess = (struct process*)malloc(sizeof(struct process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *myShellPL;
    *myShellPL = newProcess;
}

void removeTerminatedProcess(process** process_list){
    process *curr = *process_list;
    process *prev = NULL;
    while (curr != NULL){
        if (curr->status == TERMINATED){
            if (prev == NULL){
                myShellPL = &curr->next;
                freeCmdLines(curr->cmd);
                free(curr);
                curr = *myShellPL;
            }
            else {
                prev->next = curr-> next;
                freeCmdLines(curr->cmd);
                free(curr);
                curr = prev->next;
            }
        }
        else {
            prev = curr;
            curr = curr->next;
        }
    }
}


void printProcessList(process** process_list) {
    updateProcessList(process_list);
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
    removeTerminatedProcess(process_list);
}

void freeProcessList(process* process_list) {
    process *curr = process_list;
    process *temp = process_list;
    if (process_list != NULL) {
        while (curr != NULL) {
            freeCmdLines(curr->cmd);
            curr = curr->next;
            free(temp);
            temp = curr;
        }
    }
}

void updateProcessList(process **process_list){
    process* currProc = *process_list;
    int status = 0;
    int w;
    while (currProc != NULL) {
        w = waitpid(currProc->pid, &status, WNOHANG | WCONTINUED | WUNTRACED);
        if (w > 0){
            if (WIFCONTINUED(status)) {
                currProc->status = RUNNING;
            }
            else if (WIFSTOPPED(status)) {
                currProc->status = SUSPENDED;
            }
            else if (WIFSIGNALED(status)) {
                currProc->status = TERMINATED;
            }
        }
        currProc = currProc->next;       
    }    
}

void updateProcessStatus(process* process_list, int pid, int status){
    process* currProc = process_list;
    while (currProc != NULL) {
        if (currProc->pid == pid)
            currProc->status = status;
            break;
        currProc = currProc->next;
    }
}


void execute(cmdLine* pCmdLine) {
    int status = 0;
    int pid = fork();
    if (pid == 0) {
        if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
        if (strcmp(pCmdLine->arguments[0], "suspend") == 0) {
            pid = atoi(pCmdLine->arguments[1]);
            freeCmdLines(pCmdLine);
            if (kill(pid, SIGTSTP) < 0)
                perror("Failed to suspend");
            else 
                updateProcessStatus(*myShellPL, pid, SUSPENDED);
            sleep(atoi(pCmdLine->arguments[2]));
            if (kill(pid, SIGCONT) < 0)
                perror("Failed to continue");
            else 
                updateProcessStatus(*myShellPL, pid, RUNNING);
        }

        else if (strcmp(pCmdLine->arguments[0], "kill") == 0) {
            pid = atoi(pCmdLine->arguments[1]);
            freeCmdLines(pCmdLine);
            if (kill(pid, SIGINT) < 0)
                perror("Failed to kill");
            else
                updateProcessStatus(*myShellPL, pid, TERMINATED);
        }

        else {
            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            perror("");
            _exit(1);
        } 
    }
    else if (pid == -1) {
        perror("Failed Forking: ");
        return;
    }
    else {
        if (!(strcmp(pCmdLine->arguments[0], "suspend") == 0 || strcmp(pCmdLine->arguments[0], "kill") == 0))
            addProcess(myShellPL, pCmdLine, pid);
        if (pCmdLine->blocking) {
            waitpid(pid, &status, 0);
            updateProcessStatus(*myShellPL, pid, TERMINATED);
        }
    }
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