#include "dna.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "point.h"
#include "utils/config.h"
#include "utils/logger.h"

using std::get;
using std::getline;
using std::ifstream;
using std::min;
using std::ofstream;
using std::string;
using std::to_string;
using std::tuple;
using std::unordered_map;
using std::vector;

extern Config config;

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

void Dna::FindDeltas(const Dna& sv, size_t chunk_size) {
  for (const auto& [key, value_ref] : data_) {
    string value_sv;
    if (!sv.Get(key, &value_sv)) {
      logger.Warn("Dna::FindDeltas", "key " + key + " not found in sv chain");
      continue;
    }

    for (auto [i, j] = tuple{0, 0};
         i < value_ref.length() || j < value_sv.length();) {
      auto m = min(value_ref.length() - i, chunk_size);
      auto n = min(value_sv.length() - j, chunk_size);
      auto reach_end = m < chunk_size || n < chunk_size;
      auto next_chunk_start = FindDeltasChunk(
          key, &value_ref, i, m, &value_sv, j, n, reach_end);
      i += next_chunk_start.x_;
      j += next_chunk_start.y_;

      logger.Info(
          "Dna::FindDeltas",
          key + ": " + to_string(i) + " / " + to_string(value_ref.length()));
    }
  }
}

// Myers' diff algorithm implementation
Point Dna::FindDeltasChunk(
    const string& key,
    const string* ref_p,
    size_t ref_start,
    size_t m,
    const string* sv_p,
    size_t sv_start,
    size_t n,
    bool reach_end) {
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
  auto next_chunk_start = Point(m, n);

  /**
   * If k == -step, we must come from k-line of (k + 1).
   * If k == step, we must come from k-line of (k - 1).
   * Otherwise, we choose to start from the adjacent k-line which has a
   * greater x value.
   */
  auto is_from_up = [=](const vector<int>& end_xs, int k, int step) {
    if (k == -step) return true;
    if (k == step) return false;
    return end_xs[k + 1 + padding] > end_xs[k - 1 + padding];
  };

  for (auto step = 0; step <= max_steps; ++step) {
    /**
     * At each step, we can only reach the k-line ranged from -step to step.
     * Notice that we can only reach odd (even) k-lines after odd (even) steps,
     * we increment k by 2 at each iteration.
     */
    for (auto k = -step; k <= step; k += 2) {
      auto from_up = is_from_up(end_xs, k, step);
      auto prev_k = from_up ? k + 1 : k - 1;
      auto start_x = end_xs[prev_k + padding];
      auto start = Point(start_x, start_x - prev_k);

      auto mid_x = from_up ? start.x_ : start.x_ + 1;
      auto mid = Point(mid_x, mid_x - k);

      auto end = mid;
      auto snake = 0;
      for (; end.x_ < m && end.y_ < n; ++end.x_, ++end.y_, ++snake) {
        auto ref_char = (*ref_p)[ref_start + end.x_];
        auto sv_char = (*sv_p)[sv_start + end.y_];
        if (ref_char != sv_char && ref_char != 'N' && sv_char != 'N') break;
      }
      if (snake < config.tolerance) end = mid;

      end_xs[k + padding] = end.x_;

      if (reach_end ? end.x_ >= m && end.y_ >= n : end.x_ >= m || end.y_ >= n) {
        solution_found = true;
        next_chunk_start = Point(end.x_, end.y_);
        break;
      }
    }
    end_xss.push_back(end_xs);
    if (solution_found) break;
  }

  auto prev_from_up = -1;
  auto prev_end = Point();
  for (auto cur = next_chunk_start; cur.x_ > 0 || cur.y_ > 0;) {
    end_xs = end_xss.back();
    end_xss.pop_back();
    auto step = end_xss.size();

    auto k = cur.x_ - cur.y_;

    auto end_x = end_xs[k + padding];
    auto end = Point(end_x, end_x - k);

    auto from_up = is_from_up(end_xs, k, step);
    auto prev_k = from_up ? k + 1 : k - 1;
    auto start_x = end_xs[prev_k + padding];
    auto start = Point(start_x, start_x - prev_k);

    auto mid_x = from_up ? start.x_ : start.x_ + 1;
    auto mid = Point(mid_x, mid_x - k);

    // logger.Debug(
    //     "Dna::FindDeltasChunk",
    //     start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
    assert(mid.x_ <= end.x_ && mid.y_ <= end.y_);

    // If we meet a snake or the direction is changed, we store previous deltas.
    if (mid != end || from_up != prev_from_up) {
      if (prev_from_up == 1 && end.y_ < prev_end.y_) {
        ins_delta_.Set(key, {ref_start + end.y_, ref_start + prev_end.y_});
      } else if (!prev_from_up && end.x_ < prev_end.x_) {
        del_delta_.Set(key, {ref_start + end.x_, ref_start + prev_end.x_});
      }
      prev_end = mid;
    }

    // If we meet the start point, we should store all unsaved deltas before the
    // loop terminates.
    auto reach_start = start.x_ <= 0 && start.y_ <= 0;
    if (reach_start && prev_end != Point() && end != Point()) {
      // The unsaved deltas must have the same type as current delta, because
      // the direction must be unchanged. Otherwise, it will be handled by
      // previous procedures.
      if (from_up) {
        ins_delta_.Set(key, {ref_start, ref_start + prev_end.y_});
      } else {
        del_delta_.Set(key, {ref_start, ref_start + prev_end.x_});
      }
    }

    prev_from_up = mid == end ? from_up : -1;
    cur = start;
  }

  return next_chunk_start;
}

void Dna::ProcessDeltas() {
  ins_delta_.Combine();
  del_delta_.Combine();
}

bool Dna::PrintDeltas(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error(
        "Dna::PrintDeltas", "output file " + filename + " not found\n");
    return false;
  }

  ins_delta_.Print(out_file);
  del_delta_.Print(out_file);
  dup_delta_.Print(out_file);
  inv_delta_.Print(out_file);
  tra_delta_.Print(out_file);
  return true;
}
