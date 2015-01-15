//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <vector>
#include <pthread.h>
#include <string>

#define EALREADYEXIST 1
#define EINVALID      2
#define ENOTEXIST     4
#define ETOOMANYSUBS  8
#define MAX_SUBSCRIPTIONS 800
#define MAX_SUBSCR_SIZE 200
#define RESP_OK "<ok></ok>"

class Client
{
    public:
        static std::vector<Client*> clients;
        static pthread_mutex_t clients_lock;
        static unsigned int UsersCount;

        Client(int fd);
        ~Client();
        void Launch();
        void SendLine(std::string line);
        std::string ReadLine(bool *error);
        bool IsSubscribed(std::string site);
        int Subscribe(std::string wiki);
        int Unsubscribe(std::string wiki);
        bool SubscribedAny;
        std::string IP;
        pthread_mutex_t subscriptions_lock;
        std::vector<std::string> Subscriptions;
    private:
        static void *main(void *dummyPt);
        bool isConnected;
        pthread_t thread;
        int Socket;
};
