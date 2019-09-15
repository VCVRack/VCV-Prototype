RACK_DIR ?= ../..

FLAGS +=
CFLAGS +=
CXXFLAGS +=

LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res examples
DISTRIBUTABLES += $(wildcard LICENSE*)


# Duktape

duktape := dep/duktape-2.4.0/src/duktape.c
DEPS += $(duktape)
SOURCES += $(duktape)
FLAGS += -Idep/duktape-2.4.0/src

$(duktape):
	cd dep && $(WGET) "https://duktape.org/duktape-2.4.0.tar.xz"
	cd dep && $(UNTAR) duktape-2.4.0.tar.xz


include $(RACK_DIR)/plugin.mk
