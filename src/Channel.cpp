#include "Channel.hpp"

Channel::Channel() : _limit(0), _inviteOnly(false), _topicOnlyOps(false) {}
Channel::Channel(const std::string &name) : _name(name), _limit(0), _inviteOnly(false), _topicOnlyOps(false) {}
Channel::~Channel() {}

const std::string &Channel::getName() const { return _name; }
const std::string &Channel::getTopic() const { return _topic; }
void Channel::setTopic(const std::string &topic) { _topic = topic; }
bool Channel::isInviteOnly() const { return _inviteOnly; }
void Channel::setInviteOnly(bool value) { _inviteOnly = value; }
bool Channel::isTopicOnlyOps() const { return _topicOnlyOps; }
void Channel::setTopicOnlyOps(bool value) { _topicOnlyOps = value; }
const std::string &Channel::getKey() const { return _key; }
void Channel::setKey(const std::string &key) { _key = key; }
int Channel::getLimit() const { return _limit; }
void Channel::setLimit(int limit) { _limit = limit; }

bool Channel::hasUser(int fd) const { return _users.find(fd) != _users.end(); }
void Channel::addUser(int fd) { _users.insert(fd); }
void Channel::removeUser(int fd) { _users.erase(fd); _operators.erase(fd); _invited.erase(fd); }
bool Channel::isOperator(int fd) const { return _operators.find(fd) != _operators.end(); }
void Channel::addOperator(int fd) { _operators.insert(fd); }
void Channel::removeOperator(int fd) { _operators.erase(fd); }
void Channel::invite(int fd) { _invited.insert(fd); }
bool Channel::isInvited(int fd) const { return _invited.find(fd) != _invited.end(); }
void Channel::removeInvite(int fd) { _invited.erase(fd); }
const std::set<int> &Channel::users() const { return _users; }
bool Channel::empty() const { return _users.empty(); }
