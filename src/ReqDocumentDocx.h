#ifndef _importerDocx_h
#define _importerDocx_h

#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "req.h"


class ReqDocumentDocx: public ReqDocument {
public:
    ReqDocumentDocx(ReqFileConfig &c) {fileConfig = c;}
    int loadRequirements();
};

class ReqDocumentDocxXml: public ReqDocument {
public:
    ReqDocumentDocxXml(ReqFileConfig &c) {fileConfig = c;}
    int loadRequirements();
    int loadContents(const char *xml, size_t size);
    int loadDocxXmlNode(xmlDocPtr doc, xmlNode *a_node);


private:
    std::string contents;
};

#endif
