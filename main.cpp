#include "Server.hpp"
#include <cstdlib>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return 1;
    }

    try
    {
        int port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            std::cerr << "Error: invalid port number\n";
            return 1;
        }

        std::string password = argv[2];
        if (password.empty())
        {
            std::cerr << "Error: password cannot be empty\n";
            return 1;
        }

        Server server(port, password);
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
