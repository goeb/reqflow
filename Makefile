
ifeq ($(WIN),1)
	EXE = req.exe
	CXX = i586-mingw32msvc-g++
	BUILD_DIR = build_win
	CFLAGS += -DHAVE_STDINT
	LDFLAGS += -lws2_32 -lgdi32
	PACK_NAME = smit-win32
else
	EXE = req
	CXX = g++
	BUILD_DIR = build_linux86
	PACK_NAME = smit-linux86
endif

SRCS_CPP = 	src/main.cpp \
			src/logging.cpp \
			src/dateTools.cpp \
			src/parseConfig.cpp \
			src/req.cpp \
			src/importerTxt.cpp \
			src/importerDocx.cpp \

OBJS = $(SRCS_CPP:%.cpp=$(BUILD_DIR)/%.o)
DEPENDS = $(SRCS_CPP:%.cpp=$(BUILD_DIR)/%.d)

LDFLAGS += -lzip
LDFLAGS += -L/usr/lib/i386-linux-gnu -lxml2

CFLAGS += -g -Wall
CFLAGS += -I/usr/include/libxml2


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
