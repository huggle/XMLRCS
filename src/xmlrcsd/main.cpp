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
#include <time.h>
#include "configuration.hpp"
#include "client.hpp"
#include "tp.hpp"
#include "streamitem.hpp"
#include "generic.hpp"
#include "server.hpp"
#include "hiredis/hiredis.h"

#define UNUSED(x) (void)x

using namespace Generic;

bool IsRunning = true;

void *Killer(void *null)
{
    (void)null;
    while (IsRunning)
    {
            int last_check = time(NULL);
            usleep(10000000);
        start:
            pthread_mutex_lock(&Client::clients_lock);
            Generic::Debug("Checking clients who timed out");
            for (std::vector<Client*>::size_type i = 0; i != Client::clients.size(); i++)
            {
                if ((Client::clients[i]->LastPing + 80) < last_check)
                {
                    Generic::Log(std::string("Client ") + Client::clients[i]->IP + " timed out - removing them");
                    // we let the unlock up on Kill so that we ensure that client doesn't get killed meanwhile
                    Client::clients[i]->Kill(true);
                    // we need to go back to start of first loop, since the whole iteration now is unsafe
                    goto start;
                }
                Client::clients[i]->SendLine("<ping></ping>");
            }
            Generic::Debug("Finished checking of clients");
            pthread_mutex_unlock(&Client::clients_lock);
    }
    return NULL;
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
        TP *term = new TP(argc, argv);
        if (!term->ProcessArgs())
        {
            delete term;
            return 0;
        }
        delete term;
        if (Configuration::daemon)
        {
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
        Configuration::last_io = time(0);
        Configuration::startup_time = time(0);
        Log(std::string("Starting up XMLRCS version ") + Configuration::version);
        pthread_t killer;
        pthread_create(&killer, NULL, Killer, (void*)NULL);
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
                redisReply *reply;
                while (IsRunning)
                {
                    // get a rc message from redis and store it into internal buffer for later processing
                    reply = (redisReply*)redisCommand(redis_context, "RPOP rc");
                    if (reply->len > 0)
                    {
                        // Update last io time
                        Configuration::last_io = time(0);
                        StreamItem::ProcessItem(std::string(reply->str));
                    }
                    else
                    {
                        if (time(0) > Configuration::last_io + 12)
                        {
                            Configuration::last_io = time(0);
                            pthread_mutex_lock(&Client::clients_lock);
                            for (std::vector<Client*>::size_type i = 0; i != Client::clients.size(); i++)
                            {
                                Client::clients[i]->SendLine("<fatal>redis is empty for 10 seconds</fatal>");
                            }
                        }
                        pthread_mutex_unlock(&Client::clients_lock);
                        usleep(100000);
                    }
                    freeReplyObject(reply);
                }
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

