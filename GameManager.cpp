#include "constants.h"
#include "GameManager.h"
#include "game.h"
#include <packet.h>

using namespace FTS;
using namespace FTSSrv2;

int FTSSrv2::GameManager::addGame( FTSSrv2::Game *in_pGame )
{
    m_mutex.lock();

    m_lpGames.push_back(in_pGame);

    m_mutex.unlock();
    return ERR_OK;
}

int FTSSrv2::GameManager::remGame(FTSSrv2::Game *in_pGame)
{
    // If the game has been canceled, remove it from the list.
    m_mutex.lock();
    m_lpGames.remove(in_pGame);
    m_mutex.unlock();

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
    m_mutex.lock();

    for(std::list<FTSSrv2::Game *>::iterator i = m_lpGames.begin() ; i != m_lpGames.end() ; i++) {
        if((*i)->getName() == in_sName) {
            m_mutex.unlock();
            return *i;
        }
    }

    m_mutex.unlock();
    return NULL;
}

FTSSrv2::Game *FTSSrv2::GameManager::findGameByHost(const std::string &in_sHost)
{
    m_mutex.lock();

    for(std::list<FTSSrv2::Game *>::iterator i = m_lpGames.begin() ; i != m_lpGames.end() ; i++) {
        if((*i)->getHost() == in_sHost) {
            m_mutex.unlock();
            return *i;
        }
    }

    m_mutex.unlock();
    return NULL;
}

int FTSSrv2::GameManager::writeListToPacket(Packet *in_pPacket)
{
    if(!in_pPacket)
        return -1;

    m_mutex.lock();
    for(std::list<FTSSrv2::Game *>::iterator i = m_lpGames.begin() ; i != m_lpGames.end() ; i++) {
        (*i)->addToLstPacket(in_pPacket);
    }

    m_mutex.unlock();
    return ERR_OK;
}

