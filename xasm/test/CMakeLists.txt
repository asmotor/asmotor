# Integration test runner for ASMotor
add_executable (test_runner
    main.c
    test_runner.c
    test_runner.h
    test_helpers.c
    test_helpers.h)

target_link_libraries (test_runner PRIVATE util)

# Sanity test for patch.c error paths
add_executable (test_patch_error_paths test_patch_error_paths.c)
target_link_libraries (test_patch_error_paths motor util)

# Regression test for include path resolution (double free in inc_FindFile)
add_executable (test_include_paths test_include_paths.c)
target_link_libraries (test_include_paths motor util)

# Regression test for dependency-file path emission (relative vs absolute)
add_executable (test_dependency_paths test_dependency_paths.c)
target_link_libraries (test_dependency_paths motor util)
