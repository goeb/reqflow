#ifndef _req_h
#define _req_h

#include <regex.h>
#include <string>
#include <map>
#include <list>
#include <set>


#define DEFAULT_CONF "req.conf"

enum Encoding { UTF8, LATIN1 };

struct ReqFileConfig {
    std::string id;
    std::string path;
    std::string tagPattern;
    regex_t *tagRegex;
    std::string refPattern;
    regex_t *refRegex;
    std::list<std::string> dependencies;
    int nTotalRequirements;
    int nCoveredRequirements;
	Encoding encoding;

    ReqFileConfig(): tagRegex(0), refRegex(0), nTotalRequirements(0), nCoveredRequirements(0),
		encoding(UTF8) {}
};

struct Requirement {
    std::string id;
    std::string parentDocumentId;
    std::string parentDocumentPath;
    std::set<std::string> covers;
    std::set<std::string> coveredBy;
};

extern std::map<std::string, ReqFileConfig> ReqConfig;
extern std::map<std::string, Requirement> Requirements;

std::string getMatchingPattern(regex_t *regex, const char *text);


#endif
