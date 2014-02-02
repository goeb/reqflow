#include <stdlib.h>

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

void usage()
{
    printf("Usage: regex <expr> <text>\n");
    exit(1);
}
int main(int argc, char *argv[])
{
    if (argc < 3) usage();

    regex_t regex;
    int reti;
    char msgbuf[100];

    /* Compile regular expression */
    reti = regcomp(&regex, argv[1], 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    /* Execute regular expression */

    const int N = 5;
    regmatch_t pmatch[N];

    char buffer[256];
    const char *text = argv[2];
    reti = regexec(&regex, text, N, pmatch, 0);
    int i;
    for (i=0; i<100; i++) reti = regexec(&regex, text, N, pmatch, 0);



    if (!reti) {
        puts("Match: \n");
        int i;
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                printf("match[%d]: %s\n", i, buffer);
            } else printf("match[%d]:\n", i);
        }
    } else if (reti == REG_NOMATCH) {
        puts("No match");
    } else {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        exit(1);
    }

    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(&regex);

    return 0;
}
