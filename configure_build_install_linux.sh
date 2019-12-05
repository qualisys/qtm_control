mkdir build_linux \
&& cd build_linux \
&& cmake .. -DCMAKE_BUILD_TYPE=Release \
&& cmake --build . --config Release \
&& sudo cmake --build . --target install
