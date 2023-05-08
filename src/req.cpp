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
#include <algorithm>

#include "req.h"
#include "global.h"
#include "logging.h"

// static objects
std::map<std::string, ReqFileConfig*> ReqConfig;
std::map<std::string, Requirement> Requirements;
std::map<std::string, std::list<std::pair<std::string, std::string> > > Errors; // errors indexed by file
int ReqTotal = 0;
int ReqCovered = 0;
std::string htmlcss;

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

bool ReqCompare::operator()(const Requirement *a, const Requirement *b) const
{
    if (!a) {
        LOG_ERROR("null a in ReqCompare");
        return true;
    }

    if (!b) {
        LOG_ERROR("null b in ReqCompare");
        return false;
    }

    if (!a->parentDocument) {
        LOG_ERROR("null a->parentDocument in ReqCompare");
        return true;
    }
    if (a->parentDocument->sortMode == ReqFileConfig::SORT_ALPHANUMERIC_ORDER)
    {
        stringCompare sc;
        return sc(a->id, b->id);
    } else {
        // SORT_DOCUMENT_ORDER by default
        return a->seqnum < b->seqnum;
    }
}


ReqFileConfig::SortMode ReqFileConfig::getSortMode(const std::string &text)
{
    if (text == "document") return SORT_DOCUMENT_ORDER;
    else if (text == "alphanum") return SORT_ALPHANUMERIC_ORDER;
    else return SORT_UNKNOWN;
}


ReqFileType ReqFileConfig::getFileTypeByExtension(const std::string &extension)
{
    // Convert to lower case
    std::string lcExtension = extension;
    std::transform(lcExtension.begin(), lcExtension.end(), lcExtension.begin(), ::tolower);

    ReqFileType type = ReqFileConfig::getFileTypeByCode(lcExtension);
    if (type != RF_UNKNOWN) return type;

    // search alternative known extensions
    if (lcExtension == "xslx") return RF_XSLX;
    else if (lcExtension == "htm") return RF_HTML;
    // Users expect the following extensions be processed as text:
    // txt, ad, adoc, asc, asciidoc, md, c, h
    else return RF_TEXT;
}

struct FileTypeCodeAssociation {
    const char *textCode;
    ReqFileType filetype;
};

/** File type codes are the codes allowed for the -type option
 */
const FileTypeCodeAssociation FILE_TYPE_CODES[] {
    { "txt", RF_TEXT },
    { "odt", RF_ODT },
    { "docx", RF_DOCX },
    { "xml", RF_DOCX_XML },
    { "html", RF_HTML },
    { "pdf", RF_PDF },
    { 0, RF_UNKNOWN }
};

ReqFileType ReqFileConfig::getFileTypeByCode(const std::string &code)
{
    const FileTypeCodeAssociation *fta = FILE_TYPE_CODES;
    while (fta->textCode) {
        if (code == fta->textCode) return fta->filetype;
        fta++;
    }
    return RF_UNKNOWN;
}

std::string ReqFileConfig::getFileTypeCodes()
{
    std::string result;
    const FileTypeCodeAssociation *fta = FILE_TYPE_CODES;
    while (fta->textCode) {
        if (fta != FILE_TYPE_CODES) result += ", ";
        result += fta->textCode;
        fta++;
    }
    return result;
}

/** Set the file type according to the extension of the filename
 *
 * If no known extension is recognised, then it defaults to text.
 */
ReqFileType ReqFileConfig::getFileType()
{
    if (type != RF_UNKNOWN) return type;

    // If the type has not been explicitely specified, then
    // follow the filename extension.

    size_t i = path.find_last_of('.');
    if (i == std::string::npos) return RF_TEXT; // no extension (no dot character)
    else if (i == path.size()-1) return RF_TEXT; // empty extension
    else {
        std::string extension = path.substr(i+1);
        return getFileTypeByExtension(extension);
    }
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

    if (!reqId.empty()) {
        // add the prefix, if any
        reqId = fileConfig->prefixReq + reqId;

        if (refs.find(reqId) == refs.end()) {

            std::map<std::string, Requirement>::iterator r = Requirements.find(reqId);
            if (r != Requirements.end()) {
                PUSH_ERROR(fileConfig->id, reqId, "Duplicate requirement: also defined in '%s'",
                           r->second.parentDocument->path.c_str());
                currentRequirement.clear();

            } else {

                finalizeCurrentReq(); // finalize current req before starting a new one

                // insert new requirement in the global storage table
                Requirement req;
                req.id = reqId;
                req.seqnum = reqSeqnum; reqSeqnum++;
                req.parentDocument = fileConfig;
                Requirements[req.id] = req;
                fileConfig->requirements.insert(&(Requirements[req.id]));
                currentRequirement = req.id;
            }
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

    // Check for endReq to know if text of current requirement is finished
    std::map<std::string, regex_t *>::iterator endreq;
    // There may be several ways to end a requirement: check each one in turn.
    FOREACH(endreq, fileConfig->endReq) {

        std::string reqEndPat = extractPattern(endreq->second, text, ERASE_ALL);
        if (!reqEndPat.empty()) {
            finalizeCurrentReq(); // finalize current req before starting a new one            
        }
    }

    // TODO check for endReqStyle to know if text of current requirement is finished

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
        // If there is at least 2 groups, erase optionnaly the  first one and take the second
        // Else (if only one) take it.
        // We want to support the 2 following cases.
        // Example 1: ./req regex "<\(REQ_[-a-zA-Z_0-9]*\)>" "ex: <REQ_123> (comment)"
        // match[0]: <REQ_123>
        // match[1]: REQ_123
        // match[2]:
        //
        // Example 2: ./req regex "REQ_[-a-zA-Z_0-9]*" "ex: REQ_123 (comment)"
        // match[0]: REQ_123
        // match[1]:
        // match[2]:

        int index = 1; // try the second group first
        if (-1 == pmatch[index].rm_so) index = 0; // no second group, take the first and unique group
        if (-1 == pmatch[index].rm_so) {
            // just to be sure
            PUSH_ERROR("", "", "Error no group zero");
            return "";
        }

        // extract the group
        int length = pmatch[index].rm_eo - pmatch[index].rm_so;
        if (length >= 0) {
            result.assign(text.c_str()+pmatch[index].rm_so, length);
        } else {
            PUSH_ERROR("", "", "Error length=%d", length);
        }

        if (erase == ERASE_ALL) {
            // erase first group
            regmatch_t firstGroup = pmatch[0];
            if (firstGroup.rm_so != -1) {
                int length = firstGroup.rm_eo - firstGroup.rm_so;
                text.erase(firstGroup.rm_so, length);
            }
        } else if (erase == ERASE_LAST) {
            // erase last group
            regmatch_t lastGroup = pmatch[index];
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
        extracted = extractPattern(regex, text, ERASE_LAST);
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


