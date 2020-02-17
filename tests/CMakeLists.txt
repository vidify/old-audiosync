# The tests binaries
set(
    TEST_DEPS
    vidify_audiosync
    fftw3
    m
    pthread
    pulse
    pulse-simple
    ${HEADERS}
)

add_executable(test_cross_correlation test_cross_correlation.c)
target_link_libraries(test_cross_correlation PRIVATE ${TEST_DEPS})
add_executable(test_pearson_coefficient test_pearson_coefficient.c)
target_link_libraries(test_pearson_coefficient PRIVATE ${TEST_DEPS})
add_executable(test_pulseaudio_setup test_pulseaudio_setup.c)
target_link_libraries(test_pulseaudio_setup PRIVATE ${TEST_DEPS})

# Adding the tests one by one for the `make test` command.
add_test(cross_correlation test_cross_correlation)
add_test(pearson_coefficient test_pearson_coefficient)
add_test(pulseaudio_setup test_pulseaudio_setup_wrapper.bash)
