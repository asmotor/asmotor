/*
 * test_runner.h — Integration test runner for ASMotor
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "str.h"
#include "file.h"

/*
 * A single test case definition.
 *
 * The test runner spawns the assembler binary, captures output,
 * and compares against expected values.
 *
 * All string fields are owned by this struct (string*).
 * discover_Suite() creates owned strings; free_Suite() releases them.
 */
typedef struct {
	string* name;               /* Display name (also used for file lookup) */
	string* category;           /* Category for filtering (e.g., "error", "encoding") */
	string* source_file;        /* .rc8 / .asm source file (relative to test dir) */
	string* answer_file;        /* .answer expected output file */
	string* assembler;          /* Assembler binary name (e.g., "motorrc8") */
	string* extra_args;         /* Extra arguments (format string, {} = source file) */
	int32_t expected_exit;      /* Expected exit code (-1 = don't check) */
	bool expect_file_output;    /* Whether -o produces output file to include in comparison */
} test_case_t;

/*
 * Test result for a single test case.
 */
typedef struct {
	const test_case_t* testcase;
	bool passed;
	bool setup_error;           /* True if test couldn't run (e.g., file not found) */
	uint64_t duration_us;       /* Execution time in microseconds */
	string* failure_msg;        /* Human-readable failure message, NULL if passed */
} test_result_t;

/*
 * Run a single test case. Returns a test_result_t (caller frees via result_TestFree).
 * test_dir: directory containing source and answer files
 * build_dir: directory containing assembler binaries
 */
extern test_result_t
run_Test(const test_case_t* testcase,
         const char* test_dir,
         const char* build_dir);

/*
 * Free a test result.
 */
extern void
result_TestFree(test_result_t* result);

/*
 * Backend configuration for auto-discovering tests from a directory.
 * Fields are borrowed (const char*) — not owned by this struct.
 */
typedef struct {
	const char* name;           /* Suite name (e.g., "6502", "z80") */
	const char* assembler;      /* Assembler binary name (e.g., "motor6502") */
	const char* extra_args;     /* Extra arguments (e.g., "-mcg -fv", NULL for default "-fv") */
	const char* file_ext;       /* Source file extension (e.g., ".asm", ".rc8", ".68k") */
	const char* answer_ext;     /* Answer file extension suffix (e.g., ".answer", ".obj.answer") */
	int32_t expected_exit;      /* Expected exit code (0 or 1) */
} suite_config_t;

/*
 * Auto-discover test cases from a directory based on backend config.
 * Returns heap-allocated array of test_case_t (caller frees with free_Suite).
 * Sets *count to the number of discovered tests.
 */
extern test_case_t*
discover_Suite(const suite_config_t* config,
               const char* test_dir,
               int32_t* count);

/*
 * Run a suite of test cases and print TAP output.
 * Returns number of failures (0 = all passed).
 *
 * tests: contiguous array of test_case_t (NOT array of pointers)
 * count: number of tests
 */
extern int32_t
run_Suite(const test_case_t* tests,
          int32_t count,
          const char* test_dir,
          const char* build_dir,
          const char* filter,
          const char* category_filter);

/*
 * Free a suite of test cases discovered by discover_Suite.
 */
extern void
free_Suite(test_case_t* tests, int32_t count);
