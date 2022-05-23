// Add ping to client
// Add commands

#include "Server.h"
#define For(n) for(int i = 0; i < n; i++)

bool validSend(const std::vector<std::string>&);
bool validNick(const std::vector<std::string>&);
bool validList(const std::vector<std::string>&);
bool validpSend(const std::vector<std::string>&);

Server::Server(int port, int max) {
    info.clients = new std::unordered_map<std::string,Client*>();
    info.usernames = new std::unordered_set<std::string>();
    conn.port = port;
    info.maxUsers = max;
    createInputCommands();
    defineFileDescriptor();
    setSockOption();
    bindSocket();
    markListener();
    run();
}

Server::~Server() {
    close(conn.socket);
    delete info.clients;
    delete info.usernames;
    delete info.inputCommands;
}

void Server::defineFileDescriptor() {
    if((conn.socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOG(Log::ERROR, "Socket couldnt be created");
        exit(EXIT_FAILURE);
    }
    LOG(Log::SERVER, "Socket[Socket Stream] has been created");
}

void Server::setSockOption() {
    if(setsockopt(conn.socket, SOL_SOCKET, SO_REUSEADDR, &conn.opt,sizeof(conn.opt)) == -1) {
        LOG(Log::ERROR, "Issue setting the Socket level to: SOL_SOCKET");
        exit(EXIT_FAILURE);
        }
        LOG(Log::SERVER, "Socket level has been set towards: SOL_SOCKET");
}

void Server::bindSocket() {
    conn.hint.sin_family = AF_INET;
    conn.hint.sin_addr.s_addr = INADDR_ANY;
    conn.hint.sin_port = htons(conn.port);
    conn.sockSize = sizeof(conn.hint);

    if(bind(conn.socket,(sockaddr *)&conn.hint,conn.sockSize) == -1) {
        LOG(Log::ERROR, "Issue binding socket to hint structure");
        exit(EXIT_FAILURE);
    }
    LOG(Log::SERVER, "Socket has been bound to hint structure");
}

void Server::markListener() {
    if(listen(conn.socket,conn.listenBackLogSize) == -1) {
        LOG(Log::ERROR, "Socket failed to set listener");
        exit(EXIT_FAILURE);
    }
    LOG(Log::SERVER, "Socket is set for listening");

}

void Server::run() {
    LOG(Log::SERVER,"< SERVER STARTED >");
    int socket;
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    while(info.state==open) {
        if((socket = accept(conn.socket,(sockaddr *)&client,&clientSize)) == -1) {
            LOG(Log::ERROR, "Client socket has failed to join");
            exit(EXIT_FAILURE);
        }
        if(info.currUsers == info.maxUsers) {
            LOG(Log::WARNING, "Client: ["+ findIP(client) + "] has tried to join, Server is full");
            close(socket);
            continue;
        }
        info.currUsers++;
        addClient(socket, client);
    }
}

void Server::handle(Client *client) {
    LOG(Log::WARNING, "Client handler has started for Client: ["+findIP(client->hint)+"] testing");

    std::thread setNick(&Server::setNickTimer, this, client);
    setNick.detach();

    int bytes;
    char buffer[BUFFER_SIZE];


    while(client->connected) {
    
        bytes = recv(client->socket,buffer,BUFFER_SIZE,0);
        if(bytes == 0) {
            client->connected = false;
            break;
        }

        std::string input = std::string(buffer,0,bytes-1);

		if(input[bytes - 1] == '\r') {
			input = std::string(input.c_str(), 0, bytes - 2);
		}


        std::vector<std::string> tokens(split(input,SEPARATOR));
        Command command = getCommand(tokens,client);


        if(client->username == "")
             LOG(Log::MESSAGE, "Input Recieved from ["+client->IP+"]: "+input);
        else LOG(Log::MESSAGE, "Input Recieved from '"+std::string(client->username)+"': "+input);

        switch(command) {
            case SEND: {
                sendToAll(tokens, client);
                break;
            }
            case NICK: {
                setNickForClient(client,tokens[1]);
                break;
            }
            case LIST: {
                list(client);
                break;
            }
            case INVALID: {
                client->connected = false;
                break;
            }
            case pSEND: {
                pSend(tokens, client);
                break;
            }
        }
    }
    sleep(1);
    removeClient(client);
}

std::string Server::findIP(sockaddr_in sock) {
    unsigned long addr = sock.sin_addr.s_addr;
    char buffer[50];
    sprintf(buffer, "%d.%d.%d.%d",
            (int)(addr  & 0xff),
            (int)((addr & 0xff00) >> 8),
            (int)((addr & 0xff0000) >> 16),
            (int)((addr & 0xff000000) >> 24));
    return std::string(buffer);
}

void Server::addClient(int socket, sockaddr_in hint) {
    clientsMutex.lock();
    Client *client = new Client(socket,hint,sizeof(hint),"",findIP(hint));
    info.clients->emplace("",client);

    LOG(Log::INFO, "Client [" + findIP(client->hint) + "] has joined to the server");

    std::thread handle(&Server::handle, this, client);
    handle.detach();
    clientsMutex.unlock();
}

void Server::setNickTimer(Client *client) {
    LOG(Log::WARNING, "Username handler has started for Client: ["+findIP(client->hint)+"]");
    int ticker = 0; 
    bool set = true;
    while(client->username == "" && client->connected) {
        if(ticker == NICK_TIMER) {
            set = false;
            break;
        }
        LOG(Log::INFO,"Client: ["+client->IP+"] has '"+std::to_string((NICK_TIMER-ticker))+"' seconds remaining to set their nick");
        ticker++;
        sleep(1);
    }
    if(!set) {
        client->connected = false;
    }
    LOG(Log::WARNING, "Username handler has ended for Client: ["+findIP(client->hint)+"]");
}

void Server::setNickForClient(Client *client, std::string nick) {
    clientsMutex.lock();
    if (info.usernames->find(nick) == info.usernames->end()) {
        for(auto a = info.clients->begin(); a != info.clients->end(); a++) {
            if(a->second == client) {
                info.clients->erase(a);
                (*info.clients)[nick] = client;
                client->username = nick;
                
            }
        }
        
        LOG(Log::SERVER,"Client: "+client->IP+" has set their username to: '"+client->username+"'");
        info.usernames->insert(nick);
    } else {
        LOG(Log::WARNING,"Client: "+client->IP+" has set their username to an already existing username!");
        client->connected = false;
    }
    clientsMutex.unlock();
}

void Server::removeClient(Client *client) {
    clientsMutex.lock();

    for(auto a = info.clients->begin(); a != info.clients->end(); a++)
        if(a->second == client)
            info.clients->erase(a);

    if(client->username == "")
        client->username = client->IP;

    send(client->socket,INVALID_PROTOCOL,std::string(INVALID_PROTOCOL).length(),0);
    close(client->socket);
    info.usernames->erase(client->username);

    info.currUsers--;

    client->username == "" ? LOG(Log::INFO, "Client: '"+findIP(client->hint) + "' has disconnected!") 
    : LOG(Log::INFO, "Client: '"+client->username + "' has disconnected!");

    delete client;

    clientsMutex.unlock();
}

std::vector<std::string> Server::split(std::string &str, char separator) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string x;
    while(getline(ss, x, separator))
        if(x != "")
            tokens.emplace_back(x);
    return tokens;
}

Server::Command Server::getCommand(std::vector<std::string> &tokens,Client *client) {
    Command initialCommand;

    bool checkIntialCommand = info.inputCommands->find(tokens[0]) != info.inputCommands->end();

    if(checkIntialCommand)
        initialCommand = info.inputCommands->find(tokens[0])->second.command;

     if(client->username == "" && initialCommand != NICK)
         return INVALID;

    if(tokens.empty())
        return INVALID;


    auto a = info.inputCommands->find(tokens[0]);
    if(a == info.inputCommands->end())
        return INVALID;
    
    if(info.inputCommands->find(tokens[0])->second.valid == 0)
        return INVALID;
    else return info.inputCommands->find(tokens[0])->second.command;


    return INVALID;
}

void Server::createInputCommands() {
    (*info.inputCommands)["/send"] = {SEND,&validSend, "Command to send current clients a message"};
    (*info.inputCommands)["/list"] = {LIST,&validList, "Command to list the current clients"};
    (*info.inputCommands)["/nick"] = {NICK,&validNick, "Command to set clients username"};
    (*info.inputCommands)["/pSend"] = {pSEND,&validpSend, "Command to set clients username"};

}

bool validSend(const std::vector<std::string> &tokens) {
    if(tokens.size() == 1)
        return false;
    return true;
}

bool validList(const std::vector<std::string> &tokens) {
    if(tokens.size() == 1)
        return true;
    return false;
}

bool validNick(const std::vector<std::string> &tokens) {
    if(tokens.size() == 2)
        return true;
    if(tokens[1] == " ")
        return false;

    return false;
}

void Server::sendToAll(std::vector<std::string> tokens, Client *sender) {
    clientsMutex.lock();
    std::string msg = "";

    for(int i = 1; i < tokens.size(); i++)
        msg += tokens[i] + " ";

        // msg = interruptMsg(msg);
        msg = "[Public] ["+sender->username+"]: "+msg;

    for(auto i = info.clients->begin(); i != info.clients->end(); i++)
        if(sender->username != i->second->username)
            i->second->sendData(msg);
    clientsMutex.unlock();
}

void Server::pSend(std::vector<std::string> tokens, Client* sender) {
    clientsMutex.lock();
    std::string msg = "";

    for(int i = 2; i < tokens.size(); i++)
        msg += tokens[i] + " ";

    msg = "[Private] ["+sender->username+"]: " + msg+"\n";

    int findClient = findClientBasedOnUsername(tokens[1]);
    send(findClient,msg.c_str(),msg.length(),0);
    clientsMutex.unlock();
}

void Server::list(Client *client) {
    clientsMutex.lock();
    std::string list = "";

    for(auto i = info.clients->begin(); i != info.clients->end(); i++) {
        list+= "~['" + i->second->username+"' | '"+i->second->IP+"']";
    }
    list += "\n";

    // list = interruptMsg(list);

    send(client->socket,list.c_str(), list.size(),0);
    clientsMutex.unlock();
}

std::string Server::interruptMsg(std::string msg) {
    // for(int i = 0; i < msg.length(); i++) {
    //     msg[i] ^= KEY;
    // }
    return msg;
}

bool validpSend(const std::vector<std::string> &tokens) {
    if(tokens.size() == 2)
        return 0;
    return 1;
}

int Server::findClientBasedOnUsername(std::string user) {
    for(auto i = info.clients->begin(); i != info.clients->end(); i++)
        if(user == i->second->username)
            return i->second->socket;
    return 0;
}
