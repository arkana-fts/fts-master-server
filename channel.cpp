
#include <algorithm>

#include "constants.h"
#include "channel.h"
#include "client.h"
#include "db.h"

#include <connection.h>
#include <Logger.h>

using namespace FTS;
using namespace FTSSrv2;
using namespace std;

FTSSrv2::Channel::Channel(const channel_parameter_t & in_param, DataBase* in_pDataBase )
    : m_iID(in_param.id)
    , m_bPublic(in_param.isPublic)
    , m_sName(in_param.name)
    , m_sMotto(in_param.motto)
    , m_sAdmin(in_param.admin)
    , m_pDataBase(in_pDataBase)
{
    m_lsOperators.clear( );
    m_lpUsers.clear( );

    FTSMSGDBG("Created channel "+m_sName+" ("+string(m_bPublic ? "public" : "private")+", admin: "+m_sAdmin+")", 1);
}

FTSSrv2::Channel::~Channel( )
{
    // Kick all users that are still in the channel.
    while(!m_lpUsers.empty()) {
        this->kick(nullptr, m_lpUsers.front()->getNick());
    }

    m_lsOperators.clear();
}

int FTSSrv2::Channel::join(Client *in_pUser)
{
    string sJoinName = in_pUser->getNick();

    // Add the user to the users list.
    {
        Lock l(m_mutex);
        m_lpUsers.push_back(in_pUser);

        // Tell everybody that somebody joined.
        for(const auto& i : m_lpUsers) {
            // But don't tell myself !
            if( in_pUser == i )
                continue;

            i->sendChatJoins( sJoinName );
        }
    }

    in_pUser->setMyChannel(this);

    FTSMSGDBG(in_pUser->getNick() + " joined " + m_sName, 4);
    return ERR_OK;
}

int FTSSrv2::Channel::quit(Client *in_pUser)
{
    string sQuitName = in_pUser->getNick();

    // Remove the user from the list.
    {
        Lock l(m_mutex);
        m_lpUsers.remove(in_pUser);

        // Tell everybody that somebody left.
        for(const auto& i : m_lpUsers) {
            i->sendChatQuits(sQuitName);
        }
    }

    in_pUser->setMyChannel(nullptr);

    FTSMSGDBG(in_pUser->getNick() + " left " + m_sName, 4);
    return ERR_OK;
}

int FTSSrv2::Channel::save()
{
    string sQuery;
    Lock l(m_mutex);

    // Create this channel from scratch.
    if( m_iID < 0 ) {
        auto record = make_tuple(m_bPublic, m_sName, m_sMotto, m_sAdmin);
        auto rc = m_pDataBase->channelCreate( record );
        if( !rc ) {
            return -1;
        }

    // Or just save modifications ?
    } else {
        auto record = make_tuple(m_iID, m_bPublic, m_sName, m_sMotto, m_sAdmin);
        m_pDataBase->channelUpdate(record);
    }

    // Now we have to update the operators table.
    for(const auto& i : m_lsOperators) {
        auto record = make_tuple(i, m_iID);
        m_pDataBase->channelAddOp(record);
    }

    return ERR_OK;
}

int FTSSrv2::Channel::destroyDB(const string &in_sWhoWantsIt)
{
    // Not yet in database, ignore!
    if(m_iID < 0)
        return ERR_OK;
    auto record = make_tuple(in_sWhoWantsIt, m_sName);
    return m_pDataBase->channelDestroy(record);
}

int FTSSrv2::Channel::op(const string & in_sUser, bool in_bOninit)
{
    // Don't check all these things during the startup.
    if(!in_bOninit) {
        // Check if the user is in the channel.
        if(nullptr == this->getUserIfPresent(in_sUser)) {
            FTSMSGDBG(in_sUser+" does not exist in this channel", 4);
            return -31;
        }

        // Do not add the user to the operators list if he already is.
        if(this->isop(in_sUser)) {
            FTSMSGDBG(in_sUser+" is already operator in this channel", 4);
            return -32;
        }
    }

    if(in_sUser == m_sAdmin) {
        FTSMSGDBG(in_sUser+" is the channel admin!", 4);
        return -33;
    }

    // add to the operators list.
    {
        Lock l(m_mutex);
        m_lsOperators.push_back(in_sUser);

        if(!in_bOninit) {
            // Tell everybody that somebody op's, including to me.
            for(const auto& i : m_lpUsers) {
                i->sendChatOped( in_sUser );
            }
        }
    }

    // Store the changes in the database.
    this->save();

    FTSMSGDBG(in_sUser+" op in "+m_sName, 4);
    return ERR_OK;
}

int FTSSrv2::Channel::deop( const string & in_sUser )
{
    // Check if the user is op.
    if(!this->isop(in_sUser)) {
        FTSMSGDBG(in_sUser+" is not operator in this channel", 4);
        return -1;
    }

    // remove from the operators list.
    {
        Lock l(m_mutex);
        m_lsOperators.remove(in_sUser);

        // Tell everybody that somebody Deop's, including to me.
        for(const auto& i : m_lpUsers) {
            i->sendChatDeOped( in_sUser );
        }
    }

    // Store the changes in the database.
    this->save();

    FTSMSGDBG(in_sUser+" deop in "+m_sName, 4);
    return ERR_OK;
}

bool FTSSrv2::Channel::isop( const string & in_sUser )
{
    Lock l(m_mutex);
    for(const auto& i : m_lsOperators) {
        if(in_sUser == i) {
            return true;
        }
    }

    return false;
}

int FTSSrv2::Channel::messageToAll( const Client &in_From, const string & in_sMessage, uint8_t in_cFlags )
{
    Packet p(DSRV_MSG_CHAT_GETMSG);
    p.append(DSRV_CHAT_TYPE::NORMAL);
    p.append(in_cFlags);
    p.append(in_From.getNick());
    p.append(in_sMessage);

    FTSMSGDBG(in_From.getNick()+" says "+in_sMessage, 4);

    this->sendPacketToAll(&p);
    return ERR_OK;
}

Packet FTSSrv2::Channel::makeSystemMessagePacket( const string &in_sMessageID )
{
    Packet p(DSRV_MSG_CHAT_GETMSG);
    p.append(DSRV_CHAT_TYPE::SYSTEM);
    p.append((uint8_t)0);
    p.append(in_sMessageID);

    return p;
}

int FTSSrv2::Channel::sendPacketToAll( Packet *in_pPacket )
{
    Lock l(m_mutex);
    for(const auto& i : m_lpUsers) {
        i->sendPacket(in_pPacket);
    }

    return ERR_OK;
}

int FTSSrv2::Channel::setMotto( const string & in_sMotto, const string & in_sUser )
{
    // Only op or admin can do this !
    if( !this->isop(in_sUser) && in_sUser != m_sAdmin )
        return -20;

    {
        Lock l(m_mutex);
        m_sMotto = in_sMotto;

        // Tell everybody that somebody changed the motto, including to me.
        for(const auto& i : m_lpUsers) {
            i->sendChatMottoChanged( in_sUser, in_sMotto );
        }
    }

    return this->save( );
}

int FTSSrv2::Channel::kick( const Client *in_pFrom, const string & in_sUser )
{
    string sKicker;
    Client *pKicked = nullptr;

    // If the system wants to kick him out.
    if(in_pFrom == nullptr) {
        sKicker = "Arkana-FTS Server";
    } else {
        sKicker = in_pFrom->getNick();

        // Only op or admin can do this !
        if(!this->isop(sKicker) && sKicker != m_sAdmin)
            return -20;

        // The admin can't be kicked.
        if(in_sUser == m_sAdmin) {
            return -21;
        }

        // Only admins can kick operators.
        if(sKicker != m_sAdmin && this->isop(in_sUser)) {
            return -22;
        }
    }

    // Find the user that we want to kick.
    pKicked = this->getUserIfPresent(in_sUser);

    // User not in the channel ?
    if(pKicked == nullptr)
        return -21;

    // Kick him out to the default channel.
    quit(pKicked);
    auto defaultChannel = FTSSrv2::ChannelManager::getManager()->getDefaultChannel();
    defaultChannel->join(pKicked);

    // Tell him about it.
    Packet p(DSRV_MSG_CHAT_KICKED);
    p.append(sKicker);
    p.append(this->getName());
    pKicked->sendPacket(&p);

    // And tell all others in this channel about it.
    auto kickMsg = this->makeSystemMessagePacket("Chat_Kicked");
    kickMsg.append(sKicker);
    kickMsg.append(pKicked->getNick());
    sendPacketToAll(&kickMsg);

    return ERR_OK;
}

Client *FTSSrv2::Channel::getUserIfPresent(const string &in_sUsername)
{
    Lock l(m_mutex);
    for(const auto& pCli : m_lpUsers) {
        if(pCli->getNick() == in_sUsername) {
            return pCli;
        }
    }

    return nullptr;
}
