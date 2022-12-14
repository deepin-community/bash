/*
 * This file is in the public domain.
 * You may freely use, modify, distribute, and relicense it.
 */

#include "bash.preinst.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>

extern char **environ;

__attribute__((format(printf, 1, 0)))
static void vreportf(const char *err, va_list params, int errnum)
{
	fprintf(stderr, "bash.preinst: ");
	vfprintf(stderr, err, params);
	if (errnum)
		fprintf(stderr, ": %s", strerror(errnum));
	fprintf(stderr, "\n");
}

__attribute__((format(printf, 1, 2)))
NORETURN void die_errno(const char *fmt, ...)
{
	va_list params;
	va_start(params, fmt);
	vreportf(fmt, params, errno);
	va_end(params);
	exit(1);
}

__attribute__((format(printf, 1, 2)))
NORETURN void die(const char *fmt, ...)
{
	va_list params;
	va_start(params, fmt);
	vreportf(fmt, params, 0);
	va_end(params);
	exit(1);
}

int exists(const char *file)
{
	struct stat sb;
	if (!lstat(file, &sb))
		return 1;
	if (errno == ENOENT)
		return 0;
	die_errno("cannot get status of %s", file);
}

static void wait_or_die(pid_t child, const char *name)
{
	int status;
	if (waitpid(child, &status, 0) != child)
		die_errno("cannot wait for %s", name);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return;

	if (WIFEXITED(status))
		die("%s exited with status %d", name, WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		die("%s killed by signal %d", name, WTERMSIG(status));
	if (WIFSTOPPED(status))
		die("%s stopped by signal %d", name, WSTOPSIG(status));
	die("waitpid is confused (status=%d)", status);
}

static pid_t spawn(const char * const cmd[], int out, int err)
{
	pid_t child;
	posix_spawn_file_actions_t redir;

	if (posix_spawn_file_actions_init(&redir) ||
	    (out >= 0 && posix_spawn_file_actions_adddup2(&redir, out, 1)) ||
	    (err >= 0 && posix_spawn_file_actions_adddup2(&redir, err, 2)) ||
	    posix_spawnp(&child, cmd[0], &redir, NULL,
						(char **) cmd, environ) ||
	    posix_spawn_file_actions_destroy(&redir))
		die_errno("cannot run %s", cmd[0]);
	return child;
}

void run(const char * const cmd[])
{
	pid_t child = spawn(cmd, -1, -1);
	wait_or_die(child, cmd[0]);
}
