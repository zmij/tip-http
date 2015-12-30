#	CMakeLists.txt
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

install(
    FILES include/tip/util/facet.hpp
    DESTINATION include/tip/util
)

if(BUILD_TESTS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    enable_testing()
    add_subdirectory(test)
endif()
