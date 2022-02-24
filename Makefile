HAVE_DYNARMIC = 0
HAVE_FFMPEG = 0
HAVE_FFMPEG_STATIC = 0
HAVE_GLAD = 1
HAVE_SSE = 0
HAVE_RGLGEN = 0
HAVE_RPC = 1
FFMPEG_DISABLE_VDPAU ?= 0
HAVE_FFMPEG_CROSSCOMPILE ?= 0
FFMPEG_XC_CPU ?=
FFMPEG_XC_ARCH ?=
FFMPEG_XC_PREFIX ?=
FFMPEG_XC_SYSROOT ?=
FFMPEG_XC_NM ?=
FFMPEG_XC_AR ?=
FFMPEG_XC_AS ?=
FFMPEG_XC_CC ?=
FFMPEG_XC_LD ?=

TARGET_NAME    := citra
EXTERNALS_DIR  += ./externals
SRC_DIR        += ./src
LIBS		   = -lm
DEFINES        := -DHAVE_LIBRETRO

STATIC_LINKING := 0
AR             := ar

SPACE :=
SPACE := $(SPACE) $(SPACE)
BACKSLASH :=
BACKSLASH := \$(BACKSLASH)
filter_out1 = $(filter-out $(firstword $1),$1)
filter_out2 = $(call filter_out1,$(call filter_out1,$1))

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

ifeq (,$(ARCH))
	ARCH = $(shell uname -m)
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

GIT_REV := "$(shell git rev-parse HEAD || echo unknown)"
GIT_BRANCH := "$(shell git rev-parse --abbrev-ref HEAD || echo unknown)"
GIT_DESC := "$(shell git describe --always --long --dirty || echo unknown)"
BUILD_DATE := "$(shell date +'%d/%m/%Y_%H:%M')"

DEFINES += -DGIT_REV=\"$(GIT_REV)\" \
		   -DGIT_BRANCH=\"$(GIT_BRANCH)\" \
		   -DGIT_DESC=\"$(GIT_DESC)\" \
		   -DBUILD_NAME=\"citra-libretro\" \
		   -DBUILD_DATE=\"$(BUILD_DATE)\" \
		   -DBUILD_VERSION=\"$(GIT_BRANCH)-$(GIT_DESC)\" \
		   -DBUILD_FULLNAME=\"\" \
		   -DSHADER_CACHE_VERSION=\"0\"

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(SRC_DIR)/citra_libretro/link.T -Wl,--no-undefined
   LIBS +=-lpthread -lGL -ldl
   HAVE_FFMPEG = 1
   HAVE_FFMPEG_STATIC = 1
ifeq ($(HAVE_FFMPEG_STATIC), 1)
   LIBS += $(EXTERNALS_DIR)/ffmpeg/libavcodec/libavcodec.a $(EXTERNALS_DIR)/ffmpeg/libavutil/libavutil.a
else
   LIBS += -lavcodec -lavutil
endif

#######################################
# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
   include $(DEVKITPRO)/libnx/switch_rules
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   DEFINES += -DSWITCH=1 -D__SWITCH__=1 -DHAVE_LIBNX=1 \
   -D__LINUX_ERRNO_EXTENSIONS__ -DBOOST_ASIO_DISABLE_SIGACTION -DOS_RNG_AVAILABLE

   fpic := -fPIE
   CFLAGS = $(DEFINES) -I$(LIBNX)/include/ -I$(PORTLIBS)/include/ -specs=$(LIBNX)/switch.specs
   CFLAGS += -march=armv8-a -mtune=cortex-a57 -mtp=soft -mcpu=cortex-a57+crc+fp+simd -ffast-math
   CXXFLAGS = $(ASFLAGS) $(CFLAGS)
   ARCH = aarch64
   STATIC_LINKING = 1
   HAVE_GLAD = 0
   HAVE_RGLGEN = 1
   HAVE_RPC = 0
   DEBUG = 0
else ifneq (,$(findstring windows_msvc2019,$(platform)))
	LIBS =

	PlatformSuffix = $(subst windows_msvc2019_,,$(platform))
	ifneq (,$(findstring desktop,$(PlatformSuffix)))
		WinPartition = desktop
		MSVC2019CompileFlags = -D_UNICODE -DUNICODE -DWINVER=0x0600 -D_WIN32_WINNT=0x0600
		LDFLAGS += -MANIFEST -NXCOMPAT -DYNAMICBASE -DEBUG -OPT:REF -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MANIFESTUAC:"level='asInvoker' uiAccess='false'" -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1
	else ifneq (,$(findstring uwp,$(PlatformSuffix)))
		WinPartition = uwp
		MSVC2019CompileFlags = -DWINDLL -D_UNICODE -DUNICODE -DWRL_NO_DEFAULT_LIB
		LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
	endif

	ifeq ($(DEBUG), 1)
		MSVC2019CompileFlags += -DEBUG

	else
		MSVC2019CompileFlags += -O2 -GS"-" -MD
	endif

	MSVC2019CompileFlags += -D_WIN32=1 -DNOMINMAX -DBOOST_ALL_NO_LIB

	CFLAGS += $(MSVC2019CompileFlags) -nologo
	CXXFLAGS += $(MSVC2019CompileFlags) -nologo -EHsc -Zc:throwingNew,inline

	TargetArchMoniker = $(subst $(WinPartition)_,,$(PlatformSuffix))

	CC  = cl.exe
	CXX = cl.exe

	SPACE :=
	SPACE := $(SPACE) $(SPACE)
	BACKSLASH :=
	BACKSLASH := \$(BACKSLASH)
	filter_out1 = $(filter-out $(firstword $1),$1)
	filter_out2 = $(call filter_out1,$(call filter_out1,$1))

	reg_query = $(call filter_out2,$(subst $2,,$(shell reg query "$2" -v "$1" 2>/dev/null)))
	fix_path = $(subst $(SPACE),\ ,$(subst \,/,$1))

	b1 := (
	b2 := )
	ProgramFiles86w := $(ProgramFiles$(b1)x86$(b2))
	ProgramFiles86 := $(shell cygpath "$(ProgramFiles86w)")

	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir := $(WindowsSdkDir)

	WindowsSDKVersion ?= $(firstword $(foreach folder,$(subst $(subst \,/,$(WindowsSdkDir)Include/),,$(wildcard $(call fix_path,$(WindowsSdkDir)Include\*))),$(if $(wildcard $(call fix_path,$(WindowsSdkDir)Include/$(folder)/um/Windows.h)),$(folder),)))$(BACKSLASH)
	WindowsSDKVersion := $(WindowsSDKVersion)

	VsInstallBuildTools = $(ProgramFiles86)/Microsoft Visual Studio/2019/BuildTools
	VsInstallEnterprise = $(ProgramFiles86)/Microsoft Visual Studio/2019/Enterprise
	VsInstallProfessional = $(ProgramFiles86)/Microsoft Visual Studio/2019/Professional
	VsInstallCommunity = $(ProgramFiles86)/Microsoft Visual Studio/2019/Community

	VsInstallRoot ?= $(shell if [ -d "$(VsInstallBuildTools)" ]; then echo "$(VsInstallBuildTools)"; fi)
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallEnterprise)" ]; then echo "$(VsInstallEnterprise)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallProfessional)" ]; then echo "$(VsInstallProfessional)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallCommunity)" ]; then echo "$(VsInstallCommunity)"; fi)
	endif
	VsInstallRoot := $(VsInstallRoot)

	VcCompilerToolsVer := $(shell cat "$(VsInstallRoot)/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" | grep -o '[0-9\.]*')
	VcCompilerToolsDir := $(VsInstallRoot)/VC/Tools/MSVC/$(VcCompilerToolsVer)
	VcCompilerLibDir := $(VcCompilerToolsDir)/lib/$(TargetArchMoniker)

	WindowsSDKSharedIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\shared")
	WindowsSDKUCRTIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\ucrt")
	WindowsSDKUMIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\um")
	WindowsSDKUCRTLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\ucrt\$(TargetArchMoniker)")
	WindowsSDKUMLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\um\$(TargetArchMoniker)")

	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerLibDir)")
	INCLUDE := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/include")

# For some reason the HostX86 compiler doesn't like compiling for x64
# ("no such file" opening a shared library), and vice-versa.
# Work around it for now by using the strictly x86 compiler for x86, and x64 for x64.
# NOTE: What about ARM?
	ifneq (,$(findstring x64,$(TargetArchMoniker)))
		override TARGET_ARCH = x86_64
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)/bin/HostX64/$(TargetArchMoniker)
      	LIB := $(LIB);$(CORE_DIR)/dx9sdk/Lib/x64
	else
		override TARGET_ARCH = x86
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)/bin/HostX86/$(TargetArchMoniker)
      	LIB := $(LIB);$(CORE_DIR)/dx9sdk/Lib/x86
	endif

	PATH := $(shell IFS=$$'\n'; cygpath "$(VCCompilerToolsBinDir)"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VsInstallRoot)/Common7/IDE")

	export INCLUDE := $(INCLUDE);$(WindowsSDKSharedIncludeDir);$(WindowsSDKUCRTIncludeDir);$(WindowsSDKUMIncludeDir)
	export LIB := $(LIB);$(WindowsSDKUCRTLibDir);$(WindowsSDKUMLibDir);$(FFMPEGDIR)/Windows/$(TARGET_ARCH)/lib
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	PLATFORM_EXT = win32
	LDFLAGS += ws2_32.lib user32.lib shell32.lib winmm.lib gdi32.lib opengl32.lib imm32.lib ole32.lib oleaut32.lib version.lib uuid.lib mfuuid.lib
	HAVE_MF = 1
	# RPC crashes, TODO: Figure out why
	HAVE_RPC = 0
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   DEFINES += -D_WIN32_WINNT=0x0600 -DWINVER=0x0600
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(SRC_DIR)/citra_libretro/link.T -Wl,--no-undefined
   LDFLAGS += -static -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lws2_32

   ifeq ($(MSYSTEM),MINGW64)
   	  CC ?= x86_64-w64-mingw32-gcc
          CXX ?= x86_64-w64-mingw32-g++
	  LDFLAGS += -lopengl32 -lmfuuid
	  ASFLAGS += -DWIN64
	  HAVE_MF = 1
   endif
endif

ifneq (,$(findstring msvc,$(platform)))
CFLAGS += -D_CRT_SECURE_NO_WARNINGS
CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS
endif

# x86_64 is expected to support both SSE and Dynarmic
ifeq ($(ARCH), x86_64)
DEFINES += -DARCHITECTURE_x86_64
HAVE_DYNARMIC = 1
HAVE_SSE = 1
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
else
# Add Unix optimization flags
	ifeq (,$(findstring msvc,$(platform)))
   		CXXFLAGS += -O3 -ffast-math -ftree-vectorize -DNDEBUG
	endif
endif

# Set ffmpeg configure options
ifeq ($(HAVE_FFMPEG_STATIC), 1)
FFMPEG_CONF_OPTS =--disable-encoders --disable-decoders --enable-decoder=aac --enable-decoder=aac_fixed --enable-decoder=aac_latm --disable-programs
ifeq ($(FFMPEG_DISABLE_VDPAU), 1)
FFMPEG_CONF_OPTS += --disable-vdpau
endif
ifeq ($(HAVE_FFMPEG_CROSSCOMPILE), 1)
FFMPEG_CONF_OPTS+= --enable-cross-compile --target-os="linux"
ifeq ($(FFMPEG_XC_CPU),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_CPU provided)
else
FFMPEG_CONF_OPTS += --cpu="$(FFMPEG_XC_CPU)"
endif
ifeq ($(FFMPEG_XC_ARCH),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_ARCH provided)
else
FFMPEG_CONF_OPTS += --arch="$(FFMPEG_XC_ARCH)"
endif
ifeq ($(FFMPEG_XC_PREFIX),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_PREFIX provided)
else
FFMPEG_CONF_OPTS += --cross-prefix="$(FFMPEG_XC_PREFIX)"
endif
ifeq ($(FFMPEG_XC_SYSROOT),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_SYSROOT provided)
else
FFMPEG_CONF_OPTS += --sysroot="$(FFMPEG_XC_SYSROOT)" --sysinclude="$(FFMPEG_XC_SYSROOT)/usr/include"
endif
ifeq ($(FFMPEG_XC_NM),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_NM provided)
else
FFMPEG_CONF_OPTS += --nm="$(FFMPEG_XC_NM)"
endif
ifeq ($(FFMPEG_XC_AR),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_AR provided)
else
FFMPEG_CONF_OPTS += --ar="$(FFMPEG_XC_AR)"
endif
ifeq ($(FFMPEG_XC_AS),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_AS provided)
else
FFMPEG_CONF_OPTS += --as="$(FFMPEG_XC_AS)"
endif
ifeq ($(FFMPEG_XC_CC),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_CC provided)
else
FFMPEG_CONF_OPTS += --cc="$(FFMPEG_XC_CC)"
endif
ifeq ($(FFMPEG_XC_LD),)
$(error HAVE_FFMPEG_CROSSCOMPILE set, but no FFMPEG_XC_LD provided)
else
FFMPEG_CONF_OPTS += --ld="$(FFMPEG_XC_LD)"
endif
endif
endif

include Makefile.common

CPPFILES = $(filter %.cpp,$(SOURCES_CXX))
CCFILES = $(filter %.cc,$(SOURCES_CXX))

OBJECTS := $(SOURCES_C:.c=.o) $(CPPFILES:.cpp=.o) $(CCFILES:.cc=.o)

ifeq (,$(findstring msvc,$(platform)))
	CXXFLAGS += -std=c++17
else
	CXXFLAGS += -std:c++latest
endif


CFLAGS   	  += -D__LIBRETRO__ $(fpic) $(DEFINES) $(INCFLAGS) $(INCFLAGS_PLATFORM)
DYNARMICFLAGS += -D__LIBRETRO__ $(fpic) $(DEFINES) $(DYNARMICINCFLAGS) $(INCFLAGS_PLATFORM) $(CXXFLAGS)
CXXFLAGS 	  += -D__LIBRETRO__ $(fpic) $(DEFINES) $(INCFLAGS) $(INCFLAGS_PLATFORM)

OBJOUT   = -o
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
ifeq ($(STATIC_LINKING),1)
	LD ?= lib.exe

	ifeq ($(DEBUG), 1)
		CFLAGS += -MTd
		CXXFLAGS += -MTd
	else
		CFLAGS += -MT
		CXXFLAGS += -MT
	endif
else
	LD = link.exe

	ifeq ($(DEBUG), 1)
		CFLAGS += -MDd
		CXXFLAGS += -MDd
	else
		CFLAGS += -MD
		CXXFLAGS += -MD
	endif
endif
else
	LD = $(CXX)
endif

all: shaders $(TARGET)

ffmpeg_configure:
ifeq ($(HAVE_FFMPEG_STATIC), 1)
	cd $(EXTERNALS_DIR)/ffmpeg && ./configure $(FFMPEG_CONF_OPTS)
endif
ffmpeg_static: ffmpeg_configure
ifeq ($(HAVE_FFMPEG_STATIC), 1)
	cd $(EXTERNALS_DIR)/ffmpeg && $(MAKE) -j$(NUMPROC)
endif

$(TARGET): ffmpeg_static $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(LD) $(fpic) $(SHARED) $(INCLUDES) $(LINKOUT)$@ $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c $(OBJOUT)$@ $<

$(foreach p,$(OBJECTS),$(if $(findstring $(EXTERNALS_DIR)/dynarmic/src,$p),$p,)):
	$(CXX) $(DYNARMICFLAGS) $(fpic) -c $(OBJOUT)$@ $(@:.o=.cpp)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(fpic) -c $(OBJOUT)$@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(fpic) -c $(OBJOUT)$@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
ifeq ($(HAVE_FFMPEG_STATIC), 1)
	cd $(EXTERNALS_DIR)/ffmpeg && $(MAKE) clean
endif

shaders: $(SHADER_FILES)
	mkdir -p $(SRC_DIR)/video_core/shaders
	for SHADER_FILE in $^; do \
		FILENAME=$$(basename "$$SHADER_FILE"); \
		SHADER_NAME=$$(echo "$$FILENAME" | sed -e "s/\./_/g"); \
		rm -f $(SRC_DIR)/video_core/shaders/$$FILENAME; \
		echo "#pragma once" >> $(SRC_DIR)/video_core/shaders/$$FILENAME; \
		echo "constexpr std::string_view $$SHADER_NAME = R\"(" >> $(SRC_DIR)/video_core/shaders/$$FILENAME; \
		cat $$SHADER_FILE >> $(SRC_DIR)/video_core/shaders/$$FILENAME; \
		echo ")\";" >> $(SRC_DIR)/video_core/shaders/$$FILENAME; \
	done


.PHONY: clean ffmpeg_static
