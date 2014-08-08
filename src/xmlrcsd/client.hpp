//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <pthread.h>
#include <string>

class Client
{
    public:
        Client(int fd);
        void Launch();
        std::string ReadLine();
    private:
        static void *main(void *dummyPt);
        bool isConnected;
        pthread_t thread;
        int Socket;
};
