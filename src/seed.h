#pragma once
#include "database.h"
#include "models/soc_models.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <vector>

inline void seed_correlation_rules(Database &db) {
  auto existing = db.storage().get_all<CorrelationRule>();
  if (!existing.empty()) {
    spdlog::info("Database already has {} correlation rules, skipping rules seed",
                 existing.size());
    return;
  }

  auto make = [](const char *name, const char *desc, const char *cond,
                 int window, const char *sev) {
    CorrelationRule r;
    r.name = name;
    r.description = desc;
    r.conditions = cond;
    r.time_window = window;
    r.severity = sev;
    r.active = 1;
    return r;
  };

  std::vector<CorrelationRule> rules;
  rules.push_back(make("SSRF contra EC2 Metadata Service",
                       "Uso de credenciales temporales EC2 desde una IP externa (posible compromiso por SSRF)",
                       "{\"type\":\"property\",\"event_name\":\"AssumeRole\",\"ec2_role\":true,\"external_ip\":true}",
                       60, "Critical"));

  rules.push_back(make("Uso anómalo de credenciales temporales IAM",
                       "Acceso anómalo detectado mediante credenciales temporales IAM desde IP pública",
                       "{\"type\":\"property\",\"temp_creds\":true,\"external_ip\":true}",
                       60, "High"));

  rules.push_back(make("Enumeración de recursos AWS",
                       "Detección de comandos secuenciales de enumeración (GetCallerIdentity + ListBuckets + GetObject)",
                       "{\"type\":\"sequence\",\"events\":[\"GetCallerIdentity\",\"ListBuckets\",\"GetObject\"]}",
                       60, "High"));

  rules.push_back(make("Acceso a buckets S3 sensibles",
                       "Acceso no autorizado a buckets S3 sensibles utilizando credenciales temporales desde IP externa",
                       "{\"type\":\"property\",\"event_name\":\"GetObject\",\"bucket\":\"capital-one-sensitive-data\",\"temp_creds\":true,\"external_ip\":true}",
                       60, "Critical"));

  rules.push_back(make("Descarga masiva de archivos S3",
                       "Descarga masiva de archivos desde buckets S3 en un corto periodo de tiempo (posible exfiltración)",
                       "{\"type\":\"threshold\",\"event_name\":\"GetObject\",\"limit\":5}",
                       10, "High"));

  rules.push_back(make("Modificación crítica de Security Group",
                       "Apertura crítica de puertos en Security Groups permitiendo tráfico global (0.0.0.0/0)",
                       "{\"type\":\"property\",\"event_name\":\"AuthorizeSecurityGroupIngress\",\"cidr\":\"0.0.0.0/0\"}",
                       60, "High"));

  rules.push_back(make("Cambios sospechosos en IAM",
                       "Creación de usuarios o asignación de permisos administrativos sospechosos en IAM",
                       "{\"type\":\"any_of\",\"events\":[\"CreateUser\",\"CreateAccessKey\",\"AttachUserPolicy\",\"PutUserPolicy\"]}",
                       60, "High"));

  for (auto &r : rules) {
    db.storage().insert(r);
  }
  spdlog::info("Seeded {} correlation rules into the database", rules.size());
}

inline void seed_incidents(Database &db) {
  // Seed the rules first
  seed_correlation_rules(db);

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

