#include "dna.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <fstream>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.h"
#include "dna_overlap.h"
#include "logger.h"
#include "point.h"
#include "range.h"
#include "utils.h"

using std::endl;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::priority_queue;
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
    logger.Error("Dna::Import", "Input file " + filename + " not found");
    return false;
  }

  while (!in_file.eof()) {
    string key, value;
    in_file >> key >> value;
    if (!key.length()) break;
    data_[key.substr(1)] = value;
  }

  in_file.close();
  return true;
}

bool Dna::ImportIndex(const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
    logger.Error("Dna::ImportIndex", "Input file " + filename + " not found");
    return false;
  }

  while (!in_file.eof()) {
    uint64_t hash = 0;
    string key;
    size_t start, end;
    in_file >> hash >> key >> start >> end;
    if (!hash || !key.length()) break;
    range_index_[hash] = {key, {start, end}};
  }

  in_file.close();
  return true;
}

bool Dna::get(const string& key, string* value) const {
  try {
    *value = data_.at(key);
    return true;
  } catch (std::out_of_range error) {
    *value = "";
    return false;
  }
}

bool Dna::Print(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error("Dna::Print", "Cannot create output file " + filename);
    return false;
  }

  for (const auto& [key, value] : data_) {
    out_file << ">" << key << "\n" << value << endl;
  }

  out_file.close();
  return true;
}

uint64_t Dna::NextHash(uint64_t hash, char next_base) {
  const unordered_map<char, char> dna_base_map{
      {'A', 0},
      {'T', 1},
      {'C', 2},
      {'G', 3},
      {'N', 0},
  };
  const uint64_t mask = ~(UINT64_MAX << (config.hash_size << 1));
  return ((hash << 2) & mask) | dna_base_map.at(next_base);
}

void Dna::CreateIndex() {
  for (const auto& [key, value_ref] : data_) {
    uint64_t prev_hash = 0;
    assert(config.hash_size > 0);
    for (auto i = 0; i < config.hash_size - 1; ++i) {
      prev_hash = NextHash(prev_hash, value_ref[i]);
    }

    using HashPos = pair<uint64_t, size_t>;
    auto compare = [](const HashPos& h1, const HashPos& h2) {
      return h1.first > h2.first;
    };
    priority_queue<HashPos, vector<HashPos>, decltype(compare)> hashes{compare};

    assert(config.hash_size <= config.window_size);
    auto capacity = config.window_size - config.hash_size + 1;
    for (auto i = config.hash_size - 1; i < value_ref.length(); ++i) {
      while (hashes.size() && hashes.top().second + config.window_size < i) {
        hashes.pop();
      }
      prev_hash = NextHash(prev_hash, value_ref[i]);
      hashes.push({prev_hash, i - config.hash_size});
      if (hashes.size() < capacity) continue;
      auto min_hash = hashes.top();
      if (!range_index_.count(min_hash.first)) {
        range_index_[min_hash.first] = {
            key,
            {min_hash.second, min_hash.second + config.hash_size},
        };
      }
    }
  }
}

bool Dna::PrintIndex(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error("Dna::PrintIndex", "Cannot create output file " + filename);
    return false;
  }

  for (const auto& [hash, entry] : range_index_) {
    out_file << hash << " " << entry.second.Stringify(entry.first) << "\n";
  }

  out_file.close();
  return true;
}

bool Dna::FindOverlaps(const Dna& ref) {
  if (!ref.range_index_.size()) {
    logger.Warn("Dna::FindOverlaps", "No index found in reference data");
    return false;
  }

  auto find_overlaps = [&](const string& key_seg, const string& chain_seg) {
    uint64_t prev_hash = 0;
    assert(config.hash_size > 0);
    for (auto i = 0; i < config.hash_size - 1; ++i) {
      prev_hash = NextHash(prev_hash, chain_seg[i]);
    }

    DnaOverlap overlaps;
    for (auto i = config.hash_size - 1; i < chain_seg.length(); ++i) {
      prev_hash = NextHash(prev_hash, chain_seg[i]);
      if (ref.range_index_.count(prev_hash)) {
        const auto& [key_ref, range_ref] = ref.range_index_.at(prev_hash);
        auto range_seg = Range(i - config.hash_size + 1, i);
        overlaps += {
            key_ref,
            range_ref,
            key_seg,
            range_seg,
        };
      }
    }
    return overlaps;
  };

  auto is_valid = [](uint64_t overlap_size, uint64_t chain_size) {
    return overlap_size >= config.strict_equal_rate * chain_size;
  };

  for (auto&& [key_seg, value_seg] : data_) {
    auto overlaps = find_overlaps(key_seg, value_seg);
    if (is_valid(overlaps.size(), value_seg.length())) {
      overlaps_ += overlaps;
    } else {
      auto inverted_value_seg = Invert(value_seg);
      auto overlaps_invert = find_overlaps(key_seg, inverted_value_seg);
      if (overlaps.size() >= overlaps_invert.size()) {
        overlaps_ += overlaps;
      } else {
        value_seg = inverted_value_seg;
        overlaps_ += overlaps_invert;
      }
    }

    logger.Debug("Dna::FindOverlaps", key_seg + ": Done");
  }

  overlaps_.Sort();
  return true;
}

void Dna::ProcessOverlaps() {}

bool Dna::PrintOverlaps(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error("Dna::PrintOverlaps", "Cannot create output file " + filename);
    return false;
  }

  overlaps_.Print(out_file);

  out_file.close();
  return true;
}

void Dna::FindDeltas(const Dna& sv, size_t chunk_size) {
  for (const auto& [key, value_ref] : data_) {
    string value_sv;
    if (!sv.get(key, &value_sv)) {
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
      if (snake < config.snake_min_len) end = mid;

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

string Dna::Invert(const string& chain) {
  const unordered_map<char, char> dna_base_pair{
      {'A', 'T'},
      {'T', 'A'},
      {'C', 'G'},
      {'G', 'C'},
      {'N', 'N'},
  };
  string inverted_chain;
  for (auto i = chain.rbegin(); i < chain.rend(); ++i) {
    inverted_chain += dna_base_pair.at(*i);
  }
  return inverted_chain;
}

void Dna::FindInvDeltas() {
  for (auto&& [key, ranges_ins] : ins_deltas_.data_) {
    for (auto range_i = ranges_ins.begin(); range_i < ranges_ins.end();) {
      auto erased = false;
      if (del_deltas_.data_.count(key)) {
        auto& ranges_del = del_deltas_.data_[key];
        for (auto range_j = ranges_del.begin(); range_j < ranges_del.end();
             ++range_j) {
          if (FuzzyCompare(*range_i, *range_j)) {
            auto size = range_i->size();
            range_i->start_ = range_j->start_;
            range_i->end_ = range_j->start_ + size;
            if (FuzzyCompare(range_i->value_, Invert(range_j->value_))) {
              inv_deltas_.Set(key, *range_j);
              range_i = ranges_ins.erase(range_i);
              range_j = ranges_del.erase(range_j);
              erased = true;
              break;
            }
          }
        }
      }
      if (!erased) ++range_i;
    }
  }
}

void Dna::FindTraDeltas() {
  auto ins_cache = vector<tuple<string, Range>>{};
  auto del_cache = vector<tuple<string, Range>>{};

  for (auto&& [key, ranges_ins] : ins_deltas_.data_) {
    for (auto range_i = ranges_ins.begin(); range_i < ranges_ins.end();) {
      auto erased = false;
      if (del_deltas_.data_.count(key)) {
        auto& ranges_del = del_deltas_.data_[key];
        for (auto range_j = ranges_del.begin(); range_j < ranges_del.end();
             ++range_j) {
          if (FuzzyCompare(range_i->size(), range_j->size())) {
            ins_cache.push_back({key, *range_i});
            del_cache.push_back({key, *range_j});
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

  for (auto entry_i = ins_cache.begin(); entry_i < ins_cache.end();) {
    auto erased = false;
    auto [key_ins, range_ins] = *entry_i;
    for (auto entry_j = del_cache.begin(); entry_j < del_cache.end();
         ++entry_j) {
      auto [key_del, range_del] = *entry_j;
      if (FuzzyCompare(range_ins.value_, range_del.value_)) {
        tra_deltas_.Set(key_ins, range_ins, key_del, range_del);
        entry_i = ins_cache.erase(entry_i);
        entry_j = del_cache.erase(entry_j);
        erased = true;
        break;
      }
    }
    if (!erased) ++entry_i;
  }

  for (const auto& [key, range] : ins_cache) {
    auto& ranges_ins = ins_deltas_.data_[key];
    ranges_ins.push_back(range);
  }

  for (const auto& [key, range] : del_cache) {
    auto& ranges_del = del_deltas_.data_[key];
    ranges_del.push_back(range);
  }
}

void Dna::ProcessDeltas() {
  FindDupDeltas();
  FindInvDeltas();
  FindTraDeltas();
}

bool Dna::PrintDeltas(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    logger.Error("Dna::PrintDeltas", "Cannot create output file " + filename);
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
