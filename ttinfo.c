/* ttinfo displays information about a process, group or tty 
 *
 * Copyright (c) 2021  Joachim Wiberg <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

/* see proc(5) for details on the /proc/[pid]/stat format */
struct procstat {
	int           pid;       
	char          comm[32];  
	char          state;     
	int           ppid;    
	int           pgrp;    
	int           sid;     
	int           tty;     
	int           tpgid;   
	unsigned int  flags;   
};

static char *find_tty(int dev)
{
	static char name[42];

	if (major(dev) == 136) {	/* /dev/pts/N */
		struct stat st;

		snprintf(name, sizeof(name), "/dev/pts/%d", minor(dev));
		if (stat(name, &st) == -1) {
			warn("stat(%s)", name);
			return NULL;
		}
	} else {
		char buf[256];
		char path[80];
		ssize_t len;

		snprintf(path, sizeof(path), "/sys/dev/char/%d:%d", major(dev), minor(dev));
		len = readlink(path, buf, sizeof(buf));
		if (len == -1) {
			if (errno != ENOENT)
				warn("readlink(%s)", path);
			return NULL;
		}
		buf[len] = 0;

		snprintf(name, sizeof(name), "/dev/%s", basename(buf));
	}

	return name;
}

static int filter(const struct dirent *d)
{
	if (!isdigit(d->d_name[0]))
		return 0;
	return 1;
}

static char *token(char **line, char *sep)
{
	char *tok;

	tok = strtok(*line, sep);
	*line = NULL;

	return tok;
}

#define TOKEN(s)  tok = token(&line, s)
#define TOKINT(s) atoi(token(&line, s) ?: "-1")

static int getproc(int pid, struct procstat *p)
{
	char buf[512];
	char fn[512];
	FILE *fp;

	snprintf(fn, sizeof(fn), "/proc/%d/stat", pid);
	fp = fopen(fn, "r");
	if (!fp)
		return -1;

	if (!p) {
		fclose(fp);
		return 0;	/* pid exists */
	}

	if (fgets(buf, sizeof(buf), fp)) {
		char *line = buf;
		char *tok;

		p->pid = TOKINT(" \t");   /* 1 */

		TOKEN("()");		  /* 2 */
		if (tok)
			snprintf(p->comm, sizeof(p->comm), "%s", tok);

		TOKEN(" \t");		  /* 3 */
		if (tok)
			p->state = *tok;

		p->ppid  = TOKINT(" \t"); /* 4 */
		p->pgrp  = TOKINT(" \t"); /* 5 */
		p->sid   = TOKINT(" \t"); /* 6 */
		p->tty   = TOKINT(" \t"); /* 7 */
		p->tpgid = TOKINT(" \t"); /* 8 */
		p->flags = TOKINT(" \t"); /* 9 */
	}
	fclose(fp);

	return 0;
}

static int show(pid_t pid, const struct dirent *d)
{
	struct procstat p;

	if (getproc(atoi(d->d_name), &p))
		return -1;

	if (pid == p.sid || pid == p.pgrp) {
		char *tty = find_tty(p.tty);

		if (!tty)
			tty = "0:0";
		printf(" %s(%d %d %d %s) ", p.comm, p.pid, p.pgrp, p.sid, tty);
	}

	return 0;
}

static int find_ppid(pid_t pid)
{
	struct procstat p;

	if (getproc(pid, &p))
		return -1;

	return p.ppid;
}

static int find_dev(pid_t pid)
{
	struct procstat p;

	if (getproc(pid, &p))
		return -1;

	return p.tty;
}

static int pid_exists(pid_t pid)
{
	return !getproc(pid, NULL);
}

static int list(pid_t sid)
{
	struct dirent **d;
	int i, n;

	n = scandir("/proc", &d, filter, alphasort);
	if (n < 0) {
		puts("");
		return -1;
	}

	for (i = 0; i < n; i++) {
		show(sid, d[i]);
		free(d[i]);
	}
	if (d)
		free(d);
	puts("");

	return 0;
}

static int usage(int rc)
{
	printf("usage: ttinfo [-g pgid] [-p pid] [-s sid] [TTY]\n");
	return rc;
}

int main(int argc, char *argv[])
{
	char *tty  = NULL;
	pid_t pgid = -1;
	pid_t sid  = -1;
	pid_t pid  = -1;
	pid_t tgid = -1;
	pid_t ppid = -1;
	int fd, c;

	while ((c = getopt(argc, argv, "g:h?p:s:")) != EOF) {
		switch(c) {
		case 'h':
		case '?':
			return usage(0);

		case 'g':
			pgid = atoi(optarg);
			break;

		case 'p':
			pid = atoi(optarg);
			break;

		case 's':
			sid = atoi(optarg);
			break;

		default:
			return usage(1);
		}
	}

	if (pid != -1 && !pid_exists(pid))
		errx(1, "cannot find pid %d", pid);

	if (optind < argc)
		tty = argv[optind];
	else {
		if (pid != -1) {
			int dev;

			dev = find_dev(pid);
			if (dev != -1)
				tty = find_tty(dev);
		} else {
			tty = ttyname(STDIN_FILENO);
			if (!tty)
				tty = "/dev/tty";
		}
	}

	if (tty) {
		fd = open(tty, O_RDWR | O_NONBLOCK);
		if (fd == -1)
			warn("failed opening %s", tty);
		else {
			tgid = tcgetpgrp(fd);
			close(fd);
		}
	}

	if (pid == -1) {
		pid = getpid();
		ppid = getppid();
	} else
		ppid = find_ppid(pid);
	
	if (pgid == -1)
		pgid = getpgid(pid);
	if (sid == -1)
		sid = getsid(pid);

	printf("TeleType device    | TTY  : %s\n", tty ?: "0:0");
	printf("Process ID         | PID  : %d\n", pid);
	printf("Process group ID   | PGID : %d\n", pgid);
	printf("Parent process ID  | PPID : %d\n", ppid);
	printf("Session ID         | SID  : %d\n", sid);
	printf("Foreground PGID    | TGID : %d\n", tgid);
	printf("Procs in same SID  | "); list(sid);
	printf("Procs in same PGID | "); list(pgid);

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
