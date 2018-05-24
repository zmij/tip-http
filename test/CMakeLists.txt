#	/awm-server/lib/http/test/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 30, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT GTEST_INCLUDE_DIRS)
    find_package(GTest REQUIRED)
endif()
if (NOT CMAKE_THREAD_LIBS_INIT)
    find_package(Threads REQUIRED)
endif()

include_directories(${GTEST_INCLUDE_DIRS} ${TIP_IRI_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# Test requests
set(test_http_REQUESTS
    01.get.request
    02.get.request
    03.get.request
    04.post.request
    05.post.request
)

foreach(req_file ${test_http_REQUESTS})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${req_file}
        DEPENDS ${req_file}
        COMMENT "Copy test request file ${req_file}"
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${req_file} ${CMAKE_CURRENT_BINARY_DIR}/${req_file}
    )
endforeach()

add_custom_target(test-http-requests
    DEPENDS ${test_http_REQUESTS}
    COMMENT "Copy test HTTP request files"
)

if (GTEST_XML_OUTPUT)
    set (
        TEST_COMMON_OUT_ARGS
        --gtest_output=xml:test-http-common-detail.xml
    )
    set(
        TEST_CLIENT_OUT_ARGS
        --gtest_output=xml:test-http-client-detail.xml
    )
    set(
        TEST_SERVER_OUT_ARGS
        --gtest_output=xml:test-http-server-detail.xml
    )
endif()

configure_file(config.in.hpp config.hpp)

# Test binaries
# HTTP Common test
set(
    test_http_common_SRCS 
    generate_tests.cpp
    base64_test.cpp
    datetime_parse_test.cpp
    datetime_gen_test.cpp
    req_parse_test.cpp
    req_gen_test.cpp
    resp_parse_test.cpp
    resp_gen_test.cpp
    cookie_parse_test.cpp
    cookie_gen_test.cpp
    header_fields_parse_test.cpp
    response_body_test.cpp
    prereq_handler_test.cpp
    misc_tests.cpp
    test-main.cpp
)
add_executable(test-http-common ${test_http_common_SRCS})
target_link_libraries(test-http-common
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIB}
    ${TIP_IRI_LIB}
    ${HTTP_COMMON_LIB}
)
add_test(
    NAME test-http-common
    COMMAND test-http-common ${TEST_COMMON_OUT_ARGS}
)

# HTTP Client test
set(
    client_test_SRCS
    client_tests.cpp
    test-main.cpp
)
add_executable(test-http-client ${client_test_SRCS})
target_link_libraries(
    test-http-client
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIB}
    ${TIP_IRI_LIB}
    ${HTTP_CLIENT_LIB}
)
add_test(
    NAME test-http-client
    COMMAND test-http-client ${TEST_CLIENT_OUT_ARGS}
)

# HTTP Server test
set(
    server_test_SRCS
    test-main.cpp
    wildcard_test.cpp
    pathmatcher_parse_test.cpp
    pathmatcher_test.cpp
    locale_name_parse_test.cpp
    locale_manager_test.cpp
    #error_serialize.cpp
)

add_executable(test-http-server ${server_test_SRCS})
target_link_libraries(test-http-server
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${HTTP_SERVER_LIB}
    #${PUSHKIN_L10N_LIBRARIES}
)

add_test(
    NAME test-http-server
    COMMAND test-http-server ${TEST_SERVER_OUT_ARGS}
)

add_executable(psst-get psst_get.cpp)
target_link_libraries(
    psst-get
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TIP_LOG_LIB}
    ${TIP_IRI_LIB}
    ${HTTP_CLIENT_LIB}
)


