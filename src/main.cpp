#include "Server.hpp"
#include "signal.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

/*
** Validates that the string represents a numeric port in range [1, 65535]
*/
static bool validPort(const char *s, int &port) {
    if (!s || !*s) return false;
    for (int i = 0; s[i]; ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    port = std::atoi(s);
    return port > 0 && port <= 65535;
}

volatile sig_atomic_t g_running = 1;
void signalHandler(int sig) {
    (void) sig;
    g_running = 0;
}

/*
** - Ensures correct number of arguments, valid port number and no empty password
** - Signal handling: ignore sigpipe signals
** - Creates the server object and runs the server
*/
int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port;
    if (!validPort(argv[1], port)) {
        std::cerr << "Error: invalid port" << std::endl;
        return 1;
    }
    if (std::string(argv[2]).empty()) {
        std::cerr << "Error: empty password" << std::endl;
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);

    try {
        Server server(port, argv[2]);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
