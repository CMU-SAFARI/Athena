#ifndef OFFCHIP_PRED_TLP_H
#define OFFCHIP_PRED_TLP_H

#include "bitmap.h"
#include "offchip_pred_base.h"
#include "perc_pred.h"
#include <deque>
#include <unordered_set>
#include <vector>

using namespace perc;

// Page buffer entry for TLP
class tlp_feature_t : public ocp_base_feature_t {
public:
  state_info_t *info;
  float perc_weight_sum;

  tlp_feature_t() {
    info = NULL;
    perc_weight_sum = 0.0;
  }
  virtual ~tlp_feature_t() // MUST be virtual
  {
    if (info) {
      delete info;
    }
  }
};

class tlp_page_buf_entry_t {
public:
  uint64_t page;
  Bitmap bmp_access;
  uint32_t age;

public:
  tlp_page_buf_entry_t() {
    page = 0;
    bmp_access.reset();
    age = 0;
  }
};

class OffchipPredTLP : public OffchipPredBase {
private:
  // First Level Predictor (FLP) - for off-chip prediction
  perceptron_pred_t *flp_pred;

  // Second Level Predictor (SLP) - for prefetch filtering
  perceptron_pred_t *slp_pred;

  vector<deque<tlp_page_buf_entry_t *>> m_page_buffer;
  vector<deque<tlp_page_buf_entry_t *>> m_page_buffer_pf;
  deque<uint64_t> last_n_load_pcs;

  // Thresholds for FLP
  float tau_1, tau_2;

  // Statistics
  uint64_t true_pos, false_pos, false_neg, true_neg;
  uint64_t true_pos_pf, false_pos_pf, false_neg_pf, true_neg_pf;
  uint64_t miss_hit_l1d, miss_hit_l2c;
  uint64_t train_count;

  // for measuring stats
  unordered_set<uint64_t> unique_pages;
  unordered_set<uint64_t> unique_pages_pf;

  struct {
    struct {
      uint64_t called;
    } train;

    struct {
      uint64_t called;
      uint64_t outcome[2];
    } predict;

    struct {
      uint64_t called;
      uint64_t hit;
      uint64_t eviction;
      uint64_t insertion;
    } page_buf;

    struct {
      uint64_t called;
      uint64_t hit;
      uint64_t eviction;
      uint64_t insertion;
    } page_buf_pf;

    struct {
      uint64_t called;
      uint64_t increment;
      uint64_t decrement;
      float min_observed_thresh;
      float max_observed_thresh;
    } act_thresh_update;

  } stats;

  // Helper functions
  state_info_t *get_state(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
  state_info_t *get_state_on_prefetch(PACKET &pf_packet);

  void lookup_address(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access);
  void lookup_address_on_prefetch(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access);
  uint32_t get_set(uint64_t vpage);
  void get_control_flow_signatures(LSQ_ENTRY *lq_entry, uint64_t &last_n_load_pc_sig, uint64_t &last_n_pc_sig);

  string print_activated_features(vector<int32_t> activated_features);

public:
  OffchipPredTLP(uint32_t cpu, string _type, uint64_t _seed);
  ~OffchipPredTLP();

  void print_config();
  void dump_stats();
  void reset_stats();

  // FLP
  void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
  bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);

  // SLP
  bool predict_on_prefetch(PACKET &pf_packet);
  void train_on_prefetch(PACKET &pf_packet);
};

#endif /* OFFCHIP_PRED_TLP_H */