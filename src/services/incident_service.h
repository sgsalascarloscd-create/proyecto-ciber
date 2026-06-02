#pragma once
#include "database.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <vector>

class IncidentService {
public:
  explicit IncidentService(Database &db);

  std::vector<Incident> getAll();
  std::optional<Incident> getById(int id);
  int create(const nlohmann::json &body);
  void update(int id, const nlohmann::json &body);
  void remove(int id);

private:
  Database &db_;
};
