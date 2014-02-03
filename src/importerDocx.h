#ifndef _importerDocx_h
#define _importerDocx_h

#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "req.h"


void loadDocxXmlNode(ReqFileConfig &fileConfig, xmlDocPtr doc, xmlNode *a_node);
void loadDocxXml(ReqFileConfig &fileConfig, const std::string &contents);
void loadDocx(ReqFileConfig &fileConfig);

#endif
