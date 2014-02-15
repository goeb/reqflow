#ifndef _importerHtml_h
#define _importerHtml_h

#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include "req.h"


class ReqDocumentHtml: public ReqDocument {
public:
    ReqDocumentHtml(const ReqFileConfig &c) {fileConfig = c;}
    int loadRequirements(bool debug);
    int loadHtmlNode(xmlDocPtr doc, xmlNode *a_node, bool debug);
};

#endif
