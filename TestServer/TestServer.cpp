// TestServer.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <string>
#include <vector>

#include "stdafx.h"
#include "net/packet.h"
#include "net/connection.h"
#include "utilities/threading.h"
#include "utilities/utilities.h"
#include "toolcompat.h"
#include "logging/Chronometer.h"

#if WINDOOF
#pragma comment(lib, "Ws2_32.lib")
#endif

using namespace FTS;
bool g_exit = false;
char * ServerName = "localhost";

struct client_functions
{
    void connectToServer();
    void login(String user, String pwd);
    void logout();
    void joinChat( String ChatRoom );
    void getChatList();
    void chatMessage( String message, String toUser = String::EMPTY );
    int emptyReceiverQueue();

    String user;
    String pwd;
    String chatRoom;
    FTS::Connection* connection;
    std::vector<int> nMreqSendErr;
    std::vector<int> nMsgGetErr;
    std::vector<String> nickNames;
};

void client_functions::login( String user, String pwd )
{
    this->user = user;
    this->pwd = pwd;
    // Encrypt the password.
    //char buffMD5[32];

    //md5Encode( in_sPassword.c_str(), in_sPassword.len(), buffMD5 );

    // Cut all the data to the right size and store the MD5 hash.
    //String sNick( in_sNickname );
    //m_sMD5 = String( buffMD5, 0, 32 );
    Packet *p = new Packet( DSRV_MSG_LOGIN );
    p->append( user );
    p->append( pwd );

    // And login.
    auto rc = (int)connection->mreq( p );
    if( rc != ERR_OK ) {
        nMreqSendErr.push_back( rc );
    } else {
        rc = p->get();
        if( rc != ERR_OK ) {
            nMsgGetErr.push_back( rc );
        }
    }
    delete p;
}

void client_functions::joinChat( String ChatRoom )
{
    chatRoom = ChatRoom;
    auto p = new Packet( DSRV_MSG_CHAT_JOIN );
    p->append( pwd );
    p->append( chatRoom );

    auto rc = (int) connection->mreq( p );
    if( rc != ERR_OK ) {
        nMreqSendErr.push_back( rc );
    } else {
        rc = p->get();
        if( rc != ERR_OK ) {
            nMsgGetErr.push_back( rc );
        }
    }
    delete p;
}

void client_functions::getChatList()
{
    auto p = new Packet( DSRV_MSG_CHAT_LIST );
    p->append( pwd );

    auto rc = (int) connection->mreq( p );
    if( rc != ERR_OK ) {
        nMreqSendErr.push_back( rc );
    } else {
        rc = p->get();
        if( (rc != ERR_OK) && (rc != -3) /*no list available*/ ) {
            nMsgGetErr.push_back( rc );
        } else {
            int nUsers = 0;
            p->get( nUsers );
            for( int32_t i = 0; i < nUsers; ++i ) {
                auto sNickName = p->get_string();
                if( sNickName != user ) {
                    nickNames.push_back( sNickName );
                }
            }
        }
    }
    delete p;

}

void client_functions::chatMessage(String message, String toUser /*= String::EMPTY*/)
{
    auto p = new Packet( DSRV_MSG_CHAT_SENDMSG );
    uint8_t type = toUser == String::EMPTY ? 1 /*DSRV_CHAT_TYPE_NORMAL*/ : 2/*DSRV_CHAT_TYPE_WHISPERS*/;
    p->append( pwd );
    p->append( type );
    p->append( (uint8_t)0 /* flags */ );
    if ( type == 2 )
    {
        p->append( toUser );
    }
    p->append( message );

    auto rc = (int) connection->mreq( p );
    if( rc != ERR_OK ) {
        nMreqSendErr.push_back( rc );
    } else {
        rc = p->get();
        if( (rc != ERR_OK) ) {
            nMsgGetErr.push_back( rc );
        }
    }
    delete p;
}

void client_functions::logout()
{
    auto p = new Packet( DSRV_MSG_LOGOUT );
    p->append( pwd );

    auto rc = (int) connection->mreq( p );
    if( rc != ERR_OK ) {
        nMreqSendErr.push_back( rc );
    } else {
        rc = p->get();
        if( rc != ERR_OK ) {
            nMsgGetErr.push_back( rc );
        }
    }
}

int client_functions::emptyReceiverQueue()
{
    int nQueueLeft = 0;
    while( true ) {
        auto p = connection->getReceivedPacketIfAny();
        if( p != nullptr ) {
            nQueueLeft++;
            delete p;
        } else {
            break;
        }
    }
    return nQueueLeft;
}

void workerThread( void* port )
{
    int nMsgCreated = 0;
    int nConClosed = 0;
    int nChatMsgs = 0;
    int nQueueLeft = 0;
    int nReceived = 0;
    std::vector<double> times;
    client_functions client;

    while( !g_exit )
    {
        client.connection = new FTS::TraditionalConnection( ServerName, (int)port, 1000 );
        if( !client.connection->isConnected() )
        {
            FTSMSGDBG( "[{1}] Connection to {2} failed\nQuit!\n", 1, String::nr( ( int ) port ), ServerName );
        }

        String user = "Test" + String::nr( ( int ) port );
        String pwd = "Test" + String::nr( ( int ) port );
        nMsgCreated++;
        client.login( user, pwd );

        // Join a chat room. 
        client.joinChat( "UselessChan" );
        
        while( !g_exit ) {
            Chronometer timer;
            nChatMsgs++;

            // Get list of chat #players and the user names. 
            client.getChatList();
            times.push_back( timer.measure() );

            // Chat a message 
            client.chatMessage( user + " now here!" );
            times.push_back( timer.measure() );
            
            nReceived += client.emptyReceiverQueue();
        }
        
        // And logout.
        client.logout();

        nQueueLeft = client.emptyReceiverQueue();

        nConClosed++;
        client.connection->disconnect();
    }

    String sGetErr;
    sGetErr = "{";
    for( auto no : client.nMsgGetErr ) {
        sGetErr += String::nr( no ) + ",";
    }
    sGetErr += "}";

    String sSendErr;
    sSendErr = "{";
    for( auto no : client.nMreqSendErr ) {
        sSendErr += String::nr( no ) + ",";
    }
    sSendErr += "}";

    double avg = 0;
    if( times.size() > 0 ) {
        for( auto t : times ) {
            avg += t;
        }
        avg = avg / times.size() * 1000.;
    }

    FTSMSGDBG( "[{1}] Queue {2}/{9} Chat {3} SendErr {4} {5} GetErr {6} {7} {8} ms", 1 
               ,String::nr( ( int ) port )
               ,String::nr( nQueueLeft )
               ,String::nr( nChatMsgs )
               ,String::nr( client.nMreqSendErr.size() )
               ,sSendErr
               ,String::nr(client.nMsgGetErr.size())
               ,sGetErr
               , String::nr(avg)
               , String::nr( nReceived)
               );

}

void workerThreadChat( void* port )
{
    int nMsgCreated = 0;
    int nConClosed = 0;
    int nChatMsgs = 0;
    int nQueueLeft = 0;
    int nReceived = 0;
    std::vector<double> times;

    client_functions client;

    while( !g_exit ) {
        client.connection = new FTS::TraditionalConnection( ServerName, ( int ) port, 1000 );
        if( !client.connection->isConnected() ) {
            FTSMSGDBG( "[{1}] Connection to {2} failed\nQuit!\n", 1, String::nr( ( int ) port ), ServerName );
        }

        String user = "Test" + String::nr( ( int ) port );
        String pwd = "Test" + String::nr( ( int ) port );
        nMsgCreated++;
        client.login( user, pwd );

        // Join a chat room.
        client.joinChat( "UselessChan" );

        // Wait for a while to have a chance some one is in the chat room.
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

        bool someOneOutThere = false;
        int retries = 0;
        while( !someOneOutThere && retries < 10)
        {
            // Get list of chat #players and the user names. 
            client.getChatList();
            someOneOutThere = client.nickNames.size() != 0;
            retries++;
        }
        int nOthers = client.nickNames.size();
        while( !g_exit && nOthers) {
            Chronometer timer;
            nChatMsgs++;

            // Chat a message 
            client.chatMessage( user + " now here!", client.nickNames[(nOthers+1) % nOthers] );
            times.push_back( timer.measure() );

            nReceived += client.emptyReceiverQueue();
        }

        // And logout.
        client.logout();

        nQueueLeft = client.emptyReceiverQueue();

        nConClosed++;
        client.connection->disconnect();
    }

    String sGetErr;
    sGetErr = "{";
    for( auto no : client.nMsgGetErr ) {
        sGetErr += String::nr( no ) + ",";
    }
    sGetErr += "}";

    String sSendErr;
    sSendErr = "{";
    for( auto no : client.nMreqSendErr ) {
        sSendErr += String::nr( no ) + ",";
    }
    sSendErr += "}";

    double avg = 0;
    if( times.size() > 0 ) {
        for( auto t : times ) {
            avg += t;
        }
        avg = avg / times.size() * 1000.;
    }

    FTSMSGDBG( "[{1}] Queue {2}/{9} Chat {3} SendErr {4} {5} GetErr {6} {7} {8} ms", 1
               , String::nr( ( int ) port )
               , String::nr( nQueueLeft )
               , String::nr( nChatMsgs )
               , String::nr( client.nMreqSendErr.size() )
               , sSendErr
               , String::nr( client.nMsgGetErr.size() )
               , sGetErr
               , String::nr( avg )
               , String::nr( nReceived )
               );

    String nameList = "{";
    for( auto name : client.nickNames )
    {
        nameList += name + ",";
    }
    nameList += "}";
    FTSMSGDBG( "[{1}] Users {2} List {3}", 1
               , String::nr( ( int ) port )
               , String::nr( client.nickNames.size() )
               , nameList
               );

}



int _tmain(int argc, _TCHAR* argv[])
{
#if WINDOOF
    //----------------------
    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( iResult != NO_ERROR )
    {
        std::cout << "WSAStartup failed with error:" << iResult << "\n";
        return 1;
    }
#endif
    // Logging
    // =======
    new FTSTools::MinimalLogger();
#define NTHREADS    2
    std::thread worker[NTHREADS];
    Chronometer timer;
    for( int i = 0; i < NTHREADS - 1; ++i )
    {
        worker[i] = std::thread( workerThread, ( void * ) (44917+i) );
    }
    worker[NTHREADS - 1] = std::thread( workerThreadChat, ( void * ) (44917 + NTHREADS - 1) );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    auto timeConsumed = timer.measure();
    
    g_exit = true;
    for ( int i = 0; i < NTHREADS; ++i )
    {
        worker[i].join();
    }

    std::cout << "Time used " << timeConsumed << std::endl;
    std::string s;
    std::getline( std::cin, s );

    return 0;
}

