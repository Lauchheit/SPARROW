#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <numeric>
#include <cstring>
#include <bitset>
#include <iomanip>
#include <fstream>
#include <map>
#include <chrono>

#include "helpers/bit_operations.h"
#include "helpers/file_operations.h"

#include "input_strategies/source_context.h"
#include "input_strategies/clean_signal_strategy.h"
#include "input_strategies/disturbed_signal_strategy.h"
#include "input_strategies/file_signal_strategy.h"

#include "compression_algorithms/sparrow/sparrow.h"
#include "compression_algorithms/gorilla/gorilla.h"
#include "compression_algorithms/zlib/zlib.h"
#include "compression_algorithms/lz4/lz4_compression.h"
#include "compression_algorithms/zstd/zstd_compression.h"

using namespace std;

//g++ -fdiagnostics-color=always -g compression_algorithms/sparrow/encode.cpp compression_algorithms/sparrow/decode.cpp compression_algorithms/sparrow/frequency_selection.cpp helpers/bit_operations.cpp helpers/file_operations.cpp compare.cpp -o compare.exe -lfftw3 -lm

int main(int argc, char* argv[]){
    if(argc < 2) {
        cerr << "Usage: compare.exe <algo_type>" << endl;
        return 1;
    }
    
    int algo_type = atoi(argv[1]);
    CompressionAlgorithm* algorithm = nullptr;
    switch (algo_type) {
        case 1: 
            algorithm = new SparrowCompression();
            break;
        case 2: 
            algorithm = new GorillaCompression();
            break;
        case 3:
            algorithm = new ZlibCompression();
            break;
        case 4: 
            algorithm = new LZ4Compression();
            break;
        case 5: 
            algorithm = new ZstdCompression();
            break;
        default:
            cerr << "Algorithm indicator " << algo_type << " does not exist." << endl;
            return 1;
    }

    string input_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\signal_data.txt";
    string code_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\code.txt";
    string decode_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\decode.txt";

    SignalContext context(std::make_unique<FileSignalStrategy>(input_filepath));
    vector<double> original = context.getSignal();

    // Time encoding
    auto encode_start = chrono::high_resolution_clock::now();
    vector<bool> code = algorithm->encode(input_filepath);
    auto encode_end = chrono::high_resolution_clock::now();
    auto encode_duration = chrono::duration_cast<chrono::milliseconds>(encode_end - encode_start);
    
    write_bitvector_to_file(code, code_filepath);

    BinaryFileReader codeReader = {code_filepath};
    
    // Time decoding
    auto decode_start = chrono::high_resolution_clock::now();
    vector<double> reconstructed = algorithm->decode(codeReader);
    auto decode_end = chrono::high_resolution_clock::now();
    auto decode_duration = chrono::duration_cast<chrono::milliseconds>(decode_end - decode_start);
    
    write_doublevector_to_file(reconstructed, decode_filepath);

    delete algorithm;

    size_t N = original.size();

    if(N != reconstructed.size()){
        cerr << "ERROR: reconstructed signal has unequal entries." << endl 
             << "Original: " << N << endl 
             << "Reconstruction: " << reconstructed.size() << endl;
        return 1;
    }

    int error = 0;
    for(int i=0; i<N; i++){
        error += abs(original[i] - reconstructed[i]);
    }
    
    // Print timing and error to stderr in a parseable format
    cerr << "ENCODE_TIME_MS:" << encode_duration.count() << endl;
    cerr << "DECODE_TIME_MS:" << decode_duration.count() << endl;
    cerr << "RECONSTRUCTION_ERROR:" << error << endl;

    return error ? 1 : 0;
}