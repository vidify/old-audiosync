cmake_minimum_required(VERSION 3.10)

project(VidifyAudiosync
        VERSION 1.0
        DESCRIPTION "A vidify extension to sync audio from YouTube and your PC"
        LANGUAGES C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Debug configuration
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g3 -std=c99 \
                         -DDEBUG -fsanitize=undefined -fsanitize=address")
# Release configuration
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -Wextra -O3")

# Main project configuration
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Enable testing
    include(CTest)
    # Enable folders for targets for some IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

include_directories(include)

# Finding the FFTW library and showing it on the terminal.
find_package(FFTW REQUIRED)
if (FFTW_FOUND)
    message(STATUS "Found FFTW library path: ${FFTW_LIBRARIES}")
    message(STATUS "Found FFTW include-dir path: ${FFTW_INCLUDES}")
    include_directories(${FFTW_INCLUDES})
else ()
    message(FATAL_ERROR "FFTW wasn't found in the system. Please install the \
            library and try again.")
endif()

add_subdirectory(src)
add_subdirectory(apps)

# Testing only available if this is the main app
if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND BUILD_TESTING)
    add_subdirectory(tests)
endif ()

# To generate compile_commands.json use this in the build directory:
# cmake -H.. -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
# cd ..
# ln -s build/compile_commands.json .
