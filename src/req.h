#ifndef _req_h
#define _req_h

#include <pcreposix.h>
#include <string>
#include <map>
#include <list>
#include <set>


#define DEFAULT_CONF "conf.req"

enum Encoding { UTF8, LATIN1 };
enum BlockStatus { NOT_STARTED, STOP_REACHED, REQ_OK };

struct ReqFileConfig {
    std::string id;
    std::string path;
    std::string reqPattern;
    regex_t *reqRegex;
    std::string refPattern;
    regex_t *refRegex;
	std::string startAfter;
	regex_t *startAfterRegex;
	std::string stopAfter;
	regex_t *stopAfterRegex;
    std::list<std::string> dependencies;

    // for dependency graph
    std::set<std::string> upstreamDocuments;
    std::set<std::string> downstreamDocuments;

    int nTotalRequirements;
    int nCoveredRequirements;
	Encoding encoding;

	// indicate if no coverage check should be done on this document
	// ie: no forward traceability is printed and no uncovered status neither
	bool nocov;


    ReqFileConfig(): reqRegex(0), refRegex(0), startAfterRegex(0), stopAfterRegex(0),
		nTotalRequirements(0), nCoveredRequirements(0), encoding(UTF8), nocov(false) {}
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
extern std::list<std::string> Errors;
extern int ReqTotal;
extern int ReqCovered;


class ReqDocument {
public:
    virtual int loadRequirements(bool debug) = 0;
    BlockStatus processBlock(const std::string &text);
protected:
    void init();
    bool acquisitionStarted; // indicate if the parsing passed the point after which requirement may be acquired
    std::string currentRequirement;
    std::string currentText;
    ReqFileConfig fileConfig;
};

#define BF_SZ 1024
#define PUSH_ERROR(...) do { \
    char buffer[BF_SZ]; \
    snprintf(buffer, BF_SZ, __VA_ARGS__); \
    Errors.push_back(buffer); \
    } while(0)

// exported functions

void dumpText(const char *text);
void printErrors();
Requirement *getRequirement(std::string id);
ReqFileConfig *getDocument(std::string docId);
void consolidateCoverage();
void checkUndefinedRequirements();
std::string getMatchingPattern(regex_t *regex, const std::string &text);
std::string getMatchingPattern(regex_t *regex, const char *text);
void computeGlobalStatistics();


#endif
