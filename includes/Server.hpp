#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include <string>
#include <vector>
#include <map>
#include <poll.h>

class Server {
public:
    Server(int port, const std::string &password);
    ~Server();

    void run();

private:
    int _port;
    std::string _password;
    int _serverFd;
    bool _running;
    std::vector<struct pollfd> _polls;
    std::map<int, Client *> _clients;
    std::map<std::string, int> _nickToFd;
    std::map<std::string, Channel> _channels;

    void setupSocket();
    void addPollFd(int fd, short events);
    void setPollFdEvents(int fd, short events);
    void removeClient(int fd, const std::string &reason);
    void acceptClient();
    void readClient(int fd);
    void writeClient(int fd);
    void processBuffer(Client &client);
    void handleLine(Client &client, const std::string &line);

    void reply(Client &client, const std::string &msg);
    void numeric(Client &client, int code, const std::string &msg);
    void tryRegister(Client &client);
    Client *findNick(const std::string &nick);

    void broadcast(Channel &channel, const std::string &msg, int exceptFd);
    void removeFromAllChannels(int fd, const std::string &reason);
    std::string nickList(Channel &channel);

    void cmdPass(Client &client, const std::vector<std::string> &args);
    void cmdNick(Client &client, const std::vector<std::string> &args);
    void cmdUser(Client &client, const std::vector<std::string> &args);
    void cmdPing(Client &client, const std::vector<std::string> &args);
    void cmdJoin(Client &client, const std::vector<std::string> &args);
    void cmdPart(Client &client, const std::vector<std::string> &args);
    void cmdPrivmsg(Client &client, const std::vector<std::string> &args);
    void cmdQuit(Client &client, const std::vector<std::string> &args);
    void cmdKick(Client &client, const std::vector<std::string> &args);
    void cmdInvite(Client &client, const std::vector<std::string> &args);
    void cmdTopic(Client &client, const std::vector<std::string> &args);
    void cmdMode(Client &client, const std::vector<std::string> &args);
};

#endif
