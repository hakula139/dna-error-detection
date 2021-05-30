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
#include "range.h"

using std::ceil;
using std::cout;
using std::max;
using std::min;
using std::out_of_range;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

std::pair<Range, Range> LongestCommonSubstring(
    const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));

  auto substr_len = 0;
  Range str1_substr, str2_substr;
  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
        if (dp[i][j] > substr_len) {
          substr_len = dp[i][j];
          str1_substr.end_ = i;
          str2_substr.end_ = j;
        }
      } else {
        dp[i][j] = 0;
      }
    }
  }

  str1_substr.start_ = str1_substr.end_ - substr_len;
  str2_substr.start_ = str2_substr.end_ - substr_len;
  str1_substr.value_ = str1.substr(str1_substr.start_, substr_len);
  str2_substr.value_ = str1_substr.value_;
  return {str1_substr, str2_substr};
}

size_t LongestCommonSubstringLength(const string& str1, const string& str2) {
  return LongestCommonSubstring(str1, str2).first.value_.length();
}

size_t LongestCommonSubsequenceLength(
    const string& str1, const string& str2, vector<vector<int>>* dp_p) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  auto&& dp = dp_p ? *dp_p
                   : vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));

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
  auto dp = vector<vector<int>>(len1 + 1, vector<int>(len2 + 1));
  LongestCommonSubsequenceLength(str1, str2, &dp);

  string result;
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
  return result;
}

void Concat(string* base_p, const string* str_p) {
  auto base_len = base_p->length();
  auto str_len = str_p->length();
  auto max_overlap_len = min(base_len, str_len);

  if (!base_len) {
    *base_p = *str_p;
    return;
  }

  size_t replace_start = base_len - max_overlap_len;
  string replace_str;
  auto base_suffix_str = base_p->substr(replace_start);
  auto common_str = LongestCommonSubstring(base_suffix_str, *str_p);
  auto common_str_len = common_str.first.value_.length();
  if (common_str_len >= Config::OVERLAP_MIN_LEN) {
    replace_start += common_str.first.end_;
    replace_str = str_p->substr(common_str.second.end_);

    Logger::Trace(
        "Concat",
        "Common substring length: " + to_string(common_str_len) + " \tused");
  } else {
    replace_str = ShortestCommonSupersequence(base_suffix_str, *str_p);

    Logger::Trace(
        "Concat",
        "Common substring length: " + to_string(common_str_len) +
            " \tnot used, finding common supersequence");
  }

  base_p->erase(base_p->begin() + replace_start, base_p->end());
  *base_p += replace_str;
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
