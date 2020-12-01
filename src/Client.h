#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <unistd.h> 
#include <netinet/in.h>
#include <iostream>

class Client {
public:
    int socket;
    sockaddr_in hint;
    socklen_t hintSize;
    std::string username;
    std::string IP;
    bool connected;

    void sendData(std::string&);
    Client(int, sockaddr_in, socklen_t, std::string, std::string);
    ~Client();
};

#endif // CLIENT_H