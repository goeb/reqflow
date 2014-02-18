#ifndef _stringTools_h
#define _stringTools_h

#include <string>
#include <list>
#include <map>
#include <vector>

void trimLeft(std::string & s, const char *c);
void trimRight(std::string &s, const char *c);
void trim(std::string &s, const char *c);
void trimBlanks(std::string &s);
std::string pop(std::list<std::string> & L);
std::vector<std::string> split(const std::string &s, const char *c, int limit = -1);

std::string join(const std::list<std::string> &items, const char *separator);
std::string getBasename(const std::string &path);
std::string replaceAll(const std::string &in, char c, const char *replaceBy);


#endif
