
# Capture Algorithm

Reqflow parses documents sequentially and works line by line.

Reqflow considers the following as a single line:

* a line of a raw text or PDF document
* a `<p>` paragraph of an XML based document (HTML, docx, odt)

Reqflow expects the document to be structured as a sequence of requirements with mixed text, where a requirement REQ is structured as follows:


```
REQ        := LINE_REQ
              [ LINES_TEXT ]
              [ REFS ]

LINE_REQ   := REQ_ID [ REFS ]

REQ_ID     := the unique identifier of the requirement

LINES_TEXT := some text, on one or more lines

REFS       := REF [ REF ... ]

REF        := the unique identifier of a reference

```

As a result:

* a REQ_ID is always before its REF.
* a REF is always associated to the REQ_ID before it


The Reqflow parameters are as follows:

* `-req` tells how to capture the REQ. Parentheses may be used to identify REQ inside a broader expression.
	* Eg: `<(REQ_[-a-zA-Z_0-9]*)>` for matching `<REQ_123>` and extracting `REQ_123`
* `-ref` tells how to capture the REFS. Parentheses may be used to identify one or more REFS inside a broader expression.
	* Eg: `Ref:[, ]*(REF_[0-9]+)` for matching `Ref: REF_01, REF_02, REF_03` and extracting `REF_01`, `REF_02`, `REF_03`
	* Eg: `REF_[0-9]+` for matching `REF_01`, and extracting the same.
* `-end-req` tells where the capture of the text of the requirement shall end


