LOCAL_PATH := $(call my-dir)
EXTERNALS_DIR := $(LOCAL_PATH)/../externals
SRC_DIR := $(LOCAL_PATH)/../src


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

# Build the faad2 module as a static library
include $(CLEAR_VARS)
LOCAL_MODULE := faad2
ARCH := aarch64
include ../Makefile.common
LOCAL_CFLAGS := $(FAAD2FLAGS)
LOCAL_SRC_FILES := $(FAAD2SOURCES_C)
include $(BUILD_STATIC_LIBRARY)

# Build the libressl module as a static library
include $(CLEAR_VARS)
LOCAL_MODULE := ssl
ARCH := aarch64
include ../Makefile.common
LOCAL_CFLAGS := $(LIBRESSLFLAGS)
LOCAL_SRC_FILES := $(LIBRESSLSOURCES_C)
include $(BUILD_STATIC_LIBRARY)


# Build the final shared library
include $(CLEAR_VARS)

ARCH := aarch64
#HAVE_LIBRETRO_VFS := 1
HAVE_DYNARMIC = 0
HAVE_GLAD := 1
HAVE_SSE := 1
HAVE_RGLGEN := 0
HAVE_RPC := 1
SRC_DIR := ../src
EXTERNALS_DIR := ../externals

LOCAL_MODULE := retro

DEFINES += -DARCHITECTURE_ARM64 \
           -DDYNARMIC_IGNORE_ASSERTS \
           -DHAVE_DYNARMIC \
           -DHAVE_LIBRETRO \
           -DUSING_GLES \
           -DCRYPTOPP_ARM_CRC32_AVAILABLE=0 -DCRYPTOPP_ARM_PMULL_AVAILABLE=0 -DCRYPTOPP_ARM_AES_AVAILABLE=0 -DCRYPTOPP_ARM_SHA_AVAILABLE=0 \
           -DHAVE_GETPROGNAME -DHAVE_ARC4RANDOM_BUF -DHAVE_ASPRINTF \
           -DHAVE_STDINT_H

fpic := -fPIC

include ../Makefile.common

SOURCES_C += $(EXTERNALS_DIR)/android-ifaddrs/ifaddrs.c
SOURCES_CXX += $(SRC_DIR)/core/arm/dynarmic/arm_dynarmic.cpp \
               $(SRC_DIR)/core/arm/dynarmic/arm_dynarmic_cp15.cpp

CFLAGS += $(LIBRESSLFLAGS)
INCFLAGS += -I$(SRC_DIR)/audio_core/hle -I$(EXTERNALS_DIR)/android-ifaddrs

LOCAL_SRC_FILES := $(ANDROID_SOURCES) $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CPPFLAGS := -Wall -Wno-unused-local-typedef -std=c++20 -D__LIBRETRO__ \$(fpic) $(DEFINES) $(CUSTOM_DEF) $(ANDROID_INCLUDES) $(INCFLAGS) $(INCFLAGS_PLATFORM) $(ANDROID_INCLUDES)
LOCAL_CFLAGS := -D__LIBRETRO__ $(fpic) $(DEFINES) $(CUSTOM_DEF) $(INCFLAGS) $(INCFLAGS_PLATFORM) $(ANDROID_INCLUDES) $(CFLAGS)
LOCAL_STATIC_LIBRARIES += dynarmic faad ssl cpufeatures
LOCAL_LDLIBS := -llog -lmediandk

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/cpufeatures)
