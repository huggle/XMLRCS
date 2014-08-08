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
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include "configuration.hpp"
#include "server.hpp"
#include "hiredis/hiredis.h"

#define UNUSED(x) (void)x

bool IsRunning = true;

void Log(std::string text)
{
    if (!Configuration::daemon)
        std::cout << text << std::endl;
    else
        syslog(LOG_INFO, text.c_str());
}

redisContext *Redis_Connect()
{
    struct timeval timeout = { 1, 500000 };
    redisContext * redis_context = redisConnectWithTimeout(Configuration::redis_host.c_str(),
                                                           (int)Configuration::redis_port,
                                                           timeout);
    if (!redis_context)
    {
        throw std::runtime_error("Redis connection returned NULL");
    }
    else if (redis_context->err)
    {
        std::string error(redis_context->errstr);
        redisFree(redis_context);
        throw std::runtime_error(std::string("Error while connecting to redis: ") + error);
    }
    return redis_context;
}

void *Listen(void *threadid)
{
    UNUSED(threadid);
    try
    {
        // open a listener
        Server server;
        Log("Opened a listener");
        server.Listen();
    } catch (std::runtime_error exception)
    {
        Log(std::string("FATAL ERROR: ") + exception.what());
    }
    IsRunning = false;
    return NULL;
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc > 1 && std::string("-d") == argv[1])
        {
            Configuration::daemon = true;
            pid_t pid = fork();
            if (pid < 0)
            {
                Log("Failed to daemonize itself call to fork() failed");
                return 80;
            }
            // we successfuly forked
            if (pid > 0)
                return 0;
        }
        Log(std::string("Starting up XMLRCS version ") + Configuration::version);
        pthread_t listener;
        pthread_create(&listener, NULL, Listen, (void*)NULL);
        // we need to init redis now
        redisContext *redis_context;
        while (IsRunning)
        {
            // we need to keep redis alive in a loop
            try
            {
                redis_context = Redis_Connect();
                Log("Successfuly connected to redis server");
                while (IsRunning)
                    usleep(100000);
                break;
            } catch (std::runtime_error exception)
            {
                Log(std::string("ERROR: ") + exception.what());
                Log("Reconnecting to redis in 10 seconds");
                usleep(10000000);
            }
        }
        // delete redis object
        redisFree(redis_context);
        Log("Terminated");
        return 0;
    } catch (std::runtime_error exception)
    {
        Log(std::string("FATAL ERROR: ") + exception.what());
    }
    return 2;
}

