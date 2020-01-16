@echo off
mkdir build_windows
cd build_windows
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../build_windows_output
cmake --build . --config Release
cmake --build . --target install --config Release
cd ..