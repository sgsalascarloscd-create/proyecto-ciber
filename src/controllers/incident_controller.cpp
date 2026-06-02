#include "incident_controller.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

IncidentController::IncidentController(std::shared_ptr<Database> db)
    : service_(*db), db_(std::move(db)) {}

nlohmann::json IncidentController::to_json(const Incident &inc) {
  return {{"id", inc.id},
          {"description", inc.description},
          {"actor", inc.actor},
          {"threat", inc.threat},
          {"responsible", inc.responsible},
          {"status", inc.status},
          {"priority", inc.priority},
          {"created_at", inc.created_at},
          {"updated_at", inc.updated_at}};
}

drogon::HttpResponsePtr
IncidentController::make_response(const nlohmann::json &data,
                                  drogon::HttpStatusCode code) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(code);
  resp->setBody(data.dump());
  resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  return resp;
}

drogon::HttpResponsePtr
IncidentController::make_error(const std::string &message,
                               drogon::HttpStatusCode code) {
  return make_response({{"error", message}}, code);
}

void IncidentController::getAll(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto incidents = service_.getAll();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &inc : incidents) {
      arr.push_back(to_json(inc));
    }
    callback(make_response(arr));
  } catch (const std::exception &e) {
    spdlog::error("getAll failed: {}", e.what());
    callback(make_error("Internal server error",
                        drogon::k500InternalServerError));
  }
}

void IncidentController::getById(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id) {
  try {
    auto inc = service_.getById(id);
    if (!inc) {
      callback(make_error("Incident not found", drogon::k404NotFound));
      return;
    }
    callback(make_response(to_json(*inc)));
  } catch (const std::exception &e) {
    spdlog::error("getById failed: {}", e.what());
    callback(make_error("Internal server error",
                        drogon::k500InternalServerError));
  }
}

void IncidentController::create(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto body = req->getBody();
    if (body.empty()) {
      callback(make_error("Request body is empty", drogon::k400BadRequest));
      return;
    }
    auto json =
        nlohmann::json::parse(body.data(), body.data() + body.size());
    int id = service_.create(json);
    callback(make_response({{"id", id}}, drogon::k201Created));
  } catch (const nlohmann::json::parse_error &) {
    callback(make_error("Invalid JSON in request body",
                        drogon::k400BadRequest));
  } catch (const std::invalid_argument &e) {
    callback(make_error(e.what(), drogon::k400BadRequest));
  } catch (const std::exception &e) {
    spdlog::error("create failed: {}", e.what());
    callback(make_error("Internal server error",
                        drogon::k500InternalServerError));
  }
}

void IncidentController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id) {
  try {
    auto body = req->getBody();
    if (body.empty()) {
      callback(make_error("Request body is empty", drogon::k400BadRequest));
      return;
    }
    auto json =
        nlohmann::json::parse(body.data(), body.data() + body.size());
    service_.update(id, json);
    auto inc = service_.getById(id);
    callback(make_response(to_json(*inc)));
  } catch (const nlohmann::json::parse_error &) {
    callback(make_error("Invalid JSON in request body",
                        drogon::k400BadRequest));
  } catch (const std::runtime_error &e) {
    callback(make_error(e.what(), drogon::k404NotFound));
  } catch (const std::exception &e) {
    spdlog::error("update failed: {}", e.what());
    callback(make_error("Internal server error",
                        drogon::k500InternalServerError));
  }
}

void IncidentController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id) {
  try {
    service_.remove(id);
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
  } catch (const std::runtime_error &e) {
    callback(make_error(e.what(), drogon::k404NotFound));
  } catch (const std::exception &e) {
    spdlog::error("remove failed: {}", e.what());
    callback(make_error("Internal server error",
                        drogon::k500InternalServerError));
  }
}
