#include "oogway_helper.h"
#include "knobs.h"
#include <string>

std::string og_state_t::to_string() {
  std::stringstream ss;
  ss << "pref_acc_0: " << pref_acc[0] << " pref_acc_1: " << pref_acc[1] << " ocp_acc: " << ocp_acc << " overall_bw: " << overall_bw << " pref_bw: " << pref_bw
     << " ocp_bw: " << ocp_bw << " bw_needed: " << bw_needed

     << " total_scheduled: " << total_scheduled << " demand_scheduled: " << demand_scheduled << " pf_scheduled: " << pf_scheduled
     << " ocp_scheduled: " << ocp_scheduled

     << " pref_pollution: " << pref_pollution;
  ss << " raw32: " << std::hex << get_raw32() << std::dec;
  return ss.str();
}

uint64_t og_state_t::get_raw64() {
  // Define the maximum values for each feature
  const uint32_t pref_acc_max = 100;
  const uint32_t ocp_acc_max = 100;
  const uint32_t overall_bw_max = 64;
  const uint32_t pref_bw_max = 64;
  const uint32_t ocp_bw_max = 64;
  const uint32_t bw_needed_max = 64;
  const uint32_t pref_pollution_max = 255;

  // Normalize each feature value to the range [0, 255]
  uint64_t raw64 = 0;
  raw64 |= (static_cast<uint64_t>(pref_acc[0] * 255 / pref_acc_max) & 0xFF) << 56;
  raw64 |= (static_cast<uint64_t>(ocp_acc * 255 / ocp_acc_max) & 0xFF) << 48;
  raw64 |= (static_cast<uint64_t>(overall_bw * 255 / overall_bw_max) & 0xFF) << 40;
  raw64 |= (static_cast<uint64_t>(pref_bw * 255 / pref_bw_max) & 0xFF) << 32;
  raw64 |= (static_cast<uint64_t>(ocp_bw * 255 / ocp_bw_max) & 0xFF) << 24;
  raw64 |= (static_cast<uint64_t>(bw_needed * 255 / bw_needed_max) & 0xFF) << 16;
  raw64 |= (static_cast<uint64_t>(pref_pollution * 255 / pref_pollution_max) & 0xFF) << 8;

  return raw64;
}

uint32_t og_state_t::get_raw32() {
  const uint32_t pref_acc_max = 100;
  const uint32_t ocp_acc_max = 100;
  const uint32_t overall_bw_max = 100;
  const uint32_t pref_bw_max = 64;
  const uint32_t ocp_bw_max = 64;
  const uint32_t bw_needed_max = 64;
  const uint32_t pref_pollution_max = 255;

  uint32_t raw32 = 0;
  // raw32 |= (static_cast<uint32_t>(pref_acc * 15 / pref_acc_max) & 0xF) << 28;  // 4 bits
  // raw32 |= (static_cast<uint32_t>(ocp_acc * 15 / ocp_acc_max) & 0xF) << 24;   // 4 bits
  // raw32 |= (static_cast<uint32_t>(overall_bw * 15 / overall_bw_max) & 0xF) << 20; // 4 bits
  // raw32 |= (static_cast<uint32_t>(pref_bw * 15 / pref_bw_max) & 0xF) << 16;   // 4 bits
  // raw32 |= (static_cast<uint32_t>(ocp_bw * 15 / ocp_bw_max) & 0xF) << 12;     // 4 bits
  // raw32 |= (static_cast<uint32_t>(bw_needed * 15 / bw_needed_max) & 0xF) << 8; // 4 bits
  // raw32 |= (static_cast<uint32_t>(pref_pollution * 255 / pref_pollution_max) & 0xFF); // 8 bits

  if (knob::og_feature_mask[0]) {
    raw32 |= (static_cast<uint32_t>(pref_acc[0] * 15 / pref_acc_max) & 0xF) << 28;
    raw32 |= (static_cast<uint32_t>(pref_acc[1] * 15 / pref_acc_max) & 0xF) << 24;
  }

  if (knob::og_feature_mask[1]) {
    raw32 |= (static_cast<uint32_t>(ocp_acc * 15 / ocp_acc_max) & 0xF) << 20;
  }

  if (knob::og_feature_mask[2]) {
    raw32 |= (static_cast<uint32_t>(overall_bw * 15 / overall_bw_max) & 0xF) << 16;
  }

  if (knob::og_feature_mask[3]) {
    raw32 |= (static_cast<uint32_t>(demand_scheduled * 15 / (total_scheduled + 1)) & 0xF) << 12;
  }

  if (knob::og_feature_mask[4]) {
    raw32 |= (static_cast<uint32_t>(pf_scheduled * 15 / (total_scheduled + 1)) & 0xF) << 8;
  }

  if (knob::og_feature_mask[5]) {
    raw32 |= (static_cast<uint32_t>(ocp_scheduled * 15 / (total_scheduled + 1)) & 0xF) << 4;
  }

  if (knob::og_feature_mask[6]) {
    raw32 |= (static_cast<uint32_t>(pref_pollution * 15 / pref_pollution_max) & 0xF);
  }
  return raw32;
}

std::string og_sysevent_type_str_t[] = {"CYCLE", "LLC_LOAD_MISS", "LLC_LOAD_MISS_LAT", "MISPRED_BRANCH", "LOAD_INST_ISSUE", "L1D_LOAD_MISS"};

std::string og_sysevent_type2str(og_sysevent_type_t sevent) {
  assert(sevent < NUM_EVENTS);
  return og_sysevent_type_str_t[sevent];
}

std::string og_sysevent_t::to_string() {
  std::stringstream ss;
  for (uint32_t i = 0; i < NUM_EVENTS; ++i) {
    ss << og_sysevent_type2str((og_sysevent_type_t)i) << ": " << events[i] << ", ";
  }
  return ss.str();
}