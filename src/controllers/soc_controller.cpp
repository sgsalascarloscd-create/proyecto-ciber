#include "soc_controller.h"
#include "services/security_service.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

SocController::SocController(std::shared_ptr<Database> db)
    : service_(*db), db_(std::move(db)) {}

static nlohmann::json safe_parse(const std::string &str) {
  try {
    return nlohmann::json::parse(str);
  } catch (...) {
    return str;
  }
}

static nlohmann::json event_to_json(const AwsEvent &ev) {
  return {{"id", ev.id},
          {"event_id", ev.event_id},
          {"event_name", ev.event_name},
          {"event_source", ev.event_source},
          {"event_time", ev.event_time},
          {"user_identity", safe_parse(ev.user_identity)},
          {"request_parameters", safe_parse(ev.request_parameters)},
          {"response_elements", safe_parse(ev.response_elements)},
          {"source_ip_address", ev.source_ip_address},
          {"user_agent", ev.user_agent},
          {"processed", ev.processed}};
}

static nlohmann::json rule_to_json(const CorrelationRule &r) {
  return {{"id", r.id},
          {"name", r.name},
          {"description", r.description},
          {"conditions", safe_parse(r.conditions)},
          {"time_window", r.time_window},
          {"severity", r.severity},
          {"active", r.active}};
}

static nlohmann::json evidence_to_json(const Evidence &ev) {
  return {{"id", ev.id},
          {"incident_id", ev.incident_id},
          {"aws_event_id", ev.aws_event_id},
          {"details", ev.details},
          {"created_at", ev.created_at}};
}

static nlohmann::json timeline_to_json(const IncidentTimeline &t) {
  return {{"id", t.id},
          {"incident_id", t.incident_id},
          {"event_time", t.event_time},
          {"title", t.title},
          {"description", t.description},
          {"created_at", t.created_at}};
}

drogon::HttpResponsePtr SocController::make_response(const nlohmann::json &data,
                                                     drogon::HttpStatusCode code) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(code);
  resp->setBody(data.dump());
  resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  resp->addHeader("Access-Control-Allow-Origin", "*");
  return resp;
}

drogon::HttpResponsePtr SocController::make_error(const std::string &message,
                                                  drogon::HttpStatusCode code) {
  return make_response({{"error", message}}, code);
}

void SocController::login(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto body = req->getBody();
    if (body.empty()) {
      callback(make_error("Empty request body", drogon::k400BadRequest));
      return;
    }

    auto js = nlohmann::json::parse(body.data(), body.data() + body.size());
    std::string username = js.value("username", "");
    std::string password = js.value("password", "");

    const char *env_user = std::getenv("ADMIN_USER");
    const char *env_pass = std::getenv("ADMIN_PASSWORD");

    std::string correct_user = env_user ? env_user : "admin";
    std::string correct_pass = env_pass ? env_pass : "admin-password";

    if (username == correct_user && password == correct_pass) {
      std::string token = SecurityService::generate_jwt(username);
      callback(make_response({{"token", token}}));
    } else {
      callback(make_error("Invalid credentials", drogon::k401Unauthorized));
    }
  } catch (...) {
    callback(make_error("Invalid request format", drogon::k400BadRequest));
  }
}

void SocController::ingestEvent(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto body = req->getBody();
    std::string body_str(body.data(), body.size());

    // Check signature verification bypass toggle (useful for simulation or testing)
    const char *disable_sig = std::getenv("DISABLE_SIGNATURE_CHECK");
    bool enforce_sig = !(disable_sig && std::string(disable_sig) == "true");

    if (enforce_sig) {
      std::string sig = req->getHeader("X-SOC-Signature");
      if (sig.empty() || !SecurityService::verify_webhook_signature(body_str, sig)) {
        spdlog::warn("Ingestion rejected: invalid or missing signature header.");
        callback(make_error("Invalid or missing X-SOC-Signature", drogon::k401Unauthorized));
        return;
      }
    }

    auto js = nlohmann::json::parse(body_str);
    int id = service_.ingest_event(js);

    callback(make_response({{"status", "ingested"}, {"id", id}}, drogon::k201Created));
  } catch (const nlohmann::json::parse_error &e) {
    callback(make_error(std::string("Malformed JSON payload: ") + e.what(), drogon::k400BadRequest));
  } catch (const std::exception &e) {
    spdlog::error("Ingestion failure: {}", e.what());
    callback(make_error("Internal processing error", drogon::k500InternalServerError));
  }
}

void SocController::getEvents(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto events = service_.get_all_events();
    auto arr = nlohmann::json::array();
    for (const auto &ev : events) {
      arr.push_back(event_to_json(ev));
    }
    callback(make_response(arr));
  } catch (const std::exception &e) {
    spdlog::error("getEvents failure: {}", e.what());
    callback(make_error("Internal server error", drogon::k500InternalServerError));
  }
}

void SocController::getRules(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto rules = service_.get_all_rules();
    auto arr = nlohmann::json::array();
    for (const auto &r : rules) {
      arr.push_back(rule_to_json(r));
    }
    callback(make_response(arr));
  } catch (const std::exception &e) {
    spdlog::error("getRules failure: {}", e.what());
    callback(make_error("Internal server error", drogon::k500InternalServerError));
  }
}

void SocController::updateRule(const drogon::HttpRequestPtr &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto body = req->getBody();
    auto js = nlohmann::json::parse(body.data(), body.data() + body.size());

    if (js.contains("toggle_id") && js.contains("active")) {
      int id = js["toggle_id"].get<int>();
      bool active = js["active"].get<bool>();
      service_.toggle_rule(id, active);
      callback(make_response({{"status", "updated"}}));
      return;
    }

    service_.create_or_update_rule(js);
    callback(make_response({{"status", "saved"}}));
  } catch (...) {
    callback(make_error("Invalid rule payload", drogon::k400BadRequest));
  }
}

void SocController::getTimeline(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                int incident_id) {
  try {
    auto timeline = service_.get_timeline_for_incident(incident_id);
    auto arr = nlohmann::json::array();
    for (const auto &t : timeline) {
      arr.push_back(timeline_to_json(t));
    }
    callback(make_response(arr));
  } catch (const std::exception &e) {
    spdlog::error("getTimeline failure: {}", e.what());
    callback(make_error("Internal server error", drogon::k500InternalServerError));
  }
}

void SocController::getEvidence(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                int incident_id) {
  try {
    auto evidence = service_.get_evidence_for_incident(incident_id);
    auto arr = nlohmann::json::array();
    for (const auto &ev : evidence) {
      arr.push_back(evidence_to_json(ev));
    }
    callback(make_response(arr));
  } catch (const std::exception &e) {
    spdlog::error("getEvidence failure: {}", e.what());
    callback(make_error("Internal server error", drogon::k500InternalServerError));
  }
}

void SocController::runSimulation(const drogon::HttpRequestPtr &req,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  try {
    auto body = req->getBody();
    auto js = nlohmann::json::parse(body.data(), body.data() + body.size());
    std::string scenario = js.value("scenario", "capital_one");

    service_.trigger_simulation(scenario);
    callback(make_response({{"status", "simulation_completed"}, {"scenario", scenario}}));
  } catch (const std::exception &e) {
    spdlog::error("runSimulation failure: {}", e.what());
    callback(make_error("Internal simulation failure", drogon::k500InternalServerError));
  }
}
