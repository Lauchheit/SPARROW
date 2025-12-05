#ifndef FILE_SIGNAL_STRATEGY_H
#define FILE_SIGNAL_STRATEGY_H

#include "signal_source_strategy.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

class FileSignalStrategy : public SignalSourceStrategy {
private:
    std::string filename;
    
    double parseLine(const std::string& line) const {
        std::string cleaned;
        for (char c : line) {
            if (c != ' ') {
                cleaned += c;
            }
        }
        
        for (char& c : cleaned) {
            if (c == ',') {
                c = '.';
            }
        }
        
        if (cleaned.empty()) {
            throw std::invalid_argument("Empty line");
        }
        
        return std::stod(cleaned);
    }

public:
    FileSignalStrategy(const std::string& fname) : filename(fname) {}
    
    std::vector<double> getSignal() const override {

        std::ifstream file(filename);

        if (!file.good()){
            throw std::runtime_error("File does not exist: " + filename);
        }

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        std::vector<double> signal;
        
        std::string line;
        int lineNumber = 0;
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            try {
                double value = parseLine(line);
                signal.push_back(value);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse line " << lineNumber 
                         << ": " << line << std::endl;
            }
        }
        
        file.close();
        
        if (signal.empty()) {
            throw std::runtime_error("No data read from file: " + filename);
        }
        
        std::cout << "Successfully read " << signal.size() 
                  << " data points from " << filename << std::endl;
        
        return signal;
    }
    
    std::string getDescription() const override {
        return "File-based signal from: " + filename;
    }
};

#endif 