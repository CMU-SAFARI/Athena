//---------------------------------------------------------------//
// This file defines a utility tool to trace a RL-based engine.
// The goal of this tool is to provide visalization of
// how the RL engine is learning and making decisions.
//
// Author: Rahul Bera (write2bera@gmail.com)
//---------------------------------------------------------------//

#ifndef LE_TRACER_H
#define LE_TRACER_H

#include "oogway_helper.h"
#include <sstream>
#include <string>
#include <zlib.h>

class LETracer {
private:
  bool is_enabled = false;
  std::string base_tracename;
  FILE *fp_s = nullptr; // for tracing state
  FILE *fp_a = nullptr; // for tracing actions
  FILE *fp_r = nullptr; // for tracing rewards

  // buffer to defer printing
  std::stringstream s_buf, a_buf, r_buf;
  uint32_t buffer_count = 0;

public:
  LETracer(std::string basename);
  ~LETracer();

  // functionalities
  inline void enable() {
    is_enabled = true;
  }
  inline void disable() {
    is_enabled = false;
  }
  void trace(og_state_t *state, uint32_t action, float reward);
};

#endif
