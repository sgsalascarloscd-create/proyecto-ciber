#pragma once
#include "services/soc_service.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <memory>

class SocController {
public:
  SocController(std::shared_ptr<Database> db);

  // Authentication
  void login(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  // Event Ingestion (webhook from Lambda)
  void ingestEvent(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  // Get Ingested Events
  void getEvents(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  // Rule management
  void getRules(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void updateRule(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  // Timeline & Evidence details
  void getTimeline(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                   int incident_id);
  void getEvidence(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                   int incident_id);

  // Simulation Trigger
  void runSimulation(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);

private:
  static drogon::HttpResponsePtr make_response(const nlohmann::json &data,
                                               drogon::HttpStatusCode code = drogon::k200OK);
  static drogon::HttpResponsePtr make_error(const std::string &message,
                                            drogon::HttpStatusCode code);

  SocService service_;
  std::shared_ptr<Database> db_;
};
