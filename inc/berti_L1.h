#ifndef BERTI_L1_H
#define BERTI_L1_H

#include "cache.h"
#include "prefetcher.h"
#include <cstdint>
#include <vector>

// Berti L1D Prefetcher
class BertiL1 : public Prefetcher {
private:
  CACHE *m_parent_cache;

// Berti-specific data structures and functions
// These will be moved from the original berti_L1.cc implementation

// TIME AND OVERFLOWS
#define L1D_TIME_BITS 16
#define L1D_TIME_OVERFLOW ((uint64_t)1 << L1D_TIME_BITS)
#define L1D_TIME_MASK (L1D_TIME_OVERFLOW - 1)

  // STRIDE
  int l1d_calculate_stride(uint64_t prev_offset, uint64_t current_offset);

// CURRENT PAGES TABLE
#define L1D_CURRENT_PAGES_TABLE_INDEX_BITS 6
#define L1D_CURRENT_PAGES_TABLE_ENTRIES ((1 << L1D_CURRENT_PAGES_TABLE_INDEX_BITS) - 1)
#define L1D_CURRENT_PAGES_TABLE_NUM_BERTI 10
#define L1D_CURRENT_PAGES_TABLE_NUM_BERTI_PER_ACCESS 7

  typedef struct __l1d_current_page_entry {
    uint64_t page_addr;
    uint64_t ip;
    uint64_t u_vector;
    uint64_t first_offset;
    int berti[L1D_CURRENT_PAGES_TABLE_NUM_BERTI];
    unsigned berti_ctr[L1D_CURRENT_PAGES_TABLE_NUM_BERTI];
    uint64_t last_burst;
    uint64_t lru;
  } l1d_current_page_entry;

  l1d_current_page_entry l1d_current_pages_table[L1D_CURRENT_PAGES_TABLE_ENTRIES];

// PREVIOUS REQUESTS TABLE
#define L1D_PREV_REQUESTS_TABLE_INDEX_BITS 10
#define L1D_PREV_REQUESTS_TABLE_ENTRIES (1 << L1D_PREV_REQUESTS_TABLE_INDEX_BITS)
#define L1D_PREV_REQUESTS_TABLE_MASK (L1D_PREV_REQUESTS_TABLE_ENTRIES - 1)
#define L1D_PREV_REQUESTS_TABLE_NULL_POINTER L1D_CURRENT_PAGES_TABLE_ENTRIES

  typedef struct __l1d_prev_request_entry {
    uint64_t page_addr_pointer;
    uint64_t offset;
    uint64_t time;
  } l1d_prev_request_entry;

  l1d_prev_request_entry l1d_prev_requests_table[L1D_PREV_REQUESTS_TABLE_ENTRIES];
  uint64_t l1d_prev_requests_table_head;

// PREVIOUS PREFETCHES TABLE
#define L1D_PREV_PREFETCHES_TABLE_INDEX_BITS 9
#define L1D_PREV_PREFETCHES_TABLE_ENTRIES (1 << L1D_PREV_PREFETCHES_TABLE_INDEX_BITS)
#define L1D_PREV_PREFETCHES_TABLE_MASK (L1D_PREV_PREFETCHES_TABLE_ENTRIES - 1)
#define L1D_PREV_PREFETCHES_TABLE_NULL_POINTER L1D_CURRENT_PAGES_TABLE_ENTRIES

  typedef struct __l1d_prev_prefetch_entry {
    uint64_t page_addr_pointer;
    uint64_t offset;
    uint64_t time_lat;
    bool completed;
  } l1d_prev_prefetch_entry;

  l1d_prev_prefetch_entry l1d_prev_prefetches_table[L1D_PREV_PREFETCHES_TABLE_ENTRIES];
  uint64_t l1d_prev_prefetches_table_head;

// RECORD PAGES TABLE
#define L1D_RECORD_PAGES_TABLE_ENTRIES (((1 << 10) + (1 << 8) + (1 << 7)) - 1)
#define L1D_TRUNCATED_PAGE_ADDR_BITS 32
#define L1D_TRUNCATED_PAGE_ADDR_MASK (((uint64_t)1 << L1D_TRUNCATED_PAGE_ADDR_BITS) - 1)

  typedef struct __l1d_record_page_entry {
    uint64_t page_addr;
    uint64_t u_vector;
    uint64_t first_offset;
    int berti;
    uint64_t lru;
  } l1d_record_page_entry;

  l1d_record_page_entry l1d_record_pages_table[L1D_RECORD_PAGES_TABLE_ENTRIES];

// IP TABLE
#define L1D_IP_TABLE_INDEX_BITS 10
#define L1D_IP_TABLE_ENTRIES (1 << L1D_IP_TABLE_INDEX_BITS)
#define L1D_IP_TABLE_INDEX_MASK (L1D_IP_TABLE_ENTRIES - 1)
#define L1D_IP_TABLE_NULL_POINTER L1D_RECORD_PAGES_TABLE_ENTRIES

  uint64_t l1d_ip_table[L1D_IP_TABLE_ENTRIES];

  // Helper functions
  uint64_t l1d_get_latency(uint64_t cycle, uint64_t cycle_prev);
  void l1d_init_current_pages_table();
  uint64_t l1d_get_current_pages_entry(uint64_t page_addr);
  void l1d_update_lru_current_pages_table(uint64_t index);
  uint64_t l1d_get_lru_current_pages_entry();
  int l1d_get_berti_current_pages_table(uint64_t index, uint64_t &ctr);
  void l1d_add_current_pages_table(uint64_t index, uint64_t page_addr, uint64_t ip, uint64_t offset);
  uint64_t l1d_update_demand_current_pages_table(uint64_t index, uint64_t offset);
  void l1d_add_berti_current_pages_table(uint64_t index, int berti);
  bool l1d_requested_offset_current_pages_table(uint64_t index, uint64_t offset);
  void l1d_remove_current_table_entry(uint64_t index);

  void l1d_init_prev_requests_table();
  uint64_t l1d_find_prev_request_entry(uint64_t pointer, uint64_t offset);
  void l1d_add_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_reset_pointer_prev_requests(uint64_t pointer);
  uint64_t l1d_get_latency_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_get_berti_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle, int *berti);

  void l1d_init_prev_prefetches_table();
  uint64_t l1d_find_prev_prefetch_entry(uint64_t pointer, uint64_t offset);
  void l1d_add_prev_prefetches_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_reset_pointer_prev_prefetches(uint64_t pointer);
  void l1d_reset_entry_prev_prefetches_table(uint64_t pointer, uint64_t offset);
  uint64_t l1d_get_and_set_latency_prev_prefetches_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  uint64_t l1d_get_latency_prev_prefetches_table(uint64_t pointer, uint64_t offset);

  void l1d_init_record_pages_table();
  uint64_t l1d_get_lru_record_pages_entry();
  void l1d_update_lru_record_pages_table(uint64_t index);
  void l1d_add_record_pages_table(uint64_t index, uint64_t page_addr, uint64_t vector, uint64_t first_offset, int berti);
  uint64_t l1d_get_entry_record_pages_table(uint64_t page_addr, uint64_t first_offset);
  uint64_t l1d_get_entry_record_pages_table(uint64_t page_addr);
  void l1d_copy_entries_record_pages_table(uint64_t index_from, uint64_t index_to);

  void l1d_init_ip_table();
  void l1d_record_current_page(uint64_t index_current);

  // Statistics
  struct {
    uint64_t prefetches_issued;
    uint64_t prefetches_useful;
    uint64_t prefetches_useless;
  } stats;

  void init_stats();

public:
  BertiL1(std::string type, CACHE *cache);
  ~BertiL1();

  // Prefetcher interface
  void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, std::vector<uint64_t> &pref_addr);
  void dump_stats();
  void print_config();

  // Additional Berti-specific interface
  void cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);
};

#endif /* BERTI_L1_H */

