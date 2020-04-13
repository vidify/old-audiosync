# Find the PulseAudio includes and library
#
# PULSEAUDIO_INCLUDES  - the PulseAudio include directory
# PULSEAUDIO_LIBRARIES - the libraries needed to use PulseAudio
# PULSEAUDIO_FOUND     - system has the PulseAudio library

if (PULSEAUDIO_INCLUDES AND PULSEAUDIO_LIBRARIES)
    # Already in cache, be silent
    set(PULSEAUDIO_FIND_QUIETLY TRUE)
endif ()

find_path(PULSEAUDIO_INCLUDES pulse/pulseaudio.h)

find_library(PULSEAUDIO_LIBRARIES NAMES pulse)

# Handle the QUIETLY and REQUIRED arguments and set PULSEAUDIO_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(PulseAudio DEFAULT_MSG PULSEAUDIO_LIBRARIES PULSEAUDIO_INCLUDES)

mark_as_advanced(PULSEAUDIO_INCLUDES PULSEAUDIO_LIBRARIES)
