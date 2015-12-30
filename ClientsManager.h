#ifndef D_CLIENTSMANAGER_H
#define D_CLIENTSMANAGER_H

#include <list>
#include <map>
#include <string>

#include "Mutex.h"
#include "Singleton.h"

namespace FTS {
    class Packet;
    class Connection;
}

namespace FTSSrv2 {
    class Client;

    class ClientsManager : public FTS::Singleton<ClientsManager>
    {
    private:
        std::map<std::string, Client *>m_mClients;
        Mutex m_mutex;
    protected:
    public:
        ClientsManager();
        virtual ~ClientsManager();

        static ClientsManager *getManager();
        static void deinit();

        Client *createClient( FTS::Connection *in_pConnection );
        void registerClient( Client *in_pClient );
        void unregisterClient( Client *in_pClient );
        Client *findClient( const std::string &in_sName );
        Client *findClient( const FTS::Connection *in_pConnection );
        void deleteClient( const std::string &in_sName );
    };

}

#endif
