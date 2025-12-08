#include "prefetcher.h"
#include "knobs.h"
#include "util.h"
#include <algorithm>
#include <assert.h>

PTracker::PTracker(Prefetcher *_parent, uint32_t sets, uint32_t ways) : parent(_parent), num_sets(sets), num_ways(ways) {
  // init tracker structure
  std::deque<uint64_t> d;
  tracker.resize(num_sets, d);
}

PTracker::~PTracker() {
}

void PTracker::track_pref(uint64_t pref_addr) {
  uint32_t set = get_set(pref_addr);
  auto it = std::find_if(tracker[set].begin(), tracker[set].end(), [pref_addr](uint64_t addr) { return addr == pref_addr; });

  if (it != tracker[set].end()) {
    ; // do nothing
  } else {
    if (tracker[set].size() == num_ways) {
      tracker[set].pop_front();
    }
    tracker[set].push_back(pref_addr);
    unique_pref++;
  }
}

void PTracker::track_demand(uint64_t demand_addr) {
  uint32_t set = get_set(demand_addr);
  auto it = std::find_if(tracker[set].begin(), tracker[set].end(), [demand_addr](uint64_t addr) { return addr == demand_addr; });
  if (it != tracker[set].end()) {
    demand_hits++;
  }
}

uint32_t PTracker::get_set(uint64_t val) {
  uint32_t folded_val = folded_xor(val, 2);
  uint32_t hash = HashZoo::getHash(knob::ptracker_hash_type, folded_val);
  return hash % num_sets;
}

void Prefetcher::create_ptracker(uint32_t sets, uint32_t ways) {
  ptracker = new PTracker(this, sets, ways);
  assert(ptracker);
}