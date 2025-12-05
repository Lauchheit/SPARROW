#ifndef BINARY_READER_H
#define BINARY_READER_H
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>


class BinaryFileReader {
private:
    std::string filepath;
public:
    BinaryFileReader(std::string filepath){
        this->filepath = filepath;
    }

    std::vector<bool> getSignalCode() const {
        std::ifstream infile(this->filepath, std::ios::binary | std::ios::ate);
    
        if (!infile.is_open()) {
            throw std::runtime_error("Error: Could not open file " + this->filepath);
        }
        
        std::streamsize file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> bytes(file_size);
        if (!infile.read(reinterpret_cast<char*>(bytes.data()), file_size)) {
            throw std::runtime_error("Error: Could not read file " + this->filepath);
        }
        
        infile.close();
        
        std::vector<bool> bits;
        bits.reserve(file_size * 8);
        
        for (uint8_t byte : bytes) {
            for (int i = 7; i >= 0; i--) {
                bits.push_back((byte >> i) & 1);
            }
        }
        
        //std::cout << "Successfully read " << file_size << " bytes (" << bits.size() << " bits) from " << filename << std::endl;
        
        return bits;
    }
};

#endif // BINARY_READER_H