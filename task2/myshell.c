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
    cmdLine* cmd;                             /* the parsed command line*/
    pid_t pid; 		                          /* the process id that is running the command*/
    int status;                               /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

int debugFlag = 0;

int isQuit (cmdLine* cmdLine);
int isCd (cmdLine* cmdLine);
int isProcs (cmdLine* cmdLine);
void changeDir(char* path);
process* execute(cmdLine* pCmdLine, process** processList);
cmdLine* copyCmdLine (cmdLine* oldCmdLine);
void freeProcessList(process *process_list);
void updateProcessList(process *process_list);
void updateProcessStatus(process *process_list, int pid, int status);

process* addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process* newProcess = (struct process*)malloc(sizeof(struct process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    if (*process_list == NULL) { // first
        newProcess->next = NULL;
    }
    else {
        newProcess->next = *(process_list);
    }
    return newProcess;
}


process* printProcessList(process** process_list) {
    process* head = *process_list;
    char* status = "";
    updateProcessList(*process_list);
    printf("PID\t Command\t STATUS\n");
    if (process_list != NULL) {
        process* currProc = head;
        process *prevProc = NULL;
        while (currProc != NULL) {
            switch(currProc->status) {
                case (1) : status = "RUNNING"; break;
                case (0) : status = "SUSPENDED"; break;
                case (-1) : status = "TERMINATED"; break;
            }
            printf("%d\t %s\t %s\n", currProc->pid, currProc->cmd->arguments[0], status);
            /* Delete "freshly" terminated processes */
            if (currProc->status == TERMINATED) {
                if (prevProc == NULL) {
                    head = currProc->next;
                    freeCmdLines(currProc->cmd);
                    free(currProc);
                    currProc = head;
                }
                else {
                    prevProc->next = currProc->next;
                    freeCmdLines(currProc->cmd);
                    free(currProc);
                    currProc = prevProc->next;
                }
            }
            else {
                prevProc = currProc;
                currProc = currProc->next;
            }
        }
    }
    return head;
}

void freeProcessList(process *process_list){
    process *curr = process_list;
    process *prev = process_list;
    if (process_list != NULL) {
        while (curr != NULL) {
            freeCmdLines(curr->cmd);
            curr = curr->next;
            free(prev);
            prev = curr;
        }
    }
}

void updateProcessList(process *process_list){
    process* currProc = process_list;
    int status;
    pid_t w;
    while (currProc != NULL) {
        w = waitpid(currProc->pid, &status, WNOHANG | WCONTINUED | WUNTRACED);
        if (w >= 0) {
            if (WIFCONTINUED(status)) {
                currProc->status = RUNNING;
                printf("1\n");
            }
            else if (WIFSTOPPED(status)) {
                currProc->status = SUSPENDED;
                printf("2\n");
            }
            else if (WIFSIGNALED(status)) {
                currProc->status = TERMINATED;
                printf("3\n");
            }
        }
        else {
            perror("Error in waitpid call");
        }
        currProc = currProc->next;
    }
}

void updateProcessStatus(process *process_list, int pid, int status){
    process* currProc = process_list;
    while (currProc != NULL) {
        if (currProc->pid == pid)
            currProc->status = status;
            break;
        currProc = currProc->next;
    }
}

process* execCommand(cmdLine* cmdLine, process** processList) {
    if (isCd(cmdLine)) {
        changeDir(cmdLine->arguments[1]);
        freeCmdLines(cmdLine);
        return *processList;
    }
    else if (isProcs(cmdLine)) {
        freeCmdLines(cmdLine);
        return printProcessList(processList);
    }
    else return execute(cmdLine, processList);
}

process* execute(cmdLine* pCmdLine, process** processList) {
    int pid, status;
    process* temp = *processList;
    if (!(pid=fork())) {
        if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);

        if (strcmp(pCmdLine->arguments[0], "suspend") == 0) {
            pid = atoi(pCmdLine->arguments[1]);
            freeCmdLines(pCmdLine);
            if (kill(pid, SIGTSTP) < 0)
                perror("Failed to suspend");
            else 
                updateProcessStatus(temp, pid, SUSPENDED);
            sleep(atoi(pCmdLine->arguments[2]));
            if (kill(pid, SIGCONT) < 0)
                perror("Failed to continue");
            else 
                updateProcessStatus(temp, pid, RUNNING);
        }

        else if (strcmp(pCmdLine->arguments[0], "kill") == 0) {
            pid = atoi(pCmdLine->arguments[1]);
            freeCmdLines(pCmdLine);
            if (kill(pid, SIGINT) < 0)
                perror("Failed to kill");
            else
                updateProcessStatus(temp, pid, TERMINATED);
        } 
        else {
            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            perror("");
            _exit(1);
        }
    }
    
    if (!(strcmp(pCmdLine->arguments[0], "suspend") == 0 || strcmp(pCmdLine->arguments[0], "kill") == 0)){
        temp = addProcess(processList, pCmdLine, pid);
    }
    
    if (pCmdLine->blocking == 1) waitpid(pid, &status, 0);
    return temp;
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
    // char dirBuff[PATH_MAX];
    char userBuff[2048];
    cmdLine* currentLine;
    int quitFlag = 0;
    process* myShellPL;

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!quitFlag) {
        // getcwd(dirBuff, PATH_MAX);
        // printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        currentLine = parseCmdLines(userBuff);
        quitFlag = isQuit(currentLine);
        if (!quitFlag) myShellPL = execCommand(currentLine, &myShellPL);
    }
    freeProcessList(myShellPL);
    return 0;
}