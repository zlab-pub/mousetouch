#！/bin/bash
cd $(dirname $0)

rm -rf build
mkdir -p build

cd build

#cmake -G "Visual Studio 16 2019" -A x64 ..
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . 
