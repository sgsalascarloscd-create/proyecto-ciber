// University Assignment - Incident Backend
#pragma once
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

struct AppConfig {
  int port = 8080;
  std::string db_path = "backend.db";
  std::string log_level = "info";
};

inline void load_dotenv(const std::string &path = ".env") {
  std::ifstream file(path);
  if (!file.is_open()) return;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    auto key = line.substr(0, eq);
    auto val = line.substr(eq + 1);
    setenv(key.c_str(), val.c_str(), 0);
  }
}

inline AppConfig load_config() {
  load_dotenv();

  AppConfig cfg;

  // 2. Parse Port from environment variable "PORT"
  if (const char *env_port = std::getenv("PORT")) {
    try {
      cfg.port = std::stoi(env_port);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Invalid PORT environment variable. Falling back "
                   "to default: "
                << cfg.port << '\n';
    }
  }

  // 3. Parse Database Path from environment variable "DB_PATH"
  if (const char *env_db = std::getenv("DB_PATH")) {
    cfg.db_path = env_db;
  }

  // 4. Parse Log Level from environment variable "LOG_LEVEL"
  if (const char *env_log = std::getenv("LOG_LEVEL")) {
    cfg.log_level = env_log;
  }

  return cfg;
}
