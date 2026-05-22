#include "Client.hpp"

// ─── Constructor ────────────────────────────────────────────────────────────

Client::Client(int tmp_fd)
    : fd(tmp_fd),
      buffer(""),
      nickname(""),
      username(""),
      realname(""),
      hostname(""),
      hasPassword(false),
      hasNickname(false),
      hasUsername(false),
      authentication(false)
{}

// ─── Getters ─────────────────────────────────────────────────────────────────

int Client::getFd()
{
    return fd;
}

std::string& Client::getnickname()
{
    return nickname;
}

std::string& Client::getUsername()
{
    return username;
}

std::string& Client::getRealname()
{
    return realname;
}

std::string& Client::getHostname()
{
    return hostname;
}

std::string& Client::getBuffer()
{
    return buffer;
}

bool Client::isAuthenticated()
{
    return authentication;
}

bool Client::password_status()
{
    return hasPassword;
}

bool Client::nickname_status()
{
    return hasNickname;
}

bool Client::username_status()
{
    return hasUsername;
}

// ─── Setters ─────────────────────────────────────────────────────────────────

void Client::setfd(int tmp_fd)
{
    fd = tmp_fd;
}

void Client::setNickname(const std::string& tmp_nickname)
{
    nickname = tmp_nickname;
}

void Client::setUsername(const std::string& tmp_username)
{
    username = tmp_username;
}

void Client::setRealname(const std::string& tmp_realname)
{
    realname = tmp_realname;
}

void Client::sethostname(const std::string& tmp_hostname)
{
    hostname = tmp_hostname;
}

void Client::setAuthenticated(bool authentica_status)
{
    authentication = authentica_status;
}

void Client::set_password_status(bool pass_status)
{
    hasPassword = pass_status;
}

void Client::set_nickname_status(bool nick_status)
{
    hasNickname = nick_status;
}

void Client::set_username_status(bool user_status)
{
    hasUsername = user_status;
}

// ─── Buffer management ───────────────────────────────────────────────────────

void Client::appendToBuffer(const std::string& buff)
{
    buffer += buff;
}

void Client::clearbuffer()
{
    buffer.clear();
}

void Client::removeFromBuffer(size_t count)
{
    // Removes the first `count` characters from the buffer.
    // Called by the parser after it extracts one complete \r\n line.
    // Any remaining partial data stays at the front of the buffer.
    buffer.erase(0, count);
}
