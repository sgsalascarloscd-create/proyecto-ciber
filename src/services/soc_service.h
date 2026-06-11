#pragma once
#include "database.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

class SocService {
public:
  explicit SocService(Database &db);

  // Event Ingestion
  int ingest_event(const nlohmann::json &raw_event_json);
  
  // Fetch lists
  std::vector<AwsEvent> get_all_events();
  std::vector<CorrelationRule> get_all_rules();
  void toggle_rule(int id, bool active);
  void create_or_update_rule(const nlohmann::json &body);

  // Evidence & Timelines
  std::vector<Evidence> get_evidence_for_incident(int incident_id);
  std::vector<IncidentTimeline> get_timeline_for_incident(int incident_id);

  // Notifications
  std::vector<Notification> get_notifications();
  void mark_notifications_read();

  // Attack simulator helper
  void trigger_simulation(const std::string &scenario);

private:
  void run_correlation(const AwsEvent &new_event);
  
  // Specific correlation checkers
  bool check_sequence_rule(const AwsEvent &new_event, const CorrelationRule &rule, const nlohmann::json &cond);
  bool check_property_rule(const AwsEvent &new_event, const CorrelationRule &rule, const nlohmann::json &cond);
  bool check_threshold_rule(const AwsEvent &new_event, const CorrelationRule &rule, const nlohmann::json &cond);
  bool check_any_of_rule(const AwsEvent &new_event, const CorrelationRule &rule, const nlohmann::json &cond);

  // Auto creators
  void generate_incident(const std::string &title, const std::string &severity, 
                         const std::string &threat_type, const std::string &actor,
                         const std::string &description, const std::vector<AwsEvent> &triggering_events);

  Database &db_;
};
