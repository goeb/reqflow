#include <time.h>
#include <sys/time.h>

#include "renderingHtml.h"
#include "logging.h"
#include "req.h"
#include "global.h"

std::string getDatetime()
{
    struct tm date;
    struct timeval tv;
    gettimeofday(&tv, 0);
    localtime_r(&tv.tv_sec, &date);
    //int milliseconds = tv.tv_usec / 1000;
    char buffer[100];
    sprintf(buffer, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", date.tm_year + 1900,
            date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    return buffer;
}

void htmlPrintHeader()
{
    printf("<!DOCTYPE HTML><html>\n"
           "<head>\n"
           "<title>Requirements Coverage</title>\n"
           "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">\n"
           "<style>\n"
           "body { font-family: Verdana,sans-serif; }\n"
           ".r_date { font-size: small; }\n"
           ".r_path { font-size: small; }\n"
           ".r_errors { border: 1px solid #BBB; white-space: pre; font-bold; color: red; }\n"
           ".r_warning { color: red; font-weight: bold; }\n"
           ".r_no_error { color: grey; }\n"
           "table { border-collapse:collapse; }\n"
           "td.r_summary { text-align:right; border-bottom: 1px grey solid; padding-left: 1em; }\n"
           "th.r_summary { text-align:center; padding-left: 1em; }\n"
           "td.r_coverage { text-align:left; border-bottom: 1px grey solid; padding-left: 1em; }\n"
           "th.r_coverage { text-align:left; padding-left: 1em; }\n"
           "</style>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Requirements Coverage</h1>"
           "<div class=\"r_date\">Generated: %s</div>",
           getDatetime().c_str()
           );
}

void htmlPrintToc()
{
    // TODO
}

void htmlPrintErrors()
{

    if (Errors.empty()) {
        printf("<div class=\"r_no_error\">\n");
        printf("No Error.\n");
    } else {
        printf("<div class=\"r_errors\">\n");
        fprintf(stderr, "Error(s): %d\n", Errors.size());
        std::list<std::string>::iterator e;
        FOREACH(e, Errors) {
            fprintf(stderr, "%s\n", e->c_str());
        }
    }
    printf("</div>\n");
}

void htmlPrintSummaryRow(const char *docId, int ratio, int covered, int total, const char *path)
{
    const char *warning = "";
    if (ratio != 100) warning = "r_warning";
    printf("<tr class=\"r_summary %s\"><td class=\"r_summary\">%s</td>"
           "<td class=\"r_summary\">%d</td>"
           "<td class=\"r_summary\">%d</td>"
           "<td class=\"r_summary\">%d</td>"
           "<td class=\"r_summary\">%s</td></tr>\n",
           warning, docId, ratio, covered, total, path);

}

void htmlPrintSummaryOfFile(const ReqFileConfig &f, int &total, int &covered)
{
    total += f.nTotalRequirements;
    covered += f.nCoveredRequirements;

    int ratio = -1;
    if (f.nTotalRequirements > 0) ratio = 100*f.nCoveredRequirements/f.nTotalRequirements;

    htmlPrintSummaryRow(f.id.c_str(), ratio, f.nCoveredRequirements, f.nTotalRequirements, f.path.c_str());
}

void htmlPrintSummaryContents(const std::list<std::string> &documents)
{
    std::list<std::string>::const_iterator docId;
    int total = 0;
    int covered = 0;

    FOREACH(docId, documents) {
        std::map<std::string, ReqFileConfig>::const_iterator file = ReqConfig.find(*docId);
        if (file == ReqConfig.end()) PUSH_ERROR("Invalid document id: %s", docId->c_str());
        else htmlPrintSummaryOfFile(file->second, total, covered);
    }

    int ratio = -1;
    if (total > 0) ratio = 100*covered/total;
    htmlPrintSummaryRow("Total", ratio, covered, total, "");

}

void htmlPrintSummary(const std::list<std::string> &documents)
{
    printf("<table class=\"r_summary\">");
    printf("<tr class=\"r_summary\"><th class=\"r_summary\">Documents</th>");
    printf("<th class=\"r_summary\">Coverage (%%)</th>");
    printf("<th class=\"r_summary\">Req Covered</th>");
    printf("<th class=\"r_summary\">Req Total</th>");
    printf("<th class=\"r_summary\">Document Path</th>");
    printf("</tr>");


    htmlPrintSummaryContents(documents);

    printf("</table>");
}

void htmlPrintCoverageRow(const char *req1, const char *req2, const char *doc2Id, bool warning)
{
    const char *warningStyle = "";
    if (warning) warningStyle = "r_warning";

    printf("<tr class=\"r_coverage %s\">", warningStyle);
    printf("<td class=\"r_coverage\">%s</td>", req1);
    printf("<td class=\"r_coverage\">%s</td>", req2);
    printf("<td class=\"r_coverage\">%s</td>", doc2Id);
    printf("\n");
}


void htmlPrintCoverage(const Requirement &r, bool forward)
{
    if (forward) { // A covered by B
        if (r.coveredBy.empty()) htmlPrintCoverageRow(r.id.c_str(), "", "", true);
        else {
            std::set<std::string>::iterator c;
            FOREACH(c, r.coveredBy) {
                const char* docId = 0;
                bool warning = false;
                Requirement *above = getRequirement(c->c_str());
                if (!above) {
                    docId = "Undefined";
                    warning = true;
                } else docId = above->parentDocumentId.c_str();

                htmlPrintCoverageRow(r.id.c_str(), c->c_str(), docId, warning);
            }
        }
    } else { // A covering B
        if (r.covers.empty()) htmlPrintCoverageRow(r.id.c_str(), "", "", false);
        else {
            std::set<std::string>::iterator c;
            FOREACH(c, r.covers) {
                const char* docId = 0;
                Requirement *ref = getRequirement(c->c_str());
                if (!ref) docId = "Undefined";
                else docId = ref->parentDocumentId.c_str();

                htmlPrintCoverageRow(r.id.c_str(), c->c_str(), docId, false);
            }
        }

    }
}

void htmlPrintMatrix(const ReqFileConfig &f, bool forward)
{

    printf("<h2 name=\"%s\">", f.id.c_str());
    if (forward) printf("Forward Coverage");
    else printf("Reverse Coverage");
    printf(" for: %s</h2>\n", f.id.c_str());

    printf("<div class=\"r_path\">%s</div>", f.path.c_str());

    printf("<table class=\"r_coverage\">");
    printf("<tr class=\"r_coverage\"><th class=\"r_coverage\">Requirements</th>");
    if (forward) {
        printf("<th class=\"r_coverage\">Requirements Above</th>");
        printf("<th class=\"r_coverage\">Documents Above</th>");
    } else {
        // reverse
        printf("<th class=\"r_coverage\">Requirements Underneath</th>");
        printf("<th class=\"r_coverage\">Documents Underneath</th>");
    }
    printf("</tr>\n");

    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (r->second.parentDocumentId == f.id) {
            htmlPrintCoverage(r->second, forward);
        }
    }
    printf("</table>\n");
}

void htmlPrintCoverage(const std::list<std::string> documents, bool forward)
{
    std::list<std::string>::const_iterator docId;
    FOREACH(docId, documents) {
        std::map<std::string, ReqFileConfig>::const_iterator file = ReqConfig.find(*docId);
        if (file == ReqConfig.end()) PUSH_ERROR("Invalid document id: %s", docId->c_str());
        else htmlPrintMatrix(file->second, forward);
    }
}

void htmlPrintAllCoverage(const std::list<std::string> &documents)
{
    // forward coverage (A covered by B)
    printf("<div class=\"r_coverage\">");

    htmlPrintCoverage(documents, true);

    htmlPrintCoverage(documents, false);

    printf("</div>");
}


void htmlPrintFooter()
{
    printf("</body></html>\n");
}


void htmlRender(int argc, const char **argv)
{

    // parse HTML specific options
    // no option at the moment

    // build list of documents
    std::list<std::string> documents;
    if (argc) {
        while (argc) {
            documents.push_back(argv[0]);
            argv++; argc--;
        }
    } else {
        // take all documents
        std::map<std::string, ReqFileConfig>::const_iterator file;
        FOREACH(file, ReqConfig) documents.push_back(file->first);
    }


    // print header
    htmlPrintHeader();

    // print table of contents
    htmlPrintToc();

    // print summary
    htmlPrintSummary(documents);

    // print dependencies between documents

    // print coverage for each document (A>B and A<B)
    htmlPrintAllCoverage(documents);

    // print errors
    htmlPrintErrors();

    // print footer
    htmlPrintFooter();
}
