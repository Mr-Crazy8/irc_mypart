#include "Message.hpp"
#include <cctype>

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Converts a string to uppercase in-place (C++98 compatible).
static void toUpperCase(std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
}

// ─── Parser ──────────────────────────────────────────────────────────────────

// IRC message grammar (simplified):
//
//   message  = [ ":" prefix SPACE ] command [ params ] CRLF
//   prefix   = servername / ( nickname [ "!" user ] [ "@" host ] )
//   command  = letter { letter } / 3digit
//   params   = *14( SPACE middle ) [ SPACE ":" trailing ]
//            / 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
//
// The key rule for params:
//   - Words are split on spaces.
//   - If a word starts with ":", everything from that colon to end-of-line
//     is ONE parameter (the "trailing"). Stop splitting after that.

Message parseMessage(const std::string& rawLine)
{
    Message msg;

    if (rawLine.empty())
        return msg;

    size_t pos = 0;
    size_t len = rawLine.size();

    // ── 1. Optional prefix (:prefix) ─────────────────────────────────────────
    if (rawLine[0] == ':')
    {
        size_t spacePos = rawLine.find(' ', pos);
        if (spacePos == std::string::npos)
            return msg; // malformed — nothing after prefix
        msg.prefix = rawLine.substr(1, spacePos - 1); // strip the leading ':'
        pos = spacePos + 1;
    }

    // ── 2. Command ────────────────────────────────────────────────────────────
    {
        size_t spacePos = rawLine.find(' ', pos);
        if (spacePos == std::string::npos)
        {
            // Command with no params — rest of line is the command
            msg.command = rawLine.substr(pos);
            toUpperCase(msg.command);
            return msg;
        }
        msg.command = rawLine.substr(pos, spacePos - pos);
        toUpperCase(msg.command);
        pos = spacePos + 1;
    }

    // ── 3. Parameters ─────────────────────────────────────────────────────────
    while (pos < len)
    {
        // Skip multiple consecutive spaces (be lenient with clients)
        while (pos < len && rawLine[pos] == ' ')
            ++pos;
        if (pos >= len)
            break;

        // Trailing parameter — starts with ':'
        // Everything from here to end of line is one single param
        if (rawLine[pos] == ':')
        {
            msg.params.push_back(rawLine.substr(pos + 1)); // strip the ':'
            break; // trailing is always last
        }

        // Middle parameter — read until next space
        size_t spacePos = rawLine.find(' ', pos);
        if (spacePos == std::string::npos)
        {
            msg.params.push_back(rawLine.substr(pos));
            break;
        }
        msg.params.push_back(rawLine.substr(pos, spacePos - pos));
        pos = spacePos + 1;
    }

    return msg;
}
