#pragma once
#ifndef __event_processor_h_
#define __event_processor_h_

#include "bimap.h"

#include <queue>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace pc_club {
enum class event_type : std::int8_t {
  enter = 1,
  take = 2,
  wait = 3,
  leave = 4
};

struct event {
  std::int32_t time;
  event_type type;
  std::string name;
  std::int32_t table;
};

struct table {
  std::int32_t occupied_since;
  std::int64_t revenue;
  std::int32_t usage;
};

class event_processor {
public:
  event_processor(std::int32_t tables, std::int32_t price, std::int32_t open_time, std::int32_t close_time);
  void process_event(const event& e);
  void close();

private:
  static std::string format_time(std::int32_t minutes);

  void close_table(std::int32_t table_id, std::int32_t current_time);
  void assign_next(std::int32_t table_id, std::int32_t current_time);

  void enter(const event& e);
  void take(const event& e);
  void wait(const event& e);
  void leave(const event& e);

private:
  std::int32_t _tables_count;
  std::int32_t _price;
  std::int32_t _open_time;
  std::int32_t _close_time;

  std::unordered_set<std::string> _clients;
  std::vector<table> _tables;
  std::queue<std::string> _waiting;
  bimap<std::string, std::int32_t> _client_table;
};
} // namespace pc_club

#endif // !__event_processor_h_
