#ifndef D_CONSTANTS_H
#  define D_CONSTANTS_H

// please refer to the dokuwiki page for more detailed information.

// For uint8_t and family
#include <sys/types.h>

/////////////////////////////////////
// The constants used in messages. //
/////////////////////////////////////

// Chat flags
/////////////
#  define DSRV_CHAT_TYPE_NORMAL  ((uint8_t)0x01)
#  define DSRV_CHAT_TYPE_WHISPER ((uint8_t)0x02)
#  define DSRV_CHAT_TYPE_SYSTEM  ((uint8_t)0x04)

// Player status
////////////////
#  define DSRV_CHAT_USER_NORMAL   ((uint8_t)0)
#  define DSRV_CHAT_USER_OPERATOR ((uint8_t)1)
#  define DSRV_CHAT_USER_ADMIN    ((uint8_t)2)

////////////////////////////////////////////////////////
// The indexes of the different fields in the talbes. //
////////////////////////////////////////////////////////

// table: users
///////////////
#  define DSRV_TBL_USR             "users"
#  define DSRV_TBL_USR_ID          ((uint8_t)0)
#  define DSRV_TBL_USR_NICK        ((uint8_t)1)
#  define DSRV_TBL_USR_PASS_MD5    ((uint8_t)2)
#  define DSRV_TBL_USR_PASS_SHA    ((uint8_t)3)
#  define DSRV_TBL_USR_MAIL        ((uint8_t)4)
#  define DSRV_TBL_USR_FNAME       ((uint8_t)5)
#  define DSRV_TBL_USR_NAME        ((uint8_t)6)
#  define DSRV_TBL_USR_BDAY        ((uint8_t)7)
#  define DSRV_TBL_USR_SEX         ((uint8_t)8)
#  define DSRV_TBL_USR_CMT         ((uint8_t)9)
#  define DSRV_TBL_USR_JABBER      ((uint8_t)10)
#  define DSRV_TBL_USR_CONTACT     ((uint8_t)11)
#  define DSRV_TBL_USR_IP          ((uint8_t)12)
#  define DSRV_TBL_USR_LOCATION    ((uint8_t)13)
#  define DSRV_TBL_USR_SIGNUPD     ((uint8_t)14)
#  define DSRV_TBL_USR_LASTON      ((uint8_t)15)
#  define DSRV_TBL_USR_WEEKON      ((uint8_t)16)
#  define DSRV_TBL_USR_TOTALON     ((uint8_t)17)
#  define DSRV_TBL_USR_WINS        ((uint8_t)18)
#  define DSRV_TBL_USR_LOOSES      ((uint8_t)19)
#  define DSRV_TBL_USR_DRAWS       ((uint8_t)20)
#  define DSRV_TBL_USR_CLAN        ((uint8_t)21)
#  define DSRV_TBL_USR_FLAGS       ((uint8_t)22)
#  define DSRV_TBL_USR_COUNT       ((uint8_t)23)

// table: channels
//////////////////
#  define DSRV_TBL_CHANS         "channels"
#  define DSRV_TBL_CHANS_ID      ((uint8_t)0)
#  define DSRV_TBL_CHANS_NAME    ((uint8_t)1)
#  define DSRV_TBL_CHANS_MOTTO   ((uint8_t)2)
#  define DSRV_TBL_CHANS_ADMIN   ((uint8_t)3)
#  define DSRV_TBL_CHANS_PUBLIC  ((uint8_t)4)
#  define DSRV_TBL_CHANS_COUNT   ((uint8_t)5)

// table: user feedback
///////////////////////
#  define DSRV_TBL_FEEDBACK         "user_feedback"
#  define DSRV_TBL_FEEDBACK_ID      ((uint8_t)0)
#  define DSRV_TBL_FEEDBACK_NICK    ((uint8_t)1)
#  define DSRV_TBL_FEEDBACK_MSG     ((uint8_t)2)
#  define DSRV_TBL_FEEDBACK_WHEN    ((uint8_t)3)
#  define DSRV_TBL_FEEDBACK_COUNT   ((uint8_t)4)

// view: v_channelOps
/////////////////////
#  define DSRV_VIEW_CHANOPS        "v_channelOps"
#  define DSRV_VIEW_CHANOPS_NICK   ((uint8_t)0)
#  define DSRV_VIEW_CHANOPS_CHAN   ((uint8_t)1)
#  define DSRV_VIEW_CHANOPS_COUNT  ((uint8_t)2)

// connect procedure: in_where
///////////////////////////////
#  define DSRV_PROC_CONNECT_NONE   0x00
#  define DSRV_PROC_CONNECT_INGAME 0x01
#  define DSRV_PROC_CONNECT_INPAGE 0x02

/////////////////////////////////////////////////
// Other misc. constants used in the programs. //
/////////////////////////////////////////////////

// Flags of the players:
#  define DSRV_PLAYER_FLAG_HIDEMAIL ((uint32_t)0x01)

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
