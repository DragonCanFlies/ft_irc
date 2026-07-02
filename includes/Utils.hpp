#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string trimCrlf(const std::string &s);
std::vector<std::string> splitIrcLine(const std::string &line);
int toInt(const std::string &s);
bool isValidChannelName(const std::string &name);
bool isValidNick(const std::string &nick);

#endif
