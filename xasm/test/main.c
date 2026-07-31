/*
 * main.c — Entry point for ASMotor integration test runner
 */

#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "mem.h"
#include "str.h"
#include "test_runner.h"

/* Backend configurations */
static const suite_config_t backends[] = {
	{ "6502", "motor6502", NULL, ".asm", ".asm.answer", -1 },
	{ "6809", "motor6809", NULL, ".asm", ".asm.answer", -1 },
	{ "680x0", "motor68k", "-mga -fv", ".68k", ".68k.obj.answer", -1 },
	{ "dcpu-16", "motordcpu16", NULL, ".asm", ".asm.answer", -1 },
	{ "gameboy", "motorz80", "-mcg -fv", ".asm", ".asm.answer", -1 },
	{ "mips", "motormips", NULL, ".asm", ".asm.answer", -1 },
	{ "rc8", "motorrc8", NULL, ".rc8", ".rc8.answer", -1 },
	{ "schip", "motorschip", NULL, ".asm", ".asm.answer", -1 },
	{ "z80", "motorz80", "-mcz -fv", ".asm", ".asm.answer", -1 },
};

static const int32_t backend_count = sizeof(backends) / sizeof(backends[0]);

/* Common tests: run each source against all assemblers */
static const char* common_assemblers[] = {
	"motordcpu16", "motor6809", "motor68k", "motor6502",
	"motorz80", "motormips", "motorschip", "motorrc8"
};
static const int32_t common_asm_count = sizeof(common_assemblers) / sizeof(common_assemblers[0]);

typedef struct {
	const char* name;
	test_case_t* tests;
	int32_t count;
	bool auto_discover;      /* If true, discover from directory at runtime */
	const suite_config_t* config;  /* Config for auto-discovery */
} suite_t;

static void
print_Usage(const char* prog) {
	fprintf(stderr,
	        "Usage: %s [options]\n"
	        "Options:\n"
	        "  --test-dir DIR     Directory containing test source files\n"
	        "  --build-dir DIR    Directory containing assembler binaries\n"
	        "  --filter PATTERN   Run only tests matching pattern (*, ? wildcards)\n"
	        "  --category CAT     Run only tests in category CAT\n"
	        "  --list             List all test cases without running\n"
	        "  --suite SUITE      Run specific suite: 6502,6809,...,common (default: all)\n"
	        "  -h, --help         Show this help\n",
	        prog);
}

int
main(int argc, char** argv) {
	const char* test_base_dir = NULL;
	const char* build_dir = NULL;
	const char* filter = NULL;
	const char* category_filter = NULL;
	const char* suite_name = NULL;
	bool list_only = false;

	static struct option long_options[] = {
		{ "test-dir", required_argument, NULL, 't' },
		{ "build-dir", required_argument, NULL, 'b' },
		{ "filter", required_argument, NULL, 'f' },
		{ "category", required_argument, NULL, 'c' },
		{ "list", no_argument, NULL, 'l' },
		{ "suite", required_argument, NULL, 's' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "+t:b:f:c:s:hl", long_options, NULL)) != -1) {
		switch (opt) {
			case 't': test_base_dir = optarg; break;
			case 'b': build_dir = optarg; break;
			case 'f': filter = optarg; break;
			case 'c': category_filter = optarg; break;
			case 's': suite_name = optarg; break;
			case 'l': list_only = true; break;
			case 'h': print_Usage(argv[0]); return 0;
			default:  print_Usage(argv[0]); return 1;
		}
	}

	/* Resolve default paths */
	if (!build_dir)
		build_dir = "./xasm/rc8";
	if (!test_base_dir)
		test_base_dir = "../../test";

	/* Build suite list */
	int32_t suite_count = 0;
	if (suite_name) {
		suite_count = 1;
	} else {
		suite_count = backend_count + 1; /* +1 for common */
	}

	suite_t* suites = mem_Alloc((size_t)suite_count * sizeof(suite_t));
	if (!suites) { fprintf(stderr, "Out of memory\n"); return 1; }

	int32_t idx = 0;
	for (int32_t i = 0; i < backend_count; i++) {
		if (suite_name && strcmp(backends[i].name, suite_name) != 0)
			continue;

		char test_dir[1024];
		snprintf(test_dir, sizeof(test_dir), "%s/%s", test_base_dir, backends[i].name);

		suites[idx].name = backends[i].name;
		suites[idx].tests = discover_Suite(&backends[i], test_dir, &suites[idx].count);
		suites[idx].auto_discover = true;
		suites[idx].config = &backends[i];
		idx++;
	}

	/* Common suite */
	if (!suite_name || strcmp(suite_name, "common") == 0) {
		char test_dir[1024];
		snprintf(test_dir, sizeof(test_dir), "%s/%s", test_base_dir, "common");

		/* Discover common tests: each source x each assembler */
		DIR* dir = opendir(test_dir);
		int32_t n = 0;
		if (dir) {
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL) {
				string* name = str_Create(entry->d_name);
				if (str_EndsWith(name, ".asm"))
					n++;
				str_Free(name);
			}
			closedir(dir);
		}

		int32_t total = n * common_asm_count;
		test_case_t* tests = NULL;
		int32_t tidx = 0;
		if (total > 0) {
			tests = mem_Alloc((size_t)total * sizeof(test_case_t));
		}

		dir = opendir(test_dir);
		if (dir) {
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL) {
				string* d_name = str_Create(entry->d_name);
				if (!str_EndsWith(d_name, ".asm")) {
					str_Free(d_name);
					continue;
				}

				for (int32_t a = 0; a < common_asm_count; a++) {
					string* name = str_CreateFormat("%s_%s", str_String(d_name), common_assemblers[a]);
					ssize_t base_no_ext_len = (ssize_t)str_Length(d_name) - (ssize_t)strlen(".asm");
					string* base_no_ext = str_Slice(d_name, 0, base_no_ext_len);

					char ans_full[256];
					snprintf(ans_full, sizeof(ans_full), "%s.answer", str_String(base_no_ext));

					tests[tidx].name = str_Create(str_String(name));
					tests[tidx].category = str_Create("common");
					tests[tidx].source_file = str_Create(str_String(d_name));
					tests[tidx].answer_file = str_Create(ans_full);
					tests[tidx].assembler = str_Create(common_assemblers[a]);
					tests[tidx].extra_args = NULL;
					tests[tidx].expected_exit = 0;
					tests[tidx].expect_file_output = true;
					tidx++;

					str_Free(name);
					str_Free(base_no_ext);
				}
				str_Free(d_name);
			}
			closedir(dir);
		}

		suites[idx].name = "common";
		suites[idx].tests = tests;
		suites[idx].count = tidx;
		suites[idx].auto_discover = true;
		suites[idx].config = NULL;
		idx++;
	}

	if (list_only) {
		printf("%-40s %-12s %s\n", "TEST", "CATEGORY", "SOURCE");
		{ char sep[80]; memset(sep, '-', 76); sep[76] = '\0'; printf("%s\n", sep); }
		for (int32_t s = 0; s < idx; s++) {
			for (int32_t i = 0; i < suites[s].count; i++) {
				const test_case_t* tc = &suites[s].tests[i];
				printf("%-40s %-12s %s\n",
				       str_String(tc->name),
				       tc->category ? str_String(tc->category) : "",
				       str_String(tc->source_file));
			}
		}
		int32_t total = 0;
		for (int32_t s = 0; s < idx; s++)
			total += suites[s].count;
		printf("\n%d tests total\n", total);
		/* Cleanup */
		for (int32_t s = 0; s < idx; s++)
			free_Suite(suites[s].tests, suites[s].count);
		mem_Free(suites);
		return 0;
	}

	/* Run suites */
	int32_t total_failures = 0;
	for (int32_t s = 0; s < idx; s++) {
		char test_dir[1024];
		snprintf(test_dir, sizeof(test_dir), "%s/%s", test_base_dir, suites[s].name);

		printf("# Suite: %s (%d tests)\n", suites[s].name, suites[s].count);
		int32_t failures = run_Suite(suites[s].tests,
		                             suites[s].count,
		                             test_dir, build_dir, filter, category_filter);
		total_failures += failures;
		if (s < idx - 1)
			printf("\n");

		free_Suite(suites[s].tests, suites[s].count);
	}

	printf("# Total: %d failures\n", total_failures);
	mem_Free(suites);
	return total_failures > 0 ? 1 : 0;
}
