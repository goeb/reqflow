
#include <fstream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-global.h>
typedef std::vector<char> byte_array;

			byte_array x;

#include "importerPdf.h"
#include "logging.h"
#include "req.h"

void extractText(const char *file, Encoding encoding)
{
	poppler::document *doc = poppler::document::load_from_file(file);
	if (!doc) {
		LOG_ERROR("Cannot open file: %s", file);
		return;
	}
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);
	
	for (int i = 0; i < pagesNbr; ++i) {
        switch(encoding) {
        case LATIN1:
            printf("%s", doc->create_page(i)->text().to_latin1().c_str());
            break;
        case UTF8:
        default:
        {
            byte_array pageText = doc->create_page(i)->text().to_utf8();
            std::string pageLines(pageText.begin(), pageText.end());
			printf("%s", pageLines.c_str());
            break;
        }
        }
	}
}

void loadPdf(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &Requirements)
{
	LOG_DEBUG("loadPdf: %s", fileConfig.path.c_str());

	poppler::document *doc = poppler::document::load_from_file(fileConfig.path.c_str());
	if (!doc) {
		LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
		return;
	}
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);

	std::string currentRequirement;

	bool started = true;
	if (fileConfig.startAfterRegex) started = false;

	for (int i = 0; i < pagesNbr; ++i) {
		LOG_DEBUG("page %d", i+1);
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

			// check the startAfter pattern
			if (!started && fileConfig.startAfterRegex) {
				std::string start = getMatchingPattern(fileConfig.startAfterRegex, line.c_str());
				if (!start.empty()) started = true;
			}

			if (started) {
				// check the stopAfter pattern
				std::string stop = getMatchingPattern(fileConfig.stopAfterRegex, line.c_str());
				if (!stop.empty()) {
					LOG_DEBUG("stop: %s", stop.c_str());
					LOG_DEBUG("line: %s", line.c_str());
					delete doc;
					return;
				}

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
			}

			if (!endOfLine) break;
			if (endOfLine == startOfLine + strlen(startOfLine) - 1) break;
			startOfLine = endOfLine + 1;
		}
	}
	delete doc;
}
