//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <iostream>
#include <syslog.h>
#include "configuration.hpp"
#include "generic.hpp"

void Generic::Log(std::string text)
{
    if (!Configuration::daemon)
        std::cout << text << std::endl;
    else
        syslog(LOG_INFO, "%s", text.c_str());
}
