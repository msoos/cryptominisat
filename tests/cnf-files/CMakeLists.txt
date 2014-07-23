get_target_property(cryptominisat4_EXECUTABLE_PATH cryptominisat LOCATION)

# Create llvm-lit configuration file
configure_file(lit.site.cfg.in lit.site.cfg @ONLY)

# Make sure this test-suite runs when "test" is target
add_test(CNF ${PYTHON_EXECUTABLE} ${LIT_TOOL} ${LIT_ARGS} .)

