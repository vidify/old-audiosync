set(
    HEADERS
    "${PROJECT_SOURCE_DIR}/include/audiosync/audiosync.h"
    "${PROJECT_SOURCE_DIR}/include/audiosync/cross_correlation.h"
    "${PROJECT_SOURCE_DIR}/include/audiosync/ffmpeg_pipe.h"
    "${PROJECT_SOURCE_DIR}/include/audiosync/download/linux_download.h"
    "${PROJECT_SOURCE_DIR}/include/audiosync/capture/linux_capture.h"
)

add_library(
    audiosync
    audiosync.c
    cross_correlation.c
    ffmpeg_pipe.c
    download/linux_download.c
    capture/linux_capture.c
    ${HEADERS}
)

target_include_directories(audiosync PUBLIC ../include)

# C99 is required
target_compile_features(audiosync PUBLIC c_std_99)

# For the IDEs
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADERS})
