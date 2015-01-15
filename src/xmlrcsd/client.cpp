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
#include <sys/socket.h>
#include <arpa/inet.h>
#include "configuration.hpp"
#include "client.hpp"
#include "generic.hpp"

std::vector<Client*> Client::clients;
pthread_mutex_t Client::clients_lock;

Client::Client(int fd)
{
    this->SubscribedAny = false;
    this->isConnected = true;
    this->Socket = fd;
    struct sockaddr_storage addr;
    char ipstr[200];
    socklen_t len = sizeof(addr);
    struct sockaddr_in *s = (struct sockaddr_in *) &addr;
    getsockname(fd, (struct sockaddr *) &addr, &len);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    this->IP = std::string(ipstr);
    Generic::Log("Incoming connection from " + this->IP);
}

Client::~Client()
{
    pthread_mutex_lock(&Client::clients_lock);
    //delete this
    std::vector<Client*>::iterator position = std::find(clients.begin(), clients.end(), this);
    if (position != clients.end())
        clients.erase(position);
    pthread_mutex_unlock(&Client::clients_lock);
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

std::string Client::ReadLine(bool *error)
{
    std::string line;
    *error = false;
    while (true)
    {
        char buffer[10];
        ssize_t bytes = read(this->Socket, buffer, 10);
        if (bytes < 0)
        {
            // there was some error
            *error = true;
            return line;
        }
        if (!bytes)
        {
            // there is nothing in a buffer yet, so we sleep for a short time and then try again so that CPU is not exhausted
            usleep(2000);
            continue;
        }
        int i = 0;
        while (i < bytes)
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
                return line;
            }
            line += buffer[i];
            i++;
        }
    }
    return line;
}

static std::string mker(std::string text)
{
    return std::string("<error>") + text + std::string("</error>");
}

static std::string Sanitize(std::string name)
{
    if (name.size() == 0)
        return name;
    char last = name[name.size() - 1];
    while (name.size() > 0 && (last == '\n' || last == '\r'))
    {
        // we remove the last character because it's causing problems
        name = name.substr(0, name.size() - 1);
    }
    return name;
}

bool Client::IsSubscribed(std::string site)
{
    pthread_mutex_lock(&this->subscriptions_lock);
    std::vector<std::string>::iterator position = std::find(this->Subscriptions.begin(),
                                                            this->Subscriptions.end(),
                                                            site);
    bool result = (position != this->Subscriptions.end());
    pthread_mutex_unlock(&this->subscriptions_lock);
    return result;
}

int Client::Subscribe(std::string wiki)
{
    if (wiki == "all")
    {
        if (this->SubscribedAny)
            return EALREADYEXIST;
        this->SubscribedAny = true;
        return 0;
    }
    wiki = Sanitize(wiki);
    if (this->Subscriptions.size() > MAX_SUBSCRIPTIONS)
        return ETOOMANYSUBS;
    if (wiki.length() == 0 || wiki.length() > MAX_SUBSCR_SIZE)
        return EINVALID;
    if (this->IsSubscribed(wiki))
        return EALREADYEXIST;
    pthread_mutex_lock(&this->subscriptions_lock);
    this->Subscriptions.push_back(wiki);
    pthread_mutex_unlock(&this->subscriptions_lock);
    return 0;
}

int Client::Unsubscribe(std::string wiki)
{
    if (wiki == "all")
    {
        if (!this->SubscribedAny)
            return ENOTEXIST;
        this->SubscribedAny = false;
        return 0;
    }
    wiki = Sanitize(wiki);
    if (wiki.length() == 0)
        return EINVALID;
    pthread_mutex_lock(&this->subscriptions_lock);
    std::vector<std::string>::iterator position = std::find(this->Subscriptions.begin(),
                                                            this->Subscriptions.end(),
                                                            wiki);
    if (position == this->Subscriptions.end())
        return ENOTEXIST;
    this->Subscriptions.erase(position);
    pthread_mutex_unlock(&this->subscriptions_lock);
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
        bool er;
        std::string line = _this->ReadLine(&er);
        if (er || line == "exit")
        {
            close(_this->Socket);
            goto exit;
        }
        if (line == "version")
            _this->SendLine(std::string("<versioninfo>") + Configuration::version + std::string("</versioninfo>"));
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
                        _this->SendLine(mker("This is not a valid wiki name"));
                        break;
                    case EALREADYEXIST:
                        _this->SendLine(mker("You are already subscribed to this one"));
                        break;
                    case ETOOMANYSUBS:
                        _this->SendLine(mker("You subscribed to too many wikis now"));
                        break;
                }
            } else
            {
                _this->SendLine(RESP_OK);
            }
        }
        else if (line[0] == 'D' && line[1] == ' ')
        {
            // user wants to unsubscribe to a wiki
            std::string wiki = line.substr(2);
            while (wiki[0] == ' ')
                wiki = wiki.substr(1);
            int result = _this->Unsubscribe(wiki);
            if (result)
            {
                switch(result)
                {
                    case EINVALID:
                        _this->SendLine(mker("This is not a valid wiki name"));
                        break;
                    case ENOTEXIST:
                        _this->SendLine(mker("You are not subscribed to this one"));
                        break;
                }
            } else
            {
                _this->SendLine(RESP_OK);
            }
        }
        else if (line == "clear")
        {
            pthread_mutex_lock(&_this->subscriptions_lock);
            _this->Subscriptions.clear();
            pthread_mutex_unlock(&_this->subscriptions_lock);
            _this->SendLine(RESP_OK);
        }
        else if (line == "ping")
        {
            // reply
            _this->SendLine("<pong></pong>");
        }
        else if (line == "stat")
        {
            // Write some statistics to user
            std::string uptime = std::string("<stat>uptime since: ") + retrieve_uptime() + "</stat>";
            _this->SendLine(uptime);
        } else
        {
            _this->SendLine(mker(std::string("Unknown: ") + line));
        }
    }
    exit:
        Generic::Log("Connection closed: " + _this->IP);
        delete _this;
        return NULL;
}
