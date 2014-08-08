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

class Client
{
    public:
        static std::vector<Client*> clients;

        Client(int fd);
        ~Client();
        void Launch();
        void SendLine(std::string line);
        std::string ReadLine();
        int Subscribe(std::string wiki);
        std::vector<std::string> Subscriptions;
    private:
        static void *main(void *dummyPt);
        bool isConnected;
        pthread_t thread;
        int Socket;
};
