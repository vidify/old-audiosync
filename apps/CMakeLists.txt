add_executable(
    main
    main.c
    ${HEADER_LIST}
)

target_compile_features(main PRIVATE c_std_11)

target_link_libraries(main PRIVATE vidify_audiosync fftw3 m pthread)
