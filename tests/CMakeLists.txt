# The tests dependencies for C binaries
set(
    TEST_DEPS
    audiosync
    fftw3
    m
    pthread
    pulse
    pulse-simple
    ${HEADERS}
)

# Useful to add execution permissions to a file
function (enable_execution file)
    execute_process(COMMAND chmod u+x "${file}")
endfunction()

# Checking if the python module is installed
execute_process(COMMAND python3 -c "import audiosync"
                RESULT_VARIABLE ret
                OUTPUT_QUIET ERROR_QUIET)
if (ret EQUAL "0")
    message(STATUS "Python tests enabled")
    set(PYTHON_MODULE_INSTALLED true)
else ()
    message(STATUS "Skipping python tests")
endif ()

# Adding all the tests
add_executable(test_cross_correlation test_cross_correlation.c)
target_link_libraries(test_cross_correlation PRIVATE ${TEST_DEPS})

add_executable(test_pearson_coefficient test_pearson_coefficient.c)
target_link_libraries(test_pearson_coefficient PRIVATE ${TEST_DEPS})

# This test uses a wrapper. It's a shell script so it may require permissions
# before its execution
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_pulseaudio_setup_wrapper.sh"
               "${CMAKE_CURRENT_BINARY_DIR}/test_pulseaudio_setup_wrapper.sh")
enable_execution("${CMAKE_CURRENT_BINARY_DIR}/test_pulseaudio_setup_wrapper.sh")
add_executable(test_pulseaudio_setup test_pulseaudio_setup.c)
target_link_libraries(test_pulseaudio_setup PRIVATE ${TEST_DEPS})

# The python bindings test. It will be skipped if the module wasn't installed.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_bindings.py"
               "${CMAKE_CURRENT_BINARY_DIR}/test_bindings.py")
# execute_process(COMMAND chmod u+x 
enable_execution("${CMAKE_CURRENT_BINARY_DIR}/test_bindings.py")

# Adding the tests one by one for CTest.
add_test(cross_correlation test_cross_correlation)
add_test(pearson_coefficient test_pearson_coefficient)
add_test(pulseaudio_setup test_pulseaudio_setup_wrapper.sh)
if (${PYTHON_MODULE_INSTALLED})
    add_test(bindings test_bindings.py)
endif ()
