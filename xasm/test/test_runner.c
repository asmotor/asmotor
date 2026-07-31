/*
 * test_runner.c — Integration test runner for ASMotor
 *
 * Spawns assembler binaries, captures output, compares against expected.
 * Produces TAP output for CI integration.
 */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "mem.h"
#include "str.h"
#include "strbuf.h"
#include "file.h"
#include "test_helpers.h"
#include "test_runner.h"

/* Timing helper */
static uint64_t
get_time_us(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/*
 * Match a string against a pattern with * wildcard.
 * "*" matches anything, "?" matches one character.
 */
static bool
pattern_Match(const char* pattern, const char* str) {
	const char* p = pattern;
	const char* s = str;

	while (*p) {
		if (*p == '*') {
			p++;
			if (*p == '\0') return true; /* * at end matches anything */
			while (*s) {
				if (pattern_Match(p, s)) return true;
				s++;
			}
			return false;
		} else if (*p == '?') {
			if (*s == '\0') return false;
			p++; s++;
		} else {
			if (*p != *s) return false;
			if (*s == '\0') return false;
			p++; s++;
		}
	}
	return *s == '\0';
}

test_case_t*
discover_Suite(const suite_config_t* config,
               const char* test_dir,
               int32_t* count) {
	DIR* dir = opendir(test_dir);
	if (!dir) {
		*count = 0;
		return NULL;
	}

	/* First pass: count matching files */
	int32_t n = 0;
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		string* name = str_Create(entry->d_name);
		if (str_EndsWith(name, config->answer_ext))
			n++;
		str_Free(name);
	}
	closedir(dir);

	if (n == 0) {
		*count = 0;
		return NULL;
	}

	/* Second pass: create test cases */
	test_case_t* tests = mem_Alloc((size_t)n * sizeof(test_case_t));
	if (!tests) {
		*count = 0;
		return NULL;
	}

	dir = opendir(test_dir);
	int32_t idx = 0;
	while ((entry = readdir(dir)) != NULL) {
		string* name = str_Create(entry->d_name);
		if (!str_EndsWith(name, config->answer_ext)) {
			str_Free(name);
			continue;
		}

		/* Derive source file name from answer file name */
		/* e.g., "allcodes.rc8.answer" -> "allcodes.rc8" */
		size_t answer_ext_len = strlen(".answer");
		ssize_t base_len = (ssize_t)str_Length(name) - (ssize_t)answer_ext_len;
		string* base = str_Slice(name, 0, base_len);
		if (!base) {
			str_Free(name);
			closedir(dir);
			mem_Free(tests);
			*count = 0;
			return NULL;
		}

		/* Check source file exists */
		char source_path[1024];
		snprintf(source_path, sizeof(source_path), "%s/%s", test_dir, str_String(base));
		if (fexists(source_path)) {
			tests[idx].name = str_Create(str_String(base));
			tests[idx].category = str_Create(config->name);
			tests[idx].source_file = str_Create(str_String(base));
			tests[idx].answer_file = str_Create(str_String(name));
			tests[idx].assembler = str_Create(config->assembler);
			tests[idx].extra_args = config->extra_args ? str_Create(config->extra_args) : NULL;
			tests[idx].expected_exit = config->expected_exit;
			tests[idx].expect_file_output = true;
			idx++;
		}

		str_Free(base);
		str_Free(name);
	}
	closedir(dir);

	*count = idx;
	return tests;
}

void
free_Suite(test_case_t* tests, int32_t count) {
	if (!tests) return;
	for (int32_t i = 0; i < count; i++) {
		str_Free(tests[i].name);
		str_Free(tests[i].category);
		str_Free(tests[i].source_file);
		str_Free(tests[i].answer_file);
		str_Free(tests[i].assembler);
		str_Free(tests[i].extra_args);
	}
	mem_Free(tests);
}

/*
 * Build command line args from format string.
 * "{}" is replaced with the source file name.
 * Returns heap-allocated NULL-terminated string** array.
 * Strings are built via strbuf for consistency.
 * Caller passes to spawn_Assembler which converts to char* for execv and frees.
 */

static string**
build_Args(const char* prog_name, const char* source_file, const char* output_suffix, const char* extra_args) {
	/* Count arguments: prog_name + extra_args + NULL */
	int32_t count = 4; /* prog_name, -fv, -o<output>, <source>, NULL */
	if (extra_args) {
		for (const char* p = extra_args; *p; p++)
			if (*p == ' ') count++;
	}

	string** args = mem_Alloc((size_t)(count + 2) * sizeof(string*));
	if (!args) return NULL;

	int32_t idx = 0;
	args[idx++] = str_Create(prog_name);

	if (extra_args) {
		string_buffer* buf = strbuf_Create();
		bool in_placeholder = false;
		for (const char* p = extra_args; *p; p++) {
			if (*p == '{' && *(p + 1) == '}') {
				in_placeholder = true;
				p++; /* skip } */
				continue;
			}
			if (in_placeholder) {
				strbuf_AppendChars(buf, source_file, strlen(source_file));
				in_placeholder = false;
				continue;
			}
			if (*p == ' ') {
				if (strbuf_Size(buf) > 0) {
					args[idx++] = strbuf_String(buf);
					strbuf_Truncate(buf, 0);
				}
			} else {
				strbuf_AppendChar(buf, *p);
			}
		}
		if (strbuf_Size(buf) > 0) {
			args[idx++] = strbuf_String(buf);
		}
		strbuf_Free(buf);

		/* Always add -o<output> and <source> */
		args[idx++] = str_CreateFormat("-o%s", output_suffix);
		args[idx++] = str_Create(source_file);
	} else {
		/* Default: -fv -o<output> <source> */
		args[idx++] = str_Create("-fv");
		args[idx++] = str_CreateFormat("-o%s", output_suffix);
		args[idx++] = str_Create(source_file);
	}

	args[idx] = NULL;
	return args;
}

test_result_t
run_Test(const test_case_t* testcase,
         const char* test_dir,
         const char* build_dir) {
	test_result_t result = {
		.testcase = testcase,
		.passed = false,
		.setup_error = false,
		.duration_us = 0,
		.failure_msg = NULL,
	};

	uint64_t start = get_time_us();

	/* Build paths */
	char source_path[1024];
	char answer_path[1024];
	char assembler_path[1024];

	snprintf(source_path, sizeof(source_path), "%s/%s", test_dir, str_String(testcase->source_file));
	snprintf(answer_path, sizeof(answer_path), "%s/%s", test_dir, str_String(testcase->answer_file));
	snprintf(assembler_path, sizeof(assembler_path), "%s/%s", build_dir, str_String(testcase->assembler));

	/* Check source file exists */
	if (!fexists(source_path)) {
		result.setup_error = true;
		result.failure_msg = str_CreateFormat("Source file not found: %s", source_path);
		result.duration_us = get_time_us() - start;
		return result;
	}

	/* Check answer file exists */
	if (!fexists(answer_path)) {
		result.setup_error = true;
		result.failure_msg = str_CreateFormat("Answer file not found: %s", answer_path);
		result.duration_us = get_time_us() - start;
		return result;
	}

	/* Check assembler exists */
	if (!fexists(assembler_path)) {
		result.setup_error = true;
		result.failure_msg = str_CreateFormat("Assembler not found: %s", assembler_path);
		result.duration_us = get_time_us() - start;
		return result;
	}

	/* Resolve to absolute path (needed since we chdir before exec) */
	char abs_assembler[1024];
	if (!realpath(assembler_path, abs_assembler)) {
		result.setup_error = true;
		result.failure_msg = str_Create("Failed to resolve assembler path");
		result.duration_us = get_time_us() - start;
		return result;
	}

	/* Build arguments */
	char output_name[1024];
	snprintf(output_name, sizeof(output_name), "%s.r", str_String(testcase->name));
	char full_output_path[1024];
	snprintf(full_output_path, sizeof(full_output_path), "%s/%s", test_dir, output_name);

	string** args = build_Args(abs_assembler, str_String(testcase->source_file), output_name,
	                           testcase->extra_args ? str_String(testcase->extra_args) : NULL);
	if (!args) {
		result.setup_error = true;
		result.failure_msg = str_Create("Failed to build arguments");
		result.duration_us = get_time_us() - start;
		return result;
	}

	process_result_t proc_result;
	bool spawn_ok = spawn_Assembler(abs_assembler, test_dir, (string* const*)args,
	                                testcase->expect_file_output ? full_output_path : NULL,
	                                5000, /* 5 second timeout */
	                                &proc_result);

	/* spawn_Assembler frees the string* args after execv returns */

	if (!spawn_ok) {
		result.setup_error = true;
		result.failure_msg = str_Create("Failed to spawn assembler process");
		result.duration_us = get_time_us() - start;
		result_Free(&proc_result);
		return result;
	}

	result.duration_us = get_time_us() - start;

	/* Check exit code */
	if (testcase->expected_exit >= 0 && proc_result.exit_code != testcase->expected_exit) {
		result.passed = false;
		result.failure_msg = str_CreateFormat("Exit code: expected %d, got %d",
		                                      testcase->expected_exit, proc_result.exit_code);
		result_Free(&proc_result);
		unlink(full_output_path);
		return result;
	}

	/* Read answer file */
	string* answer_buf = NULL;
	FILE* answer_file = fopen(answer_path, "rb");
	if (answer_file) {
		answer_buf = fgetstr(answer_file);
		fclose(answer_file);
	}
	if (!answer_buf) {
		result.setup_error = true;
		result.failure_msg = str_Create("Failed to read answer file");
		result_Free(&proc_result);
		unlink(full_output_path);
		return result;
	}

	/* Concatenate actual output: file_buf + stdout_buf + stderr_buf */
	string_buffer* out_buf = strbuf_Create();
	if (proc_result.file_buf)
		strbuf_AppendString(out_buf, proc_result.file_buf);
	if (proc_result.stdout_buf)
		strbuf_AppendString(out_buf, proc_result.stdout_buf);
	if (proc_result.stderr_buf)
		strbuf_AppendString(out_buf, proc_result.stderr_buf);
	string* actual_buf = strbuf_String(out_buf);
	strbuf_Free(out_buf);
	result_Free(&proc_result);
	unlink(full_output_path);

	/* Compare */
	if (!str_Equal(answer_buf, actual_buf)) {
		result.passed = false;
		diff_Summary(answer_buf, actual_buf, &result.failure_msg);
		if (!result.failure_msg)
			result.failure_msg = str_Create("Output mismatch");
	} else {
		result.passed = true;
	}

	str_Free(answer_buf);
	str_Free(actual_buf);
	return result;
}

void
result_TestFree(test_result_t* result) {
	str_Free(result->failure_msg);
	result->failure_msg = NULL;
}

/* Format duration for display */
static void
format_Duration(uint64_t us, char* buf, size_t buf_size) {
	string_buffer* b = strbuf_Create();
	if (us < 1000)
		strbuf_AppendFormat(b, "%llua", (uint64_t)us);
	else if (us < 1000000)
		strbuf_AppendFormat(b, "%.1fms", (double)us / 1000.0);
	else
		strbuf_AppendFormat(b, "%.2fs", (double)us / 1000000.0);

	string* s = strbuf_String(b);
	strbuf_Free(b);

	size_t slen = str_Length(s);
	if (slen >= buf_size) slen = buf_size - 1;
	memcpy(buf, str_String(s), slen);
	buf[slen] = '\0';
	str_Free(s);
}

int32_t
run_Suite(const test_case_t* tests,
          int32_t count,
          const char* test_dir,
          const char* build_dir,
          const char* filter,
          const char* category_filter) {
	int32_t passed = 0;
	int32_t failed = 0;
	int32_t skipped = 0;
	int32_t setup_errors = 0;
	uint64_t total_us = 0;

	printf("TAP version 13\n");
	printf("1..%d\n", count);

	for (int32_t i = 0; i < count; i++) {
		const test_case_t* tc = &tests[i];

		/* Apply filters */
		if (filter && !pattern_Match(filter, str_String(tc->name))) {
			skipped++;
			continue;
		}
		if (category_filter && str_String(tc->category) &&
		    strcmp(str_String(tc->category), category_filter) != 0) {
			skipped++;
			continue;
		}

		test_result_t result = run_Test(tc, test_dir, build_dir);
		char dur[32];
		format_Duration(result.duration_us, dur, sizeof(dur));
		total_us += result.duration_us;

		int32_t test_num = passed + failed + 1;

		if (result.setup_error) {
			printf("not ok %d - %s # SKIP setup error: %s\n",
			       test_num, str_String(tc->name),
			       result.failure_msg ? str_String(result.failure_msg) : "unknown");
			setup_errors++;
		} else if (result.passed) {
			printf("ok %d - %s %s\n", test_num, str_String(tc->name), dur);
			passed++;
		} else {
			printf("not ok %d - %s %s\n", test_num, str_String(tc->name), dur);
			printf("#   Failed test: %s (%s)\n", str_String(tc->name),
			       tc->category ? str_String(tc->category) : "unknown");
			printf("#   %s\n", result.failure_msg ? str_String(result.failure_msg) : "unknown failure");
			failed++;
		}

		result_TestFree(&result);
	}

	char total_dur[32];
	format_Duration(total_us, total_dur, sizeof(total_dur));

	printf("\n# Results: %d passed, %d failed, %d skipped, %d setup errors (%s)\n",
	       passed, failed, skipped, setup_errors, total_dur);

	return failed + setup_errors;
}
