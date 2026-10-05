/*
 * test_dependency_paths.c — Regression test for dependency-file path emission
 *
 * dep_AddDependency() records each path exactly as resolved (normalized, but
 * not absolutized): relative inputs stay relative, absolute inputs stay
 * absolute. The generated Makefile dependency (.d) file therefore reflects the
 * form the build used to reference each file, which is what make resolves from
 * its own directory. (A file reached via an absolute -i path that points inside
 * the tree is emitted absolute; the firmware build uses relative -i, so this
 * does not arise there.)
 *
 * Scenarios covered:
 *   1. Relative path                 -> emitted unchanged
 *   2. Bare path                     -> emitted unchanged
 *   3. Absolute path inside the cwd  -> emitted unchanged (absolute)
 *   4. Absolute path outside the cwd -> emitted unchanged (absolute)
 *   5. Relative inputs are not cwd-prefixed
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dependency.h"
#include "file.h"
#include "str.h"

static int g_failures = 0;

static void
check(bool condition, const char* what) {
	if (condition) {
		printf("ok - %s\n", what);
	} else {
		printf("not ok - %s\n", what);
		++g_failures;
	}
}

int
main(void) {
	char tmpl[] = "/tmp/asmotor_depx_XXXXXX";
	char* base = mkdtemp(tmpl);
	if (base == NULL) {
		fprintf(stderr, "failed to create temp dir\n");
		return 2;
	}

	// The .d file is written relative to the cwd, so anchor there.
	if (chdir(base) != 0) {
		fprintf(stderr, "failed to chdir to temp dir\n");
		return 2;
	}

	// getcwd() resolves symlinks (e.g. /tmp -> /private/tmp on macOS); use the
	// physical cwd to build the in-tree absolute path.
	char cwdbuf[4096];
	if (getcwd(cwdbuf, sizeof(cwdbuf)) == NULL) {
		fprintf(stderr, "failed to getcwd\n");
		return 2;
	}
	const char* cwd = cwdbuf;

	const char* depFile = "test.d";
	const char* outTree = "/tmp/asmotor_dep_outside_test.i";

	dep_Initialize(depFile);

	string* mainOut = str_Create("out.xobj");
	dep_SetMainOutput(mainOut);
	str_Clear(&mainOut);

	// 1. Relative path (must be emitted unchanged).
	string* rel = str_Create("sub/beta.i");
	dep_AddDependency(rel);
	str_Clear(&rel);

	// 2. Bare path (must be emitted unchanged).
	string* bare = str_Create("gamma.i");
	dep_AddDependency(bare);
	str_Clear(&bare);

	// 3. Absolute path inside the cwd (must be emitted unchanged, absolute).
	string* inTree = str_CreateFormat("%s/sub/alpha.i", cwd);
	dep_AddDependency(inTree);

	// 4. Absolute path outside the cwd (must be emitted unchanged, absolute).
	string* outside = str_Create(outTree);
	dep_AddDependency(outside);
	str_Clear(&outside);

	dep_WriteDependencyFile();

	FILE* f = fopen(depFile, "rb");
	string* content = (f != NULL) ? fgetstr(f) : str_Create("");
	if (f != NULL)
		fclose(f);

	const char* contentStr = str_String(content);

	check(strstr(contentStr, "sub/beta.i") != NULL,
	       "relative path emitted unchanged");
	check(strstr(contentStr, "gamma.i") != NULL,
	       "bare path emitted unchanged");
	check(strstr(contentStr, str_String(inTree)) != NULL,
	       "absolute in-tree path emitted unchanged");
	check(strstr(contentStr, outTree) != NULL,
	       "absolute out-of-tree path emitted unchanged");

	// Relative inputs must not be cwd-prefixed.
	string* prefixed = str_CreateFormat("%s/sub/beta.i", cwd);
	check(strstr(contentStr, str_String(prefixed)) == NULL,
	       "relative path not cwd-prefixed");
	str_Clear(&prefixed);
	str_Clear(&inTree);

	str_Clear(&content);
	dep_Exit();

	// Clean up.
	remove(depFile);
	remove(outTree);
	snprintf(tmpl, sizeof(tmpl), "rm -rf '%s'", base);
	(void) system(tmpl);

	if (g_failures == 0) {
		printf("All dependency path tests passed.\n");
	} else {
		printf("%d dependency path test(s) FAILED.\n", g_failures);
	}
	return g_failures;
}
