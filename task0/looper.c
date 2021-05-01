#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

void handler(int sig) {
	printf("Caught signal %d\n", sig);
	signal(sig, SIG_DFL);
	handler2(sig);
	raise(sig);
}

void handler2(char* sig) {
	if (sig == 19) signal(strsignal(18), handler); /* if sig is SIGCONT */
	else if (sig == 18) signal(strsignal(19), handler); /* if sig is SIGSTP */
}

int main(int argc, char **argv){ 
	printf("Starting the program\n");
	signal(strsignal(2), handler); /* SIGINT */
	signal(strsignal(18), handler); /* SIGSTP */
	signal(strsignal(19), handler); /* SIGCONT */

	while(1) {
		sleep(2);
	}

	return 0;
}