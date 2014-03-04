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
#include <stdlib.h>


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

/** Compare 2 strings in order to have the following ordering:
  * REQ_1.1
  * REQ_1.2
  * REQ_1.10
  * REQ_3.1.1
  * REQ_3.17
  *
  */
bool stringCompare::operator()(const std::string &s1, const std::string &s2)
{
	enum mode_t { STRING, NUMBER } mode = STRING;
	const char *l = s1.c_str();
	const char *r = s2.c_str();

	while (*l && *r) {
		if (mode == STRING) {
			while (*l && *r) {
				// check if this are digit characters
				const int l_digit = isdigit(*l);
				const int r_digit = isdigit(*r);
				// if both characters are digits, we continue in NUMBER mode
				if (l_digit && r_digit) {
					mode = NUMBER;
					break;
				}
				// if only the left character is a digit, we have a result
				if (l_digit) return true;
				// if only the right character is a digit, we have a result
				if (r_digit) return false;

				// if they differ we have a result
				if (*l < *r) return true;
				else if (*l > *r) return false;
				// otherwise process the next characters
				l++;
				r++;
			}
		} else { // mode==NUMBER
			// get the left number
			char *end;
			unsigned long l_int = strtoul(l, &end, 0);
			l = end;

			// get the right number
			unsigned long r_int = strtoul(r, &end, 0);
			r = end;

			// if the difference is not equal to zero, we have a comparison result
			if (l_int < r_int) return true;
			else if (l_int > r_int) return false;

			// otherwise we process the next substring in STRING mode
			mode = STRING;
		}
	}

	if (*r) return true;
	return false;
}

/** Double quoting is needed when the string contains:
  *    a " character
  *    \n or \r
  *    a comma ,
  */
std::string escapeCsv(const std::string &input)
{
    if (input.find_first_of("\n\r\",") == std::string::npos) return input; // no need for quotes

    size_t n = input.size();
    std::string result = "\"";

    size_t i;
    for (i=0; i<n; i++) {
        if (input[i] == '"') result += "\"\"";
        else result += input[i];
    }
    result += '"';
    return result;
}
