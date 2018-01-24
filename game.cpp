
#include <connection.h>
#include <Logger.h>
#include <packet.h>

#include "constants.h"
#include "game.h"
#include "client.h"
#include "dummy_types.h"


using namespace FTS;
using namespace FTSSrv2;
using namespace std;

FTSSrv2::Game::Game(const string &in_sCreater, const string &in_sCreaterIP, const string &in_sGameName, Packet *out_pPacket)
{
    m_bStarted = false;

    // Get the IP and player name of the client.
    m_sIP = in_sCreaterIP;
    m_sHost = in_sCreater;

    // Ok, let's collect data about the game ...
    m_sName = in_sGameName;
    out_pPacket->get(m_usPort);
    m_sMapName = out_pPacket->get_string();
    m_sMapDesc = out_pPacket->get_string();
    m_nMapMinPlayers = out_pPacket->get();
    m_nMapMaxPlayers = out_pPacket->get();
    m_sMapSuggPlayers = out_pPacket->get_string();
    m_sMapAuthor = out_pPacket->get_string();
    m_dtMapLastModif = out_pPacket->get_string();
    out_pPacket->get(); // Discard the info if the game is started or not.
    m_bPressBtnToStart = out_pPacket->get() == 1;
    m_gPreview.readFromPacket(out_pPacket);
    m_gIcon.readFromPacket(out_pPacket);

    string sLog = "Making game: \"" + in_sGameName + "\"\n"
                   "  at IP:port = " + in_sCreaterIP + ":" + toString(m_usPort);
    FTSMSGDBG(sLog, 1);

    // Join the game.
    this->playerJoined(in_sCreater);
    this->playerJoined("InexistentTestUser001");
    this->playerJoined("InexistentTestUser002");
    this->playerJoined("InexistentTestUser003");
}

FTSSrv2::Game::~Game( )
{
}

void FTSSrv2::Game::playerJoined(const string &in_sPlayer)
{
    lock_guard<recursive_mutex> l(m_mutex);
    m_lpPlayers.remove(in_sPlayer); // Prohibit double entries.
    m_lpPlayers.push_back(in_sPlayer);
}

void FTSSrv2::Game::playerLeft(const string &in_sPlayer)
{
    lock_guard<recursive_mutex> l(m_mutex);
    m_lpPlayers.remove(in_sPlayer);
}

int FTSSrv2::Game::addToInfoPacket(Packet *out_pPacket)
{
    if(!out_pPacket)
        return -1;

    lock_guard<recursive_mutex> l(m_mutex);
    out_pPacket->append(m_sIP);
    out_pPacket->append(m_usPort);
    out_pPacket->append(m_sHost);
    out_pPacket->append((uint8_t)m_lpPlayers.size());
    for( auto i : m_lpPlayers ) {
        out_pPacket->append(i);
    }
    out_pPacket->append(m_sMapName);
    out_pPacket->append(m_sMapDesc);
    out_pPacket->append(m_nMapMinPlayers);
    out_pPacket->append(m_nMapMaxPlayers);
    out_pPacket->append(m_sMapSuggPlayers);
    out_pPacket->append(m_sMapAuthor);
    out_pPacket->append(m_dtMapLastModif);
    out_pPacket->append(m_bStarted ? (uint8_t)1 : (uint8_t)0);
    out_pPacket->append(m_bPressBtnToStart ? (uint8_t)1 : (uint8_t)0);
    m_gPreview.writeToPacket(out_pPacket);
    m_gIcon.writeToPacket(out_pPacket);

    return ERR_OK;
}

int FTSSrv2::Game::addToLstPacket(Packet *out_pPacket)
{
    if(!out_pPacket)
        return -1;

    lock_guard<recursive_mutex> l(m_mutex);
    out_pPacket->append(m_sName);
    out_pPacket->append(m_sMapName);
    out_pPacket->append(m_bStarted ? (uint8_t)1 : (uint8_t)0);
    m_gIcon.writeToPacket(out_pPacket);
    return ERR_OK;
}

 /* EOF */
