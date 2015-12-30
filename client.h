#ifndef D_CLIENT_H
#  define D_CLIENT_H

#  include <list>
#  include <map>

#  include "Mutex.h"
#  include <connection.h>
#include "ClientsManager.h"

namespace FTS {
    class Packet;
    class Connection;
}


namespace FTSSrv2 {
    class Game;
    class Channel;

class Client {
private:
    bool m_bLoggedIn;               ///< Whether the user is logged in or not.
    std::string m_sNick;            ///< The nickname of the user that is logged on.
    std::string m_sPassMD5;         ///< The MD5 encoded password of the user that is logged on.

    Game *m_pMyGame;                ///< The game I have started or I am in. NULL else.
    Channel *m_pMyChannel;          ///< The chat channel the player is currently in.
    Mutex m_mutex;                  ///< Protect the sendPacket, it is used by other clients to send a msg directly.
    FTS::Connection *m_pConnection; ///< The connection to my client.

public:
    friend class ClientsManager;

    Client(FTS::Connection *in_pConnection);
    virtual ~Client();

    static void starter(Client *in_pThis);
    int run();
    bool workPacket(FTS::Packet *in_pPacket);
    int quit();
    int tellToQuit();

    inline std::string getNick() const { return m_sNick; };
    inline bool isLoggedIn() const { return m_bLoggedIn; };

    inline std::string getCounterpartIP() const { return m_pConnection->getCounterpartIP(); };

    inline Channel *getMyChannel() const { return m_pMyChannel; };
    inline void setMyChannel(Channel *in_pChannel) { m_pMyChannel = in_pChannel; };
    inline Game *getMyGame() const { return m_pMyGame; };
    inline void setMyGame(Game *in_pGame) { m_pMyGame = in_pGame; };

    int getID() const;
    static int getIDByNick(const std::string &in_sNick);

    bool sendPacket(FTS::Packet *in_pPacket);
    int sendChatJoins(const std::string &in_sPlayer);
    int sendChatQuits(const std::string &in_sPlayer);
    int sendChatOped(const std::string &in_sPlayer);
    int sendChatDeOped(const std::string &in_sPlayer);
    int sendChatMottoChanged(const std::string &in_sFrom, const std::string &in_sMotto);

private:

    //     int onNull( void );
    bool onLogin(const std::string &in_sNick, const std::string &in_sMD5);
    bool onLogout();

    bool onSignup(const std::string &in_sNick, const std::string &in_sMD5, const std::string &in_sEMail);
    bool onFeedback(const std::string &in_sMessage);
    bool onPlayerSet(uint8_t in_cField, FTS::Packet *out_pPacket);
    bool onPlayerGet(uint8_t in_cField, const std::string &in_sNick, FTS::Packet *out_pPacket);
    bool onPlayerSetFlag(uint32_t in_cFlag, bool in_bValue);

    bool onGameIns(FTS::Packet *out_pPacket);
    bool onGameRem();
    bool onGameLst();
    bool onGameInfo(const std::string &in_sName);
    bool onGameStart();

    bool onChatSend(FTS::Packet *out_pPacket);
    bool onChatIuNai();
    bool onChatJoin(const std::string &in_sChan);
    bool onChatList();
    bool onChatMottoGet();
    bool onChatMottoSet(const std::string &in_sMotto);
    bool onChatUserGet(const std::string &in_sUser);
    bool onChatPublics();
    bool onChatKick(const std::string &in_sUser);
    bool onChatOp(const std::string &in_sUser);
    bool onChatDeop(const std::string &in_sUser);
    bool onChatListMyChans();
    bool onChatDestroyChan(const std::string &in_sChan);
};


} // namespace FTSSrv2

#endif                          /* D_CLIENT_H */
