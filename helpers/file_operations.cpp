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
using namespace std;

void write_bitvector_to_file(const std::vector<bool>& bitvec, const std::string& filename = "code.bin") {
    std::ofstream outfile(filename, std::ios::binary);
    
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    // Konvertiere zu Bytes
    std::vector<uint8_t> bytes;
    uint8_t current_byte = 0;
    int bit_count = 0;
    
    for(bool bit : bitvec) {
        current_byte <<= 1;
        if(bit) {
            current_byte |= 1;
        }
        bit_count++;
        
        if(bit_count == 8) {
            bytes.push_back(current_byte);
            current_byte = 0;
            bit_count = 0;
        }
    }
    
    // Füge letztes Byte hinzu (mit Padding)
    if(bit_count > 0) {
        current_byte <<= (8 - bit_count);  // Pad mit Nullen
        bytes.push_back(current_byte);
    }
    
    outfile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    outfile.close();
    
    std::cout << "Successfully saved " << bitvec.size() << " bits (" 
              << bytes.size() << " bytes) in " << filename << "!" << std::endl;
}

void write_doublevector_to_file(const std::vector<double>& values, const std::string& filename = "decode.txt") {
    std::ofstream outfile(filename);
    
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    for(const auto& value : values) {
        outfile << value << "\n";
    }
    
    outfile.close();
    
    std::cout << "Successfully saved " << values.size() 
              << " values to " << filename << "!" << std::endl;
}
