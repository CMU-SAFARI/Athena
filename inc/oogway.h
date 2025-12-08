#ifndef OOGWAY_H
#define OOGWAY_H

#include "learning_engine_hashed.h"
#include "oogway_helper.h"
#include "util.h"
#include <assert.h>

//-----------------------------------------------------------------//
// Named after the all-knowing old turtle.
// "You just need to beleive. Promise me Shifu, you will beleive!"
// - Grand Master Oogway
//----------------------------------------------------------------//
class Oogway {
private:
  uint32_t cpu = 0;
  uint64_t instr_count = 0;
  uint64_t cycle_count = 0;
  uint64_t epoch_count = 0;

  //----------------------------------------------------------//
  // Consider this timeline:
  //
  // |----E0----|----E1----|----E2----|----E3----|
  //         (S0,A0)    (S1,A1)    (S2,A2)
  //                                  ^
  //                             We are here
  //
  // System has just completed epoch E2 and transitioning into E3.
  // At this point, you would know E2's state.
  // curr_state   => S2, i.e., E2's state
  // curr_action  => A2, i.e., Action taken at the end of E2
  // prev_state   => S1, i.e., E1's state
  // prev_action  => A1, i.e., Action taken at the end of E1
  //----------------------------------------------------------/

  og_state_t *prev_state = NULL;
  uint32_t prev_action = 0;
  og_state_t *curr_state = NULL;
  uint32_t curr_action = 0;

  LearningEngineHashed *brain = NULL; // The RL engine

  // events
  og_sysevent_t prev_event, curr_event;

  // stats
  struct {
    uint64_t nr_epochs;
    uint64_t action_hist[8];
    uint64_t pf_throttled;
    uint64_t ocp_throttled;
  } stats;

  // Variables for actions
  bool prefetch_enabled[2] = {false, false};
  bool ocp_enabled = false;

  // MAB
  struct {
    float r_arm[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float n_arm[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    float n_total = 0.0f;
  } mab;

  // Reward tracking for each action
  std::vector<std::vector<float>> action_rewards = {
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
  };

  void init_knobs();
  void init_stats();

  //-----------------------------------//
  // Functions to maintain RL agent
  //-----------------------------------//
  float compute_reward();

  //-----------------------------------//
  // Heuristic decision function
  //-----------------------------------//
  uint32_t heuristic_decision(og_state_t *state);

public:
  Oogway(uint32_t cpu);
  ~Oogway();
  void dump_stats();
  void print_config();

  bool check_inst_epoch_end();
  bool check_cycle_epoch_end();
  void train_and_take_action(og_state_t *curr_state);
  inline void incr_inst() {
    instr_count++;
  }
  inline void incr_pf_throttled() {
    stats.pf_throttled++;
  }
  inline void incr_ocp_throttled() {
    stats.ocp_throttled++;
  }
  inline void incr_nr_epochs() {
    stats.nr_epochs++;
  }
  inline void record_sys_event(og_sysevent_type_t sevent, uint64_t val, uint32_t _cpu) {
    assert(_cpu == cpu);
    curr_event.record(sevent, val);
  }
  float compute_relative_improvement(float prev, float curr, float lambda) {
    return (prev > 0.1f) ? (prev - curr) / prev * lambda : 0.0f;
  }
  inline uint64_t get_epoch_num() {
    return epoch_count;
  }

  // Getter and setter for action variables
  inline bool is_prefetch_enabled() const {
    return prefetch_enabled[0];
  }
  inline bool is_prefetch_enabled(uint32_t id) const {
    if (id < 2) {
      return prefetch_enabled[id];
    }
    return false;
  }
  inline bool is_ocp_enabled() const {
    return ocp_enabled;
  }
  inline void set_prefetch_enabled(bool value) {
    prefetch_enabled[0] = value;
  }
  inline void set_prefetch_enabled(uint32_t id, bool value) {
    if (id < 2) {
      prefetch_enabled[id] = value;
    }
  }
  inline void set_ocp_enabled(bool value) {
    ocp_enabled = value;
  }
};

#endif /* OOGWAY_H */
