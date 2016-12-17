
#include "catch.hpp"
#include <utility>
#include "../Server.h"
#include "../ClientsManager.h"
#include "../db.h"

using namespace FTS;
using namespace std;

//----------------------------------------------
// The following lines are to fake the real implementation, which are not needed for the tests.
namespace FTSSrv2 {
   DataBase::~DataBase() {}

   int DataBase::init() { return 0; }

   std::vector<std::tuple<int, bool, std::string, std::string, std::string>> DataBase::getChannels() { return std::vector<std::tuple<int, bool, std::string, std::string, std::string>>(); }
   std::vector<std::tuple<std::string, std::string>> DataBase::getChannelOperators() { return std::vector<std::tuple<std::string, std::string>>(); }
   bool DataBase::channelCreate(const std::tuple<bool, std::string, std::string, std::string>& record) { return false; }
   bool DataBase::channelUpdate(const std::tuple<int, bool, std::string, std::string, std::string>& record) { return false; }
   void DataBase::channelAddOp(const std::tuple<std::string, int>& record) {}
   int DataBase::channelDestroy(const std::tuple<std::string, std::string>& record) { return 0; }
   int DataBase::disconnect(const std::tuple<std::string, std::string, std::string, int>& record) { return 0; }
   int DataBase::connect(const std::tuple<std::string, std::string, std::string, int>& record) { return 0; }
   int DataBase::signup(const std::tuple<std::string, std::string, std::string>& record) { return 0; }
   int DataBase::insertFeedback(const std::tuple<std::string, std::string>& record) { return 0; }
   int DataBase::updatePlayerSet(const std::tuple<std::uint8_t, std::string, std::string, std::string>& record) { return 0; }
   std::tuple<int, std::string> DataBase::getUserPropertyNo(const std::tuple<std::uint8_t, std::string, std::string>& record) { return std::tuple<int, std::string>(); }
   int DataBase::setUserFlag(const std::tuple<std::string, std::uint32_t, bool>& record) { return 0; }
   int DataBase::updateLocation(const std::tuple<std::string, std::string>& record) { return 0; }
}

namespace FTSSrv2 {
   ClientsManager::~ClientsManager() {}
   void ClientsManager::add(Client *in_pClient) {}
   void ClientsManager::remove(Client *in_pClient) {}
   Client *ClientsManager::find(const std::string &in_sName) { return nullptr; }
}
//----------------------------------------------


class TestServer : public FTSSrv2::Server
{
public:
   TestServer( std::string in_logDir, bool in_bVerbose, int in_dbgLevel ) : FTSSrv2::Server(in_logDir, in_bVerbose, in_dbgLevel, nullptr ) {};
   virtual ~TestServer() {};
};

class TestServerFixture
{
public:
    TestServerFixture() {}
    ~TestServerFixture()
    {
        delete TestServer::getSingletonPtr();
    }
};

TEST_CASE_METHOD(TestServerFixture, "Create Singleton class Server", "[Server]")
{
    new TestServer("",false,0);
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
}

TEST_CASE_METHOD( TestServerFixture, "Create Singleton class Server with log dir", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 0 );
}

TEST_CASE_METHOD( TestServerFixture, "Create with log dir and verbose on", "[Server]" )
{
    new TestServer( ".", true, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == true );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 0 );
}

TEST_CASE_METHOD( TestServerFixture, "Create with log dir, verbose off and DbgLevel ", "[Server]" )
{
    new TestServer( ".", false, 4 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 4 );
}

TEST_CASE_METHOD( TestServerFixture, "Check members default initialization", "[Server]" )
{
    new TestServer( ".", false, 4 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    REQUIRE( TestServer::getSingleton().getLogfilename() == "./fts_server.log" );
    REQUIRE( TestServer::getSingleton().getVerbose() == false );
    REQUIRE( TestServer::getSingleton().getDbgLevel() == 4 );
    REQUIRE( TestServer::getSingleton().getPlayersfilename() == "./nplayers.txt" );
    REQUIRE( TestServer::getSingleton().getGamesfilename() == "./ngames.txt" );
    REQUIRE( TestServer::getSingleton().getGameCount() == 0 );
    REQUIRE( TestServer::getSingleton().getPlayerCount() == 0 );
    REQUIRE( TestServer::getSingleton().getNetLogfilename() == "./fts_server.netlog" );
}

TEST_CASE_METHOD( TestServerFixture, "Setting members", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    auto server = TestServer::getSingletonPtr();
    int oldDbgLvl = server->setDbgLevel( 4 );
    bool oldVerbose = server->setVerbose( true );
    REQUIRE( oldVerbose == false );
    REQUIRE( server->getVerbose() == true );
    REQUIRE( oldDbgLvl == 0 );
    REQUIRE( server->getDbgLevel() == 4 );

    oldDbgLvl = server->setDbgLevel( 1 );
    oldVerbose = server->setVerbose( true );
    REQUIRE( oldVerbose == true );
    REQUIRE( server->getVerbose() == true );
    REQUIRE( oldDbgLvl == 4 );
    REQUIRE( server->getDbgLevel() == 1 );

    oldDbgLvl = server->setDbgLevel( 0 );
    oldVerbose = server->setVerbose( false );
    REQUIRE( oldVerbose == true );
    REQUIRE( server->getVerbose() == false );
    REQUIRE( oldDbgLvl == 1 );
    REQUIRE( server->getDbgLevel() == 0 );
}

TEST_CASE_METHOD( TestServerFixture, "Testing Game members", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    auto server = TestServer::getSingletonPtr();
    auto gameCount = server->addGame();
    REQUIRE( gameCount == 1 );
    REQUIRE( server->getGameCount() == 1 );
    gameCount = server->remGame();
    REQUIRE( gameCount == 0 );
    REQUIRE( server->getGameCount() == 0 );
    for( int i = 0; i < 10; ++i ) {
        gameCount = server->addGame();
        REQUIRE( gameCount == (i+1) );
        REQUIRE( server->getGameCount() == (i + 1) );
    }
    for( int i = 10; i > 0; --i ) {
        gameCount = server->remGame();
        REQUIRE( gameCount == (i - 1) );
        REQUIRE( server->getGameCount() == (i - 1) );
    }
}

TEST_CASE_METHOD( TestServerFixture, "Testing player members", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    auto server = TestServer::getSingletonPtr();
    auto playerCount = server->addPlayer();
    REQUIRE( playerCount == 1 );
    REQUIRE( server->getPlayerCount() == 1 );
    playerCount = server->remPlayer();
    REQUIRE( playerCount == 0 );
    REQUIRE( server->getPlayerCount() == 0 );
    for( int i = 0; i < 10; ++i ) {
        playerCount = server->addPlayer();
        REQUIRE( playerCount == (i + 1) );
        REQUIRE( server->getPlayerCount() == (i + 1) );
    }
    for( int i = 10; i > 0; --i ) {
        playerCount = server->remPlayer();
        REQUIRE( playerCount == (i - 1) );
        REQUIRE( server->getPlayerCount() == (i - 1) );
    }
}

TEST_CASE_METHOD( TestServerFixture, "Testing statistics methods", "[Server]" )
{
    new TestServer( ".", false, 0 );
    REQUIRE( TestServer::getSingletonPtr() != nullptr );
    auto server = TestServer::getSingletonPtr();
    auto stats = server->getStatTotalPackets();
    REQUIRE( stats.size() == 0 );
    stats[1] = std::make_pair( 10, 20 );
    stats[2] = std::make_pair( 1, 2 );
    server->addStats(stats);
    stats = server->getStatTotalPackets();
    REQUIRE( stats.size() == 2 );
    REQUIRE( stats.count( 1 ) == 1 );
    REQUIRE( stats.count( 2 ) == 1 );
    REQUIRE( stats.count( 10 ) == 0 );
    REQUIRE( stats[1] == std::make_pair((uint64_t)10, (uint64_t) 20) );
    REQUIRE( stats[2] == std::make_pair((uint64_t) 1, (uint64_t)2 ) );
    server->clearStats();
    stats = server->getStatTotalPackets();
    REQUIRE( stats.size() == 0 );
}
