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
using std::max_element;
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
    if (key.length() <= 1) break;

    key = key.substr(1);
    data_[key] = value;

    auto density_size = value.size() + Config::PADDING_SIZE;
    ins_deltas_.density_[key].resize(density_size);
    del_deltas_.density_[key].resize(density_size);
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
    int64_t start_ref, end_ref;
    int64_t start_seg, end_seg;

    in_file >> key_ref >> start_ref >> end_ref;
    in_file >> key_seg >> start_seg >> end_seg;
    if (!key_ref.length() || !key_seg.length()) break;

    const auto& value_ref = this->data_.at(key_ref);
    // Invert the segment chain if marked inverted.
    auto&& value_seg = segments_p->data_[key_seg];
    if (start_seg <= 0 && end_seg <= 0) {
      value_seg = Transform(value_seg, COMPLEMENT);
      start_seg = -start_seg;
      end_seg = -end_seg;
    }
    if (start_seg > end_seg) {
      value_seg = Transform(value_seg, REVERSE);
      swap(start_seg, end_seg);
    }

    Range range_ref{
        static_cast<size_t>(start_ref),
        static_cast<size_t>(end_ref),
        &value_ref,
    };
    Range range_seg{
        static_cast<size_t>(start_seg),
        static_cast<size_t>(end_seg),
        &value_seg,
    };

    Logger::Trace("Dna::ImportOverlaps " + key_ref, "Minimizer:");
    Logger::Trace("", "REF: \t" + range_ref.Head());
    Logger::Trace("", "SEG: \t" + range_seg.Head());

    if (Verify(range_ref, range_seg)) {
      overlaps_.Insert(key_ref, {range_ref, key_seg, range_seg});
    } else {
      Logger::Warn(
          "Dna::ImportOverlaps",
          key_ref + " " + key_seg + " minimizer not matched");
    }
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
  unordered_map<string, size_t> index_count;

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
        ++index_count[key_ref];

        Logger::Trace(
            "Dna::CreateIndex",
            "Saved " + range_ref.get() + " " + to_string(min_hash.hash_));
      }

      ++progress;
    }

    Logger::Debug(
        "Dna::CreateIndex " + key_ref,
        "Count: " + to_string(index_count[key_ref]));
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
                           Mode mode) {
    auto chain_seg = Transform(raw_chain_seg, mode);
    assert(chain_seg.length() > 0);

    uint64_t hash = 0;
    for (size_t i = 0; i < Config::HASH_SIZE - 1; ++i) {
      hash = NextHash(hash, chain_seg[i]);
    }

    DnaOverlap overlaps;
    for (size_t i = 0; i < chain_seg.length() - Config::HASH_SIZE + 1; ++i) {
      hash = NextHash(hash, chain_seg[i + Config::HASH_SIZE - 1]);

      if (ref.range_index_.count(hash)) {
        const auto& entry_ref_range = ref.range_index_.equal_range(hash);
        Range range_seg{i, i + Config::HASH_SIZE, &raw_chain_seg, mode};

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
    vector<DnaOverlap> overlaps_map;
    for (auto mode : {NORMAL, REVERSE, COMPLEMENT, REVR_COMP}) {
      overlaps_map.push_back(find_overlaps(key_seg, value_seg, mode));
    }

    auto best_i = max_element(overlaps_map.begin(), overlaps_map.end());

    auto overlap_count = best_i->size();
    if (overlap_count < Config::OVERLAP_MIN_COUNT) {
      Logger::Trace(
          "Dna::FindOverlaps " + key_seg,
          to_string(overlap_count) + " not used");
    } else {
      auto mode = static_cast<Mode>(best_i - overlaps_map.begin());
      value_seg = Transform(value_seg, mode);
      overlaps_ += *best_i;

      Logger::Debug(
          "Dna::FindOverlaps " + key_seg,
          to_string(overlap_count) + " using mode: " + to_string(mode));
    }

    ++progress;
  }

  overlaps_.Merge();
  overlaps_.SelectChain();
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

      Logger::Debug(
          "Dna::FindDeltasFromSegments",
          range_ref.Stringify(key_ref) + " " + range_seg.Stringify(key_seg));

      Logger::Trace("", "REF: \t" + range_ref.Head());
      Logger::Trace("", "SEG: \t" + range_seg.Head());

      if (!Verify(range_ref, range_seg)) {
        Logger::Warn(
            "Dna::FindDeltasFromSegments",
            key_ref + " " + key_seg + " minimizer not matched");
      }

      FindDeltasChunk(
          key_ref,
          value_ref,
          range_ref.start_,
          range_ref.size(),
          key_seg,
          value_seg,
          range_seg.start_,
          range_seg.size(),
          true,
          true);

      auto merge = [](DnaDelta& deltas,
                      const string& key_ref,
                      const Minimizer& minimizer) {
        const auto& [range_ref, key_seg, range_seg] = minimizer;
        vector<Range> delta_ranges;

        auto density = deltas.GetDensity(key_ref, range_ref, &delta_ranges);
        if (density > Config::SIGNAL_RATE) {
          for (const auto& delta_range : delta_ranges) {
            deltas.Merge(key_ref, key_seg, delta_range);
          }
        }
        deltas.Filter(key_ref, key_seg);
      };

      merge(ins_deltas_, key_ref, minimizer);
      merge(del_deltas_, key_ref, minimizer);
      ++progress;
    }

    ins_deltas_.Merge(key_ref);
    del_deltas_.Merge(key_ref);
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
    if (k == -step) return TOP;
    if (k == step) return LEFT;
    if (end_xs[k + 1 + padding] > end_xs[k - 1 + padding]) {
      return TOP;
    } else {
      return LEFT;
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
      auto prev_k = direction == TOP ? k + 1 : k - 1;
      auto start_x = end_xs[prev_k + padding];
      auto start = Point(start_x, start_x - prev_k);

      auto mid_x = direction == TOP ? start.x_ : start.x_ + 1;
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

  auto prev_direction = TOP_LEFT;
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
    auto prev_k = direction == BOTTOM ? k + 1 : k - 1;
    auto start_x = end_xs[prev_k + padding];
    auto start = Point(start_x, start_x - prev_k);

    auto mid_x = direction == TOP ? start.x_ : start.x_ + 1;
    auto mid = Point(mid_x, mid_x - k);

    // Logger::Trace(
    //     "Dna::FindDeltasChunk",
    //     start.Stringify() + " " + mid.Stringify() + " " + end.Stringify());
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
      if (prev_direction == TOP) {
        insert_delta(end, prev_end);
      } else if (prev_direction == LEFT) {
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
      if (direction == TOP) {
        insert_delta({}, prev_end);
      } else if (direction == LEFT) {
        delete_delta({}, prev_end);
      }
    }

    prev_direction = start != mid ? direction : TOP_LEFT;
    cur = start;
  }

  return next_chunk_start;
}

void Dna::IgnoreSmallDeltas(const string& key_ref, const string& key_seg) {
  ins_deltas_.Filter(key_ref, key_seg);
  del_deltas_.Filter(key_ref, key_seg);

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

string Dna::Transform(const string& chain, Mode mode) {
  const unordered_map<char, char> dna_base_pair{
      {'A', 'T'},
      {'T', 'A'},
      {'C', 'G'},
      {'G', 'C'},
      {'N', 'N'},
  };

  string output_chain;

  switch (mode) {
    case REVERSE:
      for (auto i = chain.rbegin(); i < chain.rend(); ++i) {
        output_chain += *i;
      }
      break;
    case COMPLEMENT:
      for (auto i = chain.begin(); i < chain.end(); ++i) {
        output_chain += dna_base_pair.at(*i);
      }
      break;
    case REVR_COMP:
      for (auto i = chain.rbegin(); i < chain.rend(); ++i) {
        output_chain += dna_base_pair.at(*i);
      }
      break;
    case NORMAL:
    default:
      output_chain = chain;
      break;
  }
  return output_chain;
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
            // auto size = range_ref_i.size();
            // range_ref_i.start_ = range_ref_j.start_;
            // range_ref_i.end_ = range_ref_j.start_ + size;

            if (FuzzyCompare(
                    range_seg_i.get(),
                    Transform(range_seg_j.get(), REVR_COMP))) {
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
