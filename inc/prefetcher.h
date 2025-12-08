#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

// forward declaration
class Prefetcher;

//--------------------------------------------------------//
// Prefetch tracker is a seprate option structure
// present in a base prefetcher.
// It's goal is to monitor every prefetch request
// and get a rough measurement of prefetcher's accuracy.
//-------------------------------------------------------//
class PTracker {
private:
  Prefetcher *parent = NULL;
  uint32_t num_sets = 128;
  uint32_t num_ways = 16;
  std::vector<std::deque<uint64_t>> tracker;

  // main stats
  uint64_t unique_pref = 0;
  uint64_t demand_hits = 0;

  uint32_t get_set(uint64_t addr);

public:
  PTracker(Prefetcher *parent, uint32_t sets, uint32_t ways);
  ~PTracker();
  void track_pref(uint64_t addr);
  void track_demand(uint64_t addr);
  uint32_t get_accuracy() {
    return unique_pref ? (float)demand_hits / unique_pref * 100 : 100;
  }
  void reset_tracker() {
    unique_pref = 0;
    demand_hits = 0;
    for (uint32_t i = 0; i < num_sets; ++i) {
      tracker[i].clear();
    }
  }
};

class Prefetcher {
public:
  uint32_t id;
  std::string type;
  PTracker *ptracker = NULL;

public:
  Prefetcher(std::string _type) {
    type = _type;
    id = 0; // Default ID
  }
  ~Prefetcher() {
  }

  inline void set_id(uint32_t _id) {
    id = _id;
  }

  inline uint32_t get_id() {
    return id;
  }

  std::string get_type() {
    return type;
  }
  virtual void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, std::vector<uint64_t> &pref_addr) = 0;
  virtual void dump_stats() = 0;
  virtual void print_config() = 0;

  //----------------------------------------------//
  // Functionality for orchestrating prefetchers
  //----------------------------------------------//
  virtual void create_ptracker(uint32_t sets, uint32_t ways);
  virtual uint32_t get_accuracy() {
    return ptracker ? ptracker->get_accuracy() : 100;
  }
  virtual void reset_accuracy() {
    if (ptracker) {
      ptracker->reset_tracker();
    }
  }
};

#endif /* PREFETCHER_H */
