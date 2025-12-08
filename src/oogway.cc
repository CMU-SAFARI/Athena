#include "oogway.h"
#include "champsim.h" // for warmup_complete
#include "knobs.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string.h>

#if 0
#define LOCKED(...)                                                                                                                                            \
  {                                                                                                                                                            \
    fflush(stdout);                                                                                                                                            \
    __VA_ARGS__;                                                                                                                                               \
    fflush(stdout);                                                                                                                                            \
  }

#define LOGID(cpu) fprintf(stdout, "[CPU %2d | %25s@%3u] ", (int)(cpu), __FUNCTION__, __LINE__)

#define MYLOG(cpu, ...)                                                                                                                                        \
  if (knob::og_debug && warmup_complete[cpu]) {                                                                                                                \
    LOCKED(LOGID(cpu); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)                                                                                   \
  }
#else
#define MYLOG(cpu, ...)                                                                                                                                        \
  {                                                                                                                                                            \
  }
#endif

void Oogway::init_knobs() {
}

void Oogway::init_stats() {
  bzero(&stats, sizeof(stats));
}

//----------------------------------------//
// Init Oogway
//----------------------------------------//
Oogway::Oogway(uint32_t _cpu) : cpu(_cpu) {
  // init the RL engine
  if (!knob::heuristic_enable && !knob::mab_enable) {
    brain = new LearningEngineHashed(NULL, knob::og_alpha, knob::og_gamma, knob::og_epsilon, knob::og_seed, knob::og_policy, knob::og_learning_type,
                                     knob::og_brain_zero_init);
  }
  // init stats
  init_stats();
}

//----------------------------------------//
// Fini Oogway
//----------------------------------------//
Oogway::~Oogway() {
}

//----------------------------------------//
// Print Oogway config
//----------------------------------------//
void Oogway::print_config() {
  cout << "og_instr_epoch_len " << knob::og_instr_epoch_len << endl
       << "og_alpha " << knob::og_alpha << endl
       << "og_gamma " << knob::og_gamma << endl
       << "og_epsilon " << knob::og_epsilon << endl
       << "og_reward_exponent " << array_to_string(knob::og_reward_exponent) << endl
       << "og_feature_mask " << array_to_string(knob::og_feature_mask) << endl
       << "og_seed " << knob::og_seed << endl
       << "og_policy " << knob::og_policy << endl
       << "og_learning_type " << knob::og_learning_type << endl
       << "og_brain_zero_init " << knob::og_brain_zero_init << endl
       << "og_reward_type " << knob::og_reward_type << endl
       << "og_reward_rescale " << knob::og_reward_rescale << endl
       << "og_reward_max" << knob::og_reward_max << endl
       << "og_prefetcher_level " << knob::og_prefetcher_level << endl
       << endl
       << "leh_num_planes " << knob::leh_num_planes << endl
       << "leh_plane_dim " << knob::leh_plane_dim << endl
       << "leh_num_actions " << knob::leh_num_actions << endl
       << "leh_q_init_val " << knob::leh_q_init_val << endl
       << "leh_pooling_type " << knob::leh_pooling_type << endl
       << "leh_plane_hash_types " << array_to_string(knob::leh_plane_hash_types) << endl
       << "leh_tracing_enable " << knob::leh_tracing_enable << endl
       << "leh_trace_basename " << knob::leh_trace_basename << endl
       << "leh_concatenate " << knob::leh_concatenate << endl
       << "mab_enable " << knob::mab_enable << endl
       << "mab_rr_restart_prob " << knob::mab_rr_restart_prob << endl
       << "mab_gamma " << knob::mab_gamma << endl
       << "heuristic_enable " << knob::heuristic_enable << endl
       << "og_l2c_only_coordination " << knob::og_l2c_only_coordination << endl;
  if (knob::heuristic_enable) {
    cout << "heuristic_pref_accuracy " << array_to_string(knob::heuristic_pref_accuracy) << endl;
    cout << "heuristic_ocp_accuracy " << array_to_string(knob::heuristic_ocp_accuracy) << endl;
  }
}

//----------------------------------------//
// Print Oogway stats
//----------------------------------------//
void Oogway::dump_stats() {
  cout << endl;
  cout << "Core_" << cpu << "_og_nr_epochs " << stats.nr_epochs << endl;

  for (uint32_t a = 0; a < 8; ++a) {
    cout << "Core_" << cpu << "_og_action_" << a << " " << stats.action_hist[a] << endl;
  }

  cout << "Core_" << cpu << "_og_pf_throttled " << stats.pf_throttled << endl;
  cout << "Core_" << cpu << "_og_ocp_throttled " << stats.ocp_throttled << endl;

  for (uint32_t a = 0; a < 8; ++a) {
    cout << "Core_" << cpu << "_mab_r_arm_" << a << " " << mab.r_arm[a] << endl;
    cout << "Core_" << cpu << "_mab_n_arm_" << a << " " << mab.n_arm[a] << endl;
  }

  cout << "Core_" << cpu << "_mab_n_total " << mab.n_total << endl;

  for (int action = 0; action < 8; ++action) {
    cout << "Core_" << cpu << "_og_action_" << action << "_reward ";
    for (int metric = 0; metric < 6; ++metric) {
      cout << action_rewards[action][metric] / stats.action_hist[action] << " ";
    }
    cout << endl;
  }
  cout << endl;
  if (!knob::heuristic_enable && !knob::mab_enable) {
    brain->print_q_cube();
    brain->dump_stats();
    brain->summarize_q_cube();
    cout << endl;
  }
}

//----------------------------------------//
// Gets called after an epoch finishes
// First takes action for the next epoch
// Then trains the model
//----------------------------------------//
void Oogway::train_and_take_action(og_state_t *_state) {
  MYLOG(cpu, "============================");

  epoch_count++;
  MYLOG(cpu, "Epoch: %lu, num_retired_insts: %lu", epoch_count, _state->num_retired_insts);

  og_state_t *tmp = NULL;

  // First, record the current state
  curr_state = _state;
  MYLOG(cpu, "Current state: %s", curr_state->to_string().c_str());

  // Second, compute the reward
  float reward = compute_reward();
  MYLOG(cpu, "Reward: %f", reward);

  // Third, make decision for the next epoch
  if (knob::heuristic_enable) { // Heuristic
    curr_action = heuristic_decision(curr_state);
  } else if (knob::mab_enable) { // MAB
    uint32_t max_actions;
    if (knob::og_multi_prefetcher_enable) {
      if (knob::og_l2c_only_coordination) {
        max_actions = 4; // L2C-only coordination uses 4 actions
      } else {
        max_actions = 8; // OCP + 2 prefetchers uses 8 actions
      }
    } else {
      max_actions = 4; // Legacy mode
    }

    if (stats.nr_epochs < max_actions) { // Initial Round Robin Phase
      curr_action = stats.nr_epochs;
    } else {                                       // main loop
      if (drand48() < knob::mab_rr_restart_prob) { // Round robin restarts
        stats.nr_epochs = 0;
      }
      std::vector<float> ducb; // discounted upper confidence bound
      for (uint32_t i = 0; i < max_actions; i++) {
        ducb.push_back(mab.r_arm[i] + 0.04f * sqrt(log(mab.n_total) / mab.n_arm[i]));
      }
      curr_action = std::distance(ducb.begin(), std::max_element(ducb.begin(), ducb.end()));
    }
  } else { // Athena
    curr_action = brain->chooseAction(curr_state);
  }
  MYLOG(cpu, "Action taken: %d", curr_action);

  // Fourth, train the model
  MYLOG(cpu, "Traning model");
  if (prev_state) {
    MYLOG(cpu, "   * Prev state : %s", prev_state->to_string().c_str());
    MYLOG(cpu, "   * Prev action: %d", prev_action);
  }
  MYLOG(cpu, "   * Curr reward: %f", reward);
  MYLOG(cpu, "   * Curr state : %s", curr_state->to_string().c_str());
  MYLOG(cpu, "   * Curr action: %d", curr_action);

  // Learning for MAB and RL

  if (prev_state) {
    if (knob::heuristic_enable) { // Heuristic
      // No learning needed for heuristic
    } else if (knob::mab_enable) {                                                                                                                // MAB
      float mab_reward = prev_event.get(CYCLE) ? static_cast<float>(knob::og_instr_epoch_len) / static_cast<float>(prev_event.get(CYCLE)) : 1.0f; // IPC
      uint32_t max_actions;
      if (knob::og_multi_prefetcher_enable) {
        if (knob::og_l2c_only_coordination) {
          max_actions = 4; // L2C-only coordination uses 4 actions
        } else {
          max_actions = 8; // OCP + 2 prefetchers uses 8 actions
        }
      } else {
        max_actions = 4; // Legacy mode
      }

      if (stats.nr_epochs < max_actions) { // Initial Round Robin Phase
        mab.r_arm[stats.nr_epochs] = reward;
      } else {

        // updSels
        for (int i = 0; i < max_actions; i++) {
          mab.n_arm[i] *= knob::mab_gamma;
          if (i == curr_action) {
            mab.n_arm[i] += 1;
          }
        }
        mab.n_total = knob::mab_gamma * mab.n_total + 1;

        // main loop
        mab.r_arm[prev_action] = (mab.r_arm[prev_action] * (mab.n_arm[prev_action] - 1) + mab_reward) / mab.n_arm[prev_action];
      }
      MYLOG(cpu, "   * Arm reward: %f, %f, %f, %f", mab.r_arm[0], mab.r_arm[1], mab.r_arm[2], mab.r_arm[3]);
    } else { // Athena
      brain->learn(prev_state, prev_action, reward, curr_state, curr_action);
      // brain->print_q_cube();

      // std::cout<<"CPU: " << cpu << " ";
      // brain->summarize_q_cube();
    }
  }
  // Fourth, throttle mechanisms based on action
  stats.action_hist[curr_action]++;

  if (knob::og_multi_prefetcher_enable) {
    if (knob::og_l2c_only_coordination) {
      // 4-action space for L2C-only coordination (no OCP)
      switch (curr_action) {
      case 0: // 00 - Neither L2C prefetcher
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, false);
        // set_ocp_enabled(false);
        break;
      case 1: // 01 - Only L2C prefetcher 1
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, true);
        // set_ocp_enabled(false);
        break;
      case 2: // 10 - Only L2C prefetcher 0
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, false);
        // set_ocp_enabled(false);
        break;
      case 3: // 11 - Both L2C prefetchers
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, true);
        // set_ocp_enabled(false);
        break;
      }
    } else {
      // 8-action space for multi-prefetcher support with OCP
      switch (curr_action) {
      case 0: // 000
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, false);
        set_ocp_enabled(false);
        break;
      case 1: // 001
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, false);
        set_ocp_enabled(true);
        break;
      case 2: // 010
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, true);
        set_ocp_enabled(false);
        break;
      case 3: // 011
        set_prefetch_enabled(0, false);
        set_prefetch_enabled(1, true);
        set_ocp_enabled(true);
        break;
      case 4: // 100
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, false);
        set_ocp_enabled(false);
        break;
      case 5: // 101
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, false);
        set_ocp_enabled(true);
        break;
      case 6: // 110
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, true);
        set_ocp_enabled(false);
        break;
      case 7: // 111
        set_prefetch_enabled(0, true);
        set_prefetch_enabled(1, true);
        set_ocp_enabled(true);
        break;
      }
    }
  } else {
    // 4-action space for backward compatibility
    switch (curr_action) {
    case 0: // Only OCP
      set_prefetch_enabled(false);
      set_ocp_enabled(true);
      break;
    case 1: // Neither
      set_prefetch_enabled(false);
      set_ocp_enabled(false);
      break;
    case 2: // Only prefetch
      set_prefetch_enabled(true);
      set_ocp_enabled(false);
      break;
    case 3: // Both
      set_prefetch_enabled(true);
      set_ocp_enabled(true);
      break;
    }
  }

  // Fifth, prepare to transition to next epoch
  tmp = prev_state;
  prev_state = curr_state;
  prev_action = curr_action;
  instr_count = 0;
  cycle_count = 0;
  delete (tmp); // at this point, this is not needed anymore
  MYLOG(cpu, "============================");
}

bool Oogway::check_inst_epoch_end() {
  return instr_count == knob::og_instr_epoch_len;
}

bool Oogway::check_cycle_epoch_end() {
  return cycle_count == knob::og_cycle_epoch_len;
}

//----------------------------------------//
// Gets called at the end of every epoch
// Checks events in the current and
// previous epochs and calculate reward.
//----------------------------------------//
float Oogway::compute_reward() {

  // RBERA: during warmup there won't be any LLC miss latency!
  // No point considering this metric during warmup
  float avg_llc_miss_lat_curr =
      curr_event.get(LLC_LOAD_MISS) ? (float)curr_event.get(LLC_LOAD_MISS_LAT) / curr_event.get(LLC_LOAD_MISS) : 0.1 /* small default value */;
  float avg_llc_miss_lat_prev =
      prev_event.get(LLC_LOAD_MISS) ? (float)prev_event.get(LLC_LOAD_MISS_LAT) / prev_event.get(LLC_LOAD_MISS) : 0.1 /* small default value */;

  /* Reward that is invariant to Oogway's decision */

  assert(knob::og_reward_exponent.size() == 6 && "og_reward_exponent size mismatch");

  float R, R_UNCORR, R_CORR;
  if (knob::og_reward_type == 0) { // Product
    R = 1.0;
    R_UNCORR = 1.0;
    R_CORR = 1.0;
    R_UNCORR *= pow((float)(curr_event.get(MISPRED_BRANCH) + 1) / (prev_event.get(MISPRED_BRANCH) + 1), knob::og_reward_exponent[0]);
    R_UNCORR *= pow((float)(curr_event.get(LOAD_INST_ISSUE) + 1) / (prev_event.get(LOAD_INST_ISSUE) + 1), knob::og_reward_exponent[1]);
    R_UNCORR *= pow((float)(curr_event.get(L1D_LOAD_MISS) + 1) / (prev_event.get(L1D_LOAD_MISS) + 1), knob::og_reward_exponent[2]);

    /* Reward that is dependent on Oogway's decision */
    R_CORR *= pow((float)(prev_event.get(CYCLE) + 1) / (curr_event.get(CYCLE) + 1), knob::og_reward_exponent[3]);
    R_CORR *= pow((float)(prev_event.get(LLC_LOAD_MISS) + 1) / (curr_event.get(LLC_LOAD_MISS) + 1), knob::og_reward_exponent[4]);
    // Dont consider this metric during warmup
    R_CORR *= warmup_complete[cpu] ? pow((avg_llc_miss_lat_prev + 1.0f) / (avg_llc_miss_lat_curr + 1.0f), knob::og_reward_exponent[5]) : 1.0;

    /* Final reward */
    R = R_CORR * R_UNCORR;
  } else if (knob::og_reward_type == 1) { // Sum
    R = 0.0;
    R_UNCORR = 0.0;
    R_CORR = 0.0;

    R_UNCORR += compute_relative_improvement(prev_event.get(MISPRED_BRANCH), curr_event.get(MISPRED_BRANCH), knob::og_reward_exponent[0]);
    action_rewards[prev_action][0] += compute_relative_improvement(prev_event.get(MISPRED_BRANCH), curr_event.get(MISPRED_BRANCH), 1.0);
    R_UNCORR += compute_relative_improvement(prev_event.get(LOAD_INST_ISSUE), curr_event.get(LOAD_INST_ISSUE), knob::og_reward_exponent[1]);
    action_rewards[prev_action][1] += compute_relative_improvement(prev_event.get(LOAD_INST_ISSUE), curr_event.get(LOAD_INST_ISSUE), 1.0);
    R_UNCORR += compute_relative_improvement(prev_event.get(L1D_LOAD_MISS), curr_event.get(L1D_LOAD_MISS), knob::og_reward_exponent[2]);
    action_rewards[prev_action][2] += compute_relative_improvement(prev_event.get(L1D_LOAD_MISS), curr_event.get(L1D_LOAD_MISS), 1.0);

    R_CORR += compute_relative_improvement(prev_event.get(CYCLE), curr_event.get(CYCLE), knob::og_reward_exponent[3]);
    action_rewards[prev_action][3] += compute_relative_improvement(prev_event.get(CYCLE), curr_event.get(CYCLE), 1.0);
    R_CORR += compute_relative_improvement(prev_event.get(LLC_LOAD_MISS), curr_event.get(LLC_LOAD_MISS), knob::og_reward_exponent[4]);
    action_rewards[prev_action][4] += compute_relative_improvement(prev_event.get(LLC_LOAD_MISS), curr_event.get(LLC_LOAD_MISS), 1.0);
    // Dont consider this metric during warmup
    R_CORR += warmup_complete[cpu] ? compute_relative_improvement(avg_llc_miss_lat_prev, avg_llc_miss_lat_curr, knob::og_reward_exponent[5]) : 0.0;
    action_rewards[prev_action][5] += warmup_complete[cpu] ? compute_relative_improvement(avg_llc_miss_lat_prev, avg_llc_miss_lat_curr, 1.0) : 0.0;

    R = R_CORR - R_UNCORR;

    // tracking the reward
    // action_rewards[prev_action][0] += R_UNCORR;
    // action_rewards[prev_action][1] += R_CORR;
    // action_rewards[prev_action][2] += R;
  }
  // R = std::min(knob::og_reward_max, R);

  // float R_mapped = 0.0;
  // if (R > 20.0) {
  //   R_mapped = 20.0;
  // } else if (R >= 1.0) {
  //   R_mapped = R;
  // } else if (R < 1.0) {
  //   R_mapped = -10.0 + R * 10.0;
  // }
  // R = std::min(20.0f, R);

  // if (std::isnan(R_mapped) || R_mapped < -10.0) {
  //   cout << "Debug Info: R_mapped out of range or NaN" << endl;
  //   cout << "R_CORR: " << R_CORR << ", R_UNCORR: " << R_UNCORR << ", R: " << R << endl;
  //   cout << "Prev_epoch: " << prev_event.to_string() << endl;
  //   cout << "Curr_epoch: " << curr_event.to_string() << endl;
  //   R_mapped = 0.0;
  // }

  MYLOG(cpu, "Prev_epoch: %s", prev_event.to_string().c_str());
  MYLOG(cpu, "Curr_epoch: %s", curr_event.to_string().c_str());
  MYLOG(cpu, "R_CORR: %f, R_UNCORR: %f, R: %f", R_CORR, R_UNCORR, R);

  /* prepare for the next epoch */
  prev_event = curr_event;
  curr_event.reset();

  return R;
}

//----------------------------------------//
// Heuristic decision function
// Based on bandwidth levels and accuracy thresholds
//----------------------------------------//
uint32_t Oogway::heuristic_decision(og_state_t *state) {
  MYLOG(cpu, "Heuristic decision for state: %s", state->to_string().c_str());

  // Determine bandwidth level (0-3) based on overall bandwidth
  uint32_t bw_level = 0;

  if (state->overall_bw <= 25) {
    bw_level = 0;
  } else if (state->overall_bw <= 50) {
    bw_level = 1;
  } else if (state->overall_bw <= 75) {
    bw_level = 2;
  } else {
    bw_level = 3;
  }

  MYLOG(cpu, "Bandwidth level: %u (overall_bw: %u)", bw_level, state->overall_bw);

  // Get accuracy thresholds for this bandwidth level
  uint32_t pref_threshold = 0;
  uint32_t ocp_threshold = 0;

  if (bw_level < knob::heuristic_pref_accuracy.size()) {
    pref_threshold = knob::heuristic_pref_accuracy[bw_level];
  }

  if (bw_level < knob::heuristic_ocp_accuracy.size()) {
    ocp_threshold = knob::heuristic_ocp_accuracy[bw_level];
  }

  MYLOG(cpu, "Accuracy thresholds - Pref: %u, OCP: %u", pref_threshold, ocp_threshold);
  MYLOG(cpu, "Current accuracies - Pref: %u, OCP: %u", state->pref_acc[0], state->ocp_acc);

  // Decision logic based on accuracy thresholds
  bool enable_prefetch_0 = (state->pref_acc[0] >= pref_threshold);
  bool enable_prefetch_1 = (state->pref_acc[1] >= pref_threshold);
  bool enable_ocp = (state->ocp_acc >= ocp_threshold);

  // For L2C-only coordination, ignore OCP accuracy
  if (knob::og_multi_prefetcher_enable && knob::og_l2c_only_coordination) {
    enable_ocp = false;
  }

  uint32_t action;
  if (knob::og_multi_prefetcher_enable) {
    if (knob::og_l2c_only_coordination) {
      // 4-action space for L2C-only coordination (no OCP)
      action = (enable_prefetch_0 ? 2 : 0) + (enable_prefetch_1 ? 1 : 0);
    } else {
      // 8-action space for multi-prefetcher support with OCP
      action = (enable_prefetch_0 ? 4 : 0) + (enable_prefetch_1 ? 2 : 0) + (enable_ocp ? 1 : 0);
    }
  } else {
    bool enable_prefetch = enable_prefetch_0;
    if (enable_prefetch && enable_ocp) {
      action = 3; // Both
    } else if (enable_prefetch) {
      action = 2; // Only prefetch
    } else if (enable_ocp) {
      action = 0; // Only OCP
    } else {
      action = 1; // Neither
    }
  }

  MYLOG(cpu, "Heuristic decision: %u (pref: %s, ocp: %s)", action, enable_prefetch_0 ? "true" : "false", enable_ocp ? "true" : "false");

  return action;
}
