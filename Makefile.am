# Manage configure options
AM_CPPFLAGS=-Wall
AM_CFLAGS=-Wall
AM_CXXFLAGS=-Wall
AM_LDFLAGS=

if GCOV_ENABLED
    AM_CFLAGS += -O0 -g3 -fprofile-arcs -ftest-coverage
    AM_CXXFLAGS += -O0 -g3 -fprofile-arcs -ftest-coverage
    AM_LDFLAGS += -lgcov
endif

if GPROF_ENABLED
    AM_CFLAGS += -O0 -g3 -pg -fprofile-arcs
    AM_CXXFLAGS += -O0 -g3 -pg -fprofile-arcs
    AM_LDFLAGS += -pg -lgcov
endif

if WINDOWS
    AM_CFLAGS += -static -static-libgcc
    AM_CFLAGS += -DZIP_EXTERN= -DZLIB_INTERNAL -DLIBXML_STATIC -DPCRE_STATIC -Dpoppler_cpp_EXPORTS
    
    AM_CXXFLAGS += -static -static-libgcc
    AM_CXXFLAGS += -DZIP_EXTERN= -DZLIB_INTERNAL -DLIBXML_STATIC -DPCRE_STATIC -Dpoppler_cpp_EXPORTS
endif


# Main program
bin_PROGRAMS = req
req_SOURCES = src/main.cpp \
    src/logging.cpp \
    src/dateTools.cpp \
    src/parseConfig.cpp \
    src/req.cpp \
    src/ReqDocumentTxt.cpp \
    src/ReqDocumentDocx.cpp \
    src/ReqDocumentHtml.cpp \
    src/stringTools.cpp \
    src/renderingHtml.cpp \
    src/ReqDocumentPdf.cpp

req_CPPFLAGS = @ZIP_CFLAGS@ @XML_CFLAGS@ @POPPLER_CFLAGS@ @PCRE_CFLAGS@
req_LDADD = @ZIP_LIBS@ @XML_LIBS@ @POPPLER_LIBS@ @PCRE_LIBS@

# specific subdirs
SUBDIRS = test

# data
dist_pkgdata_DATA = README COPYING NEWS ChangeLog AUTHORS

# it will re-generate the ChangeLog file
dist-hook: bin/gitlog2changelog.py
	bin/gitlog2changelog.py

# Test sub-programs (will not be packaged)
main_pdf_SOURCES = src/main_pdf.cpp
main_pdf_CPPFLAGS = @POPPLER_CFLAGS@
main_pdf_LDADD = @POPPLER_LIBS@

main_regex_SOURCES = src/main_regex.c
main_regex_CPPFLAGS = @PCRE_CFLAGS@
main_regex_LDADD = @PCRE_LIBS@

main_unzip_SOURCES = src/main_unzip.c
main_unzip_CPPFLAGS = @ZIP_CFLAGS@
main_unzip_LDADD = @ZIP_LIBS@

main_xml_SOURCES = src/main_xml.c
main_xml_CPPFLAGS = @XML_CFLAGS@
main_xml_LDADD = @XML_LIBS@

main_testStringCompare_SOURCES = src/main_testStringCompare.cpp \
    src/stringTools.cpp
main_testStringCompare_CPPFLAGS = @PCRE_CFLAGS@
main_testStringCompare_LDADD = @PCRE_LIBS@

noinst_PROGRAMS = main_pdf main_regex main_unzip main_xml main_testStringCompare

# Windows installer
if WINDOWS
all-local: @PACKAGE_TARNAME@-@VERSION@-setup$(EXEEXT)

@PACKAGE_TARNAME@-@VERSION@-setup$(EXEEXT): reqflow.iss req$(EXEEXT)
	$(WINE) "@ISCC@" /q /dMyAppName=@PACKAGE_NAME@ /dMyAppVersion=@PACKAGE_VERSION@ /dMyAppExeName=req$(EXEEXT) /f@PACKAGE_TARNAME@-@VERSION@-setup $<
endif

clean-local:
	rm -f @PACKAGE_TARNAME@-@VERSION@-setup$(EXEEXT)
	rm -f src/*.gcno src/*.gcda *.*.gcov

