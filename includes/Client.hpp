#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
public:
    Client(int fd);
    ~Client();

    int getFd() const;
    bool isRegistered() const;
    void setRegistered(bool value);

    bool hasPass() const;
    void setPass(bool value);

    const std::string &getNick() const;
    void setNick(const std::string &nick);

    const std::string &getUser() const;
    void setUser(const std::string &user);

    const std::string &getRealName() const;
    void setRealName(const std::string &realName);

    std::string &input();
    std::string &output();

    bool isClosed() const;
    void closeClient();

    std::string prefix() const;

private:
    int _fd;
    bool _registered;
    bool _passOk;
    bool _closed;
    std::string _nick;
    std::string _user;
    std::string _realName;
    std::string _input;
    std::string _output;
};

#endif
