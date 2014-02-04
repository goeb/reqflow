#ifndef _importerPdf_h
#define _importerPdf_h

#include "req.h"

void loadPdf(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &Requirements);
void extractText(const char *file, Encoding encoding);


#endif
