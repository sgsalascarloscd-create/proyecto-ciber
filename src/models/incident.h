#pragma once
#include <ctime>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

struct Incident {
  int id = 0;
  std::string description;
  std::string actor; // actor de amenza
  std::string threat; // type of threat
  std::string responsible;
  std::string status;
  int priority = 0;
  std::time_t created_at = 0;
  std::time_t updated_at = 0;
};

inline auto make_incident_table() {
  using namespace sqlite_orm;
  return make_table(
      "incidents",
      make_column("id", &Incident::id, primary_key().autoincrement()),
      make_column("description", &Incident::description),
      make_column("actor", &Incident::actor),
      make_column("threat", &Incident::threat),
      make_column("responsible", &Incident::responsible),
      make_column("status", &Incident::status),
      make_column("priority", &Incident::priority),
      make_column("created_at", &Incident::created_at),
      make_column("updated_at", &Incident::updated_at));
}
