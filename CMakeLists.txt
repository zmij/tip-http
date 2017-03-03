#    CMakeLists.txt
#
#    @author zmij
#    @date Dec 7, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT PROJECT_PREFIX)
    set(PROJECT_PREFIX tip)
endif()

if (PROJECT_VERSION)
    # Built as a subproject
    set(_pversion ${PROJECT_VERSION})
else()
    set(_pversion 0.1.0)
endif()

set(_pname ${PROJECT_PREFIX}-http)

if (NOT TIP_LOG_INCLUDE_DIRS)
    set(_TIP_LOG_SUBTREE ON)
endif()

if (NOT TIP_IRI_INCLUDE_DIRS)
    set(_TIP_IRI_SUBTREE ON)
endif()

if(NOT TIP_UTIL_INCLUDE_DIRS)
    set(TIP_UTIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/util/include)
endif()

if (NOT TIP_IRI_LIB)
    set(TIP_IRI_LIB tip-iri)
endif()
if (NOT TIP_LOG_LIB)
    set(TIP_LOG_LIB tip-log)
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.0")
    cmake_policy(SET CMP0048 NEW)
    project(${_pname} VERSION ${_pversion})
else()
    project(${_pname})
    set(PROJECT_VERSION ${_pversion})
endif()

set(HTTP_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(HTTP_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/scripts/header_gen.cmake)

add_definitions("-std=c++11")

set(BOOST_COMPONENTS
    system
    thread
    locale
)

find_package(Boost 1.58 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(Threads REQUIRED)

set(HTTP_COMMON_LIB ${PROJECT_PREFIX}-http-common)
set(HTTP_CLIENT_LIB ${PROJECT_PREFIX}-http-client)
set(HTTP_SERVER_LIB ${PROJECT_PREFIX}-http-server)

add_subdirectory(data)

add_subdirectory(lib/ssl)
if(_TIP_LOG_SUBTREE)
    add_subdirectory(lib/log)
    set(_TIP_LOG_SUBTREE ${_TIP_LOG_SUBTREE} CACHE INTERNAL "Include tip-log as subtree")
endif()
if(_TIP_IRI_SUBTREE)
    add_subdirectory(lib/iri)
    set(_TIP_IRI_SUBTREE ${_TIP_IRI_SUBTREE} CACHE INTERNAL "Include tip-log as subtree")
endif()

set(CEREAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/cereal/include)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${TIP_IRI_INCLUDE_DIRS}
    ${TIP_LOG_INCLUDE_DIRS}
    ${TIP_SSL_INCLUDE_DIRS}
    ${TIP_UTIL_INCLUDE_DIRS}
    ${CEREAL_INCLUDE_DIR}
)

link_directories(${TIP_IRI_LIB_DIR} ${TIP_LOG_LIB_DIR})

add_subdirectory(include/tip/http)
add_subdirectory(src/tip/http)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_HTTP_COMMON_LIB ${HTTP_COMMON_LIB} CACHE INTERNAL "HTTP common library target name")
    set(TIP_HTTP_CLIENT_LIB ${HTTP_CLIENT_LIB} CACHE INTERNAL "HTTP client library target name")
    set(TIP_HTTP_SERVER_LIB ${HTTP_SERVER_LIB} CACHE INTERNAL "HTTP server library target name")
    set(CEREAL_INCLUDE_DIR ${CEREAL_INCLUDE_DIR} CACHE INTERNAL "Cereal include directories")
    set(
        TIP_HTTP_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CEREAL_INCLUDE_DIR}
        CACHE INTERNAL "HTTP include directories"
    )
endif()
