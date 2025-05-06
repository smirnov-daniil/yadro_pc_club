#include "event_processor.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>

pc_club::event_processor::event_processor(
    std::int32_t tables,
    std::int32_t price,
    std::int32_t open_time,
    std::int32_t close_time
)
    : _tables_count(tables)
    , _price(price)
    , _open_time(open_time)
    , _close_time(close_time)
    , _tables(tables + 1, {.occupied_since = std::numeric_limits<std::int32_t>::max(), .revenue = 0, .usage = 0}) {
  std::cout << format_time(open_time) << '\n';
}

std::string pc_club::event_processor::format_time(std::int32_t minutes) {
  std::int32_t h = minutes / 60;
  std::int32_t m = minutes % 60;
  char buf[6];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
  return buf;
}

void pc_club::event_processor::close_table(std::int32_t table_id, std::int32_t current_time) {
  std::int32_t start = _tables[table_id].occupied_since;
  std::int32_t duration = current_time - start;
  _tables[table_id].usage += duration;
  const std::int32_t hours = (duration + 59) / 60;
  _tables[table_id].revenue += static_cast<std::int64_t>(hours) * _price;
  _client_table.erase_right(table_id);
}

void pc_club::event_processor::assign_next(std::int32_t table_id, std::int32_t current_time) {
  if (_waiting.empty()) {
    return;
  }
  std::string next = _waiting.front();
  _waiting.pop();
  _client_table.insert(next, table_id);
  _tables[table_id].occupied_since = current_time;
  std::cout << format_time(current_time) << " 12 " << next << ' ' << std::to_string(table_id) << '\n';
}

void pc_club::event_processor::enter(const event& e) {
  std::cout << format_time(e.time) << " 1 " << e.name << '\n';
  if (e.time < _open_time) {
    std::cout << format_time(e.time) << " 13 NotOpenYet\n";
  } else if (!_clients.insert(e.name).second) {
    std::cout << format_time(e.time) << " 13 YouShallNotPass\n";
  } else {
    _clients.insert(e.name);
  }
}

void pc_club::event_processor::take(const event& e) {
  std::cout << format_time(e.time) << " 2 " << e.name << ' ' << std::to_string(e.table) << '\n';

  if (!_clients.contains(e.name)) {
    std::cout << format_time(e.time) << " 13 ClientUnknown\n";
  } else if (_client_table.find_right(e.table) != _client_table.end_right()) {
    std::cout << format_time(e.time) << " 13 PlaceIsBusy\n";
  } else {
    if (auto it = _client_table.find_left(e.name); it != _client_table.end_left()) {
      std::int32_t old = *it.flip();
      close_table(old, e.time);
      assign_next(old, e.time);
    }
    _client_table.insert(e.name, e.table);
    _tables[e.table].occupied_since = e.time;
  }
}

void pc_club::event_processor::wait(const event& e) {
  std::cout << format_time(e.time) << " 3 " << e.name << '\n';
  if (!_clients.contains(e.name)) {
    std::cout << format_time(e.time) << " 13 ClientUnknown\n";
  } else if (static_cast<std::int32_t>(_waiting.size()) >= _tables_count) {
    std::cout << format_time(e.time) << " 11 " << e.name << '\n';
  } else if (_client_table.size() == _tables_count) {
    _waiting.emplace(e.name);
  } else {
    std::cout << format_time(e.time) << " 13 ICanWaitNoLonger!\n";
  }
}

void pc_club::event_processor::leave(const event& e) {
  std::cout << format_time(e.time) << " 4 " << e.name << '\n';
  if (!_clients.contains(e.name)) {
    std::cout << format_time(e.time) << " 13 ClientUnknown\n";
  } else {
    if (auto it = _client_table.find_left(e.name); it != _client_table.end_left()) {
      std::int32_t tbl = *it.flip();
      close_table(tbl, e.time);
      assign_next(tbl, e.time);
    }
    _clients.erase(e.name);
  }
}

void pc_club::event_processor::process_event(const event& e) {
  switch (e.type) {
  case event_type::enter:
    enter(e);
    break;
  case event_type::take:
    take(e);
    break;
  case event_type::wait:
    wait(e);
    break;
  case event_type::leave:
    leave(e);
    break;
  default:
    break;
  }
}

void pc_club::event_processor::close() {
  for (auto it = _client_table.begin_left(); it != _client_table.end_left();) {
    auto prev = it++;
    close_table(*prev.flip(), _close_time);
  }
  std::cout << format_time(_close_time) << '\n';
  for (std::int32_t i = 1; i <= _tables_count; i++) {
    std::cout << i << ' ' << _tables[i].revenue << ' ' << format_time(_tables[i].usage) << '\n';
  }
}
