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
#include "utils/utils.h"

using std::get;
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

  while (!in_file.eof()) {
    string key, value;
    in_file >> key >> value;
    if (!key.length()) break;
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

    logger.Trace(
        "Dna::FindDeltasChunk",
        start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
    assert(mid.x_ <= end.x_ && mid.y_ <= end.y_);

    auto insert_delta = [&](const Point& start, const Point& end) {
      auto size = end.y_ - start.y_;
      ins_deltas_.Set(
          key,
          {
              ref_start + start.x_,
              ref_start + start.x_ + size,
              sv_p->substr(sv_start + start.y_, size),
          });
    };

    auto delete_delta = [&](const Point& start, const Point& end) {
      auto size = end.x_ - start.x_;
      del_deltas_.Set(
          key,
          {
              ref_start + start.x_,
              ref_start + end.x_,
              ref_p->substr(ref_start + start.x_, size),
          });
    };

    // If we meet a snake or the direction is changed, we store previous deltas.
    if (mid != end || from_up != prev_from_up) {
      if (prev_from_up == 1 && end.y_ < prev_end.y_) {
        insert_delta(end, prev_end);
      } else if (!prev_from_up && end.x_ < prev_end.x_) {
        delete_delta(end, prev_end);
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
        insert_delta({}, prev_end);
      } else {
        delete_delta({}, prev_end);
      }
    }

    prev_from_up = mid == end ? from_up : -1;
    cur = start;
  }

  return next_chunk_start;
}

void Dna::FindDupDeltas() {
  for (auto&& [key, ranges] : ins_deltas_.data_) {
    for (auto range_i = ranges.begin(); range_i < ranges.end();) {
      auto size = range_i->size();
      auto prev_start = range_i->start_ - size;
      if (prev_start < 0) continue;
      auto prev_value = data_[key].substr(prev_start, size);
      if (FuzzyCompare(range_i->value_, prev_value)) {
        dup_deltas_.Set(key, {prev_start, range_i->start_, prev_value});
        range_i = ranges.erase(range_i);
      } else {
        ++range_i;
      }
    }
  }
}

void Dna::FindInvDeltas() {
  const unordered_map<char, char> dna_base_pair{
      {'A', 'T'},
      {'T', 'A'},
      {'C', 'G'},
      {'G', 'C'},
      {'N', 'N'},
  };
  auto invert_chain = [&](const string& chain) {
    string inverted_chain;
    for (auto i = chain.rbegin(); i < chain.rend(); ++i) {
      inverted_chain += dna_base_pair.at(*i);
    }
    return inverted_chain;
  };

  for (auto&& [key, ranges_ins] : ins_deltas_.data_) {
    for (auto range_i = ranges_ins.begin(); range_i < ranges_ins.end();) {
      auto erased = false;
      if (del_deltas_.data_.count(key)) {
        auto& ranges_del = del_deltas_.data_[key];
        for (auto range_j = ranges_del.begin();
             range_j < ranges_del.end() && range_i->start_ >= range_j->start_;
             ++range_j) {
          if (range_i->start_ == range_j->end_ &&
              QuickCompare(*range_i, *range_j) &&
              FuzzyCompare(range_i->value_, invert_chain(range_j->value_))) {
            inv_deltas_.Set(key, *range_j);
            range_i = ranges_ins.erase(range_i);
            range_j = ranges_del.erase(range_j);
            erased = true;
            break;
          }
        }
      }
      if (!erased) ++range_i;
    }
  }
}

void Dna::FindTraDeltas() {
  unordered_map<size_t, vector<tuple<string, Range>>> ins_cache;
  unordered_map<size_t, vector<tuple<string, Range>>> del_cache;

  for (auto&& [key, ranges_ins] : ins_deltas_.data_) {
    for (auto range_i = ranges_ins.begin(); range_i < ranges_ins.end();) {
      auto erased = false;
      if (del_deltas_.data_.count(key)) {
        auto& ranges_del = del_deltas_.data_[key];
        for (auto range_j = ranges_del.begin();
             range_j < ranges_del.end() && range_i->start_ >= range_j->start_;
             ++range_j) {
          if (range_i->start_ == range_j->end_ &&
              QuickCompare(*range_i, *range_j)) {
            auto size = range_i->size();
            ins_cache[size].push_back({key, *range_i});
            del_cache[size].push_back({key, *range_j});
            range_i = ranges_ins.erase(range_i);
            range_j = ranges_del.erase(range_j);
            erased = true;
            break;
          }
        }
      }
      if (!erased) ++range_i;
    }
  }

  for (auto&& [size, entries_ins] : ins_cache) {
    if (del_cache.count(size)) {
      auto entries_del = del_cache[size];
      for (auto entry_i = entries_ins.begin(); entry_i < entries_ins.end();) {
        auto erased = false;
        auto [key_ins, range_ins] = *entry_i;
        for (auto entry_j = entries_del.begin(); entry_j < entries_del.end();
             ++entry_j) {
          auto [key_del, range_del] = *entry_j;
          if (QuickCompare(range_ins.value_, range_del.value_) &&
              FuzzyCompare(range_ins.value_, range_del.value_)) {
            range_ins.end_ = range_ins.start_;
            range_ins.start_ -= size;
            tra_deltas_.Set(key_ins, range_ins, key_del, range_del);
            entry_i = entries_ins.erase(entry_i);
            entry_j = entries_del.erase(entry_j);
            erased = true;
            break;
          }
        }
        if (!erased) ++entry_i;
      }
    }
  }
}

void Dna::ProcessDeltas() {
  ins_deltas_.Combine();
  del_deltas_.Combine();
  FindDupDeltas();
  FindInvDeltas();
  FindTraDeltas();
}

bool Dna::PrintDeltas(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error(
        "Dna::PrintDeltas", "output file " + filename + " not found\n");
    return false;
  }

  ins_deltas_.Print(out_file);
  del_deltas_.Print(out_file);
  dup_deltas_.Print(out_file);
  inv_deltas_.Print(out_file);
  tra_deltas_.Print(out_file);

  out_file.close();
  return true;
}
