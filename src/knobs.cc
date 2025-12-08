#include "knobs.h"
#include "defs.h"
#include "ini.h"
#include <assert.h>
#include <iostream>
#include <math.h>
#include <string.h>
#include <string>
using namespace std;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

namespace knob {
// uint64_t        warmup_instructions     = 1000000;
// uint64_t        simulation_instructions = 1000000;
// bool            cloudsuite              = false;
// bool            low_bandwidth           = false;
#define DEF_KNOB(opt, name, type, parser, defval) type name = defval;
#include "knobs.def"
#undef DEF_KNOB

/* complex knobs */
vector<string> l1d_prefetcher_types;
vector<string> l2c_prefetcher_types;
vector<string> llc_prefetcher_types;
vector<int32_t> rob_partition_size;
vector<int32_t> rob_partition_boundaries;
vector<int32_t> rob_frontal_partition_ids;
vector<int32_t> rob_dorsal_partition_ids;

} // namespace knob

void parse_args(int argc, char *argv[]) {
  for (int index = 0; index < argc; ++index) {
    string arg = string(argv[index]);
    if (arg.compare(0, 2, "--") == 0) {
      arg = arg.substr(2);
    }
    if (ini_parse_string(arg.c_str(), handler, NULL) < 0) {
      printf("error parsing commandline %s\n", argv[index]);
      exit(1);
    }
  }
}

int handler(void *user, const char *section, const char *name, const char *value) {
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  } else {
    parse_knobs(user, section, name, value);
  }
  return 1;
}

void parse_config(char *config_file_name) {
  cout << "parsing config file: " << string(config_file_name) << endl;
  if (ini_parse(config_file_name, parse_knobs, NULL) < 0) {
    printf("Failed to load %s\n", config_file_name);
    exit(1);
  }
}

int32_t get_int32(const char *str) {
  return atoi(str);
}

uint32_t get_uint32(const char *str) {
  return atoi(str);
}

uint64_t get_uint64(const char *str) {
  return atol(str);
}

bool get_bool(const char *str) {
  return !strcmp(str, "true") ? true : false;
}

string get_string(const char *str) {
  return string(str);
}

float get_float(const char *str) {
  return atof(str);
}

double get_double(const char *str) {
  return strtod(str, NULL);
}

vector<int32_t> get_int32v(const char *str) {
  std::vector<int32_t> value;
  char *tmp_str = strdup(str);
  char *pch = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(strtol(pch, NULL, 0));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

vector<float> get_floatv(const char *str) {
  std::vector<float> value;
  char *tmp_str = strdup(str);
  char *pch = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(atof(pch));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

int parse_knobs(void *user, const char *section, const char *name, const char *value) {
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  }
  /* basic knobs */
#define DEF_KNOB(opt, name, type, parser, defval)                                                                                                              \
  else if (MATCH("", #opt)) {                                                                                                                                  \
    knob::name = get_##parser(value);                                                                                                                          \
  }
#include "knobs.def"
#undef DEF_KNOB

  /* complex knobs */
  else if (MATCH("", "l1d_prefetcher_types")) {
    knob::l1d_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "l2c_prefetcher_types")) {
    knob::l2c_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "llc_prefetcher_types")) {
    knob::llc_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "rob_partition_size")) {
    knob::rob_partition_size = get_int32v(value);
    assert(knob::rob_partition_size.size() == knob::num_rob_partitions);
    int32_t len = 0;
    for (uint32_t index = 0; index < knob::rob_partition_size.size(); ++index) {
      len += knob::rob_partition_size[index];
    }
    assert(len == ROB_SIZE);
    len = 0;
    for (uint32_t index = 0; index < knob::rob_partition_size.size() - 1; ++index) {
      len += knob::rob_partition_size[index];
      knob::rob_partition_boundaries.push_back(len);
    }
  } else if (MATCH("", "rob_frontal_partition_ids")) {
    knob::rob_frontal_partition_ids = get_int32v(value);
    for (uint32_t index = 0; index < knob::rob_frontal_partition_ids.size(); ++index) {
      assert(knob::rob_frontal_partition_ids[index] < (int32_t)knob::num_rob_partitions);
    }
  } else if (MATCH("", "rob_dorsal_partition_ids")) {
    knob::rob_dorsal_partition_ids = get_int32v(value);
    for (uint32_t index = 0; index < knob::rob_dorsal_partition_ids.size(); ++index) {
      assert(knob::rob_dorsal_partition_ids[index] < (int32_t)knob::num_rob_partitions);
    }
  }

  /* default */
  else {
    printf("unable to parse section: %s, name: %s, value: %s\n", section, name, value);
    return 0;
  }
  return 1;
}
