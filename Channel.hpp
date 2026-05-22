#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

// Represents one IRC channel (e.g. #general).
//
// Stored in Server as: std::map<std::string, Channel>
// Key is the channel name lowercased.
//
// Member fds   — every client currently in the channel.
// Operator fds — members with operator privilege (can KICK, INVITE, set TOPIC, MODE).
// Invited fds  — clients invited via INVITE (needed when +i mode is active).

class Channel
{
private:
    std::string      name;          // e.g. "#general"
    std::string      topic;         // current topic, empty = no topic set
    std::string      key;           // channel password (+k), empty = no key
    int              userLimit;     // max members (+l), 0 = no limit
    bool             inviteOnly;    // +i mode
    bool             topicRestricted; // +t mode — only ops can change topic

    std::set<int>    members;       // fds of all clients in the channel
    std::set<int>    operators;     // fds of clients with op status
    std::set<int>    invited;       // fds of clients who have been /INVITE'd

public:
    // ── Constructors ─────────────────────────────────────────────────────────
    Channel();
    Channel(const std::string& name);

    // ── Name ─────────────────────────────────────────────────────────────────
    const std::string& getName() const;

    // ── Topic ─────────────────────────────────────────────────────────────────
    const std::string& getTopic() const;
    void               setTopic(const std::string& newTopic);
    bool               hasTopic() const; // true if topic is non-empty

    // ── Key (+k) ─────────────────────────────────────────────────────────────
    const std::string& getKey() const;
    void               setKey(const std::string& newKey);
    void               clearKey();
    bool               hasKey() const;

    // ── User limit (+l) ──────────────────────────────────────────────────────
    int                getUserLimit() const;
    void               setUserLimit(int limit); // pass 0 to remove limit
    bool               hasUserLimit() const;

    // ── Modes ─────────────────────────────────────────────────────────────────
    bool               isInviteOnly() const;
    void               setInviteOnly(bool val);

    bool               isTopicRestricted() const;
    void               setTopicRestricted(bool val);

    // ── Members ───────────────────────────────────────────────────────────────
    void               addMember(int fd);
    void               removeMember(int fd);
    bool               isMember(int fd) const;
    size_t             memberCount() const;
    const std::set<int>& getMembers() const; // for iterating in PRIVMSG broadcast

    // ── Operators ─────────────────────────────────────────────────────────────
    void               addOperator(int fd);
    void               removeOperator(int fd);
    bool               isOperator(int fd) const;

    // ── Invite list ───────────────────────────────────────────────────────────
    void               addInvited(int fd);
    void               removeInvited(int fd);
    bool               isInvited(int fd) const;
};

#endif
