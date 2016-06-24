#pragma once 

#include <map>
#include <string>

#include "Mutex.h"
#include "ICLientsManager.h"

namespace FTSSrv2 {
    class Client;

    class ClientsManager : public IClientsManager
    {
    public:
        ClientsManager() = default;
        ClientsManager(const ClientsManager& other) = delete;
        ClientsManager(ClientsManager&& other) = delete;

        ~ClientsManager();

        ClientsManager& operator=(const ClientsManager& other) = delete;
        ClientsManager& operator=(ClientsManager&& other) = delete;

        void add( Client *in_pClient );
        void remove( Client *in_pClient );
        Client *find( const std::string &in_sName );

    private:
        void deleteClient(const std::string &in_sName);

        std::map<std::string, Client *>m_mClients;
        Mutex m_mutex;
    };

}

