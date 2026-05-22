#include "Server.hpp"
#include <stdexcept>
#include <cctype>
#include <algorithm>

// ─── Constructor / Destructor ─────────────────────────────────────────────────

Server::Server(int port, const std::string& pw)
    : port(port), password(pw), server_fd(-1), epoll_fd(-1)
{}

Server::~Server()
{
    if (server_fd >= 0)
        close(server_fd);
    if (epoll_fd >= 0)
        close(epoll_fd);

    // Close all client connections cleanly
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
        close(it->first);
}

// ─── Public entry point ───────────────────────────────────────────────────────

void Server::run()
{
    createSocket();
    bindSocket();
    startListen();
    createEpoll();

    struct epoll_event events[MAX_EVENTS];
    std::cout << "Server listening on port " << port << "\n";

    while (true)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0)
            throw std::runtime_error("epoll_wait failed");

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd)
                acceptClient();
            else
                handleClient(fd);
        }
    }
}

// ─── Socket setup ────────────────────────────────────────────────────────────

void Server::createSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");
}

void Server::bindSocket()
{
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("bind() failed");
    }
}

void Server::startListen()
{
    if (listen(server_fd, 128) < 0)
    {
        close(server_fd);
        throw std::runtime_error("listen() failed");
    }
}

void Server::createEpoll()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("epoll_create1() failed");

    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl() ADD server_fd failed");
}

// ─── Connection lifecycle ─────────────────────────────────────────────────────

void Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t          addrLen = sizeof(clientAddr);

    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (client_fd < 0)
        return; // non-fatal — another process may have grabbed it

    // Register with epoll
    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
    {
        close(client_fd);
        return;
    }

    // Create Client object and store it
    Client newClient(client_fd);

    // Store the client's IP as their hostname
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr)))
        newClient.sethostname(std::string(ipStr));
    else
        newClient.sethostname("unknown");

    clients.insert(std::make_pair(client_fd, newClient));

    std::cout << "New connection: fd=" << client_fd
              << " host=" << newClient.getHostname() << "\n";
}

void Server::removeClient(int fd)
{
    // Remove from every channel they were in
    for (std::map<std::string, Channel>::iterator ch = channels.begin();
         ch != channels.end(); ++ch)
    {
        ch->second.removeMember(fd);
    }

    // Remove from epoll and close
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    // Erase from client map
    clients.erase(fd);

    std::cout << "Client disconnected: fd=" << fd << "\n";
}

// ─── Message pipeline ─────────────────────────────────────────────────────────

void Server::handleClient(int fd)
{
    // Look up the client — if not found, ignore (shouldn't happen)
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    Client& client = it->second;

    // Read raw bytes into a temporary buffer
    char tmp[RECV_BUFFER_SIZE];
    int bytes = recv(fd, tmp, sizeof(tmp) - 1, 0);

    if (bytes <= 0)
    {
        // 0 = clean disconnect, negative = error
        removeClient(fd);
        return;
    }

    tmp[bytes] = '\0';

    // Append to the client's persistent buffer
    client.appendToBuffer(std::string(tmp, bytes));

    // Process whatever complete messages are now in the buffer
    processBuffer(client);
}

void Server::processBuffer(Client& client)
{
    // IRC messages are terminated by \r\n.
    // A single recv() may contain 0, 1, or many complete messages,
    // possibly followed by a partial message. We extract them one by one.

    std::string& buf = client.getBuffer();

    while (true)
    {
        // Look for \r\n (standard IRC terminator)
        size_t pos = buf.find("\r\n");

        // Also accept bare \n (nc and some clients send this)
        size_t posLF = buf.find('\n');
        if (pos == std::string::npos && posLF == std::string::npos)
            break; // no complete message yet — wait for more data

        // Use whichever terminator appears first
        size_t end;
        size_t skip; // how many bytes to remove (line + terminator)
        if (pos != std::string::npos && (posLF == std::string::npos || pos <= posLF))
        {
            end  = pos;
            skip = pos + 2; // \r\n is 2 bytes
        }
        else
        {
            end  = posLF;
            skip = posLF + 1; // bare \n is 1 byte
        }

        // Extract the line (without the terminator)
        std::string rawLine = buf.substr(0, end);

        // Remove trailing \r if present (handles \r\n split across recv calls)
        if (!rawLine.empty() && rawLine[rawLine.size() - 1] == '\r')
            rawLine.erase(rawLine.size() - 1);

        // Remove the processed bytes from the buffer
        client.removeFromBuffer(skip);

        // Skip empty lines
        if (rawLine.empty())
            continue;

        // Parse and dispatch
        Message msg = parseMessage(rawLine);
        if (!msg.command.empty())
            dispatch(client, msg);
    }
}

void Server::dispatch(Client& client, const Message& msg)
{
    const std::string& cmd = msg.command;

    // ── Pre-registration commands ─────────────────────────────────────────────
    // These must work before authentication is complete.
    if      (cmd == "PASS")    handlePass(client, msg);
    else if (cmd == "NICK")    handleNick(client, msg);
    else if (cmd == "USER")    handleUser(client, msg);

    // ── Post-registration commands ────────────────────────────────────────────
    // Your handlers should check client.isAuthenticated() and send 451 if false.
    else if (cmd == "JOIN")    handleJoin(client, msg);
    else if (cmd == "PRIVMSG") handlePrivmsg(client, msg);
    else if (cmd == "TOPIC")   handleTopic(client, msg);
    else if (cmd == "MODE")    handleMode(client, msg);
    else if (cmd == "KICK")    handleKick(client, msg);
    else if (cmd == "INVITE")  handleInvite(client, msg);
    else if (cmd == "QUIT")    handleQuit(client, msg);

    // ── Unknown command ───────────────────────────────────────────────────────
    else
    {
        // 421 ERR_UNKNOWNCOMMAND
        sendToClient(client.getFd(),
            ":server 421 " + client.getnickname() + " " + cmd + " :Unknown command");
    }
}

// ─── Send helper ──────────────────────────────────────────────────────────────

void Server::sendToClient(int fd, const std::string& msg)
{
    // IRC messages must end with \r\n
    std::string out = msg + "\r\n";

    size_t total = 0;
    size_t len   = out.size();

    // send() may not send all bytes in one call — loop until done
    while (total < len)
    {
        int sent = send(fd, out.c_str() + total, len - total, 0);
        if (sent <= 0)
            break; // client disconnected or error — caller doesn't need to handle
        total += sent;
    }
}

// ─── Utility ──────────────────────────────────────────────────────────────────

std::string Server::toLower(const std::string& s)
{
    std::string result = s;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}

// Finds a client by nickname (case-insensitive).
// Returns a pointer to the Client in the map, or NULL if not found.
// Used by PRIVMSG (direct message), MODE +o, KICK, INVITE.
Client* Server::findClientByNick(const std::string& nick)
{
    std::string lowerNick = toLower(nick);
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (toLower(it->second.getnickname()) == lowerNick)
            return &it->second;
    }
    return NULL;
}

// Finds a channel by name (case-insensitive).
// Returns a pointer to the Channel in the map, or NULL if not found.
// Used by PRIVMSG, TOPIC, MODE, KICK, INVITE.
Channel* Server::findChannel(const std::string& name)
{
    std::string lower = toLower(name);
    std::map<std::string, Channel>::iterator it = channels.find(lower);
    if (it == channels.end())
        return NULL;
    return &it->second;
}

// Builds the IRC identity prefix for a client.
// Format: "nickname!username@hostname"
// Used in every outgoing message that needs to identify the sender.
// Example: ":ayoub!ayoub@192.168.1.5 PRIVMSG #general :hello\r\n"
std::string Server::getPrefix(Client& client)
{
    return client.getnickname() + "!" +
           client.getUsername() + "@" +
           client.getHostname();
}
