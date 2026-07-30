# Unit test for expression ownership leaks in patch.c
add_executable (test_expr_ownership test_expr_ownership.c)
target_link_libraries (test_expr_ownership motor util)
target_compile_definitions (test_expr_ownership PRIVATE _DEBUG)
