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

#ifndef _req_h
#define _req_h

#include <pcreposix.h>
#include <string>
#include <map>
#include <list>
#include <set>

#include "stringTools.h"

#define DEFAULT_CONF "conf.req"

enum Encoding { UTF8, LATIN1 };
enum BlockStatus { NOT_STARTED, STOP_REACHED, REQ_OK };
enum ReqSorting { SORT_DOCUMENT, SORT_ALPHANUMERIC };

struct Requirement;

enum ReqFileType { RF_TEXT, RF_ODT, RF_DOCX, RF_XSLX, RF_DOCX_XML, RF_HTML, RF_PDF, RF_UNKNOWN };


/** Functor for comparing requirements
  *
  * Usable in std::sort or containers std::set and std::map.
  */
struct ReqCompare
{
    bool operator()(const Requirement *a, const Requirement *b);
};

struct ReqFileConfig {
    std::string id;
    std::string path; // path setup in .req file
    std::string realpath; // path computed relatively to the dir of .req file
    std::string reqPattern;
    regex_t *reqRegex;
    std::string refPattern;
    regex_t *refRegex;
	std::string startAfter;
	regex_t *startAfterRegex;
	std::string stopAfter;
	regex_t *stopAfterRegex;
    std::map<std::string, regex_t *> endReq;
    std::map<std::string, regex_t *> endReqStyle;
    ReqFileType type;
    enum SortMode { SORT_DOCUMENT_ORDER, SORT_ALPHANUMERIC_ORDER, SORT_UNKNOWN };
    SortMode sortMode;

    // for dependency graph
    std::set<std::string> upstreamDocuments;
    std::set<std::string> downstreamDocuments;

    // pointers to requirements stored in the global map 'Requirements'
    // TODO replace by a std::set<Requirement*> and have the compare function
    // chose either document order or alphanumeric order
    std::set<Requirement*, ReqCompare> requirements; // use a map to keep them sorted
    int nTotalRequirements;
    int nCoveredRequirements;
	Encoding encoding;

	// indicate if no coverage check should be done on this document
	// ie: no forward traceability is printed and no uncovered status neither
	bool nocov;


    ReqFileConfig(): reqRegex(0), refRegex(0), startAfterRegex(0), stopAfterRegex(0), type(RF_UNKNOWN),
        nTotalRequirements(0), nCoveredRequirements(0), encoding(UTF8), nocov(false) {}
    static ReqFileType getFileType(const std::string &extension);
    SortMode getSortMode(const std::string &text);

    ReqFileType getFileType();
};

struct Requirement {
    std::string id;
    int seqnum; // sequence number in document order
    ReqFileConfig *parentDocument;
    std::set<std::string> covers;
    std::set<std::string> coveredBy;
    std::string text;
};



extern std::map<std::string, ReqFileConfig*> ReqConfig;
// storage for all requirements
// this storage needs not be sorted in a specific manner
// pointers to the requirements are also kept in the ReqFileConfig objects
extern std::map<std::string, Requirement> Requirements;
extern std::map<std::string, std::list<std::pair<std::string, std::string> > > Errors; // errors indexed by file
extern int ReqTotal;
extern int ReqCovered;

int getErrorNumber();
int hasErrors(const std::string &file);

class ReqDocument {
public:
    ReqDocument() { reqSeqnum = 0; }
    virtual int loadRequirements(bool debug) = 0;
    BlockStatus processBlock(std::string &text);
    void finalizeCurrentReq();
protected:
    virtual void init();
    bool acquisitionStarted; // indicate if the parsing passed the point after which requirement may be acquired
    std::string currentRequirement;
    std::string textOfCurrentReq;
    std::string currentText;
    ReqFileConfig *fileConfig;
    int reqSeqnum; // last sequence number used for a requirement
};

#define BF_SZ 1024
#define PUSH_ERROR(_file, _req, ...) do { \
    char buffer[BF_SZ]; \
    snprintf(buffer, BF_SZ, __VA_ARGS__); \
    Errors[_file].push_back(std::make_pair(_req, buffer)); \
    } while(0)

enum PolicyEraseExtracted { ERASE_NONE, ERASE_ALL, ERASE_LEAST };

// exported functions

void dumpText(const char *text);
void printErrors();
Requirement *getRequirement(std::string id);
ReqFileConfig *getDocument(std::string docId);
void consolidateCoverage();
void checkUndefinedRequirements();
std::string extractPattern(regex_t *regex, std::string &text, PolicyEraseExtracted erase = ERASE_NONE);
std::set<std::string> extractAllPatterns(regex_t *regex, std::string &text);

void computeGlobalStatistics();


#endif
