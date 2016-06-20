
#include <algorithm>
#include <TextFormatting.h>
#include "constants.h"
#include "ChannelManager.h"
#include "channel.h"
#include "client.h"
#include "db.h"
#include "config.h"

using namespace FTS;
using namespace FTSSrv2;
using namespace std;

FTSSrv2::ChannelManager::ChannelManager()
{
    m_lpChannels.clear();
}

FTSSrv2::ChannelManager::~ChannelManager()
{
    this->saveChannels( );

    for( const auto& i : m_lpChannels ) {
        delete i;
    }

    m_lpChannels.clear();
}

int FTSSrv2::ChannelManager::init()
{
    // First, load all the channels from the database.
    if(ERR_OK != loadChannels()) {
        return -1;
    }

    // Then, check if the main channel is existing.
    if(getDefaultChannel() == nullptr) {
        // If not, create it with Pompei2 as admin.
        // We do create it manually here because it's a special case.
        Channel::channel_parameter_t param;
        param.isPublic = true;
        param.name = DSRV_DEFAULT_CHANNEL_NAME;
        param.motto = DSRV_DEFAULT_CHANNEL_MOTTO;
        param.admin = DSRV_DEFAULT_CHANNEL_ADMIN;
        m_lpChannels.push_back(new FTSSrv2::Channel(param, DataBase::getUniqueDB()));

    } else if(getDefaultChannel()->getAdmin() != DSRV_DEFAULT_CHANNEL_ADMIN) {
        // Or if somehow another admin is entered in it, set it to the default admin!
        getDefaultChannel()->setAdmin(DSRV_DEFAULT_CHANNEL_ADMIN);
    }

    // Same for the dev's channel.
    if(findChannel(DSRV_DEVS_CHANNEL_NAME) == nullptr) {
        // If not, create it with Pompei2 as admin.
        // We do create it manually here because it's a special case.
        Channel::channel_parameter_t param;
        param.isPublic = true;
        param.name     = DSRV_DEVS_CHANNEL_NAME;
        param.motto    = DSRV_DEVS_CHANNEL_MOTTO;
        param.admin    = DSRV_DEVS_CHANNEL_ADMIN;
        m_lpChannels.push_back(new FTSSrv2::Channel(param, DataBase::getUniqueDB()));

    } else if(findChannel(DSRV_DEVS_CHANNEL_NAME)->getAdmin() != DSRV_DEVS_CHANNEL_ADMIN) {
        // Or if somehow another admin is entered in it, set it to the default admin!
        findChannel(DSRV_DEVS_CHANNEL_NAME)->setAdmin(DSRV_DEVS_CHANNEL_ADMIN);
    }

    return ERR_OK;
}

FTSSrv2::ChannelManager *FTSSrv2::ChannelManager::getManager()
{
    return Singleton::getSingletonPtr();
}

void FTSSrv2::ChannelManager::deinit()
{
    delete Singleton::getSingletonPtr();
}

int FTSSrv2::ChannelManager::loadChannels(void)
{
    MYSQL_RES *pRes = nullptr;
    MYSQL_ROW pRow = nullptr;

    // Do the query to get the field.
    string sQuery = "SELECT  `"+DataBase::getUniqueDB()->TblChansField(DSRV_TBL_CHANS_ID)+"`"
                            ",`"+DataBase::getUniqueDB()->TblChansField(DSRV_TBL_CHANS_PUBLIC)+"`"
                            ",`"+DataBase::getUniqueDB()->TblChansField(DSRV_TBL_CHANS_NAME)+"`"
                            ",`"+DataBase::getUniqueDB()->TblChansField(DSRV_TBL_CHANS_MOTTO)+"`"
                            ",`"+DataBase::getUniqueDB()->TblChansField(DSRV_TBL_CHANS_ADMIN)+"`"
                     " FROM `" DSRV_TBL_CHANS "`";

    if(!DataBase::getUniqueDB()->query(pRes, sQuery)) {
        DataBase::getUniqueDB()->free(pRes);
        return -1;
    }

    // Invalid record ? forget about it!
    if(pRes == nullptr || mysql_num_fields(pRes) < 5) {
        DataBase::getUniqueDB()->free(pRes);
        return -2;
    }

    // Create every single channel.
    while(nullptr != (pRow = mysql_fetch_row(pRes))) {
        Channel::channel_parameter_t param;
        param.id       = atoi(pRow[0]);
        param.isPublic = (pRow[1] == nullptr ? false : (pRow[1][0] == '0' ? false : true));
        param.name     = pRow[2];
        param.motto    = pRow[3];
        param.admin    = pRow[4];

        FTSSrv2::Channel *pChan = new FTSSrv2::Channel(param, DataBase::getUniqueDB());
        m_lpChannels.push_back(pChan);
    }

    DataBase::getUniqueDB()->free(pRes);

    // Now read all channel operators.
    sQuery = "SELECT  `"+DataBase::getUniqueDB()->TblChanOpsField(DSRV_VIEW_CHANOPS_NICK)+"`"
                    ",`"+DataBase::getUniqueDB()->TblChanOpsField(DSRV_VIEW_CHANOPS_CHAN)+"`"
             " FROM `" DSRV_VIEW_CHANOPS "`";
    if(!DataBase::getUniqueDB()->query(pRes, sQuery)) {
        DataBase::getUniqueDB()->free(pRes);
        return -3;
    }

    // Invalid record ? forget about it!
    if(pRes == nullptr || mysql_num_fields(pRes) < 2) {
        DataBase::getUniqueDB()->free(pRes);
        return -4;
    }

    // Setup every operator<->channel connection.
    // But first just put all assocs. in a list because we need to free the DB.
    std::list<std::pair<FTSSrv2::Channel *, string> > operators;
    while(nullptr != (pRow = mysql_fetch_row(pRes))) {
        FTSSrv2::Channel *pChan = this->findChannel(pRow[1]);

        if(!pChan)
            continue;

        operators.push_back(std::make_pair(pChan, pRow[0]));
    }

    DataBase::getUniqueDB()->free(pRes);

    // Now we execute that action (only now as the DB result needs to be freed).
    for( auto& i : operators ) {
        i.first->op(i.second, true);
    }

    return ERR_OK;
}

int FTSSrv2::ChannelManager::saveChannels( void )
{
    Lock l(m_mutex);
    for( const auto& i : m_lpChannels ) {
        i->save();
    }

    return ERR_OK;
}

FTSSrv2::Channel *FTSSrv2::ChannelManager::createChannel(const string & in_sName, const Client *in_pCreater, bool in_bPublic)
{
    // Every user can only create a limited amount of channels!
    if(this->countUserChannels(in_pCreater->getNick()) >= DSRV_MAX_CHANS_PER_USER) {
        return nullptr;
    }

    Channel::channel_parameter_t param;
    param.isPublic = in_bPublic;
    param.name = in_sName;
    param.motto = DSRV_DEFAULT_MOTTO;
    param.admin = in_pCreater->getNick();

    FTSSrv2::Channel *pChannel = new FTSSrv2::Channel(param, DataBase::getUniqueDB());

    Lock l(m_mutex);
    m_lpChannels.push_back(pChannel);
    pChannel->save(); // Update the database right now!

    return pChannel;
}

int FTSSrv2::ChannelManager::removeChannel(FTSSrv2::Channel *out_pChannel, const string &in_sWhoWantsIt)
{
    Lock l(m_mutex);
    for(const auto& i : m_lpChannels) {
        if(i == out_pChannel) {
            // Found! remove it from DB and manager, if we have the rights!
            if(ERR_OK == out_pChannel->destroyDB(in_sWhoWantsIt)) {
                m_lpChannels.remove(i);
                // The order here is important, as deleting the channel might kick players, that will be locking the mutex.
                delete out_pChannel;
                return ERR_OK;
            } else {
                // Found, but no right to remove it.
                return -2;
            }
        }
    }

    // Not found.
    return -1;
}

std::list<FTSSrv2::Channel *> FTSSrv2::ChannelManager::getPublicChannels()
{
    std::list<FTSSrv2::Channel *>lpPubChannels;

    Lock l(m_mutex);
    for( const auto& i : m_lpChannels ) {
        if( i->isPublic( ) ) {
            lpPubChannels.push_back(i);
        }
    }

    return lpPubChannels;
}

FTSSrv2::Channel *FTSSrv2::ChannelManager::findChannel(const string & in_sName)
{
    Lock l(m_mutex);
    for(const auto& i : m_lpChannels) {
        // "Pompei2's ChanNel" is the same as "pOmpei2's chAnnEl"
        if(ieq( i->getName(), in_sName)) {
            return i;
        }
    }

    return nullptr;
}

std::uint32_t FTSSrv2::ChannelManager::countUserChannels(const string &in_sUserName)
{
    Lock l(m_mutex);
    std::uint32_t nChans = (std::uint32_t) std::count_if( std::begin( m_lpChannels ), std::end( m_lpChannels ), [in_sUserName] ( Channel* pChan ){ return ieq(pChan->getAdmin(),in_sUserName); } );
    return  nChans;
}

std::list<string> FTSSrv2::ChannelManager::getUserChannels(const string &in_sUserName)
{
    std::list<string> sChans;

    Lock l(m_mutex);
    for(const auto& i : m_lpChannels) {
        if(ieq( i->getAdmin(), in_sUserName)) {
            sChans.push_back(i->getName());
        }
    }

    return sChans;
}

FTSSrv2::Channel *FTSSrv2::ChannelManager::getDefaultChannel(void)
{
    return this->findChannel(DSRV_DEFAULT_CHANNEL_NAME);
}


