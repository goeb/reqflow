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
#include <regex.h>
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
#include "ReqDocumentPdf.h"

void usage()
{
    printf("Usage: req <command> [<options>] [<args>]\n"
           "\n"
           "Commands:\n"
           "\n"
           "    stat [doc ...]  Print the status of requirements in all documents or the given documents.\n"
           "                    Without additionnal option, only unresolved coverage issues are reported.\n"
           "         -s         Print a one-line summary of each document.\n"
           "         -v         Print the status of all requirements.\n"
           "\n"
           "    list\n"
           "\n"
           "    config\n"
           "\n"
           "    pdf <file>\n"
           "          Dump text extracted from pdf file (debug purpose).\n"
           "\n"
           "    regex <pattern> <text>\n"
           "          Test regex given by <pattern> applied on <text>\n"
           "\n"
           "    version\n"
           "    help\n"
           "\n"
           "Options:\n"
           "    -c <config> Select configuration file.\n"
           "    -x <format> Select export format: text (default), csv"
           "\n"
           "\n");
    exit(1);
}

int showVersion()
{
    printf("Small Issue Tracker v%s\n"
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

int loadConfiguration(const char * file)
{
    LOG_DEBUG(_("Loading configuration file: '%s'"), file);

    const char *config;
    int r = loadFile(file, &config);
    if (r<0) {
        LOG_ERROR(_("Cannot load file '%s': %s"), file, strerror(errno));
        return 1;
    } else if (r == 0) {
        LOG_ERROR(_("Empty configuration file '%s'."), file);
        return 1;
    }
    // parse the configuration
    std::list<std::list<std::string> > configTokens = parseConfigTokens(config, r);
    free((void*)config);

    std::list<std::list<std::string> >::iterator line;
    int lineNum = 0;
    FOREACH(line, configTokens) {
        lineNum++;
        std::string verb = pop(*line);
        if (verb == "addFile") {
            ReqFileConfig fileConfig;
            fileConfig.id = pop(*line);
            LOG_DEBUG("addFile '%s'...", fileConfig.id.c_str());
            if (fileConfig.id.empty()) {
                LOG_ERROR("Missing identifier for file: line %d", lineNum);
            }

            while (!line->empty()) {
                std::string arg = pop(*line);
                if (arg == "-path") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -path value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.path = pop(*line);
                } else if (arg == "-start-after") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -start-after value for %s", fileConfig.id.c_str());
                        return -1;
                    }
					fileConfig.startAfter = pop(*line);
					fileConfig.startAfterRegex = new regex_t();
                    int reti = regcomp(fileConfig.startAfterRegex, fileConfig.startAfter.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile startAfter regex for %s: %s", fileConfig.id.c_str(), fileConfig.startAfter.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.startAfter.c_str(), &fileConfig.startAfterRegex);
					
                } else if (arg == "-stop-after") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -stop-after value for %s", fileConfig.id.c_str());
                        return -1;
                    }
					fileConfig.stopAfter = pop(*line);
					fileConfig.stopAfterRegex = new regex_t();
                    int reti = regcomp(fileConfig.stopAfterRegex, fileConfig.stopAfter.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile stopAfter regex for %s: %s", fileConfig.id.c_str(), fileConfig.stopAfter.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.stopAfter.c_str(), &fileConfig.stopAfterRegex);
					
                } else if (arg == "-tag") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -tag value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.tagPattern = pop(*line);
                    /* Compile regular expression */
                    fileConfig.tagRegex = new regex_t();
                    int reti = regcomp(fileConfig.tagRegex, fileConfig.tagPattern.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile tag regex for %s: %s", fileConfig.id.c_str(), fileConfig.tagPattern.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.tagPattern.c_str(), &fileConfig.tagRegex);


                } else if (arg == "-ref") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -ref value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.refPattern = pop(*line);
                    /* Compile regular expression */
                    fileConfig.refRegex = new regex_t();
                    int reti = regcomp(fileConfig.refRegex, fileConfig.refPattern.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile ref regex for %s: %s", fileConfig.id.c_str(), fileConfig.refPattern.c_str());
                        exit(1);
                    }
                    LOG_DEBUG("regcomp(%s) -> %p", fileConfig.refPattern.c_str(), &fileConfig.refRegex);
#if 0
                } else if (arg == "-depends-on") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -depends-on value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.dependencies.insert(fileConfig.dependencies.begin(), line->begin(), line->end());
                    line->clear();
#endif
                } else {
                    LOG_ERROR("Invalid token '%s': line %d", arg.c_str(), lineNum);
                }
            }
            std::map<std::string, ReqFileConfig>::iterator c = ReqConfig.find(fileConfig.id);
            if (c != ReqConfig.end()) {
                LOG_ERROR("Config error: duplicate id '%s'", fileConfig.id.c_str());
                return 1;
            }
            ReqConfig[fileConfig.id] = fileConfig;

        } else {
            LOG_ERROR("Invalid token '%s': line %d", verb.c_str(), lineNum);
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
    else if (0 == strcasecmp(extension.c_str(), "pdf")) return RF_PDF;
    else return RF_UNKNOWN;
}



bool isReqDefined(std::string id)
{
    std::map<std::string, Requirement>::iterator req = Requirements.find(id);
    if (req == Requirements.end()) return false;
    else return true;
}

/** Fulfill .coveredBy tables
  */
void consolidateCoverage()
{
    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        std::set<std::string>::iterator c;
        FOREACH(c, r->second.covers) {
            if (isReqDefined(*c)) Requirements[*c].coveredBy.insert(r->second.id);
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
            if (!isReqDefined(*c)) {
                LOG_ERROR("Undefined requirement: %s, referenced by: %s (%s)",
                          c->c_str(), r->second.id.c_str(), r->second.parentDocumentPath.c_str());
            }
        }
    }
}

int loadRequirements()
{
    int result = 0;
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        ReqFileConfig &fileConfig = c->second;
        ReqFileType fileType = getFileType(fileConfig.path);
        switch(fileType) {
        case RF_TEXT:
        {
            ReqDocumentTxt doc(fileConfig);
            doc.loadRequirements();
            break;
        }
       case RF_DOCX:
        {
            ReqDocumentDocx doc(fileConfig);
            doc.loadRequirements();
            break;
        }
        case RF_DOCX_XML: // mainly for test purpose
        {
            ReqDocumentDocxXml doc(fileConfig);
            doc.loadRequirements();
            break;
        }
		case RF_PDF:
        {
            ReqDocumentPdf doc(fileConfig);
            doc.loadRequirements();
			break;
        }
        default:
            LOG_ERROR("Cannot load unsupported file type: %s", fileConfig.path.c_str());
            result = -1;
        }
    }
    checkUndefinedRequirements();
    consolidateCoverage();

    return result;
}

void printRequirements()
{
    printf("-- Matrix A Covers B:\n");

    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (r->second.covers.empty()) {
            printf("%s covers none\n", r->second.id.c_str());
        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r->second.covers) {
                printf("%s covers %s", r->second.id.c_str(), c->c_str());
                if (!isReqDefined(*c)) {
                    printf(":Undefined");
                } else {
                    Requirements[*c].coveredBy.insert(r->second.id);
                }
                printf("\n");
            }
        }
    }
    // print inverted matrix (fulfilled via the coveredBy.insert(...) above)
    printf("-- Matrix A Covered By B:\n");
    FOREACH(r, Requirements) {
        if (r->second.coveredBy.empty()) {
            printf("%s covered-by none\n", r->second.id.c_str());
        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r->second.coveredBy) {
                printf("%s covered-by %s\n", r->second.id.c_str(), c->c_str());
            }
        }
    }
}

void printRequirementsCsv()
{
    std::cout << "Requirements,Covered Requirements" << std::endl;

    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (r->second.covers.empty()) {
            std::cout << r->second.id << ',' << std::endl;

        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r->second.covers) {
                std::cout << r->second.id << ',' << *c;

                if (!isReqDefined(*c)) {
                    printf(":Undefined");
                }
                std::cout << std::endl;
            }
        }
    }
    // print inverted matrix
    std::cout << "Requirements,Covered By" << std::endl;
    FOREACH(r, Requirements) {
        if (r->second.coveredBy.empty()) {
            std::cout << r->second.id << ',' << std::endl;

        } else {
            std::set<std::string>::iterator c;
            FOREACH(c, r->second.coveredBy) {
                std::cout << r->second.id << ',' << *c << std::endl;
            }
        }
    }
}

int cmdStat(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    enum REQ_STAT_MODE { REQ_SUMMARY, REQ_UNRESOLVED, REQ_ALL } statusMode = REQ_UNRESOLVED;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        } else if (0 == strcmp(arg, "-s")) {
            statusMode = REQ_SUMMARY;
        } else if (0 == strcmp(arg, "-v")) {
            statusMode = REQ_ALL;
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    r = loadRequirements();
    if (r != 0) return 1;

    // compute statistics
    std::map<std::string, ReqFileConfig>::iterator file;
    std::map<std::string, Requirement>::iterator req;
    FOREACH(req, Requirements) {
        file = ReqConfig.find(req->second.parentDocumentId);
        if (file == ReqConfig.end()) {
            LOG_ERROR("Cannot find parent document of requirement: %s", req->second.id.c_str());
            continue;
        }
        file->second.nTotalRequirements++;
        if (!req->second.coveredBy.empty()) {
            file->second.nCoveredRequirements++;
        }
    }

    // print statistics
    FOREACH(file, ReqConfig) {
        ReqFileConfig f = file->second;
        int ratio = -1;
        if (f.nTotalRequirements > 0) ratio = 100*f.nCoveredRequirements/f.nTotalRequirements;
        printf("%-30s %3d%% %3d / %3d %s\n", f.id.c_str(), ratio,
               f.nCoveredRequirements, f.nTotalRequirements, f.path.c_str());
    }


    return 0;
}

int cmdList(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = DEFAULT_CONF;
    const char *arg = 0;
    const char *exportFormat = "txt";
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        } else if (0 == strcmp(arg, "-x")) {
            if (i>=argc) usage();
            exportFormat = argv[i]; i++;
        }
    }
    if (!configFile) {
        LOG_ERROR(_("No configuration file specified. Please provide a -c option."));
        return 1;
    } else {
        int r = loadConfiguration(configFile);
        if (r != 0) return 1;
    }

    int r = loadRequirements();
    if (r != 0) return 1;

    if (0 == strcmp(exportFormat, "txt")) printRequirements();
    else if (0 == strcmp(exportFormat, "csv")) printRequirementsCsv();
    else {
        LOG_ERROR("Invalid export format: %s", exportFormat);
    }

    return 0;
}

int cmdConfig(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = 0;
    const char *arg = 0;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        }
    }
    if (!configFile) {
        LOG_ERROR(_("No configuration file specified. Please provide a -c option."));
        return 1;
    } else {
        int r = loadConfiguration(configFile);
        if (r != 0) return 1;
    }

    // display a summary of the configuration
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        printf("%s: %s\n", c->first.c_str(), c->second.path.c_str());
    }

    return 0;
}

int cmdPdf(int argc, const char **argv)
{
	if (!argc) usage();
	while (argc) {
        ReqDocumentPdf::dumpText(argv[0], UTF8); // TODO take encoding from command line
		argc--;
		argv++;
	}
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

    const char *command = argv[1];

    if (0 == strcmp(command, "stat"))         return cmdStat(argc-2, argv+2);
    else if (0 == strcmp(command, "version")) return showVersion();
    else if (0 == strcmp(command, "list"))    return cmdList(argc-2, argv+2);
    else if (0 == strcmp(command, "config"))  return cmdConfig(argc-2, argv+2);
    else if (0 == strcmp(command, "regex"))   return cmdRegex(argc-2, argv+2);
    else if (0 == strcmp(command, "pdf"))     return cmdPdf(argc-2, argv+2);
    else usage();


    return 0;
}
