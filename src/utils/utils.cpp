#include "utils.h"

#include <algorithm>
#include <string>
#include <vector>

#include "config.h"

using std::max;
using std::string;
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

  return dp[len1][len2] >= max(len1, len2) * config.fuzzy_match_rate;
}
