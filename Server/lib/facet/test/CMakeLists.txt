#	/facet/test/CMakeLists.txt
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

# Add your package dependencies for test here

include_directories(${GTEST_INCLUDE_DIRS})

set(
	test_facet_SRCS
	facet_test.cpp
)
add_executable(test-facet ${test_facet_SRCS})
target_link_libraries(
	test-facet
	${GTEST_BOTH_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	# Add your libraries here
)

if (GTEST_XML_OUTPUT)
    set (
        TEST_ARGS
        --gtest_output=xml:test-utils-detail.xml
    )
endif()

add_test(
    NAME test-facet
    COMMAND test-facet ${TEST_ARGS}
)
