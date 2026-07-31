# Integration test runner for ASMotor
add_executable (test_runner
    main.c
    test_runner.c
    test_runner.h
    test_helpers.c
    test_helpers.h)

target_link_libraries (test_runner PRIVATE util)
target_compile_definitions (test_runner PRIVATE _DEBUG)

# Sanity test for patch.c error paths
add_executable (test_patch_error_paths test_patch_error_paths.c)
target_link_libraries (test_patch_error_paths motor util)
target_compile_definitions (test_patch_error_paths PRIVATE _DEBUG)
