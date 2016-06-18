#ifndef D_CHANNELMANAGER_H
#define D_CHANNELMANAGER_H

#include <list>

#include "Mutex.h"
#include "Singleton.h"

namespace FTSSrv2 {

    class Channel;
    class Client;

    class ChannelManager : public FTS::Singleton<ChannelManager>
    {

    public:
        ChannelManager();
        virtual ~ChannelManager();

        // Singleton-like stuff.
        int init();
        static ChannelManager *getManager();
        static void deinit();


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
        uint32_t countUserChannels(const std::string &in_sUserName);

        std::list<Channel *>m_lpChannels; ///< This list contains all existing channels.
        Mutex m_mutex; ///< Mutex for accessing me.
    };

}
#endif