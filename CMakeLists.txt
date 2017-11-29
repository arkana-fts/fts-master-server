
project(fts-server)
message(STATUS "======================")
message(STATUS "CONFIGURING FTS-SERVER")
message(STATUS "======================")

#set(CMAKE_VERBOSE_MAKEFILE ON)

# Setup the make environment. #
################################
cmake_minimum_required(VERSION 3.1)

# Setup the cmake directory containing numerous cmake scripts and macros.
set(CMAKE_MODULE_PATH "${fts-server_SOURCE_DIR}/cmake")
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Disallow in-source builds. #
##############################
EXECUTE_PROCESS(COMMAND pwd OUTPUT_VARIABLE CURR_DIR)
if("${CURR_DIR}" STREQUAL "${fts-server_SOURCE_DIR}\n")
    message(FATAL_ERROR "In-source-builds are not allowed to build the Arkana-FTS server. Please go into the \"build\" directory and type \"cmake ..\" there.\nThank you.")
endif()

# Set the Run time lib version to static #
##########################################
if(MSVC)
    FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH(flag_var)
endif()

# The threads library to use and the mysql client library. #
############################################################
find_package(Threads REQUIRED)
find_package(MYSQL REQUIRED)

# Add fts-networking stuff #
############################
set(fts-network CACHE STRING "Location of the FTS networking library.")
set(fts-network_LIB_DIR "${fts-network}/lib")
set(fts-network_INCLUDE_DIR "${fts-network}/include")

add_library(fts-net STATIC IMPORTED)
if(MSVC)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_DEBUG ${fts-network_LIB_DIR}/Debug/fts-net_d.lib)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_RELEASE ${fts-network_LIB_DIR}/Release/fts-net.lib)
else()
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_DEBUG ${fts-network_LIB_DIR}/libfts-net_d.a)
    set_target_properties( fts-net PROPERTIES IMPORTED_LOCATION_RELEASE ${fts-network_LIB_DIR}/libfts-net.a)
endif()

# Put all source files into one variable. #
##########################################
set(src main.cpp Server.cpp channel.cpp client.cpp db.cpp db_build.cpp dummy_types.cpp game.cpp  
        ClientsManager.cpp ChannelManager.cpp GameManager.cpp
        GetOpt.cpp
   )
set(HDR constants.h Server.h channel.h client.h db.h dummy_types.h game.h  
        ClientsManager.h IClientsManager.h ChannelManager.h GameManager.h
        Singleton.h GetOpt.h Mutex.h
   ) 
   
set( CHECKSUM ./checksum/sha2.cpp ./checksum/md5.cpp )

if(MSVC)
    source_group( Checksum FILES ${CHECKSUM})
    source_group( Header FILES ${HDR})
    source_group( Source FILES ${src})
endif()

# The configuration settings.
set(RUN_AS_USER "fts" CACHE STRING "The user as which to run the server, in case root starts it.")

set(LOG_MAX_LOGFILE_KBYTES 10240 CACHE STRING "Maximum length of a logfile, in kb, before a new one is used.")
if(MSVC)
    set(LOG_DIR "$ENV{LOCALAPPDATA}/fts/log" CACHE PATH "Where to write the logfiles?")
else()
    set(LOG_DIR "/var/log" CACHE PATH "Where to write the logfiles?")
endif()
set(LOG_PLAYERCOUNT_FILE "nplayers.txt" CACHE FILEPATH "File to write the amount of players currently online to.")
set(LOG_GAMECOUNT_FILE "ngames.txt" CACHE FILEPATH "File to write the amount of games currently being played to.")

set(PORT_FIRST "0xAF75" CACHE STRING "First port in the range of ports to listen to.")
set(PORT_LAST "0xAF7F" CACHE STRING "Last port in the range of ports to listen to.")

set(MYSQL_HOST "localhost" CACHE STRING "The server holding the MySQL tables to use.")
set(MYSQL_USER "fts" CACHE STRING "The MySQL username that should access the db.")
set(MYSQL_PASS "" CACHE STRING "The password of the MySQL user.")
set(MYSQL_DB "arkana_game" CACHE STRING "The name of the database to use.")

set(CHAT_DEFAULT_CHANNEL_NAME "Talk To Survive (main channel)" CACHE STRING "The name of the channel new users land in.")
set(CHAT_DEFAULT_CHANNEL_MOTTO "This is the main channel in Arkana-FTS where everybody meets" CACHE STRING "The motto of the channel new users land in.")
set(CHAT_DEFAULT_CHANNEL_ADMIN "Pompei2" CACHE STRING "The user who should be admin of the default channel.")
set(CHAT_DEVS_CHANNEL_NAME "Dev's Channel" CACHE STRING "The name of the developer's channel.")
set(CHAT_DEVS_CHANNEL_MOTTO "The channel for the developers of Arkana-FTS and the players wanting to know more." CACHE STRING "The motto of the dev's channel")
set(CHAT_DEVS_CHANNEL_ADMIN "Pompei2" CACHE STRING "The user who should be admin of the dev's channel.")
set(CHAT_DEFAULT_MOTTO "Arkana-FTS Rules!" CACHE STRING "The motto any newly created channel has by default.")
set(CHAT_MAX_CHANS_PER_USER "3" CACHE STRING "The maximum amount of channels a user can create/own.")

configure_file("${fts-server_SOURCE_DIR}/config.h.in" "config.h" ESCAPE_QUOTES)

# The compiling process. #
##########################
add_executable(fts-server ${src} ${HDR} ${CHECKSUM})
if(MSVC)
    target_compile_definitions(fts-server PRIVATE EHsc _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(Wall)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(fts-server PRIVATE _DEBUG)
    endif()
endif()
target_include_directories(fts-server PRIVATE ${fts-server_SOURCE_DIR} ${fts-server_BINARY_DIR} ${MYSQL_INCLUDE_DIR} ${fts-network_INCLUDE_DIR})
set_property(TARGET fts-server PROPERTY CXX_STANDARD 14)
set_property(TARGET fts-server PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(fts-server fts-net ${MYSQL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS fts-server DESTINATION bin)
