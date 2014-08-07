//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <iostream>
#include <string>
#include <stdexcept>
#include "configuration.hpp"
#include "hiredis/hiredis.h"

void Log(std::string text)
{
    std::cout << text << std::endl;
}

redisContext *Redis_Connect()
{
    struct timeval timeout = { 1, 500000 };
    redisContext * redis_context = redisConnectWithTimeout(redis_host.c_str(), (int)redis_port, timeout);
    if (!redis_context)
    {
        throw std::runtime_error("Redis connection returned NULL");
    }
    else if (redis_context->err)
    {
        throw std::runtime_error(std::string("Error while connecting to redis: ") + redis_context->errstr);
    }
    return redis_context;
}

int main(int argc, char *argv[])
{
    try
    {
        Log(std::string("Starting up XMLRCS version ") + version);
        // we need to init redis now
        redisContext *redis_context = Redis_Connect();

        // delete redis object
        redisFree(redis_context);
        return 0;
    } catch (std::exception exception)
    {
        Log(std::string("FATAL ERROR: ") + exception.what());
    }
    return 2;
}

