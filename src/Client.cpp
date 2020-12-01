#include "Client.h"

Client::Client(int socket,sockaddr_in hint, socklen_t hintSize, std::string username, std::string IP) {
    this->socket = socket;
    this->hint = hint;
    this->hintSize = hintSize;
    this->username = username;
    this->IP = IP;
    this->connected = true;
}

Client::~Client() {

}

void Client::sendData(std::string &msg) {
    msg += "\n";
    send(socket,msg.c_str(),msg.size(),0);
}