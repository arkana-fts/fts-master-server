/**
 * \file socket_connection_waiter.cpp
 * \author Pompei2
 * \date 03 Oct 2008
 * \brief This file implements the class that handles a lot of
 *        Network stuff at the server-side.
 **/

#ifdef D_COMPILES_SERVER
#include <thread>
#include <chrono>

#include <Logger.h>
#include "constants.h"
#include "client.h"
#include "socket_connection_waiter.h"

#if WINDOOF
using socklen_t = int;

inline void close( SOCKET s )
{
    closesocket( s );
}
#else
#  include <unistd.h> // close
#  include <string.h> // strerror
#endif

using namespace FTS;
using namespace FTSSrv2;
using namespace std;

FTSSrv2::SocketConnectionWaiter::SocketConnectionWaiter()
{
}

FTSSrv2::SocketConnectionWaiter::~SocketConnectionWaiter()
{
    close( m_listenSocket );
}

int FTSSrv2::SocketConnectionWaiter::init(std::uint16_t in_usPort)
{
    m_port = in_usPort;
    SOCKADDR_IN serverAddress;

    // Choose our options.
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons((int)in_usPort);

    // Setup the listening socket.
    if((m_listenSocket = socket(AF_INET, SOCK_STREAM, 0 /*IPPROTO_TCP */ )) < 0) {
        FTSMSG("[ERROR] socket: "+string(strerror(errno)), MsgType::Error);
        return -1;
    }

    if(::bind(m_listenSocket, (sockaddr *) & serverAddress, sizeof(serverAddress)) < 0) {
        FTSMSG("[ERROR] socket bind: "+string(strerror(errno)), MsgType::Error);
        close(m_listenSocket);
        return -2;
    }

    // Set it to be nonblocking, so we can easily time the wait for a connection.
    TraditionalConnection::setSocketBlocking(m_listenSocket, false);

    if(listen(m_listenSocket, 100) < 0) {
        FTSMSG("[ERROR] socket listen: "+string(strerror(errno)), MsgType::Error);
        close(m_listenSocket);
        return -3;
    }

    FTSMSGDBG("Beginning to listen on port 0x"+toString(in_usPort, 0, ' ', std::ios::hex), 1);
    return ERR_OK;
}

bool FTSSrv2::SocketConnectionWaiter::waitForThenDoConnection(std::int64_t in_ulMaxWaitMillisec)
{
    auto startTime = std::chrono::steady_clock::now();
    // wait for connections a certain amount of time or infinitely.
    while(true) {
        // Nothing correct got in time, bye.
        auto nowTime = std::chrono::steady_clock::now();
        auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime).count();
        if( diffTime >= in_ulMaxWaitMillisec )
            return false;

        SOCKADDR_IN clientAddress;
        socklen_t iClientAddressSize = sizeof( clientAddress );
        SOCKET connectSocket;
        if( (connectSocket = accept( m_listenSocket, ( sockaddr * ) & clientAddress, &iClientAddressSize )) != -1 ) {
            // Yeah, we got someone !

            // Build up a class that will work this connection.
            TraditionalConnection *pCon = new TraditionalConnection(connectSocket, clientAddress);
            Client *pCli = ClientsManager::getManager()->createClient(pCon);
            if( pCli == nullptr ) {
                FTSMSG( "[ERROR] Can't create client, may be it exists already. Port<" + toString( (int) m_port, 0, ' ', std::ios::hex ) + "> con<" + toString( (const uint64_t) pCon, 4, '0', std::ios_base::hex ) + ">", MsgType::Error );
                return false;
            }

            FTSMSGDBG( "Accept connection on port 0x" + toString( ( int ) m_port, 0, ' ', std::ios::hex ) + " client<"+ toString((const uint64_t)pCli,4, '0', std::ios_base::hex) + "> con<"+ toString((const uint64_t)pCon,4,'0', std::ios_base::hex)+ ">", 4 );

            // And start a new thread for him.
            auto thr = std::thread( Client::starter, pCli );
            thr.detach();
            return true;

        } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
            // yoyo, wait a bit to avoid megaload of cpu. 1000 microsec = 1 millisec.
            std::this_thread::sleep_for( std::chrono::milliseconds(1) );
            continue;
        } else {
#if WINDOOF
            if ( connectSocket == INVALID_SOCKET)
            {
                auto err = WSAGetLastError();
                if ( err == WSAEWOULDBLOCK )
                {
                    // yoyo, wait a bit to avoid megaload of cpu. 1000 microsec = 1 millisec.
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                    continue;
                }
                
                FTSMSG( "[ERROR] socket accept: " + toString( err ), MsgType::Error );
            }

#endif
            // Some error ... but continue waiting for a connection.
            FTSMSG("[ERROR] socket accept: "+string(strerror(errno)), MsgType::Error);
            // yoyo, wait a bit to avoid megaload of cpu. 1000 microsec = 1 millisec.
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            continue;
        }
    }

    // Should never come here.
    return false;
}

#endif
