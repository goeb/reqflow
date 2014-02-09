

# Overview

`req` is a software tool for helping on Requirement Traceability across documents.


# Command line interface :

## Usage
```
Usage: req <command> [<options>] [<args>]

Commands:

    stat [doc ...]  Print the status of requirements in all documents or the given documents.
                    Without additionnal option, only unresolved coverage issues are reported.
         -s         Print a one-line summary for each document.
         -v         Print the status of all requirements.
                    Status codes:

                        'U'  Uncovered

    trac [doc ...]  Print the traceability matrix of the requirements (A covered by B).
         [-r]       Print the reverse traceability matrix (A covers B).
         [-x <fmt>] Select export format: text (default), csv.

    config          Print the list of configured documents.

    report [-html]  Generate HTML report

    pdf <file>      Dump text extracted from pdf file (debug purpose).

    regex <pattern> <text>
                    Test regex given by <pattern> applied on <text>.

    version
    help

Options:
    -c <config> Select configuration file. Defaults to 'req.conf'.
```

# Supported Input Formats

- docx (Open Xml)
- text
- pdf (experimental)

# External Dependencies

- libzip
- libxml2
- lippoppler-cpp

# License 

GPLv2


