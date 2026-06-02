#include "config.h"
#include "controllers/incident_controller.h"
#include "controllers/swagger_controller.h"
#include "database.h"
#include "seed.h"
#include <drogon/drogon.h>
#include <memory>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
  AppConfig cfg = load_config();

  spdlog::set_level(spdlog::level::from_str(cfg.log_level));
  spdlog::info("Starting server on port http://127.0.0.1:{}", cfg.port);

  auto db = std::make_shared<Database>(cfg.db_path);

  seed_incidents(*db);

  auto ctrl = std::make_shared<IncidentController>(db);
  auto swagger = std::make_shared<SwaggerController>();

  drogon::app()
      .registerHandler(
          "/api/v1/incidents",
          [ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            ctrl->getAll(req, std::move(callback));
          },
          {drogon::Get})
      .registerHandler(
          "/api/v1/incidents/{1}",
          [ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int id) { ctrl->getById(req, std::move(callback), id); },
          {drogon::Get})
      .registerHandler(
          "/api/v1/incidents",
          [ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            ctrl->create(req, std::move(callback));
          },
          {drogon::Post})
      .registerHandler(
          "/api/v1/incidents/{1}",
          [ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int id) { ctrl->update(req, std::move(callback), id); },
          {drogon::Put})
      .registerHandler(
          "/api/v1/incidents/{1}",
          [ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int id) { ctrl->remove(req, std::move(callback), id); },
          {drogon::Delete})
      .registerHandler(
          "/swagger",
          [swagger](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            swagger->serveIndex(req, std::move(callback));
          },
          {drogon::Get})
      .registerHandler(
          "/swagger/",
          [swagger](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            swagger->serveIndex(req, std::move(callback));
          },
          {drogon::Get})
      .registerHandler(
          "/swagger/openapi.json",
          [swagger](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            swagger->serveSpec(req, std::move(callback));
          },
          {drogon::Get})
      .setLogLevel(trantor::Logger::kInfo)
      .addListener("0.0.0.0", cfg.port)
      .setThreadNum(4)
      .run();

  return 0;
}
