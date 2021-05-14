#include "dna.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "utils/logger.h"

using std::get;
using std::getline;
using std::ifstream;
using std::min;
using std::string;
using std::to_string;
using std::tuple;
using std::unordered_map;
using std::vector;

struct Point {
  Point(size_t x, size_t y) : x_(x), y_(y) {}
  string Stringify() const {
    return "(" + to_string(x_) + ", " + to_string(y_) + ")";
  }
  bool operator==(const Point& that) const {
    return x_ == that.x_ && y_ == that.y_;
  }
  bool operator!=(const Point& that) const { return !(*this == that); }

  size_t x_ = 0;
  size_t y_ = 0;
};

extern Logger logger;

bool Dna::Import(const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
    logger.Error("Dna::Import", "input file " + filename + " not found\n");
    return false;
  }

  for (string key; getline(in_file, key);) {
    string value;
    getline(in_file, value);
    ImportEntry(key.substr(1), value);
  }

  in_file.close();
  return true;
}

bool Dna::ImportEntry(const string& key, const string& value) {
  data_[key] = value;
  return true;
}

bool Dna::Get(const string& key, string* value) const {
  try {
    *value = data_.at(key);
    return true;
  } catch (std::out_of_range error) {
    *value = "";
    return false;
  }
}

void Dna::FindDelta(const Dna& sv, size_t chunk_size) {
  for (const auto& [key, value_ref] : data_) {
    string value_sv;
    if (!sv.Get(key, &value_sv)) {
      logger.Warn("Dna::FindDelta", "key " + key + " not found in sv chain");
      continue;
    }

    for (auto [i, j] = tuple{0, 0};
         i < value_ref.length() || j < value_sv.length();) {
      auto m = min(value_ref.length() - i, chunk_size);
      auto n = min(value_sv.length() - j, chunk_size);
      FindDeltaChunk(key, &value_ref, i, m, &value_sv, j, n);
      i += m, j += n;
    }
  }
}

// Myers' diff algorithm implementation
void Dna::FindDeltaChunk(
    const string& key,
    const string* ref_p,
    size_t ref_start,
    size_t m,
    const string* sv_p,
    size_t sv_start,
    size_t n) {
  auto max_steps = m + n;
  auto padding = max_steps;
  /**
   * end_xs[k] stores the latest end point (x, y) on each k-line.
   * Since we define k as (x - y), we only need to store x.
   * Thus we have end_xs[k] = x, and (x, y) = (x, x - k).
   * As -(m + n) <= k <= m + n, we add (m + n) to k as the index of end_xs
   * when storing points.
   * Finally, we have end_xs[k + m + n] = x.
   */
  auto end_xs = vector<int>((max_steps << 1) + 1, 0);
  // end_xss[step] stores end_xs at each step.
  auto end_xss = vector<vector<int>>{};
  auto solution_found = false;

  /**
   * If k == -step, we must come from k-line of (k + 1).
   * If k == step, we must come from k-line of (k - 1).
   * Otherwise, we choose to start from the adjacent k-line which has a
   * greater x value.
   */
  auto is_from_up = [](const vector<int>& end_xs, int k, int step) {
    if (k == -step) return true;
    if (k == step) return false;
    return end_xs[k + 1] > end_xs[k - 1];
  };

  for (auto step = 0ul; step <= max_steps; ++step) {
    /**
     * At each step, we can only reach the k-line ranged from -step to step.
     * Notice that we can only reach odd (even) k-lines after odd (even) steps,
     * we increment k by 2 at each iteration.
     */
    for (auto k = -step; k <= step; k += 2) {
      auto from_up = is_from_up(end_xs, k, step);
      auto prev_k = from_up ? k + 1 : k - 1;
      auto start_x = end_xs[prev_k];
      auto start = Point(start_x, start_x - prev_k);

      auto mid_x = from_up ? start.x_ : start.x_ + 1;
      auto mid = Point(mid_x, mid_x - k);

      auto end = mid;
      auto snake = 0;
      for (; end.x_ < m && end.y_ < n && (*ref_p)[end.x_] == (*sv_p)[end.y_];
           ++end.x_, ++end.y_, ++snake) {
      }

      end_xs[k + padding] = end.x_;
      if (end.x_ == m && end.y_ == n) {
        solution_found = true;
        break;
      }
    }
    end_xss.push_back(end_xs);
    if (solution_found) break;
  }

  auto prev_from_up = false;
  auto prev_end = Point{m, n};
  for (auto cur = prev_end; cur.x_ > 0 || cur.y_ > 0;) {
    end_xs = end_xss.back();
    end_xss.pop_back();
    auto step = end_xss.size();

    auto k = cur.x_ - cur.y_;

    auto end_x = end_xs[k];
    auto end = Point(end_x, end_x - k);

    auto from_up = is_from_up(end_xs, k, step);
    auto prev_k = from_up ? k + 1 : k - 1;
    auto start_x = end_xs[prev_k];
    auto start = Point(start_x, start_x - prev_k);

    auto mid_x = from_up ? start.x_ : start.x_ + 1;
    auto mid = Point(mid_x, mid_x - k);

    logger.Debug(
        "Dna::FindDeltaChunk",
        start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());

    if (mid != end || from_up != prev_from_up) {
      if (prev_from_up) {
        ins_delta_.Set(key, {ref_start + end.y_, ref_start + prev_end.y_});
      } else {
        del_delta_.Set(key, {ref_start + end.x_, ref_start + prev_end.x_});
      }
      prev_end = mid;
    }

    cur = start;
  }
}

bool Dna::PrintDelta(const string& filename) const {
  return ins_delta_.Print(filename) && del_delta_.Print(filename) &&
         dup_delta_.Print(filename) && inv_delta_.Print(filename) &&
         tra_delta_.Print(filename);
}
