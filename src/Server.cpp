#include "Server.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <csignal>
#include <cctype>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "signal.hpp"

Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _serverFd(-1), _running(true) {}

Server::~Server() {
    for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    if (_serverFd != -1) close(_serverFd);
}

/*
** Creates and configures the server listening socket:
** - Creates the IPv4/TCP server socket
** - Allows address reuse: server can be restarted immediately on the same port
** - Makes the socket non-blocking
** - Initializes the sockaddr_in structure
** - Binds the socket to address and port
** - Makes the socket a listening socket
** - Registers the socket in the poll monitoring system
*/
void Server::setupSocket() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) throw std::runtime_error("socket failed");

    int yes = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        throw std::runtime_error("setsockopt failed");
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl failed");

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_serverFd, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");
    addPollFd(_serverFd, POLLIN);
}

/*
** Adds a file descriptor to the poll monitoring system
*/
void Server::addPollFd(int fd, short events) {
    struct pollfd p;
    p.fd = fd;
    p.events = events;
    p.revents = 0;
    _polls.push_back(p);
}

void Server::setPollFdEvents(int fd, short events) {
    for (std::vector<struct pollfd>::iterator it = _polls.begin(); it != _polls.end(); ++it) {
        if (it->fd == fd) {
            it->events = events;
            return;
        }
    }
}

/*
** Creates the server socket and runs the main event loop:
** Uses poll() to monitor all sockets and handle incoming events:
** - New client connections (POLLIN on server socket)
** - Client disconnection and error (POLLHUP / POLLERR / POLLNVAL)
** - Incoming client data (POLLIN)
** - Outgoing client data (POLLOUT)
*/
void Server::run() {
    setupSocket();
    std::cout << "ircserv listening on port " << _port << std::endl;
    while (g_running) {
        if (poll(&_polls[0], _polls.size(), -1) < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll failed");
        }
        for (size_t i = 0; i < _polls.size(); ++i) {
            int fd = _polls[i].fd;
            short re = _polls[i].revents;
            if (re == 0) continue;
            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                if (fd != _serverFd) { removeClient(fd, "connection lost"); i = 0; } // i is 1 next loop, i-- ?
                // else if (fd == _serverFd) {
                //     throw std::runtime_error("server socket error");
                // }
                continue;
            }
            if (fd == _serverFd && (re & POLLIN)) acceptClient();
            else {
                if (re & POLLIN) readClient(fd);
                if (_clients.find(fd) != _clients.end() && (re & POLLOUT)) writeClient(fd);
            }
        }
    }
}

/*
** Accepts all pending client connections:
** - Creates client sockets and sets them to non-blocking mode
** - Creates and stores the corresponding Client objects
** - Registers the sockets in the poll monitoring system
*/
void Server::acceptClient() {
    while (true) {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(_serverFd, (sockaddr *)&addr, &len);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            return; // log some stuff
        }
        fcntl(fd, F_SETFL, O_NONBLOCK); // check system call failure
        _clients[fd] = new Client(fd);
        addPollFd(fd, POLLIN);
        std::cout << "client connected fd=" << fd << std::endl;
    }
}

/*
** - Reads all available data from the client socket and
**   appends it to the client's input buffer.
** - Processes complete IRC commands available in the buffer.
** - Removes the client if the connection is closed.
*/
void Server::readClient(int fd) {
    Client *client = _clients[fd];
    char buf[512];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0) {
            client->input().append(buf, n);
            if (client->input().size() > 8192) { client->closeClient(); break; }
        } else if (n == 0) {
            client->closeClient();
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            client->closeClient();
            break;
        }
    }
    processBuffer(*client);
    if (client->isClosed()) removeClient(fd, "quit");
}

/*
** - Flushes as much data as possible from client's output buffer.
** - Disables POLLOUT if the output buffer is empty
** - Removes the client if it was marked closed
*/
void Server::writeClient(int fd) {
    Client *client = _clients[fd];
    std::string &out = client->output();
    while (!out.empty()) {
        ssize_t n = send(fd, out.c_str(), out.size(), 0);
        if (n > 0) out.erase(0, n);
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        else { client->closeClient(); break; }
    }
    if (out.empty() && _clients.find(fd) != _clients.end())
        setPollFdEvents(fd, POLLIN);
    if (client->isClosed()) removeClient(fd, "write error");
}

/*
** Extracts complete IRC commands from the client's input buffer
** and passes them to the command handler
*/
void Server::processBuffer(Client &client) {
    size_t pos;
    while ((pos = client.input().find('\n')) != std::string::npos) {
        std::string line = client.input().substr(0, pos + 1);
        client.input().erase(0, pos + 1);
        handleLine(client, line);
        if (client.isClosed()) break;
    }
}

/*
** Parses a complete IRC command and dispatches it to the
** corresponding command handler.
*/
void Server::handleLine(Client &client, const std::string &line) {
    std::vector<std::string> args = splitIrcLine(line);
    if (args.empty()) return;
    std::string cmd = args[0];
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = std::toupper(static_cast<unsigned char>(cmd[i]));

    if (cmd == "PASS") cmdPass(client, args);
    else if (cmd == "NICK") cmdNick(client, args);
    else if (cmd == "USER") cmdUser(client, args);
    else if (cmd == "PING") cmdPing(client, args);
    else if (cmd == "JOIN") cmdJoin(client, args);
    else if (cmd == "PART") cmdPart(client, args);
    else if (cmd == "PRIVMSG") cmdPrivmsg(client, args);
    else if (cmd == "QUIT") cmdQuit(client, args);
    else if (cmd == "KICK") cmdKick(client, args);
    else if (cmd == "INVITE") cmdInvite(client, args);
    else if (cmd == "TOPIC") cmdTopic(client, args);
    else if (cmd == "MODE") cmdMode(client, args);
    else numeric(client, 421, cmd + " :Unknown command");
}

void Server::reply(Client &client, const std::string &msg) {
    client.output() += msg + "\r\n";
    setPollFdEvents(client.getFd(), POLLIN | POLLOUT);
}

/*
** Builds an IRC numeric reply and passes it to reply(),
** which stores it in the client's output buffer and enables POLLOUT,
** allowing poll() to check when the socket is writable
*/
void Server::numeric(Client &client, int code, const std::string &msg) {
    std::stringstream ss;
    ss << ":ircserv ";
    if (code < 10) ss << "00";
    else if (code < 100) ss << "0";
    ss << code << " " << (client.getNick().empty() ? "*" : client.getNick()) << " " << msg;
    reply(client, ss.str());
}

/*
** Registers a client once PASS, NICK and USER have been provided.
** Sends a 001 welcome message on successful registration.
*/
void Server::tryRegister(Client &client) {
    if (!client.isRegistered() && client.hasPass() && !client.getNick().empty() && !client.getUser().empty()) {
        client.setRegistered(true);
        numeric(client, 001, ":Welcome to ft_irc");
    }
}

Client *Server::findNick(const std::string &nick) {
    std::map<std::string, int>::iterator it = _nickToFd.find(nick);
    if (it == _nickToFd.end()) return NULL;
    std::map<int, Client *>::iterator cit = _clients.find(it->second);
    if (cit == _clients.end()) return NULL;
    return cit->second;
}

/*
** Sends a message to all users in a channel (except the sender)
*/
void Server::broadcast(Channel &channel, const std::string &msg, int exceptFd) {
    const std::set<int> &users = channel.users();
    for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (*it == exceptFd) continue;
        if (_clients.find(*it) != _clients.end()) reply(*_clients[*it], msg);
    }
}

/*
** Returns a string containing the nicknames of all users in a channel
*/
std::string Server::nickList(Channel &channel) {
    std::string list;
    const std::set<int> &users = channel.users();
    for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (_clients.find(*it) == _clients.end()) continue;
        if (!list.empty()) list += " ";
        if (channel.isOperator(*it)) list += "@";
        list += _clients[*it]->getNick();
    }
    return list;
}

void Server::removeFromAllChannels(int fd, const std::string &reason) {
    std::vector<std::string> emptyChannels;
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        Channel &ch = it->second;
        if (ch.hasUser(fd)) {
            std::string prefix = _clients.find(fd) != _clients.end() ? _clients[fd]->prefix() : "unknown!user@localhost";
            ch.removeUser(fd);
            broadcast(ch, ":" + prefix + " QUIT :" + reason, fd);
            if (ch.empty()) emptyChannels.push_back(it->first);
        }
    }
    for (size_t i = 0; i < emptyChannels.size(); ++i) _channels.erase(emptyChannels[i]);
}

void Server::removeClient(int fd, const std::string &reason) {
    std::map<int, Client *>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;
    if (!it->second->getNick().empty()) _nickToFd.erase(it->second->getNick());
    removeFromAllChannels(fd, reason);
    close(fd);
    delete it->second;
    _clients.erase(it);
    for (std::vector<struct pollfd>::iterator p = _polls.begin(); p != _polls.end(); ++p) {
        if (p->fd == fd) { _polls.erase(p); break; }
    }
    std::cout << "client removed fd=" << fd << std::endl;
}

/*
** Handles the PASS command and attempts client registration.
*/
void Server::cmdPass(Client &client, const std::vector<std::string> &args) {
    if (client.isRegistered()) { numeric(client, 462, ":You may not reregister"); return; }
    if (args.size() < 2) { numeric(client, 461, "PASS :Not enough parameters"); return; }
    if (args[1] != _password) { numeric(client, 464, ":Password incorrect"); client.closeClient(); return; }
    client.setPass(true);
    tryRegister(client);
}

/*
** Handles the NICK command, updates the client's nickname
** and attempts client registration.
*/
void Server::cmdNick(Client &client, const std::vector<std::string> &args) {
    if (args.size() < 2) { numeric(client, 431, ":No nickname given"); return; }
    if (!isValidNick(args[1])) { numeric(client, 432, args[1] + " :Erroneous nickname"); return; }
    if (_nickToFd.find(args[1]) != _nickToFd.end() && _nickToFd[args[1]] != client.getFd()) {
        numeric(client, 433, args[1] + " :Nickname is already in use"); return;
    }
    if (!client.getNick().empty()) _nickToFd.erase(client.getNick());
    std::string oldPrefix = client.prefix();
    client.setNick(args[1]);
    _nickToFd[args[1]] = client.getFd();
    if (client.isRegistered()) reply(client, ":" + oldPrefix + " NICK :" + args[1]);
    tryRegister(client);
}

/*
** Handles the USER command, stores username and real name
** and attempts client registration.
*/
void Server::cmdUser(Client &client, const std::vector<std::string> &args) {
    if (client.isRegistered()) { numeric(client, 462, ":You may not reregister"); return; }
    if (args.size() < 5) { numeric(client, 461, "USER :Not enough parameters"); return; }
    client.setUser(args[1]);
    client.setRealName(args[4]);
    tryRegister(client);
}

/*
** Handles the PING command
*/
void Server::cmdPing(Client &client, const std::vector<std::string> &args) {
    if (args.size() < 2) reply(client, ":ircserv PONG ircserv");
    else reply(client, ":ircserv PONG ircserv :" + args[1]);
}

/*
** Handles the JOIN command:
** - Creates the channel if it does not exist
** - Applies channel access rules (invite-only, key, limit)
** - Adds the client to the channel (and sets operator if first user)
*/
void Server::cmdJoin(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 2) { numeric(client, 461, "JOIN :Not enough parameters"); return; }
    std::string name = args[1];
    std::string key = args.size() >= 3 ? args[2] : "";
    if (!isValidChannelName(name)) { numeric(client, 403, name + " :No such channel"); return; }
    if (_channels.find(name) == _channels.end()) _channels[name] = Channel(name);
    Channel &ch = _channels[name];
    if (ch.hasUser(client.getFd())) return;
    if (ch.isInviteOnly() && !ch.isInvited(client.getFd())) { numeric(client, 473, name + " :Cannot join channel (+i)"); return; }
    if (!ch.getKey().empty() && ch.getKey() != key) { numeric(client, 475, name + " :Cannot join channel (+k)"); return; }
    if (ch.getLimit() > 0 && (int)ch.users().size() >= ch.getLimit()) { numeric(client, 471, name + " :Cannot join channel (+l)"); return; }
    bool first = ch.users().empty();
    ch.addUser(client.getFd());
    ch.removeInvite(client.getFd());
    if (first) ch.addOperator(client.getFd());
    std::string joinMsg = ":" + client.prefix() + " JOIN :" + name;
    reply(client, joinMsg);
    broadcast(ch, joinMsg, client.getFd());
    if (!ch.getTopic().empty()) numeric(client, 332, name + " :" + ch.getTopic());
    numeric(client, 353, "= " + name + " :" + nickList(ch));
    numeric(client, 366, name + " :End of /NAMES list");
}

/*
** Handles the PART command: removes the client from a channel
*/
void Server::cmdPart(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 2) { numeric(client, 461, "PART :Not enough parameters"); return; }
    std::string name = args[1];
    if (_channels.find(name) == _channels.end() || !_channels[name].hasUser(client.getFd())) { numeric(client, 442, name + " :You're not on that channel"); return; }
    Channel &ch = _channels[name];
    std::string msg = ":" + client.prefix() + " PART " + name;
    reply(client, msg);
    broadcast(ch, msg, client.getFd());
    ch.removeUser(client.getFd());
    if (ch.empty()) _channels.erase(name);
}

/*
** Handles the PRIVMSG command:
** - Sends a message to all users of a channel the client belongs to
** - Sends a private message to another user
*/
void Server::cmdPrivmsg(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 3) { numeric(client, 461, "PRIVMSG :Not enough parameters"); return; }
    std::string target = args[1];
    std::string text = args[2];
    if (!target.empty() && target[0] == '#') {
        if (_channels.find(target) == _channels.end()) { numeric(client, 403, target + " :No such channel"); return; }
        Channel &ch = _channels[target];
        if (!ch.hasUser(client.getFd())) { numeric(client, 404, target + " :Cannot send to channel"); return; }
        broadcast(ch, ":" + client.prefix() + " PRIVMSG " + target + " :" + text, client.getFd());
    } else {
        Client *dest = findNick(target);
        if (!dest) { numeric(client, 401, target + " :No such nick"); return; }
        reply(*dest, ":" + client.prefix() + " PRIVMSG " + target + " :" + text);
    }
}

/*
** Handles the QUIT command
*/
void Server::cmdQuit(Client &client, const std::vector<std::string> &args) {
    (void)args;
    client.closeClient();
}

void Server::cmdKick(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 3) { numeric(client, 461, "KICK :Not enough parameters"); return; }
    std::string chName = args[1];
    std::string nick = args[2];
    std::string reason = args.size() >= 4 ? args[3] : client.getNick();
    if (_channels.find(chName) == _channels.end()) { numeric(client, 403, chName + " :No such channel"); return; }
    Channel &ch = _channels[chName];
    if (!ch.hasUser(client.getFd())) { numeric(client, 442, chName + " :You're not on that channel"); return; }
    if (!ch.isOperator(client.getFd())) { numeric(client, 482, chName + " :You're not channel operator"); return; }
    Client *target = findNick(nick);
    if (!target || !ch.hasUser(target->getFd())) { numeric(client, 441, nick + " " + chName + " :They aren't on that channel"); return; }
    std::string msg = ":" + client.prefix() + " KICK " + chName + " " + nick + " :" + reason;
    broadcast(ch, msg, -1);
    ch.removeUser(target->getFd());
    if (ch.empty()) _channels.erase(chName);
}

/*
** Handles the INVITE command:
** - Verifies that the client is a channel operator
** - Sends an invitation to another client to join the channel
*/
void Server::cmdInvite(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 3) { numeric(client, 461, "INVITE :Not enough parameters"); return; }
    Client *target = findNick(args[1]);
    std::string chName = args[2];
    if (!target) { numeric(client, 401, args[1] + " :No such nick"); return; }
    if (_channels.find(chName) == _channels.end()) { numeric(client, 403, chName + " :No such channel"); return; }
    Channel &ch = _channels[chName];
    if (!ch.hasUser(client.getFd())) { numeric(client, 442, chName + " :You're not on that channel"); return; }
    if (!ch.isOperator(client.getFd())) { numeric(client, 482, chName + " :You're not channel operator"); return; }
    ch.invite(target->getFd());
    numeric(client, 341, args[1] + " " + chName);
    reply(*target, ":" + client.prefix() + " INVITE " + args[1] + " :" + chName);
}

void Server::cmdTopic(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 2) { numeric(client, 461, "TOPIC :Not enough parameters"); return; }
    std::string chName = args[1];
    if (_channels.find(chName) == _channels.end()) { numeric(client, 403, chName + " :No such channel"); return; }
    Channel &ch = _channels[chName];
    if (!ch.hasUser(client.getFd())) { numeric(client, 442, chName + " :You're not on that channel"); return; }
    if (args.size() == 2) {
        if (ch.getTopic().empty()) numeric(client, 331, chName + " :No topic is set");
        else numeric(client, 332, chName + " :" + ch.getTopic());
        return;
    }
    if (ch.isTopicOnlyOps() && !ch.isOperator(client.getFd())) { numeric(client, 482, chName + " :You're not channel operator"); return; }
    ch.setTopic(args[2]);
    std::string msg = ":" + client.prefix() + " TOPIC " + chName + " :" + args[2];
    reply(client, msg);
    broadcast(ch, msg, client.getFd());
}

void Server::cmdMode(Client &client, const std::vector<std::string> &args) {
    if (!client.isRegistered()) { numeric(client, 451, ":You have not registered"); return; }
    if (args.size() < 2) { numeric(client, 461, "MODE :Not enough parameters"); return; }
    std::string chName = args[1];
    if (_channels.find(chName) == _channels.end()) { numeric(client, 403, chName + " :No such channel"); return; }
    Channel &ch = _channels[chName];
    if (args.size() == 2) {
        std::string modes = "+";
        if (ch.isInviteOnly()) modes += "i";
        if (ch.isTopicOnlyOps()) modes += "t";
        if (!ch.getKey().empty()) modes += "k";
        if (ch.getLimit() > 0) modes += "l";
        numeric(client, 324, chName + " " + modes);
        return;
    }
    if (!ch.hasUser(client.getFd())) { numeric(client, 442, chName + " :You're not on that channel"); return; }
    if (!ch.isOperator(client.getFd())) { numeric(client, 482, chName + " :You're not channel operator"); return; }

    std::string modes = args[2];
    bool adding = true;
    size_t paramIndex = 3;
    std::string applied = "";
    std::string params = "";
    for (size_t i = 0; i < modes.size(); ++i) {
        char m = modes[i];
        if (m == '+') { adding = true; if (applied.empty() || applied[applied.size()-1] == '-') applied += "+"; continue; }
        if (m == '-') { adding = false; if (applied.empty() || applied[applied.size()-1] == '+') applied += "-"; continue; }
        if (applied.empty()) applied += adding ? "+" : "-";
        if (m == 'i') { ch.setInviteOnly(adding); applied += "i"; }
        else if (m == 't') { ch.setTopicOnlyOps(adding); applied += "t"; }
        else if (m == 'k') {
            if (adding) {
                if (paramIndex >= args.size()) continue;
                ch.setKey(args[paramIndex]); params += " " + args[paramIndex++];
            } else ch.setKey("");
            applied += "k";
        } else if (m == 'l') {
            if (adding) {
                if (paramIndex >= args.size()) continue;
                int limit = toInt(args[paramIndex]);
                if (limit <= 0) continue;
                ch.setLimit(limit); params += " " + args[paramIndex++];
            } else ch.setLimit(0);
            applied += "l";
        } else if (m == 'o') {
            if (paramIndex >= args.size()) continue;
            Client *target = findNick(args[paramIndex]);
            if (!target || !ch.hasUser(target->getFd())) { paramIndex++; continue; }
            if (adding) ch.addOperator(target->getFd()); else ch.removeOperator(target->getFd());
            params += " " + args[paramIndex++];
            applied += "o";
        }
    }
    if (applied == "+" || applied == "-" || applied.empty()) return;
    std::string msg = ":" + client.prefix() + " MODE " + chName + " " + applied + params;
    reply(client, msg);
    broadcast(ch, msg, client.getFd());
}
