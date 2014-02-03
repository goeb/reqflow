
#include <fstream>

#include "importerTxt.h"
#include "logging.h"
#include "req.h"

void loadText(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &Requirements)
{
    LOG_DEBUG("loadText: %s", fileConfig.path.c_str());
    const int LINE_SIZE_MAX = 4096;
    char line[LINE_SIZE_MAX];

    std::ifstream ifs(fileConfig.path.c_str(), std::ifstream::in);

    std::string currentRequirement;

    if (!ifs.good()) {
        LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
        return;
    }
    while (ifs.getline(line, LINE_SIZE_MAX)) {

        // check if line covers a requirement
        std::string ref = getMatchingPattern(fileConfig.refRegex, line);
        if (!ref.empty()) {
            if (currentRequirement.empty()) {
                LOG_ERROR("Reference found whereas no current requirement: %s", ref.c_str());
            } else {
                Requirements[currentRequirement].covers.insert(ref);
            }
        }

        std::string reqId = getMatchingPattern(fileConfig.tagRegex, line);

        if (!reqId.empty() && reqId != ref) {

            std::map<std::string, Requirement>::iterator r = Requirements.find(reqId);
            if (r != Requirements.end()) {
                LOG_ERROR("Duplicate requirement %s in documents: '%s' and '%s'",
                          reqId.c_str(), r->second.parentDocumentPath.c_str(), fileConfig.path.c_str());
                currentRequirement.clear();

            } else {
                Requirement req;
                req.id = reqId;
                req.parentDocumentId = fileConfig.id;
                req.parentDocumentPath = fileConfig.path;
                Requirements[reqId] = req;
                currentRequirement = reqId;
            }
        }

    }
}
