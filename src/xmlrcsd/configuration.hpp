//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>
#include <time.h>

class Configuration
{
    public:
        static const std::string version;
        static bool         daemon;
        static int          port;
        static int          last_io;
        static std::string  redis_host;
        static unsigned int redis_port;
        static std::string  redis_pref;
        static int          debugging_level;
        static unsigned long  total_conn;
        static unsigned long  total_io;
        static time_t       startup_time;
        static bool         auto_kill;
};

#endif
