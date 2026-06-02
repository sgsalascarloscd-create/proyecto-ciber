#pragma once
#include "database.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <vector>

inline void seed_incidents(Database &db) {
  const char *env = std::getenv("SEED_DB");
  if (!env || std::string(env) != "true") {
    spdlog::info("SEED_DB not set or not 'true', skipping seed");
    return;
  }

  auto existing = db.get_all();
  if (!existing.empty()) {
    spdlog::info("Database already has {} incidents, skipping seed",
                 existing.size());
    return;
  }

  auto make = [](const char *desc, const char *actor, const char *threat,
                 const char *resp, const char *status, int pri) {
    Incident inc;
    inc.description = desc;
    inc.actor = actor;
    inc.threat = threat;
    inc.responsible = resp;
    inc.status = status;
    inc.priority = pri;
    return inc;
  };

  std::vector<Incident> seeds;
  seeds.reserve(6);
  seeds.push_back(make("SQL injection detected on login endpoint", "anonymous",
                       "sql_injection", "security_team", "open", 5));
  seeds.push_back(make("Server CPU usage above 90% for 10 minutes",
                       "monitoring", "resource_exhaustion", "infra_team",
                       "in_progress", 4));
  seeds.push_back(
      make("Unauthorized access attempt from IP 10.0.0.1", "10.0.0.1",
           "brute_force", "security_team", "triaged", 3));
  seeds.push_back(make("Database replication lag exceeds 30 seconds",
                       "monitoring", "data_inconsistency", "infra_team", "open",
                       4));
  seeds.push_back(make("TLS certificate expires in 3 days", "monitoring",
                       "certificate_expiry", "devops_team", "in_progress", 2));
  seeds.push_back(make("Phishing campaign targeting employees",
                       "external_actor", "phishing", "security_team",
                       "resolved", 3));

  for (auto &inc : seeds) {
    db.create(inc);
  }
  spdlog::info("Seeded {} incidents into the database", seeds.size());
}
