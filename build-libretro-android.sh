#!/bin/bash

set -e

cd $(dirname $0)
[ -z "$NDK" ] && echo "You must specify ndk path in env like \"NDK=/path/to/android-ndk-r26b $0\"" && exit 1
ABI=arm64-v8a
MINSDKVERSION=21
mkdir -p build-android && cd build-android
cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$MINSDKVERSION \
    -DENABLE_TESTS=OFF -DENABLE_DEDICATED_ROOM=OFF \
    -DENABLE_SDL2=OFF -DENABLE_QT=OFF -DENABLE_WEB_SERVICE=OFF -DENABLE_SCRIPTING=OFF \
    -DENABLE_OPENAL=OFF -DENABLE_LIBUSB=OFF -DCITRA_ENABLE_BUNDLE_TARGET=OFF \
    ..
make -j$(nproc)

SO=citra_libretro.so
echo "Core file is here => $(readlink -f $SO)"
$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip $SO
