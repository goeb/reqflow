
# Reqflow

Reqflow is a free and open-source tool for traceability of requirements across documents, designed to analyse documents with speed and efficiency.

## Supported Formats

### Input formats

- docx (Open Xml)
- odt (Open Document)
- text
- HTML
- PDF

### Output formats

- text
- CSV
- HTML

## Example of Report

No screenshot, but an example: [reqReport.html](reqReport.html)

## Download

### Linux

    git clone https://github.com/goeb/reqflow.git
    make
    cd test && ../req stat -s

### Windows

[reqflow-v1.2.1.zip](download/reqflow-v1.2.1.zip)

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
         [-x <fmt>] Select export format: text (default), csv, html.
                    If format 'html' is chosen, -r is ignored, as both foward and reverse
                    traceability matrices are displayed.

    review          Print the requirements with their text (experimental).
         [-f | -r]  Print also traceability (forward or backward) (not implemented yet)
         [-x <fmt>] Choose format: txt, csv.

    config          Print the list of configured documents.

    debug <file>    Dump text extracted from file (debug purpose).
                    (PDF not supported on Windows)

    regex <pattern> <text>
                    Test regex given by <pattern> applied on <text>.

    version
    help

Options:
    -c <config> Select configuration file. Defaults to 'conf.req'.

```

## Sample Configuration File

`conf.req`

```
# document <document-id> -path <document-path> -req <pattern> \
#          [-stop-after <pattern>] [-ref <pattern>] [-start-after <pattern>] \
#          [-nocov]
#
# <pattern> must be a Perl Compatible Regular Expression (PCRE)
# -req indicates how the requirements must be captured
# -ref indicates how the referenced requirements must be captured
# 
# Parameters containing spaces must be enclosed by quotes: "The file.doxc"
# Escape character (for quotes, etc.): antislash (\).
# Thus any \ must be written \\.
# 
# Keyword 'define' may be used to define values:
# 
#   define PATH
#   document x -path PATH/x.txt
#   document y -path PATH/y.txt

document SPEC -path SPEC.docx -req REQ_[-a-zA-Z0-9_]* -stop-after Annex
document TEST -path TEST.txt \
    -req T_[-a-zA-Z0-9_]* \
    -ref "Ref:  *(.*)" \
    -stop-after "Annex" \
    -start-after "Tests cases" \
    -nocov

```

## External Dependencies

- libz, libzip
- libxml2
- libpoppler, libpoppler-cpp
- libpcreposix, libpcre

The Windows release includes these as static libraries.


## License GPLv2+

Reqflow
Copyright (C) 2014 Frederic Hoerni

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

