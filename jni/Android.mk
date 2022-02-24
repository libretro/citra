LOCAL_PATH := $(call my-dir)
EXTERNALS_DIR := $(LOCAL_PATH)/../externals
SRC_DIR := $(LOCAL_PATH)../src


# Build shaders
$(shell make -C .. shaders)


# Build the dynarmic module as a static library

include $(CLEAR_VARS)
LOCAL_MODULE := dynarmic
HAVE_DYNARMIC := 1
HAVE_LIBRETRO_VFS := 0
ARCH := aarch64

include ../Makefile.common

LOCAL_CPPFLAGS := $(DYNARMICINCFLAGS) \
        -I$(EXTERNALS_DIR)/dynarmic/include \
        -I$(EXTERNALS_DIR)/dynarmic/src/frontend/A32 \
        -I$(EXTERNALS_DIR)/boost

LOCAL_SRC_FILES := $(DYNARMICSOURCES_CXX)
include $(BUILD_STATIC_LIBRARY)


# Build the final shared library
include $(CLEAR_VARS)

GIT_REV := "$(shell git rev-parse HEAD || echo unknown)"
GIT_BRANCH := "$(shell git rev-parse --abbrev-ref HEAD || echo unknown)"
GIT_DESC := "$(shell git describe --always --long --dirty || echo unknown)"
BUILD_DATE := "$(shell date +'%d/%m/%Y %H:%M')"


CUSTOM_DEF :=
CUSTOM_DEF += -DGIT_REV=\"$(GIT_REV)\" \
           -DGIT_BRANCH=\"$(GIT_BRANCH)\" \
           -DGIT_DESC=\"$(GIT_DESC)\" \
           -DBUILD_NAME=\"citra-libretro\" \
           -DBUILD_DATE=\"$(BUILD_DATE)\" \
           -DBUILD_VERSION=\"$(GIT_BRANCH)-$(GIT_DESC)\" \
           -DBUILD_FULLNAME=\"\" \
           -DSHADER_CACHE_VERSION=\"0\"

ARCH := aarch64
HAVE_LIBRETRO_VFS := 1
HAVE_DYNARMIC = 0
HAVE_FFMPEG := 0
HAVE_FFMPEG_STATIC := 0
HAVE_GLAD := 1
HAVE_SSE := 1
HAVE_RGLGEN := 0
HAVE_RPC := 1
SRC_DIR := ../src
EXTERNALS_DIR := ../externals

LOCAL_MODULE := citra

DEFINES += -DARCHITECTURE_ARM64 \
           -DDYNARMIC_IGNORE_ASSERTS \
           -DHAVE_DYNARMIC \
           -DHAVE_LIBRETRO \
           -DUSING_GLES

fpic := -fPIC

include ../Makefile.common

SOURCES_CXX += $(SRC_DIR)/core/arm/dynarmic/arm_dynarmic.cpp \
               $(SRC_DIR)/core/arm/dynarmic/arm_dynarmic_cp15.cpp

INCFLAGS += -I$(SRC_DIR)/audio_core/hle
SOURCES_CXX += $(SRC_DIR)/audio_core/hle/mediandk_decoder.cpp

LOCAL_SRC_FILES := $(ANDROID_SOURCES) $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CPPFLAGS := -Wall -std=c++17 -D__LIBRETRO__ $(fpic) $(DEFINES) $(CUSTOM_DEF) $(ANDROID_INCLUDES) $(INCFLAGS) $(INCFLAGS_PLATFORM) $(ANDROID_INCLUDES)
LOCAL_CFLAGS := -D__LIBRETRO__ $(fpic) $(DEFINES) $(CUSTOM_DEF) $(INCFLAGS) $(INCFLAGS_PLATFORM) $(ANDROID_INCLUDES) $(CFLAGS)
LOCAL_STATIC_LIBRARIES += dynarmic cpufeatures
LOCAL_LDLIBS := -llog -lmediandk

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/cpufeatures)
