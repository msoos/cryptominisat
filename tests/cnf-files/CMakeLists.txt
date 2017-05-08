# get_target_property(cryptominisat5_EXECUTABLE_PATH cryptominisat5 LOCATION)

# Create llvm-lit configuration file
configure_file(lit.site.cfg.in lit.site.cfg.in2 @ONLY)

if (MSVC)
    file(GENERATE
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/lit.site.cfg
         INPUT ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.in2)


    # Make sure this test-suite runs when "test" is target
    add_test(NAME CNF
            COMMAND ${LIT_TOOL} ${LIT_ARGS} .
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/"
    )
else()
    file(GENERATE
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
         INPUT ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.in2)


    # Make sure this test-suite runs when "test" is target
    add_test(NAME CNF
            COMMAND ${LIT_TOOL} ${LIT_ARGS} .
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()

