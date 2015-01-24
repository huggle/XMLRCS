//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <string>
#include "configuration.hpp"
#include "tp.hpp"

TP::TP(int _argc, char **_argv)
{
    this->argc = _argc;
    this->argv = _argv;
}

static void Debug()
{
    Configuration::debugging_level++;
}

bool TP::ProcessArgs()
{
    int x = 0;
    while (x < this->argc)
    {
        std::string parameter = this->argv[x];
        x++;
        if (parameter.size() > 1 && parameter[0] == '-' && parameter[1] != '-')
        {
            unsigned int b = 1;
            while (b < parameter.size())
            {
                char xx = parameter[b++];
                switch (xx)
                {
                    case 'd':
                        Configuration::daemon = true;
                        break;
                    case 'v':
                        Debug();
                        break;
                }
            }
        }
    }
    return true;
}
