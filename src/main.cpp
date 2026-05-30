// University Assignment - Incident Backend
#include "config.h"
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
  // Configuration
  AppConfig cfg = load_config();

  // Setup spdlog (console + file optional)
  spdlog::set_level(spdlog::level::from_str(cfg.log_level));
  spdlog::info("Starting server on port {}", cfg.port);
  spdlog::info(cfg.port);
  spdlog::info(cfg.db_path);
  spdlog::info(cfg.log_level);

  // Drogon application
  drogon::app()
      .setLogLevel(trantor::Logger::kInfo) // or kDebug etc.
      .addListener("0.0.0.0", cfg.port)
      .setThreadNum(4)
      .run();

  return 0;
}
