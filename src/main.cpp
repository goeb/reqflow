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
#ifndef _WIN32
#include "ReqDocumentPdf.h"
#endif
#include "renderingHtml.h"

void usage()
{
    printf("Usage: req <command> [<options>] [<args>]\n"
           "\n"
           "Commands:\n"
           "\n"
           "    stat [doc ...]  Print the status of requirements in all documents or the given documents.\n"
           "                    Without additionnal option, only unresolved coverage issues are reported.\n"
           "         -s         Print a one-line summary for each document.\n"
           "         -v         Print the status of all requirements.\n"
           "                    Status codes:\n"
           "\n"
           "                        'U'  Uncovered\n"
           "\n"
           "    trac [doc ...]  Print the traceability matrix of the requirements (A covered by B).\n"
           "         [-r]       Print the reverse traceability matrix (A covers B).\n"
           "         [-x <fmt>] Select export format: text (default), csv.\n"
           "\n"
           "    config          Print the list of configured documents.\n"
           "\n"
           "    report [-html]  Generate HTML report\n"
           "\n"
           "    debug <file>    Dump text extracted from file (debug purpose).\n"
           "                    (PDF not supported on Windows)\n"
           "\n"
           "    regex <pattern> <text>\n"
           "                    Test regex given by <pattern> applied on <text>.\n"
           "\n"
           "    version\n"
           "    help\n"
           "\n"
           "Options:\n"
           "    -c <config> Select configuration file. Defaults to 'conf.req'.\n"
           "\n"
           "\n");
    exit(1);
}

enum StatusMode { REQ_SUMMARY, REQ_UNRESOLVED, REQ_ALL };

int showVersion()
{
    printf("Req v%s\n"
           "Copyright (C) 2013 Frederic Hoerni\n"
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
           , VERSION);
    exit(1);
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



std::string replaceDefinedVariable(const std::map<std::string, std::string> &definedVariables, const std::string &token)
{
    std::string result = token;
    std::map<std::string, std::string>::const_iterator v;
    FOREACH(v, definedVariables) {
        size_t pos = v->second.find(v->first);
        if (pos != std::string::npos) {
            LOG_ERROR("Recursive pattern in defined variable: %s => %s", v->first.c_str(), v->second.c_str());
            exit(1);
        }
        while ( (pos = result.find (v->first)) != std::string::npos) {
            result = result.replace(pos, v->first.size(), v->second);
        }
    }
    LOG_DEBUG("replace(%s) => %s", token.c_str(), result.c_str());
    return result;
}


int loadConfiguration(const char * file)
{
    LOG_DEBUG(_("Loading configuration file: '%s'"), file);

    const char *config;
    int r = loadFile(file, &config);
    if (r<0) {
        PUSH_ERROR(_("Cannot load file '%s': %s"), file, strerror(errno));
        return 1;
    } else if (r == 0) {
        PUSH_ERROR(_("Empty configuration file '%s'."), file);
        return 1;
    }
    // parse the configuration
    std::list<std::list<std::string> > configTokens = parseConfigTokens(config, r);
    free((void*)config);

    std::map<std::string, std::string> defs;

    std::list<std::list<std::string> >::iterator line;
    int lineNum = 0;
    FOREACH(line, configTokens) {
        lineNum++;
        std::string verb = pop(*line);
        if (verb == "document") {
            ReqFileConfig fileConfig;
            fileConfig.id = pop(*line);
            LOG_DEBUG("document '%s'...", fileConfig.id.c_str());
            if (fileConfig.id.empty()) {
                PUSH_ERROR("Missing identifier for file: line %d", lineNum);
            }

            while (!line->empty()) {
                std::string arg = pop(*line);
                if (arg == "-path") {
                    if (line->empty()) {
                        PUSH_ERROR("Missing -path value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.path = replaceDefinedVariable(defs, pop(*line));

                } else if (arg == "-start-after") {
                    if (line->empty()) {
                        PUSH_ERROR("Missing -start-after value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.startAfter = replaceDefinedVariable(defs, pop(*line));
                    fileConfig.startAfterRegex = new regex_t();
                    int reti = regcomp(fileConfig.startAfterRegex, fileConfig.startAfter.c_str(), 0);
                    if (reti) {
                        PUSH_ERROR("Cannot compile startAfter regex for %s: '%s'", fileConfig.id.c_str(), fileConfig.startAfter.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.startAfter.c_str(), &fileConfig.startAfterRegex);

                } else if (arg == "-stop-after") {
                    if (line->empty()) {
                        PUSH_ERROR("Missing -stop-after value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.stopAfter = replaceDefinedVariable(defs, pop(*line));
                    fileConfig.stopAfterRegex = new regex_t();
                    int reti = regcomp(fileConfig.stopAfterRegex, fileConfig.stopAfter.c_str(), 0);
                    if (reti) {
                        PUSH_ERROR("Cannot compile stopAfter regex for %s: '%s'", fileConfig.id.c_str(), fileConfig.stopAfter.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.stopAfter.c_str(), &fileConfig.stopAfterRegex);

                } else if (arg == "-nocov") {
                    fileConfig.nocov = true;

                } else if (arg == "-req") {
                    if (line->empty()) {
                        PUSH_ERROR("Missing -req value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.reqPattern = replaceDefinedVariable(defs, pop(*line));
                    /* Compile regular expression */
                    fileConfig.reqRegex = new regex_t();
                    int reti = regcomp(fileConfig.reqRegex, fileConfig.reqPattern.c_str(), 0);
                    if (reti) {
                        PUSH_ERROR("Cannot compile req regex for %s: '%s'", fileConfig.id.c_str(), fileConfig.reqPattern.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.reqPattern.c_str(), &fileConfig.reqRegex);

                } else if (arg == "-ref") {
                    if (line->empty()) {
                        PUSH_ERROR("Missing -ref value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.refPattern = replaceDefinedVariable(defs, pop(*line));
                    /* Compile regular expression */
                    fileConfig.refRegex = new regex_t();
                    int reti = regcomp(fileConfig.refRegex, fileConfig.refPattern.c_str(), 0);
                    if (reti) {
                        PUSH_ERROR("Cannot compile ref regex for %s: %s", fileConfig.id.c_str(), fileConfig.refPattern.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.refPattern.c_str(), &fileConfig.refRegex);

                } else {
                    PUSH_ERROR("Invalid token '%s': line %d", arg.c_str(), lineNum);
                }
            }
            std::map<std::string, ReqFileConfig>::iterator c = ReqConfig.find(fileConfig.id);
            if (c != ReqConfig.end()) {
                PUSH_ERROR("Config error: duplicate id '%s'", fileConfig.id.c_str());
                return 1;
            }
            ReqConfig[fileConfig.id] = fileConfig;

        } else if (verb == "define") {
            if (line->size() != 2) {
                PUSH_ERROR("Invalid defined: missing value, line %d", lineNum);
            } else {
                std::string key = pop(*line);
                std::string value = pop(*line);
                LOG_DEBUG("Add variable '%s'='%s'", key.c_str(), value.c_str());
                defs[key] = value;
            }
        } else {
            PUSH_ERROR("Invalid token '%s': line %d", verb.c_str(), lineNum);
        }
    }
    return 0;
}

enum ReqFileType { RF_TEXT, RF_ODT, RF_DOCX, RF_XSLX, RF_DOCX_XML, RF_PDF, RF_UNKNOWN };
ReqFileType getFileType(const std::string &path)
{
    size_t i = path.find_last_of('.');
    if (i == std::string::npos) return RF_UNKNOWN;
    if (i == path.size()-1) return RF_UNKNOWN;
    std::string extension = path.substr(i+1);
    if (0 == strcasecmp(extension.c_str(), "txt")) return RF_TEXT;
    else if (0 == strcasecmp(extension.c_str(), "odt")) return RF_ODT;
    else if (0 == strcasecmp(extension.c_str(), "docx")) return RF_DOCX;
    else if (0 == strcasecmp(extension.c_str(), "xslx")) return RF_XSLX;
    else if (0 == strcasecmp(extension.c_str(), "xml")) return RF_DOCX_XML;
#ifndef _WIN32
    else if (0 == strcasecmp(extension.c_str(), "pdf")) return RF_PDF;
#endif
    else return RF_UNKNOWN;
}

int loadRequirementsOfFile(const ReqFileConfig fileConfig, bool debug)
{
    int result = 0;
    ReqFileType fileType = getFileType(fileConfig.path);
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

#ifndef _WIN32
    case RF_PDF:
    {
        ReqDocumentPdf doc(fileConfig);
        doc.loadRequirements(debug);
        break;
    }
#endif
    default:
        LOG_ERROR("Cannot load unsupported file type: %s", fileConfig.path.c_str());
        result = -1;
    }
    return result;
}

int loadRequirements(bool debug)
{
    int result = 0;
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        ReqFileConfig &fileConfig = c->second;
        result = loadRequirementsOfFile(fileConfig, debug);
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
        printf(",,"CRLF);
        printf("Requirements of %s,", docId);
        if (reverse) {
            printf("Requirements Downstream");
        } else {
            printf("Requirements Upstream");
        }
        if (verbose) printf(",Document");
        else printf(","); // always 3 columns
        printf(CRLF);

    } else {
        printf("\n");
        printf("Requirements of %-34s ", docId);
        if (reverse) printf(ALIGN, "Requirements Downstream");
        else printf(ALIGN, "Requirements Upstream");
        if (verbose) printf(" Document");
        printf("\n");
        printf("----------------------------------------------");
        printf("----------------------------------------------------------------------\n");

    }
}

/** req2 and doc2Id may be null
 */
void printTracLine(const char *req1, const char *req2, const char *doc2Id, ReqExportFormat format)
{
    if (format == REQ_X_CSV) {
        printf("%s,", req1);
        if (req2) printf("%s", req2);
        if (doc2Id) printf(",%s", doc2Id);
        else printf(","); // always 3 columns
        printf(CRLF);

    } else { // text format
        printf(ALIGN, req1);
        if (req2) printf(" " ALIGN, req2);
        if (doc2Id) printf(" %s", doc2Id);
        printf("\n");
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
                else if (verbose) docId = ref->parentDocumentId.c_str();

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
                else if (verbose) docId = above->parentDocumentId.c_str();

                printTracLine(r.id.c_str(), c->c_str(), docId, format);
            }
        }
    }
}

/** Print traceability matrix of file.
  */
void printTracOfFile(const char *docId, bool reverse, bool verbose, ReqExportFormat format)
{
    printTracHeader(docId, reverse, verbose, format);

    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (!docId || r->second.parentDocumentId == docId) {
            printTrac(r->second, reverse, verbose, format);
        }
    }
}


/** if not 'reverse', then print matrix A covered by B
  * Else, print matrix A covers B.
  */
void printMatrix(int argc, const char **argv, bool reverse, bool verbose, ReqExportFormat format)
{
    std::map<std::string, ReqFileConfig>::iterator file;
    if (!argc) {
        FOREACH(file, ReqConfig) {
            printTracOfFile(file->second.id.c_str(), reverse, verbose, format);
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) {
            LOG_ERROR("Invalid document id: %s", docId);
        } else printTracOfFile(file->second.id.c_str(), reverse, verbose, format);
        argc--;
        argv++;
    }
}


void printSummaryHeader()
{
    printf("%-30s    %% covered / total\n", "Document");
    printf("----------------------------------------------------------------------\n");
}

/** Option 'nocov' indicates if coverage is relevant for this document
 */
void printPercent(int total, int covered, const char *docId, const char *path, bool nocov)
{
    if (nocov) printf("%-30s nocov  nocov / %5d %s\n", docId, total, path);
    else {
        int ratio = 0;
        if (total > 0) ratio = 100*covered/total;
        printf("%-30s %3d%% %7d / %5d %s\n", docId, ratio, covered, total, path);
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

    std::map<std::string, ReqFileConfig>::iterator file;

    if (!argc) {
        FOREACH(file, ReqConfig) {
            printSummaryOfFile(file->second);
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        std::map<std::string, ReqFileConfig>::iterator file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) LOG_ERROR("Invalid document id: %s", docId);
        else printSummaryOfFile(file->second);
        argc--;
        argv++;
    }

    if (doPrintTotal) printPercent(ReqTotal, ReqCovered, "Total", "", false);
}

void printRequirementsOfFile(StatusMode status, const char *documentId)
{
    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (!documentId || r->second.parentDocumentId == documentId) {
            bool doPrint = false;
            if (REQ_ALL == status || r->second.coveredBy.empty()) doPrint = true;

            if (doPrint) {
                if (r->second.coveredBy.empty()) printf("U");
                else printf(" ");

                printf(" %s", r->second.id.c_str());
                if (REQ_ALL == status) printf(" %s", r->second.parentDocumentId.c_str());
                printf("\n");
            }
        }
    }
}
void printRequirements(StatusMode status, int argc, const char **argv)
{
    std::map<std::string, ReqFileConfig>::iterator file;
    if (!argc) {
        // print requirements of all files
        FOREACH(file, ReqConfig) {
            printRequirementsOfFile(status, file->first.c_str());
        }
    } else while (argc > 0) {
        const char *docId = argv[0];
        file = ReqConfig.find(docId);
        if (file == ReqConfig.end()) LOG_ERROR("Invalid document id: %s", docId);
        else printRequirementsOfFile(status, argv[0]);
        argc--;
        argv++;
    }
}


int cmdStat(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    StatusMode statusMode = REQ_UNRESOLVED;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
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
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;

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
    if (r != 0) return 1;

    r = loadRequirements(false);
    if (r != 0) return 1;

    if (0 == strcmp(exportFormat, "txt")) printMatrix(argc-i, argv+i, reverse, verbose, REQ_X_TXT);
    else if (0 == strcmp(exportFormat, "csv")) printMatrix(argc-i, argv+i, reverse, verbose, REQ_X_CSV);
    else {
        LOG_ERROR("Invalid export format: %s", exportFormat);
    }

    return 0;
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
    printf("Config file: %s\n", configFile);

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    // display a summary of the configuration
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        printf("%s: %s\n", c->first.c_str(), c->second.path.c_str());
    }

    return 0;
}

int cmdHtml(const std::string &cmdline, int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        } else {
            i--; // push back arg into the list
            break; // leave remaining args for below
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    r = loadRequirements(false);
    if (r != 0) return 1;

    htmlRender(cmdline, argc-i, argv+i);

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

int cmdReport(const std::string &cmdline, int argc, const char **argv)
{
    std::string format = "-html"; // default format is HTML
    if (argc > 0 && 0 == strcmp("-html", argv[0])) {
        format = argv[0];
        argc--; argv++;
    }

    if (format == "-html") return cmdHtml(cmdline, argc, argv);
    else usage();
    return 0;
}


int cmdRegex(int argc, const char **argv)
{
    if (argc != 2) usage();

    regex_t regex;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex, argv[0], 0);
    if (reti) {
        LOG_ERROR("Could not compile regex");
        return 1;
    }

    /* Execute regular expression */

    const int N = 5;

    char buffer[256];
    const char *text = argv[1];
    regmatch_t pmatch[N];
    reti = regexec(&regex, text, N, pmatch, 0);
    if (!reti) {
        printf("Match: \n");
        int i;
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                printf("match[%d]: %s\n", i, buffer);
            } else printf("match[%d]:\n", i);
        }
    } else if (reti == REG_NOMATCH) {
        printf("No match\n");

    } else {
        char msgbuf[100];
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        LOG_ERROR("Regex match failed: %s", msgbuf);
        return 1;
    }

    return 0;
}



int main(int argc, const char **argv)
{
    if (argc < 2) usage();

    std::string command = argv[1];
    int rc = 0;

    if (command == "stat")         rc = cmdStat(argc-2, argv+2);
    else if (command == "version") rc = showVersion();
    else if (command == "trac")    rc = cmdTrac(argc-2, argv+2);
    else if (command == "config")  rc = cmdConfig(argc-2, argv+2);
    else if (command == "report")  {
        // get command line
        std::string cmdline;
        int i;
        for(i=0; i<argc; i++) cmdline += argv[i] + std::string(" ");
        rc = cmdReport(cmdline, argc-2, argv+2);
    } else if (command == "regex")   rc = cmdRegex(argc-2, argv+2);
#ifndef _WIN32
    else if (command == "debug")   rc = cmdDebug(argc-2, argv+2);
#endif
    else usage();

    printErrors();

    return rc;
}
