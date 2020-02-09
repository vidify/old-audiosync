# To generate compile_commands.json use this in the build directory:
# cmake -H.. -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
# cd ..
# ln -s build/compile_commands.json .

cmake_minimum_required(VERSION 3.10)
project(VidifyAudiosync
        VERSION 0.1
        DESCRIPTION "A vidify extension to sync audio from YouTube and your PC"
        LANGUAGES C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Debug configuration
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g3 -DDEBUG \
                         -fsanitize=undefined -fsanitize=address")
# Release configuration
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -Wextra -O3 \
                           -fno-finite-math-only")

# Finding the FFTW library and showing it on the terminal.
find_package(FFTW REQUIRED)
if (FFTW_FOUND)
    message(STATUS "Found FFTW library path: ${FFTW_LIBRARIES}")
    message(STATUS "Found FFTW include-dir path: ${FFTW_INCLUDES}")
    include_directories(${FFTW_INCLUDES})
else ()
    message(FATAL_ERROR "FFTW wasn't found in the system. Please install the \
            library and try again.")
endif ()

# Main directories with the code
add_subdirectory(src)
add_subdirectory(apps)
include_directories(include)

# Testing only available if this is the main app
if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND BUILD_TESTING)
    message(STATUS "Tests building enabled")
    include(CTest)
    add_subdirectory(tests)
endif ()
