#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <X11/Xlib.h>

#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		50

#include "config.h"

void sighandler(int num);
void buttonhandler(int sig, siginfo_t *si);
void replace(char *str, char old, char new);
void remove_all(char *str, char to_remove);
void getcmds(int time);
#ifndef __OpenBSD
void getsigcmds(int signal);
void setupsignals();
void sighandler(int signum);
#endif
int getstatus(char *str, char *last);
void setroot();
void statusloop();
void termhandler();

static Display *dpy;
static int screen;
static Window root;
static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][256];
static int statusContinue = 1;
static void (*writestatus) () = setroot;

void
replace(char *str, char old, char new)
{
	for(char * c = str; *c; c++)
		if(*c == old)
			*c = new;
}

void
remove_all(char *str, char to_remove)
{
	char *read = str;
	char *write = str;
	while (*read ) {
		if (*read != to_remove) {
			*write++ = *read;
		}
		++read;
	}
	*write = '\0';
}

int
gcd(int a, int b)
{
	int temp;
	while (b > 0) {
		temp = a % b;
		a = b;
		b = temp;
	}
	return a;
}

/* opens process *cmd and stores output in *output */
void getcmd(const Block *block, char *output)
{
	if (block->signal) {
		output[0] = block->signal;
		output++;
	}
	char *cmd = block->command;
	FILE *cmdf = popen(cmd, "r");
	if (!cmdf)
		return;
	char tmpstr[CMDLENGTH] = "";
	char * s;
	int e;
	do {
		errno = 0;
		s = fgets(tmpstr, CMDLENGTH-(strlen(delim)+1), cmdf);
		e = errno;
	} while (!s && e == EINTR);
	pclose(cmdf);
	int i = strlen(block->icon);;
	strcpy(output, block->icon);
	strcpy(output+i, tmpstr);
	remove_all(output, '\n');
	i = strlen(output);
	if ((i > 0 && block != &block[LENGTH(blocks) -1])) {
		strcat(output, delim);
	}
	i += strlen(delim);
	output[i++] = '\0';
}

void getcmds(int time)
{
	const Block* current;
	for (size_t i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}
#ifndef __OpenBSD__
void getsigcmds(int signal)
{
	const Block *current;
	for (int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
}

void setupsignals()
{
	struct sigaction sa;

    	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        	signal(i, SIG_IGN);

	for (size_t i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal > 0) {
			signal(SIGRTMIN+blocks[i].signal, sighandler);
			sigaddset(&sa.sa_mask, SIGRTMIN+blocks[i].signal);
		}
	}
	sa.sa_sigaction = buttonhandler;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sa, NULL);
	struct sigaction sigchld_action = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_NOCLDWAIT
	};
	sigaction(SIGCHLD, &sigchld_action, NULL);
}
#endif
int getstatus(char *str, char *last)
{
	strcpy(last, str);
	str[0] = '\0';
	for (size_t i = 0; i < LENGTH(blocks); i++) {
		strcat(str, statusbar[i]);
		if (i == LENGTH(blocks) - 1)
			strcat(str, " ");
	}
	str[strlen(str) - 1] = '\0';
	/* 0 if they are the same */
	return strcmp(str, last);
}

void setroot()
{
	/* Only set root if text has changed. */
	if (!getstatus(statusstr[0], statusstr[1]))
		return;
	Display *d = XOpenDisplay(NULL);
	if (d) {
		dpy = d;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusstr[0]);
	XCloseDisplay(dpy);
}

void pstdout()
{
	/* Only write out if text has changed. */
	if (!getstatus(statusstr[0], statusstr[1]))
		return;
	printf("%s\n",statusstr[0]);
	fflush(stdout);
}

void statusloop()
{
#ifndef __OpenBSD__
	setupsignals();
#endif
	unsigned int interval = -1;
	for(size_t i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].interval) {
			interval = gcd(blocks[i].interval, interval);
		}
	}
	unsigned int i = 0;
	int interrupted = 0;
	const struct timespec sleeptime = {
		interval,
		0
	};
	struct timespec tosleep = sleeptime;
	getcmds(-1);
	while (statusContinue) {
		interrupted = nanosleep(&tosleep, &tosleep);
		if (interrupted == -1) {
			continue;
		}
		getcmds(i);
		writestatus();
		i += interval;
		tosleep = sleeptime;
	}
}

#ifndef __OpenBSD__
void sighandler(int signum)
{
	getsigcmds(signum-SIGRTMIN);
	writestatus();
}

void buttonhandler(int sig, siginfo_t *si)
{
	char button[2] = {'0' + si->si_value.sival_int & 0xff, '\0'};
	pid_t process_id = getpid();
	sig = si->si_value.sival_int >> 8;
	if (fork() == 0) {
		const Block *current;
		for (int i = 0; i < LENGTH(blocks); i++) {
			current = blocks + i;
			if (current->signal == sig) {
				break;
			}
		}
		char shcmd[1024];
		sprintf(shcmd, "%s && kill -%d %d", current->command, current->signal+34, process_id);
		char *command[] = { "/bin/sh", "-c", shcmd, NULL };
		setenv("BLOCK_BUTTON", button, 1);
		setsid;
		execvp(command[0], command);
		exit(EXIT_SUCCESS);
	}
}
#endif
void termhandler()
{
	statusContinue = 0;
	exit(0);
}

int main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {
		if (!strcmp("-d",argv[i]))
			delim = argv[++i];
		else if (!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
	statusloop();
}

