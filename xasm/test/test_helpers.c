/*
 * test_helpers.c — Process spawning and file comparison
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mem.h"
#include "str.h"
#include "strbuf.h"
#include "test_helpers.h"

void
result_Free(process_result_t* result) {
	str_Free(result->stdout_buf);
	str_Free(result->stderr_buf);
	str_Free(result->file_buf);
	result->stdout_buf = NULL;
	result->stderr_buf = NULL;
	result->file_buf = NULL;
}

bool
buffer_Equals(const string* a, const string* b) {
	return str_Equal(a, b);
}

bool
diff_Summary(const string* expected, const string* actual, string** summary) {
	if (str_Equal(expected, actual)) {
		*summary = NULL;
		return false;
	}

	size_t first_diff = 0;
	size_t min_len = str_Length(expected) < str_Length(actual) ? str_Length(expected) : str_Length(actual);
	while (first_diff < min_len && str_CharAt(expected, (ssize_t)first_diff) == str_CharAt(actual, (ssize_t)first_diff))
		first_diff++;

	size_t diff_line = 0;
	for (size_t k = 0; k < first_diff && k < str_Length(expected); k++)
		if (str_CharAt(expected, (ssize_t)k) == '\n') diff_line++;

	string_buffer* buf = strbuf_Create();
	strbuf_AppendFormat(buf,
	    "Length: expected %u, actual %u\n"
	    "First difference at line %zu, byte %zu",
	    (uint32_t)str_Length(expected),
	    (uint32_t)str_Length(actual),
	    diff_line + 1,
	    first_diff + 1);
	*summary = strbuf_String(buf);
	strbuf_Free(buf);
	return true;
}

static volatile sig_atomic_t g_timeout_pid = 0;

static void
timeout_handler(int sig) {
	(void)sig;
	if (g_timeout_pid > 0)
		kill(g_timeout_pid, SIGKILL);
}

bool
spawn_Assembler(const char* assembler_path,
                const char* working_dir,
                string* const args[],
                const char* output_file,
                uint32_t timeout_ms,
                process_result_t* result) {
	int32_t stdout_pipe[2];
	int32_t stderr_pipe[2];

	result->stdout_buf = NULL;
	result->stderr_buf = NULL;
	result->file_buf = NULL;
	result->exit_code = -1;
	result->timed_out = false;

	if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0)
		return false;

	pid_t pid = fork();
	if (pid < 0) {
		close(stdout_pipe[0]); close(stdout_pipe[1]);
		close(stderr_pipe[0]); close(stderr_pipe[1]);
		return false;
	}

	if (pid == 0) {
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);
		dup2(stdout_pipe[1], STDOUT_FILENO);
		dup2(stderr_pipe[1], STDERR_FILENO);
		close(stdout_pipe[1]); close(stderr_pipe[1]);
		if (working_dir)
			chdir(working_dir);

		/* Convert string* args to char* for execv, right before the call */
		int32_t argc = 0;
		while (args[argc]) argc++;

		char** exec_args = mem_Alloc((size_t)(argc + 1) * sizeof(char*));
		for (int32_t i = 0; i < argc; i++)
			exec_args[i] = (char*)str_String(args[i]);
		exec_args[argc] = NULL;

		execv(assembler_path, exec_args);
		mem_Free(exec_args);
		_exit(127);
	}

	close(stdout_pipe[1]);
	close(stderr_pipe[1]);

	string_buffer* stdout_buf = strbuf_Create();
	string_buffer* stderr_buf = strbuf_Create();

	bool timeout_set = false;
	if (timeout_ms > 0) {
		struct sigaction sa, old_sa;
		sa.sa_handler = timeout_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGALRM, &sa, &old_sa);
		alarm((timeout_ms / 1000) + 1);
		g_timeout_pid = pid;
		timeout_set = true;
	}

	/* Read from both pipes until EOF */
	uint8_t tmp[4096];
	bool done_stdout = false;
	bool done_stderr = false;

	while (!done_stdout || !done_stderr) {
		fd_set readfds;
		FD_ZERO(&readfds);
		int32_t maxfd = 0;

		if (!done_stdout) {
			FD_SET(stdout_pipe[0], &readfds);
			if (stdout_pipe[0] >= maxfd) maxfd = stdout_pipe[0] + 1;
		}
		if (!done_stderr) {
			FD_SET(stderr_pipe[0], &readfds);
			if (stderr_pipe[0] >= maxfd) maxfd = stderr_pipe[0] + 1;
		}

		if (done_stdout && done_stderr) break;

		int32_t ret = select(maxfd, &readfds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) continue;
			break;
		}

		if (!done_stdout && FD_ISSET(stdout_pipe[0], &readfds)) {
			ssize_t n = read(stdout_pipe[0], tmp, sizeof(tmp));
			if (n <= 0) {
				done_stdout = true;
			} else {
				strbuf_AppendChars(stdout_buf, (const char*)tmp, (size_t)n);
			}
		}
		if (!done_stderr && FD_ISSET(stderr_pipe[0], &readfds)) {
			ssize_t n = read(stderr_pipe[0], tmp, sizeof(tmp));
			if (n <= 0) {
				done_stderr = true;
			} else {
				strbuf_AppendChars(stderr_buf, (const char*)tmp, (size_t)n);
			}
		}
	}

	close(stdout_pipe[0]);
	close(stderr_pipe[0]);

	/* Wait for child */
	int32_t status;
	pid_t waited;
	do {
		waited = waitpid(pid, &status, 0);
	} while (waited < 0 && errno == EINTR);

	if (timeout_set) {
		alarm(0);
		g_timeout_pid = 0;
		struct sigaction sa;
		sa.sa_handler = SIG_DFL;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGALRM, &sa, NULL);
	}

	result->timed_out = (waited < 0) || WIFSTOPPED(status);
	if (WIFEXITED(status))
		result->exit_code = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		result->exit_code = 128 + WTERMSIG(status);

	result->stdout_buf = strbuf_String(stdout_buf);
	result->stderr_buf = strbuf_String(stderr_buf);
	strbuf_Free(stdout_buf);
	strbuf_Free(stderr_buf);

	/* Read output file if requested */
	if (output_file) {
		usleep(10000); /* 10ms delay to ensure file is flushed */
		FILE* f = fopen(output_file, "rb");
		if (f) {
			result->file_buf = fgetstr(f);
			fclose(f);
		}
	}

	return true;
}
