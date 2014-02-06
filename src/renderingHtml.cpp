#include <time.h>
#include <sys/time.h>

#include "renderingHtml.h"
#include "logging.h"

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
           ".r_date { font-size: small; }"
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

void htmlPrintFooter()
{
    printf("</body></html>\n");
}


void htmlRender(int argc, char **argv)
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
    }


    // print header
    htmlPrintHeader();

    // print table of contents
    htmlPrintToc();

    // print errors

    // print summary

    // print dependencies between documents

    // print coverage for each document (A>B and A<B)

    // print footer
    htmlPrintFooter();
}
