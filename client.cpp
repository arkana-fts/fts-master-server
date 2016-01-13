
#include <thread>
#include <chrono>

#include <Logger.h>
#include <TextFormatting.h>

#include "channel.h"
#include "Server.h"
#include "client.h"
#include "constants.h"
#include "db.h"
#include "game.h"

using namespace FTS;
using namespace FTSSrv2;
using namespace std;

std::string DSRV_CHAT_USER_toString( DSRV_CHAT_USER user )
{
    std::string s;
    switch( user ) {
        case DSRV_CHAT_USER::ADMIN:
            s = "ADMIN";
            break;
        case DSRV_CHAT_USER::OPERATOR:
            s = "OPERATOR";
            break;
        case DSRV_CHAT_USER::NORMAL:
            s = "NORMAL";
            break;
        default:
            s = "UNKNOWN";
            break;
    }
    return s;
}

std::string DSRV_CHAT_TYPE_toString( DSRV_CHAT_TYPE type)
{
    std::string s;
    switch( type ) {
        case DSRV_CHAT_TYPE::NORMAL:
            s = "NORMAL";
            break;
        case DSRV_CHAT_TYPE::SYSTEM:
            s = "SYSTEM";
            break;
        case DSRV_CHAT_TYPE::WHISPER:
            s = "WHISPER";
            break;
        default:
            s = "NONE";
            break;
    }
    return s;
}

FTSSrv2::Client::Client(Connection *in_pConnection)
    : m_bLoggedIn(false)
    , m_pMyGame(nullptr)
    , m_pMyChannel(nullptr)
    , m_pConnection(in_pConnection)
    , m_DataBase(FTSSrv2::DataBase::getUniqueDB())
{
}

FTSSrv2::Client::~Client()
{
    delete m_pConnection;
}

int FTSSrv2::Client::getID() 
{
    if( !m_bLoggedIn || m_sNick.empty() ) {
        FTSMSG("Want to get ID of not logged-in client", MsgType::Error);
        return -1;
    }

    return getIDByNick();
}

void FTSSrv2::Client::starter(Client *in_pThis)
{
    FTSSrv2::Client *pThis = in_pThis;
    pThis->run();
}

int FTSSrv2::Client::run()
{
    // Only wait for packets as long as there is a connection up.
    // If we lost the connection, most of the time it means that
    // the user logged out.
    while(m_pConnection->isConnected()) {
        Packet *pPack = m_pConnection->waitForThenGetPacket(false);

        if(!pPack) {
            std::this_thread::sleep_for( std::chrono::microseconds( 100 ) );
            continue;
        }

        // False returned means the connection shall be closed.
        auto rc = this->workPacket( pPack );
        delete pPack;
        if(!rc) {
            m_pConnection->disconnect();
            break;
        }
    }

    // Disconnect the player.
    this->quit();
    Server::getSingleton().addStats( m_pConnection->getPacketStats() );
    delete this;
    return 0;
}

// This closes the connection, logs the user out, leaves the channel if needed.
int FTSSrv2::Client::quit()
{
    int iRet = ERR_OK;

    // Logout only if needed.
    if(m_bLoggedIn) {
        // Leave the current game.
        if(m_pMyGame) {
            m_pMyGame->playerLeft(this->getNick());
            m_pMyGame = nullptr;
        }

        // Kill all games hosted by me.
        Game *pGameHostedByMe = GameManager::getManager()->findGameByHost(m_sNick);
        if(pGameHostedByMe) {
            GameManager::getManager()->remGame(pGameHostedByMe);
        }

        // Leave the current chat room.
        if(m_pMyChannel) {
            m_pMyChannel->quit(this);
            m_pMyChannel = nullptr;
        }

        // Call the stored procedure to logout.
        iRet = m_DataBase->storedFunctionInt(string("disconnect"),
                                            "\'" + m_DataBase->escape(m_sNick) + "\', "
                                            "\'" + m_pConnection->getCounterpartIP() + "\',"
                                            "\'" + m_DataBase->escape(m_sPassMD5) + "\',"
                                            "\'" + toString(DSRV_PROC_CONNECT_INGAME) + "\'");

        if(iRet != ERR_OK) {
            FTSMSG("failed (sql logout script returned "+toString(iRet)+")", MsgType::Error);
        }
    }

    // destroy the "session data".
    m_sNick = "";
    m_sPassMD5 = "";
    m_bLoggedIn = false;

    FTSSrv2::ClientsManager::getManager()->unregisterClient(this);

    return iRet;
}

int FTSSrv2::Client::tellToQuit()
{
    // Closing the connection makes the "quitting"-stone rolling.
    m_pConnection->disconnect();
    return ERR_OK;
}

int FTSSrv2::Client::getIDByNick()
{
    string sQuery;
    MYSQL_RES *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;

    // Query the id of the user.
    sQuery = "SELECT `"+m_DataBase->TblUsrField(DSRV_TBL_USR_ID)+"`"
             " FROM `" DSRV_TBL_USR "`"
             " WHERE `"+m_DataBase->TblUsrField(DSRV_TBL_USR_NICK)+"`"
                "=\'" + m_DataBase->escape(m_sNick) + "\'";
    if(!m_DataBase->query(pRes, sQuery)) {
        m_DataBase->free(pRes);
        FTSMSG("Error during SQL-query in getIDByNick for \"" + m_sNick + "\"", MsgType::Error);
        return -2;
    }

    // Get the query result.
    if(pRes == nullptr || nullptr == (pRow = mysql_fetch_row(pRes))) {
        FTSMSG("Error during sql fetch row in getID: "+ m_DataBase->getError(), MsgType::Error);
        m_DataBase->free(pRes);
        return -3;
    }

    int iRet = pRow[0] == nullptr ? -4 : atoi(pRow[0]);
    m_DataBase->free(pRes);

    return iRet;
}

// Returning true keeps the connection up. False would close the connection.
bool FTSSrv2::Client::workPacket(Packet *in_pPacket)
{
    FTSMSGDBG("\n\ngot message 0x"+ toString(in_pPacket->getType(),-1,'0',std::ios::hex), 5);

    // Preliminary checks.
    master_request_t req = in_pPacket->getType();
    if(
        req != DSRV_MSG_NULL &&
        req != DSRV_MSG_LOGIN &&
        req != DSRV_MSG_SIGNUP &&
        req < DSRV_MSG_MAX
      ) {
        // For all messages else then the above, we may already
        // check if the player is logged in and also get the MD5
        // password hash and check it to be correct.
        int8_t iRet = ERR_OK;
        string sMD5 = in_pPacket->get_string();

        // Check if the user is logged in or not.
        if(!m_bLoggedIn) {
            FTSMSG("failed: not logged in !", MsgType::Error);
            iRet = 10;
        } else {
            // A try to hack ?
            if(m_sPassMD5 != sMD5) {
                FTSMSG("failed: wrong md5 csum ("+sMD5+", correct would be "+m_sPassMD5+")", MsgType::Error);
                iRet = 11;
            }
        }

        // When there was an error, send back the error message and quit.
        if(iRet != ERR_OK) {
            Packet Pack(req);
            Pack.append(iRet);
            sendPacket(&Pack);
            return true;
        }
    }

    // Here begins the real message working.
    switch(in_pPacket->getType()) {
    case DSRV_MSG_NULL:
        //             this->onNull( );
        break;

    case DSRV_MSG_LOGIN:
        {
            string sNick = in_pPacket->get_string();
            string sMD5 = in_pPacket->get_string();

            return this->onLogin(trim(sNick), sMD5);
        }
    case DSRV_MSG_LOGOUT:
        {
            return this->onLogout();
        }
    case DSRV_MSG_SIGNUP:
        {
            string sNick = in_pPacket->get_string();
            string sMD5 = in_pPacket->get_string();
            string sEMail = in_pPacket->get_string();
            
            return this->onSignup(trim(sNick), sMD5, trim(sEMail));
        }
    case DSRV_MSG_FEEDBACK:
        return this->onFeedback(trim(in_pPacket->get_string()));
    case DSRV_MSG_PLAYER_SET:
        {
            uint8_t cField;
            in_pPacket->get(cField);

            return this->onPlayerSet(cField, in_pPacket);
        }
    case DSRV_MSG_PLAYER_GET:
        {
            uint8_t cField;
            in_pPacket->get(cField);
            string sNick = in_pPacket->get_string();

            return this->onPlayerGet(cField, trim(sNick), in_pPacket);
        }
    case DSRV_MSG_PLAYER_SET_FLAG:
        {
            uint32_t cFlag;
            in_pPacket->get(cFlag);
            uint8_t bValue;
            in_pPacket->get(bValue);

            return this->onPlayerSetFlag(cFlag, bValue != 0);
        }
    case DSRV_MSG_GAME_INS:
        return this->onGameIns(in_pPacket);
    case DSRV_MSG_GAME_REM:
        return this->onGameRem();
    case DSRV_MSG_GAME_LST:
        return this->onGameLst();
    case DSRV_MSG_GAME_INFO:
        return this->onGameInfo(trim(in_pPacket->get_string()));
    case DSRV_MSG_GAME_START:
        return this->onGameStart();
    case DSRV_MSG_CHAT_SENDMSG:
        return this->onChatSend(in_pPacket);
    case DSRV_MSG_CHAT_IUNAI:
        return this->onChatIuNai();
    case DSRV_MSG_CHAT_JOIN:
        return this->onChatJoin(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_LIST:
        return this->onChatList();
    case DSRV_MSG_CHAT_MOTTO_GET:
        return this->onChatMottoGet();
    case DSRV_MSG_CHAT_MOTTO_SET:
        return this->onChatMottoSet(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_USER_GET:
        return this->onChatUserGet(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_PUBLICS:
        return this->onChatPublics();
    case DSRV_MSG_CHAT_KICK:
        return this->onChatKick(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_OP:
        return this->onChatOp(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_DEOP:
        return this->onChatDeop(trim(in_pPacket->get_string()));
    case DSRV_MSG_CHAT_LIST_MY_CHANS:
        return this->onChatListMyChans();
    case DSRV_MSG_CHAT_DESTROY_CHAN:
        return this->onChatDestroyChan(trim(in_pPacket->get_string()));
    default:
        FTSMSG("[WARNING] message with ID '0x"+toString(in_pPacket->getType(),-1,'0',std::ios::hex)+"' not yet supported, ignoring.", MsgType::Error);
        break;
    }

    return false;
}

bool FTSSrv2::Client::sendPacket(Packet *in_pPacket)
{
    Lock l( m_mutex );
    FTSMSGDBG("\n\nsent message 0x"+toString(in_pPacket->getType(),-1,'0',std::ios::hex), 5);
    return m_pConnection->send(in_pPacket) == FTSC_ERR::OK;
}

int FTSSrv2::Client::sendChatJoins( const string & in_sPlayer )
{
    Packet p(DSRV_MSG_CHAT_JOINS);

    p.append(in_sPlayer);
    sendPacket(&p);

    return ERR_OK;
}

int FTSSrv2::Client::sendChatQuits( const string & in_sPlayer )
{
    Packet p(DSRV_MSG_CHAT_QUITS);

    p.append(in_sPlayer);
    sendPacket(&p);

    return ERR_OK;
}

int FTSSrv2::Client::sendChatOped( const string & in_sPlayer )
{
    Packet p(DSRV_MSG_CHAT_OPED);

    p.append(in_sPlayer);
    sendPacket(&p);

    return ERR_OK;
}

int FTSSrv2::Client::sendChatDeOped( const string & in_sPlayer )
{
    Packet p(DSRV_MSG_CHAT_DEOPED);

    p.append(in_sPlayer);
    sendPacket(&p);

    return ERR_OK;
}

int FTSSrv2::Client::sendChatMottoChanged(const string & in_sFrom, const string & in_sMotto)
{
    Packet p(DSRV_MSG_CHAT_MOTTO_CHANGED);

    p.append(in_sFrom);
    p.append(in_sMotto);

    sendPacket(&p);

    return ERR_OK;
}

bool FTSSrv2::Client::onLogin(const string & in_sNick, const string & in_sMD5)
{
    FTSMSGDBG(in_sNick+" is logging in ... ", 4);

    // Call the stored procedure to login.
    string sIP = m_pConnection->getCounterpartIP();
    int iRet = m_DataBase->storedFunctionInt("connect",
                                        "\'" + m_DataBase->escape(in_sNick) + "\', " +
                                        "\'" + m_DataBase->escape(in_sMD5)  + "\', " +
                                        "\'" + m_DataBase->escape(sIP) + "\'," +
                                        "\'" + toString(DSRV_PROC_CONNECT_INGAME) + "\'");

    if( iRet == ERR_OK ) {
        // save some "session data".
        m_sNick = in_sNick;
        m_sPassMD5 = in_sMD5;
        m_bLoggedIn = true;
        FTSSrv2::ClientsManager::getManager()->registerClient( this );

        FTSMSGDBG( "success", 4 );
    } else {

        string errText = "Unknown";
        switch( iRet ) {
            case 1:
                errText = "User doesn't exist.";
                break;
            case 2:
                errText = "Incorrect password.";
                break;
            case 4:
                errText = "Wrong password for more then five times.";
                break;
            case 9:
                errText = "Already logged into the game.";
                break;
        }
        FTSMSG("failed: sql login script returned ("+toString(iRet)+") "+errText, MsgType::Error);
    }

    Packet p( DSRV_MSG_LOGIN );
    p.append((int8_t)iRet);

    // And then send the result back to the client.
    sendPacket(&p);

    // If there is an error during the login, close the connection.
    if(iRet != ERR_OK)
        return false;

    return true;
}

bool FTSSrv2::Client::onLogout()
{
    FTSMSGDBG(m_sNick+" is logging out ... ", 4);

    int iRet = this->quit();
    FTSMSGDBG( "{1}", 4, ERR_OK == iRet ? "success": "failed" );


    Packet p( DSRV_MSG_LOGOUT );
    p.append( ( int8_t ) iRet );

    // And then send the result back to the client.
    sendPacket(&p);
    return true;
}

bool FTSSrv2::Client::onSignup(const string & in_sNick, const string & in_sMD5, const string & in_sEMail)
{
    int iRet = ERR_OK;

    FTSMSGDBG("signing up "+in_sNick+" ("+in_sEMail+") ... ", 4);

    // We are logged in but wanna signup ?
    if(m_bLoggedIn) {
        FTSMSG("failed: already logged in !", MsgType::Error);
        iRet = 10;
    } else {
        string sArgs = "\'" + m_DataBase->escape( in_sNick )  + "\'," +
                       "\'" + m_DataBase->escape( in_sMD5 )   + "\'," +
                       "\'" + m_DataBase->escape( in_sEMail ) + "\'";

        // Call the stored procedure to signup.
        iRet = m_DataBase->storedFunctionInt( "signup", sArgs );
        if( iRet != ERR_OK ) {
            FTSMSG( "failed: sql signup script returned " + toString( iRet ), MsgType::Error );
        } else {
            FTSMSGDBG( "success", 4 );
        }
    }

    // And then send the result back to the client.
    Packet p( DSRV_MSG_SIGNUP );
    p.append( (int8_t) iRet );
    sendPacket(&p);

    return (iRet == ERR_OK) ? true : false;
}

bool FTSSrv2::Client::onFeedback(const string &in_sMessage)
{
    int8_t iRet = ERR_OK;
    string sQuery = "INSERT INTO `" DSRV_TBL_FEEDBACK "` ("
                        " `"+m_DataBase->TblFeedbackField(DSRV_TBL_FEEDBACK_NICK)+"`,"
                        " `"+m_DataBase->TblFeedbackField(DSRV_TBL_FEEDBACK_MSG)+"`,"
                        " `"+m_DataBase->TblFeedbackField(DSRV_TBL_FEEDBACK_WHEN)+"`) "
                     "VALUES ("
                        " '"+m_DataBase->escape(this->getNick())+"',"
                        " '"+m_DataBase->escape(in_sMessage)+"',"
                        " NOW())";

    MYSQL_RES *pRes;
    if(!m_DataBase->query(pRes, sQuery)) {
        iRet = -1;
    }

    m_DataBase->free(pRes);

    // Answer.
    Packet p(DSRV_MSG_FEEDBACK);
    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onPlayerSet(uint8_t in_cField, Packet *out_pPacket)
{
    Packet p(DSRV_MSG_PLAYER_SET);
    string sValue;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is setting 0x"+toString(in_cField,-1,'0',std::ios::hex)+" ... ", 4);

    // Format the value used in the MySQL query string.
    switch(in_cField) {
    case DSRV_TBL_USR_PASS_MD5: // 32
    case DSRV_TBL_USR_MAIL:     // 64
    case DSRV_TBL_USR_JABBER:   // 64
    case DSRV_TBL_USR_CONTACT:  // 255
    case DSRV_TBL_USR_FNAME:    // 32
    case DSRV_TBL_USR_NAME:     // 32
    case DSRV_TBL_USR_BDAY:     // 10
    case DSRV_TBL_USR_CMT:      // 255
        sValue = m_DataBase->escape(out_pPacket->get_string());
        break;

    case DSRV_TBL_USR_SEX:       // 1 Char
        sValue = toString(out_pPacket->get());
        break;

    case DSRV_TBL_USR_CLAN:      // 1 Int
        {
            uint32_t iValue = 0;
            out_pPacket->get(iValue);
            sValue=toString(iValue);
        }
        break;

        // Something wrong.
    default:
        FTSMSG("failed: unknown field definer (0x"+toString(in_cField,-1,'0',std::ios::hex)+")", MsgType::Error);
        iRet = 1;
        p.append( iRet );
        sendPacket( &p );
        return false;
    }

    // Create the query string that modifies this field.
    string sField = DataBase::TblUsrField(in_cField);
    if(sField.empty()) {
        FTSMSG("failed: unknown field definer (0x"+toString(in_cField,-1,'0',std::ios::hex)+")", MsgType::Error);
        iRet = 2;
    } else {
        string sQuery = "UPDATE `" DSRV_TBL_USR "`"
            " SET `" + sField + "` = \'" + sValue + "\'"
            " WHERE `" + m_DataBase->TblUsrField( DSRV_TBL_USR_NICK ) + "`"
            " = \'" + m_DataBase->escape( m_sNick ) + "\'"
            " AND `" + m_DataBase->TblUsrField( DSRV_TBL_USR_PASS_MD5 ) + "`"
            " = \'" + m_DataBase->escape( m_sPassMD5 ) + "\' "
            " LIMIT 1";

        MYSQL_RES *pRes;
        if( !m_DataBase->query( pRes, sQuery ) ) {
            iRet = 3;
            FTSMSG( "failed: error during the sql update.", MsgType::Error );
            m_DataBase->free( pRes );
        } else {
            FTSMSGDBG( "success", 4 );
        }
        m_DataBase->free( pRes );
    }
    
    // And then send the result back to the client.
    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onPlayerGet(uint8_t in_cField, const string & in_sNick, Packet *out_pPacket)
{
    Packet p(DSRV_MSG_PLAYER_GET);
    MYSQL_RES *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is getting 0x"+toString(in_cField,-1,'0',std::ios::hex)+" from "+in_sNick+" ... ", 4);

    // Do the query to get the field.
    string sArgs = "\'"+m_DataBase->escape(this->getNick())+"\', "+
                   +"\'"+m_DataBase->escape(in_sNick)+"\', "+
                    toString<int>(in_cField);
    if(!m_DataBase->storedProcedure(pRes, "getUserPropertyNo", sArgs)) {
        m_DataBase->free(pRes);
        iRet = 1;
        sendPacket( p.append( iRet ) );
        return false;
    }

    // Get the query result.
    if(pRes == nullptr || nullptr == (pRow = mysql_fetch_row(pRes))) {
        FTSMSG("failed: sql fetch row: "+m_DataBase->getError(), MsgType::Error);
        m_DataBase->free(pRes);
        iRet = 2;
        sendPacket( p.append( iRet ) );
        return false;
    }

    FTSMSGDBG("success", 4);

    p.append(iRet);
    p.append(in_cField);

    // And extract the data from the result and add it to the packet.
    string resultRow = pRow[0] == nullptr ? "" : pRow[0];
    switch (in_cField) {
    // 1 Int:
    case DSRV_TBL_USR_WEEKON:
    case DSRV_TBL_USR_TOTALON:
    case DSRV_TBL_USR_WINS:
    case DSRV_TBL_USR_LOOSES:
    case DSRV_TBL_USR_DRAWS:
    case DSRV_TBL_USR_CLAN:
    case DSRV_TBL_USR_FLAGS:
        p.append<uint32_t>(atoi(resultRow.c_str()));
        break;

    // 1 Char:
    case DSRV_TBL_USR_SEX:
        p.append<int8_t>(atoi( resultRow.c_str() ));
        break;

    case DSRV_TBL_USR_MAIL:
    case DSRV_TBL_USR_JABBER:
    case DSRV_TBL_USR_FNAME:
    case DSRV_TBL_USR_NAME:
    case DSRV_TBL_USR_IP:
    case DSRV_TBL_USR_CMT:
    case DSRV_TBL_USR_CONTACT:
    case DSRV_TBL_USR_LOCATION:
    // Dates are also simple strings.
    case DSRV_TBL_USR_SIGNUPD:
    case DSRV_TBL_USR_LASTON:
    case DSRV_TBL_USR_BDAY:
        p.append( resultRow );
        break;
    // In case of unsupported thing, just give back nothing.
    default:
        p.append<uint8_t>(0);
        break;
    }
    m_DataBase->free(pRes);

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onPlayerSetFlag(uint32_t in_cFlag, bool in_bValue)
{
    Packet p(DSRV_MSG_PLAYER_SET_FLAG);
    FTSMSGDBG(m_sNick+" is " + (in_bValue ? "setting" : "clearing") + " his flag 0x"+toString(in_cFlag,-1,'0',std::ios::hex)+" ... ", 4);

    // The stored procedure does everything for us.
    auto string_b = []( bool b ) -> string { return b ? string( "True" ) : std::string( "False" ); };
    string sArgs = "\'"+m_DataBase->escape(this->getNick())+"\', "+
                   +"\'"+toString<int>(in_cFlag)+"\', "+string_b(in_bValue);
    if(ERR_OK != m_DataBase->storedFunctionInt("setUserFlag", sArgs)) {
        p.append((uint8_t)1);
        FTSMSGDBG("failed", 4);
    } else {
        p.append((uint8_t)ERR_OK);
        FTSMSGDBG("success", 4);
    }

    sendPacket(&p);
    return true;
}

bool FTSSrv2::Client::onGameIns(Packet *out_pPacket)
{
    Packet p(DSRV_MSG_GAME_INS);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is making a game ... ", 4);

    // Already in a game ?
    if(m_pMyGame) {
        FTSMSG("failed: already in a game", MsgType::Error);
        iRet = 12;
        sendPacket( p.append( iRet ) );
        return false;
    }

    // Ok, let's get the game's name.
    auto sGameName = trim(out_pPacket->get_string());

    FTSMSGDBG("name: \""+sGameName+"\" ... ", 4);

    // Check if the game already exists.
    if(GameManager::getManager()->findGame(sGameName) != NULL) {
        FTSMSG("Game already exists.", MsgType::Error);
        iRet = 1;
        sendPacket( p.append( iRet ) );
        return false;
    }

    // Read the data about the game and create it.
    m_pMyGame = new Game(this->getNick(), this->getCounterpartIP(), sGameName, out_pPacket);
    GameManager::getManager()->addGame(m_pMyGame);

    FTSMSGDBG("success", 4);

    // And then send the result back to the client.
    sendPacket( p.append( iRet ) );

    return true;
}

bool FTSSrv2::Client::onGameRem()
{
    Packet p(DSRV_MSG_GAME_REM);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" has aborted/ended his game ... ", 4);

    // Do I even have a game?
    if(!m_pMyGame) {
        FTSMSG("failed: he doesn't have a game?", MsgType::Error);
        iRet = 12;
        sendPacket( p.append( iRet ) );
        return false;
    }

    GameManager::getManager()->remGame(m_pMyGame);

    // Now I no more have a game!
    m_pMyGame = nullptr;

    FTSMSGDBG("success", 4);

    // And then send the result back to the client.
    sendPacket( p.append( iRet ) );

    return true;
}

bool FTSSrv2::Client::onGameLst()
{
    Packet p(DSRV_MSG_GAME_LST);

    FTSMSGDBG(m_sNick+" gets a list of all games ... ", 4);

    p.append((int8_t)ERR_OK);
    p.append((int16_t)GameManager::getManager()->getNGames());
    GameManager::getManager()->writeListToPacket(&p);

    FTSMSGDBG("success", 4);

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onGameInfo(const string & in_sName)
{
    Packet p(DSRV_MSG_GAME_INFO);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is getting info about the game "+in_sName+" ... ", 4);

    auto pGame = GameManager::getManager()->findGame(in_sName);
    if(!pGame) {
        FTSMSGDBG("failed: game not found.", 4);
        iRet = 1;
        sendPacket( p.append( iRet ) );
        return false;
    }

    p.append(iRet);
    pGame->addToInfoPacket(&p);

    FTSMSGDBG("success", 4);

    // And then send the result back to the client.
    sendPacket( &p );

    return true;
}

bool FTSSrv2::Client::onGameStart()
{
    Packet p(DSRV_MSG_GAME_START);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is starting his game ... ", 4);

    // Do I even have a game?
    if(!m_pMyGame) {
        FTSMSG("failed: he doesn't have a game?", MsgType::Error);
        iRet = 12;
    } else {
        GameManager::getManager()->startGame( m_pMyGame );
        FTSMSGDBG( "success", 4 );
    }

    // And then send the result back to the client.
    sendPacket( p.append( iRet ) );

    return true;
}

bool FTSSrv2::Client::onChatJoin(const string & in_sChan)
{
    Packet p(DSRV_MSG_CHAT_JOIN);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is joining chan "+in_sChan+" ... ", 4);

    auto pChannel = ChannelManager::getManager()->findChannel(in_sChan);

    // If the channel doesn't exist, we gotta create it first.
    if(nullptr == pChannel) {
        FTSMSGDBG("creating the channel ... ", 4);
        pChannel = ChannelManager::getManager()->createChannel( in_sChan, this );
        if(pChannel == nullptr) {
            FTSMSGDBG("error!", 4);
            iRet = -100;
            sendPacket( p.append( iRet ) );
            return false;
        }
    }

    iRet = ChannelManager::getManager()->joinChannel( pChannel, this );
    if( iRet == ERR_OK ) {
        FTSMSGDBG("success", 4);

        // Update the location field in the database.
        string sQuery = "UPDATE `" DSRV_TBL_USR "`"
                         " SET `"+m_DataBase->TblUsrField(DSRV_TBL_USR_LOCATION)+"`"
                            "='chan:"+m_DataBase->escape(in_sChan)+"'"
                         " WHERE `"+m_DataBase->TblUsrField(DSRV_TBL_USR_NICK)+"`"
                            "='"+m_DataBase->escape(this->getNick())+"'"
                         " LIMIT 1";
        MYSQL_RES *pRes;
        m_DataBase->query(pRes, sQuery);
        m_DataBase->free(pRes);
    }

    // And then send the result back to the client.
    sendPacket(p.append( iRet ) );

    return true;
}

bool FTSSrv2::Client::onChatList()
{
    Packet p(DSRV_MSG_CHAT_LIST);
    int8_t iRet = ERR_OK;
    int32_t nPlayers = 0;

    FTSMSGDBG(m_sNick+" is listing his channel ... ", 4);

    // If we are in no channel, just return an empty list.
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = -3;
    } else {
        // Get the number of users and the list of users.
        nPlayers = m_pMyChannel->getNUsers();
        FTSMSGDBG( "success", 4 );
    }

    auto lpUsers = m_pMyChannel->getUsers();

    // Append the number of users.
    p.append(iRet);
    p.append(nPlayers);

    // And append the users.
    if( iRet == ERR_OK ) {
        for( auto i : lpUsers ) {
            p.append( i->getNick() );
        }
    }

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatSend(Packet *out_pPacket)
{
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is saying something ... ", 4);

    DSRV_CHAT_TYPE cType = DSRV_CHAT_TYPE::NONE;
    out_pPacket->get( cType );
    uint8_t cFlags = out_pPacket->get();
    auto genAnsw = [this]( int8_t rc ) { Packet p( DSRV_MSG_CHAT_SENDMSG ); this->sendPacket( p.append( rc ) ); };

    switch( cType ) {
        case DSRV_CHAT_TYPE::NORMAL:
            {
                // Are we currently in a channel ?
                if( m_pMyChannel == nullptr ) {
                    FTSMSGDBG( "Hmm, in no channel ?", 4 );
                    iRet = -1;
                    genAnsw( iRet );
                    return false;
                }
                auto sMsg = out_pPacket->get_string();
                if( sMsg.empty() ) {
                    FTSMSGDBG( "Hmm, no or corrupted text !", 4 );
                    iRet = -2;
                    genAnsw( iRet );
                    return false;
                }

                m_pMyChannel->messageToAll( *this, sMsg, cFlags );
            }
            break;
        case DSRV_CHAT_TYPE::WHISPER:
            {
                string sTo = trim( out_pPacket->get_string() );
                FTSMSGDBG( "Target is: " + sTo + ".", 4 );

                auto pTo = FTSSrv2::ClientsManager::getManager()->findClient( sTo );
                if( pTo == nullptr ) {
                    // User not online, not existent : send a error.
                    iRet = 1;
                    FTSMSGDBG( "Target not existing or online.", 4 );
                    genAnsw( iRet );
                    return false;
                } else if( pTo == this ) {
                    // User is myself: send a error.
                    iRet = 2;
                    FTSMSGDBG( "Target is myself.", 4 );
                    genAnsw( iRet );
                    return false;
                } else {
                    // Send the message to the user.
                    string sMessage = out_pPacket->get_string();
                    if( sMessage.empty() ) {
                        FTSMSGDBG( "Hmm, no or corrupted text !", 4 );
                        iRet = -2;
                        genAnsw( iRet );
                        return false;
                    }
                    Packet Ralf( DSRV_MSG_CHAT_GETMSG );
                    Ralf.append( DSRV_CHAT_TYPE::WHISPER );
                    Ralf.append( (uint8_t) 0 );
                    Ralf.append( this->getNick() );
                    Ralf.append( sMessage );

                    FTSMSGDBG( "He whisps " + sTo + ": " + sMessage, 4 );

                    if( pTo->sendPacket( &Ralf ) != ERR_OK ) {
                        iRet = 3;
                        genAnsw( iRet );
                        return false;
                    }
                }
            }
            break;
        default:
            FTSMSG( "failed: unknown type (" + DSRV_CHAT_TYPE_toString( cType ) + ") !", MsgType::Error );
            iRet = -12;
            break;
    }

    genAnsw( iRet );
    return true;
}

bool FTSSrv2::Client::onChatIuNai()
{
    Packet p(DSRV_MSG_CHAT_IUNAI);
    string sChannel;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is asking where he is ... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("He is nowhere", 4);
    } else {
        sChannel = m_pMyChannel->getName();
        FTSMSGDBG("success, he is in " + sChannel, 4);
    }
    p.append(iRet);
    p.append(sChannel);

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatMottoGet()
{
    Packet p(DSRV_MSG_CHAT_MOTTO_GET);
    string sMotto;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is getting his channel motto ... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == NULL ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = 1;
    } else {
        // Get it.
        sMotto = m_pMyChannel->getMotto();
        FTSMSGDBG( "success", 4 );
    }

    p.append(iRet);
    p.append(sMotto);

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatMottoSet(const string & in_sMotto)
{
    Packet p(DSRV_MSG_CHAT_MOTTO_SET);
    string sMotto;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is setting his channel motto to "+in_sMotto+"... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = 1;
    } else {
        // Set it.
        iRet = m_pMyChannel->setMotto( in_sMotto, m_sNick );

        if( iRet == ERR_OK )
            FTSMSGDBG( "success", 4 );
        else {
            FTSMSGDBG( "error (no rights ?)", 4 );
        }
    }

    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatUserGet(const string & in_sUser)
{
    Packet p(DSRV_MSG_CHAT_USER_GET);
    DSRV_CHAT_USER cState = DSRV_CHAT_USER::UNKNOWN;
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" is getting the state of the user "+in_sUser+"... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = 1;
        sendPacket( p.append(iRet) );
        return false;
    } 

    // Set it.
    if( ieq(m_pMyChannel->getAdmin(),in_sUser) ) {
        cState = DSRV_CHAT_USER::ADMIN;
    } else if( m_pMyChannel->isop(in_sUser) ) {
        cState = DSRV_CHAT_USER::OPERATOR;
    } else {
        cState = DSRV_CHAT_USER::NORMAL;
    }

    FTSMSGDBG("success ("+ DSRV_CHAT_USER_toString(cState)+")", 4);

    p.append(iRet);
    p.append( cState );

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatPublics()
{
    FTSMSGDBG(m_sNick+" is listing public channels ... ", 4);

    // Get the number and the list of public channels.
    auto lpChannels = ChannelManager::getManager()->getPublicChannels();
    uint32_t nChans = (uint32_t)lpChannels.size();

    FTSMSGDBG("success (there are "+toString(nChans)+" public channels)", 4);

    // Append the number of public channels.
    Packet p( DSRV_MSG_CHAT_PUBLICS );
    p.append((int8_t)ERR_OK);
    p.append(nChans);

    // And append the public channels.
    for( auto i : lpChannels ) {
        p.append(i->getName());
    }

    // And then send the result back to the client.
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatKick(const string & in_sUser)
{
    Packet p(DSRV_MSG_CHAT_KICK);
    int8_t iRet = ERR_OK;
    FTSMSGDBG(m_sNick+" wants to kick "+in_sUser+" ... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == NULL ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = -1;
    } else {
        iRet = m_pMyChannel->kick( this, in_sUser );
        if( iRet == ERR_OK ) {
            FTSMSGDBG( "success", 4 );
        } else {
            FTSMSGDBG( "failed (" + toString( iRet ) + ")", 4 );
        }
    }

    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatOp(const string & in_sUser)
{
    Packet p(DSRV_MSG_CHAT_OP);
    int8_t iRet;
    FTSMSGDBG(m_sNick+" wants to op "+in_sUser+" ... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = -1;
        sendPacket( p.append(iRet) );
        return false;
    }

    iRet = -2;

    if(m_pMyChannel->getAdmin() == m_sNick) {
        iRet = m_pMyChannel->op(in_sUser);
    }

    if(iRet == ERR_OK) {
        FTSMSGDBG("success", 4);
    } else {
        FTSMSGDBG("failed ("+toString(iRet)+")", 4);
    }

    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatDeop(const string & in_sUser)
{
    Packet p(DSRV_MSG_CHAT_DEOP);
    int8_t iRet;
    FTSMSGDBG(m_sNick+" wants to deop "+in_sUser+" ... ", 4);

    // Are we currently in a channel ?
    if( m_pMyChannel == nullptr ) {
        FTSMSGDBG("Hmm, in no channel ?", 4);
        iRet = -1;
        sendPacket( p.append( iRet ) );
        return false;
    }

    iRet = -2;

    if(m_pMyChannel->getAdmin() == m_sNick) {
        iRet = m_pMyChannel->deop(in_sUser);
    }

    if(iRet == ERR_OK) {
        FTSMSGDBG("success", 4);
    } else {
        FTSMSGDBG("failed ("+toString(iRet)+")", 4);
    }

    p.append(iRet);
    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatListMyChans()
{
    Packet p(DSRV_MSG_CHAT_LIST_MY_CHANS);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" wants a list of his chans ... ", 4);

    std::list<string> sChans = ChannelManager::getManager()->getUserChannels(this->getNick());

    p.append(iRet);
    p.append((uint32_t)sChans.size());
    for( auto i : sChans ) {
        p.append(i);
    }

    FTSMSGDBG("got that list, there are "+toString(sChans.size())+" of them", 4);

    sendPacket(&p);

    return true;
}

bool FTSSrv2::Client::onChatDestroyChan(const string &in_sChan)
{
    Packet p(DSRV_MSG_CHAT_DESTROY_CHAN);
    int8_t iRet = ERR_OK;

    FTSMSGDBG(m_sNick+" wants to destroy the channel "+in_sChan+" ... ", 4);

    Channel *pChan = ChannelManager::getManager()->findChannel(in_sChan);
    if(!pChan) {
        iRet = -1;
        FTSMSGDBG("failed: does not exist!", 4);
    } else {
        iRet = ChannelManager::getManager()->removeChannel(pChan, this->getNick());
        FTSMSGDBG( iRet == ERR_OK ? "success" : "failed", 4);
    }

    p.append(iRet);
    sendPacket(&p);

    return true;
}
