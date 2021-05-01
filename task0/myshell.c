#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include "LineParser.h"

int debugFlag = 0;

void execute(cmdLine* pCmdLine) {
    int pid, status;
    if (!(pid=fork())) {
        if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("");
        exit(1);
    }
    if (pCmdLine->blocking == 1) waitpid(pid, &status, 0);
}

int shouldQuit(cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "quit") == 0;
}

int isCd(cmdLine* cmdLine) {
    return strcmp(cmdLine->arguments[0], "cd") == 0;
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

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!shouldQuitLoop) {
        getcwd(dirBuff, PATH_MAX);
        printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        currentLine = parseCmdLines(userBuff);
        shouldQuitLoop = shouldQuit(currentLine);
        if (!shouldQuitLoop) {
            if (isCd(currentLine)) changeDir(currentLine->arguments[1]);
            else execute(currentLine);
        }
        freeCmdLines(currentLine);
    }
    return 0;
}