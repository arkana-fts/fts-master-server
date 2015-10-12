#include <stdio.h>  // EOF const

#include "GetOpt.h"


GetOpt::GetOpt( int argc, char* argv[], const std::string optstring ) : index( 1 ), argc( argc ), argv( argv ), optstring( optstring )
{
}

char GetOpt::operator()()
{
    optarg.empty();
    errtext.empty();

    // Is first character of option string a ':'
    if ( optstring[0] == ':' )
    {
        errtext = argv[0] + std::string( ": missing option argument in " ) + optstring + "\n";
        optstring.erase( 0, 1 );
        return ':';
    }

    // Is end of arg list? or not an option or empty option 
    if ( index >= argc )
        return EOF;

    // Is first character of option a ':'
    if ( argv[index][0] == ':' )
    {
        errtext = argv[0] + std::string( ": unknown option -" ) + argv[index][0] + "\n";
        index++;
        return ':';
    }

    // Is end of arg list? or empty option 
    if ( argv[index][0] != '-' || argv[index][1] == '\0' )
        return EOF;

    // Skip '-'
    auto scan = argv[index] + 1;
    index++;

    // Is current character in the option string 
    char c = *scan++;
    auto place = optstring.find_first_of( c );
    if ( place == std::string::npos || c == ':' )
    {
        errtext = argv[0] + std::string( ": unknown option -" ) + c + "\n";
        return '?';
    }
    
    // Check if an additional argument is needed.
    place++;
    if ( optstring[place] == ':' )
    {
        // Check if no space is between option and its argument.
        if ( *scan != '\0' )
        {
            optarg = scan;
        }
        else if ( index < argc )
        {
            if ( argv[index][0] != '-' )
            {
                optarg = argv[index];
                index++;
            }
            else
            {
                errtext = argv[0] + std::string( ": option requires argument -" ) + c + "\n";
                return ':';
            }
        }
        else
        {
            errtext = argv[0] + std::string( ": option requires argument -" ) + c + "\n";
            return ':';
        }
    }
    return c;
}

