RACK_DIR ?= ../..

FLAGS += -Idep/include
CFLAGS +=
CXXFLAGS +=

LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res examples
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/arch.mk

# Entropia File System Watcher
efsw := dep/lib/libefsw-static-release.a
DEPS += $(efsw)
OBJECTS += $(efsw)
$(efsw):
	cd dep && $(WGET) "https://bitbucket.org/SpartanJ/efsw/get/e6afbec564e2.zip"
	cd dep && $(SHA256) e6afbec564e2.zip 8589dbedac7434f1863637af696354a9f1fcc28c6397c37b492a797ae62976be
	cd dep && $(UNZIP) e6afbec564e2.zip
	cd dep/SpartanJ-efsw-e6afbec564e2 && premake4 gmake
	cd dep/SpartanJ-efsw-e6afbec564e2 && $(MAKE) -C make/* config=release efsw-static-lib
	cd dep/SpartanJ-efsw-e6afbec564e2 && cp lib/libefsw-static-release.a $(DEP_PATH)/lib/
	cd dep/SpartanJ-efsw-e6afbec564e2 && cp -R include/efsw $(DEP_PATH)/include/

# QuickJS
quickjs := dep/lib/quickjs/libquickjs.a
DEPS += $(quickjs)
OBJECTS += $(quickjs)
QUICKJS_MAKE_FLAGS += prefix="$(DEP_PATH)"
ifdef ARCH_WIN
	QUICKJS_MAKE_FLAGS += CONFIG_WIN32=y
endif
$(quickjs):
	cd QuickJS && $(MAKE) $(QUICKJS_MAKE_FLAGS)
	cd QuickJS && $(MAKE) $(QUICKJS_MAKE_FLAGS) install

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
