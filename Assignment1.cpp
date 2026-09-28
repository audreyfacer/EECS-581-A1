// extract_ipv4.cpp
//
// Reads lines of text and extracts a single valid IPv4 address, optionally
// followed by a port, embedded anywhere in the line. No string-to-number
// library conversions, no regex, no inet_* functions are used -- all
// scanning and digit accumulation is done by hand.

#include <iostream>
#include <string>

// A character is only ever part of a candidate address/port token if it is
// a digit, a period, or a colon. Anything else is garbage.
bool isAllowedChar(char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == ':';
}

// Parses the octet substring token[start, end) and, if it is a valid
// octet (1-3 digits, value 0-255, no leading zero unless the value is
// exactly "0"), stores its value in 'value' and returns true.
bool parseOctet(const std::string& token, size_t start, size_t end, int& value) {
    size_t len = end - start;
    if (len == 0 || len > 3) return false;
    if (len > 1 && token[start] == '0') return false; // disallowed leading zero

    int val = 0;
    for (size_t i = start; i < end; i++) {
        char c = token[i];
        if (c < '0' || c > '9') return false;
        val = val * 10 + (c - '0');
    }
    if (val > 255) return false;

    value = val;
    return true;
}

// Parses the port substring token[start, end) and, if it is a valid port
// (1-5 digits, value 0-65535, no leading zero unless the value is exactly
// "0"), stores its value in 'value' and returns true.
bool parsePort(const std::string& token, size_t start, size_t end, int& value) {
    size_t len = end - start;
    if (len == 0 || len > 5) return false;
    if (len > 1 && token[start] == '0') return false; // disallowed leading zero

    long val = 0;
    for (size_t i = start; i < end; i++) {
        char c = token[i];
        if (c < '0' || c > '9') return false;
        val = val * 10 + (c - '0');
        if (val > 65535) return false;
    }

    value = static_cast<int>(val);
    return true;
}

// Attempts to parse 'token' (a maximal run of digits/periods/colons) as a
// complete address[:port]. The whole token must match the grammar; partial
// matches are never accepted.
bool parseToken(const std::string& token, unsigned long& outAddress, int& outPort) {
    // Count colons and remember the (only allowed) position of one.
    size_t colonPos = std::string::npos;
    int colonCount = 0;
    for (size_t i = 0; i < token.size(); i++) {
        if (token[i] == ':') {
            colonCount++;
            if (colonCount == 1) colonPos = i;
        }
    }
    if (colonCount > 1) return false; // a second colon invalidates the token

    bool hasPort = (colonCount == 1);
    std::string addrPart = hasPort ? token.substr(0, colonPos) : token;
    std::string portPart = hasPort ? token.substr(colonPos + 1) : std::string();

    if (hasPort && portPart.empty()) return false;
    if (addrPart.empty()) return false;

    // Split addrPart on '.' by hand; there must be exactly 4 octets.
    int octets[4];
    int octetIdx = 0;
    size_t segStart = 0;
    for (size_t i = 0; i <= addrPart.size(); i++) {
        if (i == addrPart.size() || addrPart[i] == '.') {
            if (octetIdx >= 4) return false; // too many octets
            int val;
            if (!parseOctet(addrPart, segStart, i, val)) return false;
            octets[octetIdx++] = val;
            segStart = i + 1;
        }
    }
    if (octetIdx != 4) return false; // too few octets

    int portVal = -1;
    if (hasPort) {
        if (!parsePort(portPart, 0, portPart.size(), portVal)) return false;
    }

    outAddress = (static_cast<unsigned long>(octets[0]) << 24) |
                 (static_cast<unsigned long>(octets[1]) << 16) |
                 (static_cast<unsigned long>(octets[2]) << 8)  |
                  static_cast<unsigned long>(octets[3]);
    outPort = portVal;
    return true;
}

// Returns true if a valid address was found, false otherwise.
// On success: outAddress holds the 32-bit value,
// and outPort holds the port number, or -1 if no port was present.
// On failure: outAddress is set to 0 and outPort is set to -1.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort) {
    outAddress = 0;
    outPort = -1;

    size_t i = 0;
    const size_t n = str.size();
    while (i < n) {
        if (isAllowedChar(str[i])) {
            size_t start = i;
            while (i < n && isAllowedChar(str[i])) i++;
            std::string token = str.substr(start, i - start);

            unsigned long addr;
            int port;
            if (parseToken(token, addr, port)) {
                outAddress = addr;
                outPort = port;
                return true;
            }
            // This run failed the grammar in full; move on to the next run.
        } else {
            i++;
        }
    }
    return false;
}

void printAddress(unsigned long address, int port) {
    unsigned long o1 = (address >> 24) & 0xFF;
    unsigned long o2 = (address >> 16) & 0xFF;
    unsigned long o3 = (address >> 8) & 0xFF;
    unsigned long o4 = address & 0xFF;

    std::cout << "Extracted IPv4 address: " << o1 << "." << o2 << "." << o3 << "." << o4
               << " (decimal value: " << address << ", port: ";
    if (port == -1)
        std::cout << "none";
    else
        std::cout << port;
    std::cout << ")" << std::endl;
}

int main() {
    std::string line;
    while (true) {
        std::cout << "Enter a string (or 'END' to quit): ";
        if (!std::getline(std::cin, line)) break;

        if (line == "END") {
            std::cout << "Program terminated." << std::endl;
            break;
        }

        unsigned long address;
        int port;
        if (extractIPv4(line, address, port)) {
            printAddress(address, port);
        } else {
            std::cout << "Invalid input: no valid IPv4 address found" << std::endl;
        }
    }
    return 0;
}