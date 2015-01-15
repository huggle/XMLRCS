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
        void SendLineNow(std::string line);
        std::string ReadLine(bool *error);
        bool IsSubscribed(std::string site);
        bool IsConnected() { return this->isConnected; }
        int Subscribe(std::string wiki);
        int Unsubscribe(std::string wiki);
        pthread_mutex_t OutgoingBuffer_lock;
        std::vector<std::string> OutgoingBuffer;
        bool SubscribedAny;
        std::string IP;
        pthread_mutex_t subscriptions_lock;
        std::vector<std::string> Subscriptions;
        bool ThreadRun;
    private:
        static void *main(void *dummyPt);
        //! Thread we use to deliver messages to clients so that we don't stuck whole server
        pthread_t sender;
        bool isConnected;
        pthread_t thread;
        int Socket;
};
