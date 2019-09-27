RACK_DIR ?= ../..

FLAGS += -Idep/include
CFLAGS +=
CXXFLAGS +=

LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res examples
DISTRIBUTABLES += $(wildcard LICENSE*)


# fswatch
fswatch := dep/lib/libfswatch.a
DEPS += $(fswatch)
OBJECTS += $(fswatch)
$(fswatch):
	$(WGET) "https://github.com/emcrisostomo/fswatch/releases/download/1.14.0/fswatch-1.14.0.tar.gz"
	$(SHA256) fswatch-1.14.0.tar.gz 44d5707adc0e46d901ba95a5dc35c5cc282bd6f331fcf9dbf9fad4af0ed5b29d
	cd dep && $(UNTAR) ../fswatch-1.14.0.tar.gz
	cd dep/fswatch-1.14.0 && $(CONFIGURE) --enable-shared=no
	cd dep/fswatch-1.14.0 && $(MAKE)
	cd dep/fswatch-1.14.0 && $(MAKE) install

# Duktape
duktape := dep/duktape-2.4.0/src/duktape.c
DEPS += $(duktape)
SOURCES += $(duktape)
FLAGS += -Idep/duktape-2.4.0/src
$(duktape):
	$(WGET) "https://duktape.org/duktape-2.4.0.tar.xz"
	$(SHA256) duktape-2.4.0.tar.xz 86a89307d1633b5cedb2c6e56dc86e92679fc34b05be551722d8cc69ab0771fc
	cd dep && $(UNTAR) ../duktape-2.4.0.tar.xz

# QuickJS
quickjs := dep/QuickJS/libquickjs.a
DEPS += $(quickjs)
OBJECTS += $(quickjs)
FLAGS += -Idep/QuickJS
LDFLAGS += -Ldep/QuickJS -lquickjs
MAKEQJSFLAGS :=

$(quickjs):
	cd dep && git clone "https://github.com/JerrySievert/QuickJS"
	cd dep/QuickJS && $(MAKE) $(MAKEQJSFLAGS)

# # LuaJIT
# luajit := dep/lib/luajit.a
# DEPS += $(luajit)
# $(luajit):
# 	cd dep && $(WGET) "http://luajit.org/download/LuaJIT-2.0.5.tar.gz"
# 	cd dep && $(SHA256) LuaJIT-2.0.5.tar.gz 874b1f8297c697821f561f9b73b57ffd419ed8f4278c82e05b48806d30c1e979
# 	cd dep && $(UNTAR) LuaJIT-2.0.5.tar.gz
# 	cd dep/LuaJIT-2.0.5 && $(MAKE)

# # Julia
# julia := dep/lib/libjulia.a
# DEPS += $(julia)
# $(julia):
# 	$(WGET) "https://github.com/JuliaLang/julia/releases/download/v1.2.0/julia-1.2.0-full.tar.gz"
# 	$(SHA256) julia-1.2.0-full.tar.gz 2419b268fc5c3666dd9aeb554815fe7cf9e0e7265bc9b94a43957c31a68d9184
# 	cd dep && $(UNTAR) ../julia-1.2.0-full.tar.gz

# # Python
# python := dep/lib/libpython3.7m.a
# DEPS += $(python)
# OBJECTS += $(python)
# FLAGS += -Idep/include/python3.7m
# LDFLAGS += -lcrypt -lpthread -ldl -lutil -lm
# $(python):
# 	$(WGET) "https://www.python.org/ftp/python/3.7.4/Python-3.7.4.tar.xz"
# 	$(SHA256) Python-3.7.4.tar.xz fb799134b868199930b75f26678f18932214042639cd52b16da7fd134cd9b13f
# 	cd dep && $(UNTAR) ../Python-3.7.4.tar.xz
# 	cd dep/Python-3.7.4 && $(CONFIGURE) --build=$(MACHINE) --enable-optimizations
# 	cd dep/Python-3.7.4 && $(MAKE) build_all
# 	cd dep/Python-3.7.4 && $(MAKE) install

# # Csound
# csound := dep/lib/libcsound.a
# DEPS += $(csound)
# $(csound):
# 	$(WGET) "https://github.com/csound/csound/archive/6.13.0.tar.gz"
# 	$(SHA256) 6.13.0.tar.gz 183beeb3b720bfeab6cc8af12fbec0bf9fef2727684ac79289fd12d0dfee728b
# 	cd dep && $(UNTAR) ../6.13.0.tar.gz

# # LLVM
# llvm := dep/lib/libllvm.a
# DEPS += $(llvm)
# $(llvm):
# 	$(WGET) "https://github.com/llvm/llvm-project/releases/download/llvmorg-8.0.1/llvm-8.0.1.src.tar.xz"
# 	$(SHA256) llvm-8.0.1.src.tar.xz 44787a6d02f7140f145e2250d56c9f849334e11f9ae379827510ed72f12b75e7
# 	cd dep && $(UNTAR) ../llvm-8.0.1.src.tar.xz
# 	cd dep/llvm-8.0.1.src && mkdir -p build
# 	cd dep/llvm-8.0.1.src/build && $(CMAKE) ..
# 	cd dep/llvm-8.0.1.src/build && $(MAKE)
# 	cd dep/llvm-8.0.1.src/build && $(MAKE) install


include $(RACK_DIR)/plugin.mk

ifdef ARCH_WIN
	MAKEQJSFLAGS += CONFIG_WIN32=y
endif
