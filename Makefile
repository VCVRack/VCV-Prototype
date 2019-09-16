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
	$(WGET) "https://duktape.org/duktape-2.4.0.tar.xz"
	$(SHA256) duktape-2.4.0.tar.xz 86a89307d1633b5cedb2c6e56dc86e92679fc34b05be551722d8cc69ab0771fc
	cd dep && $(UNTAR) ../duktape-2.4.0.tar.xz

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


include $(RACK_DIR)/plugin.mk
