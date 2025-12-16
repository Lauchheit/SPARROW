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
#include "input_strategies/file_signal_strategy.h"

#include "compression_algorithms/sparrow/sparrow.h"
#include "compression_algorithms/gorilla/gorilla.h"
#include "compression_algorithms/zlib/zlib.h"
#include "compression_algorithms/lz4/lz4_compression.h"
#include "compression_algorithms/zstd/zstd_compression.h"
#include "compression_algorithms/sparrow_elf/sparrowelf_compression.h"
#include "compression_algorithms/gorilla_elf/gorillaelf_compression.h"

using namespace std;

//g++ -fdiagnostics-color=always -g compression_algorithms/sparrow/encode.cpp compression_algorithms/sparrow/decode.cpp compression_algorithms/sparrow/frequency_selection.cpp helpers/bit_operations.cpp helpers/file_operations.cpp compare.cpp -o compare.exe -lfftw3 -lm

#include <fstream>

int main(int argc, char* argv[]){
    if(argc < 2) {
        cerr << "Usage: compare.exe <algo_type>" << endl;
        return 1;
    }
    
    int algo_type = atoi(argv[1]);
    CompressionAlgorithm* algorithm = nullptr;
    string algo_name;
    
    switch (algo_type) {
        case 1: 
            algorithm = new SparrowCompression();
            algo_name = "Sparrow";
            break;
        case 2: 
            algorithm = new GorillaCompression();
            algo_name = "Gorilla";
            break;
        case 3:
            algorithm = new ZlibCompression();
            algo_name = "Zlib";
            break;
        case 4: 
            algorithm = new LZ4Compression();
            algo_name = "LZ4";
            break;
        case 5: 
            algorithm = new ZstdCompression();
            algo_name = "Zstandard";
            break;
        case 6:
            algorithm = new SparrowElfCompression();
            algo_name = "Sparrow Elf";
            break;
        case 7:
            algorithm = new GorillaElfCompression();
            algo_name = "Gorilla Elf";
            break;
        default:
            cerr << "Algorithm indicator " << algo_type << " does not exist." << endl;
            return 1;
    }

    // Use relative paths - works from any directory
    string input_filepath = "data/signal_data.txt";
    string code_filepath = "data/code.bin";
    string decode_filepath = "data/decode.txt";
    string timing_filepath = "data/timing.json";

    long long encode_time_ms = 0;
    long long decode_time_ms = 0;
    double error = 0;
    bool success = false;

    try {
        SignalContext context(std::make_unique<FileSignalStrategy>(input_filepath));
        vector<double> original = context.getSignal();

        // Time encoding
        auto encode_start = chrono::high_resolution_clock::now();
        vector<bool> code = algorithm->encode(input_filepath);
        auto encode_end = chrono::high_resolution_clock::now();
        encode_time_ms = chrono::duration_cast<chrono::milliseconds>(encode_end - encode_start).count();
        
        write_bitvector_to_file(code, code_filepath);

        BinaryFileReader codeReader = {code_filepath};
        
        // Time decoding
        auto decode_start = chrono::high_resolution_clock::now();
        vector<double> reconstructed = algorithm->decode(codeReader);
        auto decode_end = chrono::high_resolution_clock::now();
        decode_time_ms = chrono::duration_cast<chrono::milliseconds>(decode_end - decode_start).count();
        
        write_doublevector_to_file(reconstructed, decode_filepath);

        size_t N = original.size();

        if(N != reconstructed.size()){
            cerr << "ERROR: reconstructed signal has unequal entries." << endl 
                 << "Original: " << N << endl 
                 << "Reconstruction: " << reconstructed.size() << endl;
            delete algorithm;
            
            // Still write timing even on error
            ofstream timing_file(timing_filepath);
            timing_file << "{\n";
            timing_file << "  \"algorithm\": \"" << algo_name << "\",\n";
            timing_file << "  \"encode_time_ms\": " << encode_time_ms << ",\n";
            timing_file << "  \"decode_time_ms\": " << decode_time_ms << ",\n";
            timing_file << "  \"error\": -1,\n";
            timing_file << "  \"success\": false\n";
            timing_file << "}\n";
            timing_file.close();
            
            return 1;
        }

        for(int i=0; i<N; i++){
            error += abs(original[i] - reconstructed[i]);
        }
        
        success = (error == 0);
        
    } catch (const std::exception& e) {
        cerr << "EXCEPTION: " << e.what() << endl;
        delete algorithm;
        
        // Write timing even on exception
        ofstream timing_file(timing_filepath);
        timing_file << "{\n";
        timing_file << "  \"algorithm\": \"" << algo_name << "\",\n";
        timing_file << "  \"encode_time_ms\": " << encode_time_ms << ",\n";
        timing_file << "  \"decode_time_ms\": " << decode_time_ms << ",\n";
        timing_file << "  \"error\": -1,\n";
        timing_file << "  \"exception\": \"" << e.what() << "\",\n";
        timing_file << "  \"success\": false\n";
        timing_file << "}\n";
        timing_file.close();
        
        return 1;
    }

    delete algorithm;
    
    // Write timing to JSON file
    ofstream timing_file(timing_filepath);
    timing_file << "{\n";
    timing_file << "  \"algorithm\": \"" << algo_name << "\",\n";
    timing_file << "  \"encode_time_ms\": " << encode_time_ms << ",\n";
    timing_file << "  \"decode_time_ms\": " << decode_time_ms << ",\n";
    timing_file << "  \"error\": " << error << ",\n";
    timing_file << "  \"success\": " << (success ? "true" : "false") << "\n";
    timing_file << "}\n";
    timing_file.close();

    cout << "==== ERROR: " << error << " ====";

    return error ? 1 : 0;
}