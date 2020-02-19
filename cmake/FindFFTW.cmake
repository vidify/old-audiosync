# Find the FFTW includes and library
#
# FFTW_INCLUDES  - where to find fftw3.h
# FFTW_LIBRARIES - list of libraries when using FFTW.
# FFTW_FOUND     - true if FFTW found.

if (FFTW_INCLUDES AND FFTW_LIBRARIES)
    # Already in cache, be silent
    set (FFTW_FIND_QUIETLY TRUE)
endif ()

find_path (FFTW_INCLUDES fftw3.h)

find_library (FFTW_LIBRARIES NAMES fftw3)

# Handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (FFTW DEFAULT_MSG FFTW_LIBRARIES FFTW_INCLUDES)

mark_as_advanced (FFTW_LIBRARIES FFTW_INCLUDES)
