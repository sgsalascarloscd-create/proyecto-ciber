#include "config.h"
#include "controllers/incident_controller.h"
#include "controllers/swagger_controller.h"
#include "controllers/soc_controller.h"
#include "services/security_service.h"
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
  auto soc_ctrl = std::make_shared<SocController>(db);
  auto swagger = std::make_shared<SwaggerController>();

  drogon::app()
      .registerPreRoutingAdvice([](const drogon::HttpRequestPtr &req,
                                   drogon::FilterCallback &&stop,
                                   drogon::FilterChainCallback &&pass) {
        // 1. CORS Preflight
        if (req->method() == drogon::Options) {
          auto resp = drogon::HttpResponse::newHttpResponse();
          resp->addHeader("Access-Control-Allow-Origin", "*");
          resp->addHeader("Access-Control-Allow-Methods",
                          "GET, POST, PUT, DELETE, OPTIONS");
          resp->addHeader("Access-Control-Allow-Headers",
                          "Content-Type, Authorization, X-Requested-With, X-SOC-Signature");
          resp->addHeader("Access-Control-Max-Age",
                          "86400"); // Cache preflight for 24 hours

          // Terminate the chain early and send the 200 OK for OPTIONS
          stop(resp);
          return;
        }

        // 2. JWT Verification Filter for rules and get events
        std::string path = req->path();
        if (path == "/api/v1/aws/rules" || (path == "/api/v1/aws/events" && req->method() == drogon::Get)) {
          std::string auth = req->getHeader("Authorization");
          std::string token;
          if (auth.rfind("Bearer ", 0) == 0) {
            token = auth.substr(7);
          }
          std::string user;
          if (token.empty() || !SecurityService::verify_jwt(token, user)) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("{\"error\":\"Unauthorized. Valid JWT token is required.\"}");
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            stop(resp);
            return;
          }
        }

        pass(); // Pass actual requests (GET, POST, etc.) down the line
      })
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
          "/api/v1/auth/login",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            soc_ctrl->login(req, std::move(callback));
          },
          {drogon::Post})
      .registerHandler(
          "/api/v1/aws/events",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            if (req->method() == drogon::Post) {
              soc_ctrl->ingestEvent(req, std::move(callback));
            } else {
              soc_ctrl->getEvents(req, std::move(callback));
            }
          },
          {drogon::Post, drogon::Get})
      .registerHandler(
          "/api/v1/aws/rules",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            if (req->method() == drogon::Post) {
              soc_ctrl->updateRule(req, std::move(callback));
            } else {
              soc_ctrl->getRules(req, std::move(callback));
            }
          },
          {drogon::Post, drogon::Get})
      .registerHandler(
          "/api/v1/incidents/{1}/timeline",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int incident_id) {
            soc_ctrl->getTimeline(req, std::move(callback), incident_id);
          },
          {drogon::Get})
      .registerHandler(
          "/api/v1/incidents/{1}/evidence",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int incident_id) {
            soc_ctrl->getEvidence(req, std::move(callback), incident_id);
          },
          {drogon::Get})
      .registerHandler(
          "/api/v1/aws/simulate",
          [soc_ctrl](
              const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            soc_ctrl->runSimulation(req, std::move(callback));
          },
          {drogon::Post})
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
      .registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req,
                                     const drogon::HttpResponsePtr &resp) {
        if (req->path().rfind("/api/v1/", 0) == 0) {
          resp->addHeader("Access-Control-Allow-Origin", "*");
          resp->addHeader("Access-Control-Allow-Credentials", "false");
          resp->addHeader("X-Custom-Incident-Header", "Processed-Successfully");
        }
      })
      .setLogLevel(trantor::Logger::kInfo)
      .addListener("0.0.0.0", cfg.port)
      .setThreadNum(4)
      .run();

  return 0;
}

