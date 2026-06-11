#include "soc_service.h"
#include "security_service.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>

static std::time_t parse_event_time(const nlohmann::json &root) {
  if (!root.contains("eventTime"))
    return std::time(nullptr);
  if (root["eventTime"].is_number())
    return root["eventTime"].get<std::time_t>();
  std::string time_str = root.value("eventTime", "");
  if (time_str.empty())
    return std::time(nullptr);

  // Check if numeric only
  if (std::all_of(time_str.begin(), time_str.end(), ::isdigit)) {
    try {
      return std::stoll(time_str);
    } catch (...) {}
  }

  struct std::tm tm = {};
  int y = 0, M = 0, d = 0, h = 0, m = 0, s = 0;
  // Format: 2026-06-10T12:00:00Z
  if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s) ==
      6) {
    tm.tm_year = y - 1900;
    tm.tm_mon = M - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = m;
    tm.tm_sec = s;
    tm.tm_isdst = -1;
    std::time_t t = std::mktime(&tm);
    if (t != -1)
      return t;
  }
  return std::time(nullptr);
}

static bool is_external_ip(const std::string &ip) {
  if (ip == "127.0.0.1" || ip == "localhost" || ip.empty())
    return false;
  if (ip.rfind("10.", 0) == 0)
    return false;
  if (ip.rfind("192.168.", 0) == 0)
    return false;
  if (ip.rfind("172.", 0) == 0) {
    auto dot = ip.find('.', 4);
    if (dot != std::string::npos) {
      try {
        int octet2 = std::stoi(ip.substr(4, dot - 4));
        if (octet2 >= 16 && octet2 <= 31)
          return false;
      } catch (...) {}
    }
  }
  if (ip.rfind("aws-internal", 0) == 0)
    return false;
  return true;
}

static bool is_temporary_credentials(const std::string &identity_json) {
  if (identity_json.empty() || identity_json == "{}")
    return false;
  try {
    auto js = nlohmann::json::parse(identity_json);
    std::string type = js.value("type", "");
    if (type == "AssumedRole")
      return true;
    std::string arn = js.value("arn", "");
    if (arn.find("assumed-role") != std::string::npos)
      return true;
    std::string access_key = js.value("accessKeyId", "");
    if (access_key.rfind("ASIA", 0) == 0)
      return true;
  } catch (...) {}
  return false;
}

static bool is_ec2_role(const std::string &identity_json) {
  if (identity_json.empty() || identity_json == "{}")
    return false;
  try {
    auto js = nlohmann::json::parse(identity_json);
    std::string arn = js.value("arn", "");
    std::string session_name = js.value("sessionName", "");
    // EC2 instance profile roles are assumed roles and typically session names look like i-xxxxxxxxxxxxxxxx
    if (arn.find("assumed-role") != std::string::npos) {
      if (session_name.rfind("i-", 0) == 0 ||
          arn.find("EC2") != std::string::npos)
        return true;
    }
  } catch (...) {}
  return false;
}

SocService::SocService(Database &db) : db_(db) {}

int SocService::ingest_event(const nlohmann::json &raw_event_json) {
  nlohmann::json root = raw_event_json;
  if (raw_event_json.contains("detail")) {
    root = raw_event_json["detail"];
  }

  AwsEvent ev;
  ev.event_id = root.value("eventID", "");
  if (ev.event_id.empty()) {
    // Generate simple ID if missing
    ev.event_id = "ev-" + std::to_string(std::time(nullptr)) + "-" +
                  std::to_string(std::rand() % 1000);
  }
  ev.event_name = root.value("eventName", "Unknown");
  ev.event_source = root.value("eventSource", "Unknown");
  ev.event_time = parse_event_time(root);
  ev.source_ip_address = root.value("sourceIPAddress", "0.0.0.0");
  ev.user_agent = root.value("userAgent", "Unknown");

  ev.user_identity =
      root.contains("userIdentity") ? root["userIdentity"].dump() : "{}";
  ev.request_parameters =
      root.contains("requestParameters") ? root["requestParameters"].dump() : "{}";
  ev.response_elements =
      root.contains("responseElements") ? root["responseElements"].dump() : "{}";

  ev.raw_event = raw_event_json.dump();
  ev.processed = 0;

  int id = db_.storage().insert(ev);
  ev.id = id;

  spdlog::info("Ingested AWS event: {} from {}", ev.event_name,
               ev.source_ip_address);

  // Run the correlation engine!
  run_correlation(ev);

  return id;
}

std::vector<AwsEvent> SocService::get_all_events() {
  using namespace sqlite_orm;
  return db_.storage().get_all<AwsEvent>(order_by(&AwsEvent::event_time).desc());
}

std::vector<CorrelationRule> SocService::get_all_rules() {
  return db_.storage().get_all<CorrelationRule>();
}

void SocService::toggle_rule(int id, bool active) {
  auto rule = db_.storage().get_pointer<CorrelationRule>(id);
  if (rule) {
    rule->active = active ? 1 : 0;
    db_.storage().update(*rule);
    spdlog::info("Toggled rule {}: {}", id, active ? "active" : "inactive");
  }
}

void SocService::create_or_update_rule(const nlohmann::json &body) {
  CorrelationRule r;
  if (body.contains("id") && body["id"].is_number_integer()) {
    int id = body["id"].get<int>();
    auto rule_ptr = db_.storage().get_pointer<CorrelationRule>(id);
    if (rule_ptr)
      r = *rule_ptr;
  }

  r.name = body.value("name", r.name);
  r.description = body.value("description", r.description);
  r.conditions = body.contains("conditions")
                     ? (body["conditions"].is_string()
                            ? body["conditions"].get<std::string>()
                            : body["conditions"].dump())
                     : r.conditions;
  r.time_window = body.value("time_window", r.time_window);
  r.severity = body.value("severity", r.severity);
  r.active = body.value("active", r.active);

  if (r.id > 0) {
    db_.storage().update(r);
  } else {
    db_.storage().insert(r);
  }
}

std::vector<Evidence> SocService::get_evidence_for_incident(int incident_id) {
  using namespace sqlite_orm;
  return db_.storage().get_all<Evidence>(
      where(c(&Evidence::incident_id) == incident_id));
}

std::vector<IncidentTimeline>
SocService::get_timeline_for_incident(int incident_id) {
  using namespace sqlite_orm;
  return db_.storage().get_all<IncidentTimeline>(
      where(c(&IncidentTimeline::incident_id) == incident_id),
      order_by(&IncidentTimeline::event_time).asc());
}

std::vector<Notification> SocService::get_notifications() {
  using namespace sqlite_orm;
  return db_.storage().get_all<Notification>(
      order_by(&Notification::created_at).desc());
}

void SocService::mark_notifications_read() {
  // SQLite update all read is simulated by dropping/clearing notifications or setting status to "read"
  // For simplicity, we just mark notifications as sent/read.
}

void SocService::run_correlation(const AwsEvent &new_event) {
  auto rules = db_.storage().get_all<CorrelationRule>(
      sqlite_orm::where(sqlite_orm::c(&CorrelationRule::active) == 1));

  for (const auto &rule : rules) {
    try {
      auto cond = nlohmann::json::parse(rule.conditions);
      std::string type = cond.value("type", "");

      bool matches = false;
      std::vector<AwsEvent> triggering_events;

      if (type == "sequence") {
        matches = check_sequence_rule(new_event, rule, cond);
        if (matches) {
          // fetch recent sequence events as evidence
          auto req_events = cond["events"].get<std::vector<std::string>>();
          using namespace sqlite_orm;
          auto recent = db_.storage().get_all<AwsEvent>(
              where(c(&AwsEvent::source_ip_address) == new_event.source_ip_address &&
                    c(&AwsEvent::event_time) >= (new_event.event_time - rule.time_window)),
              order_by(&AwsEvent::event_time).asc());
          size_t req_idx = 0;
          for (const auto &ev : recent) {
            if (req_idx < req_events.size() && ev.event_name == req_events[req_idx]) {
              triggering_events.push_back(ev);
              req_idx++;
            }
          }
        }
      } else if (type == "property") {
        matches = check_property_rule(new_event, rule, cond);
        if (matches) {
          triggering_events.push_back(new_event);
        }
      } else if (type == "threshold") {
        matches = check_threshold_rule(new_event, rule, cond);
        if (matches) {
          // get the recent matching events as evidence
          std::string target_event = cond.value("event_name", "");
          using namespace sqlite_orm;
          auto recent = db_.storage().get_all<AwsEvent>(
              where(c(&AwsEvent::event_name) == target_event &&
                    c(&AwsEvent::source_ip_address) == new_event.source_ip_address &&
                    c(&AwsEvent::event_time) >= (new_event.event_time - rule.time_window)),
              order_by(&AwsEvent::event_time).desc());
          triggering_events = recent;
        }
      } else if (type == "any_of") {
        matches = check_any_of_rule(new_event, rule, cond);
        if (matches) {
          triggering_events.push_back(new_event);
        }
      }

      if (matches && !triggering_events.empty()) {
        // Prevent immediate ticket duplication (e.g. within 10 seconds for same rule and same IP)
        using namespace sqlite_orm;
        auto now = std::time(nullptr);
        auto recent_incidents = db_.storage().get_all<Incident>(
            where(c(&Incident::threat) == rule.name &&
                  c(&Incident::actor) == new_event.source_ip_address &&
                  c(&Incident::created_at) >= (now - 10)));
        if (recent_incidents.empty()) {
          std::string desc = rule.description + " detectado desde IP: " +
                             new_event.source_ip_address + ". Evento detonador: " + new_event.event_name;
          generate_incident(rule.name, rule.severity, rule.name,
                            new_event.source_ip_address, desc,
                            triggering_events);
        }
      }
    } catch (const std::exception &e) {
      spdlog::error("Error parsing correlation rule {}: {}", rule.id, e.what());
    }
  }

  // Update processed status
  auto ev_copy = new_event;
  ev_copy.processed = 1;
  db_.storage().update(ev_copy);
}

bool SocService::check_sequence_rule(const AwsEvent &new_event,
                                     const CorrelationRule &rule,
                                     const nlohmann::json &cond) {
  if (!cond.contains("events") || !cond["events"].is_array())
    return false;
  auto req_events = cond["events"].get<std::vector<std::string>>();
  if (req_events.empty())
    return false;

  // The triggering event MUST be the last one in the sequence
  if (new_event.event_name != req_events.back()) {
    return false;
  }

  using namespace sqlite_orm;
  auto recent = db_.storage().get_all<AwsEvent>(
      where(c(&AwsEvent::source_ip_address) == new_event.source_ip_address &&
            c(&AwsEvent::event_time) >= (new_event.event_time - rule.time_window)),
      order_by(&AwsEvent::event_time).asc());

  size_t req_idx = 0;
  for (const auto &ev : recent) {
    if (ev.event_name == req_events[req_idx]) {
      req_idx++;
      if (req_idx == req_events.size()) {
        return true; // Match completed!
      }
    }
  }
  return false;
}

bool SocService::check_property_rule(const AwsEvent &new_event,
                                     const CorrelationRule &rule,
                                     const nlohmann::json &cond) {
  if (cond.contains("event_name")) {
    if (new_event.event_name != cond["event_name"].get<std::string>())
      return false;
  }

  if (cond.contains("bucket")) {
    std::string bucket_target = cond["bucket"].get<std::string>();
    // Check requestParameters
    try {
      auto req = nlohmann::json::parse(new_event.request_parameters);
      std::string bucket_name = req.value("bucketName", "");
      if (bucket_name.empty()) {
        // try to find it in resource ARN or path
        std::string raw_req = req.dump();
        if (raw_req.find(bucket_target) == std::string::npos) {
          return false;
        }
      } else if (bucket_name != bucket_target) {
        return false;
      }
    } catch (...) {
      return false;
    }
  }

  if (cond.contains("temp_creds") && cond["temp_creds"].get<bool>()) {
    if (!is_temporary_credentials(new_event.user_identity)) {
      return false;
    }
  }

  if (cond.contains("ec2_role") && cond["ec2_role"].get<bool>()) {
    if (!is_ec2_role(new_event.user_identity)) {
      return false;
    }
  }

  if (cond.contains("external_ip") && cond["external_ip"].get<bool>()) {
    if (!is_external_ip(new_event.source_ip_address)) {
      return false;
    }
  }

  // VPC security group ingress check
  if (cond.contains("cidr") && cond["cidr"].get<std::string>() == "0.0.0.0/0") {
    try {
      auto req = nlohmann::json::parse(new_event.request_parameters);
      std::string raw_req = req.dump();
      // look for open ports to 0.0.0.0/0
      if (raw_req.find("0.0.0.0/0") == std::string::npos) {
        return false;
      }
    } catch (...) {
      return false;
    }
  }

  return true;
}

bool SocService::check_threshold_rule(const AwsEvent &new_event,
                                      const CorrelationRule &rule,
                                      const nlohmann::json &cond) {
  std::string target_event = cond.value("event_name", "");
  if (new_event.event_name != target_event) {
    return false;
  }

  int limit = cond.value("limit", 5);
  using namespace sqlite_orm;
  auto count = db_.storage().count<AwsEvent>(
      where(c(&AwsEvent::event_name) == target_event &&
            c(&AwsEvent::source_ip_address) == new_event.source_ip_address &&
            c(&AwsEvent::event_time) >= (new_event.event_time - rule.time_window)));

  return count >= limit;
}

bool SocService::check_any_of_rule(const AwsEvent &new_event,
                                   const CorrelationRule &rule,
                                   const nlohmann::json &cond) {
  if (!cond.contains("events") || !cond["events"].is_array())
    return false;
  auto ev_list = cond["events"].get<std::vector<std::string>>();
  return std::find(ev_list.begin(), ev_list.end(), new_event.event_name) !=
         ev_list.end();
}

void SocService::generate_incident(const std::string &title,
                                   const std::string &severity,
                                   const std::string &threat_type,
                                   const std::string &actor,
                                   const std::string &description,
                                   const std::vector<AwsEvent> &triggering_events) {
  Incident inc;
  inc.description = description;
  inc.actor = actor;
  inc.threat = threat_type;
  inc.responsible = "security_team";
  inc.status = "open";

  int priority = 1; // info
  if (severity == "Critical")
    priority = 5;
  else if (severity == "High")
    priority = 4;
  else if (severity == "Medium")
    priority = 3;
  else if (severity == "Low")
    priority = 2;

  inc.priority = priority;
  inc.created_at = std::time(nullptr);
  inc.updated_at = inc.created_at;

  int incident_id = db_.storage().insert(inc);

  // Link evidences
  for (const auto &ev : triggering_events) {
    Evidence evd;
    evd.incident_id = incident_id;
    evd.aws_event_id = ev.event_id;
    evd.created_at = std::time(nullptr);
    evd.details = "Correlación automática: Evento " + ev.event_name +
                  " detectado de " + ev.source_ip_address;
    db_.storage().insert(evd);
  }

  // Create timeline
  IncidentTimeline t;
  t.incident_id = incident_id;
  t.event_time = std::time(nullptr);
  t.title = "Apertura Automática del Incidente";
  t.description = "Incidente de severidad " + severity +
                  " generado automáticamente por regla de correlación: " + title;
  t.created_at = std::time(nullptr);
  db_.storage().insert(t);

  // Dispatch notifications
  Notification n_int;
  n_int.incident_id = incident_id;
  n_int.type = "internal";
  n_int.recipient = "dashboard";
  n_int.subject = "ALERTA SOC: " + title;
  n_int.message = "Se ha generado automáticamente el incidente #" +
                  std::to_string(incident_id) + " (" + severity + ") debido a: " + description;
  n_int.status = "sent";
  n_int.created_at = std::time(nullptr);
  db_.storage().insert(n_int);

  Notification n_mail = n_int;
  n_mail.type = "email";
  n_mail.recipient = "soc_analyst@capitalone-lab.local";
  db_.storage().insert(n_mail);

  spdlog::warn("SOC ALERT: Incident #{} created automatically. Severity: {}. Rule: {}",
               incident_id, severity, title);
}

void SocService::trigger_simulation(const std::string &scenario) {
  auto now = std::time(nullptr);

  if (scenario == "capital_one") {
    // Phase 1: Attack scans security groups and metadata
    // Simulated EC2 AuthorizeSecurityGroupIngress opening up metadata access or port 80 to public
    nlohmann::json ev1 = {
        {"eventID", "ev-sim-c1-01"},
        {"eventName", "AuthorizeSecurityGroupIngress"},
        {"eventSource", "ec2.amazonaws.com"},
        {"eventTime", std::to_string(now - 40)},
        {"sourceIPAddress", "203.0.113.50"}, // Hacker IP
        {"userAgent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"},
        {"userIdentity", {{"type", "IAMUser"}, {"arn", "arn:aws:iam::111122223333:user/CloudAdmin"}}},
        {"requestParameters", {{"groupId", "sg-0123456"}, {"ipPermissions", {{"ipProtocol", "tcp"}, {"fromPort", 80}, {"toPort", 80}, {"ipRanges", {{"cidrIp", "0.0.0.0/0"}}}}}}}
    };
    ingest_event(ev1);

    // Phase 2: SSRF target request: Hacker obtains credentials for EC2 metadata profile
    nlohmann::json ev2 = {
        {"eventID", "ev-sim-c1-02"},
        {"eventName", "AssumeRole"},
        {"eventSource", "sts.amazonaws.com"},
        {"eventTime", std::to_string(now - 30)},
        {"sourceIPAddress", "203.0.113.50"}, // From Attacker public IP (SSRF leaked creds used externally)
        {"userAgent", "aws-cli/2.0.30 Python/3.7.4 Windows/10"},
        {"userIdentity", {{"type", "AssumedRole"}, {"arn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"sessionName", "i-0abcdef123456789a"}}},
        {"requestParameters", {{"roleArn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"roleSessionName", "i-0abcdef123456789a"}}}
    };
    ingest_event(ev2);

    // Phase 3: Enumeration
    nlohmann::json ev3 = {
        {"eventID", "ev-sim-c1-03"},
        {"eventName", "GetCallerIdentity"},
        {"eventSource", "sts.amazonaws.com"},
        {"eventTime", std::to_string(now - 20)},
        {"sourceIPAddress", "203.0.113.50"},
        {"userAgent", "aws-cli/2.0.30 Python/3.7.4 Windows/10"},
        {"userIdentity", {{"type", "AssumedRole"}, {"arn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"sessionName", "i-0abcdef123456789a"}, {"accessKeyId", "ASIAXXYYZZAA123"}}},
        {"requestParameters", {}}
    };
    ingest_event(ev3);

    nlohmann::json ev4 = {
        {"eventID", "ev-sim-c1-04"},
        {"eventName", "ListBuckets"},
        {"eventSource", "s3.amazonaws.com"},
        {"eventTime", std::to_string(now - 15)},
        {"sourceIPAddress", "203.0.113.50"},
        {"userAgent", "aws-cli/2.0.30 Python/3.7.4 Windows/10"},
        {"userIdentity", {{"type", "AssumedRole"}, {"arn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"sessionName", "i-0abcdef123456789a"}, {"accessKeyId", "ASIAXXYYZZAA123"}}},
        {"requestParameters", {}}
    };
    ingest_event(ev4);

    // Phase 4: S3 Sensitive Access & mass download
    nlohmann::json ev5 = {
        {"eventID", "ev-sim-c1-05"},
        {"eventName", "GetObject"},
        {"eventSource", "s3.amazonaws.com"},
        {"eventTime", std::to_string(now - 10)},
        {"sourceIPAddress", "203.0.113.50"},
        {"userAgent", "aws-cli/2.0.30 Python/3.7.4 Windows/10"},
        {"userIdentity", {{"type", "AssumedRole"}, {"arn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"sessionName", "i-0abcdef123456789a"}, {"accessKeyId", "ASIAXXYYZZAA123"}}},
        {"requestParameters", {{"bucketName", "capital-one-sensitive-data"}, {"key", "credit_cards_raw.csv"}}}
    };
    ingest_event(ev5);

    // Simulate mass download
    for (int i = 0; i < 5; ++i) {
      nlohmann::json ev_mass = {
          {"eventID", "ev-sim-c1-mass-" + std::to_string(i)},
          {"eventName", "GetObject"},
          {"eventSource", "s3.amazonaws.com"},
          {"eventTime", std::to_string(now - 5 + i)},
          {"sourceIPAddress", "203.0.113.50"},
          {"userAgent", "aws-cli/2.0.30 Python/3.7.4 Windows/10"},
          {"userIdentity", {{"type", "AssumedRole"}, {"arn", "arn:aws:iam::111122223333:role/EC2WafInstanceRole"}, {"sessionName", "i-0abcdef123456789a"}, {"accessKeyId", "ASIAXXYYZZAA123"}}},
          {"requestParameters", {{"bucketName", "capital-one-sensitive-data"}, {"key", "customer_records_part_" + std::to_string(i) + ".json"}}}
      };
      ingest_event(ev_mass);
    }
  } else if (scenario == "iam_backdoor") {
    // Phase 1: attacker creates user and key
    nlohmann::json ev1 = {
        {"eventID", "ev-sim-iam-01"},
        {"eventName", "CreateUser"},
        {"eventSource", "iam.amazonaws.com"},
        {"eventTime", std::to_string(now - 20)},
        {"sourceIPAddress", "198.51.100.12"},
        {"userAgent", "Boto3/1.12.0 Python/3.6.8"},
        {"userIdentity", {{"type", "IAMUser"}, {"arn", "arn:aws:iam::111122223333:user/Operator"}}},
        {"requestParameters", {{"userName", "aws-support-backdoor"}}}
    };
    ingest_event(ev1);

    nlohmann::json ev2 = {
        {"eventID", "ev-sim-iam-02"},
        {"eventName", "AttachUserPolicy"},
        {"eventSource", "iam.amazonaws.com"},
        {"eventTime", std::to_string(now - 15)},
        {"sourceIPAddress", "198.51.100.12"},
        {"userAgent", "Boto3/1.12.0 Python/3.6.8"},
        {"userIdentity", {{"type", "IAMUser"}, {"arn", "arn:aws:iam::111122223333:user/Operator"}}},
        {"requestParameters", {{"userName", "aws-support-backdoor"}, {"policyArn", "arn:aws:iam::aws:policy/AdministratorAccess"}}}
    };
    ingest_event(ev2);
  }
}
