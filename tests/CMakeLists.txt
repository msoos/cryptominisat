include_directories(
    ${PROJECT_SOURCE_DIR}/cmsat/
    ${Boost_INCLUDE_DIRS}
)

ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_executable(basic_test
        basic_test.cpp
)

target_link_libraries(basic_test
    ${Boost_LIBRARIES}
    cryptoms
)

add_executable(assump_test
    assump_test.cpp
)

target_link_libraries(assump_test
    ${Boost_LIBRARIES}
    cryptoms
)
