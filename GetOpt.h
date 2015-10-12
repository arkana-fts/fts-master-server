#if !defined(GETOPT_H)
#define GETOPT_H

#include <string>

class GetOpt
{
public:
    GetOpt( int argc, char* argv[], const std::string optstring );
    std::string get() { return optarg; }
    std::string error() { return errtext; }

    char operator()();

private:
    std::string optarg; /* Global argument pointer. */
    int index; /* Global argv index. */
    int argc;
    char** argv;
    std::string optstring;
    std::string errtext;
};
#endif