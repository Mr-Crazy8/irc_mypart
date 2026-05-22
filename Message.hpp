#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

// Represents one fully parsed IRC message.
// After the raw line is extracted from a client's buffer (split on \r\n),
// it is handed to parseMessage() which fills this struct.
//
// Raw IRC format:
//   [:prefix] COMMAND [param1] [param2] [...] [:trailing param]\r\n
//
// Example:
//   "PRIVMSG #general :hello world"
//     -> prefix  = ""
//     -> command = "PRIVMSG"
//     -> params  = ["#general", "hello world"]
//
// Example:
//   "MODE #general +o ayoub"
//     -> prefix  = ""
//     -> command = "MODE"
//     -> params  = ["#general", "+o", "ayoub"]

struct Message
{
    std::string              prefix;   // usually empty for client->server messages
    std::string              command;  // always uppercased
    std::vector<std::string> params;   // ordered list; trailing param is already merged
};

// Parses one raw IRC line (with \r\n already stripped) into a Message.
// Returns true if parsing produced at least a command.
// Returns false for empty or malformed lines — caller should skip them.
Message parseMessage(const std::string& rawLine);

#endif
