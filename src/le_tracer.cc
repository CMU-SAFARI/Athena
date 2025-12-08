#include "le_tracer.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#define MAX_BUFFER_LEN 1000

LETracer::LETracer(std::string basename) : base_tracename(basename) {
  std::string s_trace = base_tracename + ".state.csv";
  std::string a_trace = base_tracename + ".action.csv";
  std::string r_trace = base_tracename + ".reward.csv";

  fp_s = fopen(s_trace.c_str(), "w");
  assert(fp_s);
  fp_a = fopen(a_trace.c_str(), "w");
  assert(fp_a);
  fp_r = fopen(r_trace.c_str(), "w");
  assert(fp_r);

  // print headers in each file
  fprintf(fp_s, "epoch,retired_insts,pref_acc,ocp_acc,overall_bw,pref_bw,"
                "ocp_bw,bw_needed,pref_pollution\n");
  fprintf(fp_a, "epoch,retired_insts,action\n");
  fprintf(fp_r, "epoch,retired_insts,reward\n");
  fflush(fp_s);
  fflush(fp_a);
  fflush(fp_r);

  is_enabled = false;
}

LETracer::~LETracer() {
  fclose(fp_s);
  fclose(fp_a);
  fclose(fp_r);
}

void LETracer::trace(og_state_t *state, uint32_t action, float reward) {
  if (!is_enabled) {
    return;
  }

  if (!state) {
    return;
  }

  if (buffer_count < MAX_BUFFER_LEN) { // print in buffer, not in file
    s_buf << state->epoch_num << "," << state->num_retired_insts << "," << state->pref_acc << "," << state->ocp_acc << "," << state->overall_bw << ","
          << state->pref_bw << "," << state->ocp_bw << "," << state->bw_needed << "," << state->pref_pollution << std::endl;
    a_buf << state->epoch_num << "," << state->num_retired_insts << "," << action << std::endl;
    r_buf << state->epoch_num << "," << state->num_retired_insts << "," << reward << std::endl;
    buffer_count++;
  } else {
    fprintf(fp_s, "%s", s_buf.str().c_str());
    fflush(fp_s);
    s_buf.str(std::string(""));
    fprintf(fp_a, "%s", a_buf.str().c_str());
    fflush(fp_a);
    a_buf.str(std::string(""));
    fprintf(fp_r, "%s", r_buf.str().c_str());
    fflush(fp_r);
    r_buf.str(std::string(""));
    buffer_count = 0;
  }
}