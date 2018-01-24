#pragma once

#include <list>
#include <string>
#include <cstdint>
#include <mutex>

#include "dummy_types.h"


namespace FTS {
    class Packet;
}

namespace FTSSrv2 {

class Game 
{

public:
    Game(const std::string &in_sCreater, const std::string &in_sCreaterIP, const std::string &in_sGameName, FTS::Packet *out_pPacket);
    virtual ~Game();

    void playerJoined(const std::string &in_sPlayer);
    void playerLeft(const std::string &in_sPlayer);

    inline std::string getName() const { return m_sName; }
    inline void start() { m_bStarted = true; }
    inline std::string getHost() const { return m_sHost; }

    int addToInfoPacket(FTS::Packet *out_pPacket);
    int addToLstPacket(FTS::Packet *out_pPacket);

private:
    std::string m_sName;            ///< The name of the game you want to create.
    std::string m_sIP;              ///< The IPv4 address of the host (xxx.xxx.xxx.xxx)
    std::uint16_t m_usPort;              ///< The portnumber on what you can connect to the host.
    std::string m_sHost;            ///< The player that crated this game.

    std::string m_sMapName;         ///< The name of the map.
    std::string m_sMapDesc;         ///< A description of the map.
    std::uint8_t m_nMapMinPlayers;       ///< The minimum number of players needed to play this map.
    std::uint8_t m_nMapMaxPlayers;       ///< The maximum number of players allowed to play this map.
    std::string m_sMapSuggPlayers;  ///< The suggested number of players (for example: "2v2 and 3v3").
    std::string m_sMapAuthor;       ///< This is a string containing the name of the author of the map.
    std::string m_dtMapLastModif;   ///< When this map has been last modified.
    FTSSrv2::Graphic m_gPreview;    ///< The preview image that is associated with this map.
    FTSSrv2::Graphic m_gIcon;       ///< The icon image that is associated with this map.

    bool m_bStarted;                ///< Whether this game is already started or not.
    bool m_bPressBtnToStart;        ///< Whether to press a button to start the game or not.

    std::list<std::string> m_lpPlayers;   ///< The players that are currently in this game.

    std::recursive_mutex m_mutex;   ///< A mutex to protect myself.
};

} // namespace FTSSrv2

/* EOF */
