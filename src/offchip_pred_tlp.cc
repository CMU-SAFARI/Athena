#include "offchip_pred_tlp.h"
#include "knobs.h"
#include "ooo_cpu.h"
#include "util.h"
#include <algorithm>
#include <iostream>

#if 0
#define MYLOG(cond, ...)                                                                                                                                       \
  if (cond) {                                                                                                                                                  \
    fprintf(stdout, "[%25s@%3u] ", __FUNCTION__, __LINE__);                                                                                                    \
    fprintf(stdout, __VA_ARGS__);                                                                                                                              \
    fprintf(stdout, "\n");                                                                                                                                     \
    fflush(stdout);                                                                                                                                            \
  }
#else
#define MYLOG(cond, ...)                                                                                                                                       \
  {                                                                                                                                                            \
  }
#endif

void OffchipPredTLP::print_config() {
  cout << "tlp_flp_activated_features " << print_activated_features(knob::tlp_flp_activated_features) << endl
       << "tlp_flp_weight_array_sizes " << array_to_string(knob::tlp_flp_weight_array_sizes) << endl
       << "tlp_flp_feature_hash_types " << array_to_string(knob::tlp_flp_feature_hash_types) << endl
       << "tlp_flp_activation_threshold " << knob::tlp_flp_activation_threshold << endl
       << "tlp_flp_max_weight " << knob::tlp_flp_max_weight << endl
       << "tlp_flp_min_weight " << knob::tlp_flp_min_weight << endl
       << "tlp_flp_pos_weight_delta " << knob::tlp_flp_pos_weight_delta << endl
       << "tlp_flp_neg_weight_delta " << knob::tlp_flp_neg_weight_delta << endl
       << "tlp_flp_pos_train_thresh " << knob::tlp_flp_pos_train_thresh << endl
       << "tlp_flp_neg_train_thresh " << knob::tlp_flp_neg_train_thresh << endl
       << "tlp_flp_page_buf_sets " << knob::tlp_flp_page_buf_sets << endl
       << "tlp_flp_page_buf_assoc " << knob::tlp_flp_page_buf_assoc << endl
       << "tlp_flp_last_n_load_pcs " << knob::tlp_flp_last_n_load_pcs << endl
       << "tlp_flp_last_n_pcs " << knob::tlp_flp_last_n_pcs << endl

       << "tlp_slp_activated_features " << print_activated_features(knob::tlp_slp_activated_features) << endl
       << "tlp_slp_weight_array_sizes " << array_to_string(knob::tlp_slp_weight_array_sizes) << endl
       << "tlp_slp_feature_hash_types " << array_to_string(knob::tlp_slp_feature_hash_types) << endl
       << "tlp_slp_activation_threshold " << knob::tlp_slp_activation_threshold << endl
       << "tlp_slp_max_weight " << knob::tlp_slp_max_weight << endl
       << "tlp_slp_min_weight " << knob::tlp_slp_min_weight << endl
       << "tlp_slp_pos_weight_delta " << knob::tlp_slp_pos_weight_delta << endl
       << "tlp_slp_neg_weight_delta " << knob::tlp_slp_neg_weight_delta << endl
       << "tlp_slp_pos_train_thresh " << knob::tlp_slp_pos_train_thresh << endl
       << "tlp_slp_neg_train_thresh " << knob::tlp_slp_neg_train_thresh << endl
       << "tlp_slp_page_buf_sets " << knob::tlp_slp_page_buf_sets << endl
       << "tlp_slp_page_buf_assoc " << knob::tlp_slp_page_buf_assoc << endl
       << "tlp_slp_last_n_load_pcs " << knob::tlp_slp_last_n_load_pcs << endl
       << "tlp_slp_last_n_pcs " << knob::tlp_slp_last_n_pcs << endl

       << "tlp_tau_1 " << knob::tlp_tau_1 << endl
       << "tlp_tau_2 " << knob::tlp_tau_2 << endl;
}
void OffchipPredTLP::dump_stats() {
  cout << "tlp_train_called " << stats.train.called << endl
       << "tlp_predict_called " << stats.predict.called << endl
       << "tlp_predict_offchip " << stats.predict.outcome[1] << endl
       << "tlp_predict_not_offchip " << stats.predict.outcome[0] << endl
       << endl
       << "tlp_unique_pages " << unique_pages.size() << endl
       << "tlp_page_buf_lookup_called " << stats.page_buf.called << endl
       << "tlp_page_buf_lookup_hit " << stats.page_buf.hit << endl
       << "tlp_page_buf_lookup_eviction " << stats.page_buf.eviction << endl
       << "tlp_page_buf_lookup_insertion " << stats.page_buf.insertion << endl
       << endl
       << "tlp_act_thresh_update_called " << stats.act_thresh_update.called << endl
       << "tlp_act_thresh_update_increment " << stats.act_thresh_update.increment << endl
       << "tlp_act_thresh_update_decrement " << stats.act_thresh_update.decrement << endl
       << "tlp_act_thresh_update_max_observed_thresh " << stats.act_thresh_update.max_observed_thresh << endl
       << "tlp_act_thresh_update_min_observed_thresh " << stats.act_thresh_update.min_observed_thresh << endl
       << endl
       << "tlp_flp_true_pos " << true_pos << endl
       << "tlp_flp_false_pos " << false_pos << endl
       << "tlp_flp_false_neg " << false_neg << endl
       << "tlp_flp_true_neg " << true_neg << endl
       << endl
       << "tlp_slp_true_pos_pf " << true_pos_pf << endl
       << "tlp_slp_false_pos_pf " << false_pos_pf << endl
       << "tlp_slp_false_neg_pf " << false_neg_pf << endl
       << "tlp_slp_true_neg_pf " << true_neg_pf << endl
       << endl;

  cout << "FLP stats: " << endl;
  flp_pred->dump_stats();
  cout << "SLP stats: " << endl;
  slp_pred->dump_stats();
}

void OffchipPredTLP::reset_stats() {
  bzero(&stats, sizeof(stats));
  stats.act_thresh_update.max_observed_thresh = -999999999;
  stats.act_thresh_update.min_observed_thresh = 999999999;
  flp_pred->reset_stats();
  slp_pred->reset_stats();
}

OffchipPredTLP::OffchipPredTLP(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed) {
  flp_pred = new perceptron_pred_t(knob::tlp_flp_activated_features, knob::tlp_flp_weight_array_sizes, knob::tlp_flp_feature_hash_types,
                                   knob::tlp_flp_activation_threshold, knob::tlp_flp_max_weight, knob::tlp_flp_min_weight, knob::tlp_flp_pos_weight_delta,
                                   knob::tlp_flp_neg_weight_delta, knob::tlp_flp_pos_train_thresh, knob::tlp_flp_neg_train_thresh);
  flp_pred->set_cpu(cpu);
  slp_pred = new perceptron_pred_t(knob::tlp_slp_activated_features, knob::tlp_slp_weight_array_sizes, knob::tlp_slp_feature_hash_types,
                                   knob::tlp_slp_activation_threshold, knob::tlp_slp_max_weight, knob::tlp_slp_min_weight, knob::tlp_slp_pos_weight_delta,
                                   knob::tlp_slp_neg_weight_delta, knob::tlp_slp_pos_train_thresh, knob::tlp_slp_neg_train_thresh);
  slp_pred->set_cpu(cpu);

  tau_1 = knob::tlp_tau_1;
  tau_2 = knob::tlp_tau_2;

  // init page buffer for FLP
  for (uint32_t index = 0; index < knob::tlp_flp_page_buf_sets; ++index) {
    deque<tlp_page_buf_entry_t *> d;
    d.clear();
    m_page_buffer.push_back(d);
  }

  // init page buffer for SLP
  for (uint32_t index = 0; index < knob::tlp_slp_page_buf_sets; ++index) {
    deque<tlp_page_buf_entry_t *> d;
    d.clear();
    m_page_buffer_pf.push_back(d);
  }

  true_pos = 0;
  false_pos = 0;
  false_neg = 0;
  true_neg = 0;
  true_pos_pf = 0;
  false_pos_pf = 0;
  false_neg_pf = 0;
  true_neg_pf = 0;

  reset_stats();
}

OffchipPredTLP::~OffchipPredTLP() {
}

bool OffchipPredTLP::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry) {
  state_info_t *info = get_state(arch_instr, data_index, lq_entry);
  float tlp_weight_sum = 0.0;
  bool prediction = false;

  // get prediction
  flp_pred->predict(info, prediction, tlp_weight_sum);

  // save all necessary data that would
  // later be required for training in LQ entry
  tlp_feature_t *feature = new tlp_feature_t();
  feature->info = info;
  feature->perc_weight_sum = tlp_weight_sum;
  lq_entry->ocp_feature = feature;

  stats.predict.called++;
  stats.predict.outcome[prediction]++;

  return prediction;
}

void OffchipPredTLP::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry) {
  train_count++;

  // keep track of true/false positives/negatives
  if (lq_entry->went_offchip_pred && lq_entry->went_offchip) {
    true_pos++;
  } else if (lq_entry->went_offchip_pred && !lq_entry->went_offchip) {
    false_pos++;
  } else if (!lq_entry->went_offchip_pred && lq_entry->went_offchip) {
    false_neg++;
  } else if (!lq_entry->went_offchip_pred && !lq_entry->went_offchip) {
    true_neg++;
  }

  // retreive all necessary data from LQ entry
  // that were used before for prediction making
  tlp_feature_t *feature = (tlp_feature_t *)lq_entry->ocp_feature;
  state_info_t *info = feature->info;
  float flp_weight_sum = feature->perc_weight_sum;

  // train perceptron
  flp_pred->train(info, flp_weight_sum, lq_entry->went_offchip_pred, lq_entry->went_offchip);

  stats.train.called++;
}

state_info_t *OffchipPredTLP::get_state(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry) {
  uint64_t load_pc = arch_instr->ip;
  uint64_t vaddr = lq_entry->virtual_address;
  uint64_t vpage = vaddr >> LOG2_PAGE_SIZE;
  uint32_t voffset = (vaddr >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
  uint32_t v_cl_offset = vaddr & ((1ull << LOG2_BLOCK_SIZE) - 1);
  uint32_t v_cl_word_offset = v_cl_offset >> 2;
  uint32_t v_cl_dword_offset = v_cl_offset >> 4;

  state_info_t *info = new state_info_t();

  // get control-flow features
  uint64_t last_n_load_pc_sig = 0, last_n_pc_sig = 0;
  get_control_flow_signatures(lq_entry, last_n_load_pc_sig, last_n_pc_sig);

  // get data-flow features
  bool first_access = false;
  lookup_address(vaddr, vpage, voffset, first_access);

  // populate features
  info->pc = load_pc;
  info->last_n_load_pc_sig = last_n_load_pc_sig;
  info->last_n_pc_sig = last_n_pc_sig;
  info->data_index = data_index;
  info->vaddr = vaddr;
  info->vpage = vpage;
  info->voffset = voffset;
  info->first_access = first_access;
  info->v_cl_offset = v_cl_offset;
  info->v_cl_word_offset = v_cl_word_offset;
  info->v_cl_dword_offset = v_cl_dword_offset;

  return info;
  // return NULL;
}

void OffchipPredTLP::lookup_address(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access) {
  stats.page_buf.called++;
  unique_pages.insert(vpage);

  tlp_page_buf_entry_t *entry = NULL;
  uint32_t set = get_set(vpage);
  auto it = find_if(m_page_buffer[set].begin(), m_page_buffer[set].end(), [vpage](tlp_page_buf_entry_t *entry) { return entry->page == vpage; });

  if (it != m_page_buffer[set].end()) // page hit
  {
    entry = (*it);
    first_access = !entry->bmp_access.test(voffset);
    entry->bmp_access.set(voffset);
    entry->age = 0;
    m_page_buffer[set].erase(it);
    m_page_buffer[set].push_back(entry);
    stats.page_buf.hit++;
  } else {
    if (m_page_buffer[set].size() >= knob::ocp_perc_page_buf_assoc) {
      entry = m_page_buffer[set].front();
      m_page_buffer[set].pop_front();
      stats.page_buf.eviction++;
      delete entry;
    }

    entry = new tlp_page_buf_entry_t();
    entry->page = vpage;
    entry->bmp_access.set(voffset);
    entry->age = 0;
    m_page_buffer[set].push_back(entry);
    first_access = true;
    stats.page_buf.insertion++;
  }
}

uint32_t OffchipPredTLP::get_set(uint64_t page) {
  uint32_t hash = HashZoo::fnv1a64(page);
  return hash % knob::ocp_perc_page_buf_sets;
}

void OffchipPredTLP::get_control_flow_signatures(LSQ_ENTRY *lq_entry, uint64_t &last_n_load_pc_sig, uint64_t &last_n_pc_sig) {
  // signature from last N load PCs
  uint64_t curr_pc = lq_entry->ip;
  if (last_n_load_pcs.size() >= knob::ocp_perc_last_n_load_pcs) {
    last_n_load_pcs.pop_front();
  }
  last_n_load_pcs.push_back(curr_pc);

  last_n_load_pc_sig = 0;
  for (uint32_t index = 0; index < last_n_load_pcs.size(); ++index) {
    last_n_load_pc_sig <<= 1;
    last_n_load_pc_sig ^= last_n_load_pcs[index];
  }

  // signature from last N instruction PCs
  deque<uint64_t> last_n_pcs;
  int prior = lq_entry->rob_index;
  for (int i = 0; i < (int)knob::ocp_perc_last_n_pcs - 1; ++i) {
    last_n_pcs.push_front(ooo_cpu[cpu].ROB.entry[prior].ip);
    prior--;
    if (prior < 0) {
      prior = ooo_cpu[cpu].ROB.SIZE - 1;
    }
  }

  last_n_pc_sig = 0;
  for (uint32_t index = 0; index < last_n_pcs.size(); ++index) {
    last_n_pc_sig <<= 1;
    last_n_pc_sig ^= last_n_pcs[index];
  }
}

string OffchipPredTLP::print_activated_features(vector<int32_t> activated_features) {
  std::stringstream ss;
  for (uint32_t feature = 0; feature < activated_features.size(); ++feature) {
    if (feature) {
      ss << ",";
    }
    ss << perc::feature_names[activated_features[feature]];
  }
  return ss.str();
}

// SLP (Second Level Predictor) methods for prefetch filtering

state_info_t *OffchipPredTLP::get_state_on_prefetch(PACKET &pf_packet) {
  uint64_t pf_pc = pf_packet.ip;
  uint64_t vaddr = pf_packet.full_addr;
  uint64_t vpage = vaddr >> LOG2_PAGE_SIZE;
  uint32_t voffset = (vaddr >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
  uint32_t v_cl_offset = vaddr & ((1ull << LOG2_BLOCK_SIZE) - 1);
  uint32_t v_cl_word_offset = v_cl_offset >> 2;
  uint32_t v_cl_dword_offset = v_cl_offset >> 4;

  bool first_access = false;
  lookup_address_on_prefetch(vaddr, vpage, voffset, first_access);

  // Create state info for prefetch
  state_info_t *info = new state_info_t();
  info->pc = pf_pc;
  info->last_n_load_pc_sig = 0;
  info->last_n_pc_sig = 0;
  info->data_index = 0;
  info->vaddr = vaddr;
  info->vpage = vpage;
  info->voffset = voffset;
  info->first_access = first_access;
  info->v_cl_offset = v_cl_offset;
  info->v_cl_word_offset = v_cl_word_offset;
  info->v_cl_dword_offset = v_cl_dword_offset;

  // Add the triggering load's FLP prediction as a feature
  // This is whether the triggering load is likely to go off-chip, not the prefetch itself
  info->flp_prediction = pf_packet.went_offchip_pred;

  return info;
}

void OffchipPredTLP::lookup_address_on_prefetch(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access) {
  stats.page_buf_pf.called++;
  unique_pages_pf.insert(vpage);

  tlp_page_buf_entry_t *entry = NULL;
  uint32_t set = get_set(vpage);
  auto it = find_if(m_page_buffer_pf[set].begin(), m_page_buffer_pf[set].end(), [vpage](tlp_page_buf_entry_t *entry) { return entry->page == vpage; });

  if (it != m_page_buffer_pf[set].end()) // page hit
  {
    entry = (*it);
    first_access = !entry->bmp_access.test(voffset);
    entry->bmp_access.set(voffset);
    entry->age = 0;
    m_page_buffer_pf[set].erase(it);
    m_page_buffer_pf[set].push_back(entry);
    stats.page_buf_pf.hit++;
  } else {
    if (m_page_buffer_pf[set].size() >= knob::tlp_slp_page_buf_assoc) {
      entry = m_page_buffer_pf[set].front();
      m_page_buffer_pf[set].pop_front();
      stats.page_buf_pf.eviction++;
      delete entry;
    }

    entry = new tlp_page_buf_entry_t();
    entry->page = vpage;
    entry->bmp_access.set(voffset);
    entry->age = 0;
    m_page_buffer_pf[set].push_back(entry);
    first_access = true;
    stats.page_buf_pf.insertion++;
  }
}

bool OffchipPredTLP::predict_on_prefetch(PACKET &pf_packet) {
  // SLP prediction for prefetch filtering
  // Returns true if the prefetch should be filtered (discarded)

  // Get state information for the prefetch
  state_info_t *info = get_state_on_prefetch(pf_packet);
  float slp_weight_sum = 0.0;
  bool filter_prediction = false;

  // Get SLP prediction
  slp_pred->predict(info, filter_prediction, slp_weight_sum);

  // Save the weight sum for training
  tlp_feature_t *feature = new tlp_feature_t();
  feature->info = info;
  feature->perc_weight_sum = slp_weight_sum;
  pf_packet.ocp_feature = feature;

  // Update statistics
  stats.predict.called++;
  stats.predict.outcome[filter_prediction]++;

  return filter_prediction; // true means filter (discard) the prefetch
}

void OffchipPredTLP::train_on_prefetch(PACKET &pf_packet) {
  // Train the SLP based on prefetch outcome
  train_count++;

  // Retrieve feature information
  tlp_feature_t *feature = (tlp_feature_t *)pf_packet.ocp_feature;
  state_info_t *info = feature->info;
  float slp_weight_sum = feature->perc_weight_sum;

  slp_pred->train(info, slp_weight_sum, pf_packet.pf_went_offchip_pred, pf_packet.pf_went_offchip);

  // Update statistics
  if (pf_packet.pf_went_offchip_pred && pf_packet.pf_went_offchip) {
    true_pos_pf++;
  } else if (pf_packet.pf_went_offchip_pred && !pf_packet.pf_went_offchip) {
    false_pos_pf++;
  } else if (!pf_packet.pf_went_offchip_pred && pf_packet.pf_went_offchip) {
    false_neg_pf++;
  } else if (!pf_packet.pf_went_offchip_pred && !pf_packet.pf_went_offchip) {
    true_neg_pf++;
  }

  stats.train.called++;
}
