#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <vector>
#include <string>

class Client {
    int fd;
    std::string buffer;
    std::string nickname;
    std::string username;
    std::string realname;
    std::string hostname;
    bool hasPassword;
    bool hasNickname;
    bool hasUsername;
    bool authentication;
public:
    std::string &getnickname(void);
    std::string &getUsername();
    std::string &getRealname();
    std::string &getHostname();
    bool password_status();
    bool nickname_status();
    bool username_status();
    int getFd();
    bool isAuthenticated();
    std::string &getBuffer();
    void setNickname(const std::string& tmp_nickname);
    void setUsername(const std::string& tmp_username);
    void setRealname(const std::string& tmp_realname);
    void setAuthenticated(bool authentica_status);
    void appendToBuffer(const std::string& buff);
    void sethostname(const std::string& tmp_hostname);
    void setfd(int tmp_fd);
    Client(int tmp_fd);
    void clearbuffer();
    void removeFromBuffer(size_t count);
    void set_password_status(bool pass_status);
    void set_nickname_status(bool nick_status);
    void set_username_status(bool user_status);
};

#endif
