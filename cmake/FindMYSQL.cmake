# - Find MySQL
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if( WIN32 )
# On windows 64bit the default cmake finds the include in the x86 installation of the connectors.
# If so, reset the cache and search in the default 64bit program installation path.
# Also the env ProgramFiles always results to the x86 path. Only explicitly pointing to ProgramW6432 works.
	if( "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "AMD64" )
		if( MYSQL_INCLUDE_DIR )
			unset( MYSQL_INCLUDE_DIR CACHE)
		endif()
		set( ProgramPath $ENV{ProgramW6432} )
	else()
		set( ProgramPath $ENV{ProgramFiles} )
	endif()
endif()

find_path(MYSQL_INCLUDE_DIR mysql.h
    /usr/include/mysql
    /usr/local/include/mysql
	${ProgramPath}/MySQL/*/include
	$ENV{SystemDrive}/MySQL/*/include
    )

if( MSVC )
# Set lib path suffixes
# dist = for mysql binary distributions
# build = for custom built tree
	find_library(MYSQL_LIBRARIES NAMES mysqlclient
		PATHS
		$ENV{MYSQL_DIR}/lib
		${ProgramPath}/MySQL/*/lib
		$ENV{SystemDrive}/MySQL/*/lib
	)
	
	find_path( MYSQL_LIBRARIES_DIR mysqlclient.lib
		$ENV{MYSQL_DIR}/lib
		${ProgramPath}/MySQL/*/lib
		$ENV{SystemDrive}/MySQL/*/lib
	)
else(MSVC)
	find_library(MYSQL_LIBRARIES NAMES mysqlclient_r
		PATHS
		/usr/lib
		/usr/lib/mysql
		/usr/lib64/mysql
		/usr/local/lib/mysql
		/usr/local/lib64/mysql
		/usr/lib/x86_64-linux-gnu  # Ubuntu
	)
	find_path(MYSQL_LIBRARIES_DIR libmysqlclient_r.so
		/usr/lib
		/usr/lib/mysql
		/usr/lib64/mysql
		/usr/local/lib/mysql
		/usr/local/lib64/mysql
		/usr/lib/x86_64-linux-gnu  # Ubuntu
        )
endif(MSVC)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MYSQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_LIBRARIES_DIR MYSQL_INCLUDE_DIR)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES MYSQL_LIBRARIES_DIR)
