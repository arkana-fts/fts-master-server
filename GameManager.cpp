#include "constants.h"
#include "GameManager.h"
#include "game.h"
#include <packet.h>

using namespace FTS;
using namespace FTSSrv2;

int FTSSrv2::GameManager::addGame( FTSSrv2::Game *in_pGame )
{
    Lock l(m_mutex);

    m_lpGames.push_back(in_pGame);

    return ERR_OK;
}

int FTSSrv2::GameManager::remGame(FTSSrv2::Game *in_pGame)
{
    // If the game has been canceled, remove it from the list.
    Lock l(m_mutex);
    m_lpGames.remove(in_pGame);

    delete in_pGame;

    return ERR_OK;
}

int FTSSrv2::GameManager::startGame(FTSSrv2::Game *in_pGame)
{
    // If the game has been started, keep it in the list and set it to started state.
    in_pGame->start();
    return ERR_OK;
}

FTSSrv2::Game *FTSSrv2::GameManager::findGame( const std::string &in_sName )
{
    Lock l(m_mutex);

    for( auto i : m_lpGames ) {
        if(i->getName() == in_sName) {
            return i;
        }
    }

    return nullptr;
}

FTSSrv2::Game *FTSSrv2::GameManager::findGameByHost(const std::string &in_sHost)
{
    Lock l(m_mutex);

    for( auto i : m_lpGames ) {
        if(i->getHost() == in_sHost) {
            return i;
        }
    }

    return nullptr;
}

int FTSSrv2::GameManager::writeListToPacket(Packet *in_pPacket)
{
    if(!in_pPacket)
        return -1;

    Lock l(m_mutex);
    for( auto i : m_lpGames) {
        i->addToLstPacket(in_pPacket);
    }

    return ERR_OK;
}

