#ifndef _importerDocx_h
#define _importerDocx_h

#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "req.h"


void loadDocxXml(ReqFileConfig &fileConfig, const std::string &contents, std::map<std::string, Requirement> &requirements);
void loadDocx(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &Requirements);

#endif
