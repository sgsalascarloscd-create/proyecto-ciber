#pragma once
#include <ctime>
#include <sqlite_orm/sqlite_orm.h>
#include <string>

struct AwsEvent {
  int id = 0;
  std::string event_id;
  std::string event_name;
  std::string event_source;
  std::time_t event_time = 0;
  std::string user_identity;
  std::string request_parameters;
  std::string response_elements;
  std::string source_ip_address;
  std::string user_agent;
  std::string raw_event;
  int processed = 0; // 0 = false, 1 = true
};

struct IndicatorOfCompromise {
  int id = 0;
  int incident_id = 0;
  std::string name;
  std::string description;
  std::string severity; // Low, Medium, High, Critical
  std::time_t detected_at = 0;
  std::string evidence_json;
};

struct CorrelationRule {
  int id = 0;
  std::string name;
  std::string description;
  std::string conditions; // JSON condition rules
  int time_window = 60;   // in seconds
  std::string severity;   // Low, Medium, High, Critical
  int active = 1;         // 0 = false, 1 = true
};

struct Evidence {
  int id = 0;
  int incident_id = 0;
  std::string aws_event_id;
  std::string details;
  std::time_t created_at = 0;
};

struct IncidentTimeline {
  int id = 0;
  int incident_id = 0;
  std::time_t event_time = 0;
  std::string title;
  std::string description;
  std::time_t created_at = 0;
};

struct Notification {
  int id = 0;
  int incident_id = 0;
  std::string recipient;
  std::string type; // "email", "internal"
  std::string subject;
  std::string message;
  std::string status; // "sent", "failed"
  std::time_t created_at = 0;
};

inline auto make_aws_event_table() {
  using namespace sqlite_orm;
  return make_table(
      "aws_events",
      make_column("id", &AwsEvent::id, primary_key().autoincrement()),
      make_column("event_id", &AwsEvent::event_id),
      make_column("event_name", &AwsEvent::event_name),
      make_column("event_source", &AwsEvent::event_source),
      make_column("event_time", &AwsEvent::event_time),
      make_column("user_identity", &AwsEvent::user_identity),
      make_column("request_parameters", &AwsEvent::request_parameters),
      make_column("response_elements", &AwsEvent::response_elements),
      make_column("source_ip_address", &AwsEvent::source_ip_address),
      make_column("user_agent", &AwsEvent::user_agent),
      make_column("raw_event", &AwsEvent::raw_event),
      make_column("processed", &AwsEvent::processed));
}

inline auto make_ioc_table() {
  using namespace sqlite_orm;
  return make_table(
      "indicators_of_compromise",
      make_column("id", &IndicatorOfCompromise::id, primary_key().autoincrement()),
      make_column("incident_id", &IndicatorOfCompromise::incident_id),
      make_column("name", &IndicatorOfCompromise::name),
      make_column("description", &IndicatorOfCompromise::description),
      make_column("severity", &IndicatorOfCompromise::severity),
      make_column("detected_at", &IndicatorOfCompromise::detected_at),
      make_column("evidence_json", &IndicatorOfCompromise::evidence_json));
}

inline auto make_correlation_rule_table() {
  using namespace sqlite_orm;
  return make_table(
      "correlation_rules",
      make_column("id", &CorrelationRule::id, primary_key().autoincrement()),
      make_column("name", &CorrelationRule::name),
      make_column("description", &CorrelationRule::description),
      make_column("conditions", &CorrelationRule::conditions),
      make_column("time_window", &CorrelationRule::time_window),
      make_column("severity", &CorrelationRule::severity),
      make_column("active", &CorrelationRule::active));
}

inline auto make_evidence_table() {
  using namespace sqlite_orm;
  return make_table(
      "evidence",
      make_column("id", &Evidence::id, primary_key().autoincrement()),
      make_column("incident_id", &Evidence::incident_id),
      make_column("aws_event_id", &Evidence::aws_event_id),
      make_column("details", &Evidence::details),
      make_column("created_at", &Evidence::created_at));
}

inline auto make_timeline_table() {
  using namespace sqlite_orm;
  return make_table(
      "incident_timeline",
      make_column("id", &IncidentTimeline::id, primary_key().autoincrement()),
      make_column("incident_id", &IncidentTimeline::incident_id),
      make_column("event_time", &IncidentTimeline::event_time),
      make_column("title", &IncidentTimeline::title),
      make_column("description", &IncidentTimeline::description),
      make_column("created_at", &IncidentTimeline::created_at));
}

inline auto make_notification_table() {
  using namespace sqlite_orm;
  return make_table(
      "notifications",
      make_column("id", &Notification::id, primary_key().autoincrement()),
      make_column("incident_id", &Notification::incident_id),
      make_column("recipient", &Notification::recipient),
      make_column("type", &Notification::type),
      make_column("subject", &Notification::subject),
      make_column("message", &Notification::message),
      make_column("status", &Notification::status),
      make_column("created_at", &Notification::created_at));
}
