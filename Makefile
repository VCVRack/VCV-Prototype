RACK_DIR ?= ../..

FLAGS += -Idep/include
CFLAGS +=
CXXFLAGS +=

LDFLAGS +=

SOURCES += src/Prototype.cpp

DISTRIBUTABLES += res examples
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/arch.mk

DUKTAPE ?= 0
QUICKJS ?= 1
PYTHON ?= 1
LUA ?= 1

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
	mkdir -p dep/lib dep/include
	cd dep/SpartanJ-efsw-e6afbec564e2 && cp lib/libefsw-static-release.a $(DEP_PATH)/lib/
	cd dep/SpartanJ-efsw-e6afbec564e2 && cp -R include/efsw $(DEP_PATH)/include/

# Duktape
ifeq ($(DUKTAPE), 1)
SOURCES += src/DuktapeEngine.cpp
duktape := dep/duktape-2.4.0/src/duktape.c
DEPS += $(duktape)
SOURCES += $(duktape)
FLAGS += -Idep/duktape-2.4.0/src
$(duktape):
	$(WGET) "https://duktape.org/duktape-2.4.0.tar.xz"
	$(SHA256) duktape-2.4.0.tar.xz 86a89307d1633b5cedb2c6e56dc86e92679fc34b05be551722d8cc69ab0771fc
	cd dep && $(UNTAR) ../duktape-2.4.0.tar.xz
endif

# QuickJS
ifeq ($(QUICKJS), 1)
SOURCES += src/QuickJSEngine.cpp
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
endif

# Python
ifeq ($(PYTHON), 1)
SOURCES += src/PythonEngine.cpp
# Note this is a dynamic library, not static.
python := dep/lib/libpython3.7m.so.1.0
DEPS += $(python) $(numpy)
FLAGS += -Idep/include/python3.7m
# TODO Test these flags on all platforms
# Make dynamic linker look in the plugin folder for libpython.
LDFLAGS += -Wl,-rpath,'$$ORIGIN'/dep/lib
LDFLAGS += -Ldep/lib -lpython3.7m
LDFLAGS += -lcrypt -lpthread -ldl -lutil -lm
DISTRIBUTABLES += $(python)
DISTRIBUTABLES += dep/lib/python3.7
$(python):
	$(WGET) "https://www.python.org/ftp/python/3.7.4/Python-3.7.4.tar.xz"
	$(SHA256) Python-3.7.4.tar.xz fb799134b868199930b75f26678f18932214042639cd52b16da7fd134cd9b13f
	cd dep && $(UNTAR) ../Python-3.7.4.tar.xz
	cd dep/Python-3.7.4 && $(CONFIGURE) --build=$(MACHINE) --enable-shared --enable-optimizations
	cd dep/Python-3.7.4 && $(MAKE) build_all
	cd dep/Python-3.7.4 && $(MAKE) install

numpy := dep/lib/python3.7/site-packages/numpy-1.17.2-py3.7-linux-x86_64.egg
FLAGS += -Idep/lib/python3.7/site-packages/numpy-1.17.2-py3.7-linux-x86_64.egg/numpy/core/include
$(numpy): $(python)
	$(WGET) "https://github.com/numpy/numpy/releases/download/v1.17.2/numpy-1.17.2.tar.gz"
	$(SHA256) numpy-1.17.2.tar.gz 81a4f748dcfa80a7071ad8f3d9f8edb9f8bc1f0a9bdd19bfd44fd42c02bd286c
	cd dep && $(UNTAR) ../numpy-1.17.2.tar.gz
	# Don't try to find an external BLAS and LAPACK library.
	cd dep/numpy-1.17.2 && LD_LIBRARY_PATH=../lib NPY_BLAS_ORDER= NPY_LAPACK_ORDER= "$(DEP_PATH)"/bin/python3.7 setup.py build -j4 install

# scipy: $(numpy)
# 	$(WGET) "https://github.com/scipy/scipy/releases/download/v1.3.1/scipy-1.3.1.tar.xz"
# 	$(SHA256) scipy-1.3.1.tar.xz 326ffdad79f113659ed0bca80f5d0ed5e28b2e967b438bb1f647d0738073a92e
# 	cd dep && $(UNTAR) ../scipy-1.3.1.tar.xz
# 	cd dep/scipy-1.3.1 && "$(DEP_PATH)"/bin/python3.7 setup.py build -j4 install
endif

# LuaJIT
ifeq ($(LUA), 1)
SOURCES += src/LuaJITEngine.cpp
luajit := dep/lib/libluajit-5.1.a
OBJECTS += $(luajit)
DEPS += $(luajit)
$(luajit):
	$(WGET) "http://luajit.org/download/LuaJIT-2.0.5.tar.gz"
	$(SHA256) LuaJIT-2.0.5.tar.gz 874b1f8297c697821f561f9b73b57ffd419ed8f4278c82e05b48806d30c1e979
	cd dep && $(UNTAR) ../LuaJIT-2.0.5.tar.gz
	cd dep/LuaJIT-2.0.5 && $(MAKE)
	cd dep/LuaJIT-2.0.5 && $(MAKE) PREFIX="$(DEP_PATH)" install
endif

# # Julia
# julia := dep/lib/libjulia.a
# DEPS += $(julia)
# $(julia):
# 	$(WGET) "https://github.com/JuliaLang/julia/releases/download/v1.2.0/julia-1.2.0-full.tar.gz"
# 	$(SHA256) julia-1.2.0-full.tar.gz 2419b268fc5c3666dd9aeb554815fe7cf9e0e7265bc9b94a43957c31a68d9184
# 	cd dep && $(UNTAR) ../julia-1.2.0-full.tar.gz

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
