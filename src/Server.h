#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <unistd.h> 
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <sstream>
#include "Client.h"
#include "Log.h"

#define BUFFER_SIZE 512
#define NICK_TIMER 15
#define INVALID_PROTOCOL (char*)">> Invalid Protocol\r\n"
#define SEPARATOR (char)' '
#define KEY 896

class Server {
public:
    enum Command {
        SEND,
        NICK,
        LIST,
        pSEND,
        INVALID
    };

    enum State {
        open,
        closed
    };
    
    struct input {
        Command command;
        bool (*valid)(const std::vector<std::string>&);
        std::string desc;
    };
    
private:
    void defineFileDescriptor();
    void setSockOption();
    void bindSocket();
    void markListener();
    void run();
    void handle(Client*);
    std::string findIP(sockaddr_in);
    void addClient(int, sockaddr_in);
    void setNickTimer(Client*);
    void setNickForClient(Client*,std::string);
    void removeClient(Client*);
    std::vector<std::string> split(std::string &, char);
    Command getCommand(std::vector<std::string>&,Client *);
    void createInputCommands();
    void sendToAll(std::vector<std::string>, Client*);
    void pSend(std::vector<std::string>, Client*);
    void list(Client*);
    std::string interruptMsg(std::string);
    int findClientBasedOnUsername(std::string user);

    std::mutex clientsMutex;

    struct info {
        State state = open;
        int maxUsers;
        int currUsers;
        std::unordered_map<std::string, Client*> *clients = NULL;
        std::unordered_set<std::string> *usernames = NULL;
        std::unordered_map<std::string, input> *inputCommands = new std::unordered_map<std::string, input>();
    }info;

    struct connection {
        int port;
        int socket;
        sockaddr_in hint;
        socklen_t sockSize;
        int listenBackLogSize = 5;
        int opt = 1;
    }conn;

public:
    Server(int, int);
    ~Server();
    void operator=(const Server&) = delete;

    friend bool validSend(const std::vector<std::string>&);
    friend bool validNick(const std::vector<std::string>&);
    friend bool validList(const std::vector<std::string>&);
    friend bool validpSend(const std::vector<std::string>&);
};
#endif // SERVER_H