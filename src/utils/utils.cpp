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

// Find longest common subsequence.
bool FuzzyCompare(const string& str1, const string& str2) {
  auto len1 = str1.length();
  auto len2 = str2.length();

  auto dp = vector<vector<int16_t>>(len1 + 1, vector<int16_t>(len2 + 1, 0));
  for (auto i = 1; i <= len1; ++i) {
    for (auto j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }

  return dp[len1][len2] >= max(len1, len2) * config.fuzzy_rate;
}

bool QuickCompare(int num1, int num2) {
  return abs(num1 - num2) <= ceil(max(num1, num2) * (1 - config.fuzzy_rate));
}

bool QuickCompare(const string& str1, const string& str2) {
  if (!QuickCompare(str1.length(), str2.length())) return false;
  unordered_map<char, size_t> count1, count2;
  for (const auto& c : str1) ++count1[c];
  for (const auto& c : str2) ++count2[c];
  for (const auto& [key, value] : count1) {
    if (key == 'N') continue;
    if (!QuickCompare(count1[key], count2[key])) return false;
  }
  return true;
}
