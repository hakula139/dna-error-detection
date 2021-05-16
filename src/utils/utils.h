#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <string>

bool FuzzyCompare(int num1, int num2);
bool FuzzyCompare(const std::string& str1, const std::string& str2);
bool QuickCompare(int num1, int num2);
bool QuickCompare(const std::string& str1, const std::string& str2);

#endif  // SRC_UTILS_UTILS_H_
