#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
public:
    Channel();
    Channel(const std::string &name);
    ~Channel();

    const std::string &getName() const;
    const std::string &getTopic() const;
    void setTopic(const std::string &topic);

    bool isInviteOnly() const;
    void setInviteOnly(bool value);

    bool isTopicOnlyOps() const;
    void setTopicOnlyOps(bool value);

    const std::string &getKey() const;
    void setKey(const std::string &key);

    int getLimit() const;
    void setLimit(int limit);

    bool hasUser(int fd) const;
    void addUser(int fd);
    void removeUser(int fd);

    bool isOperator(int fd) const;
    void addOperator(int fd);
    void removeOperator(int fd);

    void invite(int fd);
    bool isInvited(int fd) const;
    void removeInvite(int fd);

    const std::set<int> &users() const;
    bool empty() const;

private:
    std::string _name;
    std::string _topic;
    std::string _key;
    int _limit;
    bool _inviteOnly;
    bool _topicOnlyOps;
    std::set<int> _users;
    std::set<int> _operators;
    std::set<int> _invited;
};

#endif
