$(shell mkdir -p build >/dev/null)
$(shell rm -f build/main.o)
PROJECT_DIR = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
STRIP = $(COMPILER_PREFIX)strip
STRIPFLAGS = --strip-all
CXXFLAGS = -std=c++14 -pedantic -Wall -pipe $(DEFINES) \
           -I$(PROJECT_DIR)/beast/include -I$(PROJECT_DIR)/zwo/asi/include \
           -I$(PROJECT_DIR)/zwo/efw/include -I$(PROJECT_DIR)/json/src
OPTFLAGS = -O2
DBGFLAGS = -O0 -g -DDEBUG
DEPFLAGS = -MT $@ -MMD -MP -MF build/$*.Td
LDFLAGS =
CXX = $(COMPILER_PREFIX)g++

SRCFILES := main.cpp
LIBS = $(PROJECT_DIR)/zwo/efw/lib/x64/libEFWFilter.a \
       $(PROJECT_DIR)/zwo/asi/lib/x64/libASICamera2.a -ludev -lusb-1.0 -lpthread \
       -lutil -lboost_system -lboost_filesystem -lboost_regex
TARGET = rasicap
APP_BUILD_TIME := $(shell date -u +'%Y-%m-%d %H:%M:%S UTC')
APP_BUILD_UUID := $(shell uuid)
APP_GIT_COMMIT := $(shell git rev-parse --short HEAD)
APP_VERSION := "0.1"

SOURCES := $(patsubst %.cpp,src/%.cpp,$(SRCFILES))
OBJS := $(patsubst %.cpp,build/%.o,$(SRCFILES))
DEFINES = -D_BSD_SOURCE -DAPP_NAME="\"$(TARGET)\"" \
          -DAPP_BUILD_UUID="\"$(APP_BUILD_UUID)\"" \
          -DAPP_BUILD_TIME="\"$(APP_BUILD_TIME)\"" \
          -DAPP_VERSION="\"$(APP_VERSION)\"" \
          -DAPP_GIT_COMMIT="\"$(APP_GIT_COMMIT)\""

SUFFIXES += .d

all: release

debug: CXXFLAGS := $(DBGFLAGS) $(CXXFLAGS)
debug: $(TARGET)

release: CXXFLAGS := $(OPTFLAGS) $(CXXFLAGS)
release: $(TARGET)
	$(STRIP) $(STRIPFLAGS) $(TARGET)

prueba: CXXFLAGS := $(DBGFLAGS) $(CXXFLAGS)
prueba: $(PROJECT_DIR)/zwo/asi/lib/x64/libASICamera2.a src/prueba.cpp
	$(CXX) $(CXXFLAGS) -o prueba $(LDFLAGS) src/prueba.cpp $(PROJECT_DIR)/zwo/asi/lib/x64/libASICamera2.a -lcfitsio -lCCfits -lusb-1.0 -lpthread

build/%.o: src/%.cpp
build/%.o: src/%.cpp build/%.d
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	@mv -f build/$*.Td build/$*.d && touch $@

$(TARGET): $(PROJECT_DIR)/zwo/efw/lib/x64/libEFWFilter.a $(PROJECT_DIR)/zwo/asi/lib/x64/libASICamera2.a $(PROJECT_DIR)/beast/include/boost/beast.hpp $(PROJECT_DIR)/json/src/json.hpp $(OBJS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

$(PROJECT_DIR)/beast/include/boost/beast.hpp:
	$(PROJECT_DIR)/do-submodule.sh beast

$(PROJECT_DIR)/json/src/json.hpp:
	$(PROJECT_DIR)/do-submodule.sh json

build/%.d: ;
.PRECIOUS: build/%.d

clean:
	rm -f build/* $(TARGET) prueba *~ src/*~ core

-include $(OBJS:.o=.d)
