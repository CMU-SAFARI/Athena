#ifndef OOGWAY_HELPER_H
#define OOGWAY_HELPER_H

#include <cassert>
#include <cstdint>
#include <sstream>
#include <string>

//----------------------------------------//
// System state information
//----------------------------------------//
class og_state_t {
public:
  // state variables
  // Type 1: accuracy metrics
  uint32_t pref_acc[2] = {0, 0};
  uint32_t ocp_acc = 0;

  // Type 2: bandwidth metrics
  uint32_t overall_bw = 0;
  uint32_t pref_bw = 0;
  uint32_t ocp_bw = 0;
  uint32_t bw_needed = 0;

  uint32_t total_scheduled = 0;
  uint32_t demand_scheduled = 0;
  uint32_t pf_scheduled = 0;
  uint32_t ocp_scheduled = 0;

  // Type 3: pollution metrics
  uint32_t pref_pollution = 0;

  // Aux variables - not used as state feature
  uint64_t epoch_num = 0;
  uint64_t num_retired_insts = 0;

  std::string to_string();
  uint64_t get_raw64(); // RBERA: TODO
  uint32_t get_raw32();
};

//----------------------------------------//
// System events, which are used
// to calculate rewards
//----------------------------------------//
typedef enum {
  /* events that Oogway can affect */
  CYCLE,
  LLC_LOAD_MISS,
  LLC_LOAD_MISS_LAT,
  /* events that invariant to Oogway */
  MISPRED_BRANCH,
  LOAD_INST_ISSUE,
  L1D_LOAD_MISS,
  NUM_EVENTS,
} og_sysevent_type_t;

std::string og_sysevent_type2str(og_sysevent_type_t sevent);

class og_sysevent_t {
private:
  uint64_t events[NUM_EVENTS];

public:
  og_sysevent_t() {
    reset();
  }
  ~og_sysevent_t() {
  }
  std::string to_string();

  // for recording events
  inline void record(og_sysevent_type_t sevent, uint64_t val) {
    assert(sevent < NUM_EVENTS);
    events[sevent] += val;
  }

  // retrieving events
  inline uint64_t get(og_sysevent_type_t sevent) {
    assert(sevent < NUM_EVENTS);
    return events[sevent];
  }

  // for resetting events
  inline void reset(og_sysevent_type_t sevent = NUM_EVENTS) {
    if (sevent == NUM_EVENTS) {
      for (uint32_t i = 0; i < NUM_EVENTS; ++i) {
        events[i] = 0;
      }
    } else {
      assert(sevent < NUM_EVENTS);
      events[sevent] = 0;
    }
  }

  // for copying events
  og_sysevent_t &operator=(const og_sysevent_t &other) {
    if (this == &other) {
      return *this;
    }
    for (uint32_t i = 0; i < NUM_EVENTS; ++i) {
      events[i] = other.events[i];
    }
    return *this;
  }
};

#endif
