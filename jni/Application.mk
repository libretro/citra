NDK_TOOLCHAIN_VERSION ?= 4.8
APP_ABI := arm64-v8a
APP_STL := c++_static
APP_GNUSTL_CPP_FEATURES := exceptions
APP_PLATFORM := android-21

APP_CPPFLAGS := -fexceptions -frtti -std=c++20
