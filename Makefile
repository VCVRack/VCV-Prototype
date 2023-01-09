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
LUAJIT ?= 1
PYTHON ?= 0
SUPERCOLLIDER ?= 1
VULT ?= 1
LIBPD ?= 1
FAUST ?= 1

# Vult depends on both LuaJIT and QuickJS
ifeq ($(VULT), 1)
QUICKJS := 1
LUAJIT := 1
endif


# Entropia File System Watcher
ifdef ARCH_WIN
	efsw := dep/lib/efsw-static-release.lib
else
	efsw := dep/lib/libefsw-static-release.a
endif
DEPS += $(efsw)
OBJECTS += $(efsw)
$(efsw):
ifdef ARCH_WIN
	cd efsw && premake5 gmake
	cd efsw && $(MAKE) -C make/* config=release_x86_64 efsw-static-lib
else
	cd efsw && premake4 gmake
	cd efsw && $(MAKE) -C make/* config=release efsw-static-lib
endif
	mkdir -p dep/lib dep/include
ifdef ARCH_WIN
	cd efsw && cp lib/efsw-static-release.lib $(DEP_PATH)/lib/
else
	cd efsw && cp lib/libefsw-static-release.a $(DEP_PATH)/lib/
endif
	cd efsw && cp -R include/efsw $(DEP_PATH)/include/


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
	cd dep && git clone "https://github.com/JerrySievert/QuickJS.git"
	cd dep/QuickJS && git checkout 807adc8ca9010502853d471bd8331cdc1d376b94
	cd dep/QuickJS && $(MAKE) $(QUICKJS_MAKE_FLAGS) install
endif


# LuaJIT
ifeq ($(LUAJIT), 1)
SOURCES += src/LuaJITEngine.cpp
luajit := dep/lib/libluajit-5.1.a
OBJECTS += $(luajit)
DEPS += $(luajit)
$(luajit):
	$(WGET) "http://luajit.org/download/LuaJIT-2.0.5.tar.gz"
	$(SHA256) LuaJIT-2.0.5.tar.gz 874b1f8297c697821f561f9b73b57ffd419ed8f4278c82e05b48806d30c1e979
	cd dep && $(UNTAR) ../LuaJIT-2.0.5.tar.gz
	cd dep/LuaJIT-2.0.5 && $(MAKE) BUILDMODE=static PREFIX="$(DEP_PATH)" install
endif


# SuperCollider
ifeq ($(SUPERCOLLIDER), 1)
SOURCES += src/SuperColliderEngine.cpp
FLAGS += -Idep/supercollider/include -Idep/supercollider/include/common -Idep/supercollider/lang -Idep/supercollider/common -Idep/supercollider/include/plugin_interface
# FLAGS += -DSC_VCV_ENGINE_TIMING # uncomment to turn on timing printing while running
supercollider := dep/supercollider/build/lang/libsclang.a
OBJECTS += $(supercollider)
DEPS += $(supercollider)
DISTRIBUTABLES += dep/supercollider/SCClassLibrary
DISTRIBUTABLES += support/supercollider_extensions
SUPERCOLLIDER_CMAKE_FLAGS += -DSUPERNOVA=0 -DSC_EL=0 -DSC_VIM=0 -DSC_ED=0 -DSC_IDE=0 -DSC_ABLETON_LINK=0 -DSC_QT=0 -DCMAKE_BUILD_TYPE=Release -DSCLANG_SERVER=0 -DBUILD_TESTING=0 -DNO_LIBSNDFILE=1
SUPERCOLLIDER_SUBMODULES += external_libraries/hidapi external_libraries/nova-simd external_libraries/nova-tt external_libraries/portaudio/portaudio_submodule external_libraries/yaml-cpp
SUPERCOLLIDER_BRANCH := 3.12

OBJECTS += dep/supercollider/build/external_libraries/libtlsf.a
OBJECTS += dep/supercollider/build/external_libraries/hidapi/linux/libhidapi.a
OBJECTS += dep/supercollider/build/external_libraries/hidapi/hidapi_parser/libhidapi_parser.a
OBJECTS += dep/supercollider/build/external_libraries/libboost_thread_lib.a
OBJECTS += dep/supercollider/build/external_libraries/libboost_system_lib.a
OBJECTS += dep/supercollider/build/external_libraries/libboost_regex_lib.a
OBJECTS += dep/supercollider/build/external_libraries/libboost_filesystem_lib.a
OBJECTS += dep/supercollider/build/external_libraries/libyaml.a

LDFLAGS += -lpthread -lasound -ludev

$(supercollider):
	cd dep && git clone "https://github.com/supercollider/supercollider" --branch $(SUPERCOLLIDER_BRANCH) --depth 1
	cd dep/supercollider && git checkout 3a6eabc84ed63ac4ae1f3e8aa27d942d46262d54
	cd dep/supercollider && git submodule update --depth 1 --init -- $(SUPERCOLLIDER_SUBMODULES)
	cd dep/supercollider && mkdir build
	cd dep/supercollider/build && $(CMAKE) .. $(SUPERCOLLIDER_CMAKE_FLAGS)
	cd dep/supercollider/build && $(MAKE) libsclang
endif


# Python
ifeq ($(PYTHON), 1)
SOURCES += src/PythonEngine.cpp
# Note this is a dynamic library, not static.
python := dep/lib/libpython3.8.so.1.0
DEPS += $(python) $(numpy)
FLAGS += -Idep/include/python3.8
# TODO Test these flags on all platforms
# Make dynamic linker look in the plugin folder for libpython.
LDFLAGS += -Wl,-rpath,'$$ORIGIN'/dep/lib
LDFLAGS += -Ldep/lib -lpython3.8
LDFLAGS += -lcrypt -lpthread -ldl -lutil -lm
DISTRIBUTABLES += $(python)
DISTRIBUTABLES += dep/lib/python3.8
$(python):
	$(WGET) "https://www.python.org/ftp/python/3.8.0/Python-3.8.0.tar.xz"
	$(SHA256) Python-3.8.0.tar.xz b356244e13fb5491da890b35b13b2118c3122977c2cd825e3eb6e7d462030d84
	cd dep && $(UNTAR) ../Python-3.8.0.tar.xz
	cd dep/Python-3.8.0 && $(CONFIGURE) --build=$(MACHINE) --enable-shared --enable-optimizations
	cd dep/Python-3.8.0 && $(MAKE) build_all
	cd dep/Python-3.8.0 && $(MAKE) install

numpy := dep/lib/python3.8/site-packages/numpy
FLAGS += -Idep/lib/python3.8/site-packages/numpy/core/include
$(numpy): $(python)
	$(WGET) "https://github.com/numpy/numpy/releases/download/v1.17.3/numpy-1.17.3.tar.gz"
	$(SHA256) numpy-1.17.3.tar.gz c93733dbebc2599d2747ceac4b18825a73767d289176ed8e02090325656d69aa
	cd dep && $(UNTAR) ../numpy-1.17.3.tar.gz
	# Don't try to find an external BLAS and LAPACK library.
	# Don't install to an egg folder.
	# Make sure to use our built Python.
	cd dep/numpy-1.17.3 && LD_LIBRARY_PATH=../lib NPY_BLAS_ORDER= NPY_LAPACK_ORDER= "$(DEP_PATH)"/bin/python3.8 setup.py build -j4 install --single-version-externally-managed --root=/

# scipy: $(numpy)
# 	$(WGET) "https://github.com/scipy/scipy/releases/download/v1.3.1/scipy-1.3.1.tar.xz"
# 	$(SHA256) scipy-1.3.1.tar.xz 326ffdad79f113659ed0bca80f5d0ed5e28b2e967b438bb1f647d0738073a92e
# 	cd dep && $(UNTAR) ../scipy-1.3.1.tar.xz
# 	cd dep/scipy-1.3.1 && "$(DEP_PATH)"/bin/python3.7 setup.py build -j4 install
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


# Vult
ifeq ($(VULT), 1)
SOURCES += src/VultEngine.cpp
vult := dep/vult/vultc.h
$(vult):
	cd dep && mkdir -p vult
	cd dep/vult && $(WGET) "https://github.com/modlfo/vult/releases/download/v0.4.12/vultc.h"
	$(SHA256) $(vult) 3e80f6d30defe7df2804568f0314dbb33e6bf69d53d18a804c8b8132cf5ad146
FLAGS += -Idep/vult
DEPS += $(vult)
endif


# LibPD
ifeq ($(LIBPD), 1)
libpd := dep/lib/libpd.a
SOURCES += src/LibPDEngine.cpp
OBJECTS += $(libpd)
DEPS += $(libpd)
FLAGS += -Idep/include/libpd -DHAVE_LIBDL

ifdef ARCH_WIN
	# PD_INTERNAL leaves the function declarations for libpd unchanged
	# not specifying that flag would enable the  "EXTERN __declspec(dllexport) extern" macro
	# which throws a linker error. I guess this macro should only be used for the windows
	# specific .dll dynamic linking format.
	# The corresponding #define resides in "m_pd.h" inside th Pure Data sources
	FLAGS += -DPD_INTERNAL -Ofast
	LDFLAGS += -Wl,--export-all-symbols
	LDFLAGS += -lws2_32
endif

$(libpd):
	cd dep && git clone "https://github.com/libpd/libpd.git" --recursive
	cd dep/libpd && git checkout 5772a612527f06597d44d195843307ad0e3578fe
	
ifdef ARCH_MAC
	# libpd's Makefile is handmade, and it doesn't honor CFLAGS and LDFLAGS environments.
	# So in order for Mac 10.15 (for example) to make a build that works on Mac 10.7+, we have to manually add DEP_MAC_SDK_FLAGS to CFLAGS and LDFLAGS.
	# We can't just add the environment's CFLAGS/LDFLAGS because `-march=nocona` makes libpd segfault when initialized.
	# Perhaps inline assembly is used in libpd? Who knows.
	cd dep/libpd && $(MAKE) MULTI=true STATIC=true ADDITIONAL_CFLAGS='-DPD_LONGINTTYPE="long long" $(DEP_MAC_SDK_FLAGS) -stdlib=libc++' ADDITIONAL_LDFLAGS='$(DEP_MAC_SDK_FLAGS) -stdlib=libc++'
else
	cd dep/libpd && $(MAKE) MULTI=true STATIC=true ADDITIONAL_CFLAGS='-DPD_LONGINTTYPE="long long"'
endif
	cd dep/libpd && $(MAKE) install prefix="$(DEP_PATH)"
endif


# Faust
ifeq ($(FAUST), 1)
libfaust := dep/lib/libfaust.a
SOURCES += src/FaustEngine.cpp
OBJECTS += $(libfaust)
DEPS += $(libfaust)
FLAGS += -DINTERP
DISTRIBUTABLES += faust_libraries

# Test using LLVM
#LDFLAGS += -L/usr/local/lib -lfaust

# Test using MIR
#LDFLAGS += -L/usr/local/lib -lfaust dep/lib/mir-gen.o dep/lib/mir.o

$(libfaust):
	cd dep && git clone "https://github.com/grame-cncm/faust.git" --recursive
	cd dep/faust && git checkout 1dfc452a8250f3123b5100edf8c882e1cea407a1
	cd dep/faust/build && make cmake BACKENDS=interp.cmake TARGETS=interp.cmake
	cd dep/faust/build && make install PREFIX="$(DEP_PATH)"
	cp -r dep/faust/libraries/* faust_libraries/
	rm -rf faust_libraries/doc
	rm -rf faust_libraries/docs

endif

include $(RACK_DIR)/plugin.mk
