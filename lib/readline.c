#include <inc/stdio.h>
#include <inc/error.h>

#include <inc/string.h>

#define BUFLEN 1024
static char buf[BUFLEN];

static bool already_history = false;
static char history[BUFLEN];
static void history_dispatch(char, int *);
static void commit_history(void);
static char *get_history(char, int *);

char *
readline(const char *prompt)
{
	int i, c, echoing;

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("\e[0;32m%s\e[0m", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = 0;
	echoing = iscons(0);
	while (1) {
		c = getchar();
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			if (echoing)
				cputchar('\b');
			i--;
		} else if (c == 0x1b) {
            if ((c = getchar()) != 0x5b)
                cprintf("err: arrow up\n");
            c = getchar();
            if (c == 0x41 || c == 0x42) {
                history_dispatch(c, &i);
            } else {
                continue;
            }
        } else if (c == 0xE2 || c == 0xE3) {
            history_dispatch(c, &i);
        } else if (c >= ' ' && i < BUFLEN-1) {
			if (echoing){
				cprintf("\e[35m");
				cputchar(c);
				cprintf("\e[0m");
			}
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			if (echoing)
				cputchar('\n');
			buf[i] = 0;
            commit_history();
			return buf;
		}
	}
}

static void
history_dispatch(char c, int *p)
{
    char *last_cmd = get_history(c, p);
    cprintf("\e[35m");
    cprintf("%s", last_cmd);
    cprintf("\e[0m");
    strcpy(&buf[*p], last_cmd);
    *p += strlen(last_cmd);
}

static char *
get_history(char c, int *p) 
{
    if (*p == 0) {
        already_history = true;
        return history;
    }
    if (already_history)
        return "";
    else {
        already_history = true;
        return history;
    }
}

static void
commit_history(void)
{
    already_history = false;
    strcpy(history, buf);
}
