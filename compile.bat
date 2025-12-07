@echo off
C:\msys64\mingw64\bin\g++ -std=c++17 -g ^
    compression_algorithms/gorilla/encode.cpp ^
    compression_algorithms/gorilla/decode.cpp ^
    compression_algorithms/sparrow/encode.cpp ^
    compression_algorithms/sparrow/decode.cpp ^
    compression_algorithms/sparrow/frequency_selection.cpp ^
    helpers/bit_operations.cpp ^
    helpers/file_operations.cpp ^
    compare.cpp ^
    -o compare.exe -lfftw3 -lm

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
)