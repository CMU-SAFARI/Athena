#include "berti_L1.h"
#include "cache.h"
#include "ipcp_L1.h"
#include "knobs.h"
#include "next_line.h"
#include "ooo_cpu.h"
#include "prefetcher.h"
#include "stride.h"
#include <assert.h>
#include <string>

using namespace std;

void CACHE::l1d_prefetcher_initialize() {
  for (uint32_t index = 0; index < knob::l1d_prefetcher_types.size(); ++index) {
    if (!knob::l1d_prefetcher_types[index].compare("none")) {
      cout << "adding L1D_PREFETCHER: NONE" << endl;
    } else if (!knob::l1d_prefetcher_types[index].compare("next_line")) {
      cout << "adding L1D_PREFETCHER: next_line" << endl;
      NextLinePrefetcher *pref_nl = new NextLinePrefetcher(knob::l1d_prefetcher_types[index]);
      l1d_prefetchers.push_back(pref_nl);
    } else if (!knob::l1d_prefetcher_types[index].compare("stride")) {
      cout << "adding L1D_PREFETCHER: Stride" << endl;
      StridePrefetcher *pref_stride = new StridePrefetcher(knob::l1d_prefetcher_types[index]);
      l1d_prefetchers.push_back(pref_stride);
    } else if (!knob::l1d_prefetcher_types[index].compare("ipcp")) {
      cout << "adding L1D_PREFETCHER: IPCP" << endl;
      IPCP_L1 *pref_ipcp_l1 = new IPCP_L1(knob::l1d_prefetcher_types[index], this);
      l1d_prefetchers.push_back(pref_ipcp_l1);
    } else if (!knob::l1d_prefetcher_types[index].compare("berti")) {
      cout << "adding L1D_PREFETCHER: Berti" << endl;
      BertiL1 *pref_berti = new BertiL1(knob::l1d_prefetcher_types[index], this);
      l1d_prefetchers.push_back(pref_berti);
    } else {
      cout << "unsupported prefetcher type " << knob::l1d_prefetcher_types[index] << endl;
      exit(1);
    }
  }

  for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
    l1d_prefetchers[index]->set_id(index);
    cout << " L1D pref " << l1d_prefetchers[index]->get_type() << " got ID " << l1d_prefetchers[index]->get_id() << endl;
  }

  assert(knob::l1d_prefetcher_types.size() == l1d_prefetchers.size() || !knob::l1d_prefetcher_types[0].compare("none"));

  /* enable ptracker */
  if (knob::ptracker_enable) {
    for (uint32_t i = 0; i < l1d_prefetchers.size(); ++i) {
      l1d_prefetchers[i]->create_ptracker(knob::ptracker_num_sets, knob::ptracker_num_ways);
      cout << "L1D Prefetcher " << l1d_prefetchers[i]->get_type() << " PTracker enabled with " << knob::ptracker_num_sets << " sets and "
           << knob::ptracker_num_ways << " ways." << endl;
    }
  }
}

void CACHE::l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type) {
  vector<uint64_t> pref_addr;
  pref_addr.clear();

  for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
    // record the demand
    if (l1d_prefetchers[index]->ptracker) {
      l1d_prefetchers[index]->ptracker->track_demand(addr);
    }
    // invoke IPCP
    l1d_prefetchers[index]->invoke_prefetcher(ip, addr, cache_hit, type, pref_addr);
    if (knob::l1d_prefetcher_types[index].compare("ipcp") && !pref_addr.empty()) {
      // not IPCP
      for (uint32_t addr_index = 0; addr_index < pref_addr.size(); ++addr_index) {
        if (!knob::og_enable || ooo_cpu[cpu].oogway->is_prefetch_enabled()) {
          prefetch_line(ip, addr, pref_addr[addr_index], FILL_L1, 0);
          if (l1d_prefetchers[index]->ptracker) {
            l1d_prefetchers[index]->ptracker->track_pref(pref_addr[addr_index]);
          }
        } else if (knob::og_enable) {
          ooo_cpu[cpu].oogway->incr_pf_throttled();
          // printf("IPCP_L1: throttled prefetching %lx\n", pref_addr[addr_index]);
        }
      }
    }
  }
  pref_addr.clear();
}

void CACHE::l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in) {
  if (prefetch) {
    for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
      if (!l1d_prefetchers[index]->get_type().compare("next_line")) {
        NextLinePrefetcher *pref_nl = (NextLinePrefetcher *)l1d_prefetchers[index];
        pref_nl->register_fill(addr);
      }
    }
  }

  // Call Berti cache fill for Berti prefetcher
  for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
    if (!l1d_prefetchers[index]->get_type().compare("berti")) {
      BertiL1 *pref_berti = (BertiL1 *)l1d_prefetchers[index];
      pref_berti->cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    }
  }
}

uint32_t CACHE::l1d_prefetcher_prefetch_hit(uint64_t addr, uint64_t ip, uint32_t metadata_in) {
  return metadata_in;
}

void CACHE::l1d_prefetcher_final_stats() {
  for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
    l1d_prefetchers[index]->dump_stats();
  }
}

void CACHE::l1d_prefetcher_print_config() {
  for (uint32_t index = 0; index < l1d_prefetchers.size(); ++index) {
    l1d_prefetchers[index]->print_config();
  }
}

void CACHE::l1d_prefetcher_broadcast_bw(uint8_t bw_level) {
}

void CACHE::l1d_prefetcher_broadcast_ipc(uint8_t ipc) {
}

void CACHE::l1d_prefetcher_broadcast_acc(uint32_t acc_level) {
}
