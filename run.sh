#!/bin/bash
rm -rf build
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DLOTTIE_QT=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j$(nproc)


