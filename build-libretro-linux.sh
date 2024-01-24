#!/bin/bash

set -e

cd $(dirname $0)
mkdir -p build-linux && cd build-linux
cmake -DENABLE_TESTS=OFF -DENABLE_DEDICATED_ROOM=OFF \
    -DENABLE_SDL2=OFF -DENABLE_QT=OFF -DENABLE_WEB_SERVICE=OFF -DENABLE_SCRIPTING=OFF \
    -DENABLE_OPENAL=OFF -DENABLE_LIBUSB=OFF -DCITRA_ENABLE_BUNDLE_TARGET=OFF \
    ..
make -j$(nproc)

SO=citra_libretro.so
echo "Core file is here => $(readlink -f $SO)"
strip $SO
