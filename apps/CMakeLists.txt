add_executable(
    main
    main.c
    ${HEADERS}
)

target_compile_features(main PRIVATE c_std_99)

target_link_libraries(main PRIVATE audiosync fftw3 m pthread pulse pulse-simple)
