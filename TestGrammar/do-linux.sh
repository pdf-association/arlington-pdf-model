#!/bin/sh
cmake -B cmake-linux/debug -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Debug .
cmake --build cmake-linux/debug --config Debug
cmake -B cmake-linux/release -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Release .
cmake --build cmake-linux/release --config Release
ls -lsa bin/linux
rm -rf cmake-linux

