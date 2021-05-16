#include "utils.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.h"

using std::ceil;
using std::max;
using std::string;
using std::unordered_map;
using std::vector;

extern Config config;

bool QuickCompare(int num1, int num2) {
  return abs(num1 - num2) <= ceil(max(num1, num2) * (1 - config.cover_rate));
}

bool FuzzyCompare(int num1, int num2) {
  return abs(num1 - num2) <= config.tolerance;
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
  if (common_len >= max_len * config.strict_rate) return true;

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
  if (dp[len1][len2] >= max_len * config.fuzzy_rate) return true;

  return false;
}
