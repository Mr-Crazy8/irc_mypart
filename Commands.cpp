#include "Server.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// THIS FILE IS YOURS TO IMPLEMENT.
//
// All command handlers live here. Each function receives:
//   - client : reference to the Client object that sent the message
//   - msg    : the fully parsed Message (msg.params[0], msg.params[1], etc.)
//
// Use sendToClient(fd, string) to send any response.
// Use getPrefix(client) to build ":nick!user@host" for outgoing messages.
// Use findClientByNick(nick) to find another client by nickname.
// Use findChannel(name) to find a channel by name.
// channels["#name"] to create/access a channel in the map.
// ─────────────────────────────────────────────────────────────────────────────


// PASS — verify the connection password
// msg.params[0] = the password the client sent
//
// What to do:
//   1. If client is already authenticated, send 462 (already registered)
//   2. If params is empty, send 461 (not enough params)
//   3. Compare params[0] with server password
//   4. If correct: client.set_password_status(true)
//   5. If wrong:   send 464 (password incorrect) — you may also close the connection
//   6. After setting password true, check if nick and user are also done.
//      If all three are true: client.setAuthenticated(true) and send 001 welcome.
void Server::handlePass(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// NICK — set or change the client's nickname
// msg.params[0] = desired nickname
//
// What to do:
//   1. If params is empty, send 431 (no nickname given)
//   2. Validate nickname format (letters, digits, some special chars — no spaces)
//      If invalid, send 432 (erroneous nickname)
//   3. Check if the nickname is already taken by another client (findClientByNick)
//      If taken, send 433 (nickname already in use)
//   4. Set the nickname: client.setNickname(params[0])
//      client.set_nickname_status(true)
//   5. If client is already authenticated and changes nick, broadcast to all
//      channels they are in: ":oldnick!user@host NICK :newnick"
//   6. Check if all three steps (pass, nick, user) are done.
//      If yes: client.setAuthenticated(true) and send 001 welcome.
void Server::handleNick(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// USER — set username and realname (called once during registration)
// msg.params[0] = username
// msg.params[1] = mode (usually "0", can be ignored)
// msg.params[2] = unused (usually "*")
// msg.params[3] = realname (trailing param)
//
// What to do:
//   1. If client is already authenticated, send 462 (already registered)
//   2. If params.size() < 4, send 461 (not enough params)
//   3. client.setUsername(params[0])
//      client.setRealname(params[3])
//      client.set_username_status(true)
//   4. Check if all three steps are done.
//      If yes: client.setAuthenticated(true) and send 001 welcome.
void Server::handleUser(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// JOIN — join or create a channel
// msg.params[0] = channel name (e.g. "#general")
// msg.params[1] = channel key, if any (optional)
//
// What to do:
//   1. Check client.isAuthenticated() — send 451 if not
//   2. Check params[0] exists and starts with '#' — send 461 if missing
//   3. findChannel(params[0]):
//      - If exists and has +i mode:   check isInvited — send 473 if not
//      - If exists and has +k mode:   check params[1] matches key — send 475 if not
//      - If exists and has +l mode:   check memberCount < limit — send 471 if full
//      - If not exists:               create it: channels[lowercase_name] = Channel(name)
//   4. channel.addMember(client.getFd())
//   5. If channel was just created, also: channel.addOperator(client.getFd())
//   6. Send to the joining client:
//      ":nick!user@host JOIN #channel"
//   7. If channel has a topic, send 332 to the joining client
//      Otherwise send 331
//   8. Send 353 (NAMES list) and 366 (end of names) to the joining client
void Server::handleJoin(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// PRIVMSG — send a message to a channel or a user
// msg.params[0] = target (channel name or nickname)
// msg.params[1] = message text
//
// What to do:
//   See the detailed plan already discussed.
//   Use getPrefix(client) for the sender identity.
//   Use findChannel() for channel targets.
//   Use findClientByNick() for nickname targets.
//   Use sendToClient() for delivery.
//   Remember: sender does NOT receive their own channel message.
void Server::handlePrivmsg(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// TOPIC — view or change the topic of a channel
// msg.params[0] = channel name
// msg.params[1] = new topic (optional — if absent, just view)
//
// What to do:
//   See the detailed plan already discussed.
void Server::handleTopic(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// MODE — change channel modes
// msg.params[0] = channel name
// msg.params[1] = mode string (e.g. "+i", "-k", "+o")
// msg.params[2] = mode argument if needed (key, nickname, limit)
//
// What to do:
//   See the detailed plan already discussed.
//   Handle: +i -i +t -t +k -k +o -o +l -l
void Server::handleMode(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// KICK — eject a client from a channel
// msg.params[0] = channel name
// msg.params[1] = nickname to kick
// msg.params[2] = kick reason (optional)
//
// What to do:
//   1. Check authenticated
//   2. findChannel — send 403 if not found
//   3. Check sender is operator — send 482 if not
//   4. findClientByNick(params[1]) — send 441 if not in channel
//   5. channel.removeMember(target fd)
//   6. Broadcast to all channel members (including kicked):
//      ":nick!user@host KICK #channel targetNick :reason"
void Server::handleKick(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// INVITE — invite a client to a channel
// msg.params[0] = nickname to invite
// msg.params[1] = channel name
//
// What to do:
//   1. Check authenticated
//   2. findChannel — send 403 if not found
//   3. Check sender is member — send 442 if not
//   4. If channel is +i: check sender is operator — send 482 if not
//   5. findClientByNick(params[0]) — send 401 if not found
//   6. channel.addInvited(target fd)
//   7. Send 341 to sender: ":server 341 senderNick targetNick #channel"
//   8. Send to target: ":nick!user@host INVITE targetNick :#channel"
void Server::handleInvite(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}


// QUIT — client disconnects gracefully
// msg.params[0] = quit message (optional)
//
// What to do:
//   1. Build quit reason string (use params[0] if present, else "Leaving")
//   2. Broadcast to all channels the client is in:
//      ":nick!user@host QUIT :reason"
//   3. Call removeClient(client.getFd())
void Server::handleQuit(Client& client, const Message& msg)
{
    (void)client;
    (void)msg;
    // YOUR CODE HERE
}
