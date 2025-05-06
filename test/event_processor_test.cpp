#include "event_processor.h"

#include <catch2/catch_all.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> split_lines(const std::string& s) {
  std::vector<std::string> lines;
  std::istringstream iss(s);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}
} // namespace

TEST_CASE("Simple one-table scenario: enter, take, leave, close", "[event_processor]") {
  using namespace pc_club;

  std::ostringstream oss;
  auto* old_buf = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(1, 10, 0, 120);

  ep.process_event({.time = 0, .type = event_type::enter, .name = "Alice", .table = -1});
  ep.process_event({.time = 0, .type = event_type::take, .name = "Alice", .table = 1});
  ep.process_event({.time = 61, .type = event_type::leave, .name = "Alice", .table = -1});

  ep.close();

  std::cout.rdbuf(old_buf);

  auto lines = split_lines(oss.str());

  REQUIRE(lines[0] == "00:00");
  REQUIRE(lines[1] == "00:00 1 Alice");
  REQUIRE(lines[2] == "00:00 2 Alice 1");
  REQUIRE(lines[3] == "01:01 4 Alice");
  REQUIRE(lines[4] == "02:00");
  REQUIRE(lines[5] == "1 20 01:01");
}

TEST_CASE("Error conditions: NotOpenYet, ClientUnknown, PlaceIsBusy", "[event_processor][errors]") {
  using namespace pc_club;

  std::ostringstream oss;
  auto* old_buf = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(1, 5, 60, 120);

  ep.process_event({.time = 30, .type = event_type::enter, .name = "Bob", .table = -1});
  ep.process_event({.time = 61, .type = event_type::take, .name = "Charlie", .table = 1});
  ep.process_event({.time = 61, .type = event_type::enter, .name = "Bob", .table = -1});
  ep.process_event({.time = 62, .type = event_type::take, .name = "Bob", .table = 1});
  ep.process_event({.time = 63, .type = event_type::take, .name = "Bob", .table = 1});

  ep.close();
  std::cout.rdbuf(old_buf);

  auto lines = split_lines(oss.str());

  REQUIRE(lines[1] == "00:30 1 Bob");
  REQUIRE(lines[2] == "00:30 13 NotOpenYet");

  REQUIRE(lines[3] == "01:01 2 Charlie 1");
  REQUIRE(lines[4] == "01:01 13 ClientUnknown");

  REQUIRE(lines[5] == "01:01 1 Bob");
  REQUIRE(lines[6] == "01:02 2 Bob 1");
  REQUIRE(lines[7] == "01:03 2 Bob 1");
  REQUIRE(lines[8] == "01:03 13 PlaceIsBusy");
}

TEST_CASE("Wait before enter yields ClientUnknown", "[wait][errors]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(2, 5, 0, 100);
  ep.process_event({.time = 10, .type = event_type::wait, .name = "Diana", .table = -1});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[1] == "00:10 3 Diana");
  REQUIRE(lines[2] == "00:10 13 ClientUnknown");
}

TEST_CASE("Wait when tables free prints ICanWaitNoLonger!", "[wait]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(2, 5, /*open*/ 0, /*close*/ 100);
  ep.process_event({.time = 5, .type = event_type::enter, .name = "Eve", .table = -1});
  ep.process_event({.time = 6, .type = event_type::wait, .name = "Eve", .table = -1});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[1] == "00:05 1 Eve");
  REQUIRE(lines[2] == "00:06 3 Eve");
  REQUIRE(lines[3] == "00:06 13 ICanWaitNoLonger!");
}

TEST_CASE("Queue overflow: more waiters than tables", "[wait][errors]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(1, 5, 0, 100);
  ep.process_event({.time = 0, .type = event_type::enter, .name = "Alice", .table = -1});
  ep.process_event({.time = 1, .type = event_type::enter, .name = "Bob", .table = -1});
  ep.process_event({.time = 2, .type = event_type::enter, .name = "Carol", .table = -1});
  ep.process_event({.time = 3, .type = event_type::take, .name = "Alice", .table = 1});
  ep.process_event({.time = 4, .type = event_type::wait, .name = "Bob", .table = -1});
  ep.process_event({.time = 5, .type = event_type::wait, .name = "Carol", .table = -1});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[6] == "00:05 3 Carol");
  REQUIRE(lines[7] == "00:05 11 Carol");
}

TEST_CASE("Assign next from queue when table freed", "[assign_next]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(1, 10, 0, 100);
  ep.process_event({.time = 0, .type = event_type::enter, .name = "A", .table = -1});
  ep.process_event({.time = 0, .type = event_type::take, .name = "A", .table = 1});
  ep.process_event({.time = 1, .type = event_type::enter, .name = "B", .table = -1});
  ep.process_event({.time = 2, .type = event_type::wait, .name = "B", .table = -1});
  ep.process_event({.time = 3, .type = event_type::enter, .name = "C", .table = -1});
  ep.process_event({.time = 4, .type = event_type::wait, .name = "C", .table = -1});
  ep.process_event({.time = 10, .type = event_type::leave, .name = "A", .table = -1});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[8] == "00:10 4 A");
  REQUIRE(lines[9] == "00:10 12 B 1");
}

TEST_CASE("Leave before enter yields ClientUnknown", "[leave][errors]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(1, 5, 0, 100);
  ep.process_event({.time = 20, .type = event_type::leave, .name = "Zoe", .table = -1});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[1] == "00:20 4 Zoe");
  REQUIRE(lines[2] == "00:20 13 ClientUnknown");
}

TEST_CASE("Multiple tables revenue and usage aggregation", "[close][revenue][usage]") {
  using namespace pc_club;
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());

  event_processor ep(2, 7, 0, 180);
  ep.process_event({.time = 0, .type = event_type::enter, .name = "X", .table = -1});
  ep.process_event({.time = 0, .type = event_type::take, .name = "X", .table = 1});
  ep.process_event({.time = 61, .type = event_type::leave, .name = "X", .table = -1});
  ep.process_event({.time = 30, .type = event_type::enter, .name = "Y", .table = -1});
  ep.process_event({.time = 30, .type = event_type::take, .name = "Y", .table = 2});

  ep.close();
  std::cout.rdbuf(old);

  auto lines = split_lines(oss.str());
  REQUIRE(lines[lines.size() - 2] == "1 14 01:01");
  REQUIRE(lines[lines.size() - 1] == "2 21 02:30");
}
