//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "client.hpp"

Client::Client(int fd)
{
    this->isConnected = true;
    this->Socket = fd;
}

void Client::Launch()
{
    pthread_create(&this->thread, NULL, this->main, this);
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

    }
}
