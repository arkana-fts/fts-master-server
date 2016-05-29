#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <condition_variable>


#if defined(_WIN32)
#  include <WinSock2.h>
#  pragma comment(lib, "Ws2_32.lib")
#  pragma warning (disable: 4996) // disable complaining about not ISO C++ conformant name open etc.
#else
#  include <signal.h>
#  include <pwd.h>
#  include <sys/stat.h>
#  include <string.h>
#  include <unistd.h>
#  include <fcntl.h>
#endif

#include <fts-net.h>
#include <Logger.h>
#include <connection.h>
#include <connection_waiter.h>

#include "Server.h"
#include "constants.h"
#include "db.h"
#include "client.h"
#include "ClientsManager.h"
#include "ChannelManager.h"
#include "GameManager.h"
#include "GetOpt.h"
#include "config.h"


/* Change this to whatever your daemon is called */
#define DAEMON_NAME "fts-server"

/* The name of the lockfile, directory will be home at runtime. */
#define LOCK_FILE DAEMON_NAME ".lock"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

bool g_bExit = false;
std::mutex mtxExit;
std::condition_variable cvExit;

bool stopSpamThread = false;

void connectionListener(uint16_t in_iPort);
void help( const std::string& in_pszLine );
void printServerStats();
void cmdline();

static void daemonize( const char *lockfile, const char *dir );
static void trytokill(const char *lockfile);

using namespace std;
using namespace FTSSrv2;
using namespace FTS;

std::string getNextToken( stringstream& sb, char delimiter = ' ' )
{
    string token;
    do {
        getline( sb, token, delimiter );
    } while( token.empty() && !sb.eof() );

    return token;
}

int main(int argc, char *argv[])
{
#if defined(_DEBUG) && defined(_WIN32)
    int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
    _CrtSetDbgFlag( flag );
    //_CrtSetBreakAlloc( 5382 ); // Comment or un-comment on need basis
#endif
    bool bServerMode = false, bDaemon = false, bVerbose = false;
    string logdir(DSRV_LOG_DIR);
    int opt = -1;
    int dbgLevel = 1;
#if defined(_WIN32)
    //----------------------
    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( iResult != NO_ERROR )
    {
        std::cout << "WSAStartup failed with error: " << iResult << "\n";
        return 1;
    }
#endif

#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN); /* Ignore broken pipe */
#endif /* SIGPIPE */

    string sHome = getenv("HOME");
    bool bJustKill = false;
#if defined(_WIN32)
    // No run as daemon possible 
    std::string options = "H:l:vskhg:";
#else
    std::string options = "H:l:vsdkhg:";
#endif
    GetOpt getopt( argc, argv, options );
    while((opt = getopt()) != -1) {
        switch(opt) {
        case 'H':
            sHome = string(getopt.get());
            std::replace( std::begin( sHome ), std::end( sHome ), '\\', '/' );
            if( !sHome.empty() && sHome.back() != '/') {
                sHome += "/";
            }
            break;
        case 'l':
            logdir = string(getopt.get());
            break;
        case 'v':
            bVerbose = true;
            break;
        case 's':
            bServerMode = true;
            break;
        case 'd':
            bDaemon = true;
            break;
        case 'k':
            bJustKill = true;
            break;
        case 'g':
            dbgLevel = atoi( getopt.get().c_str() );
            break;
        case 'h':
        default:
            std::cout << "usage: " << " [-H HOMEDIR] [-l LOGDIR] [-v] [-h]\n" ;
            std::cout << "      -H HOMEDIR assume the home directory to be HOMEDIR.\n"
                      << "                  The lockfile will be written there.\n";
            std::cout << "      -l LOGDIR  sets the directory to write logfiles\n";
            std::cout << "      -v         activates verbose mode\n";
#if !defined(_WIN32)
            std::cout << "      -d         start as a daemon (not interactive) (EXPERIMENTAL)\n";
            std::cout << "      -s         start without REPL. Implied by -d.\n";
#endif
            std::cout << "      -k         shut down the active daemon (TODO)\n";
            std::cout << "      -g LEVEL   sets the gravity level of the logger\n";
            std::cout << "      -h         shows this help message\n";
            exit(EXIT_SUCCESS);
            break;
        }
    }

    if(bDaemon) bServerMode = true;
    string sLockFile = sHome + "/" + LOCK_FILE;

    if(bJustKill) {
        trytokill(sLockFile.c_str());
        exit(EXIT_SUCCESS);
    }

    // Lockfile checking to start only once.
    // =====================================
    int lfp = -1;

#if defined(_DEBUG) && defined(WIN32)
#else
    // Check the lockfile
    lfp = open(sLockFile.c_str(),O_RDONLY);

    if(lfp >= 0) {
        std::cout << "A lockfile already exists, this usually means that"
               " the server is already started. Please first stop the"
               " server using the -k switch or delete the lockfile if"
               " you are sure the server is not running.\n";
        exit(EXIT_FAILURE);
    } 

    // Create the lockfile.
    lfp = open(sLockFile.c_str(),O_RDWR|O_CREAT,0640);
    if ( lfp < 0 ) {
        std::cerr << "unable to create lock file " << sLockFile << " (" << strerror(errno) << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    close( lfp );
#endif

    // Logging and daemonizing.
    // ========================
    new Server(logdir, bVerbose, dbgLevel);
    std::ofstream * outs = nullptr;
    if( bDaemon ) {
        outs = new std::ofstream();
        outs->open(Server::getSingleton().getLogfilename().c_str(), ios::ate);
    }

    if( !FTS::NetworkLibInit( dbgLevel , outs) ) {
        std::cout << "Fatal Error: Can't initialize the FTS network library. Abort\n";
        delete Server::getSingletonPtr();
        delete outs;
        exit( EXIT_FAILURE );
    }

    // Daemonize if wanted.
    if(bDaemon)
        daemonize(sLockFile.c_str(), sHome.c_str());

    // Database and time init.
    // =======================
    srand( (unsigned) time( NULL ) );
    setlocale( LC_ALL, "C" );

    // Try to connect to the mysql database.
    if(ERR_OK != DataBase::initUniqueDB()) {
        // We still need to remove the lockfile.
        FTSMSGDBG("Removing the lockfile "+sLockFile+".\n", 1);
        if(0 != remove(sLockFile.c_str())) {
            FTSMSG("Error removing the lockfile "+sLockFile+
                   ". If the file still exists, you should try to remove "
                   "it by hand, so the server will start next time.\n", MsgType::Error);
        }

        FTSMSGDBG("Everything done, bye\n", 1);
        return -1;
    }

    new ChannelManager();
    ChannelManager::getManager()->init();
    new ClientsManager();
    new GameManager();

    // Begin to listen on all ports.
    // =============================

    // Create a thread waiting on each port. Including the fts port: 0xAF75 :)
    // DEBUG: the +1 at the end.
    std::vector<std::thread> threads;

    for(uint16_t i = DSRV_PORT_FIRST; i < DSRV_PORT_LAST + 1; i++)
        threads.emplace_back( connectionListener, i );

    // Don't read stdin if not in interactive mode (such as being a daemon).
    if( bServerMode ) {
        std::unique_lock<std::mutex> lk(mtxExit);
        cvExit.wait(lk);
    } else {
        cmdline();
    }

    // Now all is lost, nobody likes fts anymore :( clean up.

    FTSMSGDBG("Waiting for every thread to close ...", 1);
    int i = 0;
    for( auto& thread : threads ) {
        thread.join();
        FTSMSGDBG( "Thread on port 0x" + toString( DSRV_PORT_FIRST + i++, -1, '0', std::ios::hex ) + " successfully closed.", 1 );
    }

    // Shutdown all connectons to all clients that still exist.
    FTSMSGDBG("All threads successfully closed, waiting for all clients to shutdown.", 1);
    ClientsManager::deinit();

    FTSMSGDBG("All clients successfully shot down, waiting for all games to shutdown.", 1);
    GameManager::deinit();

    FTSMSGDBG("All games successfully shot down, waiting for all channels to shutdown.", 1);
    ChannelManager::deinit();
    FTSMSGDBG("All channels successfully shut down.", 1);

    DataBase::deinitUniqueDB();

    // We still need to remove the lockfile.
    FTSMSGDBG("Removing the lockfile "+sLockFile+".\n", 1);
    if(0 != remove(sLockFile.c_str())) {
        FTSMSG("Error removing the lockfile "+sLockFile+
               ". If the file still exists, you should try to remove "
               "it by hand, so the server will start next time.\n", MsgType::Error);
        FTSMSG( "The Error Text is " + std::string( strerror( errno ) ), MsgType::Error );
    }

    FTSMSGDBG("Everything done, bye\n", 1);
    delete Server::getSingletonPtr();
    delete outs;
    return EXIT_SUCCESS;
}

void cmdline()
{
    while( !g_bExit ) {
        
        std::cout << "> ";
        
        string s;
        getline(cin, s);
        auto linebuf = s.erase(0, s.find_first_not_of(" \t\n"));
        stringstream sb(linebuf);
        string cmd;

        while( getline(sb, cmd, ' ') ) {
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower); // Thanks to SO
            // Parse it !
            if( cmd == "help" ) {
                auto arg = getNextToken(sb);
                std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
                help(arg);
            } else if( cmd == "exit" ) {
                g_bExit = true;
                break;
            } else if( cmd == "nplayers" ) {
                size_t nPlayers = FTSSrv2::Server::getSingletonPtr()->getPlayerCount();
                std::cout << "Number of players that are logged in: " << toString(nPlayers) << std::endl;
            } else if( cmd == "ngames" ) {
                size_t nGames = FTSSrv2::Server::getSingletonPtr()->getGameCount();
                std::cout << "Number of games that are opened: " << toString(nGames) << std::endl;
            } else if( cmd == "version" ) {
                FTSMSGDBG("The version of the server is " D_SERVER_VERSION_STR, 1);
            } else if( cmd == "verbose" ) {
                auto arg = getNextToken(sb);
                std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower); // Thanks to SO
                if( arg == "on" ) {
                    bool bOld = FTSSrv2::Server::getSingletonPtr()->setVerbose(true);
                    std::cout << "Verbose mode was " << string(bOld ? "on" : "off") << ", now it is on." << std::endl;
                } else if( arg == "off" ) {
                    bool bOld = FTSSrv2::Server::getSingletonPtr()->setVerbose(false);
                    std::cout << "Verbose mode was " << string(bOld ? "on" : "off") << ", now it is off." << std::endl;
                } else {
                    bool b = FTSSrv2::Server::getSingletonPtr()->getVerbose();
                    std::cout << "Verbose mode is currently " << string(b ? "on" : "off") << std::endl;
                }
            } else if( cmd == "stats" ) {
                printServerStats();
            } else if( cmd == "clearstats" ) {
                FTSSrv2::Server::getSingletonPtr()->clearStats();
            } else {
                std::cout << "Unknown command '" << string(cmd) << ", try typing 'help' to get some help." << std::endl;
            }
        }
    }
}

void printServerStats()
{
    auto totals = FTSSrv2::Server::getSingletonPtr()->getStatTotalPackets();
    FTSMSGDBG( " ", 1 );
    std::cout << "Req No    Snd  Recv\n";
    std::cout << "---------+----+----+\n";
    for( const auto& kv : totals ) {
        std::cout << "req " << toString((int) kv.first, 2, ' ') << "   |" << toString(kv.second.first, 4, ' ') << "|"<< toString(kv.second.second, 4, ' ') << "|\n";
    }
    std::cout << "---------+----+----+\n";
    using kvstat = std::pair<master_request_t, std::pair<uint64_t, uint64_t>>;
    auto totalSend = std::accumulate( std::begin( totals ), std::end( totals ), 0ULL, [] ( uint64_t sum, const kvstat& p ) { return sum + p.second.second; } );
    auto totalRecv = std::accumulate( std::begin( totals ), std::end( totals ), 0ULL, [] ( uint64_t sum, const kvstat& p ) { return sum + p.second.first; } );
    std::cout << "Totals   |" << toString(totalSend, 4, ' ') << "|" << toString(totalRecv, 4, ' ') << "|\n";
}

// Display some help.
void help(const string& topic)
{
    if(topic == "help") {
        std::cout << "Just type help once, It won't help to type help more often :p\n";
    } else if(topic == "exit") {
        std::cout << "SYNTAX:\n";
        std::cout << "\texit\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis will close the connection to all clients and shutdown the server.\n";
        std::cout << "Normally, every client should get a warning message that the server has been shutdown.\n";
        std::cout << "\n";
    } else if( topic == "stats" ) {
        std::cout << "SYNTAX:\n";
        std::cout << "\tstats\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis will show the statistics about request packets.\n";
        std::cout << "\n";
    } else if( topic == "clearstats" ) {
        std::cout << "SYNTAX:\n";
        std::cout << "\tclearstats\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis will clear all accumulated request packets statistics.\n";
        std::cout << "\n";
    } else if( topic == "nplayers" ) {
        std::cout << "SYNTAX:\n";
        std::cout << "\tnplayers\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis just prints out how much players are actually connected.\n";
        std::cout << "The players that are connected, but not logged in are also counted here.\n";
        std::cout << "You can always see this in the file " DSRV_FILE_NPLAYERS".\n";
        std::cout << "\n";
    } else if(topic == "ngames") {
        std::cout << "SYNTAX:\n";
        std::cout << "\tngames\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis just prints out how much games are actually opened.\n";
        std::cout << "The players that are opened, but not started yet are also counted here.\n";
        std::cout << "You can always see this in the file " DSRV_FILE_NGAMES".\n";
        std::cout << "\n";
    } else if(topic == "version") {
        std::cout << "SYNTAX:\n";
        std::cout << "\tversion\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis just prints out the version of the server.\n";
    } else if(topic == "verbose") {
        std::cout << "SYNTAX:\n";
        std::cout << "\tverbose [on|off]\n";
        std::cout << "\n";
        std::cout << "DESC:\n";
        std::cout << "\tThis changes the verbosity mode of the server.\n";
        std::cout << "If verbosity is off, the server only tells you about the errors\n";
        std::cout << "and loggs all the rest into the logfile. If verbosity is on,\n";
        std::cout << "the server tells you everything that happens too.\n";
        std::cout << "If no argument is given, it prints the current verbosity state.\n";
        std::cout << "\n";
    } else {
        std::cout << "Type: help [command], where command is one of the followings:\n";
        std::cout << "\n";
        std::cout << "  exit     shuts down this server.\n";
        std::cout << "  version  see the server-version.\n";
        std::cout << "  nplayers see the number of players.\n";
        std::cout << "  ngames   see the number of games.\n";
        std::cout << "  spam     start/stop a spam bot in the main channel.\n";
        std::cout << "  verbose  let me talk much or not.\n";
        std::cout << "  stats    show some statistics.\n";
        std::cout << "  clearstats deletes the statistics.\n";
        std::cout << "\n";
        std::cout << "FILES:\n";
        std::cout << "All files are located in the current working directory.\n";
        std::cout << "  " << FTSSrv2::Server::getSingletonPtr()->getLogfilename() << " contains a lot of logging messages." << std::endl;
        std::cout << "  " << FTSSrv2::Server::getSingletonPtr()->getErrfilename() << " contains all error messages that happened." << std::endl;
        std::cout << "  " << FTSSrv2::Server::getSingletonPtr()->getPlayersfilename() << " contains the number of players actually connected." << std::endl;
        std::cout << "  " << FTSSrv2::Server::getSingletonPtr()->getGamesfilename() << " contains the number of games actually opened." << std::endl;
    }
}

// This sets up everything to listen on a certain port, and then goes listen.
void connectionListener(uint16_t in_iPort)
{
    auto startClient = [in_iPort](FTS::Connection* pCon) 
    {
        pCon->setMaxWaitMillisec( 100 ); // Set standard connection time out to 100 ms.
        Client *pCli = ClientsManager::getManager()->createClient( pCon );
        if( pCli == nullptr ) {
            FTSMSG( "[ERROR] Can't create client, may be it exists already. Port<" + toString( (int) in_iPort, 0, ' ', std::ios::hex ) + "> con<" + toString( (const uint64_t) pCon, 4, '0', std::ios_base::hex ) + ">", MsgType::Error );
        }

        FTSMSGDBG( "Accept connection on port 0x" + toString( (int) in_iPort, 0, ' ', std::ios::hex ) + " client<" + toString( (const uint64_t) pCli, 4, '0', std::ios_base::hex ) + "> con<" + toString( (const uint64_t) pCon, 4, '0', std::ios_base::hex ) + ">", 4 );

        // And start a new thread for him.
        auto thr = std::thread( Client::starter, pCli );
        thr.detach();
    };

    auto pWaiter = std::unique_ptr<FTS::ConnectionWaiter>( FTS::ConnectionWaiter::create(FTS::ConnectionWaiter::ConnectionType::SOCKET) );

    if( ERR_OK != pWaiter->init(in_iPort, startClient) )
        return ;
    
    // wait for connections unless we need to quit.
    while(!g_bExit) {

        // Wait for a connection&packet for 1000 ms, if none is got,
        // wait a bit to avoid megaload of cpu. 100 microsec = 0.1 millisec.
        pWaiter->waitForThenDoConnection(1000);
        std::this_thread::sleep_for( std::chrono::microseconds(100) );
    }

}

static void child_handler(int signum)
{
#if !defined(_WIN32)
    switch(signum) {
    case SIGALRM: exit(EXIT_FAILURE); break;
    case SIGUSR1: exit(EXIT_SUCCESS); break;
    case SIGUSR2: g_bExit = true; cvExit.notify_all(); break;
    case SIGCHLD: exit(EXIT_FAILURE); break;
    }
#endif
}

// From: http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize
static void daemonize( const char *lockfile, const char *dir )
{
#if !defined(_WIN32)
    pid_t pid, sid, parent;

    /* already a daemon */
    if ( getppid() == 1 ) {
        FTSMSG("Already a daemon?", MsgType::Error);
        return;
    }

    /* Drop user if there is one, and we were run as root */
    if ( getuid() == 0 || geteuid() == 0 ) {
        struct passwd *pw = getpwnam(DSRV_ROOT_IS_BAD);
        if ( pw ) {
            FTSMSGDBG("setting user to " DSRV_ROOT_IS_BAD "\n", 1);
            setuid( pw->pw_uid );
        }
    }

    /* Trap signals that we expect to recieve */
    signal(SIGCHLD,child_handler);
    signal(SIGUSR1,child_handler);
    signal(SIGUSR2,child_handler);
    signal(SIGALRM,child_handler);

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        FTSMSG("unable to fork daemon, ("+string(strerror(errno))+")\n", MsgType::Error);
        exit(EXIT_FAILURE);
    }

    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {

        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
           for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();

        exit(EXIT_FAILURE);
    }

    /* At this point we are executing as the child process */
    parent = getppid();

    /* Cancel certain signals */
    signal(SIGCHLD,SIG_DFL); /* A child process dies */
    signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        FTSMSG("unable to create a new session, ("+string(strerror(errno))+")\n", MsgType::Error);
        exit(EXIT_FAILURE);
    }
   
    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir(dir)) < 0) {
        FTSMSG("unable to change directory to " + string(dir) + ", ("+string(strerror(errno))+")\n", MsgType::Error);
        exit(EXIT_FAILURE);
    }

    /* Write the pid to the lockfile. */
    FILE *pFile = fopen(lockfile, "w+");
    fprintf(pFile, "%d", getpid());
    fflush(pFile);
    fclose(pFile);

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    /* Tell the parent process that we are A-okay */
    kill( parent, SIGUSR1 );
#endif
}

static void trytokill(const char *lockfile)
{
#if !defined(_WIN32)
    if(lockfile == NULL || lockfile[0] == '\0')
        return;

    // Get the PID of the server.
    int pid;
    FILE *pFile = fopen(lockfile, "r");
    if(!pFile) {
        printf("Could not stop the server, as the lockfile %s does not exist !"
               " Bye.\n", lockfile);
        printf("This could also mean that the server is not running.\n");
        return ;
    }
    if(EOF == fscanf(pFile, "%d", &pid)) {
        printf("The lockfile %s is empty (does not contain the pid) ! Just deleting it.\n", lockfile);
        fclose(pFile);
        if(0 != remove(lockfile)) {
            printf("Removing the lockfile did not work, thus you have a problem now.\n");
        } else {
            printf("Removing the lockfile did work. Everything should be fine now.\n");
        }
        return ;
    }
    fclose(pFile);

    printf("The server (PID: %d) is shutting down, waiting for it to be done ... \n", pid);
    fflush(stdout);

    // Tell the server to quit.
    if(0 != kill(pid, SIGUSR2)) {
        if(errno == ESRCH) {
            printf("There is one problem, I see two reasons for it:\n");
            printf("  * Either the daemon is no more running but the lockfile\n"
                   "    %s still exists.\n"
                   "    I Will try to delete it, if that succeeds, everything is ok.\n", lockfile);
            printf("  * Or either the daemon has changed its PID (very strange)\n"
                   "    or the lockfile %s has been modified.\n"
                   "    You should check if the daemon is still running.\n"
                   "    by typing \"ps aux | grep fts\". And kill it manually\n"
                   "    by typing \"kill -12 [THE PID]\". Good Luck !\n", lockfile);
            printf("Trying to remove the lockfile anyway ...\n");
            if(0 != remove(lockfile)) {
                printf("Removing the lockfile did not work, thus you have a problem now.\n");
            } else {
                printf("Removing the lockfile did work. Everything should be fine now (hopefully).\n");
            }
        } else {
            printf("Could not stop the server: %d: %s\n", errno, strerror(errno));
            // Here, we don't remove the lockfile, as the server maybe could not be killed
            // because of permissions and is most likely to still run.
        }
    }

    // Give the server some time ...
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

    printf("If you see any numbers below, the server is still running ! Maybe you just need to give it more time, check 'ps | grep " DAEMON_NAME "' in a few seconds again.\n");
    std::string ps_cmd = "ps | grep " + std::to_string(pid);
    system(ps_cmd.c_str());
    printf("\nIf you didn't see any number one line above, the server has quit successfully.\n");
#endif
}
