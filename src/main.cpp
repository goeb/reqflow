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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pcreposix.h>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

#include "global.h"
#include "logging.h"
#include "parseConfig.h"
#include "req.h"
#include "ReqDocumentDocx.h"
#include "ReqDocumentTxt.h"
#include "ReqDocumentHtml.h"
#include "stringTools.h"
#include "ReqDocumentPdf.h"
#include "renderingHtml.h"

std::string Cmdline;

// Directory where the config file is
// Relative paths are computed from there.
std::string RootDir = ".";



void usage()
{
    printf("Usage: 1. reqflow <command> [<options>] [<args>]\n"
           "       2. reqflow <config-file>\n"
           "\n"
           "\n"
           "1. reqflow <command> [<options>] [<args>]\n"
           "\n"
           "Commands:\n"
           "\n"
           "    stat [doc ...]  Print the status of requirements in all documents or the\n"
           "                    given documents. Without additionnal option, only\n"
           "                    unresolved coverage issues are reported.\n"
           "         -s         Print a one-line summary for each document.\n"
           "         -v         Print the status of all requirements.\n"
           "                    Status codes:\n"
           "\n"
           "                      'U'  Uncovered\n"
           "\n"
           "    trac [doc ...]  Print the traceability matrix of the requirements \n"
           "                    (A covered by B).\n"
           "         [-r]       Print the reverse traceability matrix (A covers B).\n"
           "         [-x <fmt>] Select export format: text (default), csv, html.\n"
           "                    If format 'html' is chosen, -r is ignored, as both foward\n"
           "                    and reverse traceability matrices are displayed.\n"
           "\n"
           "    review          Print the requirements with their text.\n"
           "         [-f | -r]  Print also traceability (forward or backward)\n"
           "         [-x <fmt>] Choose format: txt, csv.\n"
           "\n"
           "    config          Print the list of configured documents.\n"
           "\n"
           "    debug <file>    Dump text extracted from file (debug purpose).\n"
           "\n"
           "    regex <pattern> [<text>]\n"
           "                    Test regex given by <pattern> applied on <text>.\n"
           "                    If <text> is omitted, then the text is read from stdin.\n"
           "\n"
           "    version\n"
           "    help\n"
           "\n"
           "Options:\n"
           "    -c <config>  Select configuration file. Defaults to 'conf.req'.\n"
           "    -o <file>    Output to file instead of stdout.\n"
           "                 Not supported for commands 'config', debug' and 'regex'."
           "\n"
           "\n"
           "2. reqflow <config>\n"
           "This is equivalent to:\n"
           "    reqflow trac -c <config> -x html -o <outfile> && start <outfile>\n"
           "\n"
           "Purpose: This usage is suitable for double-cliking on the config file.\n"
           "Note: <config> must be different from the commands of use case 1.\n"
           "\n");
    exit(1);
}

enum StatusMode { REQ_SUMMARY, REQ_UNRESOLVED, REQ_ALL };

int showVersion()
{
    printf("%s\n"
           "Copyright (C) 2014 Frederic Hoerni\n"
           "\n"
           "This program is free software; you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation; either version 2 of the License, or\n"
           "(at your option) any later version.\n"
           "\n"
           "This program is distributed in the hope that it will be useful,\n"
           "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
           "GNU General Public License for more details.\n"
           , PACKAGE_STRING);
    exit(1);
}

std::string replaceDefinedVariable(const std::list<std::pair<std::string, std::string> > &definedVariables, const std::string &token)
{
    std::string result = token;
    std::list<std::pair<std::string, std::string> >::const_iterator v;
    FOREACH(v, definedVariables) {
        size_t pos = v->second.find(v->first);
        if (pos != std::string::npos) {
            PUSH_ERROR("[config]", "", "Recursive pattern in defined variable: %s => %s", v->first.c_str(), v->second.c_str());
            return result;
        }
        while ( (pos = result.find (v->first)) != std::string::npos) {
            result = result.replace(pos, v->first.size(), v->second);
        }
    }
    LOG_DEBUG("replace(%s) => %s", token.c_str(), result.c_str());
    return result;
}

void appendEnvVar(const char *startVarName, size_t len, std::string &result)
{
    std::string varname;
    varname.assign(startVarName, len);
    const char *value = getenv(varname.c_str());
    if (value) result.append(value);
}

/** Replace environment variables in a string
 *
 *  The syntax for inserting env variable is: ${xx} or $xx.
 *
 * @param[in.out] s
 */
void replaceEnv(std::string &s)
{
    std::string result;
    // look for patterns ${xxx} or $xxx
    const char *startVarName = 0; // position of the first char of the variable name
    const char *startVarDef = 0; // position of the dollar character

    const char *p = s.c_str();
    bool usedBraces = false;
    while (*p) {

        if (!startVarDef && *p == '$') {
            startVarDef = p;

        } else if (!startVarDef) {
            result += *p;

// past this point startVarDef is defined

        } else if (!startVarName && *p == '{') {
            startVarName = p+1;
            usedBraces = true;

        } else if (!startVarName && (isalnum(*p) || '_' == *p) ) {
            startVarName = p;

        } else if (!startVarName) {
            // was a isolated '$', push it...
            result.append(startVarDef, p-startVarDef+1);
            startVarDef = 0;

// past this point startVarName is defined

        } else if (isalnum(*p) || '_' == *p) {

        } else if (usedBraces && '}' != *p ) {
            // malformed var syntax, abandon var
            result.append(startVarDef, p-startVarDef);
            startVarDef = 0;
            startVarName = 0;

        } else {
            // got a correct var definition
            appendEnvVar(startVarName, p-startVarName, result);

            if (usedBraces && '}' == *p) { } // nominal termination, do not push current }
            else result += *p;

            startVarName = 0;
            startVarDef = 0;

        }
        p++;
    }

    // push hold chars, if any
    if (startVarDef && startVarName) appendEnvVar(startVarName, p-startVarName, result);
    else if (startVarDef) result.append(startVarDef, p-startVarDef);
    s = result;
}

/** Replace defined variables and environment variables
  *
  */
std::string consolidateToken(const std::list<std::pair<std::string, std::string> > &definedVariables, const std::string &token)
{
    std::string result = replaceDefinedVariable(definedVariables, token);
    replaceEnv(result);
    return result;
}

/** Syntax of config file:
  *
  *
  * define <x> <y>
  *
  * document <id>
  *     -path <token>
  *     -req <token>
  *     -ref <token>
  *     -nocov
  *     -start-after <token>
  *     -stop-after <token>
  *     -type <type>
  *
  * A 'define' instanciates the definition of a variable, that will be used as replacement in the tokens.
  * A 'document' starts a new definition os document.
  * The options of a document may be on the same line or on the following lines.
  * A 'define' interrupts the current document, if any.
  * Defined variables apply only on the <token> parts mentionned above, and in the order they appear.
  *
  *
  */
int loadConfiguration(const char * file)
{
    LOG_DEBUG(_("Loading configuration file: '%s'"), file);

    std::string config;
    int r = loadFile(file, config);
    if (r < 0) {
        PUSH_ERROR(file, "", _("Cannot load file: %s"), strerror(errno));
        return 1;
    } else if (r == 0) {
        PUSH_ERROR(file, "", _("Empty configuration"));
        return 1;
    }

    RootDir = getDirname(file);

    // parse the configuration
    std::list<std::list<std::string> > configTokens = parseConfigTokens(config.c_str(), r);

    std::list<std::pair<std::string, std::string> > defs;

    std::list<std::list<std::string> >::iterator line;
    ReqFileConfig *fileConfig = 0;
    int lineNum = 0;
    FOREACH(line, configTokens) {
        lineNum++;

        // handle primary verbs first
        if (line->size()) {
            std::string verb = line->front();

            if (verb == "document") {
                pop(*line);
                std::string id = pop(*line);
                if (id.empty()) {
                    PUSH_ERROR(file, "", "Missing identifier for file: line %d", lineNum);
                    return 1;
                }

                // check if document id already exists
                std::map<std::string, ReqFileConfig*>::iterator c = ReqConfig.find(id);
                if (c != ReqConfig.end()) {
                    PUSH_ERROR(file, "", "Config error: duplicate id '%s'", id.c_str());
                    return 1;
                }

                // initiates a new document
                fileConfig = new ReqFileConfig;

                fileConfig->id = id;
                LOG_DEBUG("document '%s'...", fileConfig->id.c_str());
                ReqConfig[id] = fileConfig;

            } else if (verb == "define") {
                pop(*line);
                if (line->size() != 2) {
                    PUSH_ERROR(file, "", "Invalid define: missing value, line %d", lineNum);
                    return 1;

                } else {
                    fileConfig = 0; // interrupts current document

                    std::string key = pop(*line);
                    std::string value = pop(*line);
                    LOG_DEBUG("Add variable '%s'='%s'", key.c_str(), value.c_str());
                    defs.push_front(std::make_pair(key, value));
                }
            } // else, parse arguments below...
        }

        while (!line->empty()) {
            std::string arg = pop(*line);
            if (fileConfig && arg == "-path") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -path value for %s", fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->path = consolidateToken(defs, pop(*line));

            } else if (fileConfig && arg == "-start-after") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -start-after value for %s", fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->startAfter = consolidateToken(defs, pop(*line));
                fileConfig->startAfterRegex = new regex_t();
                int reti = regcomp(fileConfig->startAfterRegex, fileConfig->startAfter.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile startAfter regex for %s: '%s'", fileConfig->id.c_str(), fileConfig->startAfter.c_str());
                    return -1;
                }
                LOG_DEBUG("regcomp(%s) -> %p", fileConfig->startAfter.c_str(), &fileConfig->startAfterRegex);

            } else if (fileConfig && arg == "-stop-after") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -stop-after value for %s", fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->stopAfter = consolidateToken(defs, pop(*line));
                fileConfig->stopAfterRegex = new regex_t();
                int reti = regcomp(fileConfig->stopAfterRegex, fileConfig->stopAfter.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile stopAfter regex for %s: '%s'", fileConfig->id.c_str(), fileConfig->stopAfter.c_str());
                    return -1;
                }
                LOG_DEBUG("regcomp(%s) -> %p", fileConfig->stopAfter.c_str(), &fileConfig->stopAfterRegex);

            } else if (fileConfig && arg == "-nocov") {
                fileConfig->nocov = true;

            } else if (fileConfig && arg == "-req") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -req value for %s", fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->reqPattern = consolidateToken(defs, pop(*line));
                /* Compile regular expression */
                fileConfig->reqRegex = new regex_t();
                int reti = regcomp(fileConfig->reqRegex, fileConfig->reqPattern.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile req regex for %s: '%s'", fileConfig->id.c_str(), fileConfig->reqPattern.c_str());
                    return -1;
                }
                LOG_DEBUG("regcomp(%s) -> %p", fileConfig->reqPattern.c_str(), &fileConfig->reqRegex);

            } else if (fileConfig && arg == "-ref") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -ref value for %s", fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->refPattern = consolidateToken(defs, pop(*line));
                /* Compile regular expression */
                fileConfig->refRegex = new regex_t();
                int reti = regcomp(fileConfig->refRegex, fileConfig->refPattern.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile ref regex for %s: %s", fileConfig->id.c_str(), fileConfig->refPattern.c_str());
                    return -1;
                }
                LOG_DEBUG("regcomp(%s) -> %p", fileConfig->refPattern.c_str(), &fileConfig->refRegex);

            } else if (fileConfig && arg == "-end-req") {
                if (line->empty()) {
                    PUSH_ERROR(file, "", "Missing -end-req value for %s", fileConfig->id.c_str());
                    return -1;
                }
                std::string endReq = pop(*line);
                regex_t *regex = new regex_t();
                int reti = regcomp(regex, endReq.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile regex: %s", endReq.c_str());
                    return -1;
                }

                fileConfig->endReq[endReq] = regex;

            } else if (fileConfig && arg == "-end-req-style") {
                if (line->empty()) {
                    PUSH_ERROR(file, "","Missing -end-req-style value for %s", fileConfig->id.c_str());
                    return -1;
                }
                std::string endReqStyle = pop(*line);
                regex_t *regex = new regex_t();
                int reti = regcomp(regex, endReqStyle.c_str(), 0);
                if (reti) {
                    PUSH_ERROR(file, "", "Cannot compile regex: %s", endReqStyle.c_str());
                    return -1;
                }

                fileConfig->endReqStyle[endReqStyle] = regex;

            } else if (fileConfig && arg == "-sort") {
                if (line->empty()) {
                    PUSH_ERROR(file, "","Missing -sort value for %s", fileConfig->id.c_str());
                    return -1;
                }
                std::string s = pop(*line);
                fileConfig->sortMode = fileConfig->getSortMode(s);
                if (fileConfig->sortMode == ReqFileConfig::SORT_UNKNOWN) {
                    PUSH_ERROR(file, "","Unknown sort mode '%s' for %s", s.c_str(), fileConfig->id.c_str());
                    return -1;
                }

            } else if (fileConfig && arg == "-type") {
                if (line->empty()) {
                    PUSH_ERROR(file, "","Missing -type value for %s", fileConfig->id.c_str());
                    return -1;
                }
                std::string t = pop(*line);
                ReqFileType type = fileConfig->getFileType(t);
                if (type == RF_UNKNOWN) {
                    PUSH_ERROR(file, "","Unknown file type '%s' for %s", t.c_str(), fileConfig->id.c_str());
                    return -1;
                }
                fileConfig->type = type;

            } else if (!fileConfig) {
                PUSH_ERROR(file, "", "Invalid token '%s' within no document: line %d", arg.c_str(), lineNum);
                return 1;
            } else {
                PUSH_ERROR(file, "", "Invalid token '%s': line %d", arg.c_str(), lineNum);
            }
        }
    }
    return 0;
}


int loadRequirementsOfFile(ReqFileConfig &fileConfig, bool debug)
{
    int result = 0;

    if (isPathAbsolute(fileConfig.path)) fileConfig.realpath = fileConfig.path;
    else {
        // prefix with the root path
        fileConfig.realpath = RootDir + '/' + fileConfig.path;
    }

    ReqFileType fileType = fileConfig.getFileType();
    switch(fileType) {
    case RF_TEXT:
    {
        ReqDocumentTxt doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
    case RF_ODT:
    case RF_DOCX:
    {
        ReqDocumentDocx doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
    case RF_DOCX_XML: // mainly for test purpose
    {
        ReqDocumentDocxXml doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
    case RF_HTML: // mainly for test purpose
    {
        ReqDocumentHtml doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
    case RF_PDF:
    {
        ReqDocumentPdf doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
    default:
        PUSH_ERROR(fileConfig.id, "", "Cannot load unsupported file type: %s", fileConfig.path.c_str());
        result = -1;
    }
    return result;
}

int loadRequirements(bool debug)
{
    int result = 0;
    std::map<std::string, ReqFileConfig*>::iterator c;
    FOREACH(c, ReqConfig) {
        ReqFileConfig *fileConfig = c->second;
        result = loadRequirementsOfFile(*fileConfig, debug);
    }
    checkUndefinedRequirements();
    consolidateCoverage();
    computeGlobalStatistics();

    return result;
}

enum ReqExportFormat { REQ_X_TXT, REQ_X_CSV };
#define ALIGN "%-50s"
#define CRLF "\r\n"
void printTracHeader(const char *docId, bool reverse, bool verbose, ReqExportFormat format)
{
    if (format == REQ_X_CSV) {
        OUTPUT(",,"CRLF);
        OUTPUT("Requirements of %s,", docId);
        if (reverse) {
            OUTPUT("Requirements Upstream");
        } else {
            OUTPUT("Requirements Downstream");
        }
        if (verbose) OUTPUT(",Document");
        else OUTPUT(","); // always 3 columns
        OUTPUT(CRLF);

    } else {
        OUTPUT("\n");
        OUTPUT("Requirements of %-34s ", docId);
        if (reverse) OUTPUT(ALIGN, "Requirements Upstream");
        else OUTPUT(ALIGN, "Requirements Downstream");
        if (verbose) OUTPUT(" Document");
        OUTPUT("\n");
        OUTPUT("----------------------------------------------");
        OUTPUT("----------------------------------------------------------------------\n");

    }
}

/** req2 and doc2Id may be null
 */
void printTracLine(const char *req1, const char *req2, const char *doc2Id, ReqExportFormat format)
{
    if (format == REQ_X_CSV) {
        OUTPUT("%s,", req1);
        if (req2) OUTPUT("%s", req2);
        if (doc2Id) OUTPUT(",%s", doc2Id);
        else OUTPUT(","); // always 3 columns
        OUTPUT(CRLF);

    } else { // text format
        OUTPUT(ALIGN, req1);
        if (req2) OUTPUT(" " ALIGN, req2);
        if (doc2Id) OUTPUT(" %s", doc2Id);
        OUTPUT("\n");
    }
}

void printTrac(const Requirement &r, bool reverse, bool verbose, ReqExportFormat format)
{
    if (reverse) { // A covering B
        if (r.covers.empty()) {
            printTracLine(r.id.c_str(), 0, 0, format);

        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r.covers) {
                const char* docId = 0;
                Requirement *ref = getRequirement(c->c_str());
                if (!ref) docId = "Undefined";
                else if (verbose) docId = ref->parentDocument->id.c_str();

                printTracLine(r.id.c_str(), c->c_str(), docId, format);
            }
        }

    } else { // A covered by B
        if (r.coveredBy.empty()) {
            printTracLine(r.id.c_str(), 0, 0, format);

        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r.coveredBy) {
                const char* docId = 0;
                Requirement *above = getRequirement(c->c_str());
                if (!above) docId = "Undefined";
                else if (verbose) docId = above->parentDocument->id.c_str();

                printTracLine(r.id.c_str(), c->c_str(), docId, format);
            }
        }
    }
}

/** Print traceability matrix of file.
  */
void printTracOfFile(const ReqFileConfig &rfc, bool reverse, bool verbose, ReqExportFormat format)
{
    printTracHeader(rfc.id.c_str(), reverse, verbose, format);

    std::set<Requirement*, ReqCompare>::iterator r;
    FOREACH(r, rfc.requirements) {
        printTrac(*(*r), reverse, verbose, format);
    }
}


/** if not 'reverse', then print matrix A covered by B
  * Else, print matrix A covers B.
  */
void printMatrix(int argc, const char **argv, bool reverse, bool verbose, ReqExportFormat format)
{
    std::map<std::string, ReqFileConfig*>::iterator file;
    if (!argc) {
        FOREACH(file, ReqConfig) {
            printTracOfFile(*(file->second), reverse, verbose, format);
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) {
            LOG_ERROR("Invalid document id: %s", docId);
        } else printTracOfFile(*(file->second), reverse, verbose, format);
        argc--;
        argv++;
    }
}

void printIndented(const std::string &text)
{
    size_t pos0 = 0;
    while (pos0 < text.size()) {
        std::string line;
        size_t pos1 = text.find('\n', pos0);
        if (pos1 == std::string::npos) {
            // take last line
            line = text.substr(pos0);
            pos0 = text.size(); // stop the loop
        } else {
            line = text.substr(pos0, pos1-pos0);
            pos0 = pos1 + 1;
        }

        trimBlanks(line);
        OUTPUT("        %s\n", line.c_str());
    }
}

enum ReviewMode { RM_RAW, RM_TRACE_FORWARD, RM_TRACE_BACKWARD };
/** Print on 2 columns the requirements, and their text
  */
void printTextOfReqs(int argc, const char **argv, ReviewMode rm, ReqExportFormat format)
{
    // build list of documents
    std::list<ReqFileConfig*> documents;
    if (argc) {
        while (argc) {
            std::map<std::string, ReqFileConfig*>::iterator rc = ReqConfig.find(argv[0]);
            if (rc == ReqConfig.end()) {
                OUTPUT("Unknown document: %s\n", argv[0]);
            } else documents.push_back(rc->second);
            argv++; argc--;
        }
    } else {
        // take all documents
        std::map<std::string, ReqFileConfig*>::iterator file;
        FOREACH(file, ReqConfig) {
            documents.push_back(file->second);
        }
    }

    std::list<ReqFileConfig*>::iterator doc;
    FOREACH(doc, documents) {
        std::set<Requirement*, ReqCompare>::iterator req;
        FOREACH(req, (*doc)->requirements) {
            if (format == REQ_X_CSV) {
                OUTPUT("%s,%s\r\n", escapeCsv((*req)->id).c_str(), escapeCsv((*req)->text).c_str());
            } else {
                OUTPUT("%s\n", (*req)->id.c_str());
                printIndented((*req)->text);
                OUTPUT("\n");
            }

        }
    }
    if (rm != RM_RAW) OUTPUT("Review with traceability not implemented yet.\n");
}

void printSummaryHeader()
{
    OUTPUT("%-30s    %% covered / total\n", "Document");
    OUTPUT("----------------------------------------------------------------------\n");
}

/** Option 'nocov' indicates if coverage is relevant for this document
 */
void printPercent(int total, int covered, const char *docId, const char *path, bool nocov)
{
    if (nocov) OUTPUT("%-30s nocov  nocov / %5d %s\n", docId, total, path);
    else {
        int ratio = 0;
        if (total > 0) ratio = 100*covered/total;
        OUTPUT("%-30s %3d%% %7d / %5d %s\n", docId, ratio, covered, total, path);
    }
}

void printSummaryOfFile(const ReqFileConfig &f)
{
    if (!f.nocov) {
        ReqTotal += f.nTotalRequirements;
        ReqCovered += f.nCoveredRequirements;
    }
    printPercent(f.nTotalRequirements, f.nCoveredRequirements, f.id.c_str(), f.path.c_str(), f.nocov);

}

void printSummary(int argc, const char **argv)
{
    // print statistics
    bool doPrintTotal = true;
    if (argc == 1) doPrintTotal = false;

    printSummaryHeader();

    std::map<std::string, ReqFileConfig*>::iterator file;

    if (!argc) {
        FOREACH(file, ReqConfig) {
            printSummaryOfFile(*(file->second));
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        std::map<std::string, ReqFileConfig*>::iterator file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) LOG_ERROR("Invalid document id: %s", docId);
        else printSummaryOfFile(*(file->second));
        argc--;
        argv++;
    }

    if (doPrintTotal) printPercent(ReqTotal, ReqCovered, "Total", "", false);
}

void printRequirementsOfFile(StatusMode status, const ReqFileConfig &rfc)
{
    std::set<Requirement*, ReqCompare>::iterator r;
    FOREACH(r, rfc.requirements) {
        bool doPrint = false;
        if (REQ_ALL == status || (*r)->coveredBy.empty()) doPrint = true;

        if (doPrint) {
            if ((*r)->coveredBy.empty()) OUTPUT("U");
            else OUTPUT(" ");

            OUTPUT(" %s", (*r)->id.c_str());
            if (REQ_ALL == status) OUTPUT(" %s", (*r)->parentDocument->id.c_str());
            OUTPUT("\n");
        }
    }
}
void printRequirements(StatusMode status, int argc, const char **argv)
{
    std::map<std::string, ReqFileConfig*>::iterator file;
    if (!argc) {
        // print requirements of all files
        FOREACH(file, ReqConfig) {
            printRequirementsOfFile(status, *(file->second));
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) LOG_ERROR("Invalid document id: %s", docId);
        else printRequirementsOfFile(status, *(file->second));
        argc--;
        argv++;
    }
}


int cmdStat(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *output = 0;
    const char *arg = 0;
    StatusMode statusMode = REQ_UNRESOLVED;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        } else if (0 == strcmp(arg, "-o")) {
            if (i>=argc) usage();
            output = argv[i]; i++;
        } else if (0 == strcmp(arg, "-s")) {
            statusMode = REQ_SUMMARY;
        } else if (0 == strcmp(arg, "-v")) {
            statusMode = REQ_ALL;
        } else {
            i--; // push back arg into the list
            break; // leave remaining args for below
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    r = loadRequirements(false);
    if (r != 0) return 1;

    if (output) initOutputFd(output);

    if (REQ_SUMMARY == statusMode) {
        printSummary(argc-i, argv+i);
    } else printRequirements(statusMode, argc-i, argv+i);

    return 0;
}

int cmdTrac(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    const char *exportFormat = "txt";
    bool reverse = false;
    bool verbose = false;
    const char *output = 0;

    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;

        } else if (0 == strcmp(arg, "-o")) {
            if (i>=argc) usage();
            output = argv[i]; i++;

        } else if (0 == strcmp(arg, "-x")) {
            if (i>=argc) usage();
            exportFormat = argv[i]; i++;

        } else if (0 == strcmp(arg, "-r")) reverse = true;
        else if (0 == strcmp(arg, "-v")) verbose = true;
        else {
            i--; // push back arg into the list
            break; // leave remaining args for below
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) {
        if (0 == strcmp(exportFormat, "html")) {
            if (output) initOutputFd(output);
            htmlRender(Cmdline, argc-i, argv+i);
        }
        return 1;
    }

    r = loadRequirements(false);
    if (r != 0) return 1;

    if (output) initOutputFd(output);

    if (0 == strcmp(exportFormat, "txt")) printMatrix(argc-i, argv+i, reverse, verbose, REQ_X_TXT);
    else if (0 == strcmp(exportFormat, "csv")) printMatrix(argc-i, argv+i, reverse, verbose, REQ_X_CSV);
    else if (0 == strcmp(exportFormat, "html")) htmlRender(Cmdline, argc-i, argv+i);
    else {
        LOG_ERROR("Invalid export format: %s", exportFormat);
    }

    return 0;
}

int cmdReview(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    const char *exportFormat = "txt";
    ReviewMode reviewMode = RM_RAW;
    const char *output = 0;

    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;

        } else if (0 == strcmp(arg, "-o")) {
            if (i>=argc) usage();
            output = argv[i]; i++;

        } else if (0 == strcmp(arg, "-x")) {
            if (i>=argc) usage();
            exportFormat = argv[i]; i++;

        } else if (0 == strcmp(arg, "-r")) reviewMode = RM_TRACE_BACKWARD;
        else if (0 == strcmp(arg, "-f")) reviewMode = RM_TRACE_FORWARD;
        else {
            i--; // push back arg into the list
            break; // leave remaining args for below
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    r = loadRequirements(false);
    if (r != 0) return 1;

    if (output) initOutputFd(output);

    if (0 == strcmp(exportFormat, "txt")) printTextOfReqs(argc-i, argv+i, reviewMode, REQ_X_TXT);
    else if (0 == strcmp(exportFormat, "csv")) printTextOfReqs(argc-i, argv+i, reviewMode, REQ_X_CSV);
    else {
        LOG_ERROR("Invalid export format: %s", exportFormat);
    }

    return 0;
}

/** cmdOpen is suitable for opening by double-click in Windows
  *
  * It is equivalent to "req trac -c <config> -x html -o <config>-yyyymmdd-HHMMSS.html"
  * and opening the resulting file.
  *
  */
int cmdOpen(const char *file)
{
    std::string outputFile = file;
    trimExtension(outputFile);
    // add timestamp and ".html"
    outputFile += "-" + getDatetimePath() + ".html";
    const char *argv[6] = { "-c", file, "-x", "html", "-o", outputFile.c_str() };
    int r = cmdTrac(6, argv);

    // close output file
    closeOutputFd();
    if (r == 0) {
        // open in a web browser
        std::string cmd;
#ifdef _WIN32
        // On windows, we have to put double-quotes around the entire command

        cmd = "\"start \"\" \"" + outputFile + "\"\"";
#else
        cmd = "echo \"Output file: " + outputFile + "\"";
#endif
        r = system(cmd.c_str());
    }
    return r;
}


/** Print configuration documents
  */
int cmdConfig(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;

    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        }
    }

    OUTPUT("Config file: %s\n", configFile);

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    // display a summary of the configuration
    std::map<std::string, ReqFileConfig*>::iterator c;
    FOREACH(c, ReqConfig) {
        ReqFileConfig f = *(c->second);
        OUTPUT("%s: -path '%s'\n", c->first.c_str(), f.path.c_str());
        OUTPUT("%s: -req '%s'", c->first.c_str(), f.reqPattern.c_str());
        if (!f.refPattern.empty()) OUTPUT(" -ref '%s'", f.refPattern.c_str());
        if (!f.startAfter.empty()) OUTPUT(" -start '%s'", f.startAfter.c_str());
        if (!f.stopAfter.empty()) OUTPUT(" -stop '%s'", f.stopAfter.c_str());
        if (f.nocov) OUTPUT(" -nocov");
        OUTPUT("\n");
    }

    return 0;
}

int cmdDebug(int argc, const char **argv)
{
    if (argc == 0) usage();

    std::map<std::string, ReqFileConfig>::iterator c;
    ReqFileConfig fileConfig;
    fileConfig.path = argv[0];
    loadRequirementsOfFile(fileConfig, true);
    return 0;
}


/** Apply a regex on some text and print the result
  *
  */
void testRegex(regex_t regex, const char *text, bool verbose)
{
    const int N = 5;

    char buffer[256];
    regmatch_t pmatch[N];
    int reti = regexec(&regex, text, N, pmatch, 0);
    if (!reti) {
        if (verbose) OUTPUT("Match: \n");
        // if there are groups and not in verbose mode
        // we want only the groups, thus not the first match
        // that encompasses all matching pattern
        int i = 0;
        if (!verbose && pmatch[1].rm_so != -1) i = 1;
        for (; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                if (verbose) OUTPUT("match[%d]: %s\n", i, buffer);
                else OUTPUT("%s ", buffer);
            }
        }
        if (!verbose) OUTPUT("\n");
    } else { // reti == REG_NOMATCH)
        if (verbose) OUTPUT("No match\n");
    }
}

/** Test a regex on some pattern
  *
  * If stdin is used, reqflow is similar to 'grep' (but far more simple)
  * Example: capture requirements
  *
  * req debug SPEC2.docx | req regex "(PRINTF_[^ ]*)"
  *
  */
int cmdRegex(int argc, const char **argv)
{
    if (argc < 1 || argc > 2) usage();

    regex_t regex;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex, argv[0], 0);
    if (reti) {
        LOG_ERROR("Could not compile regex");
        return 1;
    }

    if (argc == 2) testRegex(regex, argv[1], true);
    else {
        // get the lines from stdin
        std::string line;
        while (std::getline(std::cin, line)) testRegex(regex, line.c_str(), false);
    }


    return 0;
}



int main(int argc, const char **argv)
{
    // get command line
    int i;
    for(i=0; i<argc; i++) Cmdline += argv[i] + std::string(" ");

    if (argc < 2) usage();

    std::string command = argv[1];
    int rc = 0;

    if (command == "stat")         rc = cmdStat(argc-2, argv+2);
    else if (command == "version") rc = showVersion();
    else if (command == "trac")    rc = cmdTrac(argc-2, argv+2);
    else if (command == "config")  rc = cmdConfig(argc-2, argv+2);
    else if (command == "regex")   rc = cmdRegex(argc-2, argv+2);
    else if (command == "debug")   rc = cmdDebug(argc-2, argv+2);
    else if (command == "review")  rc = cmdReview(argc-2, argv+2);
    else if (argc == 2)            rc = cmdOpen(argv[1]);
    else usage();

    printErrors();

    return rc;
}
