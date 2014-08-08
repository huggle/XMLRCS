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
    }
    exit:
        delete _this;
        return NULL;
}
