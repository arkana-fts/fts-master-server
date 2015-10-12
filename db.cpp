
//#include <stdio.h>
//#  include <fcntl.h>
//#  include <io.h>

#if defined(_MSC_VER)
#include <WinSock2.h>
#include <mysql.h>
#include <errmsg.h>
#else
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif

#include <Logger.h>

#include "db.h"
#include "config.h"
using namespace FTS;
using namespace FTSSrv2;
using namespace std;

std::string FTSSrv2::DataBase::m_psTblUsrFields[DSRV_TBL_USR_COUNT];
std::string FTSSrv2::DataBase::m_psTblChansFields[DSRV_TBL_CHANS_COUNT];
std::string FTSSrv2::DataBase::m_psTblChanOpsFields[DSRV_VIEW_CHANOPS_COUNT];
std::string FTSSrv2::DataBase::m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_COUNT];

FTSSrv2::DataBase::DataBase()
{
    m_pSQL = nullptr;

    // Fill the arrays with the table names.
    m_psTblUsrFields[DSRV_TBL_USR_ID]="id";
    m_psTblUsrFields[DSRV_TBL_USR_NICK]="nickname";
    m_psTblUsrFields[DSRV_TBL_USR_PASS_MD5]="password";
    m_psTblUsrFields[DSRV_TBL_USR_PASS_SHA]="passwordSHA";
    m_psTblUsrFields[DSRV_TBL_USR_MAIL]="e_mail";
    m_psTblUsrFields[DSRV_TBL_USR_JABBER]="jabber";
    m_psTblUsrFields[DSRV_TBL_USR_CONTACT]="contact";
    m_psTblUsrFields[DSRV_TBL_USR_FNAME]="f_name";
    m_psTblUsrFields[DSRV_TBL_USR_NAME]="name";
    m_psTblUsrFields[DSRV_TBL_USR_BDAY]="bday";
    m_psTblUsrFields[DSRV_TBL_USR_SEX]="sex";
    m_psTblUsrFields[DSRV_TBL_USR_CMT]="comment";
    m_psTblUsrFields[DSRV_TBL_USR_LOCATION]="location";
    m_psTblUsrFields[DSRV_TBL_USR_IP]="ip";
    m_psTblUsrFields[DSRV_TBL_USR_LOCATION]="location";
    m_psTblUsrFields[DSRV_TBL_USR_SIGNUPD]="signup_date";
    m_psTblUsrFields[DSRV_TBL_USR_LASTON]="last_online";
    m_psTblUsrFields[DSRV_TBL_USR_WEEKON]="week_online";
    m_psTblUsrFields[DSRV_TBL_USR_TOTALON]="total_online";
    m_psTblUsrFields[DSRV_TBL_USR_WINS]="wins";
    m_psTblUsrFields[DSRV_TBL_USR_LOOSES]="looses";
    m_psTblUsrFields[DSRV_TBL_USR_DRAWS]="draws";
    m_psTblUsrFields[DSRV_TBL_USR_CLAN]="clan";
    m_psTblUsrFields[DSRV_TBL_USR_FLAGS]="flags";

    m_psTblChansFields[DSRV_TBL_CHANS_ID]="id";
    m_psTblChansFields[DSRV_TBL_CHANS_NAME]="name";
    m_psTblChansFields[DSRV_TBL_CHANS_MOTTO]="motto";
    m_psTblChansFields[DSRV_TBL_CHANS_ADMIN]="admin";
    m_psTblChansFields[DSRV_TBL_CHANS_PUBLIC]="public";

    m_psTblChanOpsFields[DSRV_VIEW_CHANOPS_NICK]="nickname";
    m_psTblChanOpsFields[DSRV_VIEW_CHANOPS_CHAN]="channel";

    m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_ID]="ID";
    m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_NICK]="nickname";
    m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_MSG]="feedback";
    m_psTblFeedbackFields[DSRV_TBL_FEEDBACK_WHEN]="when";
}

FTSSrv2::DataBase::~DataBase()
{
    if(m_pSQL) {
        mysql_close(m_pSQL);
        m_pSQL = nullptr;
    }
}

int FTSSrv2::DataBase::init()
{
    if(nullptr == (m_pSQL = mysql_init(nullptr))) {
        FTSMSG("[ERROR] MySQL init: "+this->getError(), MsgType::Error);
        return -1;
    }

    char bTrue = 1;
    mysql_options(m_pSQL, MYSQL_OPT_RECONNECT, &bTrue);
    mysql_options(m_pSQL, MYSQL_OPT_COMPRESS, &bTrue);

    if(nullptr == mysql_real_connect(m_pSQL, DSRV_MYSQL_HOST, DSRV_MYSQL_USER,
                                  DSRV_MYSQL_PASS, DSRV_MYSQL_DB, 0, NULL,
                                  CLIENT_MULTI_STATEMENTS)) {
        FTSMSG("[ERROR] MySQL connect: "+this->getError(), MsgType::Error);
        mysql_close(m_pSQL);
        m_pSQL = nullptr;
        return -2;
    }

    if(0 != mysql_set_character_set(m_pSQL, "utf8"))
        FTSMSG("[ERROR] MySQL characterset: "+this->getError()+" .. ignoring", MsgType::Error);

    this->buildupDataBase();

    // Call our stored procedure that initialises the database.
    // We don't care about the result, there is none.
    MYSQL_RES *pRes;
    this->storedProcedure(pRes, "init", "");
    FTSSrv2::DataBase::getUniqueDB()->free(pRes);

    return 0;
}

int FTSSrv2::DataBase::free(MYSQL_RES *&out_pRes)
{
    // Free the current result set first:
    if(out_pRes) {
        mysql_free_result(out_pRes);
    }
    out_pRes = nullptr;

    // And then every other if there are some more from multiple queries:
    while(mysql_next_result(m_pSQL) == 0) {
        MYSQL_RES *pRes = mysql_store_result(m_pSQL);
        if(pRes) {
            mysql_free_result(pRes);
        }

        // We don't care about errors.
    }

    m_mutex.unlock();
    return 0;
}

/// Returns the MySQL escaped version of this string.
/// this creates a string that is a copy of this one, but escaped for mysql commands.
///
/// \param in_sStr The string to escape.
///
/// \return The MySQL escaped copy of this string.
std::string FTSSrv2::DataBase::escape( const std::string & in_sStr )
{
    char *buf = new char[in_sStr.size() * 2 + 1];
    auto iLen = mysql_real_escape_string( m_pSQL, buf, in_sStr.c_str(), (unsigned long)in_sStr.size() );

    std::string sRet( buf, iLen );
    delete[] buf;

    return sRet;

}

std::string FTSSrv2::DataBase::getError()
{
    return std::string( mysql_error( m_pSQL ) );
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! DONT FORGET to call this->free on the returned value when it's != NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! when finished to work with it, cause it then gets unblocked for other threads to execute queries. !! //
// !! Also, DONT FORGET TO END THIS function's parameter list with a NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! Also, note that only the last query result is kept !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
bool FTSSrv2::DataBase::query(MYSQL_RES *&out_pRes, std::string in_sQuery)
{
    // If the result has to be freed, this mutex only gets unlocked when calling free.
    m_mutex.lock();
    FTSMSGDBG( "SQL Query : " + in_sQuery, 4 );
    // Execute the query.
    if(0 != mysql_query(m_pSQL, in_sQuery.c_str())) {
        FTSMSG("[ERROR] MySQL query\nQuery string: "+ in_sQuery +"\nError: "+this->getError(), MsgType::Error);

        unsigned int error = mysql_errno(m_pSQL);
        if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR || error == CR_SERVER_LOST_EXTENDED) {
            FTSMSG("Server lost, bye bye.\n", MsgType::Error);
            exit(0);
        }
        return false;
    }

    out_pRes = mysql_store_result(m_pSQL);
    if(out_pRes == nullptr && mysql_field_count(m_pSQL) != 0) {
        // Errors occured during the query. (connection ? too large result ?)
        FTSMSG("[ERROR] MySQL field count\nQuery string: "+ in_sQuery +"\nError: "+this->getError(), MsgType::Error);
        return false;
    }

    return true;
}

int FTSSrv2::DataBase::storedFunctionInt(std::string in_pszFunc, std::string in_pszArgs)
{
    MYSQL_RES *pRes = nullptr;
    string sQuery = "SELECT `" DSRV_MYSQL_DB "`.`" + in_pszFunc + "` ( "+ in_pszArgs + " )";
    if(!this->query(pRes, sQuery)) {
        return -1;
    }

    MYSQL_ROW row = nullptr;
    // How can it happen that a stored function returns nothing ??
    if(nullptr == (row = mysql_fetch_row(pRes))) {
        this->free(pRes);
        FTSMSG("[ERROR] MySQL fetch stored function "+ in_pszFunc +"\nArguments: "+ in_pszArgs +"\nError: "+this->getError(), MsgType::Error);
        return -1;
    }

    int iRet = atoi(row[0]);
    this->free(pRes);
    return iRet;
}

bool FTSSrv2::DataBase::storedProcedure(MYSQL_RES *&out_pRes,  std::string in_pszProc, std::string in_pszArgs)
{
    return this->query(out_pRes, string( "CALL `" DSRV_MYSQL_DB "`.`" ) + in_pszProc + "` ( " + in_pszArgs + " )");
}

static FTSSrv2::DataBase *g_pTheDatabase = nullptr;

int FTSSrv2::DataBase::initUniqueDB()
{
    g_pTheDatabase = new FTSSrv2::DataBase();
    if(0 != g_pTheDatabase->init()) {
        delete g_pTheDatabase;
        g_pTheDatabase = nullptr;
        return -1;
    }

    return 0;
}

FTSSrv2::DataBase *FTSSrv2::DataBase::getUniqueDB()
{
    return g_pTheDatabase;
}

int FTSSrv2::DataBase::deinitUniqueDB()
{
    delete g_pTheDatabase;
    g_pTheDatabase = nullptr;
    return 0;
}
