#include "Server.hpp"
#include <cctype>

// ─── Internal helpers (used only in this file) ────────────────────────────────

// Returns true if the nickname string is valid IRC nickname format:
//   - 1 to 9 characters
//   - First char: letter or one of: _ \ [ ] { } | `
//   - Remaining: letter, digit, or one of: - _ \ [ ] { } | `
static bool isValidNick(const std::string& nick)
{
    if (nick.empty() || nick.size() > 9)
        return false;
    const std::string special = "_\\[]{}|`";
    if (!std::isalpha((unsigned char)nick[0]) &&
        special.find(nick[0]) == std::string::npos)
        return false;
    for (size_t i = 1; i < nick.size(); ++i)
    {
        char c = nick[i];
        if (!std::isalnum((unsigned char)c) &&
            special.find(c) == std::string::npos && c != '-')
            return false;
    }
    return true;
}

// Checks whether all three registration steps are complete.
// If yes, marks the client as authenticated and sends the welcome burst.
static void checkAndFinishRegistration(Server* srv, Client& client,
                                        void (Server::*sendFn)(int, const std::string&))
{
    if (client.password_status() && client.nickname_status() && client.username_status())
    {
        client.setAuthenticated(true);
        // 001 RPL_WELCOME
        (srv->*sendFn)(client.getFd(),
            ":server 001 " + client.getnickname() +
            " :Welcome to the IRC Network " +
            client.getnickname() + "!" +
            client.getUsername() + "@" +
            client.getHostname());
        // 002 RPL_YOURHOST
        (srv->*sendFn)(client.getFd(),
            ":server 002 " + client.getnickname() +
            " :Your host is ircserv, running version 1.0");
        // 003 RPL_CREATED
        (srv->*sendFn)(client.getFd(),
            ":server 003 " + client.getnickname() +
            " :This server was created recently");
        // 004 RPL_MYINFO
        (srv->*sendFn)(client.getFd(),
            ":server 004 " + client.getnickname() +
            " ircserv 1.0 o itkol");
    }
}

// ─── PASS ─────────────────────────────────────────────────────────────────────

void Server::handlePass(Client& client, const Message& msg)
{
    // Already registered
    if (client.isAuthenticated())
    {
        sendToClient(client.getFd(),
            ":server 462 " + client.getnickname() + " :You may not reregister");
        return;
    }
    // Missing parameter
    if (msg.params.empty())
    {
        sendToClient(client.getFd(),
            ":server 461 * PASS :Not enough parameters");
        return;
    }
    // Wrong password
    if (msg.params[0] != password)
    {
        sendToClient(client.getFd(),
            ":server 464 * :Password incorrect");
        removeClient(client.getFd());
        return;
    }
    client.set_password_status(true);
    checkAndFinishRegistration(this, client, &Server::sendToClient);
}

// ─── NICK ─────────────────────────────────────────────────────────────────────

void Server::handleNick(Client& client, const Message& msg)
{
    // Missing parameter
    if (msg.params.empty())
    {
        sendToClient(client.getFd(),
            ":server 431 " + client.getnickname() + " :No nickname given");
        return;
    }

    const std::string& newNick = msg.params[0];

    // Invalid format
    if (!isValidNick(newNick))
    {
        sendToClient(client.getFd(),
            ":server 432 " + client.getnickname() + " " + newNick +
            " :Erroneous nickname");
        return;
    }

    // Already in use by a different client
    Client* existing = findClientByNick(newNick);
    if (existing && existing->getFd() != client.getFd())
    {
        sendToClient(client.getFd(),
            ":server 433 " + client.getnickname() + " " + newNick +
            " :Nickname is already in use");
        return;
    }

    std::string oldPrefix = getPrefix(client);
    std::string oldNick   = client.getnickname();

    client.setNickname(newNick);
    client.set_nickname_status(true);

    // If already registered, broadcast nick change to all shared channels
    if (client.isAuthenticated())
    {
        std::string nickMsg = ":" + oldPrefix + " NICK :" + newNick;
        // Collect unique fds that share a channel with this client
        std::set<int> notified;
        notified.insert(client.getFd());
        sendToClient(client.getFd(), nickMsg);

        for (std::map<std::string, Channel>::iterator ch = channels.begin();
             ch != channels.end(); ++ch)
        {
            if (!ch->second.isMember(client.getFd()))
                continue;
            const std::set<int>& members = ch->second.getMembers();
            for (std::set<int>::const_iterator m = members.begin();
                 m != members.end(); ++m)
            {
                if (notified.find(*m) == notified.end())
                {
                    sendToClient(*m, nickMsg);
                    notified.insert(*m);
                }
            }
        }
        return;
    }

    checkAndFinishRegistration(this, client, &Server::sendToClient);
}

// ─── USER ─────────────────────────────────────────────────────────────────────

void Server::handleUser(Client& client, const Message& msg)
{
    // Already registered
    if (client.isAuthenticated())
    {
        sendToClient(client.getFd(),
            ":server 462 " + client.getnickname() + " :You may not reregister");
        return;
    }
    // Need at least 4 parameters: username mode unused :realname
    if (msg.params.size() < 4)
    {
        sendToClient(client.getFd(),
            ":server 461 * USER :Not enough parameters");
        return;
    }

    client.setUsername(msg.params[0]);
    client.setRealname(msg.params[3]);
    client.set_username_status(true);

    checkAndFinishRegistration(this, client, &Server::sendToClient);
}

// ─── JOIN ─────────────────────────────────────────────────────────────────────

void Server::handleJoin(Client& client, const Message& msg)
{
    // Not registered
    if (!client.isAuthenticated())
    {
        sendToClient(client.getFd(),
            ":server 451 * :You have not registered");
        return;
    }
    // Missing channel name
    if (msg.params.empty())
    {
        sendToClient(client.getFd(),
            ":server 461 " + client.getnickname() + " JOIN :Not enough parameters");
        return;
    }

    const std::string& chanName = msg.params[0];

    // Must start with #
    if (chanName.empty() || chanName[0] != '#')
    {
        sendToClient(client.getFd(),
            ":server 403 " + client.getnickname() + " " + chanName +
            " :No such channel");
        return;
    }

    std::string lowerName = toLower(chanName);
    bool isNew = (channels.find(lowerName) == channels.end());

    if (isNew)
    {
        // Create the channel
        channels.insert(std::make_pair(lowerName, Channel(chanName)));
    }

    Channel& channel = channels[lowerName];

    // Already a member — silently ignore
    if (channel.isMember(client.getFd()))
        return;

    if (!isNew)
    {
        // +i invite-only check
        if (channel.isInviteOnly() && !channel.isInvited(client.getFd()))
        {
            sendToClient(client.getFd(),
                ":server 473 " + client.getnickname() + " " + chanName +
                " :Cannot join channel (+i)");
            return;
        }
        // +k key check
        if (channel.hasKey())
        {
            std::string givenKey = (msg.params.size() >= 2) ? msg.params[1] : "";
            if (givenKey != channel.getKey())
            {
                sendToClient(client.getFd(),
                    ":server 475 " + client.getnickname() + " " + chanName +
                    " :Cannot join channel (+k)");
                return;
            }
        }
        // +l user limit check
        if (channel.hasUserLimit() &&
            channel.memberCount() >= static_cast<size_t>(channel.getUserLimit()))
        {
            sendToClient(client.getFd(),
                ":server 471 " + client.getnickname() + " " + chanName +
                " :Cannot join channel (+l)");
            return;
        }
    }

    // Add member (and operator if first to join)
    channel.addMember(client.getFd());
    if (isNew)
        channel.addOperator(client.getFd());

    // Remove from invite list now that they've joined
    channel.removeInvited(client.getFd());

    // Broadcast JOIN to everyone in the channel (including the joiner)
    std::string joinMsg = ":" + getPrefix(client) + " JOIN " + chanName;
    const std::set<int>& members = channel.getMembers();
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendToClient(*it, joinMsg);

    // Send topic to joining client
    if (channel.hasTopic())
        sendToClient(client.getFd(),
            ":server 332 " + client.getnickname() + " " + chanName +
            " :" + channel.getTopic());
    else
        sendToClient(client.getFd(),
            ":server 331 " + client.getnickname() + " " + chanName +
            " :No topic is set");

    // 353 RPL_NAMREPLY — list of members
    std::string namesList = "";
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it)
    {
        std::map<int, Client>::iterator cit = clients.find(*it);
        if (cit == clients.end())
            continue;
        if (!namesList.empty())
            namesList += " ";
        if (channel.isOperator(*it))
            namesList += "@";
        namesList += cit->second.getnickname();
    }
    sendToClient(client.getFd(),
        ":server 353 " + client.getnickname() + " = " + chanName +
        " :" + namesList);

    // 366 RPL_ENDOFNAMES
    sendToClient(client.getFd(),
        ":server 366 " + client.getnickname() + " " + chanName +
        " :End of /NAMES list");
}

// ─── KICK ─────────────────────────────────────────────────────────────────────

void Server::handleKick(Client& client, const Message& msg)
{
    if (!client.isAuthenticated())
    {
        sendToClient(client.getFd(), ":server 451 * :You have not registered");
        return;
    }
    if (msg.params.size() < 2)
    {
        sendToClient(client.getFd(),
            ":server 461 " + client.getnickname() + " KICK :Not enough parameters");
        return;
    }

    const std::string& chanName  = msg.params[0];
    const std::string& targetNick = msg.params[1];
    std::string reason = (msg.params.size() >= 3) ? msg.params[2] : "Kicked";

    Channel* channel = findChannel(chanName);
    if (!channel)
    {
        sendToClient(client.getFd(),
            ":server 403 " + client.getnickname() + " " + chanName +
            " :No such channel");
        return;
    }
    // Sender must be in the channel
    if (!channel->isMember(client.getFd()))
    {
        sendToClient(client.getFd(),
            ":server 442 " + client.getnickname() + " " + chanName +
            " :You're not on that channel");
        return;
    }
    // Sender must be an operator
    if (!channel->isOperator(client.getFd()))
    {
        sendToClient(client.getFd(),
            ":server 482 " + client.getnickname() + " " + chanName +
            " :You're not channel operator");
        return;
    }

    Client* target = findClientByNick(targetNick);
    if (!target || !channel->isMember(target->getFd()))
    {
        sendToClient(client.getFd(),
            ":server 441 " + client.getnickname() + " " + targetNick +
            " " + chanName + " :They aren't on that channel");
        return;
    }

    // Broadcast KICK to everyone in the channel before removing the target
    std::string kickMsg = ":" + getPrefix(client) + " KICK " + chanName +
                          " " + targetNick + " :" + reason;
    const std::set<int>& members = channel->getMembers();
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendToClient(*it, kickMsg);

    channel->removeMember(target->getFd());
}

// ─── INVITE ───────────────────────────────────────────────────────────────────

void Server::handleInvite(Client& client, const Message& msg)
{
    if (!client.isAuthenticated())
    {
        sendToClient(client.getFd(), ":server 451 * :You have not registered");
        return;
    }
    if (msg.params.size() < 2)
    {
        sendToClient(client.getFd(),
            ":server 461 " + client.getnickname() + " INVITE :Not enough parameters");
        return;
    }

    const std::string& targetNick = msg.params[0];
    const std::string& chanName   = msg.params[1];

    Channel* channel = findChannel(chanName);
    if (!channel)
    {
        sendToClient(client.getFd(),
            ":server 403 " + client.getnickname() + " " + chanName +
            " :No such channel");
        return;
    }
    // Sender must be in the channel
    if (!channel->isMember(client.getFd()))
    {
        sendToClient(client.getFd(),
            ":server 442 " + client.getnickname() + " " + chanName +
            " :You're not on that channel");
        return;
    }
    // If +i, only operators can invite
    if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
    {
        sendToClient(client.getFd(),
            ":server 482 " + client.getnickname() + " " + chanName +
            " :You're not channel operator");
        return;
    }

    Client* target = findClientByNick(targetNick);
    if (!target)
    {
        sendToClient(client.getFd(),
            ":server 401 " + client.getnickname() + " " + targetNick +
            " :No such nick/channel");
        return;
    }
    // Already in the channel
    if (channel->isMember(target->getFd()))
    {
        sendToClient(client.getFd(),
            ":server 443 " + client.getnickname() + " " + targetNick +
            " " + chanName + " :is already on channel");
        return;
    }

    channel->addInvited(target->getFd());

    // 341 RPL_INVITING to sender
    sendToClient(client.getFd(),
        ":server 341 " + client.getnickname() + " " + targetNick +
        " " + chanName);

    // INVITE notice to target
    sendToClient(target->getFd(),
        ":" + getPrefix(client) + " INVITE " + targetNick + " :" + chanName);
}

// ─── QUIT ─────────────────────────────────────────────────────────────────────

void Server::handleQuit(Client& client, const Message& msg)
{
    std::string reason = msg.params.empty() ? "Leaving" : msg.params[0];
    std::string quitMsg = ":" + getPrefix(client) + " QUIT :" + reason;

    // Broadcast to every channel this client is in
    std::set<int> notified;
    for (std::map<std::string, Channel>::iterator ch = channels.begin();
         ch != channels.end(); ++ch)
    {
        if (!ch->second.isMember(client.getFd()))
            continue;
        const std::set<int>& members = ch->second.getMembers();
        for (std::set<int>::const_iterator m = members.begin();
             m != members.end(); ++m)
        {
            if (*m != client.getFd() && notified.find(*m) == notified.end())
            {
                sendToClient(*m, quitMsg);
                notified.insert(*m);
            }
        }
    }

    removeClient(client.getFd());
}

// ─── YOUR HANDLERS ────────────────────────────────────────────────────────────
// PRIVMSG, TOPIC, MODE — implement these below.

void Server::handlePrivmsg(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}

void Server::handleTopic(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}

void Server::handleMode(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}
