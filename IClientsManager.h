#pragma once

namespace FTSSrv2 {
    class Client;

    class IClientsManager
    {
    public:
        IClientsManager() = default;
        IClientsManager(const IClientsManager& other) = delete;
        IClientsManager(IClientsManager&& other) = delete;
        IClientsManager& operator=(const IClientsManager& other) = delete;
        IClientsManager& operator=(IClientsManager&& other) = delete;

        virtual void add(Client* in_pClient ) = 0 ;
        virtual void remove(Client *in_pClient) = 0;
        virtual Client *find(const std::string &in_sName) = 0;
    protected:
        virtual ~IClientsManager() = default; // don't delete through interface pointer.
    };
}