#ifndef GENERIC_HPP
#define GENERIC_HPP

#include <sstream>
#include <string>

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

namespace Generic
{

}

#endif // GENERIC_HPP
