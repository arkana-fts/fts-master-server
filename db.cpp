
#include <tuple>

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

namespace {
inline MYSQL* conv(FTSSrv2::db_ptr* p) { return (MYSQL*) p; }
inline MYSQL_RES* conv(FTSSrv2::db_result* p) { return (MYSQL_RES*) p; }

}

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
        mysql_close(conv(m_pSQL));
        m_pSQL = nullptr;
    }
}

std::vector<std::tuple<int, bool, std::string, std::string, std::string>> FTSSrv2::DataBase::getChannels()
{
    db_result *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;

    // Do the query to get the field.
    string sQuery = "SELECT  `" + TblChansField(DSRV_TBL_CHANS_ID) + "`"
        ",`" + TblChansField(DSRV_TBL_CHANS_PUBLIC) + "`"
        ",`" + TblChansField(DSRV_TBL_CHANS_NAME) + "`"
        ",`" + TblChansField(DSRV_TBL_CHANS_MOTTO) + "`"
        ",`" + TblChansField(DSRV_TBL_CHANS_ADMIN) + "`"
        " FROM `" DSRV_TBL_CHANS "`";

    std::vector<std::tuple<int, bool, std::string, std::string, std::string>> result;

    if( !query(pRes, sQuery) ) {
        free(pRes);
        return result;
    }

    // Invalid record ? forget about it!
    if( pRes == nullptr || mysql_num_fields(conv(pRes)) < 5 ) {
        free(pRes);
        return result;
    }

    while( nullptr != (pRow = mysql_fetch_row(conv(pRes))) ) {
        std::tuple<int, bool, std::string, std::string, std::string> r = make_tuple(atoi(pRow[0]),
            (pRow[1] == nullptr ? false : (pRow[1][0] == '0' ? false : true)), pRow[2], pRow[3], pRow[4]);
        result.push_back(r);
    }

    free(pRes);
    return result;
}

std::vector<std::tuple<std::string, std::string>> FTSSrv2::DataBase::getChannelOperators()
{
    db_result *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;
    std::vector<std::tuple<std::string, std::string>> res;

    string sQuery = "SELECT `" + TblChanOpsField(DSRV_VIEW_CHANOPS_NICK) + "`,`" + TblChanOpsField(DSRV_VIEW_CHANOPS_CHAN) + "` "
                    "FROM `" DSRV_VIEW_CHANOPS "`";
    if( !query(pRes, sQuery) ) {
        free(pRes);
        return res;
    }

    // Invalid record ? forget about it!
    if( pRes == nullptr || mysql_num_fields(conv(pRes)) < 2 ) {
        free(pRes);
        return res;
    }

    // Setup every operator<->channel connection.
    // But first just put all assocs. in a list because we need to free the DB.
    while( nullptr != (pRow = mysql_fetch_row(conv(pRes))) ) {
        auto row = make_tuple(pRow[0],pRow[1]);
        res.push_back(row);
    }

    free(pRes);

    return res;
}

bool FTSSrv2::DataBase::channelCreate(const std::tuple<bool, std::string, std::string, std::string>& record)
{
    string sQuery = "\'" + escape(std::get<1>(record)) + "\', " +
        "\'" + escape(std::get<2>(record)) + "\', " +
        "\'" + escape(std::get<3>(record)) + "\', " +
        string(std::get<0>(record) ? "1" : "0");

    auto iID = storedFunctionInt("channelCreate", sQuery);
    if( iID < 0 ) {
        FTSMSG("Error saving the channel:mysql stored function returned " + toString(iID), MsgType::Error);
        return false;
    }
    return true;
}

bool FTSSrv2::DataBase::channelUpdate(const std::tuple<int, bool, std::string, std::string, std::string>& record)
{
    string sQuery = "UPDATE `" DSRV_TBL_CHANS "`"
        " SET `" + TblChansField(DSRV_TBL_CHANS_NAME) + "`"
        "=\'" + escape(std::get<2>(record)) + "\',"
        "`" + TblChansField(DSRV_TBL_CHANS_MOTTO) + "`"
        "=\'" + escape(std::get<3>(record)) + "\',"
        "`" + TblChansField(DSRV_TBL_CHANS_ADMIN) + "`"
        "=\'" + escape(std::get<4>(record)) + "\',"
        "`" + TblChansField(DSRV_TBL_CHANS_PUBLIC) + "`"
        "=" + (std::get<1>(record) ? string("1") : string("0")) +
        " WHERE `" + TblChansField(DSRV_TBL_CHANS_ID) + "`"
        "=" + toString(std::get<0>(record)) +
        " LIMIT 1";

    db_result *pDummy;
    query(pDummy, sQuery);
    free(pDummy);
    return true;
}

void FTSSrv2::DataBase::channelAddOp(const std::tuple<std::string, int>& record)
{
    string sQuery = "\'" + escape(std::get<0>(record)) + "\'," + toString(std::get<1>(record));
    storedFunctionInt("channelAddOp", sQuery);
}

int FTSSrv2::DataBase::channelDestroy(const std::tuple<std::string, std::string>& record)
{
    string sQuery = "\'" + escape(std::get<0>(record)) + "\', \'" + escape(std::get<1>(record)) + "\'";
    return storedFunctionInt("channelDestroy", sQuery);
}

int FTSSrv2::DataBase::disconnect(const std::tuple<std::string, std::string, std::string, int>& record)
{
    auto iRet = storedFunctionInt(string("disconnect"),
                                         "\'" + escape(std::get<0>(record)) + "\', "
                                         "\'" + std::get<1>(record) + "\',"
                                         "\'" + escape(std::get<2>(record)) + "\',"
                                         "\'" + toString(std::get<3>(record)) + "\'");

    return iRet;
}

int FTSSrv2::DataBase::connect(const std::tuple<std::string, std::string, std::string, int>& record)
{
    auto iRet = storedFunctionInt(string("connect"),
                                  "\'" + escape(std::get<0>(record)) + "\', "
                                  "\'" + std::get<1>(record) + "\',"
                                  "\'" + escape(std::get<2>(record)) + "\',"
                                  "\'" + toString(std::get<3>(record)) + "\'");

    return iRet;
}

int FTSSrv2::DataBase::signup(const std::tuple<std::string, std::string, std::string>& record)
{
    auto iRet = storedFunctionInt(string("connect"),
                                  "\'" + escape(std::get<0>(record)) + "\', "
                                  "\'" + escape(std::get<1>(record)) + "\',"
                                  "\'" + escape(std::get<2>(record)) + "\'");

    return iRet;
}

int FTSSrv2::DataBase::insertFeedback(const std::tuple<std::string, std::string>& record)
{
    string sQuery = "INSERT INTO `" DSRV_TBL_FEEDBACK "` ("
        " `" + TblFeedbackField(DSRV_TBL_FEEDBACK_NICK) + "`,"
        " `" + TblFeedbackField(DSRV_TBL_FEEDBACK_MSG) + "`,"
        " `" + TblFeedbackField(DSRV_TBL_FEEDBACK_WHEN) + "`) "
        "VALUES ("
        " '" + escape(std::get<0>(record)) + "',"
        " '" + escape(std::get<1>(record)) + "',"
        " NOW())";

    db_result *pRes;
    int iRet = ERR_OK;
    if( !query(pRes, sQuery) ) {
        iRet = -1;
    }

    free(pRes);
    return iRet;
}

int FTSSrv2::DataBase::updatePlayerSet(const std::tuple<std::uint8_t, std::string, std::string, std::string>& record)
{
    string sField = TblUsrField(std::get<0>(record));
    if( sField.empty() ) {
        FTSMSG("failed: unknown field definer (0x" + toString(std::get<0>(record), -1, '0', std::ios::hex) + ")", MsgType::Error);
        return 2;
    }
    string sQuery = "UPDATE `" DSRV_TBL_USR "`"
        " SET `" + sField + "` = \'" + escape(std::get<1>(record)) + "\'"
        " WHERE `" + TblUsrField(DSRV_TBL_USR_NICK) + "`"
        " = \'" + escape(std::get<2>(record)) + "\'"
        " AND `" + TblUsrField(DSRV_TBL_USR_PASS_MD5) + "`"
        " = \'" + escape(std::get<3>(record)) + "\' "
        " LIMIT 1";

    db_result *pRes;
    int iRet = ERR_OK;
    if( !query(pRes, sQuery) ) {
        iRet = 3;
        FTSMSG("failed: error during the sql update.", MsgType::Error);
    } else {
        FTSMSGDBG("success", 4);
    }
    free(pRes);
    return ERR_OK;
}

std::tuple<int, std::string>  FTSSrv2::DataBase::getUserPropertyNo(const std::tuple<std::uint8_t, std::string, std::string>& record)
{
    string sArgs = "\'" + escape(std::get<1>(record)) + "\', " +
        +"\'" + escape(std::get<2>(record)) + "\', " +
        toString<int>(std::get<0>(record));

    db_result *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;
    int iRet = ERR_OK;
    string resultRow;
    if( !storedProcedure(pRes, "getUserPropertyNo", sArgs) ) {
        iRet = 1;
    } else {
        // Get the query result.
        if( pRes == nullptr || nullptr == (pRow = mysql_fetch_row(conv(pRes))) ) {
            FTSMSG("failed: sql fetch row: " + getError(), MsgType::Error);
            iRet = 2;
        } else {
            // And extract the data from the result and add it to the packet.
            resultRow = pRow[0] == nullptr ? "" : pRow[0];
        }
    }
    free(pRes);

    return make_tuple(iRet, resultRow);
}

int FTSSrv2::DataBase::setUserFlag(const std::tuple<std::string, std::uint32_t, bool>& record)
{
    auto string_b = [](bool b) -> string { return b ? string("True") : std::string("False"); };
    string sArgs = "\'" + escape(std::get<0>(record)) + "\', " +
        +"\'" + toString<int>(std::get<1>(record)) + "\', " + string_b(std::get<2>(record));
    return storedFunctionInt("setUserFlag", sArgs);
}

int FTSSrv2::DataBase::updateLocation(const std::tuple<std::string, std::string>& record)
{
    string sQuery = "UPDATE `" DSRV_TBL_USR "`"
        " SET `" + TblUsrField(DSRV_TBL_USR_LOCATION) + "`"
        "='chan:" + escape(std::get<0>(record)) + "'"
        " WHERE `" + TblUsrField(DSRV_TBL_USR_NICK) + "`"
        "='" + escape(std::get<1>(record)) + "'"
        " LIMIT 1";
    db_result *pRes = nullptr;
    query(pRes, sQuery);
    free(pRes);
    return ERR_OK;
}

int FTSSrv2::DataBase::init()
{
    if(nullptr == (m_pSQL = (db_ptr*)mysql_init(nullptr))) {
        FTSMSG("[ERROR] MySQL init.", MsgType::Error);
	return -1;
    }
    MYSQL* pSQL = conv(m_pSQL);

    char bTrue = 1;
    mysql_options(pSQL, MYSQL_OPT_RECONNECT, &bTrue);
    mysql_options(pSQL, MYSQL_OPT_COMPRESS, &bTrue);

    if(nullptr == mysql_real_connect(pSQL, DSRV_MYSQL_HOST, DSRV_MYSQL_USER,
                                  DSRV_MYSQL_PASS, DSRV_MYSQL_DB, 0, NULL,
                                  CLIENT_MULTI_STATEMENTS)) {
        FTSMSG("[ERROR] MySQL connect: "+this->getError(), MsgType::Error);
        mysql_close(pSQL);
        m_pSQL = nullptr;
        return -2;
    }

    if(0 != mysql_set_character_set(pSQL, "utf8"))
        FTSMSG("[ERROR] MySQL characterset: "+this->getError()+" .. ignoring", MsgType::Error);

    this->buildupDataBase();

    // Call our stored procedure that initialises the database.
    // We don't care about the result, there is none.
    db_result *pRes;
    this->storedProcedure(pRes, "init", "");
    free(pRes);

    return 0;
}

int FTSSrv2::DataBase::free(db_result *&out_pRes)
{
    // Free the current result set first:
    if(out_pRes) {
        mysql_free_result(conv(out_pRes));
    }
    out_pRes = nullptr;

    // And then every other if there are some more from multiple queries:
    while(mysql_next_result(conv(m_pSQL)) == 0) {
        MYSQL_RES *pRes = mysql_store_result(conv(m_pSQL));
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
    auto iLen = mysql_real_escape_string( conv(m_pSQL), buf, in_sStr.c_str(), (unsigned long)in_sStr.size() );

    std::string sRet( buf, iLen );
    delete[] buf;

    return sRet;

}

std::string FTSSrv2::DataBase::getError()
{
    return std::string( mysql_error( conv(m_pSQL) ) );
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! DONT FORGET to call this->free on the returned value when it's != NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! when finished to work with it, cause it then gets unblocked for other threads to execute queries. !! //
// !! Also, DONT FORGET TO END THIS function's parameter list with a NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !! Also, note that only the last query result is kept !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
bool FTSSrv2::DataBase::query(db_result *&out_pRes, std::string in_sQuery)
{
    // If the result has to be freed, this mutex only gets unlocked when calling free.
    m_mutex.lock();
    FTSMSGDBG( "SQL Query : " + in_sQuery, 4 );
    // Execute the query.
    auto pSQL = conv(m_pSQL);
    if(0 != mysql_query(pSQL, in_sQuery.c_str())) {
        FTSMSG("[ERROR] MySQL query\nQuery string: "+ in_sQuery +"\nError: "+this->getError(), MsgType::Error);

        unsigned int error = mysql_errno(pSQL);
        if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR || error == CR_SERVER_LOST_EXTENDED) {
            FTSMSG("Server lost, bye bye.\n", MsgType::Error);
            exit(0);
        }
        return false;
    }

    out_pRes = (db_result*)mysql_store_result(pSQL);
    if(out_pRes == nullptr && mysql_field_count(pSQL) != 0) {
        // Errors occured during the query. (connection ? too large result ?)
        FTSMSG("[ERROR] MySQL field count\nQuery string: "+ in_sQuery +"\nError: "+this->getError(), MsgType::Error);
        return false;
    }

    return true;
}

int FTSSrv2::DataBase::storedFunctionInt(std::string in_pszFunc, std::string in_pszArgs)
{
    db_result *pRes = nullptr;
    string sQuery = "SELECT `" DSRV_MYSQL_DB "`.`" + in_pszFunc + "` ( "+ in_pszArgs + " )";
    if(!this->query(pRes, sQuery)) {
        return -1;
    }

    MYSQL_ROW row = nullptr;
    // How can it happen that a stored function returns nothing ??
    if(nullptr == (row = mysql_fetch_row(conv(pRes)))) {
        this->free(pRes);
        FTSMSG("[ERROR] MySQL fetch stored function "+ in_pszFunc +"\nArguments: "+ in_pszArgs +"\nError: "+this->getError(), MsgType::Error);
        return -1;
    }

    int iRet = atoi(row[0]);
    this->free(pRes);
    return iRet;
}

bool FTSSrv2::DataBase::storedProcedure(db_result *&out_pRes,  std::string in_pszProc, std::string in_pszArgs)
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
