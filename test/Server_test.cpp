
#include "catch.hpp"
#include "../Server.h"

using namespace FTS;

class TestServer : public FTSSrv2::Server
{
public:
   TestServer( std::string in_logDir, bool in_bVerbose, int in_dbgLevel ) : FTSSrv2::Server(in_logDir, in_bVerbose, in_dbgLevel) {};
   virtual ~TestServer() {};
};

class TestServerFixure
{
public:
    TestServerFixure() {}
    ~TestServerFixure()
    {
        delete TestServer::getSingletonPtr();
    }
};

TEST_CASE_METHOD(TestServerFixure, "Create Singleton class Server", "[Server]")
{
    new TestServer("",false,0);
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
}

TEST_CASE_METHOD( TestServerFixure, "Create Singleton class Server with log dir", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 0 );
}

TEST_CASE_METHOD( TestServerFixure, "Create with log dir and verbose on", "[Server]" )
{
    new TestServer( ".", true, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == true );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 0 );
}

TEST_CASE_METHOD( TestServerFixure, "Create with log dir, verbose off and DbgLevel ", "[Server]" )
{
    new TestServer( ".", false, 4 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 4 );
}

TEST_CASE_METHOD( TestServerFixure, "Create and check members", "[Server]" )
{
    new TestServer( ".", false, 4 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 4 );
    REQUIRE( TestServer::getSingleton().getPlayersfilename() == "./nplayers.txt" );
    REQUIRE( TestServer::getSingleton().getGamesfilename() == "./ngames.txt" );
}
