include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_executable(basic_test
    basic_test.cpp
)

target_link_libraries(basic_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

add_executable(assump_test
    assump_test.cpp
)

target_link_libraries(assump_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

add_executable(readme_test
    readme_test.cpp
)

add_executable(heap_test
    heap_test.cpp
)

target_link_libraries(heap_test
    ${Boost_LIBRARIES}
    libcryptominisat4
)

target_link_libraries(readme_test
    libcryptominisat4
)
