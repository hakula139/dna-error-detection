#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <string>
#include <unordered_map>

bool FuzzyCompare(int num1, int num2);
bool FuzzyCompare(const std::string& str1, const std::string& str2);

void ShowManual();
bool ReadArgs(std::unordered_map<char, bool>* arg_flags, int argc, char** argv);

#endif  // SRC_UTILS_UTILS_H_
