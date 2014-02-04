#ifndef _importerPdf_h
#define _importerPdf_h

#include "req.h"

class ReqDocumentPdf: public ReqDocument {
public:
    ReqDocumentPdf(ReqFileConfig &c) {fileConfig = c;}
    int loadRequirements();
    static void dumpText(const char *file, Encoding encoding);
};



#endif
