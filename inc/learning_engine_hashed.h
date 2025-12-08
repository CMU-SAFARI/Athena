#ifndef LEARNING_ENGINE_HASHED_H
#define LEARNING_ENGINE_HASHED_H

#include "le_tracer.h"
#include "learning_engine_base.h"
#include "oogway_helper.h"
#include <random>
#include <vector>

class LearningEngineHashed : public LearningEngineBase {
private:
  float ***q_cube; // that's the main q-value cube

  std::default_random_engine m_generator;
  std::bernoulli_distribution *m_explore;
  std::uniform_int_distribution<int> *m_actiongen;

  // infra for RL engine tracing
  LETracer *engine_tracer = NULL;

  /* stats */
  struct {
    struct {
      uint64_t called;
      uint64_t explore;
      uint64_t exploit;
      uint64_t tie;
      uint64_t dist[MAX_ACTIONS][2]; /* 0:explored, 1:exploited */
    } action;

    struct {
      uint64_t called;
    } learn;

    struct {
      uint64_t total;
    } consensus;

  } stats;

private:
  void init_knobs();
  void init_stats();
  uint32_t getMaxAction(og_state_t *state, float &max_q);
  float getQ(og_state_t *state, uint32_t act);
  float getPartialQ(uint32_t plane, og_state_t *state, uint32_t act);
  void setPartialQ(uint32_t plane, og_state_t *state, uint32_t act, float q);
  uint32_t compute_hash(uint32_t plane, uint64_t state_raw);
  uint32_t compute_hash_32(uint32_t plane, uint32_t state_raw);

public:
  LearningEngineHashed(Prefetcher *p, float alpha, float gamma, float epsilon, uint64_t seed, std::string policy, std::string type, bool zero_init);
  ~LearningEngineHashed();
  uint32_t chooseAction(og_state_t *state);
  void learn(og_state_t *state1, uint32_t action1, float reward, og_state_t *state2, uint32_t action2);
  void dump_stats();
  void print_q_cube() const;
  void summarize_q_cube() const;
};

#endif /* LEARNING_ENGINE_HASHED_H */
