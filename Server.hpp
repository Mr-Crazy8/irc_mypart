#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>

#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"

#define MAX_EVENTS 64
#define RECV_BUFFER_SIZE 4096

class Server
{
private:
    int                          port;
    std::string                  password;
    int                          server_fd;
    int                          epoll_fd;

    // Every connected client, keyed by their file descriptor.
    // Populated in run() when accept() succeeds.
    // Erased in removeClient() when the connection closes.
    std::map<int, Client>        clients;

    // Every active channel, keyed by lowercased channel name (e.g. "#general").
    // Created by your JOIN handler when a client joins a non-existent channel.
    std::map<std::string, Channel> channels;

    // ── Socket setup (unchanged from your original) ───────────────────────────
    void createSocket();
    void bindSocket();
    void startListen();
    void createEpoll();

    // ── Connection lifecycle ─────────────────────────────────────────────────
    void acceptClient();          // called from run() when server_fd is ready
    void handleClient(int fd);    // called from run() when a client fd is ready
    void removeClient(int fd);    // closes fd, erases from map, removes from channels

    // ── Message pipeline ─────────────────────────────────────────────────────
    // Step 1: accumulate bytes into client buffer, extract complete lines
    void processBuffer(Client& client);
    // Step 2: parse one raw line into a Message struct
    // Step 3: dispatch to the right handler based on message.command
    void dispatch(Client& client, const Message& msg);

    // ── Send helper ──────────────────────────────────────────────────────────
    // All command handlers use this. Never call send() directly.
    // Appends \r\n automatically. Handles partial sends.
    void sendToClient(int fd, const std::string& msg);

    // ── Utility ──────────────────────────────────────────────────────────────
    // Find a client by nickname (case-insensitive). Returns NULL if not found.
    Client* findClientByNick(const std::string& nick);
    // Find a channel by name (case-insensitive). Returns NULL if not found.
    Channel* findChannel(const std::string& name);
    // Lowercase a string (for case-insensitive comparisons)
    static std::string toLower(const std::string& s);
    // Build the IRC identity prefix for a client: "nick!user@host"
    static std::string getPrefix(Client& client);

public:
    Server(int port, const std::string& password);
    ~Server();

    void run(); // main loop — call this from main()

    // ── YOUR COMMAND HANDLERS ────────────────────────────────────────────────
    // Implement these in a separate Commands.cpp file.
    // Each receives the sending client and the fully parsed message.
    void handlePass(Client& client, const Message& msg);
    void handleNick(Client& client, const Message& msg);
    void handleUser(Client& client, const Message& msg);
    void handleJoin(Client& client, const Message& msg);
    void handlePrivmsg(Client& client, const Message& msg);
    void handleTopic(Client& client, const Message& msg);
    void handleMode(Client& client, const Message& msg);
    void handleKick(Client& client, const Message& msg);
    void handleInvite(Client& client, const Message& msg);
    void handleQuit(Client& client, const Message& msg);
};

#endif
