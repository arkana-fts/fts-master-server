#pragma once

#include <string>
#include <unordered_map>
#include <connection.h>
#include <mutex>

#include "Singleton.h"

// Taken from guideline support library (https://github.com/Microsoft/GSL/blob/master/include/gsl.h) to make ownership explicit.
// The GSL is propagated by ISOCPP committee.
template <class T>
using owner = T;
//-----------------

namespace FTSSrv2
{
class IClientsManager;
class ClientsManager;
class Client;
class DataBase;

class Server : public FTS::Singleton<Server>
{
public:

    Server(std::string in_logDir, bool in_bVerbose, int in_dbgLevel, DataBase* pDataBase);
    virtual ~Server();
    std::string getLogfilename(void) const { return m_sLogFile; };
    std::string getErrfilename(void) const { return m_sErrFile; };
    std::string getNetLogfilename(void) const { return m_sNetLogFile; };
    std::string getPlayersfilename(void) const { return m_sPlayersFile; };
    std::string getGamesfilename(void) const { return m_sGamesFile; };

    std::size_t addPlayer(void);
    std::size_t remPlayer(void);
    std::size_t getPlayerCount(void) const { return m_nPlayers; };
    std::size_t addGame(void);
    std::size_t remGame(void);
    std::size_t getGameCount(void) const { return m_nGames; };
    void clearStats();
    void addStats( PacketStats stats);
    PacketStats getStatTotalPackets();

    bool getVerbose() const { return m_bVerbose; };
    bool setVerbose(bool in_b) { bool bOld = m_bVerbose; m_bVerbose = in_b; return bOld; };
    int  getDbgLevel() const { return m_dbgLvl; }
    int  setDbgLevel(int dbgLvl) { int oldDbgLvl = m_dbgLvl; m_dbgLvl = dbgLvl; return oldDbgLvl; }

    IClientsManager* getClientsManager();
    DataBase* getDb() { return m_pDataBase; }
protected:
    /// Protect from copying.
    Server(const Server&) = delete;

    std::size_t m_nPlayers = 0 ;    ///< Number of players logged in.
    std::size_t m_nGames = 0;       ///< Number of games currently opened.

    std::recursive_mutex m_mutex;   ///< Protects from threaded calls.
    std::string m_sErrFile;         ///< The file to write error messages to.
    std::string m_sLogFile;         ///< The file to write logging messages to.
    std::string m_sNetLogFile;      ///< The file to log network traffic to.
    std::string m_sPlayersFile;     ///< The file to write the actual player count to.
    std::string m_sGamesFile;       ///< The file to write the actual games count to.
    bool m_bDaemon = false;         ///< Whether I'm started as a daemon or not.
    bool m_bVerbose = false;        ///< Whether I'm verbose or not.
    int m_dbgLvl = 0;               ///< All debug message must have a level .lt. this to be printed.
private:
    std::string tryFile(const std::string &in_sFilename, const std::string &in_sDir) const;

    owner<ClientsManager*> m_pClientsManager = nullptr; // Server owns the ptr
    owner<DataBase*>       m_pDataBase = nullptr;       // Server owns the ptr
    PacketStats m_totalPackets;
};

}

