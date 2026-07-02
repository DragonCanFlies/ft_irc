#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _registered(false), _passOk(false), _closed(false) {}
Client::~Client() {}

int Client::getFd() const { return _fd; }
bool Client::isRegistered() const { return _registered; }
void Client::setRegistered(bool value) { _registered = value; }
bool Client::hasPass() const { return _passOk; }
void Client::setPass(bool value) { _passOk = value; }
const std::string &Client::getNick() const { return _nick; }
void Client::setNick(const std::string &nick) { _nick = nick; }
const std::string &Client::getUser() const { return _user; }
void Client::setUser(const std::string &user) { _user = user; }
const std::string &Client::getRealName() const { return _realName; }
void Client::setRealName(const std::string &realName) { _realName = realName; }
std::string &Client::input() { return _input; }
std::string &Client::output() { return _output; }
bool Client::isClosed() const { return _closed; }
void Client::closeClient() { _closed = true; }

std::string Client::prefix() const {
    std::string nick = _nick.empty() ? "*" : _nick;
    std::string user = _user.empty() ? "user" : _user;
    return nick + "!" + user + "@localhost";
}
