#include "utils.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.h"
#include "logger.h"
#include "range.h"

using std::ceil;
using std::cout;
using std::max;
using std::min;
using std::out_of_range;
using std::pair;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

std::pair<Range, Range> LongestCommonSubstring(
    const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  vector<vector<int>> dp(len1 + 1, vector<int>(len2 + 1));

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
        dp[i][j] = max(dp[i - 1][j - 1] - Config::DP_PENALTY, 0);
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

size_t LongestCommonSubsequenceLength(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();
  vector<vector<pair<int, char>>> dp{
      len1 + 1,
      vector<pair<int, char>>{len2 + 1, {0, -1}},
  };

  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      auto& max_cur = dp[i][j];
      auto get_cur = [&](const pair<int, char>& prev, char from_up) {
        const auto& [prev_len, prev_from_up] = prev;
        auto cur = prev_len + (from_up == -1);
        if (prev_from_up != from_up) cur = max(cur - Config::DP_PENALTY, 0);
        if (cur > max_cur.first) max_cur = {cur, from_up};
      };

      if (str1[i - 1] == str2[j - 1]) {
        get_cur(dp[i - 1][j - 1], -1);
      }
      if (dp[i - 1][j] > dp[i][j - 1]) {
        get_cur(dp[i - 1][j], 0);
      }
      if (dp[i - 1][j] <= dp[i][j - 1]) {
        get_cur(dp[i][j - 1], 1);
      }
    }
  }
  return dp[len1][len2].first;
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
    Logger::Trace(
        "Concat",
        "Common substring length: " + to_string(common_str_len) + " \tused");

    replace_start += common_str.first.end_;
    replace_str = str_p->substr(common_str.second.end_);
  } else {
    Logger::Trace(
        "Concat",
        "Common substring length: " + to_string(common_str_len) +
            " \tnot used, concatenate directly");

    *base_p += *str_p;
    return;
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
