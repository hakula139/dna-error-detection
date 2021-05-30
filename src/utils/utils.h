#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "range.h"

std::pair<Range, Range> LongestCommonSubstring(
    const std::string& str1, const std::string& str2);

size_t LongestCommonSubstringLength(
    const std::string& str1, const std::string& str2);

size_t LongestCommonSubsequenceLength(
    const std::string& str1, const std::string& str2);

void Concat(std::string* base_p, const std::string* str_p);

bool FuzzyCompare(int num1, int num2);
bool FuzzyCompare(const std::string& str1, const std::string& str2);

void ShowManual();
bool ReadArgs(std::unordered_map<char, bool>* arg_flags, int argc, char** argv);

#endif  // SRC_UTILS_UTILS_H_
