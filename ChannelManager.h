#pragma once

#include <list>
#include <string>
#include <cstdint>
#include <mutex>

#include "Singleton.h"

namespace FTSSrv2 {

    class Channel;
    class Client;

    class ChannelManager : public FTS::Singleton<ChannelManager>
    {

    public:
        ChannelManager() = default;
        virtual ~ChannelManager();

        int init();

        Channel *createChannel( const std::string &in_sName, const Client *in_pCreater, bool in_bPublic = false );
        int removeChannel( Channel *out_pChannel, const std::string &in_sWhoWantsIt );

        std::list<Channel *> getPublicChannels();

        Channel *findChannel( const std::string &in_sName );
        Channel *getDefaultChannel();
        std::list<std::string> getUserChannels( const std::string &in_sUserName );

        std::size_t getNChannels() { return m_lpChannels.size(); }

    private:
        int loadChannels();
        int saveChannels();
        std::uint32_t countUserChannels(const std::string &in_sUserName);

        std::list<Channel *>m_lpChannels; ///< This list contains all existing channels.
        std::recursive_mutex m_mutex;     ///< Mutex for accessing me.
    };

}
