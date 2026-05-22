#include "Channel.hpp"

// ─── Constructors ────────────────────────────────────────────────────────────

Channel::Channel()
    : name(""), topic(""), key(""), userLimit(0),
      inviteOnly(false), topicRestricted(false)
{}

Channel::Channel(const std::string& channelName)
    : name(channelName),
      topic(""),
      key(""),
      userLimit(0),
      inviteOnly(false),
      topicRestricted(false)
{}

// ─── Name ─────────────────────────────────────────────────────────────────────

const std::string& Channel::getName() const
{
    return name;
}

// ─── Topic ────────────────────────────────────────────────────────────────────

const std::string& Channel::getTopic() const
{
    return topic;
}

void Channel::setTopic(const std::string& newTopic)
{
    topic = newTopic;
}

bool Channel::hasTopic() const
{
    return !topic.empty();
}

// ─── Key (+k) ────────────────────────────────────────────────────────────────

const std::string& Channel::getKey() const
{
    return key;
}

void Channel::setKey(const std::string& newKey)
{
    key = newKey;
}

void Channel::clearKey()
{
    key.clear();
}

bool Channel::hasKey() const
{
    return !key.empty();
}

// ─── User limit (+l) ─────────────────────────────────────────────────────────

int Channel::getUserLimit() const
{
    return userLimit;
}

void Channel::setUserLimit(int limit)
{
    userLimit = limit; // pass 0 to remove the limit
}

bool Channel::hasUserLimit() const
{
    return userLimit > 0;
}

// ─── Modes ───────────────────────────────────────────────────────────────────

bool Channel::isInviteOnly() const
{
    return inviteOnly;
}

void Channel::setInviteOnly(bool val)
{
    inviteOnly = val;
}

bool Channel::isTopicRestricted() const
{
    return topicRestricted;
}

void Channel::setTopicRestricted(bool val)
{
    topicRestricted = val;
}

// ─── Members ─────────────────────────────────────────────────────────────────

void Channel::addMember(int fd)
{
    members.insert(fd);
}

void Channel::removeMember(int fd)
{
    members.erase(fd);
    // If they were an operator, remove that too
    operators.erase(fd);
    // Clean up invite list entry if present
    invited.erase(fd);
}

bool Channel::isMember(int fd) const
{
    return members.count(fd) > 0;
}

size_t Channel::memberCount() const
{
    return members.size();
}

const std::set<int>& Channel::getMembers() const
{
    return members;
}

// ─── Operators ───────────────────────────────────────────────────────────────

void Channel::addOperator(int fd)
{
    operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
    operators.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    return operators.count(fd) > 0;
}

// ─── Invite list ─────────────────────────────────────────────────────────────

void Channel::addInvited(int fd)
{
    invited.insert(fd);
}

void Channel::removeInvited(int fd)
{
    invited.erase(fd);
}

bool Channel::isInvited(int fd) const
{
    return invited.count(fd) > 0;
}
