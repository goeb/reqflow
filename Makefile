
ifeq ($(WIN),1)
	EXE = req.exe
	CXX = i586-mingw32msvc-g++
	BUILD_DIR = build_win
	CFLAGS += -DHAVE_STDINT
	LIBXML = $(HOME)/Downloads/libxml2-2.9.1
	LIBZIP = $(HOME)/Downloads/libzip-0.11.2
	LIBPCRE = $(HOME)/Downloads/pcre-8.34
	LDFLAGS += -lws2_32 -lgdi32
	LDFLAGS += -Wl,-Bstatic
  	LDFLAGS += -L$(LIBXML)/.libs -lxml2
	LDFLAGS += $(LIBZIP)/lib/.libs/libzip.dll.a $(LIBZIP)/lib/.libs/libzip.a
   	LDFLAGS +=$(LIBPCRE)/.libs/libpcreposix.a $(LIBPCRE)/.libs/libpcreposix.dll.a
	CFLAGS += -I$(LIBXML)/include -I$(LIBZIP)/lib -I$(LIBPCRE)
	PACK_NAME = smit-win32
else
	EXE = req
	CXX = g++
	BUILD_DIR = build_linux86
	PACK_NAME = smit-linux86
	LDFLAGS += -lzip -lpoppler-cpp
	LDFLAGS += -L/usr/lib/i386-linux-gnu -lxml2 -lpcreposix
	CFLAGS += -I/usr/include/libxml2
endif

SRCS_CPP += \
			src/main.cpp \
			src/logging.cpp \
			src/dateTools.cpp \
			src/parseConfig.cpp \
			src/req.cpp \
			src/ReqDocumentTxt.cpp \
			src/ReqDocumentDocx.cpp \
			src/renderingHtml.cpp \


ifneq ($(WIN),1)
	SRCS_CPP += src/ReqDocumentPdf.cpp 
endif

OBJS = $(SRCS_CPP:%.cpp=$(BUILD_DIR)/%.o)
DEPENDS = $(SRCS_CPP:%.cpp=$(BUILD_DIR)/%.d)


CFLAGS += -g -Wall


ifeq ($(GCOV),1)
	CFLAGS += -fprofile-arcs -ftest-coverage
	LDFLAGS += -lgcov
endif

ifeq ($(GPROF),1)
	CFLAGS += -pg -fprofile-arcs
	LDFLAGS += -pg -lgcov
endif

all: $(EXE)

print:
	@echo SRCS=$(SRCS)
	@echo OBJS=$(OBJS)
	@echo WIN=$(WIN)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p `dirname $@`
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.d: %.cpp
	mkdir -p `dirname $@`
	@set -e; rm -f $@; \
	obj=`echo $@ | sed -e "s/\.d$$/.o/"`; \
	$(CXX) -MM $(CFLAGS) $< > $@.$$$$; \
	sed "s,.*:,$$obj $@ : ,g" < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	find . -name "*.o" -delete
	find . -name "*.d" -delete
	rm $(EXE)

include $(DEPENDS)
