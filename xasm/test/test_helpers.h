/*
 * test_helpers.h — Process spawning and file comparison for test runner
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "str.h"
#include "strbuf.h"
#include "file.h"

/* Captured output from a child process */
typedef struct {
	string* stdout_buf;   /* Heap-allocated via strbuf_String, NULL if empty */
	string* stderr_buf;   /* Heap-allocated via strbuf_String, NULL if empty */
	string* file_buf;     /* Heap-allocated file contents, NULL if empty */
	int32_t exit_code;
	bool timed_out;
} process_result_t;

/* Free all buffers in a process_result_t */
extern void
result_Free(process_result_t* result);

/*
 * Spawn the assembler binary with the given arguments.
 * Captures stdout, stderr, and reads the output file.
 *
 * assembler_path: path to the assembler binary (e.g., "motorrc8")
 * working_dir: directory to chdir to before exec (or NULL)
 * args: NULL-terminated array of string* arguments (freed by this function)
 * output_file: path to the output file to read after execution (or NULL)
 * timeout_ms: execution timeout in milliseconds (0 = no timeout)
 */
extern bool
spawn_Assembler(const char* assembler_path,
                const char* working_dir,
                string* const args[],
                const char* output_file,
                uint32_t timeout_ms,
                process_result_t* result);

/*
 * Compare two strings. Returns true if identical.
 */
extern bool
buffer_Equals(const string* a, const string* b);

/*
 * Compute a diff-like summary of two strings.
 * Writes to a heap-allocated string (caller frees via str_Free).
 * Returns true if differences were found.
 */
extern bool
diff_Summary(const string* expected, const string* actual, string** summary);
