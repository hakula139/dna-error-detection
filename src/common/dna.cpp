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
#include <unordered_set>
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
using std::move;
using std::ofstream;
using std::out_of_range;
using std::pair;
using std::priority_queue;
using std::string;
using std::swap;
using std::to_string;
using std::tuple;
using std::unordered_map;
using std::unordered_set;
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

  unordered_set<string> inverted_segs;

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
      if (!inverted_segs.count(key_seg)) {
        value_seg = Invert(value_seg);
        inverted_segs.emplace(key_seg);
      }
      swap(start_seg, end_seg);
    } else if (inverted_segs.count(key_seg)) {
      Logger::Warn("Dna::ImportOverlaps " + key_seg, "Inversion conflict");
    }

    Range range_ref{start_ref, end_ref, &(this->data_.at(key_ref))};
    Range range_seg{start_seg, end_seg, &value_seg};
    overlaps_.Insert(key_ref, {range_ref, key_seg, range_seg});

    Logger::Trace("Dna::ImportOverlaps " + key_ref, "Minimizer:");
    Logger::Trace("", "REF: \t" + range_ref.Head());
    Logger::Trace("", "SEG: \t" + range_seg.Head());
  }

  in_file.close();
  return true;
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
    for (size_t i = 0; i < Config::HASH_SIZE - 1; ++i) {
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
                           const string& raw_chain_seg,
                           bool inverted) {
    auto chain_seg = inverted ? Invert(raw_chain_seg) : raw_chain_seg;

    uint64_t hash = 0;
    for (size_t i = 0; i < Config::HASH_SIZE - 1; ++i) {
      hash = NextHash(hash, chain_seg[i]);
    }

    DnaOverlap overlaps;
    for (size_t i = 0; i < chain_seg.length() - Config::HASH_SIZE + 1; ++i) {
      hash = NextHash(hash, chain_seg[i + Config::HASH_SIZE - 1]);

      if (ref.range_index_.count(hash)) {
        const auto& entry_ref_range = ref.range_index_.equal_range(hash);
        Range range_seg{i, i + Config::HASH_SIZE, &raw_chain_seg, inverted};

        for (auto j = entry_ref_range.first; j != entry_ref_range.second; ++j) {
          const auto& [key_ref, range_ref] = j->second;
          overlaps.Insert(key_ref, {range_ref, key_seg, range_seg});

          // Logger::Trace("Dna::FindOverlaps", key_ref + ": \tMinimizer:");
          // Logger::Trace("", "REF: \t" + range_ref.get());
          // Logger::Trace("", "SEG: \t" + range_seg.get());
        }
      }
    }
    return overlaps;
  };

  Progress progress{"Dna::FindOverlaps", data_.size(), 100};
  for (auto&& [key_seg, value_seg] : data_) {
    auto overlaps = find_overlaps(key_seg, value_seg, false);
    auto overlaps_invert = find_overlaps(key_seg, value_seg, true);

    if (overlaps.size() >= Config::OVERLAP_MIN_COUNT &&
        overlaps.size() >= overlaps_invert.size()) {
      overlaps_ += overlaps;

      Logger::Trace(
          "Dna::FindOverlaps",
          key_seg + ": " + to_string(overlaps.size()) + " > " +
              to_string(overlaps_invert.size()) + " \tnot inverted");
    } else if (
        overlaps_invert.size() >= Config::OVERLAP_MIN_COUNT &&
        overlaps.size() < overlaps_invert.size()) {
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
  overlaps_.CheckCoverage();
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
  for (const auto& [key_ref, value_ref] : data_) {
    const auto& value_sv = sv.data_.at(key_ref);
    Progress progress{"Dna::FindDeltas " + key_ref, value_ref.length()};

    for (size_t i = 0, j = 0;
         i < value_ref.length() || j < value_sv.length();) {
      auto m = min(value_ref.length() - i, chunk_size);
      auto n = min(value_sv.length() - j, chunk_size);
      auto reach_start = true;
      auto reach_end = m < chunk_size || n < chunk_size;

      auto next_chunk_start = FindDeltasChunk(
          key_ref,
          value_ref,
          i,
          m,
          key_ref,
          value_sv,
          j,
          n,
          reach_start,
          reach_end);

      i += next_chunk_start.x_;
      j += next_chunk_start.y_;
      progress.Set(i);
    }
  }
}

void Dna::FindDeltasFromSegments() {
  for (const auto& [key_ref, value_ref] : data_) {
    const auto& entries = overlaps_.data_.at(key_ref);
    unordered_set<string> used_segs;
    Progress progress{
        "Dna::FindDeltasFromSegments " + key_ref,
        entries.size(),
        100,
    };

    for (const auto& minimizer : entries) {
      const auto& [range_ref, key_seg, range_seg] = minimizer;
      if (used_segs.count(key_seg)) {
        ++progress;
        continue;
      }
      used_segs.insert(key_seg);

      const auto& value_seg = *(range_seg.value_p_);

      auto ref_size = value_ref.size();
      auto seg_size = value_seg.size();

      auto start_padding = range_seg.start_ + Config::DELTA_MAX_LEN;
      auto end_padding = seg_size - range_seg.end_ + Config::DELTA_MAX_LEN;

      Range ref{
          max(range_ref.start_, start_padding) - start_padding,
          min(range_ref.end_ + end_padding, ref_size),
          &value_ref,
      };
      Range seg{
          0,
          seg_size,
          &value_seg,
      };

      Logger::Debug(
          "Dna::FindDeltasFromSegments",
          ref.Stringify(key_ref) + " " + seg.Stringify(key_seg));

      Logger::Trace("", "REF: \t" + Head(value_ref, ref.start_));
      Logger::Trace("", "SEG: \t" + Head(value_seg));
      Logger::Trace("", "REF minimizer: \t" + range_ref.Head());
      Logger::Trace("", "SEG minimizer: \t" + range_seg.Head());

      FindDeltasChunk(
          key_ref,
          value_ref,
          ref.start_,
          ref.size(),
          key_seg,
          value_seg,
          seg.start_,
          seg.size(),
          false,
          false);

      IgnoreSmallDeltas(key_ref, key_seg);
      ++progress;
    }
  }
}

// Myers' diff algorithm implementation
Point Dna::FindDeltasChunk(
    const string& key_ref,
    const string& ref,
    size_t ref_start,
    size_t m,
    const string& key_sv,
    const string& sv,
    size_t sv_start,
    size_t n,
    bool reach_start,
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
  auto get_direction = [=](const vector<int>& end_xs, int k, int step) {
    if (k == -step) return Direction::TOP;
    if (k == step) return Direction::LEFT;
    if (end_xs[k + 1 + padding] > end_xs[k - 1 + padding]) {
      return Direction::TOP;
    } else {
      return Direction::LEFT;
    }
  };

  for (auto step = 0ul; step <= max_steps; ++step) {
    /**
     * At each step, we can only reach the k-line ranged from -step to step.
     * Notice that we can only reach odd (even) k-lines after odd (even) steps,
     * we increment k by 2 at each iteration.
     */
    for (int k = -step; k <= static_cast<int>(step); k += 2) {
      auto direction = get_direction(end_xs, k, step);
      auto prev_k = direction == Direction::TOP ? k + 1 : k - 1;
      auto start_x = end_xs[prev_k + padding];
      auto start = Point(start_x, start_x - prev_k);

      auto mid_x = direction == Direction::TOP ? start.x_ : start.x_ + 1;
      auto mid = Point(mid_x, mid_x - k);

      auto end = mid;
      auto snake = 0ul;
      for (auto [error_len, error_score] = tuple{0, 0.0};
           end.x_ < static_cast<int>(m) && end.y_ < static_cast<int>(n);
           ++end.x_, ++end.y_, ++snake) {
        auto ref_char = ref[ref_start + end.x_];
        auto sv_char = sv[sv_start + end.y_];
        if (ref_char != sv_char && ref_char != 'N' && sv_char != 'N') {
          ++error_len, ++error_score;
          if (error_score > Config::ERROR_MAX_SCORE) {
            --error_len;
            end = {end.x_ - error_len, end.y_ - error_len};
            snake -= error_len;
            break;
          }
        } else {
          error_score = max(error_score - Config::MYERS_PENALTY, 0.0);
          if (!error_score) error_len = 0ul;
        }
      }
      if (snake < Config::SNAKE_MIN_LEN) end = mid;

      end_xs[k + padding] = end.x_;

      auto x_reach_end = end.x_ >= static_cast<int>(m);
      auto y_reach_end = end.y_ >= static_cast<int>(n);
      auto terminate_end = reach_end ? x_reach_end && y_reach_end
                                     : x_reach_end || y_reach_end;
      if (terminate_end) {
        solution_found = true;
        next_chunk_start = end;
        break;
      }
    }
    end_xss.emplace_back(end_xs);
    if (solution_found) break;
  }

  auto prev_direction = Direction::TOP_LEFT;
  auto prev_end = Point();
  auto terminate_start = false;

  for (auto cur = next_chunk_start; !terminate_start;) {
    end_xs = end_xss.back();
    end_xss.pop_back();
    auto step = end_xss.size();

    auto k = cur.x_ - cur.y_;

    auto end_x = end_xs[k + padding];
    auto end = Point(end_x, end_x - k);

    auto direction = get_direction(end_xs, k, step);
    auto prev_k = direction == Direction::BOTTOM ? k + 1 : k - 1;
    auto start_x = end_xs[prev_k + padding];
    auto start = Point(start_x, start_x - prev_k);

    auto mid_x = direction == Direction::TOP ? start.x_ : start.x_ + 1;
    auto mid = Point(mid_x, mid_x - k);

    Logger::Trace(
        "Dna::FindDeltasChunk",
        start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
    assert(start.x_ <= mid.x_ && start.y_ <= mid.y_);
    assert(mid.x_ <= end.x_ && mid.y_ <= end.y_);

    auto insert_delta = [&](const Point& start, const Point& end) {
      auto size = end.y_ - start.y_;
      if (!size) return;
      ins_deltas_.Set(
          key_ref,
          {
              {ref_start + start.x_, ref_start + start.x_ + size, &ref},
              key_sv,
              {sv_start + start.y_, sv_start + end.y_, &sv},
          });
    };

    auto delete_delta = [&](const Point& start, const Point& end) {
      auto size = end.x_ - start.x_;
      if (!size) return;
      del_deltas_.Set(
          key_ref,
          {
              {ref_start + start.x_, ref_start + end.x_, &ref},
              key_sv,
              {ref_start + start.x_, ref_start + end.x_, &ref},
          });
    };

    // If we meet a snake or the direction is changed, we store previous deltas.
    if (mid != end || direction != prev_direction) {
      if (prev_direction == Direction::TOP) {
        insert_delta(end, prev_end);
      } else if (prev_direction == Direction::LEFT) {
        delete_delta(end, prev_end);
      }
      prev_end = mid;
    }

    /**
     * If we meet the start point, we should store all unsaved deltas before the
     * loop terminates.
     */
    auto x_reach_start = start.x_ <= 0;
    auto y_reach_start = start.y_ <= 0;
    terminate_start = reach_start ? x_reach_start && y_reach_start
                                  : x_reach_start || y_reach_start;

    if (terminate_start) {
      /**
       * The unsaved deltas must have the same type as current delta, because
       * the direction must be unchanged. Otherwise, it will be handled by
       * previous procedures.
       */
      if (direction == Direction::TOP) {
        insert_delta({}, prev_end);
      } else if (direction == Direction::LEFT) {
        delete_delta({}, prev_end);
      }
    }

    prev_direction = start != mid ? direction : Direction::TOP_LEFT;
    cur = start;
  }

  return next_chunk_start;
}

void Dna::IgnoreSmallDeltas(const string& key_ref, const string& key_seg) {
  auto filter = [&](DnaDelta& all_deltas) {
    auto type = all_deltas.type_;

    auto filter_ref = [&](vector<Minimizer>& deltas, const string& key_ref_i) {
      for (auto delta_i = deltas.rbegin();
           delta_i < deltas.rend() && delta_i->key_seg_ == key_seg;) {
        const auto& range_ref = (delta_i++)->range_ref_;
        if (range_ref.size() < Config::DELTA_MIN_LEN) {
          deltas.erase(delta_i.base());
        } else {
          Logger::Debug(
              "DnaDelta::IgnoreSmallDeltas",
              "Saved: \t" + type + " " + range_ref.Stringify(key_ref_i));
        }
      }
    };

    if (key_ref.empty()) {
      for (auto&& [key_ref_i, deltas] : all_deltas.data_) {
        filter_ref(deltas, key_ref_i);
      }
    } else {
      auto&& deltas = all_deltas.data_[key_ref];
      filter_ref(deltas, key_ref);
    }
  };

  filter(ins_deltas_);
  filter(del_deltas_);

  if (key_ref.empty()) {
    Logger::Info("Dna::IgnoreSmallDeltas", "Done");
  }
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
      auto prev_value = range_ref.value_p_->substr(prev_start, size);

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

  Logger::Info("Dna::FindDupDeltas", "Done");
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

  Logger::Info("Dna::FindInvDeltas", "Done");
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

  Logger::Info("Dna::FindTraDeltas", "Done");
}

void Dna::ProcessDeltas() {
  IgnoreSmallDeltas();
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
