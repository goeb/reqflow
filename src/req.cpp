#include <stdarg.h>
#include <list>
#include <string>
#include <stdio.h>

#include "req.h"
#include "global.h"
#include "logging.h"

// static objects
std::map<std::string, ReqFileConfig> ReqConfig;
std::map<std::string, Requirement, stringCompare> Requirements;
std::list<std::string> Errors;
int ReqTotal = 0;
int ReqCovered = 0;

void dumpText(const char *text)
{
    printf("%s\n", text);
}

void printErrors()
{
    if (Errors.empty()) fprintf(stderr, "Ok.\n");
    else {
        fprintf(stderr, "Error(s): %d\n", Errors.size());
        std::list<std::string>::iterator e;
        FOREACH(e, Errors) {
            fprintf(stderr, "%s\n", e->c_str());
        }
    }
}

void ReqDocument::init()
{
    acquisitionStarted = true;
    if (fileConfig.startAfterRegex) acquisitionStarted = false;
    currentRequirement = "";
}


Requirement *getRequirement(std::string id)
{
    std::map<std::string, Requirement>::iterator req = Requirements.find(id);
    if (req == Requirements.end()) return 0;
    else return &(req->second);
}

ReqFileConfig *getDocument(std::string docId)
{
    std::map<std::string, ReqFileConfig>::iterator doc = ReqConfig.find(docId);
    if (doc == ReqConfig.end()) return 0;
    else return &(doc->second);
}


/** Process a block of text (a line or paragraph)
  *
  * This block is searched through for:
  *    - reference to requirements
  *    - requirements
  *    - accumulation of text of the current requirement
  *
  * Contextual variables:
  *    acquisitionStarted
  *    currentRequirement
  */

BlockStatus ReqDocument::processBlock(const std::string &text)
{
	LOG_DEBUG("processBlock: %s", text.c_str());
    // check the startAfter pattern
    if (!acquisitionStarted) {
        std::string start = getMatchingPattern(fileConfig.startAfterRegex, text);
        if (!start.empty()) acquisitionStarted = true;
    }

    if (!acquisitionStarted) return NOT_STARTED;

    // check the stopAfter pattern
    std::string stop = getMatchingPattern(fileConfig.stopAfterRegex, text);
    if (!stop.empty()) {
        LOG_DEBUG("stop: %s", stop.c_str());
        LOG_DEBUG("line: %s", text.c_str());
        return STOP_REACHED;
    }

    // check if text covers a requirement
    std::set<std::string> refs = getAllPatterns(fileConfig.refRegex, text.c_str());
    if (!refs.empty()) {
        std::set<std::string>::iterator ref;
        FOREACH(ref, refs) {
            if (currentRequirement.empty()) {
                PUSH_ERROR("%s: Reference, but no current requirement, file: %s",
                           ref->c_str(), fileConfig.path.c_str());
            } else {
                Requirements[currentRequirement].covers.insert(*ref);
            }
        }
    }

    std::string reqId = getMatchingPattern(fileConfig.reqRegex, text);

    if (!reqId.empty() && refs.find(reqId) == refs.end()) {
        std::map<std::string, Requirement>::iterator r = Requirements.find(reqId);
        if (r != Requirements.end()) {
            PUSH_ERROR("%s: Duplicate requirement in documents '%s' and '%s'",
                      reqId.c_str(), r->second.parentDocumentPath.c_str(), fileConfig.path.c_str());
            currentRequirement.clear();

        } else {
            Requirement req;
            req.id = reqId;
            req.parentDocumentId = fileConfig.id;
            req.parentDocumentPath = fileConfig.path;
            Requirements[reqId] = req;
            currentRequirement = reqId;
        }
    }
    return REQ_OK;
}


std::string getMatchingPattern(regex_t *regex, const std::string &text)
{
    return getMatchingPattern(regex, text.c_str());
}

std::string getMatchingPattern(regex_t *regex, const char *text)
{
    if (!regex) return "";

    std::string matchingText;
    // check if line matches
    const int N = 5; // max number of groups
    regmatch_t pmatch[N];
    //LOG_DEBUG("regexec(%p, %p)", regex, text);
    int reti = regexec(regex, text, N, pmatch, 0);
    if (!reti) {
        // take the last group, because we want to support the 2 following cases.
        // Example 1: ./req regex "<\(REQ_[-a-zA-Z_0-9]*\)>" "ex: <REQ_123> (comment)"
        // match[0]: <REQ_123>
        // match[1]: REQ_123
        // match[2]:
        //
        // Example 2: ./req regex "REQ_[-a-zA-Z_0-9]*" "ex: REQ_123 (comment)"
        // match[0]: REQ_123
        // match[1]:
        // match[2]:

        int i;
        const int LINE_SIZE_MAX = 4096;
        char buffer[LINE_SIZE_MAX];
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                if (length > LINE_SIZE_MAX-1) {
                    PUSH_ERROR("Requirement size too big (%d)", length);
                    break;
                }
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                //LOG_DEBUG("match[%d]: %s", i, buffer);
                matchingText = buffer; // overwrite each time, so that we take the last one

            } else break; // no more groups
        }

    } else if (reti == REG_NOMATCH) {
        matchingText = "";

    } else {
        char msgbuf[1024];
        regerror(reti, regex, msgbuf, sizeof(msgbuf));
        PUSH_ERROR("Regex match failed: %s", msgbuf);
        matchingText = "";
    }

    return matchingText;
}

std::set<std::string> getAllPatterns(regex_t *regex, const char *text)
{
    std::set<std::string> result;

    if (!regex) return result;

    // check if line matches
    const int N = 5; // max number of groups
    regmatch_t pmatch[N];
    std::string localText = text;
    int reti;
    while ( !(reti = regexec(regex, localText.c_str(), N, pmatch, 0)) ) {
        LOG_DEBUG("getAllPatterns: localText=%s", localText.c_str());
        // match
        // take the last group

        int i;
        const int LINE_SIZE_MAX = 4096;
        char buffer[LINE_SIZE_MAX];
        for (i=N-1; i>=0; i--) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                if (length <= 0) {
                    localText.clear();
                    break;
                }
                if (length > LINE_SIZE_MAX-1) {
                    PUSH_ERROR("Requirement size too big (%d)", length);
                    localText.clear();
                    break;
                }
                memcpy(buffer, localText.c_str()+pmatch[i].rm_so, length);
                buffer[length] = 0;
                result.insert(buffer);
                LOG_DEBUG("getAllPatterns: got pattern=%s", buffer);

                // modify localText
                localText.erase(pmatch[i].rm_so, length);
                break;
            }
        }
    }

    return result;
}

/** Fulfill .coveredBy tables
  */
void consolidateCoverage()
{
    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        std::set<std::string>::iterator c;
        FOREACH(c, r->second.covers) {
            Requirement *req = getRequirement(*c);
            if (req) {
                req->coveredBy.insert(r->second.id);

                // compute documents dependencies
                ReqFileConfig *fdown = getDocument(r->second.parentDocumentId);
                ReqFileConfig *fup = getDocument(req->parentDocumentId);
                if (!fdown) PUSH_ERROR("Cannot find document: %s", r->second.parentDocumentId.c_str());
                else if (!fup) PUSH_ERROR("Cannot find document: %s", req->parentDocumentId.c_str());
                else {
                    fdown->upstreamDocuments.insert(fup->id);
                    fup->downstreamDocuments.insert(fdown->id);
                }
            }
        }
    }

}

/** Check that all referenced requirements exist
  */
void checkUndefinedRequirements()
{
    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        std::set<std::string>::iterator c;
        FOREACH(c, r->second.covers) {
            if (!getRequirement(*c)) {
                PUSH_ERROR("%s: Undefined requirement, referenced by: %s (%s)",
                          c->c_str(), r->second.id.c_str(), r->second.parentDocumentPath.c_str());
            }
        }
    }
}

void computeGlobalStatistics()
{
    // compute global statistics
    std::map<std::string, ReqFileConfig>::iterator file;
    std::map<std::string, Requirement>::iterator req;
    FOREACH(req, Requirements) {
        file = ReqConfig.find(req->second.parentDocumentId);
        if (file == ReqConfig.end()) {
            PUSH_ERROR("%s: Cannot find parent document", req->second.id.c_str());
            continue;
        }
        file->second.nTotalRequirements++;
        if (!req->second.coveredBy.empty()) {
            file->second.nCoveredRequirements++;
        }
    }
}
