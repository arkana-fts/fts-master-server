#include <iostream>
#include <fstream>

#include "Server.h"
#include "config.h"
#include "constants.h"

using namespace FTS;

FTSSrv2::Server::Server(std::string in_logDir, bool in_bVerbose, int in_dbgLevel)
    : m_bVerbose(in_bVerbose)
    , m_dbgLvl(in_dbgLevel)
{
    m_sLogFile      = tryFile(DSRV_LOGFILE_LOG    ,in_logDir);
    m_sErrFile      = tryFile(DSRV_LOGFILE_ERR    ,in_logDir);
    m_sNetLogFile   = tryFile(DSRV_LOGFILE_NETLOG ,in_logDir);
    m_sGamesFile    = tryFile(DSRV_FILE_NGAMES    ,in_logDir);
    m_sPlayersFile  = tryFile(DSRV_FILE_NPLAYERS  ,in_logDir);
}

// Finds a writable path for a file.
std::string FTSSrv2::Server::tryFile(const std::string &in_sFilename, const std::string &in_sDir) const
{
    std::string sDir = in_sDir;

    // Add a trailing slash if needed.
    if(sDir[sDir.length() - 1] != '/') {
        sDir += "/";
    }

    std::string sHome = getenv("HOME");
    std::string sFile = sDir + in_sFilename;

    // First try.
    std::cout << "Trying to access to the file at '" << sFile << "' ... ";
    std::ofstream f;
    f.open(sFile);
    if( !f.is_open() ) {
        std::cout << "Failed !\n";
    } else {
        std::cout << "Success !\n";
        f.close();
        return sFile;
    }

    // Second try.
    sFile = sHome + "/" + in_sFilename;
    std::cout << "Trying to access to the file at '" << sFile << "' ... ";
    f.open(sFile);
    if(!f.is_open()) {
        std::cout << "Failed !\n";
        std::cout << "Logging will be disabled.\n";
        return "";
    } else {
        std::cout << "Success !\n";
        f.close();
        return sFile;
    }
}

size_t FTSSrv2::Server::addPlayer()
{
    // Modify the statistics.
    Lock l(m_mutex);

    m_nPlayers++;
    std::ofstream of(m_sPlayersFile.c_str(), std::ios_base::trunc);
    if(of) {
        of << m_nPlayers;
    }

    return m_nPlayers;
}

size_t FTSSrv2::Server::remPlayer()
{
    // Modify the statistics.
    Lock l(m_mutex);
    m_nPlayers--;
    std::ofstream of(m_sPlayersFile.c_str(), std::ios_base::trunc);
    if(of) {
        of << m_nPlayers;
    }

    return m_nPlayers;
}

size_t FTSSrv2::Server::addGame()
{
    // Modify the statistics.
    Lock l(m_mutex);
    m_nGames++;
    std::ofstream of(m_sGamesFile.c_str(), std::ios_base::trunc);
    if(of) {
        of << m_nGames;
    }

    return m_nGames;
}

size_t FTSSrv2::Server::remGame()
{
    // Modify the statistics.
    Lock l(m_mutex);
    m_nGames--;
    std::ofstream of(m_sGamesFile.c_str(), std::ios_base::trunc);
    if(of) {
        of << m_nGames;
    }

    return m_nGames;
}

PacketStats FTSSrv2::Server::getStatTotalPackets()
{
    return m_totalPackets;
}

void FTSSrv2::Server::clearStats()
{
    Lock l(m_mutex);
    m_totalPackets.clear();
}

void FTSSrv2::Server::addStats( PacketStats stats)
{
    for( auto it : stats ) {
        m_totalPackets[it.first].first += it.second.first;
        m_totalPackets[it.first].second += it.second.second;
    }
}
