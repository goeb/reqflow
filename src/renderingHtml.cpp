#include <time.h>
#include <sys/time.h>

#include "renderingHtml.h"
#include "logging.h"
#include "req.h"
#include "global.h"
#include "stringTools.h"

/** Encode a string for HREF
  *
  */
std::string hrefEncode(const std::string &src, char mark='%', const char *dontEscape="._-$,;~()/")
{
    static const char *hex = "0123456789abcdef";
    std::string dst;
    size_t n = src.size();
    size_t i;
    for (i = 0; i < n; i++) {
        if (isalnum((unsigned char) src[i]) ||
                strchr(dontEscape, (unsigned char) src[i]) != NULL) dst += src[i];
        else {
            dst += mark;
            dst += hex[((const unsigned char) src[i]) >> 4];
            dst += hex[((const unsigned char) src[i]) & 0xf];
        }
    }
    return dst;
}

std::string htmlEscape(const std::string &value)
{
    std::string result = replaceAll(value, '&', "&#38;");
    result = replaceAll(result, '"', "&quot;");
    result = replaceAll(result, '<', "&lt;");
    result = replaceAll(result, '>', "&gt;");
    result = replaceAll(result, '\'', "&#39;");
    return result;
}

void htmlPrintHeader()
{
    printf("<!DOCTYPE HTML><html>\n"
           "<head>\n"
           "<title>Requirements Traceability</title>\n"
           "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">\n"
           "<style>\n"
           "body { font-family: Verdana,sans-serif; }\n"
           "h1 { border-bottom: 1px #bbb solid; margin-top: 15px;}\n"
           "a[href], a[href]:link { color: blue; text-decoration: none; }\n"
           "a[href]:hover { color: blue; text-decoration: underline; }\n"
           ".r_footer { font-size: small; font-family: monospace; color: grey; }\n"
           ".r_document_summary { font-size: small; }\n"
           ".r_errors { border: 1px solid #BBB; white-space: pre; font-family: monospace; color: red; padding: 0.5em;}\n"
           ".r_errors_summary { padding: 1em; font-size: 200%%; position: absolute; right: 15px; top: 20px; background-color: #FBB; border: 1px solid black;}\n"
           ".r_warning { background-color: #FBB; }\n"
           ".r_no_error { color: grey; }\n"
           "table { border-collapse:collapse; }\n"
           "td.r_summary { text-align:right; border-bottom: 1px grey solid; padding-left: 1em; }\n"
           "td.r_summary_l { text-align:left; border-bottom: 1px grey solid; padding-left: 1em; }\n"
           "th.r_summary { text-align:center; padding-left: 1em; }\n"
           "td.r_matrix { text-align:left; border: 1px grey solid; padding-left: 1em; }\n"
           "th.r_matrix { text-align:left; padding-left: 1em; }\n"
           "td.r_upstream { border: 1px solid grey; padding: 1em;}\n"
           "td.r_downstream { border: 1px solid grey; padding: 1em; }\n"
           ".r_no_coverage { color: grey; }\n"
           "</style>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Requirements Traceability</h1>"
           );
}

void htmlPrintErrors()
{
    if (Errors.size()) {
        printf("<div class=\"r_errors_summary\">");
        printf("<a href=\"#r_errors\">Error(s): %d</a></div>\n", Errors.size());
    }

    printf("<h1 id=\"r_errors\">Errors</h1>\n");
    if (Errors.empty()) {
        printf("<div class=\"r_no_error\">\n");
        printf("No Error.\n");
    } else {
        printf("<div class=\"r_errors\">");
        printf("Error(s): %d\n", Errors.size());
        std::list<std::string>::iterator e;
        FOREACH(e, Errors) {
            printf("%s\n", htmlEscape(*e).c_str());
        }
    }
    printf("</div>\n");
}

/** a ratio -1 indicates that this info is not relevant (no coverage needed)
 */
void htmlPrintSummaryRow(const char *docId, int ratio, int covered, int total, const char *path )
{
    const char *warning = "";
    if (ratio != 100 && ratio != -1) warning = "r_warning";
    printf("<tr class=\"%s\"><td class=\"r_summary_l\">", warning);
    if (strlen(path)) printf("<a href=\"#%s\">", hrefEncode(docId).c_str()); // do not print href for the "total" line (no path)
    printf("%s", htmlEscape(docId).c_str());
    if (strlen(path)) printf("</a>"); // do not print href for the "total" line (no path)
    printf("</td>");

	if (ratio != -1) {
		printf("<td class=\"r_summary\">%d</td>"
			"<td class=\"r_summary\">%d</td>",
			ratio, covered);
	} else {
#define NOCOV "<span title=\"Coverage not relevant\r\n(option -nocov)\">nocov</span>"
		printf("<td class=\"r_summary\">" NOCOV "</td><td class=\"r_summary\">" NOCOV "</td>");
	}

	printf("<td class=\"r_summary\">%d</td>", total);
    printf("<td class=\"r_summary_l\">");
    if (strlen(path)) printf("<a href=\"%s\">", hrefEncode(path).c_str()); // do not print href for the "total" line (no path)
    printf("%s", htmlEscape(path).c_str());
    if (strlen(path)) printf("</a>"); // do not print href for the "total" line (no path)
    printf("</td></tr>\n");
}

void htmlPrintSummaryOfFile(const ReqFileConfig &f, int &total, int &covered)
{
	int ratio = 0;
	if (f.nocov) {
		ratio = -1;
	} else {
		total += f.nTotalRequirements;
		covered += f.nCoveredRequirements;
		if (f.nTotalRequirements > 0) ratio = 100*f.nCoveredRequirements/f.nTotalRequirements;
	}

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

    int ratio = 0;
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

void htmlPrintTraceabilityRow(const char *req1, const char *req2, const char *doc2Id, bool warning)
{
    const char *warningStyle = "";
    if (warning) warningStyle = "r_warning";

    printf("<tr class=\"r_matrix %s\">", warningStyle);
    printf("<td class=\"r_matrix\">%s</td>", req1);
    printf("<td class=\"r_matrix\">%s</td>", req2);

    // doc id
    printf("<td class=\"r_matrix\">");
    if (doc2Id) {
        printf("<a href=\"#%s\">%s</a>", hrefEncode(doc2Id).c_str(), htmlEscape(doc2Id).c_str());
    }
    printf("</td>");
    printf("\n");
}


void htmlPrintTraceability(const Requirement &r, bool forward)
{
    if (forward) { // A covered by B
        if (r.coveredBy.empty()) htmlPrintTraceabilityRow(r.id.c_str(), "", "", true);
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

                htmlPrintTraceabilityRow(r.id.c_str(), c->c_str(), docId, warning);
            }
        }
    } else { // A covering B
        if (r.covers.empty()) htmlPrintTraceabilityRow(r.id.c_str(), "", "", false);
        else {
            std::set<std::string>::iterator c;
            FOREACH(c, r.covers) {
                const char* docId = 0;
                Requirement *ref = getRequirement(c->c_str());
                if (!ref) docId = "Undefined";
                else docId = ref->parentDocumentId.c_str();

                htmlPrintTraceabilityRow(r.id.c_str(), c->c_str(), docId, false);
            }
        }

    }
}

void htmlPrintDependencies(const ReqFileConfig &f)
{
    printf("<h2>Dependencies for: %s</h2>", htmlEscape(f.id).c_str());

    printf("<table class=\"r_dependencies\">");
    printf("<tr class=\"r_dependencies\">");
    printf("<th class=\"r_dependencies\">Upstream Documents</th>");
    printf("<th class=\"r_dependencies\"></th>");
    printf("<th class=\"r_dependencies\">Downstream Documents</th>");
    printf("</tr>\n");

    std::set<std::string>::iterator doc;
    // upstream documents
    printf("<td class=\"r_upstream\">");
    FOREACH(doc, f.upstreamDocuments) {
        printf("<a href=\"#%s\">%s</a><br>", hrefEncode(*doc).c_str(), htmlEscape(*doc).c_str());
    }
    printf("</td>");

    printf("<td>-> %s-></td>", htmlEscape(f.id).c_str());

    // downstream documents
    printf("<td class=\"r_downstream\">");
    FOREACH(doc, f.downstreamDocuments) {
        printf("<a href=\"#%s\">%s</a><br>", hrefEncode(*doc).c_str(), htmlEscape(*doc).c_str());
    }
    printf("</td>");

    printf("</table>\n");

}

void htmlPrintMatrix(const ReqFileConfig &f, bool forward)
{

    printf("<h2>");
    if (forward) printf("Forward Coverage");
    else printf("Reverse Coverage");
    printf(" for: %s</h2>\n", htmlEscape(f.id).c_str());

    if (forward && f.nocov) {
        printf("<div class=\"r_no_coverage\">No forward tracebility configured (-nocov flag).</div>");
        return;
    }

    printf("<table class=\"r_matrix\">");
    printf("<tr class=\"r_matrix\"><th class=\"r_matrix\">Requirements</th>");
    if (forward) {
        printf("<th class=\"r_matrix\">Descendants</th>");
        printf("<th class=\"r_matrix\">Downstream Documents</th>");
    } else {
        // reverse
        printf("<th class=\"r_matrix\">Origins</th>");
        printf("<th class=\"r_matrix\">Upstream Documents</th>");
    }
    printf("</tr>\n");

    std::map<std::string, Requirement>::iterator r;
    FOREACH(r, Requirements) {
        if (r->second.parentDocumentId == f.id) {
            htmlPrintTraceability(r->second, forward);
        }
    }
    printf("</table>\n");
}

void htmlPrintAllTraceability(const std::list<std::string> documents)
{
    std::list<std::string>::const_iterator docId;
    FOREACH(docId, documents) {
        std::map<std::string, ReqFileConfig>::const_iterator file = ReqConfig.find(*docId);
        if (file == ReqConfig.end()) PUSH_ERROR("Invalid document id: %s", docId->c_str());
        else {
            ReqFileConfig f = file->second;
            printf("<h1 id=\"%s\">%s</h1>", hrefEncode(*docId).c_str(), htmlEscape(*docId).c_str());

            printf("<div class=\"r_document_summary\">");
            printf("Path: <a href=\"%s\">%s</a><br>", hrefEncode(f.path).c_str(), htmlEscape(f.path).c_str());
            printf("Requirements: %d<br>", f.nTotalRequirements);
            if (!f.nocov) {
                int ratio = 0;
                if (f.nTotalRequirements>0) ratio = 100*f.nCoveredRequirements/f.nTotalRequirements;
                printf("Covered: %d (%d%%)<br>", f.nCoveredRequirements, ratio);
            }
            printf("</div>\n");


            htmlPrintDependencies(f);
            htmlPrintMatrix(f, true);
            htmlPrintMatrix(f, false);
        }
    }
}

void htmlPrintFooter(const std::string &cmdline)
{
    printf("<br><br>\n");
    printf("<div class=\"r_footer\">Date: %s<br>Command Line: %s<br>Version: %s</div><br>",
           getDatetime().c_str(), cmdline.c_str(), VERSION);
    printf("</body></html>\n");
}


void htmlRender(const std::string &cmdline, int argc, const char **argv)
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

    // print summary
    htmlPrintSummary(documents);

    // print dependencies between documents

    // print coverage for each document (A>B and A<B)
    htmlPrintAllTraceability(documents);

    // print errors
    htmlPrintErrors();

    // print footer
    htmlPrintFooter(cmdline);
}
