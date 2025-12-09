@echo off
g++ -std=c++17 -g ^
    compression_algorithms/gorilla/encode.cpp ^
    compression_algorithms/gorilla/decode.cpp ^
    compression_algorithms/sparrow/encode.cpp ^
    compression_algorithms/sparrow/decode.cpp ^
    compression_algorithms/sparrow/frequency_selection.cpp ^
    compression_algorithms/zlib/encode.cpp ^
    compression_algorithms/zlib/decode.cpp ^
    compression_algorithms/lz4/encode.cpp ^
    compression_algorithms/lz4/decode.cpp ^
    compression_algorithms/zstd/encode.cpp ^
    compression_algorithms/zstd/decode.cpp ^
    helpers/bit_operations.cpp ^
    helpers/file_operations.cpp ^
    compare.cpp ^
    -o compare.exe -lfftw3 -lz -llz4 -lzstd -lm

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
)