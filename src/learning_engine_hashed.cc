#include "learning_engine_hashed.h"
#include "knobs.h"
#include "util.h"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <string.h>

#if 0
#define LOCKED(...)                                                                                                                                            \
  {                                                                                                                                                            \
    fflush(stdout);                                                                                                                                            \
    __VA_ARGS__;                                                                                                                                               \
    fflush(stdout);                                                                                                                                            \
  }

#define LOGID(cpu) fprintf(stdout, "[CPU %2d | %25s@%3u] ", (int)(cpu), __FUNCTION__, __LINE__);

#define MYLOG(cpu, ...) LOCKED(LOGID(cpu); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#define MYLOG(...)                                                                                                                                             \
  {                                                                                                                                                            \
  }
#endif

void LearningEngineHashed::init_knobs() {
  assert(knob::leh_plane_hash_types.size() == knob::leh_num_planes);
}

void LearningEngineHashed::init_stats() {
  /* init stats */
  bzero(&stats, sizeof(stats));
}

//----------------------------------------//
// Init engine
//----------------------------------------//
LearningEngineHashed::LearningEngineHashed(Prefetcher *parent, float alpha, float gamma, float epsilon, uint64_t seed, std::string policy, std::string type,
                                           bool zero_init)
    : LearningEngineBase(parent, alpha, gamma, epsilon, 0 /*dummy action value*/, 0 /*dummy state value*/, seed, policy, type) {
  m_actions = knob::leh_num_actions;
  float init_q_val = zero_init ? 0.0 : 1 / std::max(0.1f, (1.0f - gamma));

  /* initialize q_cube */
  q_cube = (float ***)calloc(knob::leh_num_planes, sizeof(float **));
  assert(q_cube);
  for (uint32_t plane = 0; plane < knob::leh_num_planes; ++plane) {
    q_cube[plane] = (float **)calloc(knob::leh_plane_dim, sizeof(float *));
    assert(q_cube[plane]);
    for (uint32_t dim = 0; dim < knob::leh_plane_dim; ++dim) {
      q_cube[plane][dim] = (float *)calloc(m_actions, sizeof(float));
      assert(q_cube[plane][dim]);
      for (uint32_t act = 0; act < m_actions; ++act) {
        q_cube[plane][dim][act] = init_q_val;
      }
    }
  }

  /* init random generators */
  m_generator.seed(m_seed);
  m_explore = new std::bernoulli_distribution(epsilon);
  m_actiongen = new std::uniform_int_distribution<int>(0, m_actions - 1);

  /* init stats */
  init_stats();
  init_knobs();
  /* init tracing, if any */
  if (knob::leh_tracing_enable) {
    engine_tracer = new LETracer(knob::leh_trace_basename);
    engine_tracer->enable();
  }

  // Print the q_cube
  print_q_cube();
}

//----------------------------------------//
// Fini engine
//----------------------------------------//
LearningEngineHashed::~LearningEngineHashed() {
}

//----------------------------------------//
// Inference (picks the action)
//----------------------------------------//
uint32_t LearningEngineHashed::chooseAction(og_state_t *state) {
  uint32_t action = 0;

  stats.action.called++;

  if (m_type == LearningType::SARSA && m_policy == Policy::EGreedy) {
    if ((*m_explore)(m_generator)) {        // random exploration
      action = (*m_actiongen)(m_generator); // take random action
      stats.action.explore++;
      stats.action.dist[action][0]++;
      MYLOG(cpu, "action taken %u explore, state %s", action, state->to_string().c_str());
    } else {
      float max_q = std::numeric_limits<float>::min();
      action = getMaxAction(state, max_q);
      // if (max_q < 0.0) {
      //   action = 0;
      // }
      stats.action.exploit++;
      stats.action.dist[action][1]++;
      MYLOG(cpu, "action taken %u exploit, state %s, q-val %f", action, state->to_string().c_str(), max_q);
    }
  } else {
    printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
    assert(false);
    action = 0;
  }

  return action;
}

//----------------------------------------//
// Training the agent
//----------------------------------------//
void LearningEngineHashed::learn(og_state_t *state1, uint32_t action1, float reward, og_state_t *state2, uint32_t action2) {
  uint32_t plane = 0;
  float q_sa1_partial = 0.0;
  float q_sa2_partial = 0.0;

  stats.learn.called++;

  if (m_type == LearningType::SARSA && m_policy == Policy::EGreedy) {
    for (plane = 0; plane < knob::leh_num_planes; ++plane) {
      q_sa1_partial = getPartialQ(plane, state1, action1);
      q_sa2_partial = getPartialQ(plane, state2, action2);

      /* S-A-R-S-A */
      q_sa1_partial = q_sa1_partial + m_alpha * ((float)reward + m_gamma * q_sa2_partial - q_sa1_partial);

      assert(!std::isnan(q_sa1_partial));
      // update partial Q
      setPartialQ(plane, state1, action1, q_sa1_partial);
    }

    // tracing
    if (knob::leh_tracing_enable) {
      engine_tracer->trace(state1, action1, reward);
    }
  } else {
    printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
    assert(false);
  }
}

void LearningEngineHashed::dump_stats() {

  cout << "leh_action_called " << stats.action.called << endl
       << "leh_action_explore " << stats.action.explore << endl
       << "leh_action_exploit " << stats.action.exploit << endl
       << "leh_action_tie " << stats.action.tie << endl;
  for (uint32_t act = 0; act < m_actions; ++act) {
    cout << "leh_action_" << act << "_explored " << stats.action.dist[act][0] << endl
         << "leh_action_" << act << "_exploited " << stats.action.dist[act][1] << endl;
  }
}

//----------------------------------------------//
// Retrieving action with the highest Q-value
// Inputs:
//   1. state
// Returns:
//   1. action ID with max_q_value
//   2. max_q_value
//----------------------------------------------//
uint32_t LearningEngineHashed::getMaxAction(og_state_t *state, float &max_q) {
  float act_q;
  uint32_t act_max_q = 0;

  // // max_q = std::numeric_limits<float>::min();

  std::vector<float> qs(m_actions, 0.0f);
  for (uint32_t act = 0; act < m_actions; ++act) {
    qs[act] = getQ(state, act);
  }

  max_q = *std::max_element(qs.begin(), qs.end());
  std::vector<uint32_t> candidates;

  // Collect actions that are within `tolerance` of `max_q`
  for (uint32_t act = 0; act < qs.size(); ++act) {
    if (max_q - qs[act] < 0.1f) {
      candidates.push_back(act);
    }
  }

  // If multiple candidates exist, pick randomly
  if (candidates.size() > 1) {
    std::uniform_int_distribution<uint32_t> dist(0, candidates.size() - 1);
    stats.action.tie++;
    return candidates[dist(m_generator)];
  }

  // Otherwise, return the single action with max_q
  return std::distance(qs.begin(), std::max_element(qs.begin(), qs.end()));

  // max_q = std::numeric_limits<float>::min();
  // for(uint32_t act = 0; act < m_actions; ++act) {
  //   act_q = getQ(state, act);
  //   if(act_q > max_q) {
  //     act_max_q = act;
  //     max_q     = act_q;
  //   }
  // }

  // return act_max_q;
}

//----------------------------------------------//
// Retreives the Q value of
// the corresponding <state,action> pair
//----------------------------------------------//
float LearningEngineHashed::getQ(og_state_t *state, uint32_t act) {
  uint32_t plane;
  std::vector<float> partial_qs;
  float final_q = 0.0;

  // retrieve partial Q values from each plane
  for (plane = 0; plane < knob::leh_num_planes; ++plane) {
    partial_qs.push_back(getPartialQ(plane, state, act));
  }

  // do pooling of partial Q-values
  if (knob::leh_pooling_type == 0) { // sum pooling
    final_q = std::accumulate(partial_qs.begin(), partial_qs.end(), 0.0);
  } else if (knob::leh_pooling_type == 1) { // max pooling
    final_q = *std::max_element(partial_qs.begin(), partial_qs.end());
  }

  return final_q;
}

//----------------------------------------------//
// Retreives the Q value for
// the corresponding <plane,state,action>
//----------------------------------------------//
float LearningEngineHashed::getPartialQ(uint32_t plane, og_state_t *state, uint32_t act) {

  // uint64_t state_raw   = state->get_raw64();
  // uint32_t plane_index = compute_hash(plane, state_raw);

  if (knob::leh_concatenate) {
    uint32_t state_raw = state->get_raw32();
    uint32_t plane_index = compute_hash_32(plane, state_raw);

    assert(plane < knob::leh_num_planes);
    assert(plane_index < knob::leh_plane_dim);
    assert(act < m_actions);
    return q_cube[plane][plane_index][act];
  } else {
    // uint32_t plane_index = 0;
    // switch(plane)
    // {
    //   case 0:
    //     plane_index = state->pref_acc * knob::leh_plane_dim / 100;
    //     break;
    //   case 1:
    //     plane_index = state->ocp_acc * knob::leh_plane_dim / 100;
    //     break;

    //   case 2:
    //     uint32_t overall_bw_max = 100;
    //     uint32_t overall_bw = state->overall_bw;
    //     if (overall_bw > overall_bw_max) {
    //       overall_bw = overall_bw_max;
    //     }
    //     return overall_bw * knob::leh_plane_dim / overall_bw_max;
    //   case 3:
    //     return state->pref_bw;
    //   case 4:
    //     return state->ocp_bw;
    //   case 5:
    //     return state->bw_needed;
    //   case 6:
    //     uint32_t pref_pollution_max = 255;
    //     uint32_t pref_pollution = state->pref_pollution;
    //     if (pref_pollution > pref_pollution_max) {
    //       pref_pollution = pref_pollution_max;
    //     }
    //     return pref_pollution * knob::leh_plane_dim / pref_pollution_max;
    //     plane_index  = state->pref_pollution;
    //   case 7:

    //   case 8:
    //   default:
    //     assert(false);
    // }
    return 0;
  }
}

//----------------------------------------------//
// Computes the hash for the plane
//----------------------------------------------//
uint32_t LearningEngineHashed::compute_hash(uint32_t plane, uint64_t state_raw) {
  assert(plane < knob::leh_plane_hash_types.size());
  uint32_t folded_raw = folded_xor(state_raw, 2);
  uint32_t hashed_raw = HashZoo::getHash(knob::leh_plane_hash_types[plane], folded_raw);
  return hashed_raw % knob::leh_plane_dim;
}

uint32_t LearningEngineHashed::compute_hash_32(uint32_t plane, uint32_t state_raw) {
  uint32_t hashed_raw = HashZoo::getHash(knob::leh_plane_hash_types[plane], state_raw);

  // MYLOG("plane %u state_raw 0x%x hashed_raw 0x%x reminder %u", plane, state_raw, hashed_raw, hashed_raw % knob::leh_plane_dim);
  return hashed_raw % knob::leh_plane_dim;
}

//----------------------------------------------//
// Sets the Q value for
// the corresponding <plane,state,action>
//----------------------------------------------//
void LearningEngineHashed::setPartialQ(uint32_t plane, og_state_t *state, uint32_t act, float new_q) {

  // uint64_t state_raw   = state->get_raw64();
  // uint32_t plane_index = compute_hash(plane, state_raw);

  uint32_t state_raw = state->get_raw32();
  uint32_t plane_index = compute_hash_32(plane, state_raw);
  assert(plane < knob::leh_num_planes);
  assert(plane_index < knob::leh_plane_dim);
  assert(act < m_actions);
  assert(!std::isnan(new_q));

  q_cube[plane][plane_index][act] = new_q; // update Q
}

void LearningEngineHashed::print_q_cube() const {
  for (uint32_t dim = 0; dim < knob::leh_plane_dim; ++dim) {
    std::cout << "Dim " << dim << ": ";
    for (uint32_t plane = 0; plane < knob::leh_num_planes; ++plane) {
      for (uint32_t act = 0; act < m_actions; ++act) {
        std::cout << std::fixed << std::setprecision(1) << q_cube[plane][dim][act] << " ";
      }
      std::cout << "| ";
    }
    std::cout << "\n";
  }
}

void LearningEngineHashed::summarize_q_cube() const {

  // print columnwise average
  std::vector<float> column_sums(m_actions, 0.0f);
  for (uint32_t plane = 0; plane < knob::leh_num_planes; ++plane) {
    for (uint32_t dim = 0; dim < knob::leh_plane_dim; ++dim) {
      for (uint32_t act = 0; act < m_actions; ++act) {
        column_sums[act] += q_cube[plane][dim][act];
      }
    }
  }

  std::cout << "Columnwise averages: ";
  for (uint32_t act = 0; act < m_actions; ++act) {
    std::cout << std::fixed << std::setprecision(5) << column_sums[act] / (knob::leh_num_planes * knob::leh_plane_dim) << " ";
  }
  std::cout << "\n";
}