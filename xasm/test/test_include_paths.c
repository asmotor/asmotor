/*
 * test_include_paths.c — Regression test for include path resolution in includes.c
 *
 * Verifies that inc_FindFile() resolves files via -i include paths without
 * double-freeing the candidate filename.
 *
 * Before the fix, the first include-path lookup performed after a failed
 * working-directory lookup double-freed the candidate string (str_Free left
 * a dangling pointer that the next str_Move freed again), aborting the
 * assembler (assert in debug builds, malloc double-free in release builds).
 *
 * Scenarios covered:
 *   1. File found via the first include path (no trailing slash)
 *   2. File found via the second include path (trailing slash)
 *   3. File not found anywhere → NULL
 *   4. File found next to the working file (early return)
 *   5. Working file with a bare name resolves to a canonicalized path
 *      (same identity as when found via an include path — INCLUDE ONCE)
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "file.h"
#include "mem.h"
#include "str.h"
#include "strcoll.h"
#include "vec.h"

#include "includes.h"
#include "lexer_context.h"

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

static void
writeFile(const char* path, const char* content) {
	FILE* f = fopen(path, "wb");
	if (f == NULL) {
		printf("not ok - cannot create %s\n", path);
		++g_failures;
		return;
	}
	fputs(content, f);
	fclose(f);
}

static void
makePath(const char* path) {
	char buf[1024];
	strncpy(buf, path, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	for (char* p = buf + 1; *p != '\0'; ++p) {
		if (*p == '/') {
			*p = '\0';
			(void) mkdir(buf, 0755);
			*p = '/';
		}
	}
	(void) mkdir(buf, 0755);
}

int
main(void) {
	char tmpl[] = "/tmp/asmotor_inct_XXXXXX";
	char* base = mkdtemp(tmpl);
	if (base == NULL) {
		fprintf(stderr, "failed to create temp dir\n");
		return 2;
	}

	/*
	 * Layout (mirrors the hc800 kernel build, where a working file includes
	 * files only reachable via -i include paths):
	 *
	 *   base/main.asm      fake working file
	 *   base/gamma.i       found next to the working file
	 *   base/inc1/sub/alpha.i  found via include path 1 (no trailing slash)
	 *   base/inc2/beta.i       found via include path 2 (trailing slash)
	 */
	char path[1024];

	snprintf(path, sizeof(path), "%s/inc1/sub", base);
	makePath(path);
	snprintf(path, sizeof(path), "%s/inc2", base);
	makePath(path);

	snprintf(path, sizeof(path), "%s/main.asm", base);
	writeFile(path, "");
	snprintf(path, sizeof(path), "%s/gamma.i", base);
	writeFile(path, "");
	snprintf(path, sizeof(path), "%s/inc1/sub/alpha.i", base);
	writeFile(path, "");
	snprintf(path, sizeof(path), "%s/inc2/beta.i", base);
	writeFile(path, "");

	/* Fake working file context */
	string* name = str_CreateFormat("%s/main.asm", base);
	string* content = str_Create("");
	vec_t* args = strvec_Create();
	SLexerContext* ctx = lexctx_CreateMemoryContext(name, content, args);
	str_Clear(&name);
	str_Clear(&content);
	lex_Context = ctx;

	string* p1 = str_CreateFormat("%s/inc1", base);
	inc_AddIncludePath(p1);
	str_Clear(&p1);
	string* p2 = str_CreateFormat("%s/inc2/", base);
	inc_AddIncludePath(p2);
	str_Clear(&p2);

	/* Test 1: found via the first include path (working-dir lookup fails first) */
	string* fileName = str_Create("sub/alpha.i");
	string* found = NULL;
	inc_FindFile(&found, fileName);
	str_Clear(&fileName);
	check(found != NULL && str_EndsWith(found, "sub/alpha.i") && fexists(str_String(found)),
	      "file resolved via first include path");
	str_Clear(&found);

	/* Test 2: found via the second include path (working-dir and path 1 fail first) */
	fileName = str_Create("beta.i");
	found = NULL;
	inc_FindFile(&found, fileName);
	str_Clear(&fileName);
	check(found != NULL && str_EndsWith(found, "beta.i") && fexists(str_String(found)),
	      "file resolved via second include path");
	str_Clear(&found);

	/* Test 3: not found anywhere */
	fileName = str_Create("missing.i");
	found = NULL;
	inc_FindFile(&found, fileName);
	str_Clear(&fileName);
	check(found == NULL, "missing file resolves to NULL");
	str_Clear(&found);

	/* Test 4: found next to the working file (early return) */
	fileName = str_Create("gamma.i");
	found = NULL;
	inc_FindFile(&found, fileName);
	str_Clear(&fileName);
	check(found != NULL && str_EndsWith(found, "gamma.i") && fexists(str_String(found)),
	      "file resolved next to working file");
	str_Clear(&found);

	lex_Context = NULL;
	lexctx_FreeContext(ctx);

	/*
	 * Test 5: working file with a bare name (no directory component).
	 * inc_FindFile returns a relative (normalized) path; the absolute
	 * identity used for INCLUDE ONCE dedup is computed separately via
	 * fabsolutePath (see test 6).
	 */
	if (chdir(base) == 0) {
		string* bareName = str_Create("main.asm");
		string* bareContent = str_Create("");
		vec_t* bareArgs = strvec_Create();
		SLexerContext* bareCtx = lexctx_CreateMemoryContext(bareName, bareContent, bareArgs);
		str_Clear(&bareName);
		str_Clear(&bareContent);
		lex_Context = bareCtx;

		fileName = str_Create("gamma.i");
		found = NULL;
		inc_FindFile(&found, fileName);
		str_Clear(&fileName);
		check(found != NULL && str_CharAt(found, 0) != '/' && str_EndsWith(found, "gamma.i"),
		      "bare working file resolves to a relative path");
		str_Clear(&found);

		lex_Context = NULL;
		lexctx_FreeContext(bareCtx);
	} else {
		check(false, "chdir to temp dir");
	}

	/*
	 * Test 6: fabsolutePath produces a stable identity key — equivalent
	 * spellings of the same file (bare, "./"-prefixed, with a "..") all
	 * resolve to the same absolute string. INCLUDE ONCE and the file-info
	 * map rely on this for deduplication.
	 */
	{
		string* s1 = str_Create("gamma.i");
		string* s2 = str_Create("./gamma.i");
		string* s3 = str_Create("sub/../gamma.i");
		string* a = fabsolutePath(s1);
		string* b = fabsolutePath(s2);
		string* c = fabsolutePath(s3);
		check(strcmp(str_String(a), str_String(b)) == 0 &&
		      strcmp(str_String(a), str_String(c)) == 0 &&
		      str_CharAt(a, 0) == '/',
		      "fabsolutePath maps equivalent spellings to one absolute key");
		str_Clear(&s1);
		str_Clear(&s2);
		str_Clear(&s3);
		str_Clear(&a);
		str_Clear(&b);
		str_Clear(&c);
	}

	/*
	 * Test 7: fnormalizePath keeps a leading ".." relative (no cwd anchor), so
	 * includes resolved via a relative -i path (e.g. -i../) stay relative in
	 * the .d file. fabsolutePath resolves the same path to the correct
	 * absolute form for the dedup keys.
	 */
	{
		string* n1 = fnormalizePath(str_Create("../lowlevel/hc800.i"));
		check(strcmp(str_String(n1), "../lowlevel/hc800.i") == 0,
		      "fnormalizePath keeps a leading '..' relative");
		str_Clear(&n1);

		string* n2 = fnormalizePath(str_Create("sub/../../x.i"));
		check(strcmp(str_String(n2), "../x.i") == 0,
		      "fnormalizePath collapses a resolvable '..'");
		str_Clear(&n2);

		string* a2 = fabsolutePath(str_Create("../lowlevel/hc800.i"));
		check(str_CharAt(a2, 0) == '/' && strstr(str_String(a2), "lowlevel/hc800.i") != NULL,
		      "fabsolutePath resolves a leading '..' to an absolute path");
		str_Clear(&a2);
	}

	snprintf(path, sizeof(path), "rm -rf '%s'", base);
	(void) system(path);

	if (g_failures == 0) {
		printf("All include path tests passed.\n");
	} else {
		printf("%d include path test(s) FAILED.\n", g_failures);
	}
	return g_failures;
}
