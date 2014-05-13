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
#include "config.h"

#include <stdarg.h>
#include <list>
#include <string>
#include <stdio.h>

#include "req.h"
#include "global.h"
#include "logging.h"

// static objects
std::map<std::string, ReqFileConfig*> ReqConfig;
std::map<std::string, Requirement, stringCompare> Requirements;
std::map<std::string, std::list<std::pair<std::string, std::string> > > Errors; // errors indexed by file
int ReqTotal = 0;
int ReqCovered = 0;

void dumpText(const char *text)
{
    printf("%s\n", text);
}

int getErrorNumber()
{
    int n = 0;
    std::map<std::string, std::list<std::pair<std::string, std::string> > >::iterator file;
    FOREACH(file, Errors) {
        std::list<std::pair<std::string, std::string> >::iterator e;
        FOREACH(e, file->second) {
            n++;
        }
    }
    return n;
}

int hasErrors(const std::string &file)
{
    std::map<std::string, std::list<std::pair<std::string, std::string> > >::iterator f;
    f = Errors.find(file);
    if (f != Errors.end()) return f->second.size();
    else return 0;
}

void printErrors()
{
    if (!Errors.empty()) {
        fprintf(stderr, "Error(s): %d\n", getErrorNumber());
        std::map<std::string, std::list<std::pair<std::string, std::string> > >::iterator file;
        FOREACH(file, Errors) {
            std::list<std::pair<std::string, std::string> >::iterator e;
            FOREACH(e, file->second) {
                fprintf(stderr, "%s:%s: %s\n", file->first.c_str(), e->first.c_str(), e->second.c_str());
            }
        }
    }
}

ReqFileType ReqFileConfig::getFileType(const std::string &extension)
{
    if (0 == strcasecmp(extension.c_str(), "txt")) return RF_TEXT;
    else if (0 == strcasecmp(extension.c_str(), "odt")) return RF_ODT;
    else if (0 == strcasecmp(extension.c_str(), "docx")) return RF_DOCX;
    else if (0 == strcasecmp(extension.c_str(), "xslx")) return RF_XSLX;
    else if (0 == strcasecmp(extension.c_str(), "xml")) return RF_DOCX_XML;
    else if (0 == strcasecmp(extension.c_str(), "htm")) return RF_HTML;
    else if (0 == strcasecmp(extension.c_str(), "html")) return RF_HTML;
    else if (0 == strcasecmp(extension.c_str(), "pdf")) return RF_PDF;
    else return RF_UNKNOWN;
}

ReqFileType ReqFileConfig::getFileType()
{
    if (type != RF_UNKNOWN) return type;

    size_t i = path.find_last_of('.');
    if (i == std::string::npos) return RF_UNKNOWN;
    if (i == path.size()-1) return RF_UNKNOWN;
    std::string extension = path.substr(i+1);
    return getFileType(extension);
}

void ReqDocument::init()
{
    acquisitionStarted = true;
    if (fileConfig->startAfterRegex) acquisitionStarted = false;
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
    std::map<std::string, ReqFileConfig*>::iterator doc = ReqConfig.find(docId);
    if (doc == ReqConfig.end()) return 0;
    else return doc->second;
}


/** Process a block of text (a line or paragraph)
  *
  * This block is searched through for:
  *    - reference to requirements
  *    - requirements
  *    - accumulation of text of the current requirement
  *
  * Lexical rules:
  *   - references must be after the requirement and not on the same line
  *
  * Contextual variables:
  *    acquisitionStarted
  *    currentRequirement
  */

BlockStatus ReqDocument::processBlock(std::string &text)
{
	LOG_DEBUG("processBlock: %s", text.c_str());
    // check the startAfter pattern
    if (!acquisitionStarted) {
        std::string start = extractPattern(fileConfig->startAfterRegex, text);
        if (!start.empty()) acquisitionStarted = true;
    }

    if (!acquisitionStarted) return NOT_STARTED;

    // check the stopAfter pattern
    std::string stop = extractPattern(fileConfig->stopAfterRegex, text);
    if (!stop.empty()) {
        LOG_DEBUG("stop: %s", stop.c_str());
        LOG_DEBUG("line: %s", text.c_str());

        finalizeCurrentReq();
        return STOP_REACHED;
    }

    // check if text covers a requirement
    std::set<std::string> refs = extractAllPatterns(fileConfig->refRegex, text);

    std::string reqId = extractPattern(fileConfig->reqRegex, text, ERASE_ALL);

    if (!reqId.empty() && refs.find(reqId) == refs.end()) {
        std::map<std::string, Requirement>::iterator r = Requirements.find(reqId);
        if (r != Requirements.end()) {
            PUSH_ERROR(fileConfig->id, reqId, "Duplicate requirement: also defined in '%s'",
                       r->second.parentDocument->path.c_str());
            currentRequirement.clear();

        } else {

            finalizeCurrentReq(); // finalize current req before starting a new one

            Requirement req;
            req.id = reqId;
            req.parentDocument = fileConfig;
            Requirements[reqId] = req;
            fileConfig->requirements[reqId]= &(Requirements[reqId]);
            currentRequirement = reqId;
        }
    }

    // refs are stored after capturing a possible requirement
    // this is to deal with the case where the req and refs are on the same line
    if (!refs.empty()) {
        std::set<std::string>::iterator ref;
        FOREACH(ref, refs) {
            if (currentRequirement.empty()) {
                PUSH_ERROR(fileConfig->id, *ref, "Reference without requirement");
            } else {
                Requirements[currentRequirement].covers.insert(*ref);
            }
        }
    }

    // TODO check for endReq and endReqStyle to know if text of
    // current requirement is finished

    // accumulate text of current requirement
    if (currentRequirement.size()) {
        if (textOfCurrentReq.size()) textOfCurrentReq += "\n";
        textOfCurrentReq += text;
    }

    return REQ_OK;
}

/** finalize() mainly pushes the text of the last requirement, if any
  */
void ReqDocument::finalizeCurrentReq()
{
    if (currentRequirement.size() && textOfCurrentReq.size()) {
        trimBlanks(textOfCurrentReq);
        Requirements[currentRequirement].text = textOfCurrentReq;
    }
    textOfCurrentReq.clear();
    currentRequirement.clear();
}


/** Get first matching pattern
  *
  * The erase flag is typically used as:
  *    for a requirement, erase the whole pattern, as requirements go one by one
  *    for a reference, erase the least pattern, as there may be several refs on the same line.
  *
  * Example:
  * TEST_01: ... Ref: REQ_01, REQ_02, REQ_03
  *
  * with REF_PATTERN = "Ref:[ ,]+(REQ_[0-9]+)"
  *
  * In this example, when extracting the refs, the extraction sequence will be:
  *    Ref: REQ_01, REQ_02, REQ_03
  *    Ref: , REQ_02, REQ_03
  *    Ref: , , REQ_03
  *    Ref: , ,
  * @return
  *     if not found, empty string
  *     otherwise, extracted text
  */
std::string extractPattern(regex_t *regex, std::string &text, PolicyEraseExtracted erase)
{
    LOG_DEBUG("extractPattern: textin=%s, erase=%d", text.c_str(), erase);

    if (!regex) return "";

    std::string result;
    const int N = 5; // max number of groups
    regmatch_t pmatch[N];
    int reti = regexec(regex, text.c_str(), N, pmatch, 0);
    if (reti == 0) {
        // take the last group, but erase (optionally) the first group
        // we want to support the 2 following cases.
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
        for (i=N-1; i>=0; i--) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                if (length <= 0) {
                    break;
                }
                if (length > LINE_SIZE_MAX-1) {
                    PUSH_ERROR("", "", "Requirement size too big (%d)", length);
                    break;
                }
                memcpy(buffer, text.c_str()+pmatch[i].rm_so, length);
                buffer[length] = 0;
                result = buffer;
                break;
            }
        }

        if (erase == ERASE_ALL) {
            // erase first group
            regmatch_t firstGroup = pmatch[0];
            if (firstGroup.rm_so != -1) {
                int length = firstGroup.rm_eo - firstGroup.rm_so;
                text.erase(firstGroup.rm_so, length);
            }
        } else if (erase == ERASE_LEAST) {
            // erase last group
            regmatch_t lastGroup = pmatch[i];
            if (lastGroup.rm_so != -1) {
                int length = lastGroup.rm_eo - lastGroup.rm_so;
                text.erase(lastGroup.rm_so, length);
            }
        }

    } else { // no match
        result = "";
        // erase nothing
    }

    LOG_DEBUG("extractPattern: result=%s, textout=%s", result.c_str(), text.c_str());
    return result;
}

/** Extract all matching patterns and remove them from the text
  */
std::set<std::string> extractAllPatterns(regex_t *regex, std::string &text)
{
    std::set<std::string> result;

    std::string extracted;
    do {
        extracted = extractPattern(regex, text, ERASE_LEAST);
        if (!extracted.empty()) result.insert(extracted);

    } while (!extracted.empty());

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
                ReqFileConfig *fdown = getDocument(r->second.parentDocument->id);
                ReqFileConfig *fup = getDocument(req->parentDocument->id);
                if (!fdown) PUSH_ERROR(r->second.parentDocument->id, "", "Cannot find document");
                else if (!fup) PUSH_ERROR(r->second.parentDocument->id, "", "Cannot find document");
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
                PUSH_ERROR(r->second.parentDocument->id, *c, "Undefined requirement, referenced by: %s",
                           r->second.id.c_str());
            }
        }
    }
}

void computeGlobalStatistics()
{
    // compute global statistics
    ReqFileConfig *file;
    std::map<std::string, Requirement>::iterator req;
    FOREACH(req, Requirements) {
        file = req->second.parentDocument;
        file->nTotalRequirements++;
        if (!req->second.coveredBy.empty()) {
            file->nCoveredRequirements++;
        }
    }
}
