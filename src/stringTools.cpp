/*   Req
 *   Copyright (C) 2014 Frederic Hoerni
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include <sstream>
#include <string.h>

#include "stringTools.h"
#include "global.h"


/** Remove characters at the end of string
  */
void trimRight(std::string &s, const char *c)
{
    size_t i = s.size()-1;
    while ( (i>=0) && strchr(c, s[i]) ) i--;

    if (i < 0) s = "";
    else s = s.substr(0, i+1);
}

/** Remove characters at the beginning of string
  */
void trimLeft(std::string &s, const char* c)
{
    size_t i = 0;
    while ( (s.size() > i) && strchr(c, s[i]) ) i++;

    if (i >= s.size()) s = "";
    else s = s.substr(i);
}

void trim(std::string &s, const char *c)
{
    trimLeft(s, c);
    trimRight(s, c);
}

void trimBlanks(std::string &s)
{
    trimLeft(s, "\n\t\r ");
    trimRight(s, "\n\t\r ");
}

std::string pop(std::list<std::string> & L)
{
    std::string token = "";
    if (!L.empty()) {
        token = L.front();
        L.pop_front();
    }
    return token;
}


std::vector<std::string> split(const std::string &s, const char *c, int limit)
{
    // use limit = -1 for no limit (almost)
    std::vector<std::string> tokens;
    size_t found;

    int index = 0;
    found = s.find_first_of(c, index);
    while ( (found != std::string::npos) && (limit != 0) )
    {
        tokens.push_back(s.substr(index, found-index));

        index = found + 1;
        found = s.find_first_of(c, index);
        limit --;
    }
    tokens.push_back(s.substr(index));

    return tokens;
}

std::string join(const std::list<std::string> &items, const char *separator)
{
    std::string out;
    std::list<std::string>::const_iterator i;
    FOREACH(i, items) {
        if (i != items.begin()) out += separator;
        out += (*i);
    }
    return out;
}

/** basename / -> ""
  * basename . -> .
  * basename "" -> ""
  * basename a/b/c -> c
  * basename a/b/c/ -> c
  */
std::string getBasename(const std::string &path)
{
    if (path.empty()) return "";
    size_t i;
#if defined(_WIN32)
    i = path.find_last_of("/\\");
#else
    i = path.find_last_of("/");
#endif
    if (i == std::string::npos) return path;
    else if (i == path.size()-1) return getBasename(path.substr(0, path.size()-1));
    else return path.substr(i+1);
}


std::string replaceAll(const std::string &in, char c, const char *replaceBy)
{
    std::string out;
    size_t len = in.size();
    size_t i = 0;
    size_t savedOffset = 0;
    while (i < len) {
        if (in[i] == c) {
            if (savedOffset < i) out += in.substr(savedOffset, i-savedOffset);
            out += replaceBy;
            savedOffset = i+1;
        }
        i++;
    }
    if (savedOffset < i) out += in.substr(savedOffset, i-savedOffset);
    return out;
}

