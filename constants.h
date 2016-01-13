#ifndef D_CONSTANTS_H
#  define D_CONSTANTS_H

// please refer to the dokuwiki page for more detailed information.

#include <dsrv_constants.h>

/////////////////////////////////////////////////
// Other misc. constants used in the programs. //
/////////////////////////////////////////////////

// Anti flooding system.
#  ifdef D_USE_ANTIFLOOD
     /// Per ten bytes, how many milliseconds have to be gone ?
#    define D_ANTIFLOOD_DELAY_PER_TEN_BYTE 2
     /// If two packets are sent in less then this, it's recognized as flood w/o other tests.
#    define D_ANTIFLOOD_LIMIT              30
     /// The number of floods allowed.
#    define D_ANTIFLOOD_ALLOWED_FLOODS     5
#  endif

// File names.
#  ifndef DSRV_LOGFILE_ERR
#    define DSRV_LOGFILE_ERR "fts_server.err"
#  endif
#  ifndef DSRV_LOGFILE_LOG
#    define DSRV_LOGFILE_LOG "fts_server.log"
#  endif
#  ifndef DSRV_LOGFILE_NETLOG
#    define DSRV_LOGFILE_NETLOG "fts_server.netlog"
#  endif

/// The version of this server.
#  define D_SERVER_VERSION_STR "0.9.12"

/// The common NO error return value
#define ERR_OK  0

#endif                          /* D_CONSTANTS_H */
