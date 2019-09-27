RACK_DIR ?= ../..

FLAGS +=
CFLAGS +=
CXXFLAGS += -g

LDFLAGS += -g

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
	cd dep && $(SHA256) duktape-2.4.0.tar.xz 86a89307d1633b5cedb2c6e56dc86e92679fc34b05be551722d8cc69ab0771fc
	cd dep && $(UNTAR) duktape-2.4.0.tar.xz

# QuickJS
quickjs := dep/QuickJS/libquickjs.a
DEPS += $(quickjs)
OBJECTS += $(quickjs)
FLAGS += -Idep/QuickJS
LDFLAGS += -Ldep/QuickJS -lquickjs

$(quickjs):
	cd dep && git clone "https://github.com/JerrySievert/QuickJS"
	cd dep/QuickJS && $(MAKE)

# # LuaJIT

# luajit := dep/lib/luajit.a
# DEPS += $(luajit)

# $(luajit):
# 	cd dep && $(WGET) "http://luajit.org/download/LuaJIT-2.0.5.tar.gz"
# 	cd dep && $(SHA256) LuaJIT-2.0.5.tar.gz 874b1f8297c697821f561f9b73b57ffd419ed8f4278c82e05b48806d30c1e979
# 	cd dep && $(UNTAR) LuaJIT-2.0.5.tar.gz
# 	cd dep/LuaJIT-2.0.5 && $(MAKE)


include $(RACK_DIR)/plugin.mk
