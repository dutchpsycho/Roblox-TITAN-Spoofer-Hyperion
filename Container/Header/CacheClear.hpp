#ifndef CACHE_CLEAR_HPP
#define CACHE_CLEAR_HPP

#include <string>
#include <vector>
#include <map>

extern std::vector<std::string> popularBrowsers;
extern std::map<std::string, std::vector<std::string>> browserCookiePaths;
void CacheClear();

#endif