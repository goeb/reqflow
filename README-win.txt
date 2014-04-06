
To launch Reqflow:
    - create a file with a .req extension, as follows,
    - double-click it,
    - this will generate an HTML report and open it.

Sample file 'test.req'

    define ALPHANUM [-a-zA-Z0-9_]
    define REQ_PATTERN "REQ_ALPHANUM+"
    define TST_PATTERN "TST_ALPHANUM+"
    define REF_PATTERN "Ref: (ALPHANUM+)"

    document SPEC -path SPEC.pdf -req REQ_PATTERN -stop-after Annex

    document TEST -path TEST.docx
        -req TST_PATTERN
        -ref REF_PATTERN
        -stop-after Annex
        -nocov
    
