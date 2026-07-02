#include "Utils.hpp"
#include <sstream>
#include <cstdlib>
#include <cctype>

std::string trimCrlf(const std::string &s) {
    std::string result = s;
    while (!result.empty() && (result[result.size() - 1] == '\r' || result[result.size() - 1] == '\n'))
        result.erase(result.size() - 1);
    return result;
}

std::vector<std::string> splitIrcLine(const std::string &line) {
    std::vector<std::string> out;
    std::string s = trimCrlf(line);
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && s[i] == ' ') i++;
        if (i >= s.size()) break;
        if (s[i] == ':') {
            out.push_back(s.substr(i + 1));
            break;
        }
        size_t j = i;
        while (j < s.size() && s[j] != ' ') j++;
        out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

int toInt(const std::string &s) {
    std::stringstream ss(s);
    int n;
    ss >> n;
    if (ss.fail()) return -1;
    return n;
}

bool isValidChannelName(const std::string &name) {
    return name.size() >= 2 && name[0] == '#';
}

bool isValidNick(const std::string &nick) {
    if (nick.empty()) return false;
    for (size_t i = 0; i < nick.size(); ++i) {
        char c = nick[i];
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^'))
            return false;
    }
    return true;
}
