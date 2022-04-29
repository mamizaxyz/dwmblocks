/* Modify this file to change what commands output to your statusbar,
   and recompile using the make command. */

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;

static const Block blocks[] = {
	/* Icon */	/* Command */		/* Update Interval */	/* Update Signal */
	{ "",           "sb-pacup",		3600,			8  },
	{ "",           "sb-lastup",		86400,			8  },
	/* { "",           "sb-cpu",		10,			18 }, */
	/* { "",           "sb-memory",		10,			14 }, */
	{ "",           "sb-volume",		0,			10 },
	{ "",           "sb-battery",		5,			3  },
	{ "",           "sb-nettraf",		1,			16 },
	{ "",           "sb-internet",		5,			4  },
	{ "",           "sb-clock",		60,			1  },
	{ "",           "sb-xkbmap",		0,			15 },
};

/* sets delimeter between status commands. NULL character ('\0') means no delimeter. */
static char *delim = " / ";
