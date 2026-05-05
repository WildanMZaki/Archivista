if exist build\debug rmdir /s /q build\debug
cmake -S . -B build/debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j1
.\build\debug\Archivista.exe