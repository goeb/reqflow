/*   Reqflow
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

#ifndef _parseConfig_h
#define _parseConfig_h

#include <stdint.h>
#include <string>
#include <list>

std::list<std::list<std::string> > parseConfigTokens(const char *buf, size_t len);

int loadFile(const char *filepath, const char **data);
int writeToFile(const char *filepath, const std::string &data);
int writeToFile(const char *filepath, const char *data, size_t len);

std::string serializeSimpleToken(const std::string token);
std::string serializeProperty(const std::string &key, const std::list<std::string> &values);
std::string doubleQuote(const std::string &input);
std::string popListToken(std::list<std::string> &tokens);
std::string serializeTokens(const std::list<std::list<std::string> > &linesOfTokens);



#endif
