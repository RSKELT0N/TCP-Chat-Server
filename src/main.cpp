#include "Server.h"

int main() {
    Server *server = new Server(52222,6);
    delete server;
    return 0;
}