//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <algorithm>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "configuration.hpp"
#include "client.hpp"
#include "generic.hpp"

std::vector<Client*> Client::clients;

Client::Client(int fd)
{
    this->isConnected = true;
    this->Socket = fd;
}

Client::~Client()
{
    //delete this
    std::vector<Client*>::iterator position = std::find(clients.begin(), clients.end(), this);
    if (position != clients.end())
        clients.erase(position);
}

void Client::Launch()
{
    pthread_create(&this->thread, NULL, this->main, this);
}

void Client::SendLine(std::string line)
{
    if (!this->isConnected)
        return;
    if (line[line.length() - 1] != '\n')
        line += "\r\n";
    write(this->Socket, line.c_str(), line.length());
}

std::string Client::ReadLine()
{
    std::string line;
    bool reading = true;
    while(reading)
    {
        char buffer[10];
        read(this->Socket, buffer, 9);
        int i = 0;
        while (i < 10)
        {
            if (buffer[i] == '\r')
            {
                i++;
                continue;
            }
            if (buffer[i] == '\0')
            {
                // end of string reached
                break;
            }
            if (buffer[i] == '\n')
            {
                reading = false;
                return line;
            }
            line += buffer[i];
            i++;
        }
    }
    return line;
}

int Client::Subscribe(std::string wiki)
{
    if (wiki.length() == 0)
        return EINVALID;
    std::vector<std::string>::iterator position = std::find(this->Subscriptions.begin(),
                                                            this->Subscriptions.end(),
                                                            wiki);
    if (position != this->Subscriptions.end())
        return EALREADYEXIST;
    this->Subscriptions.push_back(wiki);
    return 0;
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string retrieve_uptime()
{
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&Configuration::startup_time);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    time_t diff = time(0) - Configuration::startup_time;
    std::string up(buf);
    up += " (";
    up += SSTR(diff);
    up += " seconds)";
    return up;
}

void *Client::main(void *self)
{
    Client *_this = (Client*)self;
    while (_this->isConnected)
    {
        std::string line = _this->ReadLine();
        if (line == "exit")
        {
            close(_this->Socket);
            goto exit;
        }
        if (line == "version")
            _this->SendLine(Configuration::version);
        else if (line[0] == 'S' && line[1] == ' ')
        {
            // user wants to subscribe to a wiki
            std::string wiki = line.substr(2);
            while (wiki[0] == ' ')
                wiki = wiki.substr(1);
            int result = _this->Subscribe(wiki);
            if (result)
            {
                switch(result)
                {
                    case EINVALID:
                        _this->SendLine("ERROR: This is not a valid wiki name");
                        break;
                    case EALREADYEXIST:
                        _this->SendLine("ERROR: You are already subscribed to this one");
                        break;
                }
            } else
            {
                _this->SendLine("OK");
            }
        }
        else if (line == "stat")
        {
            // Write some statistics to user
            std::string uptime = std::string("uptime since: ") + retrieve_uptime();
            _this->SendLine(uptime);
        }
    }
    exit:
        delete _this;
        return NULL;
}
