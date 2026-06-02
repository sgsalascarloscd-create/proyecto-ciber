#pragma once
#include "services/incident_service.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <functional>
#include <memory>

class IncidentController {
public:
  explicit IncidentController(std::shared_ptr<Database> db);

  void getAll(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void getById(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               int id);
  void create(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void update(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int id);
  void remove(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int id);

private:
  static nlohmann::json to_json(const Incident &inc);
  static drogon::HttpResponsePtr make_response(const nlohmann::json &data,
                                               drogon::HttpStatusCode code = drogon::k200OK);
  static drogon::HttpResponsePtr make_error(const std::string &message,
                                            drogon::HttpStatusCode code);

  IncidentService service_;
  std::shared_ptr<Database> db_;
};
