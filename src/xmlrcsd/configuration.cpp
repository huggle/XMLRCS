//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "configuration.hpp"

const std::string Configuration::version = "1.0.5";
bool         Configuration::daemon = false;
std::string  Configuration::redis_host = "localhost";
unsigned int Configuration::redis_port = 6379;
unsigned long Configuration::total_conn = 0;
unsigned long Configuration::total_io = 0;
int          Configuration::last_io = 0;
std::string  Configuration::redis_pref = "";
bool         Configuration::auto_kill = true;
int          Configuration::debugging_level = 0;
int          Configuration::port = 8822;
time_t       Configuration::startup_time;
