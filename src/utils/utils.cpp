#include "utils.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "logger.h"

using std::ceil;
using std::cout;
using std::max;
using std::min;
using std::out_of_range;
using std::string;
using std::unordered_map;
using std::vector;

size_t LongestCommonSubstringLength(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));

  auto common_len = 0;
  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
        common_len = max(common_len, dp[i][j]);
      } else {
        dp[i][j] = 0;
      }
    }
  }
  return common_len;
}

size_t LongestCommonSubsequenceLength(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));

  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }
  return dp[len1][len2];
}

string ShortestCommonSupersequence(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  if (!len1) return str2;
  if (!len2) return str1;

  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));

  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }

  string result;
  try {
    for (int i = len1, j = len2; i > 0 || j > 0;) {
      if (i > 0 && dp[i][j] == dp[i - 1][j]) {
        result = str1[--i] + result;
      } else if (j > 0 && dp[i][j] == dp[i][j - 1]) {
        result = str2[--j] + result;
      } else {
        result = str1[--i] + result;
        --j;
      }
    }
  } catch (const out_of_range& error) {
    Logger::Fatal(
        "ShortestCommonSupersequence",
        "Unexpected branch, error: " + string(error.what()));
    throw;
  }
  return result;
}

bool FuzzyCompare(int num1, int num2) {
  return abs(num1 - num2) <= Config::GAP_MAX_DIFF;
}

bool FuzzyCompare(const string& str1, const string& str2) {
  auto max_len = max(str1.length(), str2.length());
  return LongestCommonSubstringLength(str1, str2) >=
             max_len * Config::STRICT_EQUAL_RATE ||
         LongestCommonSubsequenceLength(str1, str2) >=
             max_len * Config::FUZZY_EQUAL_RATE;
}

void ShowManual() {
  cout << "usage: solution [option] [args]\n"
       << "Options and arguments:\n"
       << "-a\t : run all preprocessing tasks and start the main process\n"
       << "-i\t : create an index of reference data only\n"
       << "-m\t : merge PacBio subsequences only\n"
       << "-s\t : start the main process only\n";
}

bool ReadArgs(unordered_map<char, bool>* arg_flags, int argc, char** argv) {
  try {
    if (argc <= 1) return false;
    for (auto i = 1; i < argc; ++i) {
      if (argv[i][0] != '-') continue;
      for (auto j = 1; argv[i][j] != '\0'; ++j) {
        auto arg = argv[i][j];
        switch (arg) {
          case 'a':
            (*arg_flags)['i'] = true;
            (*arg_flags)['m'] = true;
            (*arg_flags)['s'] = true;
            break;
          case 'i':
          case 'm':
          case 's':
            (*arg_flags)[arg] = true;
            break;
          default:
            Logger::Warn("ReadArgs", "Invalid argument: " + string(1, arg));
            return false;
        }
      }
    }
    return true;
  } catch (...) {
    return false;
  }
}
