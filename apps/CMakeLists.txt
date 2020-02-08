add_executable(
    main
    main.c
    ${HEADERS}
)

target_compile_features(main PRIVATE c_std_99)

target_link_libraries(main PRIVATE vidify_audiosync fftw3 m pthread)
