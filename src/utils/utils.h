#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.h"
#include "point.h"
#include "range.h"

std::string Head(
    const std::string& value,
    size_t start = 0,
    size_t size = Config::DISPLAY_SIZE);

std::pair<Range, Range> LongestCommonSubstring(
    const std::string& str1, const std::string& str2);

size_t LongestCommonSubstringLength(
    const std::string& str1, const std::string& str2);

std::vector<std::vector<std::pair<int, Direction>>> LongestCommonSubsequence(
    const std::string& str1, const std::string& str2);

size_t LongestCommonSubsequenceLength(
    const std::string& str1, const std::string& str2);

void Concat(std::string* base_p, const std::string* str_p);

bool FuzzyCompare(int num1, int num2, size_t threshold = Config::GAP_MAX_DIFF);
bool FuzzyCompare(const std::string& str1, const std::string& str2);

void ShowManual();
bool ReadArgs(std::unordered_map<char, bool>* arg_flags, int argc, char** argv);

#endif  // SRC_UTILS_UTILS_H_
