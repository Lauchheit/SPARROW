
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

#include "helpers/bit_operations.h"
#include "helpers/file_operations.h"

#include "input_strategies/source_context.h"
#include "input_strategies/clean_signal_strategy.h"
#include "input_strategies/disturbed_signal_strategy.h"
#include "input_strategies/file_signal_strategy.h"

#include "compression_algorithms/sparrow/sparrow.h"
#include "compression_algorithms/gorilla/gorilla.h"

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
        default:
            cout << "Algorithm indicator " << algo_type << " does not exist." << endl;
    }

    // SignalContext inputContext(std::make_unique<FileSignalStrategy>(10000, 1, 1));
    // vector<double> inputSignal = inputContext.getSignal();
    // write_doublevector_to_file(inputSignal, "data/signal_data.txt");

    string input_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\signal_data.txt";
    string code_filepath = "C:\\Users\\cleme\\OneDrive\\uni\\Informatik\\Bachelorarbeit\\code\\SPARROW\\data\\code.txt";

    SignalContext context(std::make_unique<FileSignalStrategy>(input_filepath));

    vector<double> original = context.getSignal();


    vector<bool> code = algorithm->encode(context);
    write_bitvector_to_file(code, code_filepath);

    BinaryFileReader codeReader = {code_filepath};
    vector<double> reconstructed = algorithm->decode(codeReader);

    delete algorithm;

    size_t N = original.size();

    if(N!= reconstructed.size()){
        cout << "ERROR: reconstructed signal has unequal entries." << endl << "Original: " << N << endl << "Reconstruction: " << reconstructed.size();
    }

    int error = 0;
    for(int i=0; i<N; i++){
        error += abs(original[i] - reconstructed[i]);
    }
    cout << endl << "Absolute reconstruction error: " << error;

    return 0;


}