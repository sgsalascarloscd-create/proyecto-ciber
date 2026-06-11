#pragma once
#include <string>
#include <vector>

class SecurityService {
public:
  // Secret for signing JWTs and Lambda payloads (can be overridden via environment variables)
  static std::string get_jwt_secret();
  static std::string get_webhook_secret();

  // JWT operations
  static std::string generate_jwt(const std::string &username, int expiry_seconds = 3600);
  static bool verify_jwt(const std::string &token, std::string &out_username);

  // Webhook signature verification
  static bool verify_webhook_signature(const std::string &payload, const std::string &signature_header);

  // General crypto utilities
  static std::string hmac_sha256(const std::string &data, const std::string &key);
  static std::string base64_url_encode(const std::string &data);
  static std::string base64_url_decode(const std::string &data);

private:
  static std::string clean_base64_padding(std::string str);
};
