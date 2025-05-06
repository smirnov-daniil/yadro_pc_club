#include "event_processor.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
std::int32_t parse_time(const std::string& str) {
  if (str.size() != 5) {
    return -1;
  }
  if (str[2] != ':') {
    return -1;
  }
  std::int32_t time = 0;
  try {
    if (std::int32_t h = std::stoi(str.substr(0, 2)); h >= 0 && h <= 23) {
      time += h * 60;
    } else {
      return -1;
    }
    if (std::int32_t m = std::stoi(str.substr(3, 2)); m >= 0 && m <= 59) {
      time += m;
    } else {
      return -1;
    }
  } catch (...) {
    return -1;
  }
  return time;
}

bool get_event(const std::string& line, std::int32_t tables, std::vector<std::string> tokens, pc_club::event& e) {
  if (tokens.size() != 3 && tokens.size() != 4) {
    return false;
  }

  if (tokens[1].size() != 1 || tokens[1][0] < '1' || tokens[1][0] > '4') {
    return false;
  }
  if (!std::all_of(tokens[2].begin(), tokens[2].end(), [](unsigned char c) {
        return std::islower(c) || std::isdigit(c) || c == '_';
      })) {
    return false;
  }
  std::int32_t table = -1;
  if (tokens.size() == 4) {
    if (tokens[1][0] != '2') {
      return false;
    }
    if (std::all_of(tokens[3].begin(), tokens[3].end(), [](unsigned char c) { return std::isdigit(c); })) {
      table = std::stoi(tokens[3]);
      if (table < 1 || table > tables) {
        return false;
      }
    } else {
      return false;
    }
  } else if (tokens[1][0] == '2') {
    return false;
  }
  e = {
      .time = parse_time(tokens[0]),
      .type = static_cast<pc_club::event_type>(tokens[1][0] - '0'),
      .name = tokens[2],
      .table = table
  };
  if (e.time == -1) {
    return false;
  }
  return true;
}

bool get_parameters(
    std::fstream& fin,
    std::string line,
    std::int32_t& open,
    std::int32_t& close,
    std::int32_t& tables,
    std::int32_t& price
) {
  std::getline(fin, line);
  try {
    tables = std::stoi(line);
  } catch (...) {
    std::cout << line << '\n';
    return false;
  }
  if (tables <= 0) {
    std::cout << line << '\n';
    return false;
  }

  std::getline(fin, line);
  open = parse_time(line.substr(0, 5));
  close = parse_time(line.substr(6, 5));

  if (open > close || open == -1 || close == -1) {
    std::cout << line << '\n';
    return false;
  }

  std::getline(fin, line);
  try {
    price = std::stoi(line);
  } catch (...) {
    std::cout << line << '\n';
    return false;
  }
  if (price <= 0) {
    std::cout << line << '\n';
    return false;
  }
  return true;
}
} // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <path_to_file>\n";
    return 1;
  }

  auto fin = std::fstream(argv[1], std::ios::in);

  std::string line;

  std::int32_t open = 0, close = 0, tables = 0, price = 0;

  if (!get_parameters(fin, line, open, close, tables, price)) {
    return 1;
  }

  std::vector<pc_club::event> events;
  while (std::getline(fin, line)) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
      tokens.push_back(token);
    }
    pc_club::event e;
    if (!get_event(line, tables, tokens, e)) {
      std::cout << line << '\n';
      return 1;
    }
    events.emplace_back(e);
  }

  pc_club::event_processor ep(tables, price, open, close);
  for (const auto& e : events) {
    ep.process_event(e);
  }
  ep.close();

  return 0;
}