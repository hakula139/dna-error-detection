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
using std::swap;
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
    size_t start, end;

    in_file >> hash >> key >> start >> end;
    if (!hash || !key.length()) break;

    Range range{start, end, &(this->data_.at(key))};
    range_index_.emplace(hash, pair{key, range});
  }

  in_file.close();
  return true;
}

bool Dna::ImportOverlaps(Dna* segments_p, const string& filename) {
  ifstream in_file(filename);
  if (!in_file) {
    return false;
  }

  while (!in_file.eof()) {
    string key_ref, key_seg;
    size_t start_ref, end_ref;
    size_t start_seg, end_seg;

    in_file >> key_ref >> start_ref >> end_ref;
    in_file >> key_seg >> start_seg >> end_seg;
    if (!key_ref.length() || !key_seg.length()) break;

    // Invert the segment chain if marked inverted.
    auto&& value_seg = segments_p->data_[key_seg];
    if (start_seg > end_seg) {
      swap(start_seg, end_seg);
      value_seg = Invert(value_seg);
    }

    Range range_ref{start_ref, end_ref, &(this->data_.at(key_ref))};
    Range range_seg{start_seg, end_seg, &value_seg};
    overlaps_.Insert(key_ref, {range_ref, key_seg, range_seg});

    Logger::Trace("Dna::ImportOverlaps", key_ref + ": \tSaved minimizer:");
    Logger::Trace("", "REF: \t" + range_ref.get());
    Logger::Trace("", "SEG: \t" + range_seg.get());
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
            &value_ref,
        };
        range_index_.emplace(min_hash.hash_, pair{key_ref, range_ref});
        prev_min_hash = min_hash;

        Logger::Trace(
            "Dna::CreateIndex",
            "Saved " + range_ref.get() + " " + to_string(min_hash.hash_));
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
    out_file << hash << " " << range_ref.Stringify(key_ref) << "\n";
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

  auto find_overlaps = [&](const string& key_seg,
                           const string& chain_seg,
                           bool inverted) {
    uint64_t hash = 0;
    for (auto i = 0; i < Config::HASH_SIZE - 1; ++i) {
      hash = NextHash(hash, chain_seg[i]);
    }

    DnaOverlap overlaps;
    for (size_t i = 0; i < chain_seg.length() - Config::HASH_SIZE + 1; ++i) {
      hash = NextHash(hash, chain_seg[i + Config::HASH_SIZE - 1]);
      if (ref.range_index_.count(hash)) {
        const auto& entry_ref_range = ref.range_index_.equal_range(hash);
        Range range_seg{i, i + Config::HASH_SIZE, &chain_seg, inverted};
        for (auto j = entry_ref_range.first; j != entry_ref_range.second; ++j) {
          const auto& [key_ref, range_ref] = j->second;
          overlaps.Insert(key_ref, {range_ref, key_seg, range_seg});

          Logger::Trace("Dna::FindOverlaps", key_ref + ": \tSaved minimizer:");
          Logger::Trace("", "REF: \t" + range_ref.get());
          Logger::Trace("", "SEG: \t" + range_seg.get());
        }
      }
    }
    return overlaps;
  };

  Progress progress{"Dna::FindOverlaps", data_.size(), 100};
  for (auto&& [key_seg, value_seg] : data_) {
    auto inverted_value_seg = Invert(value_seg);
    auto overlaps = find_overlaps(key_seg, value_seg, false);
    auto overlaps_invert = find_overlaps(key_seg, inverted_value_seg, true);

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
          key, value_ref, i, m, value_sv, j, n, reach_end);
      i += next_chunk_start.x_;
      j += next_chunk_start.y_;

      progress.Set(i);
    }
  }
}

void Dna::FindDeltasFromSegments() {
  for (const auto& [key, value_ref] : data_) {
    try {
      const auto& entries = overlaps_.data_.at(key);
      Progress progress{
          "Dna::FindDeltasFromSegments " + key,
          entries.size(),
          10,
      };

      for (const auto& minimizer : entries) {
        const auto& [range_ref, key_seg, range_seg] = minimizer;
        const auto& value_seg = *(range_seg.value_p_);
        assert(range_ref.start_ >= range_seg.start_);
        const auto ref_start = range_ref.start_ - range_seg.start_;

        auto show_size = 100;
        Logger::Debug("Dna::FindDeltasFromSegments", key + ": \tComparing:");
        Logger::Debug("", "REF: \t" + value_ref.substr(ref_start, show_size));
        Logger::Debug("", "SEG: \t" + value_seg.substr(0, show_size));

        auto seg_size = value_seg.size();
        FindDeltasChunk(
            key, value_ref, ref_start, seg_size, value_seg, 0, seg_size, true);

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
    const string& ref,
    size_t ref_start,
    size_t m,
    const string& sv,
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
      for (int error_len = 0, error_score = 0; end.x_ < m && end.y_ < n;
           ++end.x_, ++end.y_, ++snake) {
        auto ref_char = ref[ref_start + end.x_];
        auto sv_char = sv[sv_start + end.y_];
        if (ref_char != sv_char && ref_char != 'N' && sv_char != 'N') {
          ++error_len, ++error_score;
          if (error_score > Config::ERROR_MAX_LEN) {
            --error_len;
            end = {end.x_ - error_len, end.y_ - error_len};
            snake -= error_len;
            break;
          }
        } else {
          error_score = max(error_score - Config::DP_PENALTY, 0);
          if (!error_score) error_len = 0;
        }
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

    Logger::Trace(
        "Dna::FindDeltasChunk",
        start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
    assert(start.x_ <= mid.x_ && start.y_ <= mid.y_);
    assert(mid.x_ <= end.x_ && mid.y_ <= end.y_);

    auto insert_delta = [&](const Point& start, const Point& end) {
      auto size = end.y_ - start.y_;
      assert(size > 0);
      ins_deltas_.Set(
          key,
          {
              {ref_start + start.x_, ref_start + start.x_ + size, &ref},
              "",
              {sv_start + start.y_, sv_start + end.y_, &sv},
          });
    };

    auto delete_delta = [&](const Point& start, const Point& end) {
      auto size = end.x_ - start.x_;
      assert(size > 0);
      del_deltas_.Set(
          key,
          {
              {ref_start + start.x_, ref_start + end.x_, &ref},
              "",
              {ref_start + start.x_, ref_start + end.x_, &ref},
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

void Dna::IgnoreSmallDeltas() {
  auto ignore_small_deltas = [](DnaDelta* deltas_p) {
    for (auto&& [key, deltas] : deltas_p->data_) {
      for (auto delta_i = deltas.begin(); delta_i < deltas.end();) {
        if (delta_i->range_ref_.size() < Config::OVERLAP_MIN_LEN) {
          delta_i = deltas.erase(delta_i);
        } else {
          ++delta_i;
        }
      }
    }
  };

  ignore_small_deltas(&ins_deltas_);
  ignore_small_deltas(&del_deltas_);
}

void Dna::FindDupDeltas() {
  for (auto&& [key_ref, deltas] : ins_deltas_.data_) {
    for (auto delta_i = deltas.begin(); delta_i < deltas.end();) {
      auto&& [range_ref, key_seg, range_seg] = *delta_i;
      auto size = range_ref.size();
      auto cur_start = range_ref.start_;
      auto prev_start = cur_start - size;
      if (prev_start < 0) continue;

      auto cur_value = range_seg.get();
      auto prev_value = range_seg.value_p_->substr(prev_start, size);

      if (FuzzyCompare(cur_value, prev_value)) {
        dup_deltas_.Set(
            key_ref,
            {
                {prev_start, cur_start, range_ref.value_p_},
                key_seg,
                range_seg,
            });
        delta_i = deltas.erase(delta_i);
      } else {
        ++delta_i;
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
  for (auto&& [key_ref, deltas_ins] : ins_deltas_.data_) {
    for (auto delta_i = deltas_ins.begin(); delta_i < deltas_ins.end();) {
      auto&& [range_ref_i, key_seg_i, range_seg_i] = *delta_i;
      auto erased = false;

      if (del_deltas_.data_.count(key_ref)) {
        auto& deltas_del = del_deltas_.data_[key_ref];

        for (auto delta_j = deltas_del.begin(); delta_j < deltas_del.end();
             ++delta_j) {
          auto&& [range_ref_j, key_seg_j, range_seg_j] = *delta_j;

          if (FuzzyCompare(range_ref_i, range_ref_j)) {
            auto size = range_ref_i.size();
            range_ref_i.start_ = range_ref_j.start_;
            range_ref_i.end_ = range_ref_j.start_ + size;

            if (FuzzyCompare(range_seg_i.get(), Invert(range_seg_j.get()))) {
              inv_deltas_.Set(key_ref, *delta_j);
              delta_i = deltas_ins.erase(delta_i);
              delta_j = deltas_del.erase(delta_j);
              erased = true;
              break;
            }
          }
        }
      }

      if (!erased) ++delta_i;
    }
  }
}

void Dna::FindTraDeltas() {
  vector<tuple<string, Minimizer>> ins_cache;
  vector<tuple<string, Minimizer>> del_cache;

  for (auto&& [key, deltas_ins] : ins_deltas_.data_) {
    for (auto delta_i = deltas_ins.begin(); delta_i < deltas_ins.end();) {
      auto&& [range_ref_i, key_seg_i, range_seg_i] = *delta_i;
      auto erased = false;

      if (del_deltas_.data_.count(key)) {
        auto& deltas_del = del_deltas_.data_[key];

        for (auto delta_j = deltas_del.begin(); delta_j < deltas_del.end();
             ++delta_j) {
          auto&& [range_ref_j, key_seg_j, range_seg_j] = *delta_j;

          if (FuzzyCompare(range_ref_i.size(), range_ref_j.size())) {
            ins_cache.emplace_back(key, *delta_i);
            del_cache.emplace_back(key, *delta_j);
            delta_i = deltas_ins.erase(delta_i);
            delta_j = deltas_del.erase(delta_j);
            erased = true;
            break;
          }
        }
      }

      if (!erased) ++delta_i;
    }
  }

  for (auto entry_i = ins_cache.begin(); entry_i < ins_cache.end();) {
    auto [key_ins, delta_ins] = *entry_i;
    auto&& [range_ref_i, key_seg_i, range_seg_i] = delta_ins;
    auto erased = false;

    for (auto entry_j = del_cache.begin(); entry_j < del_cache.end();
         ++entry_j) {
      auto [key_del, delta_del] = *entry_j;
      auto&& [range_ref_j, key_seg_j, range_seg_j] = delta_del;

      if (FuzzyCompare(range_seg_i.get(), range_seg_j.get())) {
        tra_deltas_.Set(key_ins, range_ref_i, key_del, range_ref_j);
        entry_i = ins_cache.erase(entry_i);
        entry_j = del_cache.erase(entry_j);
        erased = true;
        break;
      }
    }

    if (!erased) ++entry_i;
  }

  for (const auto& [key, delta] : ins_cache) {
    auto& deltas_ins = ins_deltas_.data_[key];
    deltas_ins.emplace_back(delta);
  }

  for (const auto& [key, delta] : del_cache) {
    auto& deltas_del = del_deltas_.data_[key];
    deltas_del.emplace_back(delta);
  }
}

void Dna::ProcessDeltas() {
  // IgnoreSmallDeltas();
  // FindDupDeltas();
  // FindInvDeltas();
  // FindTraDeltas();
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
