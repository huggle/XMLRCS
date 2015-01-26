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
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "configuration.hpp"
#include "client.hpp"
#include "generic.hpp"

std::vector<Client*> Client::clients;
pthread_mutex_t Client::clients_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned int Client::LastID = 10;
unsigned int Client::UsersCount = 0;

// This thread will take data from a pool of outgoing messages and will send them
static void *SenderThread(void *c)
{
    pthread_detach(pthread_self());
    Client *client = (Client*)c;
    while (client->IsConnected())
    {
        pthread_mutex_lock(&client->OutgoingBuffer_lock);
        if (client->OutgoingBuffer.size() == 0)
        {
            pthread_mutex_unlock(&client->OutgoingBuffer_lock);
            usleep(20000);
            continue;
        }
        std::string line = client->OutgoingBuffer[0];
        client->OutgoingBuffer.erase(client->OutgoingBuffer.begin());
        pthread_mutex_unlock(&client->OutgoingBuffer_lock);
        client->SendLineNow(line);
    }
    client->ThreadRun = false;
    return NULL;
}

Client::Client(int fd)
{
    pthread_mutex_init(&this->OutgoingBuffer_lock, NULL);
    pthread_mutex_init(&this->subscriptions_lock, NULL);
    this->ThreadRun = true;
    this->closed = false;
    this->ThreadRun2 = true;
    this->SubscribedAny = false;
    this->killed = false;
    this->LastPing = time(0);
    this->ID = LastID++;
    this->SID = Generic::IntToStdString(this->ID);
    pthread_mutex_lock(&Client::clients_lock);
    UsersCount++;
    Configuration::total_conn++;
    pthread_mutex_unlock(&Client::clients_lock);
    this->isConnected = true;
    this->Socket = fd;
    struct sockaddr_storage addr;
    char ipstr[200];
    socklen_t len = sizeof(addr);
    struct sockaddr_in *s = (struct sockaddr_in *) &addr;
    pthread_create(&this->sender, NULL, SenderThread, (void*)this);
    getsockname(fd, (struct sockaddr *) &addr, &len);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    this->IP = std::string(ipstr);
    Generic::Log("Incoming connection from " + this->IP);
}

Client::~Client()
{
    pthread_mutex_lock(&Client::clients_lock);
    UsersCount--;
    //delete this
    std::vector<Client*>::iterator position = std::find(clients.begin(), clients.end(), this);
    if (position != clients.end())
        clients.erase(position);
    pthread_mutex_unlock(&Client::clients_lock);
    // ensure that thread will terminate
    this->isConnected = false;
    Generic::Debug(this->SID + " Threads wait");
    // we must wait for thread to finish, otherwise we get a segfault, or there is a high chance for that
    while (this->ThreadRun)
        usleep(200);
    Generic::Debug(this->SID + " Thread 1 finished");
    while (this->ThreadRun2)
        usleep(200);
    Generic::Debug(this->SID + " Thread 2 finished");
    this->OutgoingBuffer.clear();
}

void Client::Launch()
{
    pthread_create(&this->thread, NULL, this->main, this);
}

void Client::SendLine(std::string line)
{
    pthread_mutex_lock(&this->OutgoingBuffer_lock);
    this->OutgoingBuffer.push_back(line);
    pthread_mutex_unlock(&this->OutgoingBuffer_lock);
}

void Client::Close()
{
    if (this->closed)
        return;
    close(this->Socket);
    this->closed = true;
}

void Client::SendLineNow(std::string line)
{
    if (!this->isConnected)
        return;
    if (line[line.length() - 1] != '\n')
        line += "\r\n";
    write(this->Socket, line.c_str(), line.length());
}

static std::string ok(std::string message)
{
    return std::string("<ok>") + message + "</ok>";
}

std::string Client::ReadLine(bool *error)
{
    std::string line;
    *error = false;
    Generic::Debug(this->SID + " waiting for input from client");
    while (true)
    {
        char buffer[1];
        ssize_t bytes = read(this->Socket, buffer, 1);
        if (bytes < 0)
        {
            // there was some error
            *error = true;
            goto exit;
        }
        if (!bytes)
        {
            // end of communication channel
            *error = true;
            goto exit;
        }
        if (buffer[0] == '\r')
            continue;
        if (buffer[0] == '\n')
            goto exit;
        line += buffer[0];
    }
    exit:
    Generic::Debug(this->SID + " finished waiting for data from client");
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
    std::vector<std::string>::iterator position = std::find(this->Subscriptions.begin(),
                                                            this->Subscriptions.end(),
                                                            site);
    bool result = (position != this->Subscriptions.end());
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

void Client::Kill(bool unlock)
{
    // in case unlock is set, it means that the vector was locked by another thread
    // before issuing the kill request, just to ensure that it would
    // be atomic operation, otherwise segfault might happen
    if (this->killed)
    {
        if (unlock)
            pthread_mutex_unlock(&Client::clients_lock);
        return;
    }
    this->killed = true;
    if (unlock)
        pthread_mutex_unlock(&Client::clients_lock);
    this->Close();
    this->isConnected = false;
    delete this;
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
    pthread_detach(pthread_self());
    while (_this->IsConnected())
    {
        bool er;
        std::string line = _this->ReadLine(&er);
        if (er || line == "exit")
        {
            Generic::Debug("Error data");
            _this->Close();
            goto exit;
        }
        if (line.size())
            _this->LastPing = time(0);
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
                _this->SendLine(ok(line));
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
                _this->SendLine(ok(line));
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
            std::string uptime = std::string("<stat>uptime since: ") + retrieve_uptime() +
                                             " users online: " + SSTR(UsersCount) +
                                             " connections made since launch: " +
                                             SSTR(Configuration::total_conn) +
                                             " io " + SSTR(Configuration::total_io) +
                                             "</stat>";
            _this->SendLine(uptime);
        }
        else if (line == "pong")
        {
            // dummy
        }
        else
        {
            _this->SendLine(mker(std::string("Unknown: ") + line));
        }
    }
    exit:
        Generic::Log(_this->SID + ": Connection closed: " + _this->IP);
        _this->ThreadRun2 = false;
        _this->Kill();
        return NULL;
}
