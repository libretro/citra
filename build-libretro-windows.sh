#!/bin/bash

set -e

cd $(dirname $0)
mkdir -p build-windows && cd build-windows
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/x86_64-w64-mingw32.cmake \
    -DCITRA_WARNINGS_AS_ERRORS=OFF -DDISABLE_CLANG_TARGET=ON -DENABLE_LTO=OFF \
    -DENABLE_TESTS=OFF -DENABLE_DEDICATED_ROOM=OFF \
    -DENABLE_SDL2=OFF -DENABLE_QT=OFF -DENABLE_WEB_SERVICE=OFF -DENABLE_SCRIPTING=OFF \
    -DENABLE_OPENAL=OFF -DENABLE_LIBUSB=OFF -DCITRA_ENABLE_BUNDLE_TARGET=OFF \
    -DENABLE_CUBEB=OFF \
    ..
make -j$(nproc)

SO=bin/Release/citra_libretro.dll
echo "Core file is here => $(readlink -f $SO)"
x86_64-w64-mingw32-strip $SO
