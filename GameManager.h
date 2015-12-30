#ifndef D_GAMEMANAGER_H
#define D_GAMEMANAGER_H

#include <list>
#include <string>
#include "Mutex.h"
#include "Singleton.h"

namespace FTS {
    class Packet;
}

namespace FTSSrv2 {

    class Game;

    class GameManager : public FTS::Singleton<GameManager>
    {
    private:
        std::list<Game *>m_lpGames; ///< This list contains all existing games.
        Mutex m_mutex; ///< Mutex for accessing me.

    public:
        GameManager() = default;
        virtual ~GameManager() = default;

        int addGame( Game *in_pGame );
        int remGame( Game *in_pGame );
        int startGame( Game *in_pGame );
        Game *findGame( const std::string &in_sName );
        Game *findGameByHost( const std::string &in_sHost );

        int writeListToPacket( FTS::Packet *in_pPacket );

        int16_t getNGames()
        {
            return ( int16_t ) m_lpGames.size();
        }

        // Singleton-like stuff.
        static GameManager *getManager();
        static void deinit();
    };

}
#endif