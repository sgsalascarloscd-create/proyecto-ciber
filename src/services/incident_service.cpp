#include "incident_service.h"

IncidentService::IncidentService(Database &db) : db_(db) {}

std::vector<Incident> IncidentService::getAll() { return db_.get_all(); }

std::optional<Incident> IncidentService::getById(int id) {
  return db_.get_by_id(id);
}

int IncidentService::create(const nlohmann::json &body) {
  if (!body.contains("description") || !body["description"].is_string() ||
      body["description"].get<std::string>().empty()) {
    throw std::invalid_argument(
        "description is required and must be a non-empty string");
  }
  if (!body.contains("actor") || !body["actor"].is_string() ||
      body["actor"].get<std::string>().empty()) {
    throw std::invalid_argument(
        "actor is required and must be a non-empty string");
  }

  Incident inc;
  inc.description = body["description"].get<std::string>();
  inc.actor = body["actor"].get<std::string>();
  inc.threat = body.contains("threat") && body["threat"].is_string()
                   ? body["threat"].get<std::string>()
                   : "";
  inc.responsible = body.contains("responsible") && body["responsible"].is_string()
                        ? body["responsible"].get<std::string>()
                        : "";
  inc.status = body.contains("status") && body["status"].is_string()
                   ? body["status"].get<std::string>()
                   : "open";
  inc.priority = body.contains("priority") && body["priority"].is_number_integer()
                     ? body["priority"].get<int>()
                     : 0;

  return db_.create(inc);
}

void IncidentService::update(int id, const nlohmann::json &body) {
  auto inc = db_.get_by_id(id);
  if (!inc) {
    throw std::runtime_error("Incident not found");
  }

  if (body.contains("description") && body["description"].is_string())
    inc->description = body["description"].get<std::string>();
  if (body.contains("actor") && body["actor"].is_string())
    inc->actor = body["actor"].get<std::string>();
  if (body.contains("threat") && body["threat"].is_string())
    inc->threat = body["threat"].get<std::string>();
  if (body.contains("responsible") && body["responsible"].is_string())
    inc->responsible = body["responsible"].get<std::string>();
  if (body.contains("status") && body["status"].is_string())
    inc->status = body["status"].get<std::string>();
  if (body.contains("priority") && body["priority"].is_number_integer())
    inc->priority = body["priority"].get<int>();

  db_.update(*inc);
}

void IncidentService::remove(int id) {
  auto inc = db_.get_by_id(id);
  if (!inc) {
    throw std::runtime_error("Incident not found");
  }
  db_.remove(id);
}
