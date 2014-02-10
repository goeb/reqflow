
# Req Traceability Tool

Req is a free and open-source traceability tool designed to analyse documents with speed and efficiency.

## Supported Input Formats

- docx (Open Xml)
- text
- pdf (experimental, Linux only)


## Download

### Linux

    git clone https://github.com/goeb/req.git
    make
    cd test && ../req stat -s

### Windows

[req-v1.0.zip](download/req-v1.0.zip)

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
                    (not supported on Windows)

    regex <pattern> <text>
                    Test regex given by <pattern> applied on <text>.

    version
    help

Options:
    -c <config> Select configuration file. Defaults to 'req.conf'.

```

## Sample Configuration File

`req.conf`

```
# addFile <document-id> -path <document-path> -req <pattern> \
#         [-stop-after <pattern>] [-ref <pattern>] [-start-after <pattern>]
#
# <pattern> must be a Perl Compatible Regular Expression (PCRE)
# -req indicates how the requirements must be captured
# -ref indicates how the referenced requirements must be captured
# 
# Parameters containing spaces must be enclosed by quotes: "The file.doxc"
# Escape character (for quotes, etc.): antislash (\).
# Thus any \ must be written \\.
# 

addFile SPEC -path SPEC.docx -req REQ_[-a-zA-Z0-9_]* -stop-after Annex
addFile TEST -path TEST.txt \
    -req T_[-a-zA-Z0-9_]* \
    -ref "Ref:  *(.*)" \
    -stop-after "Annex" \
    -start-after "Tests cases"

```

## External Dependencies

- libz
- libzip
- libxml2
- lippoppler-cpp

(The Windows release includes these as static libraries)


## License GPLv2

Req
Copyright (C) 2013 Frederic Hoerni

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

