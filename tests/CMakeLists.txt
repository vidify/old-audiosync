# The tests binary includes all the different test files
add_executable(
    tests
    test_cross_correlation.c
    ${HEADERS}
)

# Minimum C11 required. Also listing the used libraries.
target_compile_features(tests PRIVATE c_std_11)
target_link_libraries(tests PRIVATE vidify_audiosync fftw3 m pthread)

# Adding the tests one by one for the make command:
#    make test
add_test(NAME test_cross_correlation COMMAND tests)
