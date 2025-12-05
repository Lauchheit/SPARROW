#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <vector>
#include <string>

void write_bitvector_to_file(const std::vector<bool>& bitvec, const std::string& filename = "code.bin");
void write_doublevector_to_file(const std::vector<double>& values, const std::string& filename = "decode.txt");

#endif // FILE_OPERATIONS_H