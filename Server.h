#if !defined(FTS_SERVER_H)
#define FTS_SERVER_H

#include <string>
#include <unordered_map>
#include <connection.h>
#include "Singleton.h"
#include "Mutex.h"

namespace FTSSrv2
{
using TotalStatPackets = std::unordered_map< int, std::pair<int, int> >;

class Server : public FTS::Singleton<Server>
{
public:

    Server(std::string in_logDir, bool in_bVerbose, int in_dbgLevel);
    virtual ~Server() {}
    inline std::string getLogfilename(void) const { return m_sLogFile; };
    inline std::string getErrfilename(void) const { return m_sErrFile; };
    inline std::string getNetLogfilename(void) const { return m_sNetLogFile; };
    inline std::string getPlayersfilename(void) const { return m_sPlayersFile; };
    inline std::string getGamesfilename(void) const { return m_sGamesFile; };

    size_t addPlayer(void);
    size_t remPlayer(void);
    inline size_t getPlayerCount(void) const { return m_nPlayers; };
    size_t addGame(void);
    size_t remGame(void);
    inline size_t getGameCount(void) const { return m_nGames; };
    void clearStats();
    void addStats( PacketStats stats);
    PacketStats getStatTotalPackets();

    inline bool getVerbose() const { return m_bVerbose; };
    inline bool setVerbose(bool in_b) { bool bOld = m_bVerbose; m_bVerbose = in_b; return bOld; };
    inline int  getDbgLevel() const { return m_dbgLvl; }
    inline int  setDbgLevel(int dbgLvl) { int oldDbgLvl = m_dbgLvl; m_dbgLvl = dbgLvl; return oldDbgLvl; }

protected:
    /// Protect from copying.
    Server(const Server&) = delete;

    size_t m_nPlayers;          ///< Number of players logged in.
    size_t m_nGames;            ///< Number of games currently opened.

    FTS::Mutex m_mutex;         ///< Protects from threaded calls.
    std::string m_sErrFile;     ///< The file to write error messages to.
    std::string m_sLogFile;     ///< The file to write logging messages to.
    std::string m_sNetLogFile;  ///< The file to log network traffic to.
    std::string m_sPlayersFile; ///< The file to write the actual player count to.
    std::string m_sGamesFile;   ///< The file to write the actual games count to.
    bool m_bDaemon;             ///< Whether I'm started as a daemon or not.
    bool m_bVerbose;            ///< Whether I'm verbose or not.
    int m_dbgLvl;               ///< All debug message must have a level .lt. this to be printed.
private:
    std::string tryFile(const std::string &in_sFilename, const std::string &in_sDir) const;

    PacketStats m_totalPackets;
};

}

#endif /* SERVER_H */
