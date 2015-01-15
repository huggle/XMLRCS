//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <stdexcept>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include "configuration.hpp"
#include "server.hpp"

Server::Server()
{
    this->Port = Configuration::port;
    this->ListenerFd = 0;
    this->open = false;
}

bool Server::IsListening()
{
    return this->open;
}

void Server::Listen()
{
    if (this->IsListening())
        throw std::runtime_error("This listener is already open");
    this->open = true;

    this->ListenerFd = socket(AF_INET, SOCK_STREAM, 0);

    if (this->ListenerFd < 0)
        throw std::runtime_error("Unable to create a socket for listener");

    sockaddr_in svrAdd, clntAdd;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_family = AF_INET;
    svrAdd.sin_port = htons(this->Port);
    //bind socket
    if (bind(this->ListenerFd, (sockaddr *)&svrAdd, sizeof(svrAdd)) < 0)
        throw std::runtime_error(std::string("Unable to bind socket: ") + strerror(errno));
    listen(this->ListenerFd, 5);
    socklen_t len = sizeof(clntAdd);
    while(this->IsListening())
    {
        int connFd = accept(this->ListenerFd, (struct sockaddr *)&clntAdd, &len);
        if (connFd < 0)
        {
            std::cerr << "Cannot accept connection" << std::endl;
            continue;
        }
        Client *client = new Client(connFd);
        pthread_mutex_lock(&Client::clients_lock);
        Client::clients.push_back(client);
        pthread_mutex_unlock(&Client::clients_lock);
        client->Launch();
    }
}
