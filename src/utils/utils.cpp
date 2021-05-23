#include "utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "logger.h"

using std::ceil;
using std::cout;
using std::max;
using std::string;
using std::unordered_map;
using std::vector;

extern Config config;
extern Logger logger;

bool FuzzyCompare(int num1, int num2) {
  return abs(num1 - num2) <= config.gap_max_diff;
}

bool FuzzyCompare(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  auto max_len = max(len1, len2);

  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1, 0));

  // Find longest common substring.
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
  if (common_len >= max_len * config.strict_equal_rate) return true;

  // Find longest common subsequence.
  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }
  if (dp[len1][len2] >= max_len * config.fuzzy_equal_rate) return true;

  return false;
}

void ShowManual() {
  cout << "usage: solution [option] [args]\n"
       << "Options and arguments:\n"
       << "-a\t : run all preprocessing tasks and start the main process\n"
       << "-i\t : create an index of reference data only\n"
       << "-p\t : combine PacBio subsequences only\n"
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
            (*arg_flags)['p'] = true;
            (*arg_flags)['s'] = true;
            break;
          case 'i':
          case 'p':
          case 's':
            (*arg_flags)[arg] = true;
            break;
          default:
            logger.Warn("ReadArgs", "Invalid argument: " + string(1, arg));
            return false;
        }
      }
    }
    return true;
  } catch (...) {
    return false;
  }
}
