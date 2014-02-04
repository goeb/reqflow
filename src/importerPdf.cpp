
#include <fstream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-global.h>
typedef std::vector<char> byte_array;

			byte_array x;

#include "importerPdf.h"
#include "logging.h"
#include "req.h"

void loadPdf(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &Requirements)
{
	LOG_DEBUG("loadPdf: %s", fileConfig.path.c_str());

	poppler::document *doc = poppler::document::load_from_file(fileConfig.path.c_str());
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);

	std::string currentRequirement;

	for (int i = 0; i < pagesNbr; ++i) {
		// process the lines
		std::string pageLines;

		switch(fileConfig.encoding) {
		case LATIN1:
			pageLines = doc->create_page(i)->text().to_latin1();
			break;
		case UTF8:
		default:
		{
			byte_array pageText = doc->create_page(i)->text().to_utf8();
			pageLines.assign(pageText.begin(), pageText.end());
			break;
		}
		}

		const char *startOfLine = pageLines.c_str();
		const char *endOfLine = 0;
		while (startOfLine) {
			endOfLine = strstr(startOfLine, "\n");
			int length = 0;
			if (endOfLine) length = endOfLine - startOfLine;
			else length = strlen(startOfLine);

			std::string line;
			line.assign(startOfLine, length);

			// check if line covers a requirement
			std::string ref = getMatchingPattern(fileConfig.refRegex, line.c_str());
			if (!ref.empty()) {
				if (currentRequirement.empty()) {
					LOG_ERROR("Reference found whereas no current requirement: %s", ref.c_str());
				} else {
					Requirements[currentRequirement].covers.insert(ref);
				}
			}

			std::string reqId = getMatchingPattern(fileConfig.tagRegex, line.c_str());

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

			if (!endOfLine) break;
			if (endOfLine == startOfLine + strlen(startOfLine) - 1) break;
			startOfLine = endOfLine + 1;
		}
	}
}
