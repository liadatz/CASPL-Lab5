#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

void handler(int sig);
void handler2(int sig);

void handler(int sig) {
	printf("Caught signal %s\n", strsignal(sig));
	signal(sig, SIG_DFL);
	if (sig == SIGTSTP) signal(SIGCONT, handler);
	else if (sig == SIGCONT) signal(SIGTSTP, handler);
	raise(sig);
}

// void handler2(int sig) {
// 	if (sig == SIGTSTP) signal(SIGCONT, handler); /* if sig is SIGSTP */
// 	else if (sig == SIGCONT) signal(SIGTSTP, handler); /* if sig is SIGCONT */
// }

int main(int argc, char **argv){ 
	printf("Starting the program\n");
	signal(SIGTSTP, handler); /* SIGSTP */
	signal(SIGINT, handler); /* SIGINT */
	signal(SIGCONT, handler); /* SIGCONT */
	
	while(1) {
		sleep(2);
	}

	return 0;
}