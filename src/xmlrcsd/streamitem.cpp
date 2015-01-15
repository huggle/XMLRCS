//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <pthread.h>
#include "streamitem.hpp"
#include "client.hpp"
#include "generic.hpp"

void StreamItem::ProcessItem(std::string text)
{
    std::string::size_type splitter = text.find('|');
    if (splitter == std::string::npos)
    {
        Generic::Log("Invalid string: " + text);
        return;
    }
    std::string site = text.substr(0, splitter);
    std::string xml = text.substr(splitter + 1);
    pthread_mutex_lock(&Client::clients_lock);
    for (std::vector<Client*>::size_type i = 0; i != Client::clients.size(); i++)
    {
        if (Client::SubscribedAny || Client::clients[i]->IsSubscribed(site))
        {
            Client::clients[i]->SendLine(xml);
        }
    }
    pthread_mutex_unlock(&Client::clients_lock);
}

StreamItem::StreamItem()
{

}

StreamItem::~StreamItem()
{

}
