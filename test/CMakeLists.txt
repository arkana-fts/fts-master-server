
project(fts-server-test)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# Setup the make environement. #
################################
cmake_minimum_required(VERSION 3.1)


# Setup the cmake directory containing numerous cmake scripts and macros.
set(CMAKE_MODULE_PATH "${fts-server-test_SOURCE_DIR}/cmake")

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Disallow in-source builds. #
##############################
EXECUTE_PROCESS(COMMAND pwd OUTPUT_VARIABLE CURR_DIR)
if("${CURR_DIR}" STREQUAL "${fts-server-test_SOURCE_DIR}\n")
    message(FATAL_ERROR "In-source-builds are not allowed to build the Arkana-FTS server. Please go into the \"build\" directory and type \"cmake ..\" there.\nThank you.")
endif()

if(MSVC)
    FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH(flag_var)
endif()

# Add fts-networking stuff
set(fts-network_SOURCE_DIR CACHE STRING "Location of the FTS networking library.")
set(fts-network_LIB_DIR "${fts-network_SOURCE_DIR}/lib")
include_directories("${fts-network_SOURCE_DIR}/include")
add_library(fts-net STATIC IMPORTED)
if(MSVC)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_DEBUG ${fts-network_LIB_DIR}/Debug/fts-net_d.lib)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_RELEASE ${fts-network_LIB_DIR}/Release/fts-net.lib)
else()
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_DEBUG ${fts-network_LIB_DIR}/libfts-net_d.a)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_RELEASE ${fts-network_LIB_DIR}/libfts-net.a)
endif()

# Define all sourcefiles. #
###########################
set(TEST_SRC Singleton_test.cpp Server_test.cpp)
set(HDR catch.hpp ../Singleton.h ../Server.h ) 
set(SRC ../Server.cpp ) 
   
if(MSVC)
    source_group( Header FILES ${HDR})
    source_group( Source FILES ${TEST_SRC})
    source_group( Imported\ Source FILES ${SRC})
endif()

# Compiler-dependent and build-dependend flags:
if( NOT MSCV)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DDEBUG=1)
    endif()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# Add additional source directories.
include_directories(${fts-server-test_SOURCE_DIR})

 
# The compiling process. #
##########################

add_executable(fts-server-test ${TEST_SRC} ${SRC} ${HDR})
set_property(TARGET fts-server-test PROPERTY CXX_STANDARD 14)
set_property(TARGET fts-server-test PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_custom_command(TARGET fts-server-test POST_BUILD
                       COMMAND $(TargetPath) ARGS "-s" COMMENT "Run the test suite")
else(MSVC)
    add_custom_command(TARGET fts-server-test POST_BUILD
                       COMMAND fts-server-test ARGS "-s" COMMENT "Run the test suite")
endif(MSVC)

target_link_libraries(fts-server-test fts-net)
