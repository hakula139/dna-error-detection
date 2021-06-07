#include "dna.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.h"
#include "dna_overlap.h"
#include "logger.h"
#include "minimizer.h"
#include "point.h"
#include "progress.h"
#include "range.h"
#include "utils.h"

using std::endl;
using std::greater;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::out_of_range;
using std::pair;
using std::priority_queue;
using std::string;
using std::to_string;
using std::tuple;
using std::unordered_map;
using std::vector;

bool Dna::Import(const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
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
    Logger::Error("Dna::ImportIndex", "Input file " + filename + " not found");
    return false;
  }

  while (!in_file.eof()) {
    uint64_t hash = 0;
    string key;
    Range range;
    in_file >> hash >> key >> range.start_ >> range.end_ >> range.value_;
    if (!hash || !key.length()) break;
    range_index_.emplace(hash, pair{key, range});
  }

  in_file.close();
  return true;
}

bool Dna::ImportOverlaps(const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
    return false;
  }

  while (!in_file.eof()) {
    string key_ref, key_seg;
    Range range_ref, range_seg;
    in_file >> key_ref >> range_ref.start_ >> range_ref.end_;
    in_file >> key_seg >> range_seg.start_ >> range_seg.end_;
    if (!key_ref.length() || !key_seg.length()) break;
    overlaps_.Insert(key_ref, {range_ref, key_seg, range_seg});
  }

  in_file.close();
  return true;
}

bool Dna::get(const string& key, string* value) const {
  try {
    *value = data_.at(key);
    return true;
  } catch (const out_of_range& error) {
    *value = "";
    return false;
  }
}

bool Dna::Print(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    Logger::Error("Dna::Print", "Cannot create output file " + filename);
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
  const uint64_t mask = ~(UINT64_MAX << (Config::HASH_SIZE << 1));
  return ((hash << 2) & mask) | dna_base_map.at(next_base);
}

struct HashPos {
  HashPos() {}
  HashPos(uint64_t hash, size_t pos) : hash_(hash), pos_(pos) {}

  bool operator<(const HashPos& that) const { return hash_ > that.hash_; }

  uint64_t hash_ = 0;
  size_t pos_ = 0;
};

void Dna::CreateIndex() {
  assert(Config::HASH_SIZE > 0 && Config::HASH_SIZE <= 30);

  for (const auto& [key_ref, value_ref] : data_) {
    uint64_t hash = 0;
    for (auto i = 0; i < Config::HASH_SIZE - 1; ++i) {
      hash = NextHash(hash, value_ref[i]);
    }

    priority_queue<HashPos> hashes;
    HashPos prev_min_hash;
    auto i_end = value_ref.length() - Config::HASH_SIZE + 1;
    Progress progress{"Dna::CreateIndex " + key_ref, i_end, 10000};
    for (size_t i = 0; i < i_end; ++i) {
      while (hashes.size() && hashes.top().pos_ + Config::WINDOW_SIZE <= i) {
        hashes.pop();
      }
      hash = NextHash(hash, value_ref[i + Config::HASH_SIZE - 1]);
      hashes.emplace(hash, i);

      auto min_hash = hashes.top();
      if (min_hash.pos_ != prev_min_hash.pos_) {
        Range range_ref{
            min_hash.pos_,
            min_hash.pos_ + Config::HASH_SIZE,
            value_ref.substr(min_hash.pos_, Config::HASH_SIZE),
        };
        range_index_.emplace(min_hash.hash_, pair{key_ref, range_ref});
        prev_min_hash = min_hash;

        Logger::Trace(
            "Dna::CreateIndex",
            "Saved " + range_ref.value_ + " " + to_string(min_hash.hash_));
      }

      ++progress;
    }
  }
}

bool Dna::PrintIndex(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    Logger::Error("Dna::PrintIndex", "Cannot create output file " + filename);
    return false;
  }

  for (const auto& [hash, entry] : range_index_) {
    const auto& [key_ref, range_ref] = entry;
    out_file << hash << " " << range_ref.Stringify(key_ref) << " "
             << range_ref.value_ << "\n";
  }

  out_file.close();
  return true;
}

bool Dna::FindOverlaps(const Dna& ref) {
  if (!ref.range_index_.size()) {
    Logger::Warn("Dna::FindOverlaps", "No index found in reference data");
    return false;
  }
  assert(Config::HASH_SIZE > 0 && Config::HASH_SIZE <= 30);

  auto find_overlaps = [&](const string& key_seg, const string& chain_seg) {
    uint64_t hash = 0;
    for (auto i = 0; i < Config::HASH_SIZE - 1; ++i) {
      hash = NextHash(hash, chain_seg[i]);
    }

    DnaOverlap overlaps;
    for (size_t i = 0; i < chain_seg.length() - Config::HASH_SIZE + 1; ++i) {
      hash = NextHash(hash, chain_seg[i + Config::HASH_SIZE - 1]);
      if (ref.range_index_.count(hash)) {
        const auto& entry_ref_range = ref.range_index_.equal_range(hash);
        Range range_seg{i, i + Config::HASH_SIZE};
        for (auto j = entry_ref_range.first; j != entry_ref_range.second; ++j) {
          const auto& [key_ref, range_ref] = j->second;
          overlaps.Insert(key_ref, {range_ref, key_seg, range_seg});
        }
      }
    }
    return overlaps;
  };

  Progress progress{"Dna::FindOverlaps", data_.size(), 100};
  for (auto&& [key_seg, value_seg] : data_) {
    auto overlaps = find_overlaps(key_seg, value_seg);
    auto inverted_value_seg = Invert(value_seg);
    auto overlaps_invert = find_overlaps(key_seg, inverted_value_seg);

    if (overlaps.size() >= Config::MINIMIZER_MIN_COUNT &&
        overlaps.size() >= overlaps_invert.size()) {
      overlaps_ += overlaps;

      Logger::Trace(
          "Dna::FindOverlaps",
          key_seg + ": " + to_string(overlaps.size()) + " > " +
              to_string(overlaps_invert.size()) + " \tnot inverted");
    } else if (
        overlaps_invert.size() >= Config::MINIMIZER_MIN_COUNT &&
        overlaps.size() < overlaps_invert.size()) {
      value_seg = inverted_value_seg;
      overlaps_ += overlaps_invert;

      Logger::Trace(
          "Dna::FindOverlaps",
          key_seg + ": " + to_string(overlaps.size()) + " < " +
              to_string(overlaps_invert.size()) + " \tinverted");
    } else {
      Logger::Trace(
          "Dna::FindOverlaps",
          key_seg + ": " + to_string(overlaps.size()) + " & " +
              to_string(overlaps_invert.size()) + " \tnot used");
    }

    ++progress;
  }

  overlaps_.Merge();
  return true;
}

bool Dna::PrintOverlaps(const string& filename) const {
  ofstream out_file(filename);
  if (!out_file) {
    Logger::Error(
        "Dna::PrintOverlaps", "Cannot create output file " + filename);
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
      Logger::Warn("Dna::FindDeltas", "key " + key + " not found in sv chain");
      continue;
    }

    Progress progress{"Dna::FindDeltas " + key, value_ref.length()};
    for (int i = 0, j = 0; i < value_ref.length() || j < value_sv.length();) {
      auto m = min(value_ref.length() - i, chunk_size);
      auto n = min(value_sv.length() - j, chunk_size);
      auto reach_end = m < chunk_size || n < chunk_size;
      auto next_chunk_start = FindDeltasChunk(
          key, &value_ref, i, m, &value_sv, j, n, reach_end);
      i += next_chunk_start.x_;
      j += next_chunk_start.y_;

      progress.Set(i);
    }
  }
}

void Dna::FindDeltasFromSegments(const Dna& segments) {
  for (const auto& [key, value_ref] : data_) {
    try {
      const auto& entries = segments.overlaps_.data_.at(key);
      Progress progress{
          "Dna::FindDeltasFromSegments " + key,
          entries.size(),
          10,
      };
      for (const auto& minimizer : entries) {
        const auto& [range_ref, key_seg, range_seg] = minimizer;
        const auto& value_seg = segments.data_.at(key_seg);
        assert(range_ref.start_ >= range_seg.start_);
        const auto ref_start = range_ref.start_ - range_seg.start_;

        auto seg_size = value_seg.size();
        auto dp = LongestCommonSubsequence(
            value_ref.substr(ref_start, seg_size), value_seg);

        auto prev_from_up = -1;
        for (int i = seg_size, j = seg_size; i > 0 || j > 0;) {
          auto from_up = dp[i][j].second;
          if (from_up == 0) {
            --i;
          } else if (from_up == 1) {
            --j;
          } else {
            --i, --j;
          }
        }

        Logger::Debug("Dna::FindDeltasFromSegments", key + ": Now comparing:");
        Logger::Debug("", "Reference: \t" + value_ref.substr(ref_start, 100));
        Logger::Debug("", "Segment: \t" + value_seg.substr(0, 100));

        ++progress;
      }
    } catch (const out_of_range& error) {
      Logger::Warn("Dna::FindDeltasFromSegments", error.what());
      continue;
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
      if (snake < Config::SNAKE_MIN_LEN) end = mid;

      end_xs[k + padding] = end.x_;

      if (reach_end ? end.x_ >= m && end.y_ >= n : end.x_ >= m || end.y_ >= n) {
        solution_found = true;
        next_chunk_start = Point(end.x_, end.y_);
        break;
      }
    }
    end_xss.emplace_back(end_xs);
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

    // Logger::Trace(
    //     "Dna::FindDeltasChunk",
    //     start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
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
            ins_cache.emplace_back(key, *range_i);
            del_cache.emplace_back(key, *range_j);
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
    ranges_ins.emplace_back(range);
  }

  for (const auto& [key, range] : del_cache) {
    auto& ranges_del = del_deltas_.data_[key];
    ranges_del.emplace_back(range);
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
    Logger::Error("Dna::PrintDeltas", "Cannot create output file " + filename);
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
