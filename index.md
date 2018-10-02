
# Reqflow

Reqflow is a free and open-source tool for traceability of requirements across documents, designed to analyse documents with speed and efficiency.

## News

- 2 Oct 2018: Reqflow v1.5.3 (see [Changes](https://raw.githubusercontent.com/goeb/reqflow/master/NEWS))
- 31 Oct 2017 Reqflow v1.5.2
- 13 Feb 2017 Reqflow v1.5.1
- 22 Nov 2016: Reqflow v1.5.0


## Supported Formats

### Input formats

- docx (Open Xml)
- odt (Open Document)
- text (extensions .txt, .ad, .adoc, .asc, .asciidoc, .md)
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

Build From Source :

    git clone https://github.com/goeb/reqflow.git
    cd reqflow
    ./configure
    make
    cd test && ../reqflow stat -s

### Windows

Latest stable:

* [reqflow-1.5.3-setup.exe](https://github.com/goeb/reqflow/releases/download/v1.5.3/reqflow-1.5.3-setup.exe)


Previous versions:

* [reqflow-1.5.1-setup.exe](https://github.com/goeb/reqflow/releases/download/v1.5.1/reqflow-1.5.1-setup.exe)
* [reqflow-1.5.2-setup.exe](https://github.com/goeb/reqflow/releases/download/v1.5.2/reqflow-1.5.2-setup.exe)

## Usage

```
Usage: 1. reqflow <command> [<options>] [<args>]
       2. reqflow <config-file>


1. reqflow <command> [<options>] [<args>]

Commands:

    stat [doc ...]  Print the status of requirements in all documents or the
                    given documents. Without additionnal option, only
                    unresolved coverage issues are reported.
         -s         Print a one-line summary for each document.
         -v         Print the status of all requirements.
                    Status codes:

                      'U'  Uncovered

    trac [doc ...]  Print the traceability matrix of the requirements 
                    (A covered by B).
         [-r]       Print the reverse traceability matrix (A covers B).
         [-x <fmt>] Select export format: text (default), csv, html.
                    If format 'html' is chosen, -r is ignored, as both foward
                    and reverse traceability matrices are displayed.

    review          Print the requirements with their text.
         [-f | -r]  Print also traceability (forward or backward)
         [-x <fmt>] Choose format: txt, csv.

    config          Print the list of configured documents.

    debug <file>    Dump text extracted from file (debug purpose).

    regex <pattern> [<text>]
                    Test regex given by <pattern> applied on <text>.
                    If <text> is omitted, then the text is read from stdin.

    version
    help

Options:
    -c <config>  Select configuration file. Defaults to 'conf.req'.
    -o <file>    Output to file instead of stdout.
                 Not supported for commands 'config', debug' and 'regex'.

2. reqflow <config>
This is equivalent to:
    reqflow trac -c <config> -x html -o <outfile> && start <outfile>

Purpose: This usage is suitable for double-cliking on the config file.
Note: <config> must be different from the commands of use case 1.
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
    -nocov -sort alphanum

```

## Environment Variables

Environment variables can be inserted in the configuration file, using a dollar sign ($) prefix.

Example:

```
document SPEC -path $DOCUMENT -req REQ_[-a-zA-Z0-9_]*
# or
document SPEC -path ${DOCUMENT} -req REQ_[-a-zA-Z0-9_]*
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

