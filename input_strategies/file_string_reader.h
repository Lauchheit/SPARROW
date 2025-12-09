// file_string_reader.h
#ifndef FILE_STRING_READER_H
#define FILE_STRING_READER_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

class FileStringReader {
private:
    std::string filename;
    bool trimWhitespace;
    bool skipEmptyLines;
    
    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

public:
    FileStringReader(const std::string& fname, 
                     bool trim = true, 
                     bool skipEmpty = true)
        : filename(fname), 
          trimWhitespace(trim), 
          skipEmptyLines(skipEmpty) {}
    
    std::vector<std::string> read() const {
        std::ifstream file(filename);
        
        if (!file.good()) {
            throw std::runtime_error("File does not exist: " + filename);
        }
        
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        
        std::vector<std::string> data;
        std::string line;
        int lineNumber = 0;
        int skippedLines = 0;
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            std::string processedLine = trimWhitespace ? trim(line) : line;
            
            if (skipEmptyLines && processedLine.empty()) {
                skippedLines++;
                continue;
            }
            
            data.push_back(processedLine);
        }
        
        file.close();
        
        if (data.empty()) {
            throw std::runtime_error("No data read from file: " + filename + 
                                    " (read " + std::to_string(lineNumber) + 
                                    " lines, all empty or skipped)");
        }
        
        std::cout << "Successfully read " << data.size() 
                  << " strings from " << filename;
        if (skippedLines > 0) {
            std::cout << " (skipped " << skippedLines << " empty lines)";
        }
        std::cout << std::endl;
        
        return data;
    }
    
    std::string getFilename() const { return filename; }
    bool isTrimming() const { return trimWhitespace; }
    bool isSkippingEmpty() const { return skipEmptyLines; }
};

#endif