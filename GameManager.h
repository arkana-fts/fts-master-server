#ifndef D_GAMEMANAGER_H
#define D_GAMEMANAGER_H

#include <list>
#include <string>
#include <cstdint>
#include <mutex>

#include "Singleton.h"

namespace FTS {
    class Packet;
}

namespace FTSSrv2 {

    class Game;

    class GameManager : public FTS::Singleton<GameManager>
    {
    private:
        std::list<Game *>m_lpGames;     ///< This list contains all existing games.
        std::recursive_mutex m_mutex;   ///< Mutex for accessing me.

    public:
        GameManager() = default;
        virtual ~GameManager() = default;

        int addGame( Game *in_pGame );
        int remGame( Game *in_pGame );
        int startGame( Game *in_pGame );
        Game *findGame( const std::string &in_sName );
        Game *findGameByHost( const std::string &in_sHost );

        int writeListToPacket( FTS::Packet *in_pPacket );

        std::int16_t getNGames() const { return (std::int16_t) m_lpGames.size(); }
    };

}
#endif