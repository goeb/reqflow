# Manage configure options
AM_CPPFLAGS=-Wall
AM_CFLAGS=-Wall
AM_CXXFLAGS=-Wall
AM_LDFLAGS=

if GCOV_ENABLED
    AM_CFLAGS+=-fprofile-arcs -ftest-coverage
    AM_CXXFLAGS+=-fprofile-arcs -ftest-coverage
    AM_LDFLAGS+=-lgcov
endif

if GPROF_ENABLED
    AM_CFLAGS+=-pg -fprofile-arcs
    AM_CXXFLAGS+=-pg -fprofile-arcs
    AM_LDFLAGS+=-pg -lgcov
endif

if WINDOWS
    AM_CPPFLAGS+=-DHAVE_STDINT -DLIBXML_STATIC -DPCRE_STATIC -DZIP_EXTERN= 
    AM_CPPFLAGS+=-DZLIB_INTERNAL -Dpoppler_cpp_EXPORTS
    
    AM_LDFLAGS+=-lws2_32 -lgdi32 -Wl,-Bstatic
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

req_CPPFLAGS = $(ZIP_CFLAGS) $(XML_CFLAGS) $(POPPLER_CFLAGS) $(PCRE_CFLAGS)
req_LDFLAGS = $(ZIP_LIBS) $(XML_LIBS) $(POPPLER_LIBS) $(PCRE_LIBS)

# specific subdirs
SUBDIRS = test

# data
dist_pkgdata_DATA = README.md COPYING CHANGES

