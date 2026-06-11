#pragma once
#include "models/incident.h"
#include "models/soc_models.h"
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
#include <vector>

class Database {
public:
  using Storage = decltype(sqlite_orm::make_storage(
      std::string{}, make_incident_table(), make_aws_event_table(),
      make_ioc_table(), make_correlation_rule_table(), make_evidence_table(),
      make_timeline_table(), make_notification_table()));

  explicit Database(std::string db_path)
      : storage_{sqlite_orm::make_storage(
            std::move(db_path), make_incident_table(), make_aws_event_table(),
            make_ioc_table(), make_correlation_rule_table(),
            make_evidence_table(), make_timeline_table(),
            make_notification_table())} {
    auto result = storage_.sync_schema();
    for (auto &[table_name, status] : result) {
      switch (status) {
      case sqlite_orm::sync_schema_result::new_table_created:
        spdlog::info("Created table: {}", table_name);
        break;
      case sqlite_orm::sync_schema_result::already_in_sync:
        spdlog::info("Table already in sync: {}", table_name);
        break;
      default:
        spdlog::info("Synced table: {} (status={})", table_name,
                     static_cast<int>(status));
        break;
      }
    }
  }

  std::vector<Incident> get_all() {
    return storage_.get_all<Incident>();
  }

  std::optional<Incident> get_by_id(int id) {
    auto ptr = storage_.get_pointer<Incident>(id);
    if (ptr)
      return std::move(*ptr);
    return std::nullopt;
  }

  int create(Incident &incident) {
    auto now = std::time(nullptr);
    incident.created_at = now;
    incident.updated_at = now;
    return storage_.insert(incident);
  }

  void update(const Incident &incident) {
    auto copy = incident;
    copy.updated_at = std::time(nullptr);
    storage_.update(copy);
  }

  void remove(int id) {
    storage_.remove<Incident>(id);
  }

  Storage &storage() { return storage_; }
  const Storage &storage() const { return storage_; }

private:
  Storage storage_;
};

