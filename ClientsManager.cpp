
/**
 * \file ClientsManager.cpp
 * \author Pompei2
 * \date 03 Oct 2008 
 * \brief Singleton to manage all clients known by the server. 
 * \note Extracted from client.cpp
 **/

#include <TextFormatting.h>

#include "ClientsManager.h"
#include "client.h"

using namespace FTS;
using namespace FTSSrv2;
using namespace std;

FTSSrv2::ClientsManager::~ClientsManager()
{
    while(!m_mClients.empty()) {
        deleteClient(m_mClients.begin()->first);
    }
}

void FTSSrv2::ClientsManager::registerClient(FTSSrv2::Client *in_pClient)
{
    // We store them only in lowercase because nicknames are case insensitive.
    // So we can search for some guy more easily.
    Lock l(m_mutex);
    m_mClients[toLower(in_pClient->getNick())] = in_pClient;
}

void FTSSrv2::ClientsManager::unregisterClient(FTSSrv2::Client *in_pClient)
{
    Lock l(m_mutex);

    for(const auto& i : m_mClients) {
        if(i.second == in_pClient) {
            m_mClients.erase(i.first);
            return ;
        }
    }
}

FTSSrv2::Client *FTSSrv2::ClientsManager::findClient(const string &in_sName)
{
    Lock l(m_mutex);
    auto i = m_mClients.find(toLower(in_sName));

    if(i == m_mClients.end()) {
       return nullptr;
    }

    return i->second;
}

void FTSSrv2::ClientsManager::deleteClient(const string &in_sName)
{
    Lock l(m_mutex);
    auto i = m_mClients.find(toLower(in_sName));

    if(i == m_mClients.end()) {
        return ;
    }

    FTSSrv2::Client* pClient = i->second;
    pClient->tellToQuit();
}

