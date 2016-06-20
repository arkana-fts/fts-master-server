#ifndef D_CHANNEL_H
#define D_CHANNEL_H

#include "Mutex.h"

#include <list>
#include "ChannelManager.h"

namespace FTS {
    class Packet;
}

namespace FTSSrv2 {
    class Client;
    class DataBase;

class Channel {
private:
    int m_iID;        ///< The MySQL ID of the row, needed when saving.
    bool m_bPublic;   ///< Whether this is publicly visible or not.
    std::string m_sName;  ///< The name of this channel.
    std::string m_sMotto; ///< The motto of this channel.
    std::string m_sAdmin; ///< The name of the admin of this channel.

    std::list<std::string>m_lsOperators; ///< The names of the operators of this channel.
    std::list<Client *>m_lpUsers;   ///< The users that are currently in this channel.

    Mutex m_mutex; ///< Mutex for accessing me.
    DataBase* m_pDataBase = nullptr; ///< Access the database.

public:
    Channel(int in_iID,
            bool in_bPublic,
            const std::string &in_sName,
            const std::string &in_sMotto,
            const std::string &in_sAdmin,
            DataBase* in_pDataBase);
    virtual ~Channel();

    int join(Client *in_pUser);
    int quit(Client *in_pUser);
    int save();
    int destroyDB(const std::string &in_sWhoWantsIt);

    int op(const std::string & in_sUser, bool in_bOninit = false);
    int deop(const std::string & in_sUser);
    bool isop(const std::string & in_sUser);

    int messageToAll(const Client &in_From, const std::string &in_sMessage, uint8_t in_cFlags);

    FTS::Packet makeSystemMessagePacket(const std::string &in_sMessageID);
    int sendPacketToAll(FTS::Packet *in_pPacket);

    int kick(const Client *in_pFrom, const std::string &in_sUser);

    inline std::string getMotto() const { return m_sMotto; }
    int setMotto(const std::string &in_sMotto, const std::string &in_sClient);

    inline std::string getName() const { return m_sName; }
    inline std::string getAdmin() const { return m_sAdmin; }
    inline void setAdmin(const std::string &in_sNewAdmin) { m_sAdmin = in_sNewAdmin; }
    inline std::list<Client *> getUsers() const { return m_lpUsers; }
    inline int getNUsers() const { return (int)this->getUsers().size(); }
    inline bool isPublic() const { return m_bPublic; }

    Client *getUserIfPresent(const std::string &in_sUsername);
};

} // namespace FTSSrv2

#endif // D_CHANNEL_H

/* EOF */
